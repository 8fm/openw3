//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include <AK/IBytes.h>

// -----------------------------------------------------------------------------
// Implementation of the IReadBytes that work on a FILE*
class ReadBytesOnFILE
	: public AK::IReadBytes
{
public:
	ReadBytesOnFILE( FILE * in_pFile );

	virtual bool ReadBytes( void * in_pData, long in_cBytes, long & out_cRead );

private:
	FILE * m_pFile;
};

// -----------------------------------------------------------------------------
// Implementation of the IWriteBytes that work on a FILE*
class WriteBytesOnFILE
	: public AK::IWriteBytes
{
public:
	WriteBytesOnFILE( FILE * in_pFile );

	virtual bool WriteBytes( const void * in_pData, long in_cBytes, long & out_cWritten );

private:
	FILE * m_pFile;
};
