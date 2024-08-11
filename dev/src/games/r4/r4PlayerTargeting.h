/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "r4PlayerTypes.h"
#include "../../common/game/targetingUtils.h"

//////////////////////////////////////////////////////////////////////////

struct STargetingInfo
{
	DECLARE_RTTI_STRUCT( STargetingInfo );

	THandle< CActor >			m_source;
	THandle< CGameplayEntity >	m_targetEntity;
	Bool						m_canBeTargetedCheck;
	Bool						m_coneCheck;
	Float						m_coneHalfAngleCos;
	Float						m_coneDist;
	Vector						m_coneHeadingVector; 
	Bool						m_distCheck;
	Bool						m_invisibleCheck;
	Bool						m_navMeshCheck; 
	Bool						m_inFrameCheck; 
	Float						m_frameScaleX; 
	Float						m_frameScaleY; 
	Bool						m_knockDownCheck; 
	Float						m_knockDownCheckDist; 
	Bool						m_rsHeadingCheck;
	Float						m_rsHeadingLimitCos;

	STargetingInfo();
};

BEGIN_CLASS_RTTI( STargetingInfo );
	PROPERTY( m_source );
	PROPERTY( m_targetEntity );
	PROPERTY( m_canBeTargetedCheck );
	PROPERTY( m_coneCheck );
	PROPERTY( m_coneHalfAngleCos );
	PROPERTY( m_coneDist );
	PROPERTY( m_coneHeadingVector );
	PROPERTY( m_distCheck );
	PROPERTY( m_invisibleCheck );
	PROPERTY( m_navMeshCheck );
	PROPERTY( m_inFrameCheck );
	PROPERTY( m_frameScaleX );
	PROPERTY( m_frameScaleY );
	PROPERTY( m_knockDownCheck );
	PROPERTY( m_knockDownCheckDist );
	PROPERTY( m_rsHeadingCheck );
	PROPERTY( m_rsHeadingLimitCos );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

struct SR4PlayerTargetingConsts
{
	DECLARE_RTTI_STRUCT( SR4PlayerTargetingConsts );

	Float				m_softLockDistance;
	Float				m_softLockFrameSize;

	SR4PlayerTargetingConsts();
};

BEGIN_CLASS_RTTI( SR4PlayerTargetingConsts );
	PROPERTY( m_softLockDistance );
	PROPERTY( m_softLockFrameSize );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

struct SR4PlayerTargetingPrecalcs
{
	DECLARE_RTTI_STRUCT( SR4PlayerTargetingPrecalcs );

	Vector	m_playerPosition;
	Float	m_playerHeading;
	Vector	m_playerHeadingVector;
	Float	m_playerRadius;
	Bool	m_playerIsInCombat;

	class CCameraDirector*	m_cameraDirector;
	Vector	m_cameraPosition;
	Vector	m_cameraDirection;
	Float	m_cameraHeading;
	Vector	m_cameraHeadingVector;

	SR4PlayerTargetingPrecalcs();
	Bool ObtainCameraDirector();
	void Calculate( CR4Player* player );
};

BEGIN_CLASS_RTTI( SR4PlayerTargetingPrecalcs );
	PROPERTY( m_playerPosition );
	PROPERTY( m_playerHeading );
	PROPERTY( m_playerHeadingVector );
	PROPERTY( m_playerRadius );
	PROPERTY( m_cameraPosition );
	PROPERTY( m_cameraDirection );
	PROPERTY( m_cameraHeading );
	PROPERTY( m_cameraHeadingVector );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

struct SR4PlayerTargetingIn
{
	DECLARE_RTTI_STRUCT( SR4PlayerTargetingIn );

	Bool				m_canFindTarget;
	Bool				m_playerHasBlockingBuffs;
	Bool				m_isHardLockedToTarget;
	Bool				m_isActorLockedToTarget;
	Bool				m_isCameraLockedToTarget;
	Bool				m_actionCheck;
	Bool				m_actionInput;
	Bool				m_isInCombatAction;
	Bool				m_isLAxisReleased;
	Bool				m_isLAxisReleasedAfterCounter;
	Bool				m_isLAxisReleasedAfterCounterNoCA;
	Bool				m_lastAxisInputIsMovement;
	Bool				m_isAiming;
	Bool				m_isSwimming;
	Bool				m_isDiving;
	Bool				m_isThreatened;
	Bool				m_isCombatMusicEnabled;
	Bool				m_isPcModeEnabled;
	Bool				m_shouldUsePcModeTargeting;
	Bool				m_isInParryOrCounter;
	EBufferActionType	m_bufferActionType;
	EOrientationTarget	m_orientationTarget;
	Float				m_coneDist;
	Float				m_findMoveTargetDist;
	Float				m_cachedRawPlayerHeading;
	Float				m_combatActionHeading;
	Vector				m_rawPlayerHeadingVector;
	Vector				m_lookAtDirection;
	THandle< CActor >	m_moveTarget;
	THandle< CActor >	m_aimingTarget;
	THandle< CActor >	m_displayTarget;
	TDynArray< THandle< CActor > >	m_finishableEnemies;
	TDynArray< THandle< CActor > >	m_hostileEnemies;
	STargetSelectionWeights			m_defaultSelectionWeights;

	SR4PlayerTargetingIn();
};

