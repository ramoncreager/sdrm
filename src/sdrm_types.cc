/*******************************************************************
 *  sdrm_types.cc - Implementation of sdrm data types.
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

#include "sdrm_types.h"
#include <memory.h>

using namespace std;

namespace sdrm
{
    iq_data_t::iq_data_t()
        : sample_count(0),
          dropped_samples(0)
    {
    }

    iq_data_t::iq_data_t(airspyhf_transfer_t *transfer)

    {
        sample_count = transfer->sample_count;
        dropped_samples = transfer->dropped_samples;
        vector<complex_float_t> cv(sample_count);
        memcpy((void *)cv.data(), transfer->samples,
               sample_count * sizeof(complex_float_t));
        samples = std::move(cv);
    }

    iq_data_t::iq_data_t(iq_data_t &&other)
    {
        *this = std::move(other);
    }

    iq_data_t::~iq_data_t()
    {
    }

    iq_data_t &iq_data_t::operator=(iq_data_t &&other)
    {
        if (this != &other)
        {
            samples = std::move(other.samples);
            sample_count = other.sample_count;
            dropped_samples = other.dropped_samples;
            other.samples.clear();
            other.sample_count = 0;
            other.dropped_samples = 0;
        }

        return *this;
    }
}
