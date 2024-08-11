/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "actionPointManager.h"
#include "communityErrorReport.h"
#include "actionPointManagerListener.h"
#include "../engine/deniedAreaComponent.h"
#include "actionPoint.h"
#include "../engine/areaComponent.h"
#include "../engine/tagManager.h"
#include "gameWorld.h"
#include "../core/gameSave.h"
#include "../engine/pathlib.h"
#include "../engine/pathlibWorld.h"
#include "../engine/gameSaveManager.h"

IMPLEMENT_ENGINE_CLASS( CActionPointManager );

Bool CActionPointManager::APFilteredIterator::m_isLocked = false;
TDynArray< CActionPointComponent* > CActionPointManager::APFilteredIterator::m_result;
Uint32 CActionPointManager::APFilteredIterator::m_currentIndex;

CActionPointManager::CActionPointManager()
	: m_listener( NULL )
{
	m_actionPointsById.Reserve( 18 * 1024 );
	m_storage.ReserveEntry( 256 * 1024 );
	m_storage.ReserveNode( 18 * 1024 );
	m_storage.ReserveNodeToEntry( 18 * 1024 );
	m_tagManager.ReserveBucket( 4 * 1024 );
}

CActionPointManager::~CActionPointManager()
{
	if ( m_listener )
	{
		m_listener->OnAPManagerDeletion();
	}
}

void CActionPointManager::RegisterActionPoint( CActionPointComponent* ap )
{
	// Collect Action Point working places

	CJobTree *jobTree = ap->GetJobTreeRes().Get();
	if ( jobTree == NULL )
	{
		WARN_GAME( TXT("Trying to register action point %s with no job tree"), ap->GetFriendlyName().AsChar() );
		return;
	}

	// Get the job tree root node

	CJobTreeNode* rootNode = jobTree->GetRootNode();
	if ( !rootNode )
	{
		WARN_GAME( TXT("Trying to register action point %s with empty job tree"), ap->GetFriendlyName().AsChar() );
		return;
	}

	// Initialize action point data

	ap->m_isActive					= ap->IsActiveOnStart();
	ap->m_safePositionCalculated	= false;
	ap->m_isOccupied				= false;
	ap->m_haveItemsReady			= true;
	ap->m_problemReports			= 0;

	// Add to by-id map

	ASSERT( ap->GetID() != ActionPointBadID );
	if ( !m_actionPointsById.Insert( ap->GetID(), ap ) )
	{
		ASSERT( false && TXT("Unique ID Error: action point with this ID already exists in the AP manager.") );
		return;
	}

#ifndef WAYPOINT_COMPONENT_NO_RUNTIME_VALIDITY
	if ( !ap->IsUsedByPathLib() || !ap->IsPositionValid() )
	{
		// this AP should not be used at all
		WARN_GAME( TXT("Ap %s has a waypoint on an invalid PE position, registering as unusable"), ap->GetFriendlyName().AsChar() );
		ap->m_apFlags |= CActionPointComponent::AP_FLAG_INVALID_WP_POS;
	}
#endif

	// Add to quad tree storage

	m_storage.Add( SActionPointComponentEntry( ap ) );

	// Add to internal tag manager

	m_tagManager.AddNode( ap, ap->GetTags() );
	m_tagManager.AddNode( ap, ap->GetEntity()->GetTags() );

	// Add to by-layer-guid map of lists

	CActionPointComponent*& layerAPList = m_actionPointsByLayerGUID.GetRef( ap->GetLayerGUID(), nullptr );
	ap->m_nextInLayer = layerAPList;
	layerAPList = ap;

	// Notify listener
	if ( m_listener )
	{
		Bool isOccupied = ( ap->m_isOccupied & CActionPointManager::REASON_COMMUNITY ) != 0;
		m_listener->UpdateAPOccupation( ap->GetID(), isOccupied );

		ap->m_isOccupied = ap->m_isOccupied & (~CActionPointManager::REASON_COMMUNITY );
		if ( isOccupied )
		{
			ap->m_isOccupied |= CActionPointManager::REASON_COMMUNITY;
		}

	}
}

