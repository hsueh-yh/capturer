/* ----------------------------------------------------------------------------
   libconfig - A library for processing structured configuration files
   Copyright (C) 2005-2010  Mark A Lindner

   This file is part of libconfig.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public License
   as published by the Free Software Foundation; either version 2.1 of
   the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with this library; if not, see
   <http://www.gnu.org/licenses/>.
   ----------------------------------------------------------------------------
*/
#ifndef _MTNDN_CONFIG_HPP_
#define _MTNDN_CONFIG_HPP_


#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <libconfig.h++>
#include "params.h"
#include "interfaces.h"


// This example reads the configuration file 'example.cfg' and displays
// some of its contents.

typedef struct _Params_
{
    GeneralParams generalParams_;
    PublisherParams publisherParams_;
    VideoCapturerParams capturerParam_;
    VideoCoderParams coderParams_;
    MediaStreamParams mediaStreamParams_;
    IExternalCapturer *capturer_;
}PParams;


int readConfiger( std::string configFIle, PParams* pObj );

#endif
