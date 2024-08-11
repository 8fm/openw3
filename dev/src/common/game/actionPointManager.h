/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "../engine/tagManager.h"
#include "../engine/layer.h"
#include "../game/itemEntity.h"		// PAKSAS TODO: dependencies inversion

#include "communityFindApResult.h"
#include "communityData.h"
#include "actionPointDataDef.h"
#include "binaryStorage.h"
#include "actionPointComponent.h"
#include "jobTree.h"

class CActionPointComponent;
class CJobTreeNode;
class CJobTree;
class CInventoryComponent;
class IAPMListener;
enum EWorkPlacementImportance : CEnum::TValueType;
const Uint32 MAX_FAUL_AP_REPORTS = 100;

///////////////////////////////////////////////////////////////////////////////

class CActionPointManager : public CObject, public IDroppedItemClaimer
{
	DECLARE_ENGINE_CLASS( CActionPointManager, CObject, 0 );

	friend class CCommunityDebugger;
private:
	struct SActionPointComponentDroppedItem
	{
		TActionPointID		m_apId;
		CItemEntityProxy*	m_item;

		Bool operator==( const SActionPointComponentDroppedItem& rhs )
		{
			return m_apId == rhs.m_apId && m_item == rhs.m_item;
		}

		friend Bool operator==( const SActionPointComponentDroppedItem& lhs, const SActionPointComponentDroppedItem& rhs )
		{
			return lhs.m_apId == rhs.m_apId && lhs.m_item == rhs.m_item;
		}
	};

	IAPMListener*										m_listener;

public:
	enum EReservationReason : Uint8
	{
		REASON_SELECTION				= FLAG( 0 ),
		REASON_PERFORMS_WORK			= FLAG( 1 ),
		REASON_COMMUNITY				= FLAG( 2 ),
		REASON_RESERVATION				= FLAG( 3 ),
		REASON_SPAWNING					= FLAG( 4 )
	};


	CActionPointManager();
	~CActionPointManager();

	//! Used when game ends
	void ShutdownManager();

	//! Creates AP based on AP component and returns ID for that AP
	void RegisterActionPoint( CActionPointComponent* apComp );

	//! Remove action point from the manager
	void UnregisterActionPoint( const TActionPointID& id );

	//! Attaches an action point listener
	void AttachListener( IAPMListener& listener );

	//! Detaches an action point listener
	void DetachListener( IAPMListener& listener );

	//! Returns 'true' if the action point with ID 'id' exists in the AP manager
	Bool DoesExist( const TActionPointID& id ) const;

	//! Returns true if AP with ID 'id' is free
	Bool IsFree( const TActionPointID& id, const CNewNPC* const askingNPC = nullptr ) const;

	//! Returns action point friendly name
	String GetFriendlyAPName( const TActionPointID& id ) const;

	//! Returns true if work in action point with ID 'id' can be interrupted
	Bool IsBreakable( const TActionPointID& id ) const;

	//! Returns true if work in action point with ID 'id' can be interrupted by cutscene
	Bool IsBreakableByCutscene( const TActionPointID& id ) const;

	//! Where can an NPC execute this action with respect to the AP position
	EWorkPlacementImportance GetPlacementImportance( const TActionPointID& id ) const;

	//! Sets AP with ID 'id' as free
	void SetFree( const TActionPointID& id, EReservationReason reason );

	//! Sets AP with ID 'id' as reserved (occupied)
	void SetReserved( const TActionPointID& id, EReservationReason reason, const CNewNPC* occupyingNPC = nullptr, Float reservationTimeout = 0.f );

	//! Returns GUID of the layer on which action point lies
	CGUID GetLayerGUID( const TActionPointID& id );

	//! Gets position, path engine position and rotation of the action point. Output pointer parameters can be NULL
	Bool GetGoToPosition( const TActionPointID& id, Vector *position /* out */, Float *rotation /* out */ );

