#include <pfaedle/osm/bounding_box.h>
#include <util/Misc.h>


#include <osmium/handler.hpp>
#include <osmium/visitor.hpp>
#include <osmium/io/pbf_input.hpp>
#include <osmium/io/xml_input.hpp>
#include <osmium/io/reader.hpp>

#ifdef __linux__
// Utility class gives us access to memory usage information
#include <osmium/util/memory.hpp>
#endif
#include <osmium/geom/haversine.hpp>
#include <osmium/geom/mercator_projection.hpp>

#include "osm_reader.h"

namespace pfaedle::osm
{
    using util::geo::webMercMeterDist;
    using util::geo::Point;

    const static size_t RELATION_INDEX = 2;
const static size_t WAY_INDEX = 1;
const static size_t NODE_INDEX = 0;

/**
     * @return the speed in km/h
     */
inline double string_to_kmh(std::string str)
{
    /**
         * The speed value used for "none" speed limit on German Autobahn is 150=30*5 as this is the biggest value
         * not explicitly used in OSM and can be precisely returned for a factor of 5, 3, 2 and 1. It is fixed and
         * not DecimalEncodedValue.getMaxInt to allow special case handling.
         */
    static constexpr double UNLIMITED_SIGN_SPEED = 150;

    if (str.empty() || util::trim(str).empty())
        return 50;

    // on some German autobahns and a very few other places
    if ("none" == str)
        return UNLIMITED_SIGN_SPEED;

    if (util::ends_with(str, ":motorway"))
        return 100;

    if (util::ends_with(str, ":rural") || util::ends_with(str, ":trunk"))
        return 80;

    if (util::ends_with(str, ":urban"))
        return 50;

    if (str == "walk" || util::ends_with(str, ":living_street"))
        return 6;

    size_t mpInteger = str.find("mp");
    size_t knotInteger = str.find("knots");
    size_t kmInteger = str.find("km");
    size_t kphInteger = str.find("kph");

    double factor;
    if (mpInteger != std::string::npos)
    {
        str = util::trim(str.substr(0, mpInteger));
        // factor = DistanceCalcEarth.KM_MILE;
        factor = 1.609344f;
    }
    else if (knotInteger != std::string::npos)
    {
        str = util::trim(str.substr(0, knotInteger));
        factor = 1.852;// see https://en.wikipedia.org/wiki/Knot_%28unit%29#Definitions
    }
    else
    {
        if (kmInteger != std::string::npos)
        {
            str = util::trim(str.substr(0, kmInteger));
        }
        else if (kphInteger != std::string::npos)
        {
            str = util::trim(str.substr(0, kphInteger));
        }
        factor = 1;
    }

    double value = 0;
    try
    {
        value = stoi(str) * factor;
    }
    catch (std::exception& e)
    {
    }

    if (value <= 0)
    {
        return 50;
    }

    return value;
}


class pfaedle_osmium_handler : public osmium::handler::Handler
{
public:
    explicit pfaedle_osmium_handler(osm_reader& reader) :
        reader{reader}
    {
    }

protected:
    osm_reader& reader;
};

class relation_handler : public pfaedle_osmium_handler
{
public:
    explicit relation_handler(osm_reader& reader, const bounding_box& bbox) :
        pfaedle_osmium_handler{reader},
        bbox{bbox}
    {
    }

    void node(const osmium::Node& node)
    {
        osmid cur_id = 0;

        double y = node.location().lat();
        double x = node.location().lon();

        if (bbox.contains(Point(x, y)))
        {
            cur_id = node.id();
            reader.usable_node_set.add(cur_id);
        }

        if (cur_id == 0)
            return;

        for (const auto& tag : node.tags())
        {
            if (reader.filter.nohup(tag.key(), tag.value()))
            {
                reader.restricted_node_set.add(cur_id);
            }
        }
    }

