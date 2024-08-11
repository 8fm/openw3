#pragma once

#include "behTreeDecorator.h"

////////////////////////////////////////////////////////////////////////
// Loop decorator
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeLoopDecoratorInstance;
class CBehTreeNodeLoopWithTimelimitDecoratorInstance;

enum EBTLoopMode
{
	BTLM_Continue,
	BTLM_ReportCompleted,
	BTLM_ReportFailed
};

BEGIN_ENUM_RTTI( EBTLoopMode )
	ENUM_OPTION( BTLM_Continue );
	ENUM_OPTION( BTLM_ReportCompleted );
	ENUM_OPTION( BTLM_ReportFailed );
END_ENUM_RTTI()


class CBehTreeNodeLoopDecoratorDefinition : public IBehTreeNodeDecoratorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeLoopDecoratorDefinition, IBehTreeNodeDecoratorDefinition, CBehTreeNodeLoopDecoratorInstance, Loop );
protected:
	EBTLoopMode				m_onCompleted;
	EBTLoopMode				m_onFailed;
	CBehTreeValInt			m_maxIterations;
	Float					m_reactivationDelay;

	static const Uint32		ITERATIONS_INFINITE = 0;

	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
public:
	CBehTreeNodeLoopDecoratorDefinition()
		: m_onCompleted( BTLM_Continue )
		, m_onFailed( BTLM_ReportFailed )
		, m_maxIterations( 0 )
		, m_reactivationDelay( -1.f )									{}
};

BEGIN_CLASS_RTTI(CBehTreeNodeLoopDecoratorDefinition);
	PARENT_CLASS(IBehTreeNodeDecoratorDefinition);
	PROPERTY_EDIT( m_onCompleted, TXT("On child completed") );
	PROPERTY_EDIT( m_onFailed, TXT("On child failed") );
	PROPERTY_EDIT( m_maxIterations, TXT("Maximum number of iterations, 0 = infinite") );
	PROPERTY_EDIT( m_reactivationDelay, TXT("Delay between goal reactivations, 0.0 = no delays") );
END_CLASS_RTTI();


////////////////////////////////////////////////////////////////////////
// Instance
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeLoopDecoratorInstance : public IBehTreeNodeDecoratorInstance
{
	typedef IBehTreeNodeDecoratorInstance Super;
protected:
	Uint32				m_iterationsCount;
	EBTLoopMode			m_onCompleted : 16;
	EBTLoopMode			m_onFailed : 16;
	Uint32				m_maxIterations;
	Float				m_reactivationDelay;
	Float				m_reactivationTime;
public:
	typedef CBehTreeNodeLoopDecoratorDefinition Definition;

	CBehTreeNodeLoopDecoratorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL )
		: IBehTreeNodeDecoratorInstance( def, owner, context, parent )
		, m_iterationsCount( 0 )
		, m_onCompleted( def.m_onCompleted )
		, m_onFailed( def.m_onFailed )
		, m_maxIterations( def.m_maxIterations.GetVal( context ) )
		, m_reactivationDelay( Max( def.m_reactivationDelay, 0.f ) )
		, m_reactivationTime( 0.f )										{}

	Bool Activate() override;
	void Update() override;

	void OnSubgoalCompleted( eTaskOutcome outcome ) override;

	IBehTreeNodeInstance* GetActiveChild() const override;
};

////////////////////////////////////////////////////////////////////////
// Loop decorator with timelimit
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeLoopWithTimelimitDecoratorDefinition : public CBehTreeNodeLoopDecoratorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeLoopWithTimelimitDecoratorDefinition, CBehTreeNodeLoopDecoratorDefinition, CBehTreeNodeLoopWithTimelimitDecoratorInstance, LoopForTime );
protected:
	Float					m_timeLimit;

	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
public:
	CBehTreeNodeLoopWithTimelimitDecoratorDefinition()
		: m_timeLimit( 3.f )											{}
};

BEGIN_CLASS_RTTI(CBehTreeNodeLoopWithTimelimitDecoratorDefinition);
	PARENT_CLASS(CBehTreeNodeLoopDecoratorDefinition);
	PROPERTY_EDIT( m_timeLimit, TXT("Time limit while loop is active") );
END_CLASS_RTTI();

////////////////////////////////////////////////////////////////////////
// Instance
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeLoopWithTimelimitDecoratorInstance : public CBehTreeNodeLoopDecoratorInstance
{
	typedef CBehTreeNodeLoopDecoratorInstance Super;
protected:
	Float					m_timeLimitDuration;
	Float					m_currentTimeLimit;
public:
	typedef CBehTreeNodeLoopWithTimelimitDecoratorDefinition Definition;

	CBehTreeNodeLoopWithTimelimitDecoratorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL )
		: Super( def, owner, context, parent )
		, m_timeLimitDuration( def.m_timeLimit )
		, m_currentTimeLimit( 0.f )										{}

	Bool Activate() override;
	void OnSubgoalCompleted( eTaskOutcome outcome ) override;
};