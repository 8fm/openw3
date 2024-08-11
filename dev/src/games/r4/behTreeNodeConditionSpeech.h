/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../../common/game/behTreeNodeCondition.h"

class IBehTreeNodeConditionSpeechInstance;
class CBehTreeNodeConditionIsInChatSceneInstance;
class CBehTreeNodeConditionIsSpeakingInstance;
class CBehTreeNodeConditionHasVoicesetInstance;

////////////////////////////////////////////////////////////////////////
// Abstract class that bands together all speech related condition
////////////////////////////////////////////////////////////////////////
class IBehTreeNodeConditionSpeechDefinition : public CBehTreeNodeConditionDefinition
{
	DECLARE_BEHTREE_ABSTRACT_NODE( IBehTreeNodeConditionSpeechDefinition, CBehTreeNodeConditionDefinition, IBehTreeNodeConditionSpeechInstance, SpeechCondition )
};

BEGIN_ABSTRACT_CLASS_RTTI( IBehTreeNodeConditionSpeechDefinition )
	PARENT_CLASS( CBehTreeNodeConditionDefinition )
END_CLASS_RTTI()

class IBehTreeNodeConditionSpeechInstance : public CBehTreeNodeConditionInstance
{
	typedef CBehTreeNodeConditionInstance Super;
public:
	typedef IBehTreeNodeConditionSpeechDefinition Definition;

	IBehTreeNodeConditionSpeechInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )							{}
};

////////////////////////////////////////////////////////////////////////
// IsInChatScene condition
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeConditionIsInChatSceneDefinition : public IBehTreeNodeConditionSpeechDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeConditionIsInChatSceneDefinition, IBehTreeNodeConditionSpeechDefinition, CBehTreeNodeConditionIsInChatSceneInstance, IsInChatScene )
protected:
	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeConditionIsInChatSceneDefinition )
	PARENT_CLASS( IBehTreeNodeConditionSpeechDefinition )
END_CLASS_RTTI()

class CBehTreeNodeConditionIsInChatSceneInstance : public IBehTreeNodeConditionSpeechInstance
{
	typedef IBehTreeNodeConditionSpeechInstance Super;

protected:
	Bool ConditionCheck() override;
public:
	typedef CBehTreeNodeConditionIsInChatSceneDefinition Definition;

	CBehTreeNodeConditionIsInChatSceneInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )							{}
};

////////////////////////////////////////////////////////////////////////
// IsSpeaking condition (IsInChatScene + actor->IsSpeaking())
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeConditionIsSpeakingDefinition : public CBehTreeNodeConditionIsInChatSceneDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeConditionIsSpeakingDefinition, CBehTreeNodeConditionIsInChatSceneDefinition, CBehTreeNodeConditionIsSpeakingInstance, IsSpeaking )
protected:
	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeConditionIsSpeakingDefinition )
	PARENT_CLASS( CBehTreeNodeConditionIsInChatSceneDefinition )
END_CLASS_RTTI()

class CBehTreeNodeConditionIsSpeakingInstance : public CBehTreeNodeConditionIsInChatSceneInstance
{
	typedef CBehTreeNodeConditionIsInChatSceneInstance Super;

protected:
	Bool ConditionCheck() override;
public:
	typedef CBehTreeNodeConditionIsSpeakingDefinition Definition;

	CBehTreeNodeConditionIsSpeakingInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )							{}
};

////////////////////////////////////////////////////////////////////////
// Has voiceset
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeConditionHasVoicesetDefintion : public IBehTreeNodeConditionSpeechDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeConditionHasVoicesetDefintion, IBehTreeNodeConditionSpeechDefinition, CBehTreeNodeConditionHasVoicesetInstance, HasVoiceset )
protected:
	CBehTreeValString							m_voiceSet;

	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeConditionHasVoicesetDefintion )
	PARENT_CLASS( IBehTreeNodeConditionSpeechDefinition )
	PROPERTY_EDIT( m_voiceSet, TXT("Voiceset name") )
END_CLASS_RTTI()

class CBehTreeNodeConditionHasVoicesetInstance : public IBehTreeNodeConditionSpeechInstance
{
	typedef IBehTreeNodeConditionSpeechInstance Super;

protected:
	String										m_voiceSet;

	Bool ConditionCheck() override;
public:
	typedef CBehTreeNodeConditionHasVoicesetDefintion Definition;

	CBehTreeNodeConditionHasVoicesetInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )							{ def.m_voiceSet.GetValRef( context, m_voiceSet ); }
};