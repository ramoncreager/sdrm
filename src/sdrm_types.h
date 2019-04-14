/*******************************************************************
 *  sdrm_types.h - Data types that are going to get sent around from
 *  component to component.
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

#if !defined(_SDRM_TYPES_H_)
#define _SDRM_TYPES_H_

#include <libairspyhf/airspyhf.h>
#include <utility>
#include <algorithm>
#include <msgpack.hpp>

// typedef struct {
//     airspyhf_device_t* device;
//     void* ctx;
//     airspyhf_complex_float_t* samples;
//     int sample_count;
//     uint64_t dropped_samples;
// } airspyhf_transfer_t;

namespace sdrm
{
    struct complex_float_t
    {
        float re;
        float im;
        MSGPACK_DEFINE(re, im);
    };

    struct iq_data_t
    {
        iq_data_t();
        iq_data_t(airspyhf_transfer_t *transfer);
        iq_data_t(iq_data_t &&other);
        ~iq_data_t();

        iq_data_t &operator=(iq_data_t &&other);

        int sample_count;
        uint64_t dropped_samples;
        std::vector<complex_float_t> samples;
        MSGPACK_DEFINE(sample_count, dropped_samples, samples);
    };
}

#endif