void CActionPointManager::UnregisterActionPoint( const TActionPointID& id )
{
	if ( CActionPointComponent* ap = GetAP( id ) )
	{
		// Remove from by-id map

		m_actionPointsById.Erase( id );

		// Remove from quad tree

		m_storage.Remove( SActionPointComponentEntry( ap ) );

		// Remove from internal tag manager

		m_tagManager.RemoveNode( ap, ap->GetTags() );
		m_tagManager.RemoveNode( ap, ap->GetEntity()->GetTags() );

		// Remove from by-layer-guid map of lists

		auto layerAPList = m_actionPointsByLayerGUID.Find( ap->GetLayerGUID() );
		if ( layerAPList != m_actionPointsByLayerGUID.End() )
		{
			CActionPointComponent** current = &layerAPList->m_second;
			while ( *current )
			{
				if ( *current == ap )
				{
					*current = ( *current )->m_nextInLayer;
					if ( !layerAPList->m_second )
					{
						m_actionPointsByLayerGUID.Erase( layerAPList );
					}
					break;
				}

				current = &( *current )->m_nextInLayer;
			}
		}
	}
}

Bool CActionPointManager::LazyCalculateAPSafePosition( CActionPointComponent* ap ) const
{
	if ( !ap->m_safePositionCalculated )
	{
		CWorld* world = GCommonGame->GetActiveWorld();
		CPathLibWorld* pathlib = world ? world->GetPathLibWorld() : NULL;
		if ( !pathlib )
		{
			return false;
		}
		Vector safePosition = ap->GetWorldPosition();
		Bool safePosAvailable = true;
		Float defaultPersonalSpace = pathlib->GetGlobalSettings().GetCategoryPersonalSpace( 0 );
		// TODO: Take care of problem with obstacles that lies between loaded and unloaded area
		PathLib::AreaId areaId = PathLib::INVALID_AREA_ID;
		if ( !pathlib->IsLocationLoaded( safePosition.AsVector3(), areaId ) )
		{
			return false;
		}
		if ( !pathlib->TestLocation( areaId, safePosition.AsVector3(), defaultPersonalSpace, PathLib::CT_DEFAULT ) )
		{
			if ( !pathlib->FindSafeSpot( areaId, safePosition.AsVector3(), 1.f, defaultPersonalSpace, safePosition.AsVector3() ) )
			{
				safePosAvailable = false;
			}
		}

		ap->m_safePosition = safePosition;
		ap->m_safePositionCalculated = true;
		ap->m_safePositionAvailable = safePosAvailable;
	}
	return true;
}

void CActionPointManager::AttachListener( IAPMListener& listener )
{
	m_listener = &listener;
}

void CActionPointManager::DetachListener( IAPMListener& listener )
{
	m_listener = NULL;
}

Bool CActionPointManager::DoesExist( const TActionPointID& id ) const
{
	const CActionPointComponent *ap = GetAP( id );
	return ap && ap->m_isActive;
}

Bool CActionPointManager::IsFree( const TActionPointID& id, const CNewNPC* const askingNPC ) const
{
	const CActionPointComponent *ap = GetAP( id );
	return ap && ap->IsFree( askingNPC );
}

String CActionPointManager::GetFriendlyAPName( const TActionPointID& id ) const
{
	const CActionPointComponent *ap = GetAP( id );
	return ap ? ap->GetFriendlyName() : String::EMPTY;
}

Bool CActionPointManager::IsBreakable( const TActionPointID& id ) const
{
	const CActionPointComponent *ap = GetAP( id );
	return ap && ap->IsBreakable();
}

Bool CActionPointManager::IsBreakableByCutscene( const TActionPointID& id ) const
{
	const CActionPointComponent *ap = GetAP( id );
	return ap && ap->IsBreakableByCutscene();
}

EWorkPlacementImportance CActionPointManager::GetPlacementImportance( const TActionPointID& id ) const
{
	const CActionPointComponent *ap = GetAP( id );
	return ap ? ap->GetPlacementImportance() : WPI_Precise;
}

void CActionPointManager::SetFree( const TActionPointID& id, EReservationReason reason )
{
	if ( CActionPointComponent* ap = GetAP( id ) )
	{
		ap->SetFree( reason );
	}
}

void CActionPointManager::SetReserved( const TActionPointID& id, EReservationReason reason, const CNewNPC* occupyingNPC, Float reservationTimeout )
{
	if ( CActionPointComponent* ap = GetAP( id ) )
	{
		ap->SetReserved( reason, occupyingNPC, reservationTimeout );
	}
}

CGUID CActionPointManager::GetLayerGUID( const TActionPointID& id )
{
	CActionPointComponent *ap = GetAP( id );
	return ap ? ap->GetLayerGUID() : CGUID::ZERO;
}

