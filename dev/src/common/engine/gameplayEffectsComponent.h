/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "component.h"
#include "node.h"
#include "entity.h"


enum EEntityGameplayEffectFlags
{
	EGEF_None							= 0,
	EGEF_FocusModeHighlight				= FLAG( 0 ),
	EGEF_CatViewHiglight				= FLAG( 1 ),
};
BEGIN_ENUM_RTTI( EEntityGameplayEffectFlags );
ENUM_OPTION( EGEF_FocusModeHighlight );
ENUM_OPTION( EGEF_CatViewHiglight );
END_ENUM_RTTI();



//////////////////////////////////////////////////////////////////////////
// NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE
//
// This is the one acceptable place for using INodeAttachListener and
// IEntityListener. They exist to support this component.
// DO NOT USE THEM ELSEWHERE!
//    -- TG
//////////////////////////////////////////////////////////////////////////

// CGameplayEffectsComponent provides a mechanism for turning on/off certain gameplay effects
// (highlights in focus mode) at runtime, through scripts. Simply put one of these in an entity,
// and it'll make the necessary adjustments to any components in the entity (or child entities,
// such as items and equipment).
class CGameplayEffectsComponent : public CComponent, public INodeAttachListener, public IEntityListener
{
	DECLARE_ENGINE_CLASS( CGameplayEffectsComponent, CComponent, 0 );

protected:
	Uint32	m_flags;								//!< Current state of gameplay effects
	Uint32	m_mask;									//!< Which effects have been modified by this GEC.

	Bool	m_enabled;

	TDynArray< THandle< CEntity > >	m_entities;		//!< The entities being affected. This is the GEC's parent entity,
													//!< plus child entities. Child entities are included, so that held
													//!< items will be affected.

public:
	CGameplayEffectsComponent();
	~CGameplayEffectsComponent();

	virtual Bool IsEnabled() const { return m_enabled; }
	virtual void SetEnabled( Bool enabled );

	virtual void OnAttached( CWorld* world ) override;
	virtual void OnDetached( CWorld* world ) override;
	virtual void OnDestroyed() override;
	virtual bool UsesAutoUpdateTransform() override { return false; }

	/// INodeAttachListener
	virtual void OnNotifyNodeChildAttached( CNode* parent, CNode* child, IAttachment* attachment );
	virtual void OnNotifyNodeChildDetached( CNode* parent, CNode* child );
	virtual void OnNotifyNodeParentAttached( CNode* parent, CNode* child, IAttachment* attachment ) {}
	virtual void OnNotifyNodeParentDetached( CNode* parent, CNode* child ) {}

	/// IEntityListener
	virtual void OnNotifyEntityComponentAdded( CEntity* entity, CComponent* component );
	virtual void OnNotifyEntityComponentRemoved( CEntity* entity, CComponent* component );
	virtual void OnNotifyEntityRenderProxyAdded( CEntity* entity, CComponent* component, IRenderProxy* proxy );
	virtual void OnNotifyEntityRenderProxyRemoved( CEntity* entity, CComponent* component ) {}


	// Set flag either on or off for the entire entity, and child entities.
	void SetGameplayEffectFlag( EEntityGameplayEffectFlags flag, Bool value );

	// Get value for given flag. If it has not been set, or has been reset, this will be false.
	RED_FORCE_INLINE Bool GetGameplayEffectFlag( EEntityGameplayEffectFlags flag ) const { return ( m_flags & flag ) != 0; }

	// Set flag back to its default, as determined by each affected component.
	void ResetGameplayEffectFlag( EEntityGameplayEffectFlags flag );


protected:
	// Called when ready to use effect flags, to send initial state and listen for changes to entities/components.
	// Safe to call multiple times.
	void StartListening();
	// Called when no longer needed (after resetting), to stop listening for changes.
	void StopListening();

	void AttachToChildEntities( CNode* parentNode );
	void DetachFromChildEntities( CNode* parentNode );


	void ResetFlagsOnAllComponents();
	void SetFlagsOnAllComponents( Uint32 flags );

	// Reset effect flags to their defaults, as given by each component.
	static void ResetFlagsOnComponents( const TDynArray< CComponent* >& components, Uint32 mask );
	static void ResetFlagsOnComponent( CComponent* component, Uint32 mask );

	static void SetFlagsOnComponents( const TDynArray< CComponent* >& components, Uint32 flags, Uint32 mask );
	static void SetFlagsOnComponent( CComponent* component, Uint32 flags, Uint32 mask );

	// Translate from gameplay effect flags to light channels.
	static void CalcLightChannels( Uint32 flags, Uint32 mask, Uint8& lc, Uint8& lcMask );

protected:
	void funcSetGameplayEffectFlag( CScriptStackFrame& stack, void* result );
	void funcGetGameplayEffectFlag( CScriptStackFrame& stack, void* result );
	void funcResetGameplayEffectFlag( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CGameplayEffectsComponent );
PARENT_CLASS( CComponent );
NATIVE_FUNCTION( "SetGameplayEffectFlag", funcSetGameplayEffectFlag );
NATIVE_FUNCTION( "GetGameplayEffectFlag", funcGetGameplayEffectFlag );
NATIVE_FUNCTION( "ResetGameplayEffectFlag", funcResetGameplayEffectFlag );
END_CLASS_RTTI();
