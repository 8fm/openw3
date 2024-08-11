/**
* Copyright c 2012 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "storySceneIncludes.h"

//////////////////////////////////////////////////////////////////////////

class IStorySceneChoiceLineAction : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IStorySceneChoiceLineAction, CObject );

public:
	virtual Bool CanUseAction()					{ return false; }
	virtual String GetActionText()				{ return String::EMPTY; }
	virtual EDialogActionIcon GetActionIcon()	{ return DialogAction_NONE; }
	virtual void PerformAction()				{}
	virtual CName GetPlayGoChunk()				{ return CName( TXT("content0") ); }
};

BEGIN_CLASS_RTTI( IStorySceneChoiceLineAction )
	PARENT_CLASS( CObject )
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////

class CStorySceneChoiceLineActionScripted : public IStorySceneChoiceLineAction
{
	DECLARE_ENGINE_CLASS( CStorySceneChoiceLineActionScripted, IStorySceneChoiceLineAction, 0 );

public:
	virtual Bool CanUseAction();
	virtual String GetActionText();
	virtual EDialogActionIcon GetActionIcon();
	virtual void PerformAction();
};

BEGIN_CLASS_RTTI( CStorySceneChoiceLineActionScripted )
	PARENT_CLASS( IStorySceneChoiceLineAction )
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////

class CStorySceneChoiceLineActionScriptedContentGuard : public CStorySceneChoiceLineActionScripted
{
	DECLARE_ENGINE_CLASS( CStorySceneChoiceLineActionScriptedContentGuard, CStorySceneChoiceLineActionScripted, 0 );

public:
	virtual Bool CanUseAction();
	virtual String GetActionText();
	virtual EDialogActionIcon GetActionIcon();
	virtual void PerformAction();
	virtual CName GetPlayGoChunk();

private:
	CName m_playGoChunk; //!< Required installed PlayGo chunk to be enabled
};

BEGIN_CLASS_RTTI( CStorySceneChoiceLineActionScriptedContentGuard )
	PARENT_CLASS( CStorySceneChoiceLineActionScripted )
	PROPERTY_CUSTOM_EDIT( m_playGoChunk, TXT("PlayGo chunk"), TXT("PlayGoChunkSelector") )
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////

class CStorySceneChoiceLineActionStallForContent : public CStorySceneChoiceLineActionScripted
{
	DECLARE_ENGINE_CLASS( CStorySceneChoiceLineActionStallForContent, CStorySceneChoiceLineActionScripted, 0 );

public:
	virtual Bool CanUseAction();
	virtual String GetActionText();
	virtual EDialogActionIcon GetActionIcon();
	virtual void PerformAction();
};

BEGIN_CLASS_RTTI( CStorySceneChoiceLineActionStallForContent )
	PARENT_CLASS( CStorySceneChoiceLineActionScripted )
END_CLASS_RTTI()