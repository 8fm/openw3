#pragma once

#include "behTreeDecorator.h"

class CBehTreeNodeActivationDelayDecoratorInstance;


////////////////////////////////////////////////////////////////////////
// Definition
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeActivationDelayDecoratorDefinition : public IBehTreeNodeDecoratorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeActivationDelayDecoratorDefinition, IBehTreeNodeDecoratorDefinition, CBehTreeNodeActivationDelayDecoratorInstance, ReactivationDelayDecorator );
protected:
	CBehTreeValFloat				m_delayOnSuccess;
	CBehTreeValFloat				m_delayOnFailure;
	CBehTreeValFloat				m_delayOnInterruption;

	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
public:
	CBehTreeNodeActivationDelayDecoratorDefinition()
		: m_delayOnSuccess( -1.f )
		, m_delayOnFailure( -1.f )				
		, m_delayOnInterruption( -1.f )
	{}
};

BEGIN_CLASS_RTTI( CBehTreeNodeActivationDelayDecoratorDefinition );
	PARENT_CLASS( IBehTreeNodeDecoratorDefinition );
	PROPERTY_EDIT( m_delayOnSuccess, TXT("Activation delay after successful completion (seconds)") );
	PROPERTY_EDIT( m_delayOnFailure, TXT("Activation delay after failed completion (seconds)") );
	PROPERTY_EDIT( m_delayOnInterruption, TXT("Delay on unexpected termination (seconds)") );
END_CLASS_RTTI();

////////////////////////////////////////////////////////////////////////
// Instance
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeActivationDelayDecoratorInstance : public IBehTreeNodeDecoratorInstance
{
	typedef IBehTreeNodeDecoratorInstance Super;
protected:
	Float							m_activationTimeout;
	Float							m_delayOnSuccess;
	Float							m_delayOnFailure;
	Float							m_delayOnInterruption;
public:
	typedef CBehTreeNodeActivationDelayDecoratorDefinition Definition;

	CBehTreeNodeActivationDelayDecoratorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

	void OnSubgoalCompleted( eTaskOutcome outcome ) override;
	Bool Activate() override;
	Bool Interrupt() override;
	void Deactivate() override;

	Int32 Evaluate() override;
	Bool IsAvailable() override;
};


////////////////////////////////////////////////////////////////////////
// Definition
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeDelayActivationDecoratorInstance;
class CBehTreeNodeDelayActivationDecoratorDefinition : public IBehTreeNodeDecoratorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeDelayActivationDecoratorDefinition, IBehTreeNodeDecoratorDefinition, CBehTreeNodeDelayActivationDecoratorInstance, DelayActivationDecorator );
protected:
	CBehTreeValFloat				m_delay;
	Float							m_activationWindow;
	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
public:
	CBehTreeNodeDelayActivationDecoratorDefinition()
		: m_delay( 5.0f )
		, m_activationWindow( 2.f )
	{}
};

BEGIN_CLASS_RTTI( CBehTreeNodeDelayActivationDecoratorDefinition )
	PARENT_CLASS( IBehTreeNodeDecoratorDefinition )
	PROPERTY_EDIT( m_delay, TXT("Delay before activation (seconds)") )
	PROPERTY_EDIT( m_activationWindow, TXT("Time window in which behavior is available. Dependent on decision node computation frequency. Put -1 for infinite activation window") )
END_CLASS_RTTI()

////////////////////////////////////////////////////////////////////////
// Instance
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeDelayActivationDecoratorInstance : public IBehTreeNodeDecoratorInstance
{
	typedef IBehTreeNodeDecoratorInstance Super;
protected:
	Float				m_delay;
	Float				m_deactivationDelay;
	Float				m_nextActivationTime;
	Float				m_deactivationTime;

	Bool Check();
	void StartTimer();
public:
	typedef CBehTreeNodeDelayActivationDecoratorDefinition Definition;

	CBehTreeNodeDelayActivationDecoratorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

	void Deactivate() override;

	Int32 Evaluate() override;
	Bool IsAvailable() override;
};