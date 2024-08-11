/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"
#include "flashValueObjectScaleform.h"
#include "flashMovieScaleform.h"
#include "guiGlobals.h"

#ifdef USE_SCALEFORM

CFlashObjectScaleform::CFlashObjectScaleform( CFlashMovieScaleform* flashMovie, const String& flashClassName )
	: CFlashObject( flashMovie )
{
	ASSERT( flashMovie );
	if ( ! flashMovie )
	{
		return;
	}

	flashMovie->m_gfxMovie->CreateObject( &m_flashValue.m_gfxValue, FLASH_TXT_TO_UTF8( flashClassName.AsChar() ) );
	ASSERT( m_flashValue.m_gfxValue.IsObject() );
	if ( ! m_flashValue.m_gfxValue.IsObject() )
	{
		flashMovie->m_gfxMovie->CreateObject( &m_flashValue.m_gfxValue, nullptr );
		GUI_ERROR( TXT("Failed to create Flash Object with class name '%ls'"), flashClassName.AsChar() );
	}
}

CFlashObjectScaleform::~CFlashObjectScaleform()
{
}

CFlashArrayScaleform::CFlashArrayScaleform( CFlashMovieScaleform* flashMovie )
	: CFlashArray( flashMovie )
{
	ASSERT( flashMovie );
	if ( ! flashMovie )
	{
		return;
	}

	flashMovie->m_gfxMovie->CreateArray( &m_flashValue.m_gfxValue );
	ASSERT( m_flashValue.m_gfxValue.IsArray() );
}

CFlashArrayScaleform::~CFlashArrayScaleform()
{
}

CFlashStringScaleform::CFlashStringScaleform( CFlashMovieScaleform* flashMovie, const String& value )
	: CFlashString( flashMovie )
{
	ASSERT( flashMovie );
	if ( ! flashMovie )
	{
		return;
	}

	flashMovie->m_gfxMovie->CreateString( &m_flashValue.m_gfxValue, FLASH_TXT_TO_UTF8( value.AsChar() ) );
	ASSERT( m_flashValue.m_gfxValue.IsString() );
}

CFlashStringScaleform::~CFlashStringScaleform()
{
}

#endif // USE_SCALEFORM