// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifndef PFAEDLE_DEF_H_
#define PFAEDLE_DEF_H_

#include <unistd.h>
#include <string>
#include <logging/logger.h>
#include "util/Misc.h"
#include "util/geo/Geo.h"
#include "util/geo/PolyLine.h"

#define __str_a(s) __str_b(s)
#define __str_b(s) #s

#define PFAEDLE_PRECISION double
#define PFAEDLE_PRECISION_STR __str_a(PFAEDLE_PRECISION)

using POINT = util::geo::Point<PFAEDLE_PRECISION>;
using LINE = util::geo::Line<PFAEDLE_PRECISION>;
using BOX = util::geo::Box<PFAEDLE_PRECISION>;
using POLYLINE = util::geo::PolyLine<PFAEDLE_PRECISION>;

#define BOX_PADDING 2500

namespace pfaedle {

// _____________________________________________________________________________
inline std::string getTmpFName(std::string dir, std::string postf) {
  if (postf.size()) postf = "-" + postf;
  if (!dir.size()) dir = util::getTmpDir();
  if (dir.size() && dir.back() != '/') dir = dir + "/";

  std::string f = dir + ".pfaedle-tmp" + postf;

  size_t c = 0;

  while (access(f.c_str(), F_OK) != -1) {
    c++;
    if (c > 10000) {
      // giving up...
      LOG_ERROR() << "Could not find temporary file name!";
      exit(1);
    }
    std::stringstream ss;
    ss << dir << ".pfaedle-tmp" << postf << "-" << std::rand();
    f = ss.str().c_str();
  }

  return f;
}

}  // namespace pfaedle

#endif  // PFAEDLE_DEF_H_
