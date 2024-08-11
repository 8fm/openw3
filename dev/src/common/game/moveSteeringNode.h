/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */
#pragma once

class CMovingAgentComponent;
struct SMoveLocomotionGoal;
class IMoveSteeringCondition;
class IMovementCommandBuffer;
class CMoveSteeringBehavior;
class ISteeringBehaviorListener;

///////////////////////////////////////////////////////////////////////////////

class IMoveSteeringNode : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IMoveSteeringNode, CObject );

protected:
#ifndef NO_EDITOR_STEERING_SUPPORT
	String					m_comment;
	Int32					m_graphPosX;			//!< Editor position X
	Int32					m_graphPosY;			//!< Editor position Y
#endif
	Bool					m_enabled;				//!< Is node enabled

public:
	IMoveSteeringNode();
	virtual ~IMoveSteeringNode() {}

	//! Called in order to build a runtime data layout
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );

	//! Called in order to initialize data in a runtime data buffer
	virtual void OnGraphActivation( CMovingAgentComponent& owner, InstanceBuffer& data );

	//! Called in order to deinitialize data in a runtime data buffer
	virtual void OnGraphDeactivation( CMovingAgentComponent& owner, InstanceBuffer& data );

	//! Called when given branch becomes deactivated
	virtual void OnBranchDeactivation( CMovingAgentComponent& owner, InstanceBuffer& data ) const;

	//! Called in order to initialize data in a runtime data buffer
	virtual void OnInitData( CMovingAgentComponent& owner, InstanceBuffer& data );

	//! Called in order to deinitialize data in a runtime data buffer
	virtual void OnDeinitData( CMovingAgentComponent& owner, InstanceBuffer& data );

	void CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta, ISteeringBehaviorListener* listener ) const;

#ifndef NO_EDITOR_STEERING_SUPPORT
	//! Get graph position X
	RED_INLINE Int32  GetGraphPosX() const { return m_graphPosX; }

	//! Get graph position Y
	RED_INLINE Int32  GetGraphPosY() const { return m_graphPosY; }

	//! Set graph position
	RED_INLINE void SetGraphPosition( Uint32 x, Uint32 y )
	{
		m_graphPosX = x;
		m_graphPosY = y;
	}

	//! Recursively offset nodes' position
	virtual void OffsetNodesPosition( Int32 offsetX, Int32 offsetY );
#endif

	//! Collects all nodes from the subtree
	virtual void CollectNodes( TDynArray< IMoveSteeringNode* >& nodes ) const;

	//! Get parent node
	RED_INLINE const IMoveSteeringNode* GetParentNode() const
	{
		// The parent of a root is a resource - in that case a NULL will be returned
		return Cast< IMoveSteeringNode >( GetParent() );
	}

	//! Get parent node
	RED_INLINE IMoveSteeringNode* GetParentNode()
	{
		// The parent of a root is a resource - in that case a NULL will be returned
		return Cast< IMoveSteeringNode >( GetParent() );
	}

	//! Is node enabled
	RED_INLINE Bool IsEnabled()  const { return m_enabled; }

#ifndef NO_EDITOR_STEERING_SUPPORT
	//! Returns the node's comment
	RED_INLINE const String& GetComment() const { return m_comment; }
#endif		// NO_EDITOR_STEERING_SUPPORT

	//! Returns the node's caption
	virtual String GetNodeCaption() const;

	//! Returns the name of the node's class
	virtual String GetNodeName() const;

	//! Correct children order to be coherent with positions, returns true if modification occurs
	virtual Bool CorrectChildrenOrder();

	//! Debug data visualization
	virtual void GenerateDebugFragments( const CMovingAgentComponent& agent, CRenderFrame* frame ) const;

protected:
	// Called in order to perform actual steering calculations
	virtual void OnCalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta, ISteeringBehaviorListener* listener ) const;

};
BEGIN_ABSTRACT_CLASS_RTTI( IMoveSteeringNode )
	PARENT_CLASS( CObject )
#ifndef NO_EDITOR_STEERING_SUPPORT
	PROPERTY_EDIT_NOT_COOKED( m_comment, TXT("Comment") )
	PROPERTY_NOT_COOKED( m_graphPosX );
	PROPERTY_NOT_COOKED( m_graphPosY );
#endif		// NO_EDITOR_STEERING_SUPPORT
	PROPERTY_EDIT( m_enabled, TXT( "Node enabled" ) );
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////

class IMoveSNComposite : public IMoveSteeringNode
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IMoveSNComposite, IMoveSteeringNode );

