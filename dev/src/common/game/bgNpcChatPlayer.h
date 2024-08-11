
#pragma once

//TODO TEMP
#include "bgNpc.h"
#include "../../common/game/storySceneExtractedLineData.h"

class CChatPlayer
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Gameplay );

	TDynArray< THandle< CBgNpc > >			m_npcs;
	TDynArray< SExtractedSceneLineData >	m_lines;

	Int32			m_currentLine;

public:
	Bool Create( const TDynArray< CBgNpc* >& npcs, const TDynArray< SExtractedSceneLineData >& lines );
	Bool Update( Float dt );

private:
	Bool GoToNextLine();
};

//////////////////////////////////////////////////////////////////////////

class CBgNpcChatPlayer
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Gameplay );

	CActorSpeech*	m_speech;

public:
	CBgNpcChatPlayer( ISceneActorInterface* actor, const SExtractedSceneLineData& line );
	~CBgNpcChatPlayer();

	Bool Update( Float dt );

	friend IFile& operator<<( IFile &file, CBgNpcChatPlayer &chatPlayer );

private:
	void CancelSpeech();
};
