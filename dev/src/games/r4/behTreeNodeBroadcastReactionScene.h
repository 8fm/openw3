#pragma once


#include "behTreeNodeTicketDecoratorBase.h"

#include "../../common/game/reactionSceneActor.h"
#include "../../common/game/behTreeWorkData.h"

class CBehTreeNodeBroadcastReactionSceneInstance;

struct SReactionSceneEvent
{
	DECLARE_RTTI_STRUCT( SReactionSceneEvent )
	CName				m_reactionScene;
	Float				m_weight;
	Bool				m_inputsAsymetric;// first input is required for invoker, second for receiver
	Bool				m_workOnlyBroadcast;
	TDynArray< String > m_requiredSceneInputs;
};
BEGIN_CLASS_RTTI( SReactionSceneEvent )
	PROPERTY_EDIT( m_reactionScene,  TXT("") );
	//PROPERTY_EDIT( m_weight,  TXT("") );
	PROPERTY_EDIT( m_requiredSceneInputs,  TXT("") );
	PROPERTY_EDIT( m_inputsAsymetric,  TXT("first input is required for invoker, second for receiver") );	
	PROPERTY_EDIT( m_workOnlyBroadcast,  TXT("first input is required for invoker, second for receiver") );	
END_CLASS_RTTI()

class CBehTreeNodeBroadcastReactionSceneDefinition : public IBehTreeNodeCombatTicketDecoratorBaseDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeBroadcastReactionSceneDefinition, IBehTreeNodeCombatTicketDecoratorBaseDefinition, CBehTreeNodeBroadcastReactionSceneInstance, BroadcastReactionScene );

protected:	
	TDynArray< SReactionSceneEvent >					m_reactionScenesToBroadcast;
	Float												m_updateInterval;	

	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;

public:
	CBehTreeNodeBroadcastReactionSceneDefinition()
		: m_updateInterval( 0.5f )	
		{}
};

BEGIN_CLASS_RTTI( CBehTreeNodeBroadcastReactionSceneDefinition );
	PARENT_CLASS( IBehTreeNodeCombatTicketDecoratorBaseDefinition );
	PROPERTY_EDIT( m_updateInterval, TXT("") );	
	PROPERTY_EDIT( m_reactionScenesToBroadcast, TXT("") );	
END_CLASS_RTTI();

class CBehTreeNodeBroadcastReactionSceneInstance : public IBehTreeNodeCombatTicketDecoratorBaseInstance
{
	typedef IBehTreeNodeCombatTicketDecoratorBaseInstance Super;
protected:		
	Float										m_updateInterval;
	TDynArray< SReactionSceneEvent >			m_reactionScenesToBroadcast;

	Float										m_nextUpdateTime;
	THandle< CReactionSceneActorComponent >		m_ownerReactionSceneComponent;		
	CBehTreeWorkDataPtr							m_workData;

private:
	void UpdateImpl();

	Bool IsRaining();

public:
	typedef CBehTreeNodeBroadcastReactionSceneDefinition Definition;

	CBehTreeNodeBroadcastReactionSceneInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )
		, m_updateInterval( def.m_updateInterval )	
		, m_reactionScenesToBroadcast( def.m_reactionScenesToBroadcast )
		, m_nextUpdateTime( 0 )		
		, m_workData( owner )
		{}

	void Update() override;

	Bool Activate() override;		
};