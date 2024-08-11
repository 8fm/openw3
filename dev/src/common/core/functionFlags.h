/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

/// This file is used also by ScriptStudio

/// Function flags
enum EFunctionFlags
{
	FF_NativeFunction		= FLAG( 0 ),		//!< Function is native ( implemented in C++ )
	FF_StaticFunction		= FLAG( 1 ),		//!< Function is static
	FF_OperatorFunction		= FLAG( 2 ),		//!< Function is data operator
	FF_ExportedFunction		= FLAG( 3 ),		//!< Function is native function that was exported to script
	FF_FinalFunction		= FLAG( 4 ),		//!< Function is final and cannot be overridden in child classes
	FF_EventFunction		= FLAG( 5 ),		//!< Function is special event function
	FF_LatentFunction		= FLAG( 6 ),		//!< Function takes time to execute
	FF_EntryFunction		= FLAG( 7 ),		//!< Function is a state entry function
	FF_ExecFunction			= FLAG( 8 ),		//!< Function can be called from console
	FF_UndefinedBody		= FLAG( 9 ),		//!< Function has no body (just a declaration)
	FF_TimerFunction		= FLAG( 10 ),		//!< Function is a timer
	FF_SceneFunction		= FLAG( 11 ),		//!< Function can be used in Scenes
	FF_QuestFunction		= FLAG( 12 ),		//!< Function can be used in Quests
	FF_CleanupFunction		= FLAG( 13 ),		//!< Function is a cleanup
	FF_PrivateFunction		= FLAG( 14 ),		//!< Function is private
	FF_ProtectedFunction	= FLAG( 15 ),		//!< Function is protected
	FF_PublicFunction		= FLAG( 16 ),		//!< Function is public
	FF_RewardFunction		= FLAG( 17 ),		//!< Function can be attached to reward

	FF_AccessModifiers		= FF_PrivateFunction | FF_ProtectedFunction | FF_PublicFunction, 
};
