#pragma once

#include "reFileBaseNode.h"

#ifdef USE_HDF5_HEADERS

bool hdfObjExists(H5::CommonFG* g, H5std_string name);

bool hdfObjExists(H5::CommonFG* g, H5std_string name, H5std_string type);
#endif
