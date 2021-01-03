// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifndef PFAEDLE_CONFIG_CONFIGREADER_H_
#define PFAEDLE_CONFIG_CONFIGREADER_H_

#include "pfaedle/config/PfaedleConfig.h"
#include <vector>

namespace pfaedle::config
{

class ConfigReader
{
public:
    ConfigReader(Config& cfg);

    void read(int argc, char** argv);
    void help(const char* bin);

private:
    Config& config_;
};
}  // namespace pfaedle::config
#endif// PFAEDLE_CONFIG_CONFIGREADER_H_
