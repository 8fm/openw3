/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

// Are we running in silent mode ?
extern bool GIsSilent;

/// Error handling
namespace DLCTool
{
	// Enable silent mode
	void EnableSilentMode();

	// Log
	void Log( const WCHAR* txt, ... );

	// Log
	void Log( const CHAR* txt, ... );

	// Debug message
	void DebugMessage( const WCHAR* txt, ... );

	// Throw system error
	void ThrowError( StringID errorStringID, ... );

	// Show message
	void ShowMessage( StringID messageStringID );

	// Display error message
	void DisplayError();
};

// Logging/error macros
#define LOG( x, ... ) DLCTool::Log( x, ## __VA_ARGS__ )
#define ERR( x, ... ) DLCTool::ThrowError( x, ## __VA_ARGS__ )
#define TEXT2(x) TEXT(x)
#define ASSERT( x ) if ( (!(x)) ) { DLCTool::ThrowError( DLCTool::String_Error_InternalError, TEXT2(__FILE__), __LINE__ ); }