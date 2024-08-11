/**
 * Copyright © 2009 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "communityData.h"
#include "actionPointManager.h"
#include "wayPointComponent.h"
#include "jobTree.h"
#include "2daProperties.h"
#include "aiParameters.h"

/// Action point work placement
enum EWorkPlacementImportance : CEnum::TValueType
{
	WPI_Anywhere,
	WPI_Nearby,
	WPI_Precise
};

BEGIN_ENUM_RTTI( EWorkPlacementImportance );
ENUM_OPTION( WPI_Anywhere );
ENUM_OPTION( WPI_Nearby );
ENUM_OPTION( WPI_Precise );
END_ENUM_RTTI();

struct SActionPointFilter
{
	TAPCategory			m_category;			// TODO: deprecate
	CGUID				m_layerGuid;
	TagList				m_actionPointTags;	
	Bool				m_onlyFree;
	Sphere				m_sphere;
	Bool				m_matchAll;
	const CActor* 		m_askingNPC;

	SActionPointFilter()
		: m_onlyFree( false )
		, m_matchAll( true )
		, m_askingNPC( nullptr )
	{}
};

/// An action point
class CActionPointComponent : public CWayPointComponent
#ifndef NO_EDITOR
	, public CActionPointCategories2dPropertyOwner
#endif
{
	DECLARE_ENGINE_CLASS( CActionPointComponent, CWayPointComponent, 0 );

public:
	friend class CNPCSpawnSetComponent; // for debug purposes only
	friend class CCommunityDebugger; // for debug purposes only
	friend class CActionPointManager;

	static const Uint8		AP_FLAG_INVALID_WP_POS	= FLAG( 0 );	//<! Action point has wp on invalid pe location
	static const Uint8		AP_FLAG_MISSING_WP		= FLAG( 1 );	//<! Action point is missing wp considering job tree place definition

public:
	static const Float MAXIMUM_Z_DISPLACEMENT;
	static const Float PRECISE_PLACEMENT_MARGIN;
	static const Float PRECISE_ANGLE_MARGIN;

protected:
	CInventoryComponent *m_inventoryComponent;

protected:
	
	TDynArray< IPerformableAction* > m_eventWorkStarted;
	TDynArray< IPerformableAction* > m_eventWorkEnded;
	THandle< CJobTree >				m_jobTreeRes;				  //!< Job tree resource
	TagList							m_preferredNextAPs;			//!< Preferred category of AP after this one
	CAIPerformCustomWorkTree*		m_customWorkTree;
	TActionPointID					m_apID;
	EWorkPlacementImportance		m_placementImportance;		//!< Placement importance	
	Bool							m_breakableByCutscene;		  //!< Can this ap's jobs be stopped by cutscene played in area
	Bool							m_actionBreakable;			//!< Is action breakable
	Bool							m_activateOnStart;			//!< if component is actice on start
	Bool							m_ignoreCollosions;
	Bool							m_disableSoftReactions;	
	Bool							m_isEnabled;				  //!< Is the action point enabled ?	
	Bool							m_fireSourceDependent;		//!< If true, then any nearby fire source is relit upon being extinguished

	Bool							m_safePositionCalculated;	//!< Was safe position lazy calculated
	Bool							m_safePositionAvailable;	//!< Is safe position available
	mutable Uint8					m_isOccupied;				//!< Is some NPC working on that action point
	Bool							m_haveItemsReady;			//!< Does this ap have all items in place?
	Bool							m_isActive;					//!< Can AP be used
	Int8							m_apFlags;					//!< Action point internal flags
	Vector							m_safePosition;
	Uint32							m_problemReports;			//!< Number of times this action point was reported to cause problems
	CGUID							m_communityOwnerGUID;		//!< ID of owner of community definition, e.g. for house templates (maybe groupId can play the same role)
	mutable THandle< CNewNPC >		m_occupyingNPC;
	CActionPointComponent*			m_nextInLayer;				//!< Next element in layer with the same GUID
	mutable EngineTime				m_reservationTimeout;
	Bool							m_forceKeepIKactive;		

	// Display debug info in editor
	virtual void WaypointGenerateEditorFragments( CRenderFrame* frame ) override;

public:

#ifndef NO_EDITOR_ENTITY_VALIDATION
	// Called when entity is being validated
	virtual Bool OnValidate( TDynArray< String >& log ) const;
#endif

	RED_INLINE Bool IsActiveOnStart() const { return m_activateOnStart; }

	// Get attachment group for this component - it determines the order
	virtual EAttachmentGroup GetAttachGroup() const;

	// Get the job tree
	RED_INLINE THandle< CJobTree > GetJobTreeRes() const { return m_jobTreeRes; }

	//! Gets unique ID
	RED_INLINE const TActionPointID& GetID() const { return m_apID; }

	//! Is the action point enabled ?
	RED_INLINE Bool IsEnabled() const { return m_isEnabled; }

	// check if this ap's actions can be stopped by other systems
	RED_INLINE Bool IsBreakableByCutscene() const { return m_breakableByCutscene; }

	//! Get inventory component in this ap's entity
	RED_INLINE CInventoryComponent* GetInventory() { return m_inventoryComponent; }

	RED_INLINE CAIPerformCustomWorkTree* GetCustomWorkTree(){ return m_customWorkTree; }

	RED_INLINE Bool GetIgnoreCollisions() const { return m_ignoreCollosions; }
	RED_INLINE Bool GetDisableSoftRactions() const { return m_disableSoftReactions; }		
	RED_INLINE Bool GetFireSourceDependent() const { return m_fireSourceDependent; }

	Bool Filter( const SActionPointFilter& filter )const;

	RED_FORCE_INLINE Bool MatchAnyTag( const TagList& tags ) const { return TagList::MatchAny( tags, GetTags() ) || TagList::MatchAny( tags, GetEntity()->GetTags() ); }
	Bool MatchAllTags( const TagList& tags ) const;
	RED_FORCE_INLINE const CGUID& GetLayerGUID() const { return GetLayer()->GetGUID(); }
	RED_FORCE_INLINE const Vector& GetActionExecutionPosition() const { return GetWorldPositionRef(); }
	RED_FORCE_INLINE const Vector& GetGoToPosition() const { return GetWorldPositionRef(); }	
	RED_FORCE_INLINE void SetFree( Uint8 reason ) { ASSERT( m_isOccupied & reason, TXT("Action point that was not used was freed") ); m_isOccupied &= ~reason; }
	
	Bool IsFree( const CActor* const askingNPC = nullptr ) const;
	void SetReserved( Uint8 reason, const CNewNPC* occupyingNPC = nullptr, Float reservationTime = 0.f );
	void SetActive( Bool active );

	Bool	KeepIKActive() const { return m_forceKeepIKactive || m_jobTreeRes->KeepIKActive(); }

protected:
	// Get sprite icon
	virtual CBitmapTexture* GetSpriteIcon() const;

public:
	//! Check if this action point can perform any of given action category
	Bool CanDoAction( const TDynArray< TAPCategory > &actionCategories ) const;

	// check if this ap's actions can be stopped by other systems
	Bool IsBreakable() const;

	//! Where can an NPC execute this action with respect to the AP position
	EWorkPlacementImportance GetPlacementImportance() const;

	//! Get categories of actions related to this action point
	const TDynArray< TAPCategory > &GetActionCategories() const;

	//! Can this action point perform given action
	Bool CanDoAction( const TAPCategory &actionCategory ) const;

	//! Returns random action category name on which this AP can work
	TAPCategory GetRandomActionCategory() const;

	//! Reset items if any
	void ResetItems();

	// Get the tags of preferred next action point
	const TagList& GetPreferredNextAPsTagList() const;

	//! Does this AP has preferred action point ?
	Bool HasPreferredNextAPs() const;

	void WorkStarted();

	void WorkEnded();

protected:
	CActionPointComponent();

	// Component was attached to world
	virtual void OnAttached( CWorld* world );

	// Component was detached from world
	virtual void OnDetached( CWorld *world );

	// Toggle action point
	virtual void SetEnabled( Bool enabled );

#ifndef NO_DATA_VALIDATION
	// Check data, can be called either for node on the level or for node in the template
	virtual void OnCheckDataErrors( Bool isInTemplate ) const;
#endif

	virtual Color CalcSpriteColor() const;

	Bool IsApPlacementValid( Vector* validPlacement = NULL ) const;

private:
	//! Attach to the AP manager
	void ConditionalAttachToAPManager( CWorld* world );

private:
	void funcGetID( CScriptStackFrame& stack, void* result );
	const CResource* GetActionPointResource() const;
};

BEGIN_CLASS_RTTI( CActionPointComponent );
	PARENT_CLASS( CWayPointComponent );
	PROPERTY_EDIT( m_isEnabled			, TXT("Is action point enabled?") );
	PROPERTY_EDIT( m_jobTreeRes			, TXT("Job tree resource") );
	PROPERTY_EDIT( m_actionBreakable	, TXT("Is action breakable") );	
	PROPERTY_EDIT( m_breakableByCutscene, TXT("Can be stopped by other systems (scenes, etc.)") );
	PROPERTY_EDIT( m_preferredNextAPs	, TXT("Preferred next action points") );
	PROPERTY_EDIT( m_activateOnStart	, TXT("Is active on start") );	
	PROPERTY_EDIT( m_placementImportance, TXT("Where can an NPC execute this action with respect to the AP position") );	
	PROPERTY_EDIT( m_ignoreCollosions	, TXT("Ignore collisions") );
	PROPERTY_EDIT( m_disableSoftReactions, TXT("Disable soft reactions") );		
	PROPERTY_EDIT( m_fireSourceDependent, TXT("JobTree depends on a constantly lit fire-source") );
	PROPERTY_EDIT( m_forceKeepIKactive,	  TXT("Keep IK active during work") );
	PROPERTY_INLINED( m_customWorkTree	, TXT("AI tree to perform custom work") );		
	PROPERTY_INLINED( m_eventWorkStarted, TXT("Actions performed whed work was started") );
	PROPERTY_INLINED( m_eventWorkEnded	, TXT("Actions performed whed work was ended") );
	NATIVE_FUNCTION( "GetID", funcGetID );
END_CLASS_RTTI();
