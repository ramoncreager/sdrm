/*******************************************************************
 *  airspyhf_handlers.cc - Handlers for Keymaster requests
 *
 *  Copyright (C) 2019 Ramon Creager.
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
#include <matrix/Keymaster.h>
#include <matrix/yaml_util.h>
#include <matrix/matrix_util.h>
#include <yaml-cpp/yaml.h>
#include <libairspyhf/airspyhf.h>
#include <boost/algorithm/string.hpp>
#include <sstream>
#include <memory.h>

#define BUF_SIZE 128
#define DEFAULT_DEVICE 0xFFFFFFFFFFFFFFFF

using namespace std;
using namespace matrix;

map<uint64_t, airspyhf_device_t *> devices;
map<airspyhf_device_t *, uint64_t> streamers;

int rx_callback(airspyhf_transfer_t *transfer);

void do_rest(YAML::Node &)
{
}

template<typename T, typename... Args>
void do_rest(YAML::Node &rval, T &a, Args... args)
{
    rval.push_back(a);
    do_rest(rval, args...);
}

template<typename T, typename... Args>
YAML::Node airspyhf_response(bool flag, T &a, Args... args)
{
    YAML::Node rval;
    string error_code = flag ? "AIRSPYHF_SUCCESS" : "AIRSPYHF_ERROR";
    rval.push_back(error_code);
    rval.push_back(a);
    do_rest(rval, args...);
    return rval;
}

airspyhf_device_t *get_airspyhf_device(uint64_t sn)
{
    airspyhf_device_t *dev{NULL};
    auto dev_pr = devices.find(sn);

    if (dev_pr != devices.end())
    {
        dev = dev_pr->second;
    }

    return dev;
}

uint64_t get_airspyhf_streaming_device_sn(airspyhf_device_t *dev)
{
    auto str_pr = streamers.find(dev);

    if (str_pr == streamers.end())
    {
        throw std::runtime_error("No streaming device found!");
    }

    return str_pr->second;
}

string get_cmd_from_key(string key)
{
    vector<string> parts;
    boost::split(parts, key, boost::is_any_of("."));
    return parts[1];
}

template <class Fun>
void call_handler_with_device(Fun &&func, shared_ptr<Keymaster> km,
                              string key, YAML::Node &n)
{
    YAML::Node rval;
    uint64_t sn = n[0].as<uint64_t>();
    vector<string> parts;

    // create return value keys. Pop off the 'params' part and replace
    // it with the 'rval' part.
    boost::split(parts, key, boost::is_any_of("."));
    string cmd = parts[1];
    parts.pop_back();
    parts.push_back("rval");
    string r_key = boost::algorithm::join(parts, ".");

    try
    {
        auto dev = get_airspyhf_device(sn);

        if (dev)
        {
            rval = func(dev, sn, cmd);
        }
        else
        {
            rval = airspyhf_response(false, cmd, "Unable to find device", sn);
        }
    }
    catch (std::runtime_error &e)
    {
        rval = airspyhf_response(false, cmd, "Runtime exception", e.what());
    }

    auto r = km->put(r_key, rval);

    if (not r)
    {
        stringstream os;
        os << "Keymaster::put(" << r_key << ", " << rval << ") failed.";
        throw runtime_error(os.str());
    }
}

template <class Fun>
void call_handler(Fun &&func, shared_ptr<Keymaster> km, string key)
{
    YAML::Node rval;
    vector<string> parts;

    // create return value keys. Pop off the 'params' part and replace
    // it with the 'rval' part.
    boost::split(parts, key, boost::is_any_of("."));
    string cmd = parts[1];
    parts.pop_back();
    parts.push_back("rval");
    string r_key = boost::algorithm::join(parts, ".");

    try
    {
        rval = func(cmd);
    }
    catch (std::runtime_error &e)
    {
        rval = airspyhf_response(false, cmd, "Runtime exception", e.what());
    }

    auto r = km->put(r_key, rval);

    if (not r)
    {
        stringstream os;
        os << "Keymaster::put(" << r_key << ", " << rval << ") failed.";
        throw runtime_error(os.str());
    }
}

void AirspyComponent::lib_version(string key, YAML::Node)
{
    auto the_handler =
        [](string cmd) -> YAML::Node
        {
            airspyhf_lib_version_t version;
            airspyhf_lib_version(&version);

            return airspyhf_response(true, cmd,
                    version.major_version,
                    version.minor_version,
                    version.revision);
        };

    call_handler(the_handler, keymaster, key);
}

void AirspyComponent::list_devices(string key, YAML::Node)
{
    auto the_handler =
        [](string cmd) -> YAML::Node
        {
            auto ndev = airspyhf_list_devices(0, 0);
            vector<uint64_t> serials(ndev, 0);

            // read all the serials and scan the receiver(s)
            if (airspyhf_list_devices((uint64_t *)serials.data(), ndev) > 0)
            {
                return airspyhf_response(true, cmd, serials);
            }

            return airspyhf_response(false, cmd, "No devices found.");
        };

    call_handler(the_handler, keymaster, key);
}

void AirspyComponent::open(string key, YAML::Node)
{
    auto the_handler =
        [](string cmd) -> YAML::Node
        {
            uint64_t sn = DEFAULT_DEVICE;
            airspyhf_device_t *dev;
            bool status = false;

            if (airspyhf_open(&dev) == AIRSPYHF_SUCCESS)
            {
                devices[sn] = dev;
                // reverse lookup
                streamers[dev] = sn;
                status = true;
            }

            return airspyhf_response(status, cmd, sn);
        };

    call_handler(the_handler, keymaster, key);
}

void AirspyComponent::open_sn(string key, YAML::Node data)
{
    auto the_handler =
        [data](string cmd) -> YAML::Node
        {
            airspyhf_device_t *dev;
            auto sn = data[0].as<uint64_t>();
            bool status = false;

            if (airspyhf_open_sn(&dev, sn) == AIRSPYHF_SUCCESS)
            {
                devices[sn] = dev;
                // reverse lookup
                streamers[dev] = sn;
                status = true;
            }

            return airspyhf_response(status, cmd, sn);
        };

    call_handler(the_handler, keymaster, key);
}

void AirspyComponent::close(string key, YAML::Node data)
{
    auto the_handler =
        [data](string cmd) -> YAML::Node
        {
            YAML::Node rval;
            uint64_t sn = data[0].as<uint64_t>();
            airspyhf_device_t *dev;
            auto dev_pr = devices.find(sn);

            if (dev_pr == devices.end())
            {
                return airspyhf_response(false, cmd, sn, "could not find the device");
            }

            dev = dev_pr->second;
            devices.erase(dev_pr);

            auto str_pr = streamers.find(dev);

            if (str_pr != streamers.end())
            {
                streamers.erase(str_pr);
            }

            bool status =
                (airspyhf_close(dev)
                 == AIRSPYHF_SUCCESS) ? true : false;
            return airspyhf_response(status, cmd, sn);
        };

    call_handler(the_handler, keymaster, key);
}

void AirspyComponent::start(string key, YAML::Node data)
{
    auto the_handler =
        [this](airspyhf_device_t *dev, uint64_t sn, string cmd) -> YAML::Node
        {
            bool status =
                (airspyhf_start(dev, &rx_callback, this)
                 == AIRSPYHF_SUCCESS) ? true : false;
            return airspyhf_response(status, cmd, sn);
        };

    call_handler_with_device(the_handler, keymaster, key, data);
}

void AirspyComponent::stop(string key, YAML::Node data)
{
    auto the_handler =
        [](airspyhf_device_t *dev, uint64_t sn, string cmd) -> YAML::Node
        {
            bool status =
                (airspyhf_stop(dev) == AIRSPYHF_SUCCESS) ? true : false;
            return airspyhf_response(status, cmd, sn);
        };

    call_handler_with_device(the_handler, keymaster, key, data);
}

void AirspyComponent::is_streaming(string key, YAML::Node data)
{
    auto the_handler =
        [](airspyhf_device_t *dev, uint64_t sn, string cmd) -> YAML::Node
        {
            bool streaming = airspyhf_is_streaming(dev);
            return airspyhf_response(true, cmd, sn, streaming);
        };

    call_handler_with_device(the_handler, keymaster, key, data);
}

void AirspyComponent::set_freq(string key, YAML::Node data)
{
    auto the_handler =
        [data](airspyhf_device_t *dev, uint64_t sn, string cmd) -> YAML::Node
        {
            uint64_t freq_hz = data[1].as<uint64_t>();
            bool status =
                (airspyhf_set_freq(dev, freq_hz)
                 == AIRSPYHF_SUCCESS) ? true : false;
            return airspyhf_response(status, cmd, sn, freq_hz);
        };

    call_handler_with_device(the_handler, keymaster, key, data);
}

void AirspyComponent::set_lib_dsp(string key, YAML::Node data)
{
    auto the_handler =
        [data](airspyhf_device_t *dev, uint64_t sn, string cmd) -> YAML::Node
        {
            bool flag = data[1].as<bool>();
            bool status =
                (airspyhf_set_lib_dsp(dev, flag)
                 == AIRSPYHF_SUCCESS) ? true : false;
            return airspyhf_response(status, cmd, sn, flag);
        };

    call_handler_with_device(the_handler, keymaster, key, data);
}

void AirspyComponent::get_samplerates(string key, YAML::Node data)
{
    auto the_handler =
        [](airspyhf_device_t *dev, uint64_t sn, string cmd) -> YAML::Node
        {
            uint32_t num;

            if (airspyhf_get_samplerates(dev, &num, 0) == AIRSPYHF_SUCCESS)
            {
                vector<uint32_t> buffer(num, 0);
                bool status =
                    (airspyhf_get_samplerates(dev, (uint32_t *)buffer.data(), num)
                     == AIRSPYHF_SUCCESS) ? true : false;
                return airspyhf_response(status, cmd, sn, buffer);
            }

            return airspyhf_response(false, cmd, sn,
                        "Failed to read sample rate numbers.");
        };

    call_handler_with_device(the_handler, keymaster, key, data);
}

void AirspyComponent::set_samplerate(string key, YAML::Node data)
{
    auto the_handler =
        [data](airspyhf_device_t *dev, uint64_t sn, string cmd) -> YAML::Node
        {
            uint64_t samplerate = data[1].as<uint64_t>();
            bool status =
                (airspyhf_set_samplerate(dev, samplerate)
                 == AIRSPYHF_SUCCESS) ? true : false;
            return airspyhf_response(status, cmd, sn, samplerate);
        };

    call_handler_with_device(the_handler, keymaster, key, data);
}

void AirspyComponent::get_calibration(string key, YAML::Node data)
{
    auto the_handler =
        [](airspyhf_device_t *dev, uint64_t sn, string cmd) -> YAML::Node
        {
            int32_t calibration;
            bool status =
                (airspyhf_get_calibration(dev, &calibration)
                 == AIRSPYHF_SUCCESS) ? true : false;
            return airspyhf_response(status, cmd, sn, calibration);
        };

    call_handler_with_device(the_handler, keymaster, key, data);
}

void AirspyComponent::set_calibration(string key, YAML::Node data)
{
    auto the_handler =
        [data](airspyhf_device_t *dev, uint64_t sn, string cmd) -> YAML::Node
        {
            int32_t ppd = data[1].as<int32_t>();
            bool status =
                (airspyhf_set_calibration(dev, ppd)
                 == AIRSPYHF_SUCCESS) ? true : false;
            return airspyhf_response(status, cmd, sn, ppd);
        };

    call_handler_with_device(the_handler, keymaster, key, data);
}

void AirspyComponent::set_optimal_iq_correction_point(string key, YAML::Node data)
{
    auto the_handler =
        [data](airspyhf_device_t *dev, uint64_t sn, string cmd) -> YAML::Node
        {
            double w = data[1].as<double>();
            bool status =
                (airspyhf_set_optimal_iq_correction_point(dev, w)
                 == AIRSPYHF_SUCCESS) ? true : false;
            return airspyhf_response(status, cmd, sn, w);
        };

    call_handler_with_device(the_handler, keymaster, key, data);
}

void AirspyComponent::iq_balancer_configure(string key, YAML::Node data)
{
    auto the_handler =
        [data](airspyhf_device_t *dev, uint64_t sn, string cmd) -> YAML::Node
        {
            int	buffers_to_skip = data[1].as<int>();
            int	fft_integration = data[2].as<int>();
            int	fft_overlap = data[3].as<int>();
            int	correlation_integration = data[4].as<int>();
            bool status =
                (airspyhf_iq_balancer_configure(dev, buffers_to_skip,
                        fft_integration, fft_overlap, correlation_integration)
                 == AIRSPYHF_SUCCESS) ? true : false;
            return airspyhf_response(status, cmd, sn,
                    buffers_to_skip,
                    fft_integration,
                    fft_overlap,
                    correlation_integration);
        };

    call_handler_with_device(the_handler, keymaster, key, data);
}

void AirspyComponent::flash_calibration(string key, YAML::Node data)
{
    auto the_handler =
        [](airspyhf_device_t *dev, uint64_t sn, string cmd) -> YAML::Node
        {
            if (airspyhf_is_streaming(dev))
            {
                return airspyhf_response(false, cmd, sn,
                        "Streaming is active. Please stop "
                        "streaming and try again.");
            }

            bool status =
                (airspyhf_flash_calibration(dev)
                 == AIRSPYHF_SUCCESS) ? true : false;
            return airspyhf_response(status, cmd, sn);
        };

    call_handler_with_device(the_handler, keymaster, key, data);
}

void AirspyComponent::board_partid_serialno_read(string key, YAML::Node data)
{
    auto the_handler =
        [](airspyhf_device_t *dev, uint64_t sn, string cmd) -> YAML::Node
        {
            airspyhf_read_partid_serialno_t pidsn;
            airspyhf_board_partid_serialno_read(dev, &pidsn);
            vector<uint32_t> serial_no;
            int sn_len = sizeof(pidsn.serial_no) / sizeof(uint32_t);

            for (int i = 0; i < sn_len; ++i)
            {
                serial_no.push_back(pidsn.serial_no[i]);
            }

            return airspyhf_response(true, cmd, sn, pidsn.part_id, serial_no);
        };

    call_handler_with_device(the_handler, keymaster, key, data);
}

void AirspyComponent::version_string_read(string key, YAML::Node data)
{
    auto the_handler =
        [](airspyhf_device_t *dev, uint64_t sn, string cmd) -> YAML::Node
        {
            YAML::Node rval;
            char ver[255];
            memset(ver, 0, 255);
            bool status =
                (airspyhf_version_string_read(dev, ver, sizeof(ver))
                 == AIRSPYHF_SUCCESS) ? true : false;
            string version = ver;
            return airspyhf_response(status, cmd, sn, version);
        };

    call_handler_with_device(the_handler, keymaster, key, data);
}

void AirspyComponent::set_user_output(string key, YAML::Node data)
{
    auto the_handler =
        [data](airspyhf_device_t *dev, uint64_t sn, string cmd) -> YAML::Node
        {
            airspyhf_user_output_t pin =
                (airspyhf_user_output_t)data[1].as<int>();
            airspyhf_user_output_state_t value =
                (airspyhf_user_output_state_t)data[2].as<int>();

            bool status =
                (airspyhf_set_user_output(dev, pin, value)
                 == AIRSPYHF_SUCCESS) ? true : false;
            return airspyhf_response(status, cmd, sn, (int)pin, (int)value);
        };

    call_handler_with_device(the_handler, keymaster, key, data);
}

void AirspyComponent::set_hf_agc(string key, YAML::Node data)
{
    auto the_handler =
        [data](airspyhf_device_t *dev, uint64_t sn, string cmd) -> YAML::Node
        {
            bool flag = data[1].as<bool>();
            bool status =
                (airspyhf_set_hf_agc(dev, flag)
                 == AIRSPYHF_SUCCESS) ? true : false;
            return airspyhf_response(status, cmd, sn, flag);
        };

    call_handler_with_device(the_handler, keymaster, key, data);
}

void AirspyComponent::set_hf_agc_threshold(string key, YAML::Node data)
{
    auto the_handler =
        [data](airspyhf_device_t *dev, uint64_t sn, string cmd) -> YAML::Node
        {
            YAML::Node rval;
            bool flag = data[1].as<bool>();
            bool status =
                (airspyhf_set_hf_agc_threshold(dev, flag)
                 == AIRSPYHF_SUCCESS) ? true : false;
            return airspyhf_response(status, cmd, sn, flag);
        };

    call_handler_with_device(the_handler, keymaster, key, data);
}

void AirspyComponent::set_hf_att(string key, YAML::Node data)
{
    auto the_handler =
        [data](airspyhf_device_t *dev, uint64_t sn, string cmd) -> YAML::Node
        {
            bool flag = data[1].as<bool>();
            bool status =
                (airspyhf_set_hf_att(dev, flag)
                 == AIRSPYHF_SUCCESS) ? true : false;
            return airspyhf_response(status, cmd, sn, flag);
        };

    call_handler_with_device(the_handler, keymaster, key, data);
}


    // typedef struct {
    //     airspyhf_device_t* device;
    //     void* ctx;
    //     airspyhf_complex_float_t* samples;
    //     int sample_count;
    //     uint64_t dropped_samples;
    // } airspyhf_transfer_t;


int rx_callback(airspyhf_transfer_t *transfer)
{
    int rval{0};

    AirspyComponent *airc = (AirspyComponent *)transfer->ctx;
    airc->write_to_source(transfer);

    return rval;
}
