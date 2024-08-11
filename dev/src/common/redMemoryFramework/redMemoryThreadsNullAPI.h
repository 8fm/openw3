/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#ifndef _RED_MEMORY_FRAMEWORK_THREADS_NULLAPI_INL
#define _RED_MEMORY_FRAMEWORK_THREADS_NULLAPI_INL
#pragma once

#include "../redSystem/types.h"

//////////////////////////////////////////////////////////////////////////

namespace Red { namespace MemoryFramework { namespace NullAPI {

//////////////////////////////////////////////////////////////////////////
// NULL Mutex
//	This should compile down to nothing
class CMutexImpl
{
public:
	class CScopedLockImpl
	{
	public:
		CScopedLockImpl( CMutexImpl *mutex ) { RED_UNUSED(mutex); }
		~CScopedLockImpl() {}
	};

	typedef CScopedLockImpl TScopedLock;

public:
	CMutexImpl() {}
	~CMutexImpl() {}

	void		Acquire() {}
	void		Release() {}
};

} } } // namespace Red { namespace MemoryFramework { namespace NullAPI {

#endif // RED_MEMORY_FRAMEWORK_THREADS_NULL_INL