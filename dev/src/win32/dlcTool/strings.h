/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

namespace DLCTool
{
	// String ID
	enum StringID
	{
		String_GameTitle,
		String_ToolTitle,
		String_ErrorTilte,
		String_Error_IOReadError,
		String_Error_IOWriteError,
		String_Error_IODirError,
		String_Error_IOReadOnly,
		String_Error_NoDLCData,
		String_Error_CorruptedDLCData,
		String_Error_NoGameInstalled,
		String_Error_NotEnoughDiskSpace,
		String_Error_CannotCreateTemporaryFile,
		String_Error_CannotCreateActivateFile,
		String_Error_CannotOpenLauncherProcess,
		String_Error_NoPrivledges,
		String_Error_InvalidGameVersion,
		String_Error_FailedOpeningPackFile,
		String_Error_FailedApplyingDLCChange,
		String_Error_FailedCreatingTempFile,
		String_Error_InternalError,
		String_Error_UnknownError,
		String_Info_DLCInstalled,
		String_Info_DLCInstallFailure,
		String_Info_RegistryUpdateFailure,
		String_State_Initializing,
		String_State_Checking,
		String_State_Decompressing,
		String_State_Applying,
		String_State_Closing,
		String_Updating_Registry,
		String_Parsing_Ini,
		String_Adding_Files,
		String_Adding_Ini,
		String_Reading_Layout,
		String_Installing_Files,
		String_Deleting_Files,
	};

	// Get string by ID
	const WCHAR* GetStringById( StringID id );

	// Change language
	void SetLanguage( const Char* name );
};