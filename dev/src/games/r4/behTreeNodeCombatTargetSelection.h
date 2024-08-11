#pragma once

#include "behTreeNodeCombatTargetSelectionBase.h"
#include "../../common/game/behTreeVarsEnums.h"

////////////////////////////////////////////////////////////////////////

enum ECombatTargetSelectionSkipTarget
{
	CTSST_SKIP_ALWAYS,
	CTSST_SKIP_IF_THERE_ARE_OTHER_TARGETS,
	CTSST_DONT_SKIP,
};

BEGIN_ENUM_RTTI( ECombatTargetSelectionSkipTarget );
	ENUM_OPTION( CTSST_SKIP_ALWAYS	);
	ENUM_OPTION( CTSST_SKIP_IF_THERE_ARE_OTHER_TARGETS	);
	ENUM_OPTION( CTSST_DONT_SKIP );
END_ENUM_RTTI();

class CBehTreeValECombatTargetSelectionSkipTarget : public TBehTreeValEnum< ECombatTargetSelectionSkipTarget, CTSST_SKIP_ALWAYS >
{
	DECLARE_RTTI_STRUCT( CBehTreeValECombatTargetSelectionSkipTarget );

private:
	typedef TBehTreeValEnum< ECombatTargetSelectionSkipTarget, CTSST_SKIP_ALWAYS > TBaseClass;

public:
	CBehTreeValECombatTargetSelectionSkipTarget( ECombatTargetSelectionSkipTarget e = CTSST_SKIP_ALWAYS )
		: TBaseClass( e )
	{
	}

	CBehTreeValECombatTargetSelectionSkipTarget( CName varName, ECombatTargetSelectionSkipTarget e = CTSST_SKIP_ALWAYS )
		: TBaseClass( e )		
	{
		m_varName = varName;
	}
};

BEGIN_NODEFAULT_CLASS_RTTI( CBehTreeValECombatTargetSelectionSkipTarget );
	PROPERTY_EDIT( m_varName, TXT("Variable") );
	PROPERTY_EDIT( m_value, TXT("Skip vehicle type") );
END_CLASS_RTTI();

////////////////////////////////////////////////////////////////////////
// Definition
////////////////////////////////////////////////////////////////////////

class CBehTreeNodeCombatTargetSelectionInstance;
class CCombatDataComponent;

class CBehTreeNodeCombatTargetSelectionDefinition : public IBehTreeNodeCombatTargetSelectionBaseDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeCombatTargetSelectionDefinition, IBehTreeNodeCombatTargetSelectionBaseDefinition, CBehTreeNodeCombatTargetSelectionInstance, CombatTargetSelection );
protected:

	CBehTreeValBool		m_targetOnlyPlayer;
	CBehTreeValFloat	m_hostileActorWeight;
	CBehTreeValFloat	m_currentTargetWeight;
	CBehTreeValInt		m_rememberedHits;
	CBehTreeValFloat	m_hitterWeight;
	CBehTreeValFloat	m_maxWeightedDistance;
	CBehTreeValFloat	m_distanceWeight;
	CBehTreeValInt		m_playerWeightProbability;
	CBehTreeValFloat	m_playerWeight;
	CBehTreeValFloat	m_monsterWeight;
	CBehTreeValECombatTargetSelectionSkipTarget		m_skipVehicle;
	CBehTreeValInt		m_skipVehicleProbability;
	CBehTreeValECombatTargetSelectionSkipTarget		m_skipUnreachable;
	CBehTreeValInt		m_skipUnreachableProbability;
	CBehTreeValECombatTargetSelectionSkipTarget		m_skipNotThreatening;
	CBehTreeValInt		m_skipNotThreateningProbability;

