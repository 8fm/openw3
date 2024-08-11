#pragma once

#include "../../common/game/behTreeDecorator.h"
#include "../../common/engine/pathlibSearchData.h"
#include "ridingAiStorage.h"

////////////////////////////////////////////////////////////////////////
// CBehTreeDecoratorRiderHorseReachabilityDefinition
////////////////////////////////////////////////////////////////////////
class CBehTreeDecoratorRiderHorseReachabilityInstance;
class CBehTreeDecoratorRiderHorseReachabilityDefinition : public IBehTreeNodeDecoratorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeDecoratorRiderHorseReachabilityDefinition, IBehTreeNodeDecoratorDefinition, CBehTreeDecoratorRiderHorseReachabilityInstance, RiderHorseReachability );
protected:
	

public:
	CBehTreeDecoratorRiderHorseReachabilityDefinition();
	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeDecoratorRiderHorseReachabilityDefinition );
	PARENT_CLASS( IBehTreeNodeDecoratorDefinition );	
END_CLASS_RTTI();


////////////////////////////////////////////////////////////////////////
// CBehTreeDecoratorRiderHorseReachabilityInstance
// Takes care of executing mount and dismount node from messages it receives
class CBehTreeDecoratorRiderHorseReachabilityInstance : public IBehTreeNodeDecoratorInstance
{
	typedef IBehTreeNodeDecoratorInstance Super;
	typedef CBehTreeDecoratorRiderHorseReachabilityDefinition Definition;
	typedef PathLib::CReachabilityData CReachabilityData;

	friend class CBehTreeDecoratorRiderHorseReachabilityDefinition;

protected:
	CReachabilityData::Ptr				m_reachabilityDataPtr;
	CAIStorageRiderData::CStoragePtr	m_riderData;

protected:
	CActor* GetHorseActor() const;

public:
	CBehTreeDecoratorRiderHorseReachabilityInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL );
	Bool Activate() override;
	void Update() override;
	void OnSubgoalCompleted( eTaskOutcome outcome )override;
};


