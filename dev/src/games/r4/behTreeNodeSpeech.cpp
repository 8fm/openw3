/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeNodeSpeech.h"


BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodePlayVoicesetDecoratorDefinition )
BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodePlayVoicesetOnDeactivationDecoratorDefinition )


////////////////////////////////////////////////////////////////////////
// CBehTreeNodePlayVoicesetDecoratorDefinition
////////////////////////////////////////////////////////////////////////
CBehTreeNodePlayVoicesetDecoratorDefinition::CBehTreeNodePlayVoicesetDecoratorDefinition()
	: m_voiceSet()
	, m_voicePriority( 0 )
	, m_minSpeechDelay( -1.f )
	, m_maxSpeechDelay( -1.f )
	, m_waitUntilSpeechIsFinished( true )
	, m_dontActivateWhileSpeaking( false )
	, m_breakCurrentSpeach( false )
{

}
////////////////////////////////////////////////////////////////////////
// CBehTreeNodePlayVoicesetDecoratorInstance
////////////////////////////////////////////////////////////////////////
CBehTreeNodePlayVoicesetDecoratorInstance::CBehTreeNodePlayVoicesetDecoratorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
	, m_minSpeechDelay( def.m_minSpeechDelay )
	, m_maxSpeechDelay( def.m_maxSpeechDelay )
	, m_voicePriority( Int8( def.m_voicePriority.GetVal( context ) ) )
	, m_waitUntilSpeechIsFinished( def.m_waitUntilSpeechIsFinished )
	, m_dontActivateWhileSpeaking( def.m_dontActivateWhileSpeaking )
	, m_breakCurrentSpeach( def.m_breakCurrentSpeach )
{
	def.m_voiceSet.GetValRef( context, m_voiceSet );
}

CBehTreeNodePlayVoicesetDecoratorInstance::~CBehTreeNodePlayVoicesetDecoratorInstance()
{

}

Bool CBehTreeNodePlayVoicesetDecoratorInstance::IsAvailable()
{
	if ( m_dontActivateWhileSpeaking )
	{
		if ( m_owner->GetActor()->IsSpeaking() )
		{
			DebugNotifyAvailableFail();
			return false;
		}
	}

	return Super::IsAvailable();
}
Int32 CBehTreeNodePlayVoicesetDecoratorInstance::Evaluate()
{
	if ( m_dontActivateWhileSpeaking )
	{
		if ( m_owner->GetActor()->IsSpeaking() )
		{
			DebugNotifyAvailableFail();
			return -1;
		}
	}

	return Super::Evaluate();
}

Bool CBehTreeNodePlayVoicesetDecoratorInstance::Activate()
{
	m_firedVoice = false;
	m_speechDelay = m_maxSpeechDelay > 0.f ? m_owner->GetLocalTime() + GEngine->GetRandomNumberGenerator().Get< Float >( m_minSpeechDelay, m_maxSpeechDelay ) : -1.f;

	return Super::Activate();
}

void CBehTreeNodePlayVoicesetDecoratorInstance::Update()
{
	if ( !m_firedVoice )
	{
		if ( m_speechDelay <= m_owner->GetLocalTime() )
		{
			m_owner->GetActor()->PlayVoiceset( EArbitratorPriorities( m_voicePriority ), m_voiceSet, m_breakCurrentSpeach );
			m_firedVoice = true;
			if ( !m_waitUntilSpeechIsFinished )
			{
				Complete( BTTO_SUCCESS );
				return;
			}
		}
	}
	// voice was fired previously, and we are waiting until speech is finished
	else if ( !m_owner->GetActor()->IsSpeaking() )
	{
		Complete( BTTO_SUCCESS );
		return;
	}

	Super::Update();
}

////////////////////////////////////////////////////////////////////////
// CBehTreeNodePlayVoicesetOnDeactivationDecoratorDefinition
////////////////////////////////////////////////////////////////////////
CBehTreeNodePlayVoicesetOnDeactivationDecoratorDefinition::CBehTreeNodePlayVoicesetOnDeactivationDecoratorDefinition()
	: m_voiceSet()
	, m_voicePriority( 0 )
	, m_playAfterXTimes( 0 )
	, m_chanceToPlay( 1.f )
	, m_breakCurrentSpeach( false )
{

}
CBehTreeNodePlayVoicesetOnDeactivationDecoratorDefinition::~CBehTreeNodePlayVoicesetOnDeactivationDecoratorDefinition()
{

}

////////////////////////////////////////////////////////////////////////
// CBehTreeNodePlayVoicesetOnDeactivationDecoratorInstance
////////////////////////////////////////////////////////////////////////
CBehTreeNodePlayVoicesetOnDeactivationDecoratorInstance::CBehTreeNodePlayVoicesetOnDeactivationDecoratorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
	, m_voicePriority( Int8( def.m_voicePriority.GetVal( context ) ) )
	, m_playAfterXTimes( def.m_playAfterXTimes )
	, m_chanceToPlay( def.m_chanceToPlay )
	, m_timesCounter( 0 )
	, m_breakCurrentSpeach( def.m_breakCurrentSpeach )
{
	def.m_voiceSet.GetValRef( context, m_voiceSet );
}


void CBehTreeNodePlayVoicesetOnDeactivationDecoratorInstance::Deactivate()
{
	if ( ++m_timesCounter >= m_playAfterXTimes )
	{
		m_timesCounter = 0;
		if ( m_chanceToPlay >= 1.f ||
			m_chanceToPlay >= GEngine->GetRandomNumberGenerator().Get< Float >() )
		{
			m_owner->GetActor()->PlayVoiceset( EArbitratorPriorities( m_voicePriority ), m_voiceSet, m_breakCurrentSpeach );
		}
	}


	Super::Deactivate();
}