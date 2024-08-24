////////////////////////////////////////////////////////////////////////
// SourceControlHelpers.h
//
// SourceControlHelpers header file
//
// Copyright 2009 Audiokinetic Inc.
//
// Author:  brodrigue
// Version: 1.0
//
////////////////////////////////////////////////////////////////////////

#pragma once

#include <vector>

namespace AK{ namespace Wwise{ class ISourceControlUtilities; } }

namespace SourceControlHelpers
{
	bool CanProceedWithMove( 
		const CStringList& in_rFilenameList, 
		const CString& in_csDestinationDir,
		std::vector<CString>& out_rNewPaths,
		AK::Wwise::ISourceControlUtilities* in_pUtilities,
		UINT in_uiIdsErrorAlreadyExist,
		UINT in_uiIdsErrorNameConflict );

	CString GetLastErrorString();

	bool WaitWithPaintLoop(HANDLE hEvent);
};
