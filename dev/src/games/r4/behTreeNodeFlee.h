/*
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "../../common/game/behTreeNode.h"
#include "../../common/game/behTreeNodeCondition.h"

struct SBecomeScaredEventData
{
	DECLARE_RTTI_STRUCT( SBecomeScaredEventData );
public:
	SBecomeScaredEventData(){}
	SBecomeScaredEventData( CNode* threadSource, Float scaredTime  )
		: m_threadSource( threadSource )
		, m_scaredTime( scaredTime )
	{}

	THandle< CNode >	m_threadSource;
	Float				m_scaredTime;
};

BEGIN_CLASS_RTTI( SBecomeScaredEventData );		
END_CLASS_RTTI();

// Sending scared events
class CBehTreeNodeSendScaredEventInstance;

class CBehTreeNodeSendScaredEventDefinition : public IBehTreeNodeDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeSendScaredEventDefinition, IBehTreeNodeDefinition, CBehTreeNodeSendScaredEventInstance, SendScaredEvent );
protected:
	Bool				m_becomeScared;
	CBehTreeValFloat	m_scaredTime;
	CBehTreeValFloat	m_scaredTimeMax;
public:
	CBehTreeNodeSendScaredEventDefinition()
		: m_becomeScared( true )
		, m_scaredTime( 0 )
		, m_scaredTimeMax( 0 )
	{}

	IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const override;

};

BEGIN_CLASS_RTTI( CBehTreeNodeSendScaredEventDefinition );
	PARENT_CLASS( IBehTreeNodeDefinition );	
	PROPERTY_EDIT( m_becomeScared	, TXT("If npc should become scared or stop being scared") );
	PROPERTY_EDIT( m_scaredTime		, TXT("How long NPC is scared Min") );
	PROPERTY_EDIT( m_scaredTimeMax	, TXT("How long NPC is scared Max") );	
END_CLASS_RTTI();

class CBehTreeNodeSendScaredEventInstance : public IBehTreeNodeInstance
{
protected:
	Bool			m_becomeScared;
	Float			m_scaredTime;
	Float			m_scaredTimeMax;
public:
	typedef CBehTreeNodeSendScaredEventDefinition Definition;

	CBehTreeNodeSendScaredEventInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );
	
	void Update() override;
};


// Listening scared events
class CBehTreeNodeReceiveScaredEventInstance;

class CBehTreeNodeReceiveScaredEventDefinition : public IBehTreeNodeDecoratorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeReceiveScaredEventDefinition, IBehTreeNodeDecoratorDefinition, CBehTreeNodeReceiveScaredEventInstance, ReceiveScaredEvent );
protected:	

	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;	
};

BEGIN_CLASS_RTTI(CBehTreeNodeReceiveScaredEventDefinition);
	PARENT_CLASS(IBehTreeNodeDecoratorDefinition);	
END_CLASS_RTTI();

class CBehTreeNodeReceiveScaredEventInstance : public IBehTreeNodeDecoratorInstance
{
	typedef  IBehTreeNodeDecoratorInstance Super;
protected:		
	THandle< CNode >	m_threatSource;
	Bool				m_isScared;
	Float				m_scareTimeout;	

public:
	typedef CBehTreeNodeReceiveScaredEventDefinition Definition;

	CBehTreeNodeReceiveScaredEventInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL );
	Bool OnListenedEvent( CBehTreeEvent& e ) override;	
	void OnDestruction() override;	
	void Update() override;
	Bool IsAvailable() override;
	Int32 Evaluate() override;	
	void Deactivate() override;
};