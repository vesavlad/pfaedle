// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#include "app.h"
#include <logging/logger.h>
#include <logging/scoped_timer.h>

int main(int argc, char* argv[])
{
    logging::configure_logging();
    logging::scoped_timer master_timer("application");

    app app(argc, argv);
    app.run();
}