    void relation(const osmium::Relation& relation)
    {
        osm_relation rel;

        rel.id = relation.id();
        // processing attributes
        {
            uint64_t keep_flags = 0;
            uint64_t drop_flags = 0;
            for (const auto& tag : relation.tags())
            {
                if (reader.kept_attribute_keys[RELATION_INDEX].count(tag.key()))
                {
                    rel.attrs[tag.key()] = tag.value();
                }
            }

            if (rel.id && !rel.attrs.empty() &&
                (keep_flags = reader.filter.keep(rel.attrs, osm_filter::REL)) &&
                !(drop_flags = reader.filter.drop(rel.attrs, osm_filter::REL)))
            {
                rel.keepFlags = keep_flags;
                rel.dropFlags = drop_flags;
            }

            reader.intermodal_rels.attributes.emplace_back(rel.attrs);
        }

        // processing members
        for (const auto& member : relation.members())
        {
            switch (member.type())
            {
                case osmium::item_type::node:
                    rel.nodes.emplace_back(member.ref());
                    rel.nodeRoles.emplace_back(member.role());
                    break;
                case osmium::item_type::way:
                    rel.ways.emplace_back(member.ref());
                    rel.wayRoles.emplace_back(member.role());
                    break;
                default:
                    break;
            }
        }

        if (rel.keepFlags & osm::REL_NO_DOWN)
        {
            reader.intermodal_rels.flat.insert(reader.intermodal_rels.attributes.size() - 1);
        }
        for (osmid id : rel.nodes)
        {
            reader.node_rels[id].push_back(reader.intermodal_rels.attributes.size() - 1);
        }
        for (osmid id : rel.ways)
        {
            reader.way_rels[id].push_back(reader.intermodal_rels.attributes.size() - 1);
        }

        // processing restrictions
        {

            if (!rel.attrs.count("type") || rel.attrs.find("type")->second != "restriction")
                return;

            bool pos = reader.filter.posRestr(rel.attrs);
            bool neg = reader.filter.negRestr(rel.attrs);

            if (!pos && !neg)
                return;

            osmid from = 0;
            osmid to = 0;
            osmid via = 0;

            for (size_t i = 0; i < rel.ways.size(); i++)
            {
                if (rel.wayRoles[i] == "from")
                {
                    if (from) return;// only one from member supported
                    from = rel.ways[i];
                }
                if (rel.wayRoles[i] == "to")
                {
                    if (to) return;// only one to member supported
                    to = rel.ways[i];
                }
            }

            for (size_t i = 0; i < rel.nodes.size(); i++)
            {
                if (rel.nodeRoles[i] == "via")
                {
                    via = rel.nodes[i];
                    break;
                }
            }

            if (from && to && via)
            {
                if (pos)
                {
                    reader.raw_rests.pos[via].emplace_back(from, to);
                }
                else if (neg)
                {
                    reader.raw_rests.neg[via].emplace_back(from, to);
                }
            }
        }
    }

private:
    const bounding_box& bbox;
};

class way_handler : public pfaedle_osmium_handler
{
public:
    explicit way_handler(osm_reader& reader) :
        pfaedle_osmium_handler{reader}
    {
    }

    void way(const osmium::Way& way)
    {
        osm_way w;
        w.id = way.id();
        for (const auto& nd : way.nodes())
        {
            osmid node_id = nd.ref();
            w.nodes.emplace_back(node_id);
        }

        for (const auto& tag : way.tags())
        {
            const auto& key = tag.key();
            const auto& val = tag.value();
            if (reader.kept_attribute_keys[WAY_INDEX].count(key))
            {
                w.attrs[key] = val;
            }
        }

        if (w.keep_way(reader.way_rels, reader.filter, reader.usable_node_set, reader.intermodal_rels.flat))
        {
            trgraph::node* last = nullptr;
            std::vector<trgraph::transit_edge_line*> lines;
            if (reader.way_rels.count(w.id))
            {
                lines = reader.get_lines(reader.way_rels.find(w.id)->second, reader.intermodal_rels,
                                         reader.configuration.options);
            }
            std::string track =
                    pfaedle::osm::osm_reader::get_attribute_by_first_match(
                            reader.configuration.options.edgePlatformRules,
                            w.id,
                            w.attrs,
                            reader.way_rels,
                            reader.intermodal_rels,
                            reader.configuration.options.trackNormzer);

            osmid lastnid = 0;
            for (osmid nid : w.nodes)
            {
                trgraph::node* n = nullptr;
                if (reader.restricted_node_set.has(nid))
                {
                    n = reader.configuration.graph.addNd();
                    reader.nodes_multi_map[nid].insert(n);
                }
                else if (!reader.nodes.count(nid))
                {
                    if (!reader.usable_node_set.has(nid)) continue;
                    n = reader.configuration.graph.addNd();
                    reader.nodes[nid] = n;
                }
                else
                {
                    n = reader.nodes[nid];
                }

                if (last)
                {
                    auto e = reader.configuration.graph.addEdg(last, n, trgraph::edge_payload());
                    if (!e) continue;

                    reader.process_restrictions(nid, w.id, e, n);
                    reader.process_restrictions(lastnid, w.id, e, last);

                    e->pl().add_lines(lines);
                    e->pl().set_level(reader.filter.level(w.attrs));
                    e->pl().set_max_speed(string_to_kmh(w.attrs["maxspeed"]));
                    if (!track.empty()) reader.e_tracks[e] = track;

                    if (reader.filter.oneway(w.attrs)) e->pl().setOneWay(1);
                    if (reader.filter.onewayrev(w.attrs)) e->pl().setOneWay(2);
                }
                lastnid = nid;
                last = n;
            }
        }
    }
};

class node_handler : public pfaedle_osmium_handler
{
public:
    explicit node_handler(osm_reader& reader) :
        pfaedle_osmium_handler{reader}
    {
    }

