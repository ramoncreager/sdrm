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

#define BUF_SIZE 128
#define DEFAULT_DEVICE 0xFFFFFFFFFFFFFFFF

using namespace std;
using namespace matrix;

map<uint64_t, airspyhf_device_t *> devices;
map<airspyhf_device_t *, uint64_t> streamers;

int rx_callback(airspyhf_transfer_t *transfer);

template<typename T, typename... Args>
void airspyhf_response(YAML::Node &rval, bool flag, T &a, Args... args)
{
    string error_code = flag ? "AIRSPYHF_SUCCESS" : "AIRSPYHF_ERROR";
    rval["error_code"].push_back("AIRSPY_SUCCESS");
    rval.push_back(a);
    airspyhf_response(rval, args...);
}

template<typename T, typename... Args>
void airspyhf_response(YAML::Node &rval, T &a, Args... args)
{
    rval.push_back(a);
    airspyhf_response(rval, args...);
}

template<typename T>
void airspyhf_response(YAML::Node &rval, T&a)
{
    rval.push_back(a);
}

// void airspyhf_response(YAML::Node &)
// {

// }

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
    parts.pop_back();
    parts.push_back("rval");
    string r_key = boost::algorithm::join(parts, ".");

    try
    {
        auto dev = get_airspyhf_device(sn);

        if (dev)
        {
            airspy_response(rval, true, func(dev, n));
        }
        else
        {
            airspyhf_response(rval, false, "Unable to find device", sn);
        }
    }
    catch (std::runtime_error e)
    {
        airspyhf_response(rval, false, "Runtime exception", e.what());
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
std::string call_handler(Fun &&func, shared_ptr<Keymaster> km,
                         string key, YAML::Node &n)
{
    YAML::Node rval;
    vector<string> parts;

    // create return value keys. Pop off the 'params' part and replace
    // it with the 'rval' part.
    boost::split(parts, key, boost::is_any_of("."));
    parts.pop_back();
    parts.push_back("rval");
    string r_key = boost::algorithm::join(parts, ".");

    try
    {
        airspyhf_response(rval, true, func(n));
    }
    catch (std::runtime_error e)
    {
        airspyhf_response(rval, false, "Runtime exception", e.what());
    }

    auto r = km->put(r_key, rval);

    if (not r)
    {
        stringstream os;
        os << "Keymaster::put(" << r_key << ", " << rval << ") failed.";
        throw runtime_error(os.str());
    }
}

void AirspyComponent::lib_version(string key, YAML::Node data)
{
    auto the_handler =
        [](string, YAML::Node) -> YAML::Node
        {
            YAML::Node rval;
            airspyhf_lib_version_t version;
            airspyhf_lib_version(&version);
            airspyhf_response(rval, true,
                    version.major_version,
                    version.minor_version,
                    version.revision);
            return rval;
        };

    call_handler(the_handler, keymaster, key, data);
}

void AirspyComponent::list_devices(string key, YAML::Node data)
{
    auto the_handler =
        [](string, YAML::Node) -> YAML::Node
        {
            YAML::Node rval;
            auto ndev = airspyhf_list_devices(0, 0);
            int num_devs = 0;
            uint64_t *serials;

            serials = (uint64_t *)malloc(ndev*sizeof(uint64_t));
            // read all the serials and scan the receiver(s)
            if (serials && (num_devs = airspyhf_list_devices(serials, ndev)) > 0)
            {
                airspyhf_response(rval, true, serials);
            }
            else
            {
                airspyhf_response(rval, false, "No devices found.");
            }

            return rval;
        };

    call_handler(the_handler, keymaster, key, data);
}

void AirspyComponent::open(string key, YAML::Node data)
{
    auto the_handler =
        [](string, YAML::Node) -> YAML::Node
        {
            YAML::Node rval;
            uint64_t sn = DEFAULT_DEVICE;
            airspyhf_device_t *dev;

            if (airspyhf_open(&dev) == AIRSPYHF_ERROR)
            {
                airspyhf_response(rval, false, "Could not open", sn);
            }
            else
            {
                devices[sn] = dev;
                // reverse lookup
                streamers[dev] = sn;
                airspyhf_response(rval, true, "Opened", sn);
            }

            return rval;
        };

    call_handler(the_handler, keymaster, key, data);
}

void AirspyComponent::open_sn(string key, YAML::Node data)
{
    auto the_handler =
        [](string, YAML::Node data) -> YAML::Node
        {
            YAML::Node rval;
            airspyhf_device_t *dev;
            auto sn = data[0].as<uint64_t>();

            if (airspyhf_open_sn(&dev, sn) == AIRSPYHF_ERROR)
            {
                stringstream os;
                os << "Could not open " << sn;
                rval["error"].push_back("AIRSPYHF_ERROR");
                rval["error"].push_back(os.str());
            }

            devices[sn] = dev;
            // reverse lookup
            streamers[dev] = sn;
            rval["error"].push_back("AIRSPYHF_SUCCESS");
            return rval;
        };

    call_handler(the_handler, keymaster, key, data);
}

void AirspyComponent::close(string key, YAML::Node data)
{
    auto the_handler =
        [](string, YAML::Node data) -> YAML::Node
        {
            YAML::Node rval;
            uint64_t sn = data[0].as<uint64_t>();
            airspyhf_device_t *dev;
            auto dev_pr = devices.find(sn);

            if (dev_pr == devices.end())
            {
                stringstream os;
                os << "could not find the device " << sn;
                rval["error"].push_back("AIRSPYHF_ERROR");
                rval["error"].push_back(os.str());
            }
            else
            {
                dev = dev_pr->second;
                devices.erase(dev_pr);

                auto str_pr = streamers.find(dev);

                if (str_pr != streamers.end())
                {
                    streamers.erase(str_pr);
                }

                if (airspyhf_close(dev) == AIRSPYHF_ERROR)
                {
                    stringstream os;
                    os << "Could not close " << sn;
                    rval["error"].push_back("AIRSPYHF_ERROR");
                    rval["error"].push_back(os.str());
                }
                else
                {
                    stringstream os;
                    os << "closed " << sn;
                    rval["error"].push_back("AIRSPYHF_SUCCESS");
                    rval["error"].push_back(os.str());
                }
            }

            return rval;
        };

    call_handler(the_handler, keymaster, key, data);
}

void AirspyComponent::start(string key, YAML::Node data)
{
    auto the_handler =
        [this](airspyhf_device_t *dev, string, YAML::Node data) -> YAML::Node
        {
            YAML::Node rval;
            uint64_t sn = data[0].as<uint64_t>();

            if (airspyhf_start(dev, &rx_callback, this) == AIRSPYHF_SUCCESS)
            {
                stringstream os;
                rval["error"].push_back("AIRSPYHF_SUCCESS");
                rval["error"].push_back("Streaming started");

                rval = Puerta::response(true, cmd, "Streaming started.");
            }
            else
            {
                rval = Puerta::response(false, cmd, "airspyhf_start failed.");
            }

            return rval;
        };

    call_handler_with_device(the_handler, keymaster, key, data);
}

void AirspyComponent::stop(string key, YAML::Node data)
{
    auto the_handler =
        [&](airspyhf_device_t *dev, string, YAML::Node) -> YAML::Node
        {
            string rval;

            if (airspyhf_stop(dev) == AIRSPYHF_SUCCESS)
            {
                rval = Puerta::response(true, cmd, "Streaming stopped.");
            }
            else
            {
                rval = Puerta::response(false, cmd, "airspyhf_stop failed.");
            }

            return rval;
        };

    call_handler_with_device(the_handler, keymaster, key, data);
}

void AirspyComponent::is_streaming(string key, YAML::Node data)
{
    auto the_handler =
        [&](airspyhf_device_t *dev, string, YAML::Node) -> YAML::Node
        {
            ei_x_buff result;
            int streaming;
            streaming = airspyhf_is_streaming(dev);
            prepare_result(&result, true, cmd, 1);
            as_x_encode_boolean(&result, streaming);
            return ei_x_to_string(&result);
        };

    call_handler_with_device(the_handler, keymaster, key, data);
}

void AirspyComponent::set_freq(string key, YAML::Node data)
{
    auto the_handler =
        [&](airspyhf_device_t *dev, string, YAML::Node) -> YAML::Node
        {
            string rval;
            ei_x_buff result;
            uint64_t freq_hz = as_decode_ulong(data, index);

            if (airspyhf_set_freq(dev, freq_hz) == AIRSPYHF_SUCCESS)
            {
                prepare_result(&result, true, cmd, 1);
                as_x_encode_ulong(&result, freq_hz);
                rval = ei_x_to_string(&result);
            }
            else
            {
                stringstream os;
                os << "Unable to set frequency " << freq_hz << " Hz";
                rval = Puerta::response(false, cmd, os.str());
            }

            return rval;
        };

    call_handler_with_device(the_handler, keymaster, key, data);
}

void AirspyComponent::set_lib_dsp(string key, YAML::Node data)
{
    auto the_handler =
        [&](airspyhf_device_t *dev, string, YAML::Node) -> YAML::Node
        {
            string rval;
            ei_x_buff result;
            int flag;

            flag = as_decode_boolean(data, index);

            if (airspyhf_set_lib_dsp(dev, flag) == AIRSPYHF_SUCCESS)
            {
                prepare_result(&result, true, cmd, 1);
                as_x_encode_boolean(&result, flag);
                rval = ei_x_to_string(&result);
            }
            else
            {
                stringstream os;
                os << "Unable to set flag " << flag;
                rval = Puerta::response(false, cmd, os.str());
            }

            return rval;
        };

    call_handler_with_device(the_handler, keymaster, key, data);
}

void AirspyComponent::get_samplerates(string key, YAML::Node data)
{
    auto the_handler =
        [&](airspyhf_device_t *dev, string, YAML::Node) -> YAML::Node
        {
            ei_x_buff result;
            uint32_t num;
            uint32_t *buffer;
            string rval;

            if (airspyhf_get_samplerates(dev, &num, 0) == AIRSPYHF_SUCCESS)
            {
                buffer = (uint32_t *)calloc(num, sizeof(uint32_t));

                if (airspyhf_get_samplerates(dev, buffer, num) == AIRSPYHF_SUCCESS)
                {
                    prepare_result(&result, true, cmd, 1);
                    as_x_encode_list_header(&result, num);

                    for (int i = 0; i < num; ++i)
                    {
                        as_x_encode_ulong(&result, buffer[i]);
                    }

                    as_x_encode_empty_list(&result);
                    rval = ei_x_to_string(&result);
                }
                else
                {
                    string msg{"Failed to get sample rates."};
                    rval = Puerta::response(false, cmd, msg);
                }

                free(buffer);
            }
            else
            {
                string msg{"Failed to read sample rate numbers."};
                rval = Puerta::response(false, cmd, msg);
            }

            return rval;
        };

    call_handler_with_device(the_handler, keymaster, key, data);
}

void AirspyComponent::set_samplerate(string key, YAML::Node data)
{
    auto the_handler =
        [&](airspyhf_device_t *dev, string, YAML::Node) -> YAML::Node
        {
            ei_x_buff result;
            string rval;
            uint64_t samplerate = as_decode_ulong(data, index);

            if (airspyhf_set_samplerate(dev, samplerate) == AIRSPYHF_SUCCESS)
            {
                prepare_result(&result, true, cmd, 1);
                as_x_encode_ulong(&result, samplerate);
                rval = ei_x_to_string(&result);
            }
            else
            {
                stringstream os;
                os << "Could not set samplerate " << samplerate;
                rval = Puerta::response(false, cmd, os.str());
            }

            return rval;
        };

    call_handler_with_device(the_handler, keymaster, key, data);
}

void AirspyComponent::get_calibration(string key, YAML::Node data)
{
    auto the_handler =
        [&](airspyhf_device_t *dev, string, YAML::Node) -> YAML::Node
        {
            ei_x_buff result;
            string rval;
            int32_t calibration;

            if (airspyhf_get_calibration(dev, &calibration) == AIRSPYHF_SUCCESS)
            {
                prepare_result(&result, true, cmd, 1);
                as_x_encode_ulong(&result, calibration);
                rval = ei_x_to_string(&result);
            }

            return rval;
        };

    call_handler_with_device(the_handler, keymaster, key, data);
}

void AirspyComponent::set_calibration(string key, YAML::Node data)
{
    auto the_handler =
        [&](airspyhf_device_t *dev, string, YAML::Node) -> YAML::Node
        {
            ei_x_buff result;
            string rval;
            int32_t ppd = as_decode_ulong(data, index);

            if (airspyhf_set_calibration(dev, ppd) == AIRSPYHF_SUCCESS)
            {
                prepare_result(&result, true, cmd, 1);
                as_x_encode_ulong(&result, ppd);
                rval = ei_x_to_string(&result);
            }
            else
            {
                stringstream os;
                os << "Failed to set calibration value " << ppd;
                rval = Puerta::response(false, cmd, os.str());
            }

            return rval;
        };

    call_handler_with_device(the_handler, keymaster, key, data);
}

void AirspyComponent::set_optimal_iq_correction_point(string key, YAML::Node data)
{
    auto the_handler =
        [&](airspyhf_device_t *dev, string, YAML::Node) -> YAML::Node
        {
            ei_x_buff result;
            string rval;
            double w = as_decode_double(data, index);

            if (airspyhf_set_optimal_iq_correction_point(dev, w) == AIRSPYHF_SUCCESS)
            {
                prepare_result(&result, true, cmd, 1);
                as_x_encode_double(&result, w);
                rval = ei_x_to_string(&result);
            }
            else
            {
                stringstream os;
                os << "failed to set optimal iq correction point " << w;
                rval = Puerta::response(false, cmd, os.str());
            }

            return rval;
        };

    call_handler_with_device(the_handler, keymaster, key, data);
}

void AirspyComponent::iq_balancer_configure(string key, YAML::Node data)
{
    auto the_handler =
        [&](airspyhf_device_t *dev, string, YAML::Node) -> YAML::Node
        {
            ei_x_buff result;
            string rval;
            int buffers_to_skip = as_decode_long(data, index);
            int fft_integration = as_decode_long(data, index);
            int fft_overlap = as_decode_long(data, index);
            int correlation_integration = as_decode_long(data, index);

            if (airspyhf_iq_balancer_configure(dev, buffers_to_skip,
                    fft_integration, fft_overlap, correlation_integration)
                == AIRSPYHF_SUCCESS)
            {
                prepare_result(&result, true, cmd, 4);
                as_x_encode_long(&result, buffers_to_skip);
                as_x_encode_long(&result, fft_integration);
                as_x_encode_long(&result, fft_overlap);
                as_x_encode_long(&result, correlation_integration);
                rval = ei_x_to_string(&result);
            }
            else
            {
                stringstream os;
                os << "failed to set optimal iq balancer configuration "
                   << buffers_to_skip << ", " << fft_integration << ", "
                   << fft_overlap << ", " << correlation_integration;
                rval = Puerta::response(false, cmd, os.str());
            }

            return rval;
        };

    call_handler_with_device(the_handler, keymaster, key, data);
}

void AirspyComponent::flash_calibration(string key, YAML::Node data)
{
    auto the_handler =
        [&](airspyhf_device_t *dev, string, YAML::Node) -> YAML::Node
        {
            string rval;

            if (airspyhf_is_streaming(dev))
            {
                string msg{"Streaming is active. Please stop streaming and try again."};
                rval = Puerta::response(false, cmd, msg);
            }
            else
            {
                if (airspyhf_flash_calibration(dev) == AIRSPYHF_SUCCESS)
                {
                    rval = Puerta::response(true, cmd, "Write succeeded.");
                }
                else
                {
                    rval = Puerta::response(false, cmd, "Write failed.");
                }
            }

            return rval;
        };

    call_handler_with_device(the_handler, keymaster, key, data);
}

void AirspyComponent::board_partid_serialno_read(string key, YAML::Node data)
{
    auto the_handler =
        [&](airspyhf_device_t *dev, string, YAML::Node) -> YAML::Node
        {
            ei_x_buff result;
            airspyhf_read_partid_serialno_t pidsn;
            airspyhf_board_partid_serialno_read(dev, &pidsn);
            prepare_result(&result, true, cmd, 2);
            as_x_encode_ulong(&result, pidsn.part_id);
            int sn_len = sizeof(pidsn.serial_no) / sizeof(uint32_t);
            as_x_encode_list_header(&result, sn_len);

            for (int i = 0; i < sn_len; ++i)
            {
                as_x_encode_ulong(&result, pidsn.serial_no[i]);
            }

            as_x_encode_empty_list(&result);
            return ei_x_to_string(&result);
        };

    call_handler_with_device(the_handler, keymaster, key, data);
}

void AirspyComponent::version_string_read(string key, YAML::Node data)
{
    auto the_handler =
        [&](airspyhf_device_t *dev, string, YAML::Node) -> YAML::Node
        {
            ei_x_buff result;
            string rval;
            char ver[255];

            if (airspyhf_version_string_read(dev, ver, sizeof(ver)) == AIRSPYHF_SUCCESS)
            {
                prepare_result(&result, true, cmd, 1);
                as_x_encode_binary(&result, ver, strlen(ver));
                rval = ei_x_to_string(&result);
            }
            else
            {
                string msg{"Failed to read device version string"};
                rval = Puerta::response(false, cmd, msg);
            }

            return rval;
        };

    call_handler_with_device(the_handler, keymaster, key, data);
}

void AirspyComponent::set_user_output(string key, YAML::Node data)
{
    auto the_handler =
        [&](airspyhf_device_t *dev, string, YAML::Node) -> YAML::Node
        {
            ei_x_buff result;
            string rval;
            airspyhf_user_output_t pin =
                (airspyhf_user_output_t)as_decode_long(data, index);
            airspyhf_user_output_state_t value =
                (airspyhf_user_output_state_t)as_decode_long(data, index);

            if (airspyhf_set_user_output(dev, pin, value) == AIRSPYHF_SUCCESS)
            {
                prepare_result(&result, true, cmd, 2);
                as_x_encode_long(&result, pin);
                as_x_encode_long(&result, value);
                rval = ei_x_to_string(&result);
            }
            else
            {
                stringstream os;
                os << "Could not set user output, pin: " << pin
                   << ", value: " << value;
                rval = Puerta::response(false, cmd, os.str());
            }

            return rval;
        };

    call_handler_with_device(the_handler, keymaster, key, data);
}

void AirspyComponent::set_hf_agc(string key, YAML::Node data)
{
    auto the_handler =
        [&](airspyhf_device_t *dev, string, YAML::Node) -> YAML::Node
        {
            ei_x_buff result;
            string rval;
            uint8_t flag = as_decode_long(data, index);

            if (airspyhf_set_hf_agc(dev, flag) == AIRSPYHF_SUCCESS)
            {
                prepare_result(&result, true, cmd, 1);
                as_x_encode_long(&result, flag);
                rval = ei_x_to_string(&result);
            }
            else
            {
                stringstream os;
                os << "Failed to set hf agc to value: " << flag;
                rval = Puerta::response(false, cmd, os.str());
            }

            return rval;
        };

    call_handler_with_device(the_handler, keymaster, key, data);
}

void AirspyComponent::set_hf_agc_threshold(string key, YAML::Node data)
{
    auto the_handler =
        [&](airspyhf_device_t *dev, string, YAML::Node) -> YAML::Node
        {
            ei_x_buff result;
            string rval;
            uint8_t flag = as_decode_long(data, index);

            if (airspyhf_set_hf_agc_threshold(dev, flag) == AIRSPYHF_SUCCESS)
            {
                prepare_result(&result, true, cmd, 1);
                as_x_encode_long(&result, flag);
                rval = ei_x_to_string(&result);
            }
            else
            {
                stringstream os;
                os << "Failed to set hf agc threshold to value: " << flag;
                rval = Puerta::response(false, cmd, os.str());
            }

            return rval;
        };

    call_handler_with_device(the_handler, keymaster, key, data);
}

void AirspyComponent::set_hf_att(string key, YAML::Node data)
{
    auto the_handler =
        [&](airspyhf_device_t *dev, string, YAML::Node) -> YAML::Node
        {
            ei_x_buff result;
            string rval;
            uint8_t flag = as_decode_long(data, index);

            if (airspyhf_set_hf_att(dev, flag) == AIRSPYHF_SUCCESS)
            {
                prepare_result(&result, true, cmd, 1);
                as_x_encode_long(&result, flag);
                rval = ei_x_to_string(&result);
            }
            else
            {
                stringstream os;
                os << "Failed to set hf att. to value: " << flag;
                rval = Puerta::response(false, cmd, os.str());
            }

            return rval;
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
