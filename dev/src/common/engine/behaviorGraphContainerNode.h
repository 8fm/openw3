/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma  once

#include "behaviorGraphNode.h"

//! Graph node containing other nodes
class CBehaviorGraphContainerNode : public CBehaviorGraphNode
{
	DECLARE_BEHAVIOR_ABSTRACT_CLASS( CBehaviorGraphContainerNode, CBehaviorGraphNode );

	friend class CUndoBehaviorGraphContainerNodeInput;

protected:
	TDynArray< CGraphBlock* >			m_nodes;				//!< Contained nodes
	TDynArray< CName >					m_animationInputs;		//!< Animation stream inputs
	TDynArray< CName >					m_valueInputs;			//!< Value inputs
	TDynArray< CName >					m_vectorValueInputs;	//!< Vector value inputs
	TDynArray< CName >					m_mimicInputs;			//!< Mimic inputs

public:
	CBehaviorGraphContainerNode();

	//! called after load
	void OnPostLoad();

	//! called during serialization
	void OnSerialize( IFile &file );

	//! Build block data layout
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );

	//! Initialize instance buffer
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	//! Destroy instance buffer
	virtual void OnReleaseInstance( CBehaviorGraphInstance& instance ) const;
	// virtual Bool IsOnReleaseInstanceManuallyOverridden() const override { return true; } This func shouldn't be called

	void CollectNodesToRelease( TDynArray< CBehaviorGraphNode* >& nodesToRelease );

	//! reset node to default state
	virtual void OnReset( CBehaviorGraphInstance& instance ) const;

	//! called during open in editor
	virtual void OnOpenInEditor( CBehaviorGraphInstance& instance ) const;

	//! Update animation cache
	virtual void OnUpdateAnimationCache( CBehaviorGraphInstance& instance ) const;

#ifndef NO_EDITOR_GRAPH_SUPPORT

	//! rebuild sockets
	virtual void OnRebuildSockets();

#endif

	//! process external event
	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const = 0;

	//! process force external event, return if the node processed event
	virtual Bool ProcessForceEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;

	//! get connected nodes that should be displayed lower level
	virtual TDynArray< CGraphBlock* >& GetConnectedChildren() { return m_nodes; }

	//! get connected nodes that should be displayed lower level
	virtual const TDynArray< CGraphBlock* >& GetConnectedChildren() const { return m_nodes; }

#ifndef NO_EDITOR_GRAPH_SUPPORT
	//! create children node
	virtual CGraphBlock* CreateChildNode( const GraphBlockSpawnInfo& info );

	//! called on adding new node
	virtual void OnChildNodeAdded( CGraphBlock* node );

	//! remove child node
	virtual void RemoveChildNode( CGraphBlock* node );

	//! check if given class can be child node
	virtual Bool ChildNodeClassSupported( CClass *nodeClass );

	//! create new animation input
	virtual void CreateAnimationInput( const CName& name );

	//! create new value input
	virtual void CreateValueInput( const CName& name );

	//! create new vector value input
	virtual void CreateVectorValueInput( const CName& name );

	//! create new mimic input
	virtual void CreateMimicInput( const CName& name );

	//! remove animation input
	virtual void RemoveAnimationInput( const CName& name );

	//! remove value input
	virtual void RemoveValueInput( const CName& name );

	//! remove vector value input
	virtual void RemoveVectorValueInput( const CName& name );

	//! remove mimic input
	virtual void RemoveMimicInput( const CName& name );

	//! Can be expanded in editor
	virtual Bool CanBeExpanded() const;
#endif

	//! access animation input list
	virtual const TDynArray< CName >& GetAnimationInputs() const;

	//! access value input list
	virtual const TDynArray< CName >& GetValueInputs() const;

	//! access vector value input list
	virtual const TDynArray< CName >& GetVectorValueInputs() const;

	//! access mimic input list
	virtual const TDynArray< CName >& GetMimicInputs() const;

	// Generate debug info
	virtual void OnGenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const;

	//! Cache block connections
	virtual void CacheConnections();

	//! Remove sockets and connections ( after caching )
	virtual void RemoveConnections();

	//! Get used variables and events
	virtual void GetUsedVariablesAndEvents( TDynArray<CName>& var, TDynArray<CName>& vecVar, TDynArray<CName>& events, TDynArray<CName>& intVar, TDynArray<CName>& intVecVar ) const;
};

BEGIN_ABSTRACT_CLASS_RTTI( CBehaviorGraphContainerNode );
	PARENT_CLASS( CBehaviorGraphNode );
	PROPERTY( m_mimicInputs );
	PROPERTY( m_vectorValueInputs );
END_CLASS_RTTI();
