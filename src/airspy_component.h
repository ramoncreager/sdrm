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

#include <iostream>
#include "matrix/Thread.h"
#include "matrix/Component.h"

class AirspyHF : public matrix::Component
{
public:
    AirspyHF(std::string name, std::string urn) :
        matrix::Component(name, urn),
        run_thread(this, &AirspyHF::run_loop), ticks(0)
    {
        run_thread.start();
    }

    void run_loop()
    {
        while(1)
        {
            sleep(1);
            YAML::Node tm = keymaster->get("components." + my_instance_name + ".time");
            std::cout << "Clock says " << tm.as<std::string>() << std::endl;
            keymaster->put("components." + my_instance_name + ".time", ++ticks, true);
        }
    }
    static Component *factory(std::string myname,std::string k)
    { std::cout << "AirspyHF ctor" << std::endl; return new AirspyHF(myname, k); }

protected:
    matrix::Thread<AirspyHF> run_thread;
    int ticks;
};
