/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_MEMORY_HOOK_H_
#define _RED_MEMORY_HOOK_H_

#include "hookTypes.h"

namespace red
{
namespace memory
{
	class RED_MEMORY_API Hook
	{
	public:

		Hook();

		void Initialize( const HookCreationParameter & param );

		void PreAllocation( HookPreParameter & param );
		void PostAllocation( HookPostParameter & param );
		
		Hook * GetNext() const;
		void SetNext( Hook * hook );

		const void * InternalGetUserData() const;
		const HookPostCallback InternalGetPostCallback() const; 

	protected:

		~Hook();

	private:

		HookPreCallback m_preCallback;
		HookPostCallback m_postCallback;
		void * m_userData;
		Hook * m_nextHook;
	};

	// FOR UNIT TEST
	RED_MEMORY_API bool operator==( const Hook & left, const Hook & right );
}
}

#endif
