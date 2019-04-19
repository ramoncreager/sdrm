/*******************************************************************
 *  simple_msgpk_client.cc - Simple message pack client to test
 *  publishers of data.
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

#include "simple_msgpk_client.h"
#include "matrix/log_t.h"
#include <memory>
#include <matrix/matrix_util.h>

using namespace std;
using namespace matrix;
using namespace mxutils;

static matrix::log_t logger("simple_msgpk_client");

ostream & operator << (ostream &o,  const sdrm::complex_float_t &v)
{
    o << "{" << v.re << ", " << v.im << "}";
    return o;
}


Component *MsgpackComponent::factory(std::string name,std::string km_url)
{
    return new MsgpackComponent(name, km_url);
}

MsgpackComponent::MsgpackComponent(std::string name, std::string keymaster_url) :
    Component(name, keymaster_url),
    _run(false),
    _run_thread_started(false),
    _run_thread(this, &MsgpackComponent::receiving_task)
{
}

MsgpackComponent::~MsgpackComponent()
{
}

bool MsgpackComponent::_do_start()
{
    connect();
    Keymaster km(keymaster_url);

    _run = true;

    if (!_run_thread.running())
    {
        logger.info(__PRETTY_FUNCTION__,
                    "DownsamplingThread::_do_start(): starting thread.");
        _run_thread.start("simple_msgpk_client_thread");
    }

    bool rval = _run_thread_started.wait(true, 5000000);

    if (rval)
    {
        logger.info(__PRETTY_FUNCTION__,
                    "simple_msgpk_client_thread started.");
    }
    else
    {
        logger.error(__PRETTY_FUNCTION__,
                     "simple_msgpk_client_thread thread failed to start!");
        _run = false;
        stop();
        _run_thread.join();
        _run_thread_started.set_value(false);
        disconnect();
    }

    return rval;
}

bool MsgpackComponent::_do_stop()
{
    logger.info(__PRETTY_FUNCTION__,
                "simple_msgpk_client_thread thread terminated");
    _run = false;
    stop();
    _run_thread.join();
    _run_thread_started.set_value(false);
    disconnect();
    return true;
}

bool MsgpackComponent::connect()
{
    input_signal_sink.reset(new matrix::DataSink<string,
                            matrix::select_only>(keymaster_url, 10));
    connect_sink(*input_signal_sink, "input_data");
    return true;
}

bool MsgpackComponent::disconnect()
{
    input_signal_sink->disconnect();
    input_signal_sink.reset();
    return true;
}


void MsgpackComponent::receiving_task()
{
    logger.info(__PRETTY_FUNCTION__, "running");
    _run_thread_started.signal(true);

    while (_run.load())
    {
        // wait for a data bufferstring scan_status
        string inbuf;

        if (input_signal_sink->timed_get(inbuf, Time::TM_ONE_SEC))
        {
            msgpack::object_handle oh = msgpack::unpack(inbuf.data(), inbuf.size());
            msgpack::object obj = oh.get();
            sdrm::iq_data_t iq_data;
            obj.convert(iq_data);

            cout << "sample_count: " << iq_data.sample_count << "; ";
            cout << "dropped_samples: " << iq_data.dropped_samples << "; ";

            for (size_t i = 0; i < 3; ++i)
            {
                cout << iq_data.samples[i] << ",";
            }

            cout << " ..." << endl;
        }
    }
}
