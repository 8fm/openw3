/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "redMemoryCallbackList.h"

namespace Red { namespace MemoryFramework {

	//////////////////////////////////////////////////////////////
	// CTor
	//
	template < class Callback, Red::System::Uint32 MaxCallbacks >
	CallbackList< Callback, MaxCallbacks >::CallbackList()
	{
	}

	//////////////////////////////////////////////////////////////
	// DTor
	//
	template < class Callback, Red::System::Uint32 MaxCallbacks >
	CallbackList< Callback, MaxCallbacks >::~CallbackList()
	{
	}
	
	//////////////////////////////////////////////////////////////
	// RegisterOutOfMemoryCallback
	//	Register a function that gets called when the manager fails to allocate
	template < class Callback, Red::System::Uint32 MaxCallbacks >
	void CallbackList< Callback, MaxCallbacks >::RegisterCallback( Callback callback )
	{
		if( m_callbackCount < MaxCallbacks )
		{
			m_callbacks[ m_callbackCount++ ] = callback;
		}
	}

	//////////////////////////////////////////////////////////////
	// UnRegisterOutOfMemoryCallback
	//	Remove a callback function from the list
	template < class Callback, Red::System::Uint32 MaxCallbacks >
	void CallbackList< Callback, MaxCallbacks >::UnRegisterCallback( Callback callback )
	{
		for( Red::System::Uint32 i=0; i<m_callbackCount; ++i )
		{
			if( m_callbacks[i] == callback )
			{
				m_callbacks[i] = m_callbacks[ --m_callbackCount ];
				return ;
			}
		}
	}

	//////////////////////////////////////////////////////////////
	// Begin
	//	Used to iterate through callbacks
	template < class Callback, Red::System::Uint32 MaxCallbacks >
	typename CallbackList< Callback, MaxCallbacks >::iterator CallbackList< Callback, MaxCallbacks >::Begin()
	{
		return &( m_callbacks[0] );
	}

	//////////////////////////////////////////////////////////////
	// End
	//	Used to iterate through callbacks
	template < class Callback, Red::System::Uint32 MaxCallbacks >
	typename CallbackList< Callback, MaxCallbacks >::iterator CallbackList< Callback, MaxCallbacks >::End()
	{
		return &( m_callbacks[m_callbackCount] );
	}

} }