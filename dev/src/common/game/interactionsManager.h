/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "entitiesDetector.h"
#include "binaryStorage.h"
#include "targetingUtils.h"

///////////////////////////////////////////////////////////////////////////////

class CInteractionsManager
	: public IGameSystem							// TODO: Think if it really should be an IGameSystem implementation
	, public IInputListener
{
	DECLARE_ENGINE_CLASS( CInteractionsManager, IGameSystem, 0 );
	
	class CActivator;

	//////////////////////////////////////////////////////////////////////////
	// Activation event

	class CActivationEvent
	{
		CInteractionComponent*		m_interaction;
		CActivator*					m_activator;
		Bool						m_isActivate;

	public:

		CActivationEvent( CInteractionComponent* interaction = nullptr, CActivator* activator = nullptr, Bool isActivate = false );
		void Call();
	};

	//////////////////////////////////////////////////////////////////////////
	// Some useful typedefs 

	typedef THandle< CInteractionComponent >	TInteraction;
	typedef CInteractionComponent*				TInteractionPtr;
	typedef TDynArray< TInteraction >			TInteractions;
	typedef TDynArray< THandle< CEntity > >		TRawActivators;
	typedef TDynArray< CActivationEvent >		TActivationEvents;
	typedef CQuadTreeStorage< CInteractionComponent, THandle< CInteractionComponent > >	TInteractionsTree;
	typedef THashSet< TInteraction >			TInteractionsToUpdate;

	//////////////////////////////////////////////////////////////////////////
	// General case activator

	class CActivator
	{
	protected:

		CEntity*				m_entity;					//!< Activator entity
		TInteractions			m_activeInteractions;		//!< List of interactions activated by this activator
		SActivatorData			m_activatorData;			//!< Cached "per-frame" activator gameplay data

	public:

		//! Constructor
		CActivator( CEntity* entity );

		//! Destroy activator
		virtual ~CActivator();

		//! Process interaction tests on this activator, generate activation/deactivation events
		void ProcessInteractions( const TInteractions& interactions, TActivationEvents& events );

		//! Get activator's entity
		RED_INLINE CEntity* GetEntity() { return m_entity; }

		//! Get activator's entity
		RED_INLINE const CEntity* GetEntity() const { return m_entity; }

		//! Get activator's world position
		RED_INLINE Vector GetWorldPosition() const { return ( m_entity != nullptr ) ? m_entity->GetWorldPosition() : Vector::ZEROS; }

		//! Get activator's active interactions
		RED_INLINE const TInteractions& GetActiveInteractions() const { return m_activeInteractions; }

		//! Get activator's per-frame data
		RED_INLINE const SActivatorData& GetActivatorData() const { return m_activatorData; }

	protected:

		//! Check if activator can process interactions
		virtual Bool CanProcessInteractions() const;

		//! Generate activation events by comparing two lists. IMPORTANT! Both interactions' lists need to be sorted, otherwise the method will not work properly.
		void GenerateActivationEvents( const TInteractions& newInteractions, const TInteractions& currentComponents, TActivationEvents& events );
	
		//! Process interaction tests on this activator, generate activation/deactivation events
		void FilterInteractions( const TInteractions& intreactions, Bool canProcessInteractions, TInteractions& filteredInteractions );
	};

	//////////////////////////////////////////////////////////////////////////
	// Player activator

	class CPlayerActivator : public CActivator
	{
	public:

		//! Constructor
		CPlayerActivator( CEntity* entity );

		//! Change player's entity
		RED_INLINE void SetEntity( CEntity* entity ) { m_entity = entity; }

	private:

		//! Can process button interaction
		virtual Bool CanProcessInteractions() const override;
	};

	//////////////////////////////////////////////////////////////////////////
	// Structure holding an activation value and action name. An array of such entries is associated with a single game input.
	
	struct SActivatedAction
	{
		Float	m_activationValue;
		String	m_actionName;
		String	m_friendlyName;

		SActivatedAction( const String& action, Float activation, const String& friendlyName ) 
			: m_actionName( action )
			, m_activationValue( activation ) 
			, m_friendlyName( friendlyName )
		{} 

		friend Bool operator== ( const SActivatedAction& l, const SActivatedAction& r )
		{
			return l.m_actionName == r.m_actionName && l.m_activationValue == r.m_activationValue && l.m_friendlyName == r.m_friendlyName;
		}
	};

protected:

	// List of registered activators
	TDynArray< CActivator* >					m_activators;			// List of registered interaction activator
	CPlayerActivator*							m_playerActivator;		// Extra link to player activator
	TRawActivators								m_activatorsToAdd;		// Activators to add, cached for the update period
	TRawActivators								m_activatorsToRemove;	// Activators to remove, cached for the update period

	// List of registered interactions
	TInteractionsTree							m_interactions;			// Interactions organized into quad-tree
	TInteractionsToUpdate						m_interactionsToUpdate;	// Set of interactions that changed their position
	Float										m_timeSinceLastUpdate;	// Time of interaction tree last update
	Red::Threads::CMutex						m_treeLock;				// Protects access to interactions tree

	Bool										m_isInTick;				// We are ticking
	Bool										m_forceGuiUpdate;		// Force updating interaction gui

	// Game input related stuff
	THashMap< String, CName >							m_actionToGameInput;	// maps action names to input keys
	THashMap< CName, TDynArray< SActivatedAction > >	m_gameInputToActions;	// maps input keys to list of possible actions to activate

public:

	CInteractionsManager();
	~CInteractionsManager();

	// IGameSystem
	virtual void OnGameStart( const CGameInfo& gameInfo ) override;
	virtual void OnGameEnd( const CGameInfo& gameInfo ) override;
	virtual void Tick( Float timeDelta ) override;

	// IInputListener
	virtual Bool OnGameInputEvent( const SInputAction& action ) override;

	//! Get first input key for given action
	Bool GetGameInputForAction( const String& actionName, CName&  gameInput, Float& activation ) const;

	//! Get friendly name for action
	Bool GetFriendlyNameForAction( const String& actionName, String& friendlyName ) const;

	// Adds an interaction component
	void AddInteractionComponent( CInteractionComponent* interaction );

	// Removes an interaction component
	void RemoveInteractionComponent( CInteractionComponent* interaction );

	// Adds interaction activator
	void AddInteractionActivator( CEntity *activator );

	// Removes interaction activator
	void RemoveInteractionActivator( CEntity *activator );

	// Register interaction component for its position update
	void RegisterForUpdate( CInteractionComponent* interaction );

protected:

	//! Update moved interactions tree position
	void UpdateTree();

	//! Update interactions buttons on gui
	void UpdateGuiButtons();
	
	//! Process input
	Bool ProcessInteractionInput( const SInputAction& inputAction );

	//! Loads key from the CSV file
	void LoadActionMappings();

	//! Clear arrays
	void ClearActionMappings();

	//! Adds a single definition (from CSV)
	void AddMapping( CName gameInput, const String& actionName, Float activation, const String& friendlyName );

	// Apply cached activator add/remove
	void ApplyCachedActivatorAddRemove();

public:

	ASSIGN_GAME_SYSTEM_ID( GS_InteractionManager );
};

///////////////////////////////////////////////////////////////////////////////

BEGIN_CLASS_RTTI( CInteractionsManager );
	PARENT_CLASS( IGameSystem )
END_CLASS_RTTI();