/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"

#ifdef USE_SCALEFORM

#include "flashPlayerScaleform.h"
#include "flashFunctionScaleform.h"
#include "flashMovieScaleform.h"

//////////////////////////////////////////////////////////////////////////
// CFlashFunctionScaleform
//////////////////////////////////////////////////////////////////////////
CFlashFunctionScaleform::CFlashFunctionScaleform( CFlashMovieScaleform* flashMovie, CFlashFunctionHandler* flashFunctionHandler )
	: CFlashFunction( flashMovie, flashFunctionHandler )
{
	ASSERT( flashMovie );	
	if ( ! flashMovie )
	{
		return;
	}

	ASSERT( flashFunctionHandler );
	if ( ! flashFunctionHandler )
	{
		return;
	}


	SF::Ptr< GFx::Movie > gfxMovie = flashMovie->m_gfxMovie;

	ASSERT( gfxMovie );
	if ( ! gfxMovie )
	{
		return;
	}

	gfxMovie->CreateFunction( &m_gfxFunc, flashFunctionHandler, this );
	ASSERT( m_gfxFunc.IsObject() );
}

CFlashFunctionScaleform::~CFlashFunctionScaleform()
{
}

#endif // USE_SCALEFORM