/*******************************************************************
 *  SDRMArchitect.h - The SDR-Matrix Architect. Controls the state of
 *  the application.
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

#if !defined(_SDRMARCHITECT_H_)
#define _SDRMARCHITECT_H_
#include "matrix/Architect.h"
#include "matrix/Thread.h"
#include "matrix/Keymaster.h"

#include <string>
#include <memory>

namespace sdrm
{
    class SDRMArchitect : public matrix::Architect
    {
    public:
        SDRMArchitect(std::string name, std::string km_url);
        virtual ~SDRMArchitect();
        std::string _configuration_name;
    };

}

#endif