Bool CActionPointManager::GetGoToPosition( const TActionPointID& id, Vector *position /* out */, Float *rotation /* out */ )
{
	if ( id == ActionPointBadID )
	{
		return false;
	}

	CActionPointComponent *ap = GetAP( id );
	if ( ap )
	{
		if ( position ) *position = ap->GetWorldPosition();
		if ( rotation ) *rotation = ap->GetWorldYaw();
		return true;
	}

	return false;
}

Bool CActionPointManager::GetSafePosition( const TActionPointID& id, Vector *position /* out */, Float *rotation /* out */ )
{
	if ( id == ActionPointBadID )
	{
		return false;
	}

	CActionPointComponent *ap = GetAP( id );
	if ( ap )
	{
		if ( ! LazyCalculateAPSafePosition( ap ) )
		{
			return false;
		}
		if ( position ) *position = ap->m_safePosition;
		if ( rotation ) *rotation = ap->GetWorldYaw();
		return true;
	}

	return false;
}

Bool CActionPointManager::GetActionExecutionPosition( const TActionPointID& id, Vector *position /* out */, Float *rotation /* out */ )
{
	if ( id == ActionPointBadID )
	{
		return false;
	}

	CActionPointComponent *ap = GetAP( id );
	if ( ap )
	{
		if ( position ) *position = ap->GetWorldPosition();
		if ( rotation ) *rotation = ap->GetWorldYaw();
		return true;
	}

	return false;
}

Bool CActionPointManager::CanDoAction( const TActionPointID& id, TAPCategory category ) const
{
	const CActionPointComponent *ap = GetAP( id );
	return ap && ap->m_isActive && ap->GetActionCategories().Exist( category );
}

Bool CActionPointManager::DoesMatchTags( const TActionPointID& id, const TagList &tags ) const
{
	const CActionPointComponent *ap = GetAP( id );
	return ap && ap->MatchAllTags( tags );
}

CEntity* CActionPointManager::GetOwner( const TActionPointID& id ) const
{
	const CActionPointComponent *ap = GetAP( id );
	return ap ? ap->GetEntity() : nullptr;
}

void CActionPointManager::SaveApId( IGameSaver* writer, CName valueName, const TActionPointID& apId ) const
{
	writer->WriteValue( valueName, apId );
}

TActionPointID CActionPointManager::LoadApId( IGameLoader* reader, CName valueName ) const
{
	TActionPointID id = ActionPointBadID;
	if ( reader->GetSaveVersion() > SAVE_COMMUNITY_AP_ID_BY_NAME )
	{
		reader->ReadValue( valueName, id );
	}
	else
	{
		reader->ReadValue( CName( String::Printf( TXT( "%s_apID" ), valueName.AsChar() ) ), id );
	}
	return id;
}

CInventoryComponent* CActionPointManager::GetInventory( const TActionPointID& id )
{
	CActionPointComponent* ap = GetAP( id );
	return ap ? ap->GetInventory() : nullptr;
}

void CActionPointManager::ResetItems( const TActionPointID& id )
{
	if ( CActionPointComponent* ap = GetAP( id ) )
	{
		if ( CInventoryComponent* inventory = ap->GetInventory() )
		{
			inventory->SpawnMountedItems();
		}
	}
}

void CActionPointManager::RegisterDroppedItem( const TActionPointID& id, CItemEntityProxy* itemProxy )
{
	// Add entry to the dropped items array
	SActionPointComponentDroppedItem itemInfo;
	itemInfo.m_apId = id;
	itemInfo.m_item = itemProxy;

	ASSERT( !m_droppedItems.Exist( itemInfo ) );
	m_droppedItems.PushBack( itemInfo );

	// Flag ap as not ready
	if ( CActionPointComponent** ap = m_actionPointsById.FindPtr( id ) )
	{
		( *ap )->m_haveItemsReady = false;
	}
}

