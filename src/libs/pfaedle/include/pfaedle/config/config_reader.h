// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifndef PFAEDLE_CONFIG_CONFIGREADER_H_
#define PFAEDLE_CONFIG_CONFIGREADER_H_

#include "pfaedle/config/config.h"
#include <vector>

namespace pfaedle::config
{

class config_reader
{
public:
    explicit config_reader(config& cfg);

    void read(int argc, char** argv);
    void help(const char* bin);

private:
    config& config_;
};
}  // namespace pfaedle::config
#endif// PFAEDLE_CONFIG_CONFIGREADER_H_
