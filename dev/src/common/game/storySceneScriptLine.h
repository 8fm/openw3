/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "storySceneScriptingBlock.h"
#include "storySceneElement.h"

class CStorySceneScriptLine : public CStorySceneElement
{
	DECLARE_ENGINE_CLASS( CStorySceneScriptLine, CStorySceneElement, 0 );

private:
	String m_script;
	CStorySceneScript*	m_sceneScript;

public:
	CStorySceneScriptLine();

	//! Start playing of this dialog element
	virtual IStorySceneElementInstanceData* OnStart( CStoryScenePlayer* player ) const;

	//! Get list of element that can be scheduled via this element
	virtual void OnGetSchedulableElements( TDynArray< const CStorySceneElement* >& elements ) const;

	virtual Bool IsPlayable() const { return true; }

	String GetScriptString() const;

	RED_INLINE void SetStorySceneScript( CStorySceneScript* sceneScript ) { m_sceneScript = sceneScript; }
	RED_INLINE CStorySceneScript* GetStorySceneScript() const { return m_sceneScript; }

	//IDynamicPropertiesSupplier* QueryDynamicPropertiesSupplier() { return m_sceneScript; }
};

BEGIN_CLASS_RTTI( CStorySceneScriptLine );
	PARENT_CLASS( CStorySceneElement );
	PROPERTY_EDIT( m_script, TXT( "Script to execute" ) );
	//PROPERTY_EDIT_IN( (*m_sceneScript), m_functionName, TXT( "Script function" ) );
	PROPERTY_INLINED( m_sceneScript, TXT( "Script function" ) );
END_CLASS_RTTI();

class CStoryScriptLineInstanceData : public IStorySceneElementInstanceData, public CScriptThread::IListener
{
private:
	const CStorySceneScript*	m_script;				//!< Current script block we are paused on
	CScriptThread*				m_scriptThread;		//!< Current thread being processed
	CPropertyDataBuffer			m_scriptReturnValue;		//!< Return value from script thread
	const CStorySceneScriptLine*	m_scriptLine;

	Bool	m_scriptFinished;

public:
	CStoryScriptLineInstanceData( const CStorySceneScriptLine* scriptLine, CStoryScenePlayer* player );
	~CStoryScriptLineInstanceData();

	//! Perform only actions related to playing an element. Preparation should be done earlier
	virtual void Play();

	virtual Bool IsBlocking() const;

	virtual String GetName() const { return String( TXT("Script line") ); }

public:
	virtual void OnScriptThreadKilled( CScriptThread * thread, Bool finished );
	virtual String GetDebugName() const { return TXT("StoryScriptLine"); }

protected:
	virtual Bool OnTick( Float timeDelta ) override;
};
