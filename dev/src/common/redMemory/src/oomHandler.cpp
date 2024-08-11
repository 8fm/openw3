/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "oomHandler.h"
#include "poolRegistry.h"

namespace red
{
namespace memory
{
	OOMHandler::OOMHandler()
		:	m_poolRegistry( nullptr ),
			m_isHandlingFailure( false )
	{}

	OOMHandler::~OOMHandler()
	{}

	void OOMHandler::HandleAllocateFailure( PoolHandle poolHandle, u32 size, u32 alignment )
	{
		CScopedLock< CMutex > lock( m_monitor );
		if( !m_isHandlingFailure )
		{
			m_isHandlingFailure = true;
			ScopedFlag< bool > recursionCheck( m_isHandlingFailure, false );

			const char * poolName = m_poolRegistry ? m_poolRegistry->GetPoolName( poolHandle ) : "<Unknown Pool>";
			OnHandleAllocateFailure( poolName, size, alignment );	
		}
	}

	void OOMHandler::SetPoolRegistry( const PoolRegistry * registry )
	{
		m_poolRegistry = registry;
	}
}
}