//////////////////////////////////////////////////////////////////////////
// IDroppedItemClaimer implementation
Bool CActionPointManager::OnDroppedItemTimeOut( CItemEntityProxy* proxy )
{
	Bool ownershipTaken = false;
	TDynArray< SActionPointComponentDroppedItem >::iterator droppedItem = m_droppedItems.Begin();
	while ( droppedItem != m_droppedItems.End() )
	{
		if ( droppedItem->m_item == proxy )
		{
			const TActionPointID apId = droppedItem->m_apId;
			CActionPointComponent* ap = GetAP( apId );
			CInventoryComponent* inventory = nullptr;
			if ( ap && ( inventory = ap->GetInventory() ) )
			{
				const SItemUniqueId itemId = inventory->GetItemId( proxy->m_itemName );
				if ( itemId != SItemUniqueId::INVALID )
				{
					CInventoryComponent::SMountItemInfo mountInfo;
					mountInfo.m_force = true;
					mountInfo.m_proxy = proxy;
					inventory->MountItem( itemId, mountInfo );
					ownershipTaken = true;
				}
			}

			m_droppedItems.Erase( droppedItem );
			droppedItem = m_droppedItems.Begin();		

			if ( ap && !HasDroppedItems( apId ) )
			{
				ap->m_haveItemsReady = true;
			}
		}
		else
		{
			++droppedItem;
		}
	}
	return ownershipTaken;
}
//////////////////////////////////////////////////////////////////////////

Bool CActionPointManager::HasDroppedItems( const TActionPointID& id )
{
	for ( Uint32 i=0; i<m_droppedItems.Size(); ++i )
	{
		if ( m_droppedItems[i].m_apId == id )
		{
			return true;
		}
	}

	return false;
}

THandle<CJobTreeNode> CActionPointManager::GetJobTreeNode( const TActionPointID& id )
{
	const CActionPointComponent *ap = GetAP( id );
	return ap ? ap->GetJobTreeRes().Get()->GetRootNode() : nullptr;
}

THandle<CJobTree> CActionPointManager::GetJobTree( const TActionPointID& id )
{
	const CActionPointComponent *ap = GetAP( id );
	return ap ? ap->GetJobTreeRes() : nullptr;
}

void CActionPointManager::FindActionPoints( const SActionPointFilter& filter, TDynArray< CActionPointComponent* >& result )
{
	static const Float maxSphereSearchRadius = 40.0f;

	result.ClearFast();

	// By layer GUID search

	if ( !filter.m_layerGuid.IsZero() )
	{
		if ( CActionPointComponent** list = m_actionPointsByLayerGUID.FindPtr( filter.m_layerGuid ) )
		{
			CActionPointComponent* current = *list;
			while ( current )
			{
				if ( current->Filter( filter ) )
				{
					result.PushBack( current );
				}
				current = current->m_nextInLayer;
			}
		}
	}

	// Inside of the sphere search

	else if ( filter.m_sphere.GetRadius() > 0.0f &&
		    ( filter.m_actionPointTags.Empty() || filter.m_sphere.GetRadius() < maxSphereSearchRadius ) ) // Only if no tags or radius not too large
	{
		const Float radius = filter.m_sphere.GetRadius();
		Box box;
		box.Min = Vector( -radius, -radius, -radius );
		box.Max = Vector( radius, radius, radius );

		struct QuadTreeSearchFunctor : public Red::System::NonCopyable
		{
			enum { SORT_OUTPUT = false };

			QuadTreeSearchFunctor( CActionPointManager* manager, TDynArray< CActionPointComponent* >& result, const SActionPointFilter& filter )
				: m_manager( manager )
				, m_result( result )
				, m_filter( filter )
			{}
			RED_FORCE_INLINE Bool operator()( SActionPointComponentEntry element )
			{
				if ( element.m_component->Filter( m_filter ) )
				{
					m_result.PushBack( element.m_component );
				}
				return true;
			}

			CActionPointManager*					m_manager;
			TDynArray< CActionPointComponent* >&	m_result;
			const SActionPointFilter&				m_filter;
		} functor( this, result, filter );

		m_storage.TQuery( filter.m_sphere.GetCenter(), functor, box, false, nullptr, 0 );
	}

	// By tag search

	else if ( !filter.m_actionPointTags.Empty() )
	{
		struct ByTagSearchFunctor : public Red::System::NonCopyable
		{
			ByTagSearchFunctor( CActionPointManager* manager, TDynArray< CActionPointComponent* >& result, const SActionPointFilter& filter )
				: m_manager( manager )
				, m_result( result )
				, m_filter( filter )
			{}
			RED_INLINE Bool EarlyTest( CNode* node ) { return true; }
			RED_INLINE void Process( CNode* node, Bool isGuaranteedUnique )
			{
				CActionPointComponent* ap = static_cast< CActionPointComponent* >( node );
				if ( ap->Filter( m_filter ) )
				{
					if ( isGuaranteedUnique )
					{
						m_result.PushBack( ap );
					}
					else
					{
						m_result.PushBackUnique( ap );
					}
				}
			}

			CActionPointManager*					m_manager;
			TDynArray< CActionPointComponent* >&	m_result;
			const SActionPointFilter&				m_filter;
		} functor( this, result, filter );

		m_tagManager.IterateTaggedNodes( filter.m_actionPointTags, functor, filter.m_matchAll ? BCTO_MatchAll : BCTO_MatchAny );
	}

	// Search through all entries

	else
	{
		for ( auto& entry : m_actionPointsById )
		{
			if ( entry.m_second->Filter( filter ) )
			{
				result.PushBack( entry.m_second );
			}
		}
	}
}

