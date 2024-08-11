/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#ifndef _RED_MEMORY_CALLBACK_LIST_H
#define _RED_MEMORY_CALLBACK_LIST_H

#include "../redSystem/types.h"

namespace Red { namespace MemoryFramework {

	///////////////////////////////////////////////////////////////////
	// This class keeps track of a number of callbacks which the manager can call
	template < class Callback, Red::System::Uint32 MaxCallbacks = 8 >
	class CallbackList
	{
	public:
		CallbackList();
		~CallbackList();

		typedef Callback* iterator;

		void RegisterCallback( Callback callbackFn );
		void UnRegisterCallback( Callback callbackFn );
		iterator Begin();
		iterator End();

	private:
		Callback m_callbacks[ MaxCallbacks ];
		Red::System::Uint32 m_callbackCount;
	};

} }

#include "redMemoryCallbackList.inl"

#endif