// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifndef PFAEDLE_GTFS_SHAPECONTAINER_H_
#define PFAEDLE_GTFS_SHAPECONTAINER_H_

#include <cppgtfs/gtfs/Shape.h>
#include <cppgtfs/gtfs/flat/Shape.h>
#include <pfaedle/Def.h>
#include <util/Misc.h>

#include <unistd.h>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <string>

namespace pfaedle::gtfs
{

struct Shape
{
    using Ref = std::string;

    explicit Shape(const std::string& id) :
        id(id)
    {}

    static std::string getId(Ref r)
    {
        return r;
    }

    template<typename T>
    bool addPoint(T p)
    {
        UNUSED(p);
        return true;
    }

    const std::string& getId() const
    {
        return id;
    }

private:
    std::string id;
};

template<typename T>
class ShapeContainer
{
public:
    ShapeContainer():
        _ptr{},
        _max{}
    {
        std::string f = pfaedle::getTmpFName("", "");
        _storage.open(f, std::fstream::in | std::fstream::out | std::fstream::trunc);

        // immediately unlink
        unlink(f.c_str());

        if (!_storage.good())
        {
            std::cerr << "Could not open temporary file " << f
                      << std::endl;
            exit(1);
        }
    }

    ~ShapeContainer()
    {
        _storage.close();
    }


    T* add(const T& ent)
    {
        _ids.insert(ent.getId());
        return reinterpret_cast<T*>(1);
    }


    bool remove(const std::string& id)
    {
        _ids.erase(id);
        return true;
    }


    T* get(const std::string& id)
    {
        if (!has(id))
            return nullptr;
        return reinterpret_cast<T*>(1);
    }


    const T* get(const std::string& id) const
    {
        if (!has(id))
            return nullptr;
        return reinterpret_cast<T*>(1);
    }


    bool has(const std::string& id) const
    {
        return _ids.count(id);
    }


    size_t size() const
    {
        return _ids.size();
    }

    void finalize()
    {}


    std::string add(const ad::cppgtfs::gtfs::Shape& s)
    {
        if (has(s.getId()))
            return s.getId();

        _ids.insert(s.getId());

        _writeBuffer << s.getId() << '\t' << s.getPoints().size();
        _writeBuffer << std::setprecision(11);
        for (auto p : s.getPoints())
        {
            _writeBuffer << " " << p.lat << " " << p.lng << " " << p.travelDist;
        }

        // entries are newline separated
        _writeBuffer << '\n';

        if (_writeBuffer.tellp() > 1000 * 5000)
        {
            _storage << _writeBuffer.rdbuf();
            _writeBuffer.clear();
        }

        return s.getId();
    }


    void open()
    {
        _storage << _writeBuffer.rdbuf();
        _writeBuffer.clear();

        _ptr = 0;
        _max = 0;
        _storage.clear();
        _storage.seekg(0, std::ios::beg);
    }


    bool nextStoragePt(ad::cppgtfs::gtfs::flat::ShapePoint* ret)
    {
        while (_storage.good() && !_storage.fail())
        {
            if (!_ptr)
            {
                _storage >> _curId;
                _storage >> _max;
            }

            if (!_storage.good() || _storage.fail())
                return false;

            _storage >> ret->lat;
            _storage >> ret->lng;
            _storage >> ret->travelDist;
            ret->seq = _ptr + 1;
            ret->id = _curId;

            if (_ptr + 1 == _max)
                _ptr = 0;
            else
                _ptr++;

            if (!_storage.good() || _storage.fail())
                return false;

            if (has(ret->id))
                return true;
        }

        return false;
    }


    const std::string getRef(const std::string& id) const
    {
        if (!has(id))
            return "";

        return id;
    }


    std::string getRef(const std::string& id)
    {
        if (!has(id))
            return "";

        return id;
    }

private:
    std::set<std::string> _ids;
    std::fstream _storage;
    size_t _ptr;
    size_t _max;
    std::string _curId;
    std::stringstream _writeBuffer;
};

}  // namespace pfaedle

#endif  // PFAEDLE_GTFS_SHAPECONTAINER_H_
