/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version: v2013.2.9  Build: 4872
  Copyright (c) 2006-2014 Audiokinetic Inc.
 ***********************************************************************/

////////////////////////////////////////////////////////////////////////////////
//
// IReadBytes / IWriteBytes implementation on a memory buffer.
//
////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <malloc.h>

#include "SFBytesMem.h"

//------------------------------------------------------------------------------
// SFReadBytesMem 

SFReadBytesMem::SFReadBytesMem()
	: m_cBytes( 0 )
	, m_pBytes( NULL )
	, m_cPos( 0 )
{
}

SFReadBytesMem::SFReadBytesMem( const void * in_pBytes, long in_cBytes )
{
	Attach( in_pBytes, in_cBytes );
}

SFReadBytesMem::~SFReadBytesMem()
{
}

bool SFReadBytesMem::ReadBytes( void * in_pData, long in_cBytes, long & out_cRead )
{
	if( m_pBytes == NULL )
		return false;
	
	long cRemaining = m_cBytes - m_cPos;
	long cToRead = min( in_cBytes, cRemaining );

	memcpy( in_pData, m_pBytes + m_cPos, cToRead );

	m_cPos += in_cBytes;
	out_cRead = cToRead;

	return out_cRead == in_cBytes;
}

void SFReadBytesMem::Attach( const void * in_pBytes, long in_cBytes )
{
	m_cBytes = in_cBytes;
	m_pBytes = reinterpret_cast<const BYTE*>( in_pBytes );
	m_cPos = 0;
}

wchar_t * SFReadBytesMem::ReadAndCopyString()
{
	long cChars;
	if ( !Read<long>( cChars ) ) 
		return NULL;

	wchar_t* szStr = (wchar_t*)malloc((cChars+1)*sizeof(wchar_t));

	long cRead;
	ReadBytes(szStr, cChars * sizeof(wchar_t), cRead);
	szStr[cChars] = 0;

	if (cRead/sizeof(wchar_t) != cChars)
	{
		//Error
		free(szStr);
		return NULL;
	}

	return szStr;
}

//------------------------------------------------------------------------------
// SFWriteBytesMem

SFWriteBytesMem::SFWriteBytesMem()
	: m_cBytes( 0 )
	, m_pBytes( NULL )
	, m_cPos( 0 )
{
}

SFWriteBytesMem::~SFWriteBytesMem()
{
	if ( m_pBytes )
		free( m_pBytes );
}

bool SFWriteBytesMem::WriteBytes( const void * in_pData, long in_cBytes, long & out_cWritten )
{
	Reserve( m_cPos + in_cBytes );

	memcpy( m_pBytes + m_cPos, in_pData, in_cBytes );

	m_cPos += in_cBytes;
	out_cWritten = in_cBytes;

	return true;
}

bool SFWriteBytesMem::Reserve( long in_cBytes )
{
	static const int kGrowBy = 1024;

	if ( m_cBytes < in_cBytes )
	{
		int cGrowBlocks = ( in_cBytes + kGrowBy-1 ) / kGrowBy;
		m_cBytes = cGrowBlocks * kGrowBy;
		m_pBytes = (BYTE *) realloc( m_pBytes, m_cBytes );
	}

	return true;
}

long SFWriteBytesMem::Count() const
{
	return m_cPos;
}

BYTE * SFWriteBytesMem::Bytes() const
{
	return m_pBytes;
}

BYTE * SFWriteBytesMem::Detach()
{
    BYTE* pByte = m_pBytes;

    m_pBytes = NULL;
    m_cBytes = 0;
    m_cPos = 0;

    return pByte;
}

void SFWriteBytesMem::Clear()
{
    m_cPos = 0;
}