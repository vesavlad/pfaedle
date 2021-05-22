#include "collector.h"

namespace pfaedle::osm
{
collector::collector(data& d) :
    d(d),
//    read_options_{read_options},
//    bounding_box_{bounding_box},
//    restrictor_{restrictor},
//    attr_keys_{read_options.get_kept_attribute_keys()},
//    filter_{read_options},
    m_handler_pass2(*this),
    m_relations_buffer(initial_buffer_size, osmium::memory::Buffer::auto_grow::yes),
    m_members_buffer(initial_buffer_size, osmium::memory::Buffer::auto_grow::yes)
{
}
std::vector<osmium::relations::MemberMeta>& collector::member_meta(const osmium::item_type type)
{
    return m_member_meta[static_cast<uint16_t>(type) - 1];
}
const std::vector<osmium::relations::RelationMeta>& collector::relations() const
{
    return m_relations;
}
bool collector::keep_relation(const osmium::Relation&) const
{
    return true;
}
bool collector::keep_member(const osmium::relations::RelationMeta&, const osmium::RelationMember&) const
{
    return true;
}
void collector::node_not_in_any_relation(const osmium::Node&)
{
}
void collector::way_not_in_any_relation(const osmium::Way&)
{
}
void collector::relation_not_in_any_relation(const osmium::Relation&)
{
}
void collector::flush()
{
}
const osmium::Relation& collector::get_relation(size_t offset) const
{
    assert(m_relations_buffer.committed() > offset);
    return m_relations_buffer.get<osmium::Relation>(offset);
}
const osmium::Relation& collector::get_relation(const osmium::relations::RelationMeta& relation_meta) const
{
    return get_relation(relation_meta.relation_offset());
}
const osmium::Relation& collector::get_relation(const osmium::relations::MemberMeta& member_meta) const
{
    return get_relation(m_relations[member_meta.relation_pos()]);
}
osmium::OSMObject& collector::get_member(size_t offset) const
{
    assert(m_members_buffer.committed() > offset);
    return m_members_buffer.get<osmium::OSMObject>(offset);
}
void collector::add_relation(const osmium::Relation& relation)
{
    const size_t offset = m_relations_buffer.committed();
    m_relations_buffer.add_item(relation);

    osmium::relations::RelationMeta relation_meta{offset};

    int n = 0;
    for (auto& member : m_relations_buffer.get<osmium::Relation>(offset).members())
    {
        if (keep_member(relation_meta, member))
        {
            member_meta(member.type()).emplace_back(member.ref(), m_relations.size(), n);
            relation_meta.increment_need_members();
        }
        else
        {
            member.set_ref(0);// set member id to zero to indicate we are not interested
        }
        ++n;
    }

    assert(offset == m_relations_buffer.committed());
    if (relation_meta.has_all_members())
    {
        m_relations_buffer.rollback();
    }
    else
    {
        m_relations_buffer.commit();
        m_relations.push_back(relation_meta);
    }
}
void collector::sort_member_meta()
{
    std::sort(m_member_meta[0].begin(), m_member_meta[0].end());
    std::sort(m_member_meta[1].begin(), m_member_meta[1].end());
    std::sort(m_member_meta[2].begin(), m_member_meta[2].end());
}
bool collector::find_and_add_object(const osmium::OSMObject& object)
{
    auto range = find_member_meta(object.type(), object.id());

    if (count_not_removed(range) == 0)
    {
        // nothing found
        return false;
    }

    {
        members_buffer().add_item(object);
        const size_t member_offset = members_buffer().commit();

        for (auto& member_meta : range)
        {
            member_meta.set_buffer_offset(member_offset);
        }
    }

    for (auto& member_meta : range)
    {
        if (member_meta.removed())
        {
            break;
        }
        assert(member_meta.member_id() == object.id());
        assert(member_meta.relation_pos() < m_relations.size());
        osmium::relations::RelationMeta& relation_meta = m_relations[member_meta.relation_pos()];
        assert(member_meta.member_pos() < get_relation(relation_meta).members().size());
        relation_meta.got_one_member();
        if (relation_meta.has_all_members())
        {
            const size_t relation_offset = member_meta.relation_pos();
            // complete_relation(relation_meta);
            clear_member_metas(relation_meta);
            m_relations[relation_offset] = osmium::relations::RelationMeta{};
            possibly_purge_removed_members();
        }
    }

    return true;
}
uint64_t collector::used_memory() const
{
    const uint64_t nmembers = m_member_meta[0].capacity() + m_member_meta[1].capacity() + m_member_meta[2].capacity();
    const uint64_t members = nmembers * sizeof(osmium::relations::MemberMeta);
    const uint64_t relations = m_relations.capacity() * sizeof(osmium::relations::RelationMeta);
    const uint64_t relations_buffer_capacity = m_relations_buffer.capacity();
    const uint64_t members_buffer_capacity = m_members_buffer.capacity();

    std::cerr << "  nR  = m_relations.capacity() ........... = " << std::setw(12) << m_relations.capacity() << "\n";
    std::cerr << "  nMN = m_member_meta[NODE].capacity() ... = " << std::setw(12) << m_member_meta[0].capacity() << "\n";
    std::cerr << "  nMW = m_member_meta[WAY].capacity() .... = " << std::setw(12) << m_member_meta[1].capacity() << "\n";
    std::cerr << "  nMR = m_member_meta[RELATION].capacity() = " << std::setw(12) << m_member_meta[2].capacity() << "\n";
    std::cerr << "  nM  = m_member_meta[*].capacity() ...... = " << std::setw(12) << nmembers << "\n";

    std::cerr << "  sRM = sizeof(RelationMeta) ............. = " << std::setw(12) << sizeof(osmium::relations::RelationMeta) << "\n";
    std::cerr << "  sMM = sizeof(MemberMeta) ............... = " << std::setw(12) << sizeof(osmium::relations::MemberMeta) << "\n\n";

    std::cerr << "  nR * sRM ............................... = " << std::setw(12) << relations << "\n";
    std::cerr << "  nM * sMM ............................... = " << std::setw(12) << members << "\n";
    std::cerr << "  relations_buffer_capacity .............. = " << std::setw(12) << relations_buffer_capacity << "\n";
    std::cerr << "  members_buffer_capacity ................ = " << std::setw(12) << members_buffer_capacity << "\n";

    const uint64_t total = relations + members + relations_buffer_capacity + members_buffer_capacity;

    std::cerr << "  total .................................. = " << std::setw(12) << total << "\n";
    std::cerr << "  =======================================================\n";

    return relations_buffer_capacity + members_buffer_capacity + relations + members;
}
void collector::clear_member_metas(const osmium::relations::RelationMeta& relation_meta)
{
    const osmium::Relation& relation = get_relation(relation_meta);
    for (const auto& member : relation.members())
    {
        if (member.ref() != 0)
        {
            const auto range = find_member_meta(member.type(), member.ref());
            assert(!range.empty());

            // if this is the last time this object was needed
            // then mark it as removed
            if (count_not_removed(range) == 1)
            {
                get_member(range.begin()->buffer_offset()).set_removed(true);
            }

            for (auto& member_meta : range)
            {
                if (!member_meta.removed() && relation.id() == get_relation(member_meta).id())
                {
                    member_meta.remove();
                    break;
                }
            }
        }
    }
}
osmium::memory::Buffer& collector::members_buffer()
{
    return m_members_buffer;
}
bool collector::is_available(osmium::item_type type, osmium::object_id_type id)
{
    const auto range = find_member_meta(type, id);
    assert(!range.empty());
    return range.begin()->is_available();
}
size_t collector::get_offset(osmium::item_type type, osmium::object_id_type id)
{
    const auto range = find_member_meta(type, id);
    assert(!range.empty());
    assert(range.begin()->is_available());
    return range.begin()->buffer_offset();
}
std::pair<bool, size_t> collector::get_availability_and_offset(osmium::item_type type, osmium::object_id_type id)
{
    const auto range = find_member_meta(type, id);
    assert(!range.empty());
    if (range.begin()->is_available())
    {
        return std::make_pair(true, range.begin()->buffer_offset());
    }
    return std::make_pair(false, 0);
}
void collector::moving_in_buffer(size_t old_offset, size_t new_offset)
{
    const osmium::OSMObject& object = m_members_buffer.get<osmium::OSMObject>(old_offset);
    auto range = find_member_meta(object.type(), object.id());
    for (auto& member_meta : range)
    {
        assert(member_meta.buffer_offset() == old_offset);
        member_meta.set_buffer_offset(new_offset);
    }
}
void collector::possibly_purge_removed_members()
{
    ++m_count_complete;
    if (m_count_complete > 10000)
    {// XXX
        //                    const size_t size_before = m_members_buffer.committed();
        m_members_buffer.purge_removed(this);
        /*
                const size_t size_after = m_members_buffer.committed();
                double percent = static_cast<double>(size_before - size_after);
                percent /= size_before;
                percent *= 100;
                std::cerr << "PURGE (size before=" << size_before << " after=" << size_after << " purged=" << (size_before - size_after) << " / " << static_cast<int>(percent) << "%)\n";
*/
        m_count_complete = 0;
    }
}
std::vector<const osmium::Relation*> collector::get_incomplete_relations() const
{
    std::vector<const osmium::Relation*> relations;
    for (const auto& relation_meta : m_relations)
    {
        if (!relation_meta.has_all_members())
        {
            relations.push_back(&get_relation(relation_meta));
        }
    }
    return relations;
}
}