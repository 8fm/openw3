
#include "build.h"
#include "bgNpcChatPlayer.h"

#include "../../common/game/actorSpeech.h"

Bool CChatPlayer::Create( const TDynArray< CBgNpc* >& npcs, const TDynArray< SExtractedSceneLineData >& lines )
{
	ASSERT( npcs.Size() == lines.Size() );

	m_lines = lines;
	m_currentLine = -1;

	// Cache npcs
	m_npcs.Resize( npcs.Size() );
	for ( Uint32 i=0; i<m_npcs.Size(); ++i )
	{
		m_npcs[ i ] = npcs[ i ];
	}

	// Start
	return GoToNextLine();
}

Bool CChatPlayer::Update( Float dt )
{
	ASSERT( m_currentLine >= 0 && m_currentLine < m_lines.SizeInt() );

	CBgNpc* npc = m_npcs[ m_currentLine ].Get();
	if ( !npc )
	{
		return false;
	}

	if ( !npc->IsPlayingChat() )
	{
		if ( !GoToNextLine() )
		{
			return false;
		}
	}

	return true;
}

Bool CChatPlayer::GoToNextLine()
{
	m_currentLine += 1;

	if ( m_currentLine >= m_lines.SizeInt() )
	{
		return false;
	}

	const SExtractedSceneLineData& line = m_lines[ m_currentLine ];
	CBgNpc* npc = m_npcs[ m_currentLine ].Get();

	if ( !npc )
	{
		return false;
	}

	return npc->PlayChat( line );
}

//////////////////////////////////////////////////////////////////////////

CBgNpcChatPlayer::CBgNpcChatPlayer( ISceneActorInterface* actor, const SExtractedSceneLineData& line )
{
	ActorSpeechData setup( line.m_stringIndex, line.m_eventName, false, line.m_modeFlags );
	m_speech = new CActorSpeech( actor, setup );
}

CBgNpcChatPlayer::~CBgNpcChatPlayer()
{
	if ( m_speech )
	{
		CancelSpeech();
	}
}

Bool CBgNpcChatPlayer::Update( Float dt )
{
	m_speech->Update( dt );

	if ( m_speech->IsFinished() )
	{
		CancelSpeech();

		return false;
	}

	return true;
}

IFile& operator<<( IFile &file, CBgNpcChatPlayer &chatPlayer )
{
	if ( chatPlayer.m_speech )
	{
		file << *chatPlayer.m_speech;
	}

	return file;
}

void CBgNpcChatPlayer::CancelSpeech()
{
	ASSERT( m_speech );

	m_speech->Cancel();

	delete m_speech;
	m_speech = NULL;
}