EFindAPResult CActionPointManager::FindActionPoint( TActionPointID& apFoundID /* out */, const SActionPointFilter& filter )
{
	TActionPointID lastApID = apFoundID; // remember last AP ID, so we will not find the same one again (if possible)
	apFoundID = ActionPointBadID;

	// Find all AP components that match actionCategory and are on loaded layer layerInfo
	APFilteredIterator it( *this, filter );

	Uint32 apFoundCount = 0;
	Bool prevFoundAPIsLastAP = false;

	while ( it )
	{
		const CActionPointComponent* ap = *it;

		++apFoundCount;

		if ( prevFoundAPIsLastAP || GEngine->GetRandomNumberGenerator().Get< Uint32 >( apFoundCount ) == 0 )
		{
			prevFoundAPIsLastAP = (lastApID == ap->GetID());
			apFoundID = ap->GetID();
		}
		++it;
	}

	return apFoundCount > 0 ? FAPR_Success : FAPR_NoCandidates;
}

Int32 CActionPointManager::GetActionPoints( const SActionPointFilter& filter, TDynArray< TActionPointID >& actionPointIdArray, TDynArray< CActionPointComponent* >* actionPointArray )
{
	// Find action points

	TDynArray< CActionPointComponent* > temp;
	TDynArray< CActionPointComponent* >& output = actionPointArray ? *actionPointArray : temp;
	FindActionPoints( filter, output );

	// Copy IDs to ID array

	actionPointIdArray.Reserve( output.Size() );
	for ( CActionPointComponent* ap : output )
	{
		actionPointIdArray.PushBack( ap->GetID() );
	}

	return output.Size();
}

Bool CActionPointManager::HasPreferredNextAPs( const TActionPointID& apID ) const
{
	const CActionPointComponent *ap = GetAP( apID );
	return ap ? !ap->GetPreferredNextAPsTagList().Empty() : false;
}

TActionPointID CActionPointManager::FindNextFreeActionPointInSequence( const TActionPointID& apID )
{
	CActionPointComponent* ap = GetAP( apID );
	if ( !ap ) 
	{
		return ActionPointBadID;
	}

	// Empty preferred next AP
	if ( ap->GetPreferredNextAPsTagList().Empty() ) 
	{
		return ActionPointBadID;
	}

	SActionPointFilter apFilter;
	apFilter.m_onlyFree = true;
	apFilter.m_actionPointTags = ap->GetPreferredNextAPsTagList();
	apFilter.m_layerGuid = ap->GetLayerGUID();

	TActionPointID result = ActionPointBadID;	
	if ( FindActionPoint( result, apFilter ) == FAPR_Success )
	{
		return result;
	}
	else
	{
		return ActionPointBadID;
	}
}

void CActionPointManager::ReportActionPointCausingProblems( const TActionPointID& apId )
{
	if ( CActionPointComponent** ap = m_actionPointsById.FindPtr( apId ) )
	{
		++( *ap )->m_problemReports;
	}
}

Bool CActionPointManager::GetDebugInfo( const TActionPointID& apId, Uint32 &problemReportsCount /* out */, Bool &flagInvalidWpPos /* out */, Bool &flagMissingWp /* out */ ) const
{
	if ( const CActionPointComponent* ap = GetAP( apId ) )
	{
		problemReportsCount = ap->m_problemReports;
		flagInvalidWpPos = (ap->m_apFlags & CActionPointComponent::AP_FLAG_INVALID_WP_POS) == 0 ? false : true;
		flagMissingWp = (ap->m_apFlags & CActionPointComponent::AP_FLAG_MISSING_WP) == 0 ? false : true;
		return true;
	}

	return false;
}

