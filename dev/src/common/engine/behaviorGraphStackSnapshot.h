/**
* Copyright © 2011 CD Projekt Red. All Rights Reserved.
*/

#pragma  once

struct SBehaviorSnapshotDataStateMachine
{
	DECLARE_RTTI_STRUCT( SBehaviorSnapshotDataStateMachine );

	SBehaviorSnapshotDataStateMachine()
		: m_stateMachineId( 0 )
		, m_currentStateId( 0 )
		, m_currentStateTime( 0.f )
	{

	}

	Uint32					m_stateMachineId;
	Uint32					m_currentStateId;
	Float					m_currentStateTime;
};

BEGIN_CLASS_RTTI( SBehaviorSnapshotDataStateMachine );
	PROPERTY_SAVED( m_stateMachineId );
	PROPERTY_SAVED( m_currentStateId );
	PROPERTY_SAVED( m_currentStateTime );
END_CLASS_RTTI();

class CBehaviorGraphInstanceSnapshot : public CObject
{	
	DECLARE_ENGINE_CLASS( CBehaviorGraphInstanceSnapshot, CObject, 0 );	
	friend class CBehaviorGraphInstance;
	friend class CBehaviorGraphStack;

private:
	CBehaviorGraphInstanceSnapshot();
	~CBehaviorGraphInstanceSnapshot();

public:
	void ReleaseAndDiscard();
	void OnSerialize( IFile& file ) override;

private:
	CName											m_instanceName;
	class InstanceBuffer*							m_instanceBuffer;
	TDynArray< Float >								m_floatVariables;
	TDynArray< Vector >								m_vectorVariables;
	TDynArray< SBehaviorSnapshotDataStateMachine >	m_stateMachineData;
};


BEGIN_CLASS_RTTI( CBehaviorGraphInstanceSnapshot );
	PARENT_CLASS( CObject );
	PROPERTY_SAVED( m_instanceName );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphStackSnapshot : public CObject
{
	DECLARE_ENGINE_CLASS( CBehaviorGraphStackSnapshot, CObject, 0 );
	friend class CBehaviorGraphStack;
private:
	TDynArray< CBehaviorGraphInstanceSnapshot* > m_instanceSnapshots;
};

BEGIN_CLASS_RTTI( CBehaviorGraphStackSnapshot );
	PARENT_CLASS( CObject );
	PROPERTY_SAVED( m_instanceSnapshots );
END_CLASS_RTTI();
