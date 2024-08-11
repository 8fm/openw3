/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "storySceneScriptLine.h"
#include "storySceneElement.h"
#include "storyScenePlayer.h"
#include "sceneLog.h"

IMPLEMENT_ENGINE_CLASS( CStorySceneScriptLine )

CStorySceneScriptLine::CStorySceneScriptLine()
	: CStorySceneElement()
{

}

IStorySceneElementInstanceData* CStorySceneScriptLine::OnStart( CStoryScenePlayer* player ) const
{
	return new CStoryScriptLineInstanceData( this, player );
}

void CStorySceneScriptLine::OnGetSchedulableElements( TDynArray< const CStorySceneElement* >& elements ) const
{
	elements.PushBack( this );
}

String CStorySceneScriptLine::GetScriptString() const
{
	if ( m_sceneScript == NULL || m_sceneScript->GetFunction() == NULL )
	{
		return TXT( "No script" );
	}

	String scriptString = TXT( "" );
	scriptString += m_sceneScript->GetFunctionName().AsString();
	scriptString += TXT( "( " );

	const TDynArray< StorySceneScriptParam >& functionParameters = m_sceneScript->GetParameters();
	for ( Uint32 i = 0; i < functionParameters.Size(); ++i )
	{
		if ( i > 0 )
		{
			scriptString += TXT( ", " );
		}
		scriptString += functionParameters[ i ].m_value.ToString();
	}

	scriptString += TXT( " )" );
	return scriptString;
}

CStoryScriptLineInstanceData::CStoryScriptLineInstanceData( const CStorySceneScriptLine* scriptLine, CStoryScenePlayer* player )
	: IStorySceneElementInstanceData( scriptLine, player )
	, m_scriptFinished( false )
	, m_script( NULL )
	, m_scriptThread( NULL )
	, m_scriptLine( scriptLine )
{	
}

CStoryScriptLineInstanceData::~CStoryScriptLineInstanceData()
{
	if ( m_scriptThread )
	{
		m_scriptThread->ForceKill();
		m_scriptThread = NULL;
	}
}

Bool CStoryScriptLineInstanceData::OnTick( Float timeDelta )
{
	return m_scriptFinished == false;
}

void CStoryScriptLineInstanceData::OnScriptThreadKilled( CScriptThread * thread, Bool finished )
{
	ASSERT( m_script != NULL );
	ASSERT( m_scriptThread == thread );

	const CStorySceneScript* block = m_script;
	m_script = NULL;
	m_scriptThread = NULL;	

	m_scriptFinished = true;	

	if ( m_scriptThread != thread )
	{
		LOG_GAME( TXT("OnScriptThreadKilled ERROR - CStoryScriptLineInstanceData. Thread has %s listener"), thread->GetListenerName().AsChar()  );
	}
}

Bool CStoryScriptLineInstanceData::IsBlocking() const
{
	return m_script != NULL && m_scriptThread != NULL;
}

void CStoryScriptLineInstanceData::Play()
{
	ASSERT( m_script == NULL );

	const CStorySceneScript* script = m_scriptLine->GetStorySceneScript();

	// Get function, if no function found report error
	CFunction* function = script->GetFunction();
	if ( function == NULL )
	{
		SCENE_WARN( TXT("Missing scene script function '%ls' in '%ls'"), script->GetFunctionName().AsString().AsChar(), m_scriptLine->GetElementID().AsChar() );

		m_scriptFinished = true;
		return;
	}

	if ( function->GetReturnValue() )
	{
		IRTTIType* returnType = function->GetReturnValue()->GetType();
		m_scriptReturnValue.Reset( returnType );
	}

	Int32 immediateResult = -1;
	CScriptThread* scriptThread = script->Execute( m_player, m_scriptReturnValue.Data(), immediateResult );

	// Script thread was created
	if ( scriptThread != NULL )
	{
		SCENE_LOG( TXT("Thread started for script block '%ls' in '%ls'"), script->GetFunctionName().AsString().AsChar(), m_scriptLine->GetElementID().AsChar() );

		m_scriptThread = scriptThread;
		m_script = script;

		scriptThread->SetListener( this );
	}
	else
	{
		m_scriptFinished = true;
	}
}