CActionPointComponent* CActionPointManager::GetAP( const TActionPointID& apID )
{
	CActionPointComponent** ap = apID != ActionPointBadID ? m_actionPointsById.FindPtr( apID ) : nullptr;
	return ap ? *ap : nullptr;
}

const CActionPointComponent* CActionPointManager::GetAP( const TActionPointID& apID ) const
{
	const CActionPointComponent* const* ap = apID != ActionPointBadID ? m_actionPointsById.FindPtr( apID ) : nullptr;
	return ap ? *ap : nullptr;
}

void CActionPointManager::ShutdownManager()
{
	RemoveAllActionPoints();
}

void CActionPointManager::RemoveAllActionPoints()
{
	m_storage.ClearFast();
	m_actionPointsById.ClearFast();
	m_droppedItems.Clear();
}

//////////////////////////////////////////////////////////////////////////

void CActionPointManager::funcHasPreferredNextAPs( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( TActionPointID, apID, ActionPointBadID );
	FINISH_PARAMETERS;

	RETURN_BOOL( HasPreferredNextAPs( apID ) );
}

void CActionPointManager::funcGetSeqNextActionPoint( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( TActionPointID, currApID, ActionPointBadID );
	FINISH_PARAMETERS;

	TActionPointID apInSeqID = FindNextFreeActionPointInSequence( currApID );

	RETURN_STRUCT( TActionPointID, apInSeqID );
}

void CActionPointManager::funcGetJobTree( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( TActionPointID, apID, ActionPointBadID );
	FINISH_PARAMETERS;

	RETURN_OBJECT( GetJobTree( apID ).Get() );
}

void CActionPointManager::funcResetItems( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( TActionPointID, apID, ActionPointBadID );
	FINISH_PARAMETERS;

	ResetItems( apID );
}

void CActionPointManager::funcGetGoToPosition( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( TActionPointID, apID, ActionPointBadID );
	GET_PARAMETER_REF( Vector, placePosOut, Vector(0,0,0) );
	GET_PARAMETER_REF( Float, placeRotOut, 0 );
	FINISH_PARAMETERS;

	RETURN_BOOL( GetGoToPosition( apID, &placePosOut, &placeRotOut ) );
}

void CActionPointManager::funcGetActionExecutionPosition( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( TActionPointID, apID, ActionPointBadID );
	GET_PARAMETER_REF( Vector, placePosOut, Vector(0,0,0) );
	GET_PARAMETER_REF( Float, placeRotOut, 0 );
	FINISH_PARAMETERS;

	RETURN_BOOL( GetActionExecutionPosition( apID, &placePosOut, &placeRotOut ) );
}

//! Gets position at which the job should be executed. Output pointer parameters can be NULL
Bool GetActionExecutionPosition( TActionPointID id, Vector *position /* out */, Float *rotation /* out */ );

void CActionPointManager::funcGetFriendlyAPName( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( TActionPointID, apID, ActionPointBadID );
	FINISH_PARAMETERS;

	RETURN_STRING( GetFriendlyAPName( apID ) );
}

void CActionPointManager::funcIsBreakable( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( TActionPointID, apID, ActionPointBadID );
	FINISH_PARAMETERS;

	RETURN_BOOL( IsBreakable( apID ) );
}

void CActionPointManager::funcGetPlacementImportance( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( TActionPointID, apID, ActionPointBadID );
	FINISH_PARAMETERS;

	RETURN_INT( GetPlacementImportance( apID ) );
}

void CActionPointManager::funcIsFireSourceDependent( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( TActionPointID, apID, ActionPointBadID );
	FINISH_PARAMETERS;

	Bool value = false;

	CActionPointComponent *ap = GetAP( apID );
	if ( ap )
		value = ap->GetFireSourceDependent();
		
	RETURN_BOOL( value );
}

void funcIsAPValid( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( TActionPointID, apID, ActionPointBadID );
	FINISH_PARAMETERS;

	Bool isValid = ( apID != ActionPointBadID );
	RETURN_BOOL( isValid );
}

void funcClearAPID( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( TActionPointID, apID, ActionPointBadID );
	FINISH_PARAMETERS;

	apID = ActionPointBadID;
}

#define APMGR_SCRIPT( x )	\
	NATIVE_GLOBAL_FUNCTION( #x, func##x );

void RegisterActionPointManagerScriptFunctions()
{
	APMGR_SCRIPT( ClearAPID );
	APMGR_SCRIPT( IsAPValid );
}
