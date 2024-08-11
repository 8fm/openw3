/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma  once

#ifndef _HAS_EXCEPTIONS 
#define _HAS_EXCEPTIONS 0
#endif // !_HAS_EXCEPTIONS 

#include "../unitTestsFramework/test.h"
#include "../../common/redSystem/settings.h"
#include "../../common/redSystem/architecture.h"
#include "../../common/redSystem/compilerExtensions.h"
#include "../../common/redSystem/os.h"
#include "../../common/redSystem/types.h"

#ifdef RED_COMPILER_MSC
#pragma warning( disable : 4127 ) // conditional expression is constant: from do{}while(false)
#endif // RED_COMPILER_MSC

