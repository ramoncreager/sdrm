/*******************************************************************
 *  SDRMArchitect.cc - Implements the SDR-Matrix Architect. The
 *  Architect is responsible for the creation and orchestration of the
 *  components.
 *
 *  Copyright (C) 2019 Ramon Creager
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *******************************************************************/

#include "SDRMArchitect.h"
#include "airspy_component.h"
#include "console_display.h"
#include "fft_component.h"
#include "matrix/Keymaster.h"
#include "matrix/yaml_util.h"
#include "matrix/log_t.h"
#include "matrix/string_format.h"

using namespace matrix;
using namespace mxutils;

static string status_base_key = "OBS_PARAMS.";
static string daq_cmd_key     = "DAQCOMMAND";
static string daq_mode_key    = status_base_key + "OBS_MODE";
static string daq_state_key   = status_base_key + "DAQSTATE";
static string daq_error_key   = "DAQERRORSTREAM";

static matrix::log_t logger("SDRMArchitect");

namespace sdrm
{
    // A template to do YAML conversions using return value instead of
    // exceptions
    template<typename T> bool yaml_as(YAML::Node n, T& value)
    {
        try
        {
            value = n.as<T>();
            return true;
        }
        catch (YAML::BadConversion &e)
        {
            return false;
        }
    }

    SDRMArchitect::SDRMArchitect(string name, string km_url)
        : Architect(name, km_url)
    {

        add_component_factory("AirspyComponent", &AirspyComponent::factory);
        add_component_factory("FFTComponent", &FFTComponent::factory);
        add_component_factory("ConsoleDisplay", &ConsoleDisplay::factory);

        try
        {
            basic_init();
        }
        catch(ArchitectException &e)
        {
            cout << e.what() << endl;
            throw move(e);
        }

        sleep(1);     // wait for everyone to start their threads
                      // before sending any commands.
        initialize(); // Sends the init event to get components initialized.
    }

    SDRMArchitect::~SDRMArchitect()
    {
    }



}
