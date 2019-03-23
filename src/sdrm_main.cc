// ======================================================================
// Copyright (C) 2019 Ramon Creager
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

#include "airspy_component.h"

#include "matrix/Architect.h"
#include "matrix/Component.h"
#include "matrix/Keymaster.h"
#include "matrix/ZMQContext.h"
#include "matrix/log_t.h"
#include "matrix/matrix_util.h"

#include <string>
#include <iostream>
#include <algorithm>
#include <yaml-cpp/yaml.h>
#include <tclap/CmdLine.h>

using namespace std;
using namespace YAML;
using namespace matrix;
using namespace mxutils;
using namespace TCLAP;

static matrix::log_t logger("main");
static string km_tcp_url{"tcp://localhost:42000"};

string get_most_local(vector<string> urls);

class SDRMArchitect : public Architect
{
public:
    SDRMArchitect(string name, string km_url);

};

SDRMArchitect::SDRMArchitect(string name, string km_url) :
    Architect(name, km_url)
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
    int rval = 0;

    try
    {
        auto ctx = ZMQContext::Instance();
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
        Keymaster km(km_tcp_url);
        auto urls = km.get_as<vector<string>>("Keymaster.URLS.AsConfigured.State");
        cout << urls << endl;
        auto km_url = get_most_local(urls);
        cout << km_url << endl;

        SDRMArchitect sdrm("control", km_url);
        // wait for the keymaster events which report components in the Standby state.
        // Probably should return if any component reports and error state????
        bool result = sdrm.wait_all_in_state("Standby", 1000000);

        if (!result)
        {
            cout << "initial standby state error" << endl;
        }

        // sdrm.set_system_mode("iq_monitor");
        // // Everybody now in standby. Get things running by issuing a start event.
        // sdrm.ready();
        // result = sdrm.wait_all_in_state("Ready", 1000000);

        // if (!result)
        // {
        //     cout << "initial standby state error" << endl;
        // }

        // sdrm.start();
        // result = sdrm.wait_all_in_state("Running", 1000000);

        // if (!result)
        // {
        //     cout << "initial standby state error" << endl;
        // }

        // while (!sdrm.wait_all_in_state("Ready", 1000000));

        // sleep(1);
    }
    catch (KeymasterException &e)
    {
        logger.fatal(e.what());
        rval = 1;
    }
    catch (ArgException &e)  // catch any exceptions
	{
        logger.fatal(e.error(), " for arg ", e.argId());
        rval = 1;
    }
    catch (runtime_error &e)
    {
        logger.fatal(e.what());
        rval = 1;
    }
    catch (zmq::error_t &e)
    {
        logger.fatal(e.what());
        rval = 1;
    }

    Architect::destroy_keymaster_server();
    return rval;
}

string get_most_local(vector<string> urls)
{
    string inproc{"inproc"};
    string ipc{"ipc"};
    string tcp{"tcp"};

    auto inproc_it = find_if(urls.begin(), urls.end(),
                             [inproc](auto x)
                             {
                                 return x.find(inproc) != string::npos;
                             });

    if (inproc_it != urls.end())
    {
        return *inproc_it;
    }

    auto ipc_iter = find_if(urls.begin(), urls.end(),
                            [ipc](auto x)
                            {
                                return x.find(ipc) != string::npos;
                            });

    if (ipc_iter != urls.end())
    {
        return *ipc_iter;
    }

    auto tcp_iter = find_if(urls.begin(), urls.end(),
                            [tcp](auto x)
                            {
                                return x.find(tcp) != string::npos;
                            });

    if (tcp_iter != urls.end())
    {
        return *tcp_iter;
    }

    return "";
}
