/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "dynarray.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////

enum EFeedbackYesNoCancelResult
{
	FeedbackYes,
	FeedbackNo,
	FeedbackCancel,
};


/// User feedback system
class IFeedbackSystem
{
public:
	virtual ~IFeedbackSystem() {};

	//! Begin a time intensive task
	virtual void BeginTask( const Char* name, Bool canCancel )=0;

	//! End a time intensive task
	virtual void EndTask()=0;

	//! Update task progress
	virtual void UpdateTaskProgress( Uint32 current, Uint32 total )=0;

	//! Update task caption
	virtual void UpdateTaskInfo( const Char* info, ... )=0;

	//! Did user canceled the task ?
	virtual Bool IsTaskCanceled()=0;

	//! Is this NullFeedback?
	virtual Bool IsNullFeedback(){ return false; };

	//! Ask user a yes/no question
	virtual Bool AskYesNo( const Char* info, ... )=0;

	//! Ask user a yes/no/cancel question. 
	virtual EFeedbackYesNoCancelResult AskYesNoCancel( const Char* info, ... )=0;

	//! Ask the user to confirm a bunch of questions
	virtual Bool ShowMultiBoolDialog( const String& title, const TDynArray<String>& questions, TDynArray<Bool>& answers )=0;

	//! Show message to user
	virtual void ShowMsg( const Char* title, const Char* msg, ... )=0;

	//! Show warning to user
	virtual void ShowWarn( const Char* msg, ... )=0;

	//! Show error message to user
	virtual void ShowError( const Char* msg, ... )=0;

	//! Show List of items for user to choose from
	virtual void ShowList( const Char* /*caption*/, const TDynArray< String >& /*itemList*/, TDynArray< Uint32 >& /*selectedIndexes*/ ) {}

	virtual void UpdateTaskName( const Char* /*name*/ ) {}

	//! Shows the object inspector in the editor (or does nothing for non-editor builds)
	virtual void InspectObject( class ISerializable* /* object */, const String& /* tag */ = String::EMPTY ) {}

	//! Shows a formatted dialog box, see the editor's utils.cpp for details
	virtual Int32 FormattedDialogBox( const String& /*caption*/, String /*code*/, ... ) { return -1; }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////

/// Splash screen
class ISplashScreen
{
public:
	virtual ~ISplashScreen() {};

	//! Update splash text
	virtual void UpdateProgress( const Char* info, ... )=0;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////

/// On screen log
class IOnScreenLog
{
public:
	virtual ~IOnScreenLog() {};

	//! Clear performance warnings, called after a loading screen, etc.
	virtual void ClearPerfWarnings()=0;

	//! Log performance warning on screen
	virtual void PerfWarning( Float timeTook, const String& group, const Char* info, ... )=0;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////

// Null feedback, the default implementation
class NullFeedback : public IFeedbackSystem
{
public:
	virtual void BeginTask( const Char*, Bool ) {};
	virtual void EndTask() {};
	virtual void UpdateTaskProgress( Uint32, Uint32 ) {};
	virtual void UpdateTaskInfo( const Char*, ... ) {};
	virtual Bool IsTaskCanceled() { return false; }
	virtual Bool AskYesNo( const Char*, ... ) { return false; };
	virtual EFeedbackYesNoCancelResult AskYesNoCancel( const Char*, ... ){ return FeedbackCancel; };
	virtual Bool ShowMultiBoolDialog( const String&, const TDynArray<String>&, TDynArray<Bool>& ) { return false; }
	virtual Bool IsNullFeedback(){ return true; };
	virtual void ShowMsg( const Char*, const Char*, ... ) {};
	virtual void ShowWarn( const Char*, ... ) {};
	virtual void ShowError( const Char*, ... ) {};
};

// Null splash screen
class NullSplash : public ISplashScreen
{
public:
	virtual void UpdateProgress( const Char*, ... ) {};
};

// Null implementation of on screen log
class NullOnScreenLog : public IOnScreenLog 
{
public:
	//! Clear performance warnings, called after a loading screen or sth
	virtual void ClearPerfWarnings() {};

	//! Log performance warning on screen
	virtual void PerfWarning( Float, const String&, const Char*, ... ) {};
};

////////////////////////////////////////////////////////////////////////////////////////////////////////

/// The feedback system
extern IFeedbackSystem* GFeedback;

/// The splash screen
extern ISplashScreen* GSplash;

/// On screen log
extern IOnScreenLog* GScreenLog;

////////////////////////////////////////////////////////////////////////////////////////////////////////

/// The null feedback system
static NullFeedback GNullFeedback;

// The null splash screen system
static NullSplash GNullSplash;

// The null on screen log
static NullOnScreenLog GNullOnScreenLog;
