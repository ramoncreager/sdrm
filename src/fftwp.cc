/*******************************************************************
 *  fftwp.cc - Port handler for fftw3 routines, uses the Puerta port
 *  framework.
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

#include "fftwp.h"
#include "matrix/log_t.h"

#include <vector>
#include <memory>
#include <string.h>
#include <fftw3.h>

using namespace std;
using namespace sdrm;
using namespace matrix;

static log_t logger("fft_data_1d");

/**
 * \struct fft_data_1d
 *
 * Contains and manages the data and plan required to do
 * one-dimensional ffts of size N, where N is constant during the
 * lifetime of this object.
 *
 */

struct fft_data_1d
{
    fft_data_1d(int n)
    {
        N = n;
        in = (fftwf_complex*) fftw_malloc(sizeof(fftwf_complex) * N);
        out = (fftwf_complex*) fftw_malloc(sizeof(fftwf_complex) * N);
        p = fftwf_plan_dft_1d(N, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
    }


    ~fft_data_1d()
    {
        fftwf_destroy_plan(p);
        fftw_free(in);
        fftw_free(out);
    }

    void execute()
    {
        fftwf_execute(p);
    }

    int N{0};
    fftwf_plan p;
    fftwf_complex *in{NULL};
    fftwf_complex *out{NULL};
};

shared_ptr<fft_data_1d> data1d;

/**
 * Receives and computes the data for a 1-d fft.
 *
 * Once the fftw data input is loaded, the fft is performed, and the
 * results (always complex with doubles) is returned.
 *
 * @param samples: An rval containing the input data. From this the
 * size N of the FFT will be known, and a new plan generated if the
 * previous size was not N.
 *
 * @return Returns a vector<complex_float_t> with the results.
 *
 */

vector<complex_float_t> one_dimensional_dfft(vector<complex_float_t> &&samples)
{
    int N = samples.size();
    size_t binsize = N * sizeof(complex_float_t);

    if (not data1d or N != data1d->N)
    {
        logger.info(__PRETTY_FUNCTION__, "initializing 'data1d'");
        data1d.reset(new fft_data_1d(N));
    }

    memcpy((void *)data1d->in, (const void *)samples.data(), binsize);
    data1d->execute();
    vector<complex_float_t> rval(N);
    memcpy((void *)rval.data(), (const void *)data1d->out, binsize);
    return rval;
}
