#pragma once

#include "behTreeNodeAtomicAction.h"
#include "behTreeVarsEnums.h"

class CScriptedExplorationTraverser;
class ActorActionExploration;

class CBehTreeNodeUseExplorationActionInstance;
class CBehTreeNodeTeleportToMetalinkDestinationInstance;

class CBehTreeNodeUseExplorationActionDefinition : public CBehTreeNodeAtomicActionDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeUseExplorationActionDefinition, CBehTreeNodeAtomicActionDefinition, CBehTreeNodeUseExplorationActionInstance, UseExplorationAction )
protected:	
	CBehTreeValEExplorationType	m_explorationType;
	CBehTreeValCName			m_entityTag;
protected:
	IBehTreeNodeInstance*	SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent  ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeUseExplorationActionDefinition )
	PARENT_CLASS( CBehTreeNodeAtomicActionDefinition )
	PROPERTY_EDIT( m_explorationType, TXT( "" ) );
	PROPERTY_EDIT( m_entityTag		, TXT( "" ) );
END_CLASS_RTTI()

class CBehTreeNodeUseExplorationActionInstance : public CBehTreeNodeAtomicActionInstance
{
	typedef CBehTreeNodeAtomicActionInstance Super;

private:
	CName									m_entityTag;
	THandle< CComponent >					m_metalinkComponent;
	EExplorationType						m_explorationType;
	SExplorationQueryToken					m_explorationToken;	
	ActorActionExploration*					m_explorationAction;
	THandle< CScriptedExplorationTraverser >m_traverser;	
	Float									m_prevLocalTime;
	Bool									m_waitingForComplete;
public:
	typedef CBehTreeNodeUseExplorationActionDefinition Definition;

	CBehTreeNodeUseExplorationActionInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

	Bool IsAvailable() override;
	Bool Activate() override;
	void Deactivate() override;
	void Update() override;
	void EnableProperWalking();
	Bool Interrupt() override;
};



// teleport to metalink destination
class CBehTreeNodeTeleportToMetalinkDestinationDefinition : public CBehTreeNodeAtomicActionDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeTeleportToMetalinkDestinationDefinition, CBehTreeNodeAtomicActionDefinition, CBehTreeNodeTeleportToMetalinkDestinationInstance, TeleportToMetalinkDestination )
protected:
	EExplorationType m_explorationType;
	CBehTreeValBool	 m_skipTeleportation;

public:
	CBehTreeNodeTeleportToMetalinkDestinationDefinition()
		: m_skipTeleportation( false ){}
protected:
	IBehTreeNodeInstance*	SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent  ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeTeleportToMetalinkDestinationDefinition )
	PARENT_CLASS( CBehTreeNodeAtomicActionDefinition )	
	PROPERTY_EDIT( m_skipTeleportation, TXT( "" ) );
END_CLASS_RTTI()

class CBehTreeNodeTeleportToMetalinkDestinationInstance : public CBehTreeNodeAtomicActionInstance
{
	typedef CBehTreeNodeAtomicActionInstance Super;

private:
	Vector3	m_target;
	Float	m_heading;
	Bool	m_skipTeleportation;

public:
	typedef CBehTreeNodeTeleportToMetalinkDestinationDefinition Definition;

	CBehTreeNodeTeleportToMetalinkDestinationInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );
	
	void Update() override;
};