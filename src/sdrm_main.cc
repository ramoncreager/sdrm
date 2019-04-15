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
#include "simple_msgpk_client.h"

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

class SDRMArchitect : public Architect
{
public:
    SDRMArchitect(string name, string km_url);

};

SDRMArchitect::SDRMArchitect(string name, string km_url) :
    Architect(name, km_url)
{
    add_component_factory("AirspyComponent", &AirspyComponent::factory);
    add_component_factory("simple_msgpk_client", &MsgpackComponent::factory);

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
        ValueArg<string> logLevel(
            "l", "log", "Log level, one of DEBUG|INFO|WARNING",
                false, "WARNING", "string");
        cmd.add(logLevel);
        // Parse the args.
        cmd.parse(argc, argv);
        // Picks basic backend if stdout is redirected to a log. Picks the
        // ANSI color backend if in a terminal
        log_t::set_default_backend();

        string loglevel = logLevel.getValue();

        if (loglevel == "INFO")
        {
            log_t::set_log_level(Levels::INFO_LEVEL);
        }
        else if (loglevel == "DEBUG")
        {
            log_t::set_log_level(Levels::DEBUG_LEVEL);
        }
        else if (loglevel == "WARNING")
        {
            log_t::set_log_level(Levels::WARNING_LEVEL);
        }
        else
        {
            log_t::set_log_level(Levels::ERROR_LEVEL);
        }

        Architect::create_keymaster_server("airspyhf.yaml");
        Keymaster km(km_tcp_url);
        auto urls = km.get_as<vector<string>>("Keymaster.URLS.AsConfigured.State");
        logger.debug(__PRETTY_FUNCTION__, "Available URLs: ", urls);
        auto km_url = get_most_local(urls);
        logger.debug(__PRETTY_FUNCTION__, "Most local url:", km_url);

        SDRMArchitect sdrm("control", km_url);
        // wait for the keymaster events which report components in the Standby state.
        // Probably should return if any component reports and error state????
        bool result = sdrm.wait_all_in_state("Standby", 1000000);

        if (!result)
        {
            auto components = sdrm.components_not_in_state("Standby");
            logger.warning(__PRETTY_FUNCTION__, "Not all in Standby: ", components);
        }

        sdrm.set_system_mode("iq_monitor");
        logger.debug(__PRETTY_FUNCTION__, "Everybody now in standby. Get things",
                     "running by issuing a start event.");
        sdrm.ready();
        logger.debug(__PRETTY_FUNCTION__, "Waiting for components to go to Ready");
        result = sdrm.wait_all_in_state("Ready", 4000000);

        if (!result)
        {
            auto components = sdrm.components_not_in_state("Ready");
            logger.warning(__PRETTY_FUNCTION__, "Not all in Ready: ", components);
        }

        Time::Time_t now, last_pulse_update = 0;
        int year, month, dayofmonth, hour, minute;
        double second;
        char buf[80];

        while(true)
        {
            now = Time::getUTC();

            if (now - last_pulse_update > Time::TM_ONE_SEC)
            {
                Time::calendarDate(now, year, month, dayofmonth, hour, minute, second);
                sprintf(buf, "%04i/%02i/%02i %02i:%02i:%02.0f",
                        year, month, dayofmonth, hour, minute, second);
                km.put_nb("OBS_PARAMS.DAQPULSE", buf);
                last_pulse_update = now;
            }

            Time::thread_delay(1000000000L);
        }
    }
    catch (KeymasterException &e)
    {
        logger.fatal(e.what());
        rval = 1;
    }
    catch (ArgException &e)
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
