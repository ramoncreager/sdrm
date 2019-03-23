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

using namespace std;
using namespace matrix;

Component *AirspyComponent::factory(std::string name,std::string km_url)
{
    return new AirspyComponent(name, km_url);
}

AirspyComponent::~AirspyComponent()
{

}

void AirspyComponent::run_loop()
{

}

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
    cout << "got " << transfer->sample_count << " samples." << endl;
}
