/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CPlayerStateBase;

#include "actor.h"
#include "itemUniqueId.h"
#include "newNpc.h"


#include "itemVisibilityController.h"
#include "interactionComponent.h"

#include "../engine/inputListener.h"

class CInterestPoint;

///////////////////////////////////////////////////////////////////////////////

struct VoiceSetDef
{
	THandle< CNewNPC >	npc;
	String				voiceSet;

	VoiceSetDef( CNewNPC* _npc = NULL, const String& _voiceSet = String() )
		: npc( _npc ), voiceSet( _voiceSet )
	{}

	Bool operator==( const VoiceSetDef& rhs ) const
	{
		return npc == rhs.npc;
	}
};


///////////////////////////////////////////////////////////////////////////////

/// The player
class CPlayer : public CActor
{
	DECLARE_ENGINE_CLASS( CPlayer, CActor, CF_Placeable );

protected:
	enum ECombatAreaSpace { CAS_Player, CAS_Camera };

	static const Float NOISE_THRESHOLD_SPEED;				//!< Above this speed player will generate noise
	static const Float INTERESTPOINT_UPDATE_INTERVAL;				//!< Generate interest point interval

	static const String EXPLORATION_INTERACTION_COMPONENT_NAME;

	static const Uint32 NUM_QUICK_SLOTS;						//!< Number of elements for the quick slots array

protected:

	TDynArray< THandle< CActionAreaComponent > >	m_activeExplorations;	//!< Exploration areas that may be activated via trigger

	// Interest points
	CInterestPoint*							m_presenceInterestPoint;
	CInterestPoint*							m_slowMovementInterestPoint;
	CInterestPoint*							m_fastMovementInterestPoint;

	CInterestPoint*							m_weaponDrawnInterestPoint;
	CInterestPoint*							m_weaponDrawMomentInterestPoint;
	CInterestPoint*							m_visionInterestPoint;
	Float									m_timeToInterestPointUpdate;

	// VoiceSets selection & playing
	Float									m_npcVoicesetCooldown;
	Float									m_npcVoicesetCooldownTimer;
	Int32									m_lockProcessButtonInteraction;
	Bool									m_isMovable;
	Bool									m_enemyUpscaling; //!< Enemy upscaling getter, moved directly from scripts

	TDynArray< VoiceSetDef >				m_voiceSetsToChooseFrom;


	CInteractionComponent*					m_explorationComponent;
	
	ItemVisibilityController				m_itemVisibilityController;

public:
	//! Check if it is player
	virtual Bool IsPlayer() const { return true; }

	//! Get vision interest point
	CInterestPoint*	GetVisionInterestPoint() const { return m_visionInterestPoint; }

public:
	CPlayer();
	virtual ~CPlayer();

	//! Get current player state
	CPlayerStateBase* GetCurrentState() const;

	//! Attached/Detach
	virtual void OnAttached( CWorld* world );
	virtual void OnDetached( CWorld* world );

	// All components of entity has been attached
	virtual void OnAttachFinished( CWorld* world );

	//! Tick
	virtual void OnTick( Float timeDelta );

	//! Generate debug fragments
	virtual void GenerateDebugFragments( CRenderFrame* frame );

	// Process interaction ( TODO: move to game code! )
	virtual void OnProcessInteractionExecute( class CInteractionComponent* interaction );

	//! Set player movability
	Bool SetPlayerMovable( Bool movable, Bool enableSteerAgent );
	Bool GetPlayerMovable() const;

	void ActivateActionArea  ( CActionAreaComponent * area );
	void DeactivateActionArea( CActionAreaComponent * area );
	void BestActionAreaToFront();

	// Exploration
	Bool IsExplorationInteraction( const CInteractionComponent* interaction ) const;
	Bool HasAnyActiveExploration() const;
	const Matrix& GetActiveExplorationTranform() const;

	virtual Bool OnExplorationStarted();
	virtual void OnExplorationEnded();

	//! Can process button interactions
	Bool CanProcessButtonInteractions() const;

	//! Lock/unlock button interaction for player
	void LockButtonInteractions( Int32 channel );
	void UnlockButtonInteractions(  Int32 channel );

    virtual void OnParentAttachmentBroken( IAttachment* attachment ) override;

	virtual void OnGrabItem( SItemUniqueId itemId, CName slot ) override;
	virtual void OnPutItem( SItemUniqueId itemId, Bool emptyHand ) override;
	virtual void OnMountItem( SItemUniqueId itemId, Bool wasHeld ) override;
	virtual void OnUnmountItem( SItemUniqueId itemId ) override;

