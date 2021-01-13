// Copyright 2016, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifndef UTIL_GEO_OUTPUT_GEOGRAPHJSONOUTPUT_H_
#define UTIL_GEO_OUTPUT_GEOGRAPHJSONOUTPUT_H_

#include <ostream>
#include <string>
#include <util/String.h>
#include <util/geo/output/GeoJsonOutput.h>
#include <util/graph/Graph.h>

namespace util::geo::output
{

class GeoGraphJsonOutput
{
public:
    GeoGraphJsonOutput() = default;

    // print a graph to the provided path
    template<typename N, typename E>
    void print(const util::graph::Graph<N, E>& outG, std::ostream& str)
    {
        printImpl(outG, str, false);
    }

    // print a graph to the provided path, but treat coordinates as Web Mercator coordinates and reproject to WGS84
    template<typename N, typename E>
    void printLatLng(const util::graph::Graph<N, E>& outG, std::ostream& str)
    {
        printImpl(outG, str, true);
    }

private:
    template<typename T>
    Line<T> createLine(const util::geo::Point<T>& a,
                       const util::geo::Point<T>& b)
    {
        Line<T> ret;
        ret.push_back(a);
        ret.push_back(b);
        return ret;
    }

    // print a graph to the provided path
    template<typename N, typename E>
    void printImpl(const util::graph::Graph<N, E>& outG, std::ostream& str, bool proj)
    {
        GeoJsonOutput json_output(str);

        // first pass, nodes
        for (util::graph::Node<N, E>* n : outG.getNds())
        {
            if (!n->pl().get_geom()) continue;

            json::Dict props{{"id", util::toString(n)},
                             {"deg", util::toString(n->getDeg())},
                             {"deg_out", util::toString(n->getOutDeg())},
                             {"deg_in", util::toString(n->getInDeg())}};

            auto addProps = n->pl().get_attrs();
            props.insert(addProps.begin(), addProps.end());

            if (proj)
            {
                json_output.printLatLng(*n->pl().get_geom(), props);
            }
            else
            {
                json_output.print(*n->pl().get_geom(), props);
            }
        }

        // second pass, edges
        for (graph::Node<N, E>* n : outG.getNds())
        {
            for (graph::Edge<N, E>* e : n->getAdjListOut())
            {
                // to avoid double output for undirected graphs
                if (e->getFrom() != n) continue;
                json::Dict props{{"from", util::toString(e->getFrom())},
                                 {"to", util::toString(e->getTo())},
                                 {"id", util::toString(e)}};

                auto addProps = e->pl().get_attrs();
                props.insert(addProps.begin(), addProps.end());

                if (!e->pl().get_geom() || e->pl().get_geom()->empty())
                {
                    if (e->getFrom()->pl().get_geom())
                    {
                        auto a = *e->getFrom()->pl().get_geom();
                        if (e->getTo()->pl().get_geom())
                        {
                            auto b = *e->getTo()->pl().get_geom();
                            if (proj)
                            {
                                json_output.printLatLng(createLine(a, b), props);
                            }
                            else
                            {
                                json_output.print(createLine(a, b), props);
                            }
                        }
                    }
                }
                else
                {
                    if (proj)
                    {
                        json_output.printLatLng(*e->pl().get_geom(), props);
                    }
                    else
                    {
                        json_output.print(*e->pl().get_geom(), props);
                    }
                }
            }
        }

        json_output.flush();
    }
};

}

#endif  // UTIL_GEO_OUTPUT_GEOGRAPHJSONOUTPUT_H_
