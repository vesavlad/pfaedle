// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifndef UTIL_JSON_WRITER_H_
#define UTIL_JSON_WRITER_H_

#include <map>
#include <ostream>
#include <stack>
#include <string>
#include <utility>
#include <vector>

namespace util::json
{

class WriterException : public std::exception
{
public:
    explicit WriterException(std::string msg) :
        _msg(std::move(msg)) {}
    ~WriterException() noexcept override = default;

    const char* what() const noexcept override { return _msg.c_str(); };

private:
    std::string _msg;
};

struct Null
{
};

struct Val
{
    enum VAL_T
    {
        INT,
        FLOAT,
        STRING,
        ARRAY,
        DICT,
        BOOL,
        JSNULL
    };
    VAL_T type;
    int i;
    double f;
    std::string str;
    std::vector<Val> arr;
    std::map<std::string, Val> dict;

    Val() { type = DICT; }
    Val(Null) { type = JSNULL; }
    Val(const std::vector<Val>& arrC) { arr = arrC, type = ARRAY; }
    Val(const std::map<std::string, Val>& dC) { dict = dC, type = DICT; }
    Val(const std::string& strC) { str = strC, type = STRING; }
    Val(const char* strC) { str = strC, type = STRING; }
    Val(double fC) { f = fC, type = FLOAT; }
    Val(int iC) { i = iC, type = INT; }
    Val(bool fC) { i = fC, type = BOOL; }
};

using Int = int;
using Float = double;
using Bool = bool;
using String = std::string;
using Array = std::vector<Val>;
typedef std::map<std::string, Val> Dict;

// simple JSON writer class without much overhead
class Writer
{
public:
    explicit Writer(std::ostream& out);
    Writer(std::ostream& out, size_t prec);
    Writer(std::ostream& out, size_t prec, bool pretty);
    Writer(std::ostream& out, size_t prec, bool pretty, size_t indent);
    ~Writer()= default;

    void obj();
    void arr();
    void key(const std::string& k);
    void val(const std::string& v);
    void val(const char* v);
    void val(double v);
    void val(int v);
    void val(bool v);
    void val(Null);
    void val(const Val& v);
    template<typename V>
    void keyVal(const std::string& k, const V& v)
    {
        key(k);
        val(v);
    }

    void close();
    void closeAll();

private:
    std::ostream& _out;

    enum NODE_T
    {
        OBJ,
        ARR,
        KEY
    };

    struct Node
    {
        NODE_T type;
        bool empty;
    };

    std::stack<Node> _stack;

    bool _pretty;
    size_t _indent;
    size_t _floatPrec;

    void valCheck();
    void prettor();
};

}  // namespace util

#endif  // UTIL_JSON_WRITER_H_
