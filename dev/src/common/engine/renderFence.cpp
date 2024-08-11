/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "renderFence.h"
#include "renderCommands.h"

IRenderFence::IRenderFence()
{
}

IRenderFence::~IRenderFence()
{
}

CRenderFenceHelper::CRenderFenceHelper( IRender* render )
	: m_interface( render )
	, m_fence( NULL )
{
	if ( m_interface )
	{
		m_fence = m_interface->CreateFence();
	}
}

CRenderFenceHelper::~CRenderFenceHelper()
{
	SAFE_RELEASE( m_fence );
}

void CRenderFenceHelper::Flush()
{
	if ( m_fence )
	{
		( new CRenderCommand_Fence( m_fence ) )->Commit();
		m_fence->FlushFence();
	}
}
