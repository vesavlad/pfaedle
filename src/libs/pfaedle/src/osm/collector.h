#ifndef CONFIG_FILE_PARSER_H_COLLECTOR_H
#define CONFIG_FILE_PARSER_H_COLLECTOR_H

#include <pfaedle/osm/osm_read_options.h>
#include <pfaedle/osm/bounding_box.h>
#include <pfaedle/osm/osm_filter.h>
#include <pfaedle/osm/osm_id_set.h>

#include <util/geo/Point.h>

#include <osmium/osm/node.hpp>
#include <osmium/osm/way.hpp>
#include <osmium/osm/relation.hpp>
#include <osmium/handler.hpp>
#include <osmium/visitor.hpp>
#include <osmium/memory/buffer.hpp>
#include <osmium/relations/detail/member_meta.hpp>
#include <osmium/relations/detail/relation_meta.hpp>
#include <osmium/util/iterator.hpp>
#include <osmium/handler/check_order.hpp>

#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <pfaedle/router/misc.h>

namespace pfaedle::osm
{

struct collector
{

    struct data
    {
    public:
        data(const osm_read_options &read_options,
             const bounding_box &bounding_box,
             trgraph::restrictor &restrictor,
             trgraph::graph &graph) :
                read_options(read_options),
                bounding_box{bounding_box},
                restrictor{restrictor},
                graph{graph},
                attr_keys{read_options.get_kept_attribute_keys()},
                filter{read_options}
        {}

        const osm_read_options &read_options;
        const bounding_box &bounding_box;
        trgraph::restrictor &restrictor;
        trgraph::graph &graph;

        std::vector<attribute_key_set> attr_keys;
        osm_filter filter;

        edge_tracks tracks;

        osm_id_set bboxNodes;
        osm_id_set noHupNodes;

        node_id_map nodes;
        node_id_multimap mult_nodes;
        relation_list intermodal_relations;
        relation_map node_relations;
        relation_map way_relations;
        router::node_set orphan_stations;

        restrictions raw_rests;
    };

    /**
     * This is the handler class for the first pass of the Collector.
     */
    class handler_pass1 : public osmium::handler::Handler
    {

        data& d;

    public:
        explicit handler_pass1(data& d) noexcept :
            d(d)
        {
        }

        void node(const osmium::Node& node)
        {
            osmid cur_id = 0;
            util::geo::Point p(node.location().lon(), node.location().lat());
            if (d.bounding_box.contains(p))
            {
                cur_id = node.id();
                d.bboxNodes.add(cur_id);
            }

            if (cur_id == 0)
                return;

            for (const auto& tag : node.tags())
            {
                if (d.filter.nohup(tag.key(), tag.value()))
                {
                    d.noHupNodes.add(cur_id);
                }
            }
        }

        void route(const osmium::Relation& relation)
        {
        }
    };

    /**
     * This is the handler class for the second pass of the Collector.
     */
    class handler_pass2 : public osmium::handler::Handler
    {

        osmium::handler::CheckOrder m_check_order;
        collector& m_collector;

    public:
        explicit handler_pass2(collector& collector) noexcept :
            m_collector(collector)
        {
        }

        void node(const osmium::Node& node)
        {
            m_check_order.node(node);
            if (!m_collector.find_and_add_object(node))
            {
                m_collector.node_not_in_any_relation(node);
            }
        }

        void way(const osmium::Way& way)
        {
            m_check_order.way(way);
            if (!m_collector.find_and_add_object(way))
            {
                m_collector.way_not_in_any_relation(way);
            }
        }

