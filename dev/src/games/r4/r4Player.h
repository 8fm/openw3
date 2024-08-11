/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/game/player.h"

#include "combatDataComponent.h"
#include "enemiesCachedData.h"
#include "r4PlayerControlAction.h"
#include "ridingAiStorage.h" 
#include "ticketSystem.h"
#include "../../common/game/asyncCheckResult.h"

#define PLAYER_HORSE_TAG ( CNAME( PLAYER_horse ) )
#define PLAYER_LAST_MOUNTED_VEHICLE_TAG ( CNAME( player_last_mounted_vehicle ) )

class CR4Player : public CPlayer
{
	DECLARE_ENGINE_CLASS( CR4Player, CPlayer, CF_Placeable );

protected:
	CR4PlayerControlAction				m_actionPlayerControl;
	EAsyncCheckResult					m_uselessProp;			//it is not used anywhere but I need this prop, so EAsyncCheckResult is visible in scripts.
	EntityHandle						m_horseWithInventory;	// handle to Roach aka "PLAYER_horse" - default 'inventory' to be summoned almost anywhere/anytime (if it's allowed by quest etc.)

protected: // HACKS
	struct Hack_SwordTrajectory
	{
		Uint64							m_tickNum;
		TDynArray< Vector >				m_pointsWS;
		Float							m_frameWeight;
	};
	Hack_SwordTrajectory				hack_prev;
	Hack_SwordTrajectory				hack_curr;

	Int32								m_isInInterior;								// Processed by r4interiorAreaComponent.cpp
	Int32								m_isInSettlement;
	Bool								m_isInCombat;
	CEnemiesCachedData					m_enemyData;
	TArrayMap< CName, CTicket* >		m_tickets;									// combat target generic ticket interface
	CCombatDataPtr						m_combatTargetData;							// combat target related
	CCombatDataPtr						m_playerCombatData;							// my own combat data
	THandle< CActor >					m_combatTarget;								// combat target
	THandle< CActor >					m_scriptTarget;								// its quite unclear that we have so many 'targets', but they are related to separate features we want to modify indepently
	THandle< CActor >					m_moveTarget;


#if !defined( NO_TELEMETRY ) || !defined( NO_SECOND_SCREEN )
	Red::System::Timer			m_positionSamplingTimer;
	Double						m_lastReportedTime;
	Vector						m_lastReportedPosition;
	Float						m_lastReportedRotation;
	Double						m_lastReportedRotationTime;

	Bool ShouldReportMovement( const Vector& position ) const;
	Bool ShouldReportRotation( const Float rotation, const Double currentTime ) const;
	Bool ShouldReportPosition( const Double currentTime ) const;

	void ReportMovement( const Vector& position, const Float rotation, const Double currentTime );
	void ReportRotation( const Vector& position, const Float rotation, const Double currentTime );
	void ReportPosition( const Double currentTime );
#endif // NO_TELEMETRY

	void ClearTickets();
public:
	CR4Player();
	~CR4Player();

	//! Sets whether the NPC is in an interior or not (r4interiorAreaComponent.cpp)
	RED_INLINE void SetInInterior(Int32 delta) { m_isInInterior += delta; }

	//! Is NPC in an interior?
	RED_INLINE Bool IsInInterior() const { return (m_isInInterior > 0); }

	//! Sets whether the NPC is in an interior or not (r4interiorAreaComponent.cpp)
	RED_INLINE void SetInSettlement(Int32 delta) { m_isInSettlement += delta; }

	//! Is NPC in an interior?
	RED_INLINE Bool IsInSettlement() const { return (m_isInSettlement > 0); }


	// Teleport entity to new location
	virtual Bool Teleport( const Vector& position, const EulerAngles& rotation ) override;

	virtual void GenerateDebugFragments( CRenderFrame* frame ) override;

	virtual void OnTick( Float timeDelta ) override;
	virtual void OnDetached( CWorld* world ) override;

	void SetPlayerCombatTarget( CActor* actor );
	Bool ObtainTicketFromCombatTarget( CName ticketName, Uint32 ticketCount );
	void FreeTicketAtCombatTarget();

	void SetIsInCombat( Bool b );
	virtual Bool IsInCombat() const override;

	Bool ActionDirectControl( const THandle< CR4LocomotionDirectController >& playerController );

	virtual CActor* GetScriptTarget() const override;
	void SetScriptTarget( CActor* actor );

	Bool IsInCombatActionCameraRotationEnabled();

	Bool IsGuarded();

	Bool IsLockedToTarget();

	RED_INLINE Bool IsEnemyVisible( THandle< CActor> enemy ) const { return m_enemyData.IsVisible( enemy ); }
	void GetVisibleEnemies( TDynArray< CActor* > & enemies ) const;
	void GetVisibleEnemies( TDynArray< THandle< CActor > > & enemies ) const;

	RED_INLINE const CEnemiesCachedData& GetEnemyData() const { return m_enemyData; }

