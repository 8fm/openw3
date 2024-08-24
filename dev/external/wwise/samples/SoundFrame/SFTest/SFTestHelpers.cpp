//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "SFTestHelpers.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// -----------------------------------------------------------------------------

ReadBytesOnFILE::ReadBytesOnFILE( FILE * in_pFile )
	: m_pFile( in_pFile )
{
	_ASSERT( m_pFile );
}

bool ReadBytesOnFILE::ReadBytes( void * in_pData, long in_cBytes, long & out_cRead )
{
	out_cRead = (long) fread( in_pData, 1, in_cBytes, m_pFile );

	return in_cBytes == out_cRead;
}

// -----------------------------------------------------------------------------

WriteBytesOnFILE::WriteBytesOnFILE( FILE * in_pFile )
	: m_pFile( in_pFile )
{
	_ASSERT( m_pFile );
}

bool WriteBytesOnFILE::WriteBytes( const void * in_pData, long in_cBytes, long & out_cWritten )
{
	out_cWritten = (long) fwrite( in_pData, 1, in_cBytes, m_pFile );

	return in_cBytes == out_cWritten;
}
