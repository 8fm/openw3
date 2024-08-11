/**
 * Copyright © 2016 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_MEMORY_MEMORY_INTERNAL_H_
#define _RED_MEMORY_MEMORY_INTERNAL_H_

#ifndef _HAS_EXCEPTIONS 
#define _HAS_EXCEPTIONS 0
#endif // !_HAS_EXCEPTIONS 

#include "../../redSystem/settings.h"
#include "../../redSystem/architecture.h"
#include "../../redSystem/compilerExtensions.h"
#include "../../redSystem/os.h"
#include "../../redSystem/types.h"

#include <xutility>
#include <xstddef>
#include <type_traits>
#include <array>
#include <new>

#include "redMemoryApi.h"
#include "settings.h"
#include "integrationStub.h"
#include "types.h"

#endif
