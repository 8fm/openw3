/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeNodeConditionSpeech.h"

BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeConditionIsInChatSceneDefinition )
BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeConditionIsSpeakingDefinition )
BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeConditionHasVoicesetDefintion )


////////////////////////////////////////////////////////////////////////
// CBehTreeNodeConditionIsInChatSceneInstance
////////////////////////////////////////////////////////////////////////
Bool CBehTreeNodeConditionIsInChatSceneInstance::ConditionCheck()
{
	CActor* actor = m_owner->GetActor();

	return actor->IsPlayingChatScene();
}

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeConditionIsSpeakingInstance
////////////////////////////////////////////////////////////////////////
Bool CBehTreeNodeConditionIsSpeakingInstance::ConditionCheck()
{
	CActor* actor = m_owner->GetActor();

	return actor->IsSpeaking() || actor->IsPlayingChatScene();
}

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeConditionHasVoicesetInstance
////////////////////////////////////////////////////////////////////////
Bool CBehTreeNodeConditionHasVoicesetInstance::ConditionCheck()
{
	CActor* actor = m_owner->GetActor();

	return actor->HasVoiceset( m_voiceSet ) == ASR_ReadyTrue;
}

