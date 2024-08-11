/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "hookHandler.h"
#include "hook.h"

namespace red
{
namespace memory
{
	HookHandler::HookHandler()
		:	m_rootHook( nullptr )
	{}

	HookHandler::~HookHandler()
	{}

	void HookHandler::Initialize()
	{}

	HookHandle HookHandler::Create( const HookCreationParameter & param )
	{
		ScopedWriteLock scopedLock( m_lock );

		Hook * hook = m_pool.TakeHook();
		hook->Initialize( param );
		Register( hook );

		return AddressOf( hook ); 
	}

	void HookHandler::Remove( HookHandle handle )
	{
		ScopedWriteLock scopedLock( m_lock );

		Hook * hook = reinterpret_cast< Hook* >( handle );
		Unregister( hook );
		m_pool.GiveHook( hook );
	}

	void HookHandler::Register( Hook * hook )
	{
		hook->SetNext( m_rootHook );
		m_rootHook = hook;
	}

	void HookHandler::Unregister( Hook * hook )
	{
		Hook * currentHook = m_rootHook;
		Hook * previousHook = nullptr;
		while( currentHook != nullptr )
		{
			Hook * nextHook = currentHook->GetNext();
			if( currentHook == hook )
			{
				if( previousHook )
				{
					previousHook->SetNext( nextHook );
				}
				else
				{
					m_rootHook = nextHook;
				}

				break;
			}

			previousHook = currentHook;
			currentHook = currentHook->GetNext();
		}
	}


	void HookHandler::ProcessPreHooks( HookPreParameter & param )
	{
		if( m_rootHook )
		{
			ScopedReadLock scopedLock( m_lock );
			if( m_rootHook )
			{
				m_rootHook->PreAllocation( param );
			}
		}
	}

	void HookHandler::ProcessPostHooks( HookPostParameter & param )
	{
		if( m_rootHook )
		{
			ScopedReadLock scopedLock( m_lock );
			if( m_rootHook )
			{
				m_rootHook->PostAllocation( param );
			}
		}
	}

}
}