	//! Gets safe position
	Bool GetSafePosition( const TActionPointID& id, Vector *position /* out */, Float *rotation /* out */ );

	//! Gets position at which the job should be executed. Output pointer parameters can be NULL
	Bool GetActionExecutionPosition( const TActionPointID& id, Vector *position /* out */, Float *rotation /* out */ );

	//! Returns 'true' if there is AP with ID 'id' in the manager and it supports category 'category'
	Bool CanDoAction( const TActionPointID& id, TAPCategory category ) const;

	//! Returns 'true' if there is AP with ID 'id' in the manager and it has tags 'tags'
	Bool DoesMatchTags( const TActionPointID& id, const TagList &tags ) const;

	//! Returns action point component owner entity
	CEntity* GetOwner( const TActionPointID& id ) const;

	//////////////////////////////////////////////////////////////////////////
	//! Saves an action point ID
	void SaveApId( IGameSaver* writer, CName valueName, const TActionPointID& apId ) const;

	//! Restores an action point ID
	TActionPointID LoadApId( IGameLoader* reader, CName valueName ) const;

	//////////////////////////////////////////////////////////////////////////
	//! Returns inventory for the AP (can return NULL)
	CInventoryComponent* GetInventory( const TActionPointID& id );

	//! Resets inventory items for the AP
	void ResetItems( const TActionPointID& id );

	//! Register action point dropped item
	void RegisterDroppedItem( const TActionPointID& id, CItemEntityProxy* itemProxy );

	//! Item entity dropped on the floor timer finished
	virtual Bool OnDroppedItemTimeOut( CItemEntityProxy* proxy );

	//! Check if there are any dropped items coming from given ap
	Bool HasDroppedItems( const TActionPointID& id );
	//////////////////////////////////////////////////////////////////////////

	//! Returns job tree node for the AP (can return NULL)
	THandle<CJobTreeNode> GetJobTreeNode( const TActionPointID& id );

	//! Returns job tree for the ap (can return NULL)
	THandle<CJobTree> GetJobTree( const TActionPointID& id );

	//! Finds random free AP that matches specified criteria and writes found AP ID to the 'apFoundID' argument
	EFindAPResult FindActionPoint( TActionPointID &apFoundID /* out */, const SActionPointFilter& filter );
	//! Finds all action points matching criteria
	void FindActionPoints( const SActionPointFilter& filter, TDynArray< CActionPointComponent* >& result );

	//! Collects all action points with specified criteria, returns the number of APs found
	Int32 GetActionPoints( const SActionPointFilter& filter, TDynArray< TActionPointID > &actionPointIdArray /* out */, TDynArray< CActionPointComponent* >* actionPointArray = nullptr /* out */);

	//! Returns 'true' if action point has preferred next action points
	Bool HasPreferredNextAPs( const TActionPointID& apID ) const;

	//! Finds random free AP on which NPC should work after working on AP with ID 'apID'
	TActionPointID FindNextFreeActionPointInSequence( const TActionPointID& apID );

	//////////////////////////////////////////////////////////////////////////
	// Debug stuff

	//! Increment problems counter fo r given ap, after exceeding MAX_FAUL_AP_REPORTS, it will be disabled
	void ReportActionPointCausingProblems( const TActionPointID& apId );

	Bool GetDebugInfo( const TActionPointID& apId, Uint32 &problemReportsCount /* out */, Bool &flagInvalidWpPos /* out */, Bool &flagMissingWp /* out */ ) const;