public:
	CBehTreeNodeCombatTargetSelectionDefinition()
		: m_targetOnlyPlayer( CNAME( targetOnlyPlayer ), false )
		, m_hostileActorWeight( CNAME( hostileActorWeight ), 10.0f )
		, m_currentTargetWeight( CNAME( currentTargetWeight ), 50.0f )
		, m_rememberedHits( CNAME( rememberedHits ), 2 )
		, m_hitterWeight( CNAME( hitterWeight ), 100.0f )
		, m_maxWeightedDistance( CNAME( maxWeightedDistance), 20.0f )
		, m_distanceWeight( CNAME( distanceWeight ), 1000.0f )
		, m_playerWeightProbability( CNAME( playerWeightProbability ), 100 )
		, m_playerWeight( CNAME( playerWeight ), 20.0f )
		, m_monsterWeight( CNAME( monsterWeight ), 0.0f )
		, m_skipVehicle( CNAME( skipVehicle), CTSST_SKIP_ALWAYS )
		, m_skipVehicleProbability( CNAME( skipVehicleProbability ), 100 )
		, m_skipUnreachable( CNAME( skipUnreachable ), CTSST_SKIP_ALWAYS )
		, m_skipUnreachableProbability( CNAME( skipUnreachableProbability ), 100 )
		, m_skipNotThreatening( CNAME( skipNotThreatening ), CTSST_SKIP_ALWAYS )
		, m_skipNotThreateningProbability( CNAME( skipNotThreateningProbability ), 100 )
	{
	}
	IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeCombatTargetSelectionDefinition );
	PARENT_CLASS( IBehTreeNodeCombatTargetSelectionBaseDefinition );
	PROPERTY_EDIT( m_targetOnlyPlayer,				TXT( "Choose only player as a target" ) );
	PROPERTY_EDIT( m_hostileActorWeight,			TXT( "Priority weight of hostile actor" ) );
	PROPERTY_EDIT( m_currentTargetWeight,			TXT( "Priority weight of current target" ) );
	PROPERTY_EDIT( m_rememberedHits,				TXT( "Number of hits (not hitters!) that actor remembers" ) );
	PROPERTY_EDIT( m_hitterWeight,					TXT( "Prioritty weight of actor that hit us" ) );
	PROPERTY_EDIT( m_maxWeightedDistance,			TXT( "Max distance that will affect distance weight" ) );
	PROPERTY_EDIT( m_distanceWeight,				TXT( "Priority weight for distance [ (0 to maxTargetDistance) * distanceWeight ]" ) );
	PROPERTY_EDIT( m_playerWeightProbability,		TXT( "Probability [0-100] that player will get additional priority" ) );
	PROPERTY_EDIT( m_playerWeight,					TXT( "Priority weight of player (with probability set in playerWeightProbability)" ) );
	PROPERTY_EDIT( m_monsterWeight,					TXT( "Priority weight of monster" ) );
	PROPERTY_EDIT( m_skipVehicle,					TXT( "Skip vehicles when selecting target" ) );
	PROPERTY_EDIT( m_skipVehicleProbability,		TXT( "Probability [0-100] that vehicle will be skipped when skipVehicle == CTSSV_SKIP_IF_THERE_ARE_OTHER_TARGETS" ) );
	PROPERTY_EDIT( m_skipUnreachable,				TXT( "Skip unreachable (by navigation) targets" ) );
	PROPERTY_EDIT( m_skipUnreachableProbability,	TXT( "Probability [0-100] that unreachable (by navigation) targets will be skipped when skipUnreachable == CTSSV_SKIP_IF_THERE_ARE_OTHER_TARGETS" ) );
	PROPERTY_EDIT( m_skipNotThreatening,			TXT( "Skip not threatening targets" ) );
	PROPERTY_EDIT( m_skipNotThreateningProbability,	TXT( "Probability [0-100] that non threatening targets will be skipped when skipNotThreatening == CTSSV_SKIP_IF_THERE_ARE_OTHER_TARGETS" ) );
END_CLASS_RTTI();

////////////////////////////////////////////////////////////////////////
// Instance
class CBehTreeNodeCombatTargetSelectionInstance : public IBehTreeNodeCombatTargetSelectionBaseInstance
{
	typedef IBehTreeNodeCombatTargetSelectionBaseInstance Super;
protected:

	Float					m_testMaxFrequency;
	Float					m_nextTestDelay;
	THandle< CActor >		m_nextTarget;
	THandle< CActor >		m_forceTarget;

	Bool					m_targetOnlyPlayer;
	Float					m_hostileActorWeight;
	Float					m_currentTargetWeight;
	Float					m_hitterWeight;
	Float					m_maxWeightedDistance;
	Float					m_distanceWeight;
	Int32					m_playerWeightProbability;
	Float					m_playerWeight;
	Float					m_monsterWeight;
	ECombatTargetSelectionSkipTarget	m_skipVehicle;
	Int32					m_skipVehicleProbability;
	ECombatTargetSelectionSkipTarget	m_skipUnreachable;
	Int32					m_skipUnreachableProbability;
	ECombatTargetSelectionSkipTarget	m_skipNotThreatening;
	Int32					m_skipNotThreateningProbability;

	TDynArray< const CActor* >			m_hitters;
	Bool								m_wasHit;
	THashMap< const CActor*, Float >	m_unreachableTargets;

	static const Float UNREACHABILITY_DURATION;

public:
	typedef CBehTreeNodeCombatTargetSelectionDefinition Definition;

	CBehTreeNodeCombatTargetSelectionInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = nullptr );

	void Update() override;
	Bool Activate() override;

	Bool OnListenedEvent( CBehTreeEvent& e ) override;
	void OnDestruction() override;

	Bool IsAvailable() override;

private:

	CActor* FindTarget();
	Float EvaluatePotentialTarget( CActor* target );
	Bool CheckForVehicle( CActor* target ) const;
	void MarkAsUnreachable( CActor* actor );
	void UpdateUnreachableTargets();
	Bool CheckForUnreachable( CActor* target ) const;
	Bool CheckForNotThreatening( CActor* target ) const;
	void AddHitter( CActor* hitter );
	Float GetHitterWeight( CActor* hitter );
};