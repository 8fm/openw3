#pragma once

#include "behTreeNodeSpecial.h"
#include "behTreeDynamicNodeBase.h"

class CBehTreeDynamicNodeInstance;

///////////////////////////////////////////////////////////////////////////////
// Dynamically created game node - base class.
///////////////////////////////////////////////////////////////////////////////
class IBehTreeDynamicNodeBaseDefinition : public IBehTreeNodeSpecialDefinition
{
	DECLARE_BEHTREE_ABSTRACT_NODE( IBehTreeDynamicNodeBaseDefinition, IBehTreeNodeSpecialDefinition, CBehTreeDynamicNodeInstance, DynamicNodeBase );
public:
	IBehTreeDynamicNodeBaseDefinition()											{}

	// special interface to parametrize instance
	virtual CName			GetEventName( CBehTreeInstance* owner, CBehTreeSpawnContext& context ) const = 0;
	virtual IAITree*		GetBaseDefinition( CBehTreeInstance* owner, CBehTreeSpawnContext& context ) const;

	IBehTreeNodeInstance*	SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const override;
	Bool					OnSpawn( IBehTreeNodeInstance* node, CBehTreeSpawnContext& context ) const override;
};

BEGIN_ABSTRACT_CLASS_RTTI( IBehTreeDynamicNodeBaseDefinition );
	PARENT_CLASS( IBehTreeNodeSpecialDefinition );
END_CLASS_RTTI();


class CBehTreeDynamicNodeInstance : public IBehTreeNodeInstance, public IBehTreeDynamicNodeBase 
{
	typedef IBehTreeNodeInstance Super;
public:
	typedef IBehTreeDynamicNodeBaseDefinition Definition;
protected:
	CName					m_eventName;
	THandle< Definition >	m_baseDefinition;

	virtual void RequestChildDespawn();

	Bool HandleSpawnEvent( SBehTreeDynamicNodeEventData& eventData );
	void HandleSaveEvent( SDynamicNodeSaveStateRequestEventData& saveEventData );
	Bool HandleEvent( SBehTreeDynamicNodeCancelEventData& eventData );
	void HandlePoolRequest();
public:

	CBehTreeDynamicNodeInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL );
	~CBehTreeDynamicNodeInstance();
	void OnDestruction() override;

	////////////////////////////////////////////////////////////////////
	// Lifecycle
	void Update() override;
	Bool Activate() override;
	void Deactivate() override;
	void OnSubgoalCompleted( eTaskOutcome outcome ) override;

	////////////////////////////////////////////////////////////////////
	//! Event handling
	Bool OnEvent( CBehTreeEvent& e ) override;
	Bool OnListenedEvent( CBehTreeEvent& e ) override;

	////////////////////////////////////////////////////////////////////
	//! Evaluation
	Bool IsAvailable() override;
	Int32 Evaluate() override;

	////////////////////////////////////////////////////////////////////
	//! Custom interface
	Bool Interrupt() override;

	////////////////////////////////////////////////////////////////////
	//! Handling children
	Int32 GetNumChildren() const override;
	Int32 GetNumPersistantChildren() const override;
	IBehTreeNodeInstance* GetChild( Int32 index ) const override;
	IBehTreeNodeInstance* GetActiveChild() const override;

	IBehTreeNodeInstance* GetDynamicChildNode() const					{ return m_childNode; }
};

///////////////////////////////////////////////////////////////////////////////
// Dynamically created game node - base class.
///////////////////////////////////////////////////////////////////////////////
class CBehTreeDynamicNodeDefinition : public IBehTreeDynamicNodeBaseDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeDynamicNodeDefinition, IBehTreeDynamicNodeBaseDefinition, CBehTreeDynamicNodeInstance, DynamicNode );
protected:
	CName					m_dynamicEventName;
	CName					m_baseTreeVar;
	THandle< CAITree >		m_baseTree;
public:
	CBehTreeDynamicNodeDefinition()												{}

	// IBehTreeDynamicNodeBaseDefinition interface
	CName GetEventName( CBehTreeInstance* owner, CBehTreeSpawnContext& context ) const override;
	IAITree* GetBaseDefinition( CBehTreeInstance* owner, CBehTreeSpawnContext& context ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeDynamicNodeDefinition );
	PARENT_CLASS( IBehTreeDynamicNodeBaseDefinition );
	PROPERTY_EDIT( m_dynamicEventName, TXT("Gameplay event name for horse spawning") );
	PROPERTY_EDIT( m_baseTreeVar, TXT("Initial tree variable name") );
	PROPERTY_EDIT( m_baseTree, TXT("Default initial tree") );
END_CLASS_RTTI();