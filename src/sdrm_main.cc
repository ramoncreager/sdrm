// ======================================================================
// Copyright (C) 2015 Associated Universities, Inc. Washington DC, USA.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// Correspondence concerning GBT software should be addressed as follows:
//  GBT Operations
//  National Radio Astronomy Observatory
//  P. O. Box 2
//  Green Bank, WV 24944-0002 USA

#include "airspy_component.h"

#include "matrix/Architect.h"
#include "matrix/Component.h"
#include "matrix/Keymaster.h"
#include "matrix/ZMQContext.h"
#include "matrix/log_t.h"

#include <string>
#include <iostream>
#include <yaml-cpp/yaml.h>
#include <tclap/CmdLine.h>

using namespace std;
using namespace YAML;
using namespace matrix;
using namespace TCLAP;

static matrix::log_t logger("main");


class SDRMArchitect : public Architect
{
public:
    SDRMArchitect();

};

SDRMArchitect::SDRMArchitect() :
    Architect("control", "inproc://sdrm.keymaster")
{
    add_component_factory("AirspyComponent", &AirspyComponent::factory);

    try
    {
        basic_init();
    }
    catch(ArchitectException &e)
    {
        cout << e.what() << endl;
        throw move(e);
    }

    initialize(); // Sends the init event to get components initialized.
}


int main(int argc, char **argv)
{
    CmdLine cmd("sdrm: Software Defined Radio, Matrix");

    // Define a switch and add it to the command line.
    ValueArg<string> logLevel("l", "log", "Log level, one of DEBUG|INFO|WARNING",
            false,
            "WARNING",
            "string"
        );
    cmd.add(logLevel);
    // Parse the args.
    cmd.parse(argc, argv);

    // Picks basic backend if stdout is redirected to a log. Picks the
    // ANSI color backend if in a terminal
    matrix::log_t::add_backend(shared_ptr<matrix::log_t::Backend>(
                new matrix::log_t::ostreamBackendColor(cout)));

    string loglevel = logLevel.getValue();

    if (loglevel == "INFO")
    {
        matrix::log_t::set_log_level(matrix::log_t::INFO_LEVEL);
    }
    else if (loglevel == "DEBUG")
    {
        matrix::log_t::set_log_level(matrix::log_t::DEBUG_LEVEL);
    }
    else
    {
        matrix::log_t::set_log_level(matrix::log_t::WARNING_LEVEL);
    }

    Architect::create_keymaster_server("airspyhf.yaml");
    SDRMArchitect sdrm;

    // wait for the keymaster events which report components in the Standby state.
    // Probably should return if any component reports and error state????
    bool result = sdrm.wait_all_in_state("Standby", 1000000);

    if (!result)
    {
        cout << "initial standby state error" << endl;
    }

    sdrm.set_system_mode("iq_monitor");
    // Everybody now in standby. Get things running by issuing a start event.
    sdrm.ready();
    result = sdrm.wait_all_in_state("Ready", 1000000);

    if (!result)
    {
        cout << "initial standby state error" << endl;
    }

    sdrm.start();
    result = sdrm.wait_all_in_state("Running", 1000000);

    if (!result)
    {
        cout << "initial standby state error" << endl;
    }

    while (!sdrm.wait_all_in_state("Ready", 1000000));
    Architect::destroy_keymaster_server();

    sleep(1);
    return 0;
}
