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
// IReadBytes / IWriteBytes implementation on a growing memory buffer.
//
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <AK/IBytes.h>

class SFReadBytesMem
	: public AK::IReadBytes
{
public:

	SFReadBytesMem();
	SFReadBytesMem( const void * in_pBytes, long in_cBytes );
	virtual ~SFReadBytesMem();

	// IReadBytes implementation

	virtual bool ReadBytes( void * in_pData, long in_cBytes, long & out_cRead );

	//Returned string must be freed.
	wchar_t *ReadAndCopyString();

	// Public methods

	void Attach( const void * in_pBytes, long in_cBytes );

private:
	long   m_cBytes;
	const BYTE * m_pBytes;

	long   m_cPos;
};

class SFWriteBytesMem
	: public AK::IWriteBytes
{
public:

	SFWriteBytesMem();
	virtual ~SFWriteBytesMem();

	// IWriteBytes implementation

	virtual bool WriteBytes( const void * in_pData, long in_cBytes, long & out_cWritten );

	// Public methods

	bool   Reserve( long in_cBytes );
	long   Count() const;
	BYTE * Bytes() const;
    BYTE * Detach();

    void   Clear();

private:
	long   m_cBytes;
	BYTE * m_pBytes;

	long   m_cPos;
};
