
#include "build.h"
#include "voicesetPlayer.h"
#include "storySceneSystem.h"
#include "actor.h"

CVoicesetPlayer::CVoicesetPlayer( EArbitratorPriorities priority, const String& voiceset )
	: m_priority( priority )
	, m_voiceset( voiceset )
{

}

CVoicesetPlayer::~CVoicesetPlayer()
{
	m_scene.Release();
}

Bool CVoicesetPlayer::IsSceneLoaded() const
{
	BaseSoftHandle::EAsyncLoadingResult ret = m_scene.GetAsync();

	// Check also ALR_Failed
	return ret == BaseSoftHandle::ALR_Loaded || ret == BaseSoftHandle::ALR_Failed;
}

void CVoicesetPlayer::PlayVoiceset( CActor* actor )
{
	if ( m_scene.GetAsync() == BaseSoftHandle::ALR_Loaded )
	{
		CStoryScene* scene = m_scene.Get();
		if ( !scene )
		{
			ASSERT( scene );
			return;
		}

		const CStorySceneInput* input = NULL;

		// Try to find new-way input
RED_MESSAGE( "We can no longer create a string here from a cname" )
		String newWayInput = String::Printf( TXT( "%s_%s" ), m_voiceset.AsChar(), actor->GetVoiceTag().AsString().AsChar() );
		input = scene->FindInput( newWayInput );
		if ( !input )
		{
			// TEMP log, until it gets annoying
			LOG_GAME( TXT( "Cannot find newWay input '%ls' in scene '%ls'" ),
				newWayInput.AsChar(), scene->GetFriendlyName().AsChar () );
			LOG_GAME( TXT( "Using OLD scene input '%ls' in scene '%ls'" ), m_voiceset.AsChar(),
				scene->GetFriendlyName().AsChar() );

			// Fallback to old way
			input = scene->FindInput( m_voiceset );
			if ( input == nullptr )
			{
				LOG_GAME( TXT( "Cannot find OLD scene input '%ls' in scene '%ls'" ), m_voiceset.AsChar(), scene->GetFriendlyName().AsChar() );
			}
		}
		else
		{
			// TEMP log, until it gets annoying
			LOG_GAME( TXT( "Using NEW scene input '%ls' in scene '%ls'" ), newWayInput.AsChar(),
				scene->GetFriendlyName().AsChar ());
		}

		if ( ! input )
		{
			return;
		}

		GCommonGame->GetSystem< CStorySceneSystem >()->PlayAsVoiceset( input, actor );
	}
}

Bool CVoicesetPlayer::StartLoading( TSoftHandle< CStoryScene >& sceneHandle )
{
	m_scene = sceneHandle;

	BaseSoftHandle::EAsyncLoadingResult ret = m_scene.GetAsync();

	return ret == BaseSoftHandle::ALR_Loaded || ret == BaseSoftHandle::ALR_InProgress;
}
