
#include "build.h"
#include "bgNpcSpeech.h"
#include "../../common/game/storyScene.h"
#include "../../common/game/storySceneInput.h"
#include "../../common/game/actorSpeech.h"
#include "../../common/game/storySceneSystem.h"

CBgNpcVoicesetPlayer::CBgNpcVoicesetPlayer( ISceneActorInterface* actor, const TSoftHandle< CStoryScene >& scene, const String& voiceset, const CName& voiceTag )
	: m_scene( scene )
	, m_state( VPS_LoadingScene )
	, m_voiceTag( voiceTag )
	, m_speech( NULL )
	, m_voiceset( voiceset )
	, m_actor( actor )
{
	Update( 0.f );
}

CBgNpcVoicesetPlayer::~CBgNpcVoicesetPlayer()
{
	if ( m_speech )
	{
		CancelSpeech();
	}

	m_scene.Release();
}

IFile& operator<<( IFile &file, CBgNpcVoicesetPlayer &voicesetPlayer )
{
	if ( voicesetPlayer.m_speech )
	{
		file << *voicesetPlayer.m_speech;
	}

	return file;
}

Bool CBgNpcVoicesetPlayer::Update( Float dt )
{
	while( 1 )
	{
		switch( m_state )
		{
		case VPS_LoadingScene:
			{
				BaseSoftHandle::EAsyncLoadingResult ret = m_scene.GetAsync();
				if ( ret == BaseSoftHandle::ALR_InProgress )
				{
					return true;
				}
				else if ( ret == BaseSoftHandle::ALR_Loaded )
				{
					SetState( VPS_PlayScene );
				}
				else if ( ret == BaseSoftHandle::ALR_Failed )
				{
					SetState( VPS_Finished );
				}
				else
				{
					ASSERT( 0 );
				}
			}
			break;


		case VPS_PlayScene:
			{
				CStoryScene* scenePtr = m_scene.Get();
				if ( scenePtr )
				{
					// Try to find new-way input
					const CStorySceneInput* input = scenePtr->FindInput( m_voiceset );
					if ( input && PlayVoiceset( input ) )
					{
						ASSERT( m_speech );

						SetState( VPS_PlayingScene );
					}
					else
					{
						if ( !input )
						{
							LOG_GAME( TXT( "Cannot find newWay input '%ls' in scene '%ls'" ), m_voiceset.AsChar(), scenePtr->GetFriendlyName().AsChar () );
						}

						SetState( VPS_Finished );
					}
				}
			}
			break;


		case VPS_PlayingScene:
			{
				ASSERT( m_speech );

				m_speech->Update( dt );

				if ( m_speech->IsFinished() )
				{
					CancelSpeech();

					SetState( VPS_Finished );
				}
				else
				{
					return true;
				}
			}
			break;


		case VPS_Finished:
			return false;			
		}
	}

	ASSERT( 0 );
	return false;
}

void CBgNpcVoicesetPlayer::SetState( EVoicesetPlayerState state )
{
	m_state = state;
}

void CBgNpcVoicesetPlayer::CancelSpeech()
{
	ASSERT( m_speech );

	m_speech->Cancel();

	delete m_speech;
	m_speech = NULL;
}

Bool CBgNpcVoicesetPlayer::PlayVoiceset( const CStorySceneInput* input )
{
	CActorSpeechQueue speechQueue;
	if ( !CStorySceneSystem::ExtractVoicesetDataFromSceneInput( input, GetVoiceTag(), &speechQueue ) )
	{
		return false;
	}

	if ( m_speech )
	{
		CancelSpeech();
	}

	if ( speechQueue.HasNextLine() )
	{
		ActorSpeechData data( 0, StringAnsi::EMPTY, false, 0 );
		speechQueue.NextLineData( data );
		m_speech = new CActorSpeech( m_actor, data );
	}
		
	return true;
}

CName CBgNpcVoicesetPlayer::GetVoiceTag() const
{
	return m_voiceTag;
}
