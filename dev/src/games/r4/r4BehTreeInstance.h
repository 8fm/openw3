#pragma once

#include "../../common/game/aiReachabilityQuery.h"
#include "../../common/game/behTreeInstance.h"

#include "combatDataComponent.h"

class CR4BehTreeInstance : public CBehTreeInstance
{
	DECLARE_ENGINE_CLASS( CR4BehTreeInstance, CBehTreeInstance, 0 );

protected:
	CCombatDataPtr								m_combatData;
	CCombatDataPtr								m_targetData;
	CAIReachabilityQuery::Ptr					m_reachabilityQuery;
	Float										m_toleranceDistanceRequiredByCombat;
	Bool										m_isInCombat;

public:
	CR4BehTreeInstance()
		: m_toleranceDistanceRequiredByCombat( 4.f )							{ }
	~CR4BehTreeInstance()														{ ASSERT( m_targetData.Get() == nullptr );}

	CCombatDataComponent* GetCombatData()
	{
		if ( m_combatData.Get() == nullptr )
		{
			m_combatData = CCombatDataPtr( GetActor() );
		}
		return m_combatData;
	}
	CCombatDataComponent* GetCombatTargetData() const							{ return m_targetData; }

	static CR4BehTreeInstance* Get( CBehTreeInstance* instance )				{ return static_cast< CR4BehTreeInstance* >( instance ); }

	void SetCombatTarget( const THandle< CActor >& node, Bool registerAsAttacker = true ) override;
	void ClearCombatTarget() override;
	void OnCombatTargetDestroyed() override;
	Bool IsCombatTargetReachable();
	void SetToleranceDistanceForCombat( Float d )								{ m_toleranceDistanceRequiredByCombat = d; }

	Bool IsInCombat() const override;
	void SetIsInCombat( Bool inCombat ) override;

	void DescribeTicketsInfo( TDynArray< String >& info ) override;
};

BEGIN_CLASS_RTTI( CR4BehTreeInstance );
	PARENT_CLASS( CBehTreeInstance );
END_CLASS_RTTI();
