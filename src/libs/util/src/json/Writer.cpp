// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#include "util/json/Writer.h"
#include "util/String.h"
#include <iomanip>
using namespace util;
using namespace json;

using std::ostream;
using std::string;

Writer::Writer(std::ostream& out) :
    _out(out),
    _pretty(false),
    _indent(2),
    _floatPrec(10)
{}

Writer::Writer(std::ostream& out, size_t prec) :
    _out(out),
    _pretty(false),
    _indent(2),
    _floatPrec(prec)
{}

Writer::Writer(std::ostream& out, size_t prec, bool pret) :
    _out(out), _pretty(pret),
    _indent(2),
    _floatPrec(prec)
{}

Writer::Writer(std::ostream& out, size_t prec, bool pret, size_t indent) :
    _out(out),
    _pretty(pret),
    _indent(indent),
    _floatPrec(prec)
{}

void Writer::obj()
{
    if (!_stack.empty() && _stack.top().type == OBJ)
        throw WriterException("Object not allowed as key");
    if (!_stack.empty() && _stack.top().type == KEY) _stack.pop();
    if (!_stack.empty() && _stack.top().type == ARR) valCheck();
    if (!_stack.empty() && _stack.top().type == ARR) prettor();
    _out << "{";
    _stack.push({OBJ, 1});
}

void Writer::key(const std::string& k)
{
    if (_stack.empty() || _stack.top().type != OBJ)
        throw WriterException("Keys only allowed in objects.");
    if (!_stack.top().empty) _out << "," << (_pretty ? " " : "");
    _stack.top().empty = 0;
    prettor();
    _out << "\"" << k << "\""
          << ":" << (_pretty ? " " : "");
    _stack.push({KEY, 1});
}

void Writer::valCheck()
{
    if (_stack.empty() || (_stack.top().type != KEY && _stack.top().type != ARR))
        throw WriterException("Value not allowed here.");
    if (!_stack.empty() && _stack.top().type == KEY) _stack.pop();
    if (!_stack.empty() && _stack.top().type == ARR)
    {
        if (!_stack.top().empty) _out << "," << (_pretty ? " " : "");
        _stack.top().empty = 0;
    }
}

void Writer::val(const std::string& v)
{
    valCheck();
    _out << "\"" << util::jsonStringEscape(v) << "\"";
}

void Writer::val(const char* v)
{
    valCheck();
    _out << "\"" << util::jsonStringEscape(v) << "\"";
}

void Writer::val(bool v)
{
    valCheck();
    _out << (v ? "true" : "false");
}

void Writer::val(int v)
{
    valCheck();
    _out << v;
}

void Writer::val(double v)
{
    valCheck();
    _out << std::fixed << std::setprecision(_floatPrec) << v;
}

void Writer::val(Null)
{
    valCheck();
    _out << "null";
}

void Writer::val(const Val& v)
{
    switch (v.type)
    {
        case Val::JSNULL:
            val(Null());
            return;
        case Val::INT:
            val(v.i);
            return;
        case Val::FLOAT:
            val(v.f);
            return;
        case Val::BOOL:
            val((bool) v.i);
            return;
        case Val::STRING:
            val(v.str);
            return;
        case Val::ARRAY:
            arr();
            for (const Val& varr : v.arr) val(varr);
            close();
            return;
        case Val::DICT:
            obj();
            for (const auto& vdic : v.dict)
            {
                keyVal(vdic.first, vdic.second);
            };
            close();
            return;
    }
}

void Writer::arr()
{
    if (!_stack.empty() && _stack.top().type == OBJ)
        throw WriterException("Array not allowed as key");
    if (!_stack.empty() && _stack.top().type == KEY) _stack.pop();
    if (!_stack.empty() && _stack.top().type == ARR) valCheck();
    _out << "[";
    _stack.push({ARR, 1});
}

void Writer::prettor()
{
    if (_pretty)
    {
        _out << "\n";
        for (size_t i = 0; i < _indent * _stack.size(); i++) _out << " ";
    }
}

void Writer::closeAll()
{
    while (!_stack.empty()) close();
}

void Writer::close()
{
    if (_stack.empty()) return;
    switch (_stack.top().type)
    {
        case OBJ:
            _stack.pop();
            prettor();
            _out << "}";
            break;
        case ARR:
            _stack.pop();
            _out << "]";
            break;
        case KEY:
            throw WriterException("Missing value.");
    }
}