    void node(const osmium::Node& node)
    {
        station_attribute_groups attr_groups;

        osm_node nd;
        nd.lat = node.location().lat();
        nd.lng = node.location().lon();
        nd.id = node.id();

        for (const auto& tag : node.tags())
        {
            const auto& key = tag.key();
            const auto& val = tag.value();

            if (reader.kept_attribute_keys[NODE_INDEX].count(key))
                nd.attrs[key] = val;
        }

        if (nd.keep_node(reader.nodes, reader.nodes_multi_map, reader.node_rels, reader.usable_node_set,
                         reader.filter, reader.intermodal_rels.flat))
        {
            auto pos = util::geo::latLngToWebMerc(nd.lat, nd.lng);
            if (reader.nodes.count(nd.id))
            {
                trgraph::node* n = reader.nodes[nd.id];
                n->pl().set_geom(pos);
                if (reader.filter.station(nd.attrs))
                {
                    auto si = pfaedle::osm::osm_reader::get_station_info(n, nd.id, pos, nd.attrs, &attr_groups,
                                                                         reader.node_rels,
                                                                         reader.intermodal_rels,
                                                                         reader.configuration.options);
                    if (si.has_value())
                    {
                        n->pl().set_si(si.value());
                    }
                }
                else if (reader.filter.blocker(nd.attrs))
                {
                    n->pl().set_blocker();
                }
            }
            else if (reader.nodes_multi_map.count(nd.id))
            {
                for (auto* n : reader.nodes_multi_map[nd.id])
                {
                    n->pl().set_geom(pos);
                    if (reader.filter.station(nd.attrs))
                    {
                        auto si = pfaedle::osm::osm_reader::get_station_info(n, nd.id, pos, nd.attrs, &attr_groups,
                                                                             reader.node_rels,
                                                                             reader.intermodal_rels,
                                                                             reader.configuration.options);
                        if (si.has_value())
                        {
                            n->pl().set_si(si.value());
                        }
                    }
                    else if (reader.filter.blocker(nd.attrs))
                    {
                        n->pl().set_blocker();
                    }
                }
            }
            else
            {
                // these are nodes without any connected edges
                if (reader.filter.station(nd.attrs))
                {
                    auto tmp = reader.configuration.graph.addNd(trgraph::node_payload(pos));
                    auto si = pfaedle::osm::osm_reader::get_station_info(tmp, nd.id, pos, nd.attrs, &attr_groups,
                                                                         reader.node_rels,
                                                                         reader.intermodal_rels,
                                                                         reader.configuration.options);

                    if (si.has_value())
                    {
                        tmp->pl().set_si(si.value());
                    }

                    if (tmp->pl().get_si())
                    {
                        tmp->pl().get_si()->set_is_from_osm(false);
                        reader.orphan_stations.insert(tmp);
                    }
                }
            }
        }
    }
};

osm::osm_reader::osm_reader(const osm_reader::read_configuration& configuration) :
    configuration{configuration},
    kept_attribute_keys{configuration.options.get_kept_attribute_keys()},
    filter{configuration.options}
{
}

void osm_reader::read(const std::string& path, const bounding_box& bbox)
{
    if (!bbox.size())
        return;

    LOG(TRACE) << "Reading bounding box nodes...";
    LOG(TRACE) << "Reading relations...";
    // read relations and nodes filtering
    osmium::io::Reader reader_pass1{path, osmium::osm_entity_bits::node | osmium::osm_entity_bits::relation};
    osmium::apply(reader_pass1, relation_handler(*this, bbox));
    reader_pass1.close();

    LOG(TRACE) << "Reading edges...";
    // read ways
    osmium::io::Reader reader_pass2{path, osmium::osm_entity_bits::way};
    osmium::apply(reader_pass2, way_handler(*this));
    reader_pass2.close();

    LOG(TRACE) << "Reading kept nodes...";
    // read nodes
    osmium::io::Reader reader_pass3{path, osmium::osm_entity_bits::node};
    osmium::apply(reader_pass3, node_handler(*this));
    reader_pass3.close();


    LOG(TRACE) << "OSM ID set lookups: " << osm::osm_id_set::LOOKUPS
               << ", file lookups: " << osm::osm_id_set::FLOOKUPS;

    LOG(TRACE) << "Applying edge track numbers...";
    for (const auto& tr : e_tracks)
    {
        if (tr.first->getTo()->pl().get_si() &&
            tr.first->getTo()->pl().get_si()->get_track().empty())
        {
            tr.first->getTo()->pl().get_si()->set_track(tr.second);
        }
        if (tr.first->getFrom()->pl().get_si() &&
            tr.first->getFrom()->pl().get_si()->get_track().empty())
        {
            tr.first->getFrom()->pl().get_si()->set_track(tr.second);
        }
    }
    e_tracks.clear();

#ifdef __linux__
    // Because of the huge amount of OSM data, some Osmium-based programs
    // (though not this one) can use huge amounts of data. So checking actual
    // memore usage is often useful and can be done easily with this class.
    // (Currently only works on Linux, not macOS and Windows.)
    osmium::MemoryUsage memory;

    LOG_INFO() << "Memory used: " << memory.peak() << " MBytes";
#endif
}


std::optional<trgraph::station_info> osm_reader::get_station_info(trgraph::node* node, osmid nid, const POINT& pos, const attribute_map& m,
                                                                  station_attribute_groups* groups, const relation_map& nodeRels,
                                                                  const relation_list& rels, const osm_read_options& ops)
{
    std::string platform;
    std::vector<std::string> names;

    names = get_matching_attributes_ranked(ops.statAttrRules.nameRule, nid, m, nodeRels, rels,
                                           ops.statNormzer);
    platform = get_attribute_by_first_match(ops.statAttrRules.platformRule, nid, m,
                                            nodeRels, rels, ops.trackNormzer);

    if (names.empty())
        return std::nullopt;

    trgraph::station_info ret(names[0], platform, true);

#ifdef PFAEDLE_STATION_IDS
    ret.set_id(get_attribute_by_first_match(ops.statAttrRules.idRule, nid, m, nodeRels,
                                            attributes, ops.idNormzer));
#endif

    for (size_t i = 1; i < names.size(); i++) ret.add_alternative_name(names[i]);

    bool group_found = false;

    for (const auto& rule : ops.statGroupNAttrRules)
    {
        if (group_found) break;
        std::string rule_val = get_attribute(rule.attr, nid, m, nodeRels, rels);
        if (!rule_val.empty())
        {
            // check if a matching group exists
            for (auto* group : (*groups)[rule.attr.attr][rule_val])
            {
                if (group_found) break;
                for (const auto* member : group->get_nodes())
                {
                    if (webMercMeterDist(*member->pl().get_geom(), pos) <= rule.maxDist)
                    {
                        // ok, group is matching
                        group_found = true;
                        if (node) group->add_node(node);
                        ret.set_group(group);
                        break;
                    }
                }
            }
        }
    }

    if (!group_found)
    {
        for (const auto& rule : ops.statGroupNAttrRules)
        {
            std::string rule_val = get_attribute(rule.attr, nid, m, nodeRels, rels);
            if (!rule_val.empty())
            {
                // add new group
                auto* g = new trgraph::station_group();
                if (node)
                    g->add_node(node);
                ret.set_group(g);
                (*groups)[rule.attr.attr][rule_val].push_back(g);
                break;
            }
        }
    }

    return ret;
}

std::string osm_reader::get_attribute(const deep_attribute_rule& s, osmid id, const attribute_map& attrs,
                                      const relation_map& entRels, const relation_list& rels)
{
    if (s.relRule.kv.first.empty())
    {
        if (attrs.find(s.attr) != attrs.end())
        {
            return attrs.find(s.attr)->second;
        }
    }
    else
    {
        if (entRels.count(id))
        {
            for (const auto& rel_id : entRels.find(id)->second)
            {
                if (osm_filter::contained(rels.attributes[rel_id], s.relRule.kv))
                {
                    if (rels.attributes[rel_id].count(s.attr))
                    {
                        return rels.attributes[rel_id].find(s.attr)->second;
                    }
                }
            }
        }
    }
    return "";
}

std::string osm_reader::get_attribute_by_first_match(const deep_attribute_list& rule, osmid id, const attribute_map& attrs,
                                                     const relation_map& entRels, const relation_list& rels,
                                                     const trgraph::normalizer& normzer)
{
    std::string ret;
    for (const auto& s : rule)
    {
        const auto& attribute = get_attribute(s, id, attrs, entRels, rels);
        ret = normzer.norm(attribute);
        if (!ret.empty())
        {
            return ret;
        }
    }
    return ret;
}

std::vector<std::string> osm_reader::get_matching_attributes_ranked(const deep_attribute_list& rule, osmid id, const attribute_map& attrs,
                                                                    const relation_map& entRels, const relation_list& rels,
                                                                    const trgraph::normalizer& normalizer)
{
    std::vector<std::string> ret;
    for (const auto& s : rule)
    {
        const auto& attribute = get_attribute(s, id, attrs, entRels, rels);
        const auto& tmp = normalizer.norm(attribute);
        if (!tmp.empty())
        {
            ret.emplace_back(tmp);
        }
    }
    return ret;
}

std::vector<trgraph::transit_edge_line*> osm_reader::get_lines(const std::vector<size_t>& edgeRels, const relation_list& rels, const osm_read_options& ops)
{
    std::vector<trgraph::transit_edge_line*> ret;
    for (size_t rel_id : edgeRels)
    {
        trgraph::transit_edge_line* elp;

        if (_relLines.count(rel_id))
        {
            elp = _relLines[rel_id];
        }
        else
        {
            trgraph::transit_edge_line el;

            bool found = false;
            for (const auto& r : ops.relLinerules.sNameRule)
            {
                for (const auto& rel_attr : rels.attributes[rel_id])
                {
                    if (rel_attr.first == r)
                    {
                        el.shortName = ops.lineNormzer.norm(rel_attr.second);
                        //ops.lineNormzer.norm(pfxml::file::decode(relAttr.second));
                        if (!el.shortName.empty())
                            found = true;
                    }
                }
                if (found) break;
            }

            found = false;
            for (const auto& r : ops.relLinerules.fromNameRule)
            {
                for (const auto& rel_attr : rels.attributes[rel_id])
                {
                    if (rel_attr.first == r)
                    {
                        el.fromStr = ops.lineNormzer.norm(rel_attr.second);
                        if (!el.fromStr.empty())
                            found = true;
                    }
                }
                if (found) break;
            }

            found = false;
            for (const auto& r : ops.relLinerules.toNameRule)
            {
                for (const auto& rel_attr : rels.attributes[rel_id])
                {
                    if (rel_attr.first == r)
                    {
                        el.toStr = ops.lineNormzer.norm(rel_attr.second);
                        if (!el.toStr.empty()) found = true;
                    }
                }
                if (found) break;
            }

            if (el.shortName.empty() && el.fromStr.empty() && el.toStr.empty())
                continue;

            if (_lines.count(el))
            {
                elp = _lines[el];
                _relLines[rel_id] = elp;
            }
            else
            {
                elp = new trgraph::transit_edge_line(el);
                _lines[el] = elp;
                _relLines[rel_id] = elp;
            }
        }
        ret.push_back(elp);
    }
    return ret;
}

void osm_reader::process_restrictions(osmid nid, osmid wid, trgraph::edge* e, trgraph::node* n) const
{
    if (raw_rests.pos.count(nid))
    {
        for (const auto& kv : raw_rests.pos.find(nid)->second)
        {
            if (kv.eFrom == wid)
            {
                e->pl().set_restricted();
                configuration.restrictor.add(e, kv.eTo, n, true);
            }
            else if (kv.eTo == wid)
            {
                e->pl().set_restricted();
                configuration.restrictor.relax(wid, n, e);
            }
        }
    }

    if (raw_rests.neg.count(nid))
    {
        for (const auto& kv : raw_rests.neg.find(nid)->second)
        {
            if (kv.eFrom == wid)
            {
                e->pl().set_restricted();
                configuration.restrictor.add(e, kv.eTo, n, false);
            }
            else if (kv.eTo == wid)
            {
                e->pl().set_restricted();
                configuration.restrictor.relax(wid, n, e);
            }
        }
    }
}

}