/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "renderLoadingScreenFence.h"

CRenderLoadingScreenFence::CRenderLoadingScreenFence( const SLoadingScreenFenceInitParams& initParams )
	: m_initParams( initParams )
	, m_state( eLoadingScreenFenceState_Shown )
{
}
