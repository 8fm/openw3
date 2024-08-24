//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include <AK/IBytes.h>

#include <math.h>
#include <float.h>

#define ReleaseClear(ptr) (((ptr) ? (ptr)->Release() : NULL), (ptr) = NULL)
#define SafeAddRef(ptr) ((ptr) ? (ptr)->AddRef() : NULL)

inline bool DoubleEqual( const double& in_rdblFirst, const double& in_rdblSecond, const double& in_rdblEpsilon = DBL_EPSILON )
{
	return fabs( in_rdblFirst - in_rdblSecond ) < in_rdblEpsilon;
}

namespace CarSimHelper
{
	CString GetFileNameOnly( const CString& in_rFileFullPath );
	void EnsureFileExtension( CString& io_rFile, const CString& in_rFileExtension );
}

// -----------------------------------------------------------------------------

class ReadBytesInFile
	: public AK::IReadBytes
{
public:
	ReadBytesInFile();
	virtual ~ReadBytesInFile();

	bool Open( const CString& in_rFileName );
	void Close();

	// AK::IReadBytes
	virtual bool ReadBytes( void * in_pData, long in_cBytes, long & out_cRead );

private:
	FILE * m_pFile;
};

// -----------------------------------------------------------------------------

class WriteBytesInFile
	: public AK::IWriteBytes
{
public:
	WriteBytesInFile();
	virtual ~WriteBytesInFile();

	bool Open( const CString& in_rFileName );
	void Close();

	// AK::IWriteBytes
	virtual bool WriteBytes( const void * in_pData, long in_cBytes, long & out_cWritten );

private:
	FILE * m_pFile;
};
