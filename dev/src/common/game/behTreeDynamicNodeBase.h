/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "behTreeNode.h"

class IBehTreeDynamicNodeBase
{
protected:
	IBehTreeNodeInstance*	m_childNode;

	~IBehTreeDynamicNodeBase();

	Bool SpawnChildNodeWithContext( IAITree* treeDef, CBehTreeSpawnContext& context, IBehTreeNodeInstance* node, CBehTreeInstance* owner );
	Bool SpawnChildNode( IAITree* treeDef, const SBehTreeDynamicNodeEventData::Parameters& params, IBehTreeNodeInstance* node, CBehTreeInstance* owner );
	void DespawnChildNode();


	////////////////////////////////////////////////////////////////////
	// Lifecycle
	void Update();
	Bool Activate();
	void Deactivate();
	void OnSubgoalCompleted( IBehTreeNodeInstance* node, IBehTreeNodeInstance::eTaskOutcome outcome );

	////////////////////////////////////////////////////////////////////
	//! Event handling
	Bool OnEvent( CBehTreeEvent& e );

	////////////////////////////////////////////////////////////////////
	//! Evaluation
	Bool IsAvailable();
	Int32 Evaluate();

	////////////////////////////////////////////////////////////////////
	//! Custom interface
	Bool Interrupt() const;

	////////////////////////////////////////////////////////////////////
	//! Handling children
	Int32 GetNumChildren() const;
	IBehTreeNodeInstance* GetChild( Int32 index ) const;
	IBehTreeNodeInstance* GetActiveChild() const;

public:
	IBehTreeDynamicNodeBase()
		: m_childNode( NULL )														{}


};