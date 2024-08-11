/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"

#include "flashValueObject.h"
#include "flashMovie.h"

//////////////////////////////////////////////////////////////////////////
// CFlashObject
//////////////////////////////////////////////////////////////////////////
CFlashObject::CFlashObject( CFlashMovie* flashMovie )
{
	ASSERT( flashMovie );
	m_flashMovie = flashMovie;
	if ( m_flashMovie )
	{
		m_flashMovie->AddRef();
	}
}

CFlashObject::~CFlashObject()
{
	ASSERT( m_flashMovie );
	if ( m_flashMovie )
	{
		m_flashMovie->Release();
		m_flashMovie = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////
// CFlashArray
//////////////////////////////////////////////////////////////////////////
CFlashArray::CFlashArray( CFlashMovie* flashMovie )
{
	ASSERT( flashMovie );
	m_flashMovie = flashMovie;
	if ( m_flashMovie )
	{
		m_flashMovie->AddRef();
	}
}

CFlashArray::~CFlashArray()
{
	ASSERT( m_flashMovie );
	if ( m_flashMovie )
	{
		m_flashMovie->Release();
		m_flashMovie = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////
// CFlashString
//////////////////////////////////////////////////////////////////////////
CFlashString::CFlashString( CFlashMovie* flashMovie )
{
	ASSERT( flashMovie );
	m_flashMovie = flashMovie;
	if ( m_flashMovie )
	{
		m_flashMovie->AddRef();
	}
}

CFlashString::~CFlashString()
{
	ASSERT( m_flashMovie );
	if ( m_flashMovie )
	{
		m_flashMovie->Release();
		m_flashMovie = nullptr;
	}
}
