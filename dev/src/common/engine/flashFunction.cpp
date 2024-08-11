/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "flashFunction.h"
#include "flashMovie.h"
#include "flashValue.h"

//////////////////////////////////////////////////////////////////////////
// CFlashFunction
//////////////////////////////////////////////////////////////////////////
CFlashFunction::CFlashFunction( CFlashMovie* flashMovie, CFlashFunctionHandler* flashFunctionHandler )
	: m_flashMovie( flashMovie )
	, m_flashFunctionHandler( flashFunctionHandler )
{
	ASSERT( m_flashMovie );

	if ( m_flashMovie )
	{
		m_flashMovie->AddRef();
	}

	if ( m_flashFunctionHandler )
	{
		m_flashFunctionHandler->AddRef();
	}
}

CFlashFunction::~CFlashFunction()
{
	if ( m_flashFunctionHandler )
	{
		m_flashFunctionHandler->Release();
		m_flashFunctionHandler = nullptr;
	}

	if ( m_flashMovie )
	{
		m_flashMovie->Release();
		m_flashMovie = nullptr;
	}
}

