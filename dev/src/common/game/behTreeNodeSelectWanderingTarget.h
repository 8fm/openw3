#pragma once

#include "aiQueryWalkableSpotInArea.h"
#include "behTreeCustomMoveData.h"
#include "behTreeDecorator.h"
#include "behTreeGuardAreaData.h"
#include "behTreeNodeAtomicAction.h"
#include "wanderPointComponent.h"

class CBehTreeNodeSelectWanderingTargetDecoratorInstance;
class CBehTreeNodeWanderingTaggedTargetDecoratorInstance;
class CBehTreeNodeRandomWanderingTargetInstance;
class CBehTreeNodeHistoryWanderingTargetInstance;
class CBehTreeNodeDynamicWanderingTargetInstance;

class CWanderData
{
public:
	const static Uint32 MAX_HISTORY_SIZE = 4;

protected:
	THandle< CWanderPointComponent >	m_history[ MAX_HISTORY_SIZE ];
	Uint16								m_lastVisitIndex;
	Uint16								m_historySize;

public:
	CWanderData();
	Bool AddHistoryEntry( CWanderPointComponent* wanderPoint );
	void ClearHistory();
	static Uint32 GetAbsoluteHistorySize()															{ return MAX_HISTORY_SIZE; }
	Uint32 GetCollectedHistorySize() const															{ return m_historySize; }
	Int32 GetHistoryPosition( CWanderPointComponent* wanderPoint ) const;
	CWanderPointComponent* GetLastVisitedPoint() const;

	CWanderPointComponent* GetHistory( Uint32 historyIndex ) const									{ return m_history[ (m_lastVisitIndex + MAX_HISTORY_SIZE - historyIndex) % MAX_HISTORY_SIZE ].Get(); }
};

////////////////////////////////////////////////////////////////////////////////////////////////////

class CBehTreeNodeSelectWanderingTargetDecoratorDefinition : public IBehTreeNodeDecoratorDefinition
{
	DECLARE_BEHTREE_ABSTRACT_NODE( CBehTreeNodeSelectWanderingTargetDecoratorDefinition, IBehTreeNodeDecoratorDefinition, CBehTreeNodeSelectWanderingTargetDecoratorInstance, SelectWanderingTargetDecorator );	

public:
	CBehTreeNodeSelectWanderingTargetDecoratorDefinition()
	{}

	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};


BEGIN_ABSTRACT_CLASS_RTTI( CBehTreeNodeSelectWanderingTargetDecoratorDefinition );
PARENT_CLASS( IBehTreeNodeDecoratorDefinition );
END_CLASS_RTTI();

////////////////////////////////////////////////////////////////////////////////////////////////////

class CBehTreeNodeSelectWanderingTargetDecoratorInstance : public IBehTreeNodeDecoratorInstance
{
	typedef IBehTreeNodeDecoratorInstance Super;
public:
	typedef CBehTreeNodeSelectWanderingTargetDecoratorDefinition Definition;

	CBehTreeNodeSelectWanderingTargetDecoratorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: IBehTreeNodeDecoratorInstance( def, owner, context, parent )
	{}

	virtual Bool SelectNextPoint();

	Bool OnEvent( CBehTreeEvent& e ) override;

	static CName SelectNextPointRequestName();
};

////////////////////////////////////////////////////////////////////////////////////////////////////

class CBehTreeNodeWanderingTaggedTargetDecoratorDefinition : public CBehTreeNodeSelectWanderingTargetDecoratorDefinition
{
	DECLARE_BEHTREE_ABSTRACT_NODE( CBehTreeNodeWanderingTaggedTargetDecoratorDefinition, CBehTreeNodeSelectWanderingTargetDecoratorDefinition, CBehTreeNodeWanderingTaggedTargetDecoratorInstance, WanderingTaggedTargetDecorator );	

protected:
	CBehTreeValCName m_pointsGroupTag;

public:
	CBehTreeNodeWanderingTaggedTargetDecoratorDefinition()	{}

	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};


BEGIN_ABSTRACT_CLASS_RTTI( CBehTreeNodeWanderingTaggedTargetDecoratorDefinition );
PARENT_CLASS( CBehTreeNodeSelectWanderingTargetDecoratorDefinition );
PROPERTY_EDIT( m_pointsGroupTag, TXT("Wander points group tag") );
END_CLASS_RTTI();

////////////////////////////////////////////////////////////////////////////////////////////////////

class CBehTreeNodeWanderingTaggedTargetDecoratorInstance : public CBehTreeNodeSelectWanderingTargetDecoratorInstance
{
	typedef CBehTreeNodeSelectWanderingTargetDecoratorInstance Super;
protected:
	CName								m_pointsGroupTag;
	THandle< CWanderPointComponent >	m_currentTarget;

public:
	typedef CBehTreeNodeWanderingTaggedTargetDecoratorDefinition Definition;

	CBehTreeNodeWanderingTaggedTargetDecoratorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

	Bool Activate() override;
	void Deactivate() override;
	void OnSubgoalCompleted( eTaskOutcome outcome ) override;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

class CBehTreeNodeRandomWanderingTargetDefinition : public CBehTreeNodeWanderingTaggedTargetDecoratorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeRandomWanderingTargetDefinition, CBehTreeNodeWanderingTaggedTargetDecoratorDefinition, CBehTreeNodeRandomWanderingTargetInstance, RandomWanderingTarget );	

