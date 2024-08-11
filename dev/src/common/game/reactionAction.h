/**
* Copyright © 2010-2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CInterestPointInstance;

///////////////////////////////////////////////////////////////////////////////

class IReactionAction : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IReactionAction, CObject );

public:
	virtual ~IReactionAction() {}

	// Performs the action on the selected npc
	virtual void Perform( CNewNPC* npc, CInterestPointInstance* interestPoint, Int32 reactionIndex ) {}

};
BEGIN_ABSTRACT_CLASS_RTTI( IReactionAction );
	PARENT_CLASS( CObject );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////

class CReactionSendEvent : public IReactionAction
{
	DECLARE_ENGINE_CLASS( CReactionSendEvent, IReactionAction, 0 );

private:
	CName m_eventName;

public:
	virtual void Perform( CNewNPC* npc, CInterestPointInstance* interestPointInstance, Int32 reactionIndex );
};

BEGIN_CLASS_RTTI( CReactionSendEvent );
	PARENT_CLASS( IReactionAction );
	PROPERTY_EDIT( m_eventName, TXT( "Name of the event to be sent" ));
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////

class CReactionQuestNotification : public IReactionAction
{
	DECLARE_ENGINE_CLASS( CReactionQuestNotification, IReactionAction, 0 );

public:
	virtual void Perform( CNewNPC* npc, CInterestPointInstance* interestPoint, Int32 reactionIndex );
};
BEGIN_CLASS_RTTI( CReactionQuestNotification );
	PARENT_CLASS( IReactionAction );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////

class CReactionAttitudeChange : public IReactionAction
{
	DECLARE_ENGINE_CLASS( CReactionAttitudeChange, IReactionAction, 0 );

private:
	EAIAttitude		m_attitude;
	Bool			m_towardSource;
	Bool			m_noticeActor;

public:
	CReactionAttitudeChange()
		: m_attitude( AIA_Hostile )
		, m_towardSource( false )
		, m_noticeActor( false )
	{
	}

	virtual void Perform( CNewNPC* npc, CInterestPointInstance* interestPoint, Int32 reactionIndex );
};
BEGIN_CLASS_RTTI( CReactionAttitudeChange );
	PARENT_CLASS( IReactionAction );
	PROPERTY_EDIT( m_attitude, TXT( "New attitude") );
	PROPERTY_EDIT( m_towardSource, TXT( "Sets attitude toward source, not toward player") );
	PROPERTY_EDIT( m_noticeActor, TXT( "Notice player or source npc") );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////

class CReactionScript : public IReactionAction
{
	DECLARE_ENGINE_CLASS( CReactionScript, IReactionAction, 0 );

public:
	virtual void Perform( CNewNPC* npc, CInterestPointInstance* interestPoint, Int32 reactionIndex );
};
BEGIN_CLASS_RTTI( CReactionScript );
	PARENT_CLASS( IReactionAction );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////

class CReactionLookAtAction : public IReactionAction
{
	DECLARE_ENGINE_CLASS( CReactionLookAtAction, IReactionAction, 0 );

private:
	EReactionLookAtType m_lookAtType;

public:
	CReactionLookAtAction() : m_lookAtType(RLT_None) {}
	virtual void Perform( CNewNPC* npc, CInterestPointInstance* interestPoint, Int32 reactionIndex );
};

BEGIN_CLASS_RTTI( CReactionLookAtAction );
	PARENT_CLASS( IReactionAction );
	PROPERTY_EDIT( m_lookAtType, TXT( "Look at type" ) )
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////

class CReactionVoiceAction : public IReactionAction
{
	DECLARE_ENGINE_CLASS( CReactionVoiceAction, IReactionAction, 0 );

private:
	String m_voiceset;
	Float m_cooldown;

public:
	CReactionVoiceAction()
		: m_cooldown( 10.0f )
	{}

	virtual void Perform( CNewNPC* npc, CInterestPointInstance* interestPoint, Int32 reactionIndex );

};

BEGIN_CLASS_RTTI( CReactionVoiceAction );
	PARENT_CLASS( IReactionAction );
	PROPERTY_EDIT( m_voiceset, TXT( "Voiceset this reaction triggers" ) )
	PROPERTY_EDIT( m_cooldown, TXT( "How often the voiceset gets played" ) )
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////

class CReactionMultiAction : public IReactionAction
{
	DECLARE_ENGINE_CLASS( CReactionMultiAction, IReactionAction, 0 );

private:
	TDynArray<IReactionAction*> m_actions;

public:
	virtual void Perform( CNewNPC* npc, CInterestPointInstance* interestPoint, Int32 reactionIndex );
};

BEGIN_CLASS_RTTI( CReactionMultiAction );
	PARENT_CLASS( IReactionAction );
	PROPERTY_INLINED( m_actions, TXT( "Set of actions to be executed" ) )
END_CLASS_RTTI();
