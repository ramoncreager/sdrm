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

#include <memory>
#include <matrix/matrix_util.h>

using namespace std;
using namespace matrix;
using namespace mxutils;

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
    run_thread(this, &MsgpackComponent::receiving_task),
    input_signal_sink()
{
}

MsgpackComponent::~MsgpackComponent()
{
}

bool MsgpackComponent::_do_start()
{
    return true;
}

bool MsgpackComponent::_do_stop()
{
    return true;
}

bool MsgpackComponent::_do_ready()
{
    return true;
}

bool MsgpackComponent::_do_standby()
{
    return true;
}


bool MsgpackComponent::connect()
{
    input_signal_sink.reset(new matrix::DataSink<msgpack::sbuffer,
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
}