public:
	CBehTreeNodeRandomWanderingTargetDefinition() {}

	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};


BEGIN_CLASS_RTTI( CBehTreeNodeRandomWanderingTargetDefinition );
	PARENT_CLASS( CBehTreeNodeWanderingTaggedTargetDecoratorDefinition );
END_CLASS_RTTI();

////////////////////////////////////////////////////////////////////////////////////////////////////

class CBehTreeNodeRandomWanderingTargetInstance : public CBehTreeNodeWanderingTaggedTargetDecoratorInstance
{
public:
	typedef CBehTreeNodeRandomWanderingTargetDefinition Definition;

	CBehTreeNodeRandomWanderingTargetInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: CBehTreeNodeWanderingTaggedTargetDecoratorInstance( def, owner, context, parent )
	{}

	Bool SelectNextPoint() override;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

class CBehTreeNodeHistoryWanderingTargetDefinition : public CBehTreeNodeWanderingTaggedTargetDecoratorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeHistoryWanderingTargetDefinition, CBehTreeNodeWanderingTaggedTargetDecoratorDefinition, CBehTreeNodeHistoryWanderingTargetInstance, HistoryWanderingTarget );	

public:
	CBehTreeNodeHistoryWanderingTargetDefinition()													{}

	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};


BEGIN_CLASS_RTTI( CBehTreeNodeHistoryWanderingTargetDefinition )
	PARENT_CLASS( CBehTreeNodeWanderingTaggedTargetDecoratorDefinition )
END_CLASS_RTTI()

////////////////////////////////////////////////////////////////////////////////////////////////////

class CBehTreeNodeHistoryWanderingTargetInstance : public CBehTreeNodeWanderingTaggedTargetDecoratorInstance
{
	typedef CBehTreeNodeWanderingTaggedTargetDecoratorInstance Super;
protected:
	CWanderData m_wanderData;

public:
	typedef CBehTreeNodeHistoryWanderingTargetDefinition Definition;

	CBehTreeNodeHistoryWanderingTargetInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: CBehTreeNodeWanderingTaggedTargetDecoratorInstance( def, owner, context, parent )			{}

	Bool SelectNextPoint() override;
	void OnSubgoalCompleted( eTaskOutcome outcome ) override;

protected:
	Int32 GetNodeScore( CWanderPointComponent* wanderComponent, const Vector& currentPos, Bool hasDesiredAngle, const Vector& desiredDir );
};

////////////////////////////////////////////////////////////////////////////////////////////////////

class CBehTreeNodeDynamicWanderingTargetDefinition : public CBehTreeNodeSelectWanderingTargetDecoratorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeDynamicWanderingTargetDefinition, CBehTreeNodeSelectWanderingTargetDecoratorDefinition, CBehTreeNodeDynamicWanderingTargetInstance, DynamicWanderingTarget );	

protected:
	CName					m_dynamicWanderAreaName_var;
	CBehTreeValFloat		m_minimalWanderDistance;
	CBehTreeValBool			m_useGuardArea;
public:
	CBehTreeNodeDynamicWanderingTargetDefinition()
		: m_dynamicWanderAreaName_var( CName::NONE )
		, m_minimalWanderDistance( 0 )
	{}

	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};


BEGIN_CLASS_RTTI( CBehTreeNodeDynamicWanderingTargetDefinition );
	PARENT_CLASS( CBehTreeNodeSelectWanderingTargetDecoratorDefinition );
	PROPERTY_EDIT( m_dynamicWanderAreaName_var, TXT("Wander area variable name") );
	PROPERTY_EDIT( m_minimalWanderDistance, TXT("Minimal distance to next random point") );
	PROPERTY_EDIT( m_useGuardArea, TXT("If on, current guard area will override default wander area") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////////////////////////////////

class CBehTreeNodeDynamicWanderingTargetInstance : public CBehTreeNodeSelectWanderingTargetDecoratorInstance
{
	typedef CBehTreeNodeSelectWanderingTargetDecoratorInstance Super;
protected:
	CBehTreeCustomMoveDataPtr					m_customMoveData;
	THandle< CAreaComponent >					m_dynamicWanderArea;
	CBehTreeGuardAreaDataPtr					m_guardAreaDataPtr;
	CQueryReacheableSpotInAreaRequest::Ptr		m_findSpotQuery;
	Float										m_queryTimeout;
	Float										m_minimalWanderDistanceSq;

	void LazyCreateQuery();
	Bool FindSpot();

	CAreaComponent* GetWanderingArea() const;
	

public:
	typedef CBehTreeNodeDynamicWanderingTargetDefinition Definition;

	CBehTreeNodeDynamicWanderingTargetInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );
	void OnSpawn( const IBehTreeNodeDefinition& def, CBehTreeSpawnContext& context ) override;

	Bool IsAvailable() override;
	Int32 Evaluate() override;

	Bool Activate() override;
	void OnSubgoalCompleted( eTaskOutcome outcome ) override;
	Bool SelectNextPoint();
	void Update() override;
};

