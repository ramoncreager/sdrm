/*******************************************************************
 *  airspy_component.cc - This component handles the control of one or
 *  more AirspyHF+ modules, and is the source of the IQ data from
 *  those modules.
 *
 *  Copyright (C) 2019 Associated Universities, Inc. Washington DC, USA.
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

#include "airspy_component.h"

#include <memory>
#include <matrix/matrix_util.h>

using namespace std;
using namespace matrix;
using namespace mxutils;

ostream & operator << (ostream &o,  const airspyhf_complex_float_t &v)
{
    o << "{" << v.re << ", " << v.im << "}";
    return o;
}


Component *AirspyComponent::factory(std::string name,std::string km_url)
{
    return new AirspyComponent(name, km_url);
}

AirspyComponent::AirspyComponent(std::string name, std::string keymaster_url) :
    Component(name, keymaster_url),
    iq_signal_source(keymaster_url, name, "iq_data")
{
    handlers =
        {
            {"lib_version",
             cb_t(new member_cb(this, &AirspyComponent::lib_version))},
            {"list_devices",
             cb_t(new member_cb(this, &AirspyComponent::list_devices))},
            {"open",
             cb_t(new member_cb(this, &AirspyComponent::open))},
            {"open_sn",
             cb_t(new member_cb(this, &AirspyComponent::open_sn))},
            {"close",
             cb_t(new member_cb(this, &AirspyComponent::close))},
            {"start",
             cb_t(new member_cb(this, &AirspyComponent::start))},
            {"stop",
             cb_t(new member_cb(this, &AirspyComponent::stop))},
            {"is_streaming",
             cb_t(new member_cb(this, &AirspyComponent::is_streaming))},
            {"set_freq",
             cb_t(new member_cb(this, &AirspyComponent::set_freq))},
            {"set_lib_dsp",
             cb_t(new member_cb(this, &AirspyComponent::set_lib_dsp))},
            {"get_samplerates",
             cb_t(new member_cb(this, &AirspyComponent::get_samplerates))},
            {"set_samplerate",
             cb_t(new member_cb(this, &AirspyComponent::set_samplerate))},
            {"get_calibration",
             cb_t(new member_cb(this, &AirspyComponent::get_calibration))},
            {"set_calibration",
             cb_t(new member_cb(this, &AirspyComponent::set_calibration))},
            {"set_optimal_iq_correction_point",
             cb_t(new member_cb(this, &AirspyComponent::set_optimal_iq_correction_point))},
            {"iq_balancer_configure",
             cb_t(new member_cb(this, &AirspyComponent::iq_balancer_configure))},
            {"flash_calibration",
             cb_t(new member_cb(this, &AirspyComponent::flash_calibration))},
            {"board_partid_serialno_read",
             cb_t(new member_cb(this, &AirspyComponent::board_partid_serialno_read))},
            {"version_string_read",
             cb_t(new member_cb(this, &AirspyComponent::version_string_read))},
            {"set_user_output",
             cb_t(new member_cb(this, &AirspyComponent::set_user_output))},
            {"set_hf_agc",
             cb_t(new member_cb(this, &AirspyComponent::set_hf_agc))},
            {"set_hf_agc_threshold",
             cb_t(new member_cb(this, &AirspyComponent::set_hf_agc_threshold))},
            {"set_hf_att",
             cb_t(new member_cb(this, &AirspyComponent::set_hf_att))}
        };

    for (auto handler: handlers)
    {
        auto key = handler.first;
        auto ptr = handler.second;
        keymaster->subscribe("AIRSPYCMDS." + key + ".request", ptr.get());
    }

    // run_thread.start();
}

AirspyComponent::~AirspyComponent()
{

}

// void AirspyComponent::run_loop()
// {

// }

/**
 * Writes to the component's source. This function is called by the
 * callback function that is given to the airspyhf library's start()
 * call. The start() function takes a void *ctx that can be anything
 * of use. In this case it is the 'this' pointer to the
 * AirspyComponent instance that is running. When called, the callback
 * unpacks it and calls this function with it.
 *
 * @param transfer: a pointer to the airspyhf_transfer_t object given
 * to the callback. The airspyhf_transfer_t structure is defined as
 * follows:
 *
 *    typedef struct {
 *        airspyhf_device_t* device;
 *        void* ctx;
 *        airspyhf_complex_float_t* samples;
 *        int sample_count;
 *        uint64_t dropped_samples;
 *    } airspyhf_transfer_t;
 *
 * of interest to us here are the samples, the sample_count, and the
 * dropped_samples count.
 *
 */

void AirspyComponent::write_to_source(airspyhf_transfer_t *transfer)
{
    cout << transfer->samples[0] << endl;
}
