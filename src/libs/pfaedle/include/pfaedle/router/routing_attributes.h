// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifndef PFAEDLE_ROUTER_ROUTINGATTRS_H_
#define PFAEDLE_ROUTER_ROUTINGATTRS_H_

#include <map>
#include <string>


namespace pfaedle
{
namespace trgraph
{
struct TransitEdgeLine;
}

namespace router
{
class routing_attributes
{
public:
    routing_attributes() :
        from(),
        to(),
        short_name(),
        simi_cache_()
    {}

    // carfull: lower return value = higher similarity
    double simi(const trgraph::TransitEdgeLine* line) const
    {
        auto i = simi_cache_.find(line);
        if (i != simi_cache_.end()) return i->second;

        double cur = 1;
        if (short_name.empty() || router::lineSimi(line->shortName, short_name) > 0.5)
            cur -= 0.333333333;

        if (to.empty() || line->toStr.empty() ||
            router::statSimi(line->toStr, to) > 0.5)
            cur -= 0.333333333;

        if (from.empty() || line->fromStr.empty() ||
            router::statSimi(line->fromStr, from) > 0.5)
            cur -= 0.333333333;

        simi_cache_[line] = cur;

        return cur;
    }

    std::string from;
    std::string to;
    std::string short_name;
private:
    mutable std::map<const trgraph::TransitEdgeLine*, double> simi_cache_;
};

inline bool operator==(const routing_attributes& a, const routing_attributes& b)
{
    return a.short_name == b.short_name && a.to == b.to &&
           a.from == b.from;
}

inline bool operator!=(const routing_attributes& a, const routing_attributes& b)
{
    return !(a == b);
}

inline bool operator<(const routing_attributes& a, const routing_attributes& b)
{
    return a.from < b.from ||
           (a.from == b.from && a.to < b.to) ||
           (a.from == b.from && a.to == b.to &&
            a.short_name < b.short_name);
}

}  // namespace pfaedle
}
#endif  // PFAEDLE_ROUTER_ROUTINGATTRS_H_