	RED_INLINE CActor*				GetScriptMoveTarget() const { return m_moveTarget.Get(); }
	CAIStorageRiderData *const		GetRiderData();
	const CActor *					GetHorseWithInventory()const				{ return static_cast< const CActor * > ( m_horseWithInventory.Get() ); }
	void							SetHorseWithInventory( CActor *const actor ){ m_horseWithInventory.Set( actor ); }

	Bool ShouldPlayPreDialogCutscene( THandle< CActor >& vehicle ) override;
	CCutsceneInstance* StartPreDialogCutscene( THandle< CActor >& vehicle ) override;
	void OnPostPreDialogCutscene( THandle< CActor >& vehicle, CStoryScenePlayer* scene ) override;

public: // Hacks
	virtual void Hack_SetSwordTrajectory( TDynArray< Vector >& dataWS, Float w );

	virtual Bool HandleWorldChangeOnBoat() override;

private:
	void funcIsInInterior( CScriptStackFrame& stack, void* result );
	void funcIsInSettlement( CScriptStackFrame& stack, void* result );
	void funcEnterSettlement( CScriptStackFrame& stack, void* result );
	void funcGetEnemiesInRange( CScriptStackFrame& stack, void* result );
	void funcGetVisibleEnemies( CScriptStackFrame& stack, void* result );
	void funcIsEnemyVisible( CScriptStackFrame& stack, void* result );
	void funcSetupEnemiesCollection( CScriptStackFrame& stack, void* result );
	void funcActionDirectControl( CScriptStackFrame& stack, void* result );
	void funcGetCombatDataComponent( CScriptStackFrame& stack, void* result );
	void funcSetPlayerTarget( CScriptStackFrame& stack, void* result );
	void funcSetPlayerCombatTarget( CScriptStackFrame& stack, void* result );
	void funcObtainTicketFromCombatTarget( CScriptStackFrame& stack, void* result );
	void funcFreeTicketAtCombatTarget( CScriptStackFrame& stack, void* result );
	void funcSetScriptMoveTarget( CScriptStackFrame& stack, void* result );
	void funcGetRiderData( CScriptStackFrame& stack, void* result );
	void funcSetIsInCombat( CScriptStackFrame& stack, void* result );
	void funcSetBacklightFromHealth( CScriptStackFrame& stack, void* result );
	void funcSetBacklightColor( CScriptStackFrame& stack, void* result );
	void funcGetTemplatePathAndAppearance( CScriptStackFrame& stack, void* result );
	void funcHACK_BoatDismountPositionCorrection( CScriptStackFrame& stack, void* result );
	void funcSaveLastMountedHorse( CScriptStackFrame& stack, void* result );
	void funcHACK_ForceGetBonePosition( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CR4Player );
	PARENT_CLASS( CPlayer );
	PROPERTY( m_uselessProp );
	PROPERTY_SAVED( m_horseWithInventory );
	NATIVE_FUNCTION( "IsInInterior", funcIsInInterior );
	NATIVE_FUNCTION( "IsInSettlement", funcIsInSettlement );
	NATIVE_FUNCTION( "EnterSettlement", funcEnterSettlement );
	NATIVE_FUNCTION( "GetEnemiesInRange", funcGetEnemiesInRange );
	NATIVE_FUNCTION( "GetVisibleEnemies", funcGetVisibleEnemies );
	NATIVE_FUNCTION( "IsEnemyVisible", funcIsEnemyVisible );
	NATIVE_FUNCTION( "SetupEnemiesCollection", funcSetupEnemiesCollection );
	NATIVE_FUNCTION( "ActionDirectControl", funcActionDirectControl );
	NATIVE_FUNCTION( "GetCombatDataComponent", funcGetCombatDataComponent );
	NATIVE_FUNCTION( "SetPlayerTarget", funcSetPlayerTarget );
	NATIVE_FUNCTION( "SetPlayerCombatTarget", funcSetPlayerCombatTarget );
	NATIVE_FUNCTION( "ObtainTicketFromCombatTarget", funcObtainTicketFromCombatTarget );
	NATIVE_FUNCTION( "FreeTicketAtCombatTarget", funcFreeTicketAtCombatTarget );
	NATIVE_FUNCTION( "SetScriptMoveTarget", funcSetScriptMoveTarget );
	NATIVE_FUNCTION( "GetRiderData", funcGetRiderData );
	NATIVE_FUNCTION( "SetIsInCombat", funcSetIsInCombat );
	NATIVE_FUNCTION( "SetBacklightFromHealth", funcSetBacklightFromHealth );
	NATIVE_FUNCTION( "SetBacklightColor", funcSetBacklightColor );
	NATIVE_FUNCTION( "GetTemplatePathAndAppearance", funcGetTemplatePathAndAppearance );
	NATIVE_FUNCTION( "HACK_BoatDismountPositionCorrection", funcHACK_BoatDismountPositionCorrection );
	NATIVE_FUNCTION( "SaveLastMountedHorse", funcSaveLastMountedHorse );
	NATIVE_FUNCTION( "HACK_ForceGetBonePosition", funcHACK_ForceGetBonePosition );
	END_CLASS_RTTI();
