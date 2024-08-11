/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "interactionArea.h"

class CHudInstance;
typedef TDynArray< CEntity* > TActivatorList;

// Component for handling interaction with user
class CInteractionComponent : public CInteractionAreaComponent, public I2dArrayPropertyOwner
{
	friend class CInteractionsManager;

	DECLARE_ENGINE_CLASS( CInteractionComponent, CInteractionAreaComponent, 0 );

protected:

	enum EInteractionComponentFlags : Uint32
	{
		ICF_IsTalkInteraction			= FLAG( 0 ),
		ICF_IsEneabledOnVehicles		= FLAG( 1 ),
		ICF_IsEnabledOnVehiclesCached	= FLAG( 2 ),
		ICF_HasGenericFriendlyName		= FLAG( 3 ),
	};

	String				m_actionName;				//!< Name of the interaction
	TActivatorList		m_activatorsList;			//!< List of activators
	Bool				m_checkCameraVisibility;	//!< Check camera visibility
	Bool				m_reportToScript;			//!< Report this interaction to the script system
	Bool				m_isEnabledInCombat;		//!< Check if the interaction is enabled while combat
	Bool				m_shouldIgnoreLocks;		//!< Should interaction ignore LockButtonInteractions
	Vector				m_lastUpdatedPosition;		//!< Stored (in manager) last world position
	mutable Uint32		m_flags;

public:
	//! Default constructor
	CInteractionComponent();

	//! Component was attached to world
	virtual void OnAttached( CWorld* world ) override;

	//! Component was detached from world
	virtual void OnDetached( CWorld *world ) override;

	//! Component is streamed in
	virtual void OnStreamIn() override;

	//! Called when we need to store gameplay state of this entity
	virtual void OnSaveGameplayState( IGameSaver* saver ) override;

	//! Called when we need to restore gameplay state of this entity
	virtual void OnLoadGameplayState( IGameLoader* loader ) override; 

	//! Component transform was updated
	virtual void OnUpdateTransformComponent( SUpdateTransformContext& context, const Matrix& prevLocalToWorld ) override;

	//! Enable or disable component
	virtual void SetEnabled( Bool enabled ) override;

	//! Set friendly name
	virtual void SetFriendlyName( String friendlyName );

	//! Test if given entity can activate this interaction
	virtual Bool ActivationTest( CEntity* activator, const SActivatorData& activatorData ) const override;

	//! Get the name of the interaction
	String GetInteractionFriendlyName();

	//! Get the hint input
	EInputKey GetInteractionKey() const;
	CName GetInteractionGameInput() const;

	//! Is the interaction active ?
	RED_INLINE Bool IsActive() const { return !m_activatorsList.Empty(); };

	//! Get the name of this interaction
	RED_INLINE const String& GetActionName() const { return m_actionName; }

	//! Set action name
	void SetActionName( const String& action );

	//! Has action
	RED_INLINE Bool HasAction() const { return m_actionName.Empty() == false; }

	//! Is interaction currently shown on Gui
	RED_INLINE Bool IsTalkInteraction() const { return ( m_flags & ICF_IsTalkInteraction ); }

	//! Can execute action (interaction can be active and visible but not "executable")
	Bool CanExecute() const;

	//! Is interaction enabled while using vehicles
	RED_INLINE Bool IsEnabledOnVehicles() const;

	//! Should interaction ignore LockButtonInteractions
	RED_INLINE Bool ShouldIgnoreLocks() const { return m_shouldIgnoreLocks; }

	//! Get interaction storage bounds
	virtual void GetStorageBounds( Box& box ) const override;

protected:

	// Update component's position held in CInteractionsManager
	void UpdatePositionInManager( Bool force );

	Bool NotifyExecution();

	virtual Bool NotifyActivation( CEntity* activator );

	virtual Bool NotifyDeactivation( CEntity* activator );

	//! Interaction was activated by entity
	virtual void OnActivate( CEntity* activator );	

	//! Interaction was deactivated by entity
	virtual void OnDeactivate( CEntity* activator );

	//! Called when interaction is executed
	virtual void OnExecute();

	//! Test if the circumstances are right for the component to be activated (like if there are dialogs to talk about etc.)
	Bool CameraVisibilityTest( const SActivatorData& activatorData ) const;

	//! Test if the circumstances are right for the component to be activated (like if there are dialogs to talk about etc.)
	Bool CircumstancesTest( const SActivatorData& activatorData ) const;

	//! Get interaction friendly name from interactions manager based on action name
	Bool UpdateGenericFriendlyName();

	//! Update component state after action name changed
	void OnActionNameChanged();

	// Get fragments color
	virtual Color GetColor() const;

	//! Generate editor fragments
	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag );

	//! I2dArrayPropertyOwner
	virtual void Get2dArrayPropertyAdditionalProperties( IProperty *property, SConst2daValueProperties &valueProperties ) override;

	//! Returns the minimum streaming distance for this component
	virtual Uint32 GetMinimumStreamingDistance() const { return 10; }

private:
	void funcGetActionName( CScriptStackFrame& stack, void* result );
	void funcSetActionName( CScriptStackFrame& stack, void* result );
	void funcGetInteractionFriendlyName( CScriptStackFrame& stack, void* result );
	void funcGetInteractionKey( CScriptStackFrame& stack, void* result );
	void funcGetInputActionName( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CInteractionComponent )
	PARENT_CLASS( CInteractionAreaComponent )
	PROPERTY_CUSTOM_EDIT( m_actionName, TXT("Action name"), TXT("2daValueSelection") );
	PROPERTY_EDIT( m_checkCameraVisibility, TXT("Check if entity with the interaction is visible in the game camera") );
	PROPERTY_EDIT( m_reportToScript, TXT("Report entering/exiting of this interaction to th script system") );
	PROPERTY_EDIT( m_isEnabledInCombat, TXT("Check if the interaction is enabled while combat") );
	PROPERTY_EDIT( m_shouldIgnoreLocks, TXT("Should interaction ignore LockButtonInteractions") );
	NATIVE_FUNCTION( "GetActionName", funcGetActionName );
	NATIVE_FUNCTION( "SetActionName", funcSetActionName );
	NATIVE_FUNCTION( "GetInteractionFriendlyName", funcGetInteractionFriendlyName );
	NATIVE_FUNCTION( "GetInteractionKey", funcGetInteractionKey );
	NATIVE_FUNCTION( "GetInputActionName", funcGetInputActionName );
END_CLASS_RTTI();
