
//
// This source file is part of appleseed.
// Visit http://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2016 Haggi Krey, The appleseedhq Organization
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#ifndef VERSION_H
#define VERSION_H

#include <string>
#include <vector>
#include "OpenImageIO//oiioversion.h"
#include "OSL/oslversion.h"
#include "boost/version.hpp"
#include <OpenEXR/ImfVersion.h>
#include "foundation/core/appleseed.h"

#define VENDOR "haggis vfx & animation"

#define MTXX_VERSION_MAJOR 0
#define MTXX_VERSION_MINOR 32
#define MTXX_VERSION_PATCH 0
#define MTXX_VERSION_RELEASE_TYPE dev

#define MTXX_MAKE_VERSION_STRING2(a,b,c,d) #a "." #b "." #c #d
#define MTXX_MAKE_VERSION_STRING(a,b,c,d) MTXX_MAKE_VERSION_STRING2(a,b,c,d)
#define MTXX_VERSION_STRING \
    MTXX_MAKE_VERSION_STRING(MTXX_VERSION_MAJOR, \
                             MTXX_VERSION_MINOR, MTXX_VERSION_PATCH, \
                             MTXX_VERSION_RELEASE_TYPE)


inline std::vector<std::string> getFullVersionString()
{
    std::vector<std::string> versionData;
    versionData.push_back(std::string("MayaToAppleseed: ") + MTXX_VERSION_STRING);
    versionData.push_back(std::string("Appleseed: ") + foundation::Appleseed::get_synthetic_version_string());
    versionData.push_back(std::string("OpenImageIO: ") + OIIO_VERSION_STRING);
    versionData.push_back(std::string("OSL: ") + OSL_LIBRARY_VERSION_STRING);
    versionData.push_back(std::string("BOOST: ") + BOOST_LIB_VERSION);
    versionData.push_back(std::string("OpenExr: ") + OPENEXR_VERSION_STRING);
    return versionData;
}

#endif