        void relation(const osmium::Relation& relation)
        {
#if 0
            osm_relation rel;
            uint64_t keep_flags = 0;
            uint64_t drop_flags = 0;

            rel.id = relation.id();
            // processing attributes
            {
                for (const auto& tag : relation.tags())
                {
                    const auto& key = tag.key();
                    const auto& value = tag.value();
                    if (keepAttrs.count(key))
                    {
                        rel.attrs[key] = value;
                    }
                }

                if (rel.id && !rel.attrs.empty() &&
                    (keep_flags = filter.keep(rel.attrs, osm_filter::REL)) &&
                    !(drop_flags = filter.drop(rel.attrs, osm_filter::REL)))
                {
                    rel.keepFlags = keep_flags;
                    rel.dropFlags = drop_flags;
                }

                attributes.attributes.emplace_back(rel.attrs);
            }

            // processing members
            for (const auto& member : relation.members())
            {
                const auto& type = member.type();
                if (type == osmium::item_type::node)
                {
                    osmid id = member.ref();
                    rel.nodes.emplace_back(id);
                    rel.nodeRoles.emplace_back(member.role());
                }
                if (type == osmium::item_type::way)
                {
                    osmid id = member.ref();
                    rel.ways.emplace_back(id);
                    rel.wayRoles.emplace_back(member.role());
                }
            }

            if (rel.keepFlags & osm::REL_NO_DOWN)
            {
                attributes.flat.insert(attributes.attributes.size() - 1);
            }
            for (osmid id : rel.nodes)
            {
                nodeRels[id].push_back(attributes.attributes.size() - 1);
            }
            for (osmid id : rel.ways)
            {
                wayRels[id].push_back(attributes.attributes.size() - 1);
            }

            // TODO(patrick): this is not needed for the filtering - remove it here!
            read_restrictions(rel, restrictions, filter);

#endif
            m_check_order.relation(relation);
            if (!m_collector.find_and_add_object(relation))
            {
                m_collector.relation_not_in_any_relation(relation);
            }
        }

        void flush()
        {
            m_collector.flush();
        }

    private:
        void read_restrictions(const osm_relation& rel,
                               restrictions& rests,
                               const osm_filter& filter) const
        {
            if (!rel.attrs.count("type") || rel.attrs.find("type")->second != "restriction")
                return;

            bool pos = filter.posRestr(rel.attrs);
            bool neg = filter.negRestr(rel.attrs);

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
                    rests.pos[via].emplace_back(from, to);
                else if (neg)
                    rests.neg[via].emplace_back(from, to);
            }
        }

    };


    data& d;

    handler_pass2 m_handler_pass2;

    // All relations we are interested in will be kept in this buffer
    osmium::memory::Buffer m_relations_buffer;

    // All members we are interested in will be kept in this buffer
    osmium::memory::Buffer m_members_buffer;

    /// Vector with all relations we are interested in
    std::vector<osmium::relations::RelationMeta> m_relations;

    /**
     * One vector each for nodes, ways, and relations containing all
     * mappings from member ids to their relations.
     */
    using mm_vector_type = std::vector<osmium::relations::MemberMeta>;
    using mm_iterator = mm_vector_type::iterator;
    std::array<mm_vector_type, 3> m_member_meta;

    int m_count_complete = 0;

    enum
    {
        initial_buffer_size = 1024UL * 1024UL
    };

    osmium::iterator_range<mm_iterator> find_member_meta(osmium::item_type type, osmium::object_id_type id)
    {
        auto& mmv = member_meta(type);
        return osmium::make_range(std::equal_range(mmv.begin(), mmv.end(), osmium::relations::MemberMeta(id)));
    }

public:
    /**
     * Create an Collector.
     */
    collector(data& d);

protected:
    std::vector<osmium::relations::MemberMeta>& member_meta(const osmium::item_type type);

    const std::vector<osmium::relations::RelationMeta>& relations() const;

    /**
     * This method is called from the first pass handler for every
     * relation in the input, to check whether it should be kept.
     *
     * Overwrite this method in a child class to only add relations
     * you are interested in, for instance depending on the type tag.
     * Storing relations takes a lot of memory, so it makes sense to
     * filter this as much as possible.
     */
    bool keep_relation(const osmium::Relation& /*relation*/) const;

    /**
     * This method is called for every member of every relation that
     * should be kept. It should decide if the member is interesting or
     * not and return true or false to signal that. Only interesting
     * members are later added to the relation.
     *
     * Overwrite this method in a child class. In the MultiPolygonCollector
     * this is for instance used to only keep members of type way and
     * ignore all others.
     */
    bool keep_member(const osmium::relations::RelationMeta& /*relation_meta*/, const osmium::RelationMember& /*member*/) const;

    /**
     * This method is called for all nodes that are not a member of
     * any relation.
     *
     * Overwrite this method in a child class if you are interested
     * in this.
     */
    void node_not_in_any_relation(const osmium::Node& /*node*/);

    /**
     * This method is called for all ways that are not a member of
     * any relation.
     *
     * Overwrite this method in a child class if you are interested
     * in this.
     */
    void way_not_in_any_relation(const osmium::Way& /*way*/);

