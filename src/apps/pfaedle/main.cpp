// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#include "app.h"
#include <logging/logger.h>

int main(int argc, char* argv[])
{
    logging::configure_logging();

    app app(argc, argv);
    app.run();
}
