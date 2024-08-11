/**
* Copyright c 2015 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "behTreeDecorator.h"
#include "behTreeWorkData.h"
#include "aiSpawnTreeParameters.h"
#include "encounterCreaturePool.h"

class CBehTreeNodePartyWorkSynchronizerDecoratorInstance;

////////////////////////////////////////////////////////////////////////
// Definition
////////////////////////////////////////////////////////////////////////
class CBehTreeNodePartyWorkSynchronizerDecoratorDefinition : public IBehTreeNodeDecoratorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodePartyWorkSynchronizerDecoratorDefinition, IBehTreeNodeDecoratorDefinition, CBehTreeNodePartyWorkSynchronizerDecoratorInstance, SynchronizePartyWork );
protected:


	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
public:
	CBehTreeNodePartyWorkSynchronizerDecoratorDefinition(){}
};

BEGIN_CLASS_RTTI( CBehTreeNodePartyWorkSynchronizerDecoratorDefinition )
	PARENT_CLASS( IBehTreeNodeDecoratorDefinition )
END_CLASS_RTTI()

////////////////////////////////////////////////////////////////////////
// INSTANCE
////////////////////////////////////////////////////////////////////////
class CBehTreeNodePartyWorkSynchronizerDecoratorInstance : public IBehTreeNodeDecoratorInstance
{
	static const Float SYNCHRONIZATION_CHECK_TIME_LIMIT_MAX;
	static const Float SYNCHRONIZATION_CHECK_TIME_LIMIT_MIN;

	typedef IBehTreeNodeDecoratorInstance Super;
public:
	typedef CBehTreeNodePartyWorkSynchronizerDecoratorDefinition Definition;

	CBehTreeNodePartyWorkSynchronizerDecoratorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

private:
	THandle< CEncounter >									m_encounter;
	Bool													m_synchronizationEnabled;
	Bool													m_prevSynchronizationCheckResult;
	CEncounterCreaturePool::CPartiesManager::PartyMember*	m_cachedPartyMember;
	Float													m_nextSynchronizationTime;	

	Bool IfRestOfPartyIsWorking();	
	Bool IfRestOfPartyIsWorkingImpl();

public:
	void OnDestruction() override;
	Bool Activate() override;
	void Update() override;
	void Deactivate() override;
	Bool Interrupt() override;

	Bool OnEvent( CBehTreeEvent& e ) override;
	Bool OnListenedEvent( CBehTreeEvent& e ) override;
};