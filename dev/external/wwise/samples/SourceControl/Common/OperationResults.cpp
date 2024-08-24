////////////////////////////////////////////////////////////////////////
// OperationResults.cpp
//
// OperationResults implementation file
//
// Copyright 2009 Audiokinetic Inc.
//
// Author:  brodrigue
// Version: 1.0
//
////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "OperationResults.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// FileOperationResult
FileOperationResult::FileOperationResult()
: m_eResult( AK::Wwise::ISourceControl::OperationResult_Succeed )
{
}

FileOperationResult::~FileOperationResult()
{
}

AK::Wwise::ISourceControl::OperationResult FileOperationResult::GetOperationResult()
{ 
	return m_eResult; 
}

void FileOperationResult::SetOperationResult( AK::Wwise::ISourceControl::OperationResult in_eResult )
{
	m_eResult = in_eResult;
}

void FileOperationResult::Destroy()
{
	delete this;
}

void FileOperationResult::GetMovedFile( unsigned int in_uiIndex, LPWSTR out_szFrom, LPWSTR out_szTo, unsigned int in_uiArraySize )
{
	ASSERT(in_uiIndex < m_files.size());
	if( in_uiIndex < m_files.size() )
	{
		wcsncpy_s( out_szFrom, in_uiArraySize, m_files[in_uiIndex].csFrom, m_files[in_uiIndex].csFrom.GetLength() );
		wcsncpy_s( out_szTo, in_uiArraySize, m_files[in_uiIndex].csTo, m_files[in_uiIndex].csTo.GetLength() );
	}
}

void FileOperationResult::GetFile( unsigned int in_uiIndex, LPWSTR out_szPath, unsigned int in_uiArraySize )
{
	ASSERT(in_uiIndex < m_files.size());
	if( in_uiIndex < m_files.size() )
	{
		wcsncpy_s( out_szPath, in_uiArraySize, m_files[in_uiIndex].csFrom, m_files[in_uiIndex].csFrom.GetLength() );
	}
}

unsigned int FileOperationResult::GetFileCount()
{
	return (unsigned int)m_files.size();
}

void FileOperationResult::AddMovedFile( const CString& in_csFrom, const CString& in_csTo )
{
	FileInfo moved = { in_csFrom, in_csTo };
	m_files.push_back(moved);
}

void FileOperationResult::AddFile( const CString& in_csPath )
{
	FileInfo file = { in_csPath, L"" };
	m_files.push_back(file);
}