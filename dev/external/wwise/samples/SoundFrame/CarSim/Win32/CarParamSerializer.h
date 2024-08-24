//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#pragma once

class CarSimEngine;

class CarParamSerializer
{
public:
	CarParamSerializer();
	~CarParamSerializer();

	void Init( CarSimEngine* in_pEngine );

	void ClearFileName();

	bool Save( const CString& in_rFileName );
	bool Load( const CString& in_rFileName );

	const CString& GetCurrentFilePath() const;

	// Return the file name with no extension
	CString GetCurrentFileNameOnly() const;

	const CString& GetFileExtension() const;

private:

	void EnsureFileExtension( CString& io_rFileName );

	CString m_csLastFile;

	CarSimEngine* m_pCarSimEngine;
};