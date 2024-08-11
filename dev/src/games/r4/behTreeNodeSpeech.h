/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../../common/game/behTreeDecorator.h"

class IBehTreeNodeSpeechDecoratorInstance;
class CBehTreeNodePlayVoicesetDecoratorInstance;
class CBehTreeNodePlayVoicesetOnDeactivationDecoratorInstance;

////////////////////////////////////////////////////////////////////////
// Abstract class that bands together all speech related decorators
////////////////////////////////////////////////////////////////////////
class IBehTreeNodeSpeechDecoratorDefinition : public IBehTreeNodeDecoratorDefinition
{
	DECLARE_BEHTREE_ABSTRACT_NODE( IBehTreeNodeSpeechDecoratorDefinition, IBehTreeNodeDecoratorDefinition, IBehTreeNodeSpeechDecoratorInstance, Speech )
};

BEGIN_ABSTRACT_CLASS_RTTI( IBehTreeNodeSpeechDecoratorDefinition )
	PARENT_CLASS( IBehTreeNodeDecoratorDefinition )
END_CLASS_RTTI()

class IBehTreeNodeSpeechDecoratorInstance : public IBehTreeNodeDecoratorInstance
{
	typedef IBehTreeNodeDecoratorInstance Super;
public:
	typedef IBehTreeNodeSpeechDecoratorDefinition Definition;

	IBehTreeNodeSpeechDecoratorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )							{}
};


////////////////////////////////////////////////////////////////////////
// Play voiceset
////////////////////////////////////////////////////////////////////////
class CBehTreeNodePlayVoicesetDecoratorDefinition : public IBehTreeNodeSpeechDecoratorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodePlayVoicesetDecoratorDefinition, IBehTreeNodeSpeechDecoratorDefinition, CBehTreeNodePlayVoicesetDecoratorInstance, PlayVoiceset )
protected:
	CBehTreeValString			m_voiceSet;
	CBehTreeValInt				m_voicePriority;
	Float						m_minSpeechDelay;
	Float						m_maxSpeechDelay;
	Bool						m_waitUntilSpeechIsFinished;
	Bool						m_dontActivateWhileSpeaking;
	Bool						m_breakCurrentSpeach;

	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
public:
	CBehTreeNodePlayVoicesetDecoratorDefinition();
};

BEGIN_CLASS_RTTI( CBehTreeNodePlayVoicesetDecoratorDefinition )
	PARENT_CLASS( IBehTreeNodeSpeechDecoratorDefinition )
	PROPERTY_EDIT( m_voiceSet, TXT("Voiceset name") )
	PROPERTY_EDIT( m_voicePriority, TXT("Voice speech priority") )
	PROPERTY_EDIT( m_minSpeechDelay, TXT("Minimum speech delay") )
	PROPERTY_EDIT( m_maxSpeechDelay, TXT("Maximum speech delay") )
	PROPERTY_EDIT( m_waitUntilSpeechIsFinished, TXT("If set to false node will complete as soon as it will hit voice playing.") )
	PROPERTY_EDIT( m_dontActivateWhileSpeaking, TXT("IF set to true node will be unavailable if 'IsSpeaking'.") )
END_CLASS_RTTI()

class CBehTreeNodePlayVoicesetDecoratorInstance : public IBehTreeNodeSpeechDecoratorInstance
{
	typedef IBehTreeNodeSpeechDecoratorInstance Super;
protected:
	// setup
	String						m_voiceSet;
	Float						m_minSpeechDelay;
	Float						m_maxSpeechDelay;

	// runtime
	Float						m_speechDelay;

	// setup flags
	Int8						m_voicePriority;
	Bool						m_waitUntilSpeechIsFinished;
	Bool						m_dontActivateWhileSpeaking;
	Bool						m_breakCurrentSpeach;

	// runtime flags
	Bool						m_firedVoice;
	

public:
	typedef CBehTreeNodePlayVoicesetDecoratorDefinition Definition;

	CBehTreeNodePlayVoicesetDecoratorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );
	~CBehTreeNodePlayVoicesetDecoratorInstance();

	Bool						IsAvailable() override;
	Int32						Evaluate() override;

	Bool						Activate() override;

	void						Update() override;
};


////////////////////////////////////////////////////////////////////////
// Play voiceset on deactivation
////////////////////////////////////////////////////////////////////////
class CBehTreeNodePlayVoicesetOnDeactivationDecoratorDefinition : public IBehTreeNodeSpeechDecoratorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodePlayVoicesetOnDeactivationDecoratorDefinition, IBehTreeNodeSpeechDecoratorDefinition, CBehTreeNodePlayVoicesetOnDeactivationDecoratorInstance, PlayVoicesetOnDeactivation )
protected:
	CBehTreeValString			m_voiceSet;
	CBehTreeValInt				m_voicePriority;

	Uint16						m_playAfterXTimes;
	Float						m_chanceToPlay;
	Bool						m_breakCurrentSpeach;

	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
public:
	CBehTreeNodePlayVoicesetOnDeactivationDecoratorDefinition();
	~CBehTreeNodePlayVoicesetOnDeactivationDecoratorDefinition();
};

BEGIN_CLASS_RTTI( CBehTreeNodePlayVoicesetOnDeactivationDecoratorDefinition )
	PARENT_CLASS( IBehTreeNodeSpeechDecoratorDefinition )
	PROPERTY_EDIT( m_voiceSet, TXT("Voiceset name") )
	PROPERTY_EDIT( m_voicePriority, TXT("Voice speech priority") )
	PROPERTY_EDIT( m_playAfterXTimes, TXT("Play voiceset after given number of deactivates") )
	PROPERTY_EDIT( m_chanceToPlay, TXT("Chance to play voiceset. [0..1]") )
END_CLASS_RTTI()

class CBehTreeNodePlayVoicesetOnDeactivationDecoratorInstance : public IBehTreeNodeSpeechDecoratorInstance
{
	typedef IBehTreeNodeSpeechDecoratorInstance Super;
protected:
	// setup
	String						m_voiceSet;
	Int8						m_voicePriority;
	Float						m_chanceToPlay;
	Uint16						m_playAfterXTimes;

	// runtime
	Uint16						m_timesCounter;
	Bool						m_breakCurrentSpeach;

public:
	typedef CBehTreeNodePlayVoicesetOnDeactivationDecoratorDefinition Definition;

	CBehTreeNodePlayVoicesetOnDeactivationDecoratorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

	void Deactivate() override;
};