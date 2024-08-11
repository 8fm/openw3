#pragma once

#include "behTreeNodeComposite.h"


class CBehTreeNodeSequenceInstance;
class CBehTreeNodePersistantSequenceInstance;
class CBehTreeNodeSequenceCheckAvailabilityInstance;
class CBehTreeNodeSequenceFowardAndBackInstance;
////////////////////////////////////////////////////////////////////////
// Sequence node
// Process each task one after another, until each task completes
// with success or until one of them fails.
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeSequenceDefinition : public IBehTreeNodeCompositeDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeSequenceDefinition, IBehTreeNodeCompositeDefinition, CBehTreeNodeSequenceInstance, Sequence );
protected:	
	IBehTreeNodeCompositeInstance* SpawnCompositeInstanceInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
public:
	CBehTreeNodeSequenceDefinition() {}
};

BEGIN_CLASS_RTTI(CBehTreeNodeSequenceDefinition);
	PARENT_CLASS(IBehTreeNodeCompositeDefinition);	
END_CLASS_RTTI();


////////////////////////////////////////////////////////////////////////
// Instance
class CBehTreeNodeSequenceInstance : public IBehTreeNodeCompositeInstance
{
	typedef IBehTreeNodeCompositeInstance Super;
public:
	typedef CBehTreeNodeSequenceDefinition Definition;

	CBehTreeNodeSequenceInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL )
		: Super( def, owner, context, parent )							{}

	Bool Activate() override;
	void OnSubgoalCompleted( eTaskOutcome outcome ) override;
};

////////////////////////////////////////////////////////////////////////
// Persistant sequence node
// Process each task one after another, until every task completes
// with success or until one of them fails. Remembers currently
// processed task upon deactivation/activation.
////////////////////////////////////////////////////////////////////////
class CBehTreeNodePersistantSequenceDefinition : public IBehTreeNodeCompositeDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodePersistantSequenceDefinition, IBehTreeNodeCompositeDefinition, CBehTreeNodePersistantSequenceInstance, PersistantSequence );
protected:	
	IBehTreeNodeCompositeInstance* SpawnCompositeInstanceInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};

BEGIN_CLASS_RTTI(CBehTreeNodePersistantSequenceDefinition);
	PARENT_CLASS(IBehTreeNodeCompositeDefinition);	
END_CLASS_RTTI();


////////////////////////////////////////////////////////////////////////
// Instance
class CBehTreeNodePersistantSequenceInstance : public IBehTreeNodeCompositeInstance
{
	typedef IBehTreeNodeCompositeInstance Super;
protected:
	Uint32									m_selectedChild;
public:
	typedef CBehTreeNodePersistantSequenceDefinition Definition;

	CBehTreeNodePersistantSequenceInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL )
		: Super( def, owner, context, parent )
		, m_selectedChild( 0 )											{}

	Bool Activate() override;
	void OnSubgoalCompleted( eTaskOutcome outcome ) override;

	// saving state support
	Bool IsSavingState() const override;
	void SaveState( IGameSaver* writer ) override;
	Bool LoadState( IGameLoader* reader ) override;
};



////////////////////////////////////////////////////////////////////////
// Sequence that checks availability node
// Process each task one after another, until each task completes
// with success or until one of them fails.
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeSequenceCheckAvailabilityDefinition : public CBehTreeNodeSequenceDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeSequenceCheckAvailabilityDefinition, CBehTreeNodeSequenceDefinition, CBehTreeNodeSequenceCheckAvailabilityInstance, SequenceCheckAvailabilty );
	friend class CBehTreeNodeSequenceCheckAvailAndUpdateInstance;
protected:	
	Bool					m_continueSequenceOnChildFailure;
	Bool					m_updateCheckIsAvailable;
	Float					m_updateCheckIsAvailFreq;

	IBehTreeNodeCompositeInstance* SpawnCompositeInstanceInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
public:
	CBehTreeNodeSequenceCheckAvailabilityDefinition()
		: m_continueSequenceOnChildFailure( true )	
		, m_updateCheckIsAvailable( false )
		, m_updateCheckIsAvailFreq( 0.5f )				{}
};

