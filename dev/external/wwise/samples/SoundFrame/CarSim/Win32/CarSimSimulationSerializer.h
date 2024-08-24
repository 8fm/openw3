//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "CarSimHelpers.h"

class CarSimEngine;

class CarSimSimulationSerializer
{
public:
	CarSimSimulationSerializer();
	~CarSimSimulationSerializer();

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

	bool LoadAssignment( ReadBytesInFile& in_rReader, AkUniqueID& in_rObjectID );
	bool SaveAssignment( WriteBytesInFile& in_rWriter, AkUniqueID& in_rObjectID );

	CString m_csLastFile;

	CarSimEngine* m_pCarSimEngine;
};