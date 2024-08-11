/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "test.h"
#include "attorney.h"
#include "../../common/core/object.h"

namespace Red
{
namespace UnitTest
{
	void Attorney::BindClass( CObject * object, CClass * correctClass )
	{
		object->m_class = correctClass;
	}
}
}
