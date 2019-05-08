/*******************************************************************
 *  fft_component.cc - Perform 1D FFT of the incoming IQ data and
 *  publish the results.
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

#include "fft_component.h"
#include "fftwp.h"
#include "matrix/log_t.h"
#include <memory>
#include <matrix/matrix_util.h>

using namespace std;
using namespace matrix;
using namespace mxutils;

static matrix::log_t logger("FFTComponent");


Component *FFTComponent::factory(std::string name, std::string km_url)
{
    return new FFTComponent(name, km_url);
}

FFTComponent::FFTComponent(std::string name, std::string keymaster_url) :
    Component(name, keymaster_url),
    _run(false),
    _run_thread_started(false),
    _run_thread(this, &FFTComponent::receiving_task),
    iq_signal_source(keymaster_url, name, "iq_data")
{
}

FFTComponent::~FFTComponent()
{
}

bool FFTComponent::_do_start()
{
    connect();
    Keymaster km(keymaster_url);

    _run = true;

    if (!_run_thread.running())
    {
        logger.info(__PRETTY_FUNCTION__, "starting thread.");
        _run_thread.start("FFT _run_thread");
    }

    bool rval = _run_thread_started.wait(true, 5000000);

    if (rval)
    {
        logger.info(__PRETTY_FUNCTION__, "_run_thread started.");
    }
    else
    {
        logger.error(__PRETTY_FUNCTION__,
                     "_run_thread failed to start!");
        _run = false;
        stop();
        _run_thread.join();
        _run_thread_started.set_value(false);
        disconnect();
    }

    return rval;
}

bool FFTComponent::_do_stop()
{
    logger.info(__PRETTY_FUNCTION__,
                "_run_thread thread terminated");
    _run = false;
    stop();
    _run_thread.join();
    _run_thread_started.set_value(false);
    disconnect();
    return true;
}

bool FFTComponent::connect()
{
    input_signal_sink.reset(
        new matrix::DataSink<std::vector<sdrm::complex_float_t>,
                            matrix::select_only>(keymaster_url, 10));
    connect_sink(*input_signal_sink, "input_data");
    return true;
}

bool FFTComponent::disconnect()
{
    input_signal_sink->disconnect();
    input_signal_sink.reset();
    return true;
}


void FFTComponent::receiving_task()
{
    logger.info(__PRETTY_FUNCTION__, "running");
    _run_thread_started.signal(true);

    while (_run.load())
    {
        // wait for a data bufferstring scan_status
        vector<sdrm::complex_float_t> inbuf;

        if (input_signal_sink->timed_get(inbuf, Time::TM_ONE_SEC))
        {
            auto fft_data = one_dimensional_dfft(std::move(inbuf));
            iq_signal_source.publish(fft_data);
        }
        else
        {
            logger.debug(__PRETTY_FUNCTION__, "Timed out waiting for FFT data.");
        }
    }
}