	//! Returns pointer to the Action Point by ID or NULL if none has been found
	CActionPointComponent* GetAP( const TActionPointID& apID );
	const CActionPointComponent* GetAP( const TActionPointID& apID ) const;

public:
	struct APFilteredIterator
	{
	private:
		static Bool									m_isLocked;			// Global iterator lock (can only be used in one place at a time to optimize m_result memory)
		static TDynArray< CActionPointComponent* >	m_result;			// Note: constructor verifies we're on main thread
		static Uint32								m_currentIndex;
	public:
		APFilteredIterator( CActionPointManager& apMan, const SActionPointFilter& filter )
		{
			ASSERT( SIsMainThread() );
			RED_FATAL_ASSERT( !m_isLocked, "Attempt to use APFilteredIterator while it's locked (i.e. used from another place)" );
			m_isLocked = true;
			m_currentIndex = 0;
			apMan.FindActionPoints( filter, m_result );
		}

		RED_FORCE_INLINE ~APFilteredIterator()				{ m_isLocked = false; }
		RED_FORCE_INLINE operator Bool() const				{ return m_currentIndex < m_result.Size(); }
		RED_FORCE_INLINE CActionPointComponent* operator*()	{ return m_result[ m_currentIndex ]; }
		RED_FORCE_INLINE APFilteredIterator& operator++()	{ ++m_currentIndex; return *this; }
	};

private:
	//! Destroys and removes all action points holden by this AP manager
	void RemoveAllActionPoints();

	//! Calculate Action Point go to safe position (if not calculated)
	Bool LazyCalculateAPSafePosition( CActionPointComponent* ap ) const;

private:
	struct SActionPointComponentEntry
	{
		CActionPointComponent* m_component;

		RED_FORCE_INLINE SActionPointComponentEntry( CActionPointComponent* component ) : m_component( component ) {}
		RED_FORCE_INLINE CActionPointComponent* Get() const { return m_component; }
	};

	THashMap< TActionPointID, CActionPointComponent* >						m_actionPointsById;		// Action points by ID

	THashMap< CGUID, CActionPointComponent* >								m_actionPointsByLayerGUID; // Action points list by layer GUID

	CQuadTreeStorage< CActionPointComponent, SActionPointComponentEntry >	m_storage;				// Spatial storage of the action points

	CTagManager																m_tagManager;			// Tag based storage of all action points

	TDynArray< SActionPointComponentDroppedItem >							m_droppedItems;			// Items from this ap that were dropped and should be restored soon

private:
	void funcHasPreferredNextAPs( CScriptStackFrame& stack, void* result );
	void funcGetSeqNextActionPoint( CScriptStackFrame& stack, void* result );
	void funcGetJobTree( CScriptStackFrame& stack, void* result );
	void funcResetItems( CScriptStackFrame& stack, void* result );
	void funcGetGoToPosition( CScriptStackFrame& stack, void* result );
	void funcGetActionExecutionPosition( CScriptStackFrame& stack, void* result );
	void funcGetFriendlyAPName( CScriptStackFrame& stack, void* result );
	void funcIsBreakable( CScriptStackFrame& stack, void* result );
	void funcGetPlacementImportance( CScriptStackFrame& stack, void* result );
	void funcIsFireSourceDependent( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CActionPointManager );
	PARENT_CLASS( CObject );
	NATIVE_FUNCTION( "HasPreferredNextAPs", funcHasPreferredNextAPs );
	NATIVE_FUNCTION( "GetSeqNextActionPoint", funcGetSeqNextActionPoint );
	NATIVE_FUNCTION( "GetJobTree", funcGetJobTree );
	NATIVE_FUNCTION( "ResetItems", funcResetItems );
	NATIVE_FUNCTION( "GetGoToPosition", funcGetGoToPosition);
	NATIVE_FUNCTION( "GetActionExecutionPosition", funcGetActionExecutionPosition);
	NATIVE_FUNCTION( "GetFriendlyAPName", funcGetFriendlyAPName );
	NATIVE_FUNCTION( "IsBreakable", funcIsBreakable );
	NATIVE_FUNCTION( "GetPlacementImportance", funcGetPlacementImportance );
	NATIVE_FUNCTION( "IsFireSourceDependent", funcIsFireSourceDependent );
END_CLASS_RTTI();
