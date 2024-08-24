////////////////////////////////////////////////////////////////////////
// OperationResults.h
//
// OperationResults header file
//
// Copyright 2009 Audiokinetic Inc.
//
// Author:  brodrigue
// Version: 1.0
//
////////////////////////////////////////////////////////////////////////

#pragma once

#include <AK/Wwise/SourceControl/ISourceControl.h>
#include <vector>


// Stores results of the move
class FileOperationResult : public AK::Wwise::ISourceControl::IFileOperationResult
{
public:
	FileOperationResult();
	~FileOperationResult();
	
	virtual AK::Wwise::ISourceControl::OperationResult GetOperationResult();
	virtual void Destroy();

	void SetOperationResult( AK::Wwise::ISourceControl::OperationResult in_eResult );

	// Get a moved file
	virtual void GetMovedFile( unsigned int in_uiIndex, LPWSTR out_szFrom, LPWSTR out_szTo, unsigned int in_uiArraySize );
	virtual void GetFile( unsigned int in_uiIndex, LPWSTR out_szPath, unsigned int in_uiArraySize );
	
	// Get the number of moved file
	virtual unsigned int GetFileCount();

	// Add a file successfully moved
	void AddMovedFile( const CString& in_csFrom, const CString& in_csTo );

	// Add a file successfully moved
	void AddFile( const CString& in_csPath );

private:

	struct FileInfo
	{
		CString csFrom;
		CString csTo;
	};

	std::vector<FileInfo> m_files;

	AK::Wwise::ISourceControl::OperationResult m_eResult;
};
