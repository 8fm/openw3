
#pragma once

#include "lookAtTypes.h"
#include "../engine/triggerAreaComponent.h"

//////////////////////////////////////////////////////////////////////////

class IBgNpcTriggerAction : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IBgNpcTriggerAction, CObject );

public:
	virtual void Init( CWorld* world ) {}
	virtual void Destroy( CWorld* world ) {}

	virtual void Start( Bool player, CComponent* component ) {}
	virtual void Stop( Bool player, CComponent* component ) {}
};

BEGIN_ABSTRACT_CLASS_RTTI( IBgNpcTriggerAction );
	PARENT_CLASS( CObject );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBgNpcTriggerActionTalk : public IBgNpcTriggerAction
{
	DECLARE_ENGINE_CLASS( CBgNpcTriggerActionTalk, IBgNpcTriggerAction, 0 );

public:
	virtual void Start( Bool player, CComponent* component );
	virtual void Stop( Bool player, CComponent* component );
};

BEGIN_CLASS_RTTI( CBgNpcTriggerActionTalk );
	PARENT_CLASS( IBgNpcTriggerAction );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBgNpcTriggerActionLookAt : public IBgNpcTriggerAction
{
	DECLARE_ENGINE_CLASS( CBgNpcTriggerActionLookAt, IBgNpcTriggerAction, 0 );

protected:
	EReactionLookAtType	m_type;
	Bool				m_onlyPlayer;
	Float				m_maxDelay;

public:
	CBgNpcTriggerActionLookAt();

	virtual void Start( Bool player, CComponent* component );
	virtual void Stop( Bool player, CComponent* component );
};

BEGIN_CLASS_RTTI( CBgNpcTriggerActionLookAt );
	PARENT_CLASS( IBgNpcTriggerAction );
	PROPERTY_EDIT( m_type, TXT("") );
	PROPERTY_EDIT( m_onlyPlayer, TXT("") );
	PROPERTY_EDIT( m_maxDelay, TXT("") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBgNpcTriggerActionSwordReaction : public IBgNpcTriggerAction
{
	DECLARE_ENGINE_CLASS( CBgNpcTriggerActionSwordReaction, IBgNpcTriggerAction, 0 );
};

BEGIN_CLASS_RTTI( CBgNpcTriggerActionSwordReaction );
	PARENT_CLASS( IBgNpcTriggerAction );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class CBgNpcTriggerComponent : public CTriggerAreaComponent
{
	DECLARE_ENGINE_CLASS( CBgNpcTriggerComponent, CTriggerAreaComponent, 0 );

	TDynArray< IBgNpcTriggerAction* > m_actions;

public:
	virtual void OnAttached( CWorld* world );
	virtual void OnDetached( CWorld* world );

public:
	virtual void EnteredArea( CComponent* component );
	virtual void ExitedArea( CComponent* component );

#ifndef NO_EDITOR
public:
	void AddVoicesetOption();
	void AddLookAtOption();

	void InitializeComponent();

#endif
};

BEGIN_CLASS_RTTI( CBgNpcTriggerComponent );
	PARENT_CLASS( CTriggerAreaComponent );
	PROPERTY_INLINED( m_actions, TXT("Actions") );
END_CLASS_RTTI();