protected:
	TDynArray< IMoveSteeringNode* >	m_children;

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;

	virtual void OnInitData( CMovingAgentComponent& owner, InstanceBuffer& data ) override;

	virtual void OnDeinitData( CMovingAgentComponent& owner, InstanceBuffer& data ) override;

	virtual void OnGraphActivation( CMovingAgentComponent& owner, InstanceBuffer& data ) override;

	virtual void OnGraphDeactivation( CMovingAgentComponent& owner, InstanceBuffer& data ) override;

	virtual void OnBranchDeactivation( CMovingAgentComponent& owner, InstanceBuffer& data ) const override;

	void AddChild( IMoveSteeringNode* child );

	void RemoveChild( IMoveSteeringNode* child );

	virtual Bool CanAddChildren() const { return true; }

	RED_INLINE Uint32 GetChildCount() const { return m_children.Size(); }

	RED_INLINE IMoveSteeringNode* GetChild( Uint32 idx ) { return m_children[ idx ]; }

#ifndef NO_EDITOR_STEERING_SUPPORT
	virtual void OffsetNodesPosition( Int32 offsetX, Int32 offsetY ) override;
	virtual Bool CorrectChildrenOrder() override;
#endif

	virtual void CollectNodes( TDynArray< IMoveSteeringNode* >& nodes ) const override;

	Int32 GetChildIdx( IMoveSteeringNode* node ) const;

	virtual void GenerateDebugFragments( const CMovingAgentComponent& agent, CRenderFrame* frame ) const override;

protected:
	virtual void OnChildrenChanged() {}
};
BEGIN_ABSTRACT_CLASS_RTTI( IMoveSNComposite )
	PARENT_CLASS( IMoveSteeringNode )
	PROPERTY( m_children )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////

class CMoveSNComposite : public IMoveSNComposite
{
	DECLARE_ENGINE_CLASS( CMoveSNComposite, IMoveSNComposite, 0 );

private:
	String								m_groupName;

public:
	CMoveSNComposite();

	virtual void OnCalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta, ISteeringBehaviorListener* listener ) const;

	virtual String GetNodeCaption() const;

	virtual String GetNodeName() const;

};
BEGIN_CLASS_RTTI( CMoveSNComposite )
	PARENT_CLASS( IMoveSNComposite )
	PROPERTY_EDIT( m_groupName, TXT( "Name of the nodes group" ) )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////

class CMoveSNCondition : public IMoveSNComposite
{
	DECLARE_ENGINE_CLASS( CMoveSNCondition, IMoveSNComposite, 0 );

private:
	static const Uint8 CHILD_UNACTIVE = 0xff;
	
	IMoveSteeringCondition*				m_condition;
	Bool								m_invertCondition;

	TInstanceVar< Uint8 >				i_childActive;

public:
	CMoveSNCondition()
		: m_condition( NULL )
		, m_invertCondition( false )										{}

	virtual Bool CanAddChildren() const;

	void SetCondition( IMoveSteeringCondition* condition );

	void OnBranchDeactivation( CMovingAgentComponent& owner, InstanceBuffer& data ) const override;

	RED_INLINE IMoveSteeringCondition* GetCondition()						{ return m_condition; }

	virtual void OnCalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta, ISteeringBehaviorListener* listener ) const;

	virtual String GetNodeCaption() const;

	virtual String GetNodeName() const;

	virtual void OnGraphActivation( CMovingAgentComponent& owner, InstanceBuffer& data ) override;

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );

	virtual void OnInitData( CMovingAgentComponent& owner, InstanceBuffer& data );

	virtual void OnDeinitData( CMovingAgentComponent& owner, InstanceBuffer& data );

	virtual void GenerateDebugFragments( const CMovingAgentComponent& agent, CRenderFrame* frame ) const override;

};
BEGIN_CLASS_RTTI( CMoveSNCondition )
	PARENT_CLASS( IMoveSNComposite )
	PROPERTY_INLINED( m_condition, TXT( "Condition" ) )
	PROPERTY_EDIT( m_invertCondition, TXT("Invert condition output") )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////

class CMoveSNTask : public IMoveSteeringNode
{
	DECLARE_ENGINE_CLASS( CMoveSNTask, IMoveSteeringNode, 0 );

private:
	IMoveSteeringTask*				m_task;

public:
	CMoveSNTask();

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;

	virtual void OnInitData( CMovingAgentComponent& owner, InstanceBuffer& data ) override;

	virtual void OnDeinitData( CMovingAgentComponent& owner, InstanceBuffer& data ) override;

	virtual void OnGraphActivation( CMovingAgentComponent& owner, InstanceBuffer& data ) override;

	virtual void OnGraphDeactivation( CMovingAgentComponent& owner, InstanceBuffer& data ) override;

	virtual void OnBranchDeactivation( CMovingAgentComponent& owner, InstanceBuffer& data ) const override;

	virtual void OnCalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta, ISteeringBehaviorListener* listener ) const override;

	virtual String GetNodeCaption() const override;

	virtual String GetNodeName() const override;

	RED_INLINE IMoveSteeringTask* GetTask()									{ return m_task; }

	void SetTask( IMoveSteeringTask* task );

	virtual void GenerateDebugFragments( const CMovingAgentComponent& agent, CRenderFrame* frame ) const override;
};
BEGIN_CLASS_RTTI( CMoveSNTask )
	PARENT_CLASS( IMoveSteeringNode )
	PROPERTY_INLINED_RO( m_task, TXT( "Task" ) )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////

