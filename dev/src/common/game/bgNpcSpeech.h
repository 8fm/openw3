
#pragma once

#include "../../common/game/storyScene.h"

class ISceneActorInterface;
class CActorSpeech;

class CBgNpcVoicesetPlayer
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Gameplay );

	enum EVoicesetPlayerState
	{
		VPS_Finished,
		VPS_LoadingScene,
		VPS_PlayScene,
		VPS_PlayingScene,
	};

	TSoftHandle< CStoryScene >	m_scene;
	String						m_voiceset;
	CName						m_voiceTag;

	ISceneActorInterface*		m_actor;
	EVoicesetPlayerState		m_state;
	CActorSpeech*				m_speech;

public:
	CBgNpcVoicesetPlayer( ISceneActorInterface* actor, const TSoftHandle< CStoryScene >& scene, const String& voiceset, const CName& voiceTag );
	~CBgNpcVoicesetPlayer();

	Bool Update( Float dt );

	friend IFile& operator<<( IFile &file, CBgNpcVoicesetPlayer &voicesetPlayer );

private:
	void SetState( EVoicesetPlayerState state );

	Bool PlayVoiceset( const CStorySceneInput* input );

	CName GetVoiceTag() const;

	void CancelSpeech();
};
