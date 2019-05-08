/*******************************************************************
 *  fft_component.h -
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

#if !defined _FFT_COMPONENT_H_
#define _FFT_COMPONENT_H_

#include "sdrm_types.h"

#include "matrix/Thread.h"
#include "matrix/Component.h"
#include "matrix/DataSource.h"

#include <iostream>
#include <map>
#include <memory>

class FFTComponent : public matrix::Component
{
public:

    virtual ~FFTComponent();
    static Component *factory(std::string myname,std::string k);

protected:
    FFTComponent(std::string name, std::string keymaster_url);

    // override various base class methods
    virtual bool _do_start() override;
    virtual bool _do_stop()  override;

    bool connect();
    bool disconnect();

    std::atomic<bool> _run;
    matrix::TCondition<bool> _run_thread_started;
    matrix::Thread<FFTComponent> _run_thread;
    std::unique_ptr<matrix::DataSink<std::vector<sdrm::complex_float_t>,
                                     matrix::select_only>> input_signal_sink;
    matrix::DataSource<std::vector<sdrm::complex_float_t>> iq_signal_source;

    void receiving_task();
};

#endif
