/*******************************************************************
 *  airspy_component.h - The Airspy HF+ radio component. Receives data
 *  from the Airspy, and sends it via a source to any interested
 *  subscribers. Loads status details in the Keymaster
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
#if !defined(_AIRSPY_COMPONENT_H_)
#define _AIRSPY_COMPONENT_H_

#include "matrix/Thread.h"
#include "matrix/Component.h"

#include <iostream>
#include <map>
#include <memory>
#include <libairspyhf/airspyhf.h>

class AirspyComponent : public matrix::Component
{
public:

    virtual ~AirspyComponent();
    static Component *factory(std::string myname,std::string k);

protected:
    AirspyComponent(std::string name, std::string keymaster_url) :
        matrix::Component(name, keymaster_url),
        run_thread(this, &AirspyComponent::run_loop)
    {
        run_thread.start();
    }

    void run_loop();
    void write_to_source(airspyhf_transfer_t *);

    // handlers
    void lib_version(std::string key, YAML::Node data);
    void list_devices(std::string key, YAML::Node data);
    void open(std::string key, YAML::Node data);
    void open_sn(std::string key, YAML::Node data);
    void close(std::string key, YAML::Node data);
    void start(std::string key, YAML::Node data);
    void stop(std::string key, YAML::Node data);
    void is_streaming(std::string key, YAML::Node data);
    void set_freq(std::string key, YAML::Node data);
    void set_lib_dsp(std::string key, YAML::Node data);
    void get_samplerates(std::string key, YAML::Node data);
    void set_samplerate(std::string key, YAML::Node data);
    void get_calibration(std::string key, YAML::Node data);
    void set_calibration(std::string key, YAML::Node data);
    void set_optimal_iq_correction_point(std::string key, YAML::Node data);
    void iq_balancer_configure(std::string key, YAML::Node data);
    void flash_calibration(std::string key, YAML::Node data);
    void board_partid_serialno_read(std::string key, YAML::Node data);
    void version_string_read(std::string key, YAML::Node data);
    void set_user_output(std::string key, YAML::Node data);
    void set_hf_agc(std::string key, YAML::Node data);
    void set_hf_agc_threshold(std::string key, YAML::Node data);
    void set_hf_att(std::string key, YAML::Node data);

    matrix::Thread<AirspyComponent> run_thread;
    std::map<std::string, decltype(&AirspyComponent::start)> handlers;
};

#endif