    /**
     * This method is called for all relations that are not a member of
     * any relation.
     *
     * Overwrite this method in a child class if you are interested
     * in this.
     */
    void relation_not_in_any_relation(const osmium::Relation& /*relation*/);

    /**
     * This method is called from the 2nd pass handler when all objects
     * of types we are interested in have been seen.
     *
     * Overwrite this method in a child class if you are interested
     * in this.
     *
     * Note that even after this call members might be missing if they
     * were not in the input file! The derived class has to handle this
     * case.
     */
    void flush();

    const osmium::Relation& get_relation(size_t offset) const;

    /**
     * Get the relation from a relation_meta.
     */
    const osmium::Relation& get_relation(const osmium::relations::RelationMeta& relation_meta) const;

    /**
     * Get the relation from a member_meta.
     */
    const osmium::Relation& get_relation(const osmium::relations::MemberMeta& member_meta) const;

    osmium::OSMObject& get_member(size_t offset) const;

private:
    /**
     * Tell the Collector that you are interested in this relation
     * and want it kept until all members have been assembled and
     * it is handed back to you.
     *
     * The relation is copied and stored in a buffer inside the
     * collector.
     */
    void add_relation(const osmium::Relation& relation);

    /**
     * Sort the vectors with the member infos so that we can do binary
     * search on them.
     */
    void sort_member_meta();

    static typename osmium::iterator_range<mm_iterator>::iterator::difference_type count_not_removed(const osmium::iterator_range<mm_iterator>& range)
    {
        return std::count_if(range.begin(), range.end(), [](osmium::relations::MemberMeta& mm) {
            return !mm.removed();
        });
    }

    /**
     * Find this object in the member vectors and add it to all
     * relations that need it.
     *
     * @returns true if the member was added to at least one
     *          relation and false otherwise
     */
    bool find_and_add_object(const osmium::OSMObject& object);

    void clear_member_metas(const osmium::relations::RelationMeta& relation_meta);

public:
    uint64_t used_memory() const;


    osmium::memory::Buffer& members_buffer();

    /**
     * Is the given member available in the members buffer?
     *
     * If you also need the offset of the object, use
     * get_availability_and_offset() instead, it is more efficient
     * that way.
     *
     * @param type Item type
     * @param id Object Id
     * @returns True if the object is available, false otherwise.
     */
    bool is_available(osmium::item_type type, osmium::object_id_type id);

    /**
     * Get offset of a member in the members buffer.
     *
     * @pre The member must be available. If you are not sure, call
     *      get_availability_and_offset() instead.
     * @param type Item type
     * @param id Object Id
     * @returns The offset of the object in the members buffer.
     */
    size_t get_offset(osmium::item_type type, osmium::object_id_type id);

    /**
     * Checks whether a member is available in the members buffer
     * and returns its offset.
     *
     * If the member is not available, the boolean returned as the
     * first element in the pair is false. In that case the offset
     * in the second element is undefined.
     *
     * If the member is available, the boolean returned as the first
     * element in the pair is true and the second element of the
     * pair contains the offset into the members buffer.
     *
     * @param type Item type
     * @param id Object Id
     * @returns Pair of bool (showing availability) and the offset.
     */
    std::pair<bool, size_t> get_availability_and_offset(osmium::item_type type, osmium::object_id_type id);


    template<typename TIter>
    void collect(TIter begin, TIter end)
    {
        handler_pass1 handler(data);
        osmium::apply(begin, end, handler);
        sort_member_meta();
    }

    template<typename TSource>
    void collect(TSource& source)
    {
        using std::begin;
        using std::end;
        collect(begin(source), end(source));

        source.close();
    }

    void moving_in_buffer(size_t old_offset, size_t new_offset);

    /**
     * Decide whether to purge removed members and then do it.
     *
     * Currently the purging is done every 10000 calls.
     * This could probably be improved upon.
     */
    void possibly_purge_removed_members();

    /**
     * Get a vector with pointers to all Relations that could not
     * be completed, because members were missing in the input
     * data.
     *
     * Note that these pointers point into memory allocated and
     * owned by the Collector object.
     */
    std::vector<const osmium::Relation*> get_incomplete_relations() const;

};// class collector
}

#endif//CONFIG_FILE_PARSER_H_COLLECTOR_H
