/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/game/player.h"

#include "directMovementLocomotionSegment.h"


class CAimHelpTargetsGatherer;
class CPlayerInventoryPanel;
class CIDInterlocutorComponent;
class CPlayerCraftingPanel;
class CR6InventoryItemComponent;
class CR6CameraComponent;

class CR6Player : public CPlayer
{
	DECLARE_ENGINE_CLASS( CR6Player, CPlayer, CF_Placeable );

private:

	Float											m_aimYawF;
	Float											m_aimPitchF;
	Vector											m_aimTarget;

	TDynArray< THandle< CR6InteractionComponent > >	m_interactionQueue;

	// Inventory / Interactions
	THandle< CPlayerInventoryPanel >				m_inventoryPanel;	
	THandle<CR6InventoryItemComponent>				m_equippedItem;

	// Crafting (TODO: Move somewhere else than player)
	THandle< CPlayerCraftingPanel >					m_craftingPanel;	


	CR6CameraComponent*								m_defaultCameraComponent;

public:
	CR6Player();

	//! Attachments
	virtual void				OnAttached	( CWorld* world );
	
	//! Tick
	virtual void				OnTick		( Float timeDelta );	

	// Equipped Item
	CR6InventoryItemComponent*	GetEquippedItem		( );

	//! Camera
	virtual void				RecreateCamera		( );
	Float						GetAimYaw			( )	const			{ return m_aimYawF;			}
	Float						GetAimPitch			( )	const			{ return m_aimPitchF;		}
	Vector						GetAimTarget		( )	const			{ return m_aimTarget;		}
	void						SetAimTarget		( Vector3 target );
	void						SetAimYawPitch		( Float yaw, Float pitch );
	
	CR6InteractionComponent*	GetInteractionTarget( );

	virtual Bool				Teleport								( const Vector& position, const EulerAngles& rotation );

	//! Dialog
	CIDInterlocutorComponent*	GetInterlocutorComponent();

	//! Debug
	void GenerateDebugFragments		( CRenderFrame* frame ) override;


public:

	CR6CameraComponent* GetCurrentAutoCameraComponent();

private:

	CR6CameraComponent* GetDefaultCameraFromEntity( CEntity* parentEntity );
	CR6CameraComponent* GetDefaultCameraComponent();

	const CResource*	GetPlayerResource() const;

public:

	/// @return true if this player is attached to AV - driving, passenging, getting in / out
	Bool	IsAttachedToAV();

	/// @return NULL if no AV is attached, otherwise a pointer to the AV node is returned
	/// @see IsAttachedToAV
	CNode* GetAttachedAVNode();

protected:
	void UpdateCameraRotation		( Float timeDelta );
	void UpdateCamHeight			( );
	
public:

	void funcGetInteractionTarget					( CScriptStackFrame& stack, void* result );
	
	void funcRayTest								( CScriptStackFrame& stack, void* result );

	void funcGetDialogInterlocutor					( CScriptStackFrame& stack, void* result );

	void funcGetInventoryPanel						( CScriptStackFrame& stack, void* result );
	void funcGetInteractionsHints					( CScriptStackFrame& stack, void* result );
	void funcFakeShoot								( CScriptStackFrame& stack, void* result );
	void funcGetEntityInteractions					( CScriptStackFrame& stack, void* result );

	void funcEnableCharacterControllerPhysics		( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CR6Player );
	PARENT_CLASS( CPlayer );
	PROPERTY_NAME( m_inventoryPanel,				TXT( "i_inventoryPanel" ) );	
	PROPERTY_NAME( m_craftingPanel,					TXT( "i_craftingPanel" ) );	
	PROPERTY_NAME( m_equippedItem,					TXT( "i_equippedItem" ) );
	PROPERTY_NAME( m_aimYawF,						TXT( "i_AimYawF" ) );
	PROPERTY_NAME( m_aimPitchF,						TXT( "i_AimPitchF" ) );
	PROPERTY_NAME( m_aimTarget,						TXT( "i_AimTargetV" ) );
	PROPERTY_NAME( m_interactionQueue,				TXT( "i_InteractionQueueCArr" ) );

	NATIVE_FUNCTION( "I_GetInteractionTargetC"					, funcGetInteractionTarget );	

	NATIVE_FUNCTION( "RayTest"									, funcRayTest);

	
	NATIVE_FUNCTION( "I_GetDialogInterlocutor"					, funcGetDialogInterlocutor);

	NATIVE_FUNCTION( "I_GetEntityInteractions"					, funcGetEntityInteractions	 );		

	NATIVE_FUNCTION( "I_EnableCharacterControllerPhysics"		, funcEnableCharacterControllerPhysics	 );		

END_CLASS_RTTI();