BEGIN_CLASS_RTTI( SR4PlayerTargetingIn );
	PROPERTY( m_canFindTarget );
	PROPERTY( m_playerHasBlockingBuffs );
	PROPERTY( m_isHardLockedToTarget );
	PROPERTY( m_isActorLockedToTarget );
	PROPERTY( m_isCameraLockedToTarget );
	PROPERTY( m_actionCheck );
	PROPERTY( m_actionInput );
	PROPERTY( m_isInCombatAction );
	PROPERTY( m_isLAxisReleased );
	PROPERTY( m_isLAxisReleasedAfterCounter );
	PROPERTY( m_isLAxisReleasedAfterCounterNoCA );
	PROPERTY( m_lastAxisInputIsMovement );
	PROPERTY( m_isAiming );
	PROPERTY( m_isSwimming );
	PROPERTY( m_isDiving );
	PROPERTY( m_isThreatened );
	PROPERTY( m_isCombatMusicEnabled );
	PROPERTY( m_isPcModeEnabled );
	PROPERTY( m_shouldUsePcModeTargeting );
	PROPERTY( m_isInParryOrCounter );	
	PROPERTY( m_bufferActionType );
	PROPERTY( m_orientationTarget );
	PROPERTY( m_coneDist );
	PROPERTY( m_findMoveTargetDist );
	PROPERTY( m_cachedRawPlayerHeading );
	PROPERTY( m_combatActionHeading );
	PROPERTY( m_rawPlayerHeadingVector );
	PROPERTY( m_lookAtDirection );
	PROPERTY( m_moveTarget );
	PROPERTY( m_aimingTarget );
	PROPERTY( m_displayTarget );
	PROPERTY( m_finishableEnemies );
	PROPERTY( m_hostileEnemies );
	PROPERTY( m_defaultSelectionWeights );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

struct SR4PlayerTargetingOut
{
	DECLARE_RTTI_STRUCT( SR4PlayerTargetingOut );

	THandle< CActor >	m_target;
	Bool				m_result;
	Bool				m_confirmNewTarget;
	Bool				m_forceDisableUpdatePosition;

	SR4PlayerTargetingOut();
	void Reset();
};

BEGIN_CLASS_RTTI( SR4PlayerTargetingOut );
	PROPERTY( m_target );
	PROPERTY( m_result );
	PROPERTY( m_confirmNewTarget );
	PROPERTY( m_forceDisableUpdatePosition );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CR4PlayerTargeting : public IScriptable
{
	DECLARE_RTTI_SIMPLE_CLASS( CR4PlayerTargeting );

	static const Float	HARD_LOCK_DISTANCE;
	static const Float	SOFT_LOCK_DISTANCE_VISIBILITY_DURATION;
	static const Float IS_THREAT_EXPANDED_DISTANCE;

	CR4Player*					m_player;
	SR4PlayerTargetingConsts	m_consts;
	SR4PlayerTargetingPrecalcs	m_precalcs;
	SR4PlayerTargetingIn		m_inValues;
	SR4PlayerTargetingOut		m_outValues;

	typedef THashMap< CActor*, EngineTime > TVisibleActors;
	TVisibleActors				m_visibleActors;

public:

	CR4PlayerTargeting();
	~CR4PlayerTargeting();

protected:

	void BeginFindTarget( const SR4PlayerTargetingIn& inValues );
	void EndFindTarget( SR4PlayerTargetingOut& outValues );
	void FindTarget();

	void UpdateVisibleActors();

	void FindTargetsInCone( TDynArray< CActor* > & targets, Vector& outHeadingVector );
	void RemoveNonTargetable( TDynArray< CActor* > & targets, STargetingInfo& info, const Vector& selectionHeadingVector );
	CActor* SelectTarget( TDynArray< CActor* > & targets, Bool useVisibilityCheck, const Vector& sourcePosition, const Vector& headingVector, STargetSelectionWeights& selectionWeights );
	void FilterActors( TDynArray< CActor* > & targets, Bool& onlyThreatTargetsFound );

	Bool IsEntityTargetable( STargetingInfo& info );
	Bool CanBeTargetedIfSwimming( CActor* actor );
	Bool IsThreat( CActor* actor );
	Bool WasVisibleInScaledFrame( CEntity* entity, Float frameSizeX, Float frameSizeY );
	Bool GetObjectBoundingVolume( CEntity* entity, Box& box );

	// helpers
	Bool GetGameplayVisibility( CActor* actor ) const;
	Bool IsKnockedUnconscious( CActor* actor ) const;
	Bool CanBeTargeted( CActor* actor ) const;
	Bool IsDodgingOrRolling( CActor* actor ) const;
	Bool IsFlying( CActor* actor ) const;
	
	void HardLockToTarget( Bool lock );
	void ForceSelectLockTarget();

	// scripts support
	void funcSetConsts( CScriptStackFrame& stack, void* result );
	void funcBeginFindTarget( CScriptStackFrame& stack, void* result );
	void funcEndFindTarget( CScriptStackFrame& stack, void* result );
	void funcFindTarget( CScriptStackFrame& stack, void* result );
	void funcWasVisibleInScaledFrame( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CR4PlayerTargeting );
	PARENT_CLASS( IScriptable );
	NATIVE_FUNCTION( "SetConsts", funcSetConsts );
	NATIVE_FUNCTION( "BeginFindTarget", funcBeginFindTarget );
	NATIVE_FUNCTION( "EndFindTarget", funcEndFindTarget );
	NATIVE_FUNCTION( "FindTarget", funcFindTarget );
	NATIVE_FUNCTION( "WasVisibleInScaledFrame", funcWasVisibleInScaledFrame );
END_CLASS_RTTI();