BEGIN_CLASS_RTTI(CBehTreeNodeSequenceCheckAvailabilityDefinition);
	PARENT_CLASS(CBehTreeNodeSequenceDefinition);
	PROPERTY_EDIT( m_continueSequenceOnChildFailure, TXT("Continue sequence when child completes with failure") );
	PROPERTY_EDIT( m_updateCheckIsAvailable, TXT(" if true this sequence will check is available on a frequency based on m_updateCheckIsAvailFreq ") );
	PROPERTY_EDIT( m_updateCheckIsAvailFreq, TXT(" [s] only aplicable if m_updateCheckIsAvailable is true ") );
END_CLASS_RTTI();

////////////////////////////////////////////////////////////////////////
// Instance
class CBehTreeNodeSequenceCheckAvailabilityInstance : public IBehTreeNodeCompositeInstance
{
	typedef IBehTreeNodeCompositeInstance Super;
protected:	
	Bool				m_continueSequenceOnChildFailure;
public:
	typedef CBehTreeNodeSequenceCheckAvailabilityDefinition Definition;

	CBehTreeNodeSequenceCheckAvailabilityInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL )
		: IBehTreeNodeCompositeInstance( def, owner, context, parent )
		, m_continueSequenceOnChildFailure( def.m_continueSequenceOnChildFailure )	{}

	Bool Activate() override;
	void OnSubgoalCompleted( eTaskOutcome outcome ) override;
};


////////////////////////////////////////////////////////////////////////
// Instance
class CBehTreeNodeSequenceCheckAvailAndUpdateInstance : public CBehTreeNodeSequenceCheckAvailabilityInstance
{
	typedef CBehTreeNodeSequenceCheckAvailabilityInstance Super;
private:
	Float m_nextTest;
	Float m_updateCheckIsAvailFreq;
public:

	CBehTreeNodeSequenceCheckAvailAndUpdateInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL )
		: CBehTreeNodeSequenceCheckAvailabilityInstance( def, owner, context, parent )
		, m_nextTest( 0.0f )
		, m_updateCheckIsAvailFreq( def.m_updateCheckIsAvailFreq ) 	{}
	Bool Activate() override;
	void Update() override;
};


////////////////////////////////////////////////////////////////////////
// 
class CBehTreeNodeSequenceFowardAndBackDefinition : public CBehTreeNodeSequenceDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeSequenceFowardAndBackDefinition, CBehTreeNodeSequenceDefinition, CBehTreeNodeSequenceFowardAndBackInstance, AimedSequence );
protected:
	Float				m_checkFrequency;
	Bool				m_forwardChildrenFailure;

	IBehTreeNodeCompositeInstance* SpawnCompositeInstanceInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;

public:
	CBehTreeNodeSequenceFowardAndBackDefinition()
		: m_checkFrequency( 2.f )
		, m_forwardChildrenFailure( true )								{}
};

BEGIN_CLASS_RTTI(CBehTreeNodeSequenceFowardAndBackDefinition);
	PARENT_CLASS(CBehTreeNodeSequenceDefinition);
	PROPERTY_EDIT( m_checkFrequency, TXT( "Test frequency" ) );
END_CLASS_RTTI();

////////////////////////////////////////////////////////////////////////
// Instance
class CBehTreeNodeSequenceFowardAndBackInstance : public CBehTreeNodeSequenceInstance
{
	typedef CBehTreeNodeSequenceInstance Super;
protected:
	Float					m_checkFrequency;
	Float					m_nextCheck;
	Uint32					m_sequencePoint;
	Bool					m_forwardChildrenFailure;
public:
	typedef CBehTreeNodeSequenceFowardAndBackDefinition Definition;

	CBehTreeNodeSequenceFowardAndBackInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )
		, m_checkFrequency( def.m_checkFrequency )
		, m_forwardChildrenFailure( def.m_forwardChildrenFailure )	{}

	Bool Activate() override;
	void Update() override;

	Bool OnEvent( CBehTreeEvent& e ) override;
	void OnSubgoalCompleted( eTaskOutcome outcome ) override;

	void MarkDirty() override;
	void MarkParentSelectorDirty() override;

};
