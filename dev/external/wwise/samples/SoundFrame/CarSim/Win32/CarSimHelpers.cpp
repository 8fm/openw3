//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "CarSimHelpers.h"

namespace CarSimHelper
{
	CString GetFileNameOnly( const CString& in_rFileFullPath )
	{
		WCHAR szFileName[_MAX_FNAME] = {0};
		if( ::_wsplitpath_s( in_rFileFullPath, NULL, 0, NULL, 0, szFileName, _MAX_FNAME, NULL, 0 ) == 0 )
			return CString( szFileName );
		else
			return CString();
	}
	
	void EnsureFileExtension( CString& io_rFile, const CString& in_rFileExtension )
	{
		WCHAR szDrive[_MAX_DRIVE] = {0};
		WCHAR szDir[_MAX_DIR] = {0};
		WCHAR szFileName[_MAX_FNAME] = {0};

		::_wsplitpath_s( io_rFile, szDrive, _MAX_DRIVE, szDir, _MAX_DIR, szFileName, _MAX_FNAME, NULL, 0 );
		::_wmakepath_s( io_rFile.GetBuffer( _MAX_PATH ), _MAX_PATH, szDrive, szDir, szFileName, in_rFileExtension );
		io_rFile.ReleaseBuffer();
	}
}

// -----------------------------------------------------------------------------

ReadBytesInFile::ReadBytesInFile()
: m_pFile( NULL )
{}

ReadBytesInFile::~ReadBytesInFile()
{
	Close();
}

bool ReadBytesInFile::Open( const CString& in_rFileName )
{
	return ::_wfopen_s( &m_pFile, in_rFileName, L"rb" ) == 0;
}

void ReadBytesInFile::Close()
{
	if( m_pFile )
		::fclose( m_pFile );
}

bool ReadBytesInFile::ReadBytes( void * in_pData, long in_cBytes, long & out_cRead )
{
	if( !m_pFile )
		return false;

	out_cRead = (long) ::fread( in_pData, 1, in_cBytes, m_pFile );

	return in_cBytes == out_cRead;
}

// -----------------------------------------------------------------------------

WriteBytesInFile::WriteBytesInFile()
: m_pFile( NULL )
{
}
	
WriteBytesInFile::~WriteBytesInFile()
{
	Close();
}
	
bool WriteBytesInFile::Open( const CString& in_rFileName )
{
	return ::_wfopen_s( &m_pFile, in_rFileName, L"wb" ) == 0;
}
	
void WriteBytesInFile::Close()
{
	if( m_pFile )
		::fclose( m_pFile );
}

bool WriteBytesInFile::WriteBytes( const void * in_pData, long in_cBytes, long & out_cWritten )
{
	if( !m_pFile )
		return false;

	out_cWritten = (long) ::fwrite( in_pData, 1, in_cBytes, m_pFile );

	return in_cBytes == out_cWritten;
}