	virtual void EquipItem( SItemUniqueId itemId, Bool ignoreMount ) override;
	virtual void UnequipItem( SItemUniqueId itemId ) override;

public:
	//////////////////////////////////////////////////////////////////////////

	//! Plays a voiceset on the npc, providing the conditions for that are met
	void PlayVoicesetForNPC( CNewNPC* npc, const String& voiceSet );

	//! Is foreground entity
	virtual Bool IsForegroundEntity() const { return true; }

protected:
	void UpdateVoicesets( Float timeElapsed );
	Float EstimateVoiceSetSpeaker( CNewNPC* npc ) const;

	//////////////////////////////////////////////////////////////////////////

	//! Activate/deactivate the internal interest point if the player moves
	void UpdateInterestPoint( Float timeDelta );

	void UpdateWeaponInterestPoint();

	//! On draw weapon, used in player
	virtual void OnDrawWeapon();
	
	virtual Bool CanStealOtherActor( const CActor* const other ) const override { return true; }

public:
	// Called on cutscene started
	virtual void OnCutsceneStarted();

	// Called on cutscene ended
	virtual void OnCutsceneEnded();

	// Determines whether pre-dialog cutscene shall be played (e.g. horse dismount)
	virtual Bool ShouldPlayPreDialogCutscene( THandle< CActor >& vehicle ) { return false; }
	// Starts cutscene to play prior to dialog (e.g. horse dismount); returns created cutscene instance
	virtual CCutsceneInstance* StartPreDialogCutscene( THandle< CActor >& vehicle ) { return nullptr; }
	// Invoked once pre-dialog cutscene is done
	virtual void OnPostPreDialogCutscene( THandle< CActor >& vehicle, CStoryScenePlayer* scene ) {}

	Bool IsGameplayLODable( ) override { return false; }

	virtual Bool HandleWorldChangeOnBoat() { return false; }

public:
	// ------------------------------------------------------------------------
	// Game saves
	// ------------------------------------------------------------------------
	//! Called when we need to restore gameplay state of this entity
	virtual void OnLoadGameplayState( IGameLoader* loader );

	//! Should save?
	virtual Bool CheckShouldSave() const { return true; }

public:
	/************************************************************************/
	/* Hacks                                                                */
	/************************************************************************/
	virtual void Hack_SetSwordsHiddenInGame( Bool state, Float distanceToCamera, Float cameraYaw );
	Bool HACK_ForceGetBonePosWS( Uint32 index, Vector& bonePosWS ) const;

private:
	void funcLockButtonInteractions( CScriptStackFrame& stack, void* result );
	void funcUnlockButtonInteractions( CScriptStackFrame& stack, void* result );
	void funcGetActiveExplorationEntity( CScriptStackFrame& stack, void* result );
	void funcSetEnemyUpscaling( CScriptStackFrame& stack, void* result );
	void funcGetEnemyUpscaling( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CPlayer )
	PARENT_CLASS( CActor )
	PROPERTY_EDIT( m_npcVoicesetCooldown, TXT( "How often can the player see another person speaking a VoiceSet" ) )
	PROPERTY_INLINED( m_presenceInterestPoint, TXT( "Interest point always generated by player" ) )
	PROPERTY_INLINED( m_slowMovementInterestPoint, TXT( "Interest point generated when player moves slowly" ) )
	PROPERTY_INLINED( m_fastMovementInterestPoint, TXT( "Interest point generated when player moves fast" ) )
	PROPERTY_INLINED( m_weaponDrawnInterestPoint, TXT( "Interest point generated when player has a weapon drawn" ) )
	PROPERTY_INLINED( m_weaponDrawMomentInterestPoint, TXT( "Interest point generated at moment of drawing weapon" ) )
	PROPERTY_INLINED( m_visionInterestPoint, TXT( "Interest point generated when npc sees player" ) )
	PROPERTY_NOSERIALIZE( m_isMovable );
	PROPERTY_SAVED( m_enemyUpscaling );
	NATIVE_FUNCTION( "LockButtonInteractions", funcLockButtonInteractions );
	NATIVE_FUNCTION( "UnlockButtonInteractions", funcUnlockButtonInteractions );
	NATIVE_FUNCTION( "GetActiveExplorationEntity", funcGetActiveExplorationEntity );
	NATIVE_FUNCTION( "SetEnemyUpscaling", funcSetEnemyUpscaling );
	NATIVE_FUNCTION( "GetEnemyUpscaling", funcGetEnemyUpscaling );
END_CLASS_RTTI()
