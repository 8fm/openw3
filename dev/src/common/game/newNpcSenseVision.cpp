/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "newNpcSenseVision.h"

#include "aiHistory.h"
#include "interestPointComponent.h"
#include "reactionsManager.h"
#include "../engine/renderFrame.h"

CNewNPCSenseVision::~CNewNPCSenseVision()
{
}

void CNewNPCSenseVision::Initialize()
{
	CacheBoneIndex();
}

void CNewNPCSenseVision::SetSenseParams( EAISenseType senseType, CAISenseParams* senseParams )
{
	switch ( senseType )
	{
	case AIST_Absolute:
		m_absoluteParams = senseParams;
		break;
	case AIST_Vision:
		m_visionParams = senseParams;
		break;
	default:
		RED_ASSERT( false, TXT( "EAISenseType not supported: %d" ), senseType );
		break;
	}
}

CAISenseParams* CNewNPCSenseVision::GetSenseParams( EAISenseType senseType ) const
{
	switch ( senseType )
	{
	case AIST_Absolute:
		return m_absoluteParams;
	case AIST_Vision:
		return m_visionParams;
	default:
		RED_ASSERT( false, TXT( "EAISenseType not supported: %d" ), senseType );
		return nullptr;
	}
}

Float CNewNPCSenseVision::GetRangeMax() const
{
	Float range = 0.0f;
	if ( IsValid( AIST_Absolute ) )
	{
		range = m_absoluteParams->m_rangeMax;
	}
	if ( IsValid( AIST_Vision ) )
	{
		range = Max( range, m_visionParams->m_rangeMax );
	}
	return range;
}

Float CNewNPCSenseVision::GetRangeMax( EAISenseType senseType ) const
{
	if ( !IsValid( senseType ) )
	{
		return 0.0f;
	}
	if ( senseType == AIST_Absolute )
	{
		return m_absoluteParams->m_rangeMax;
	}
	if ( senseType == AIST_Vision )
	{
		return m_visionParams->m_rangeMax;
	}
	return 0.0f;
}

Bool CNewNPCSenseVision::IsValid( EAISenseType senseType ) const
{
	if ( senseType == AIST_Absolute )
	{
		return m_absoluteParams != nullptr && m_absoluteParams->m_enabled;
	}
	if ( senseType == AIST_Vision )
	{
		return m_visionParams != nullptr && m_visionParams->m_enabled;
	}
	return false;
}

String CNewNPCSenseVision::GetInfo() const
{
	String s;
	if ( IsValid( AIST_Absolute ) )
	{
		return s += String::Printf( TXT("ABSOLUTE (range: %g - %g, angle: %g, height: %g); "), m_absoluteParams->m_rangeMin, m_absoluteParams->m_rangeMax, m_absoluteParams->m_rangeAngle, m_absoluteParams->m_height );
	}
	if ( IsValid( AIST_Vision ) )
	{
		return s += String::Printf( TXT("VISION (range: %g - %g, angle: %g, height: %g); "), m_visionParams->m_rangeMin, m_visionParams->m_rangeMax, m_visionParams->m_rangeAngle, m_visionParams->m_height );
	}
	return s;
}

Bool CNewNPCSenseVision::BeginUpdate( TNoticedObjects& noticedObjects )
{
	PerFramePrecalcs();
	return true;
}

NewNPCSenseUpdateFlags CNewNPCSenseVision::EndUpdate( TNoticedObjects& noticedObjects, NewNPCSenseUpdateFlags updated, Float timeDelta )
{
	NewNPCSenseUpdateFlags changed = ProcessDelayedQueries( noticedObjects );
	changed = CNewNPCSense::EndUpdate( noticedObjects, updated | changed, timeDelta ) | changed;
	return changed;
}

NewNPCSenseUpdateFlags CNewNPCSenseVision::UpdatePlayer( TNoticedObjects& noticedObjects )
{
	CPlayer* player = GCommonGame->GetPlayer();
	if ( player == nullptr )
	{
		return FLAG_NO_CHANGES;
	}

	NewNPCSenseUpdateFlags changed = FLAG_NO_CHANGES;

	// Perform range test	
	if ( ActorTest( player ) )
	{
		RangeTestResultData rangeTestResultData = RangeTest( player );		
		if ( rangeTestResultData.m_result == RTR_Passed )
		{
			changed = OnPlayerRangeTestPassed( noticedObjects, rangeTestResultData.m_flags ) | changed;
		}
		// delayed tests have been already queued for future tests
	}

	return changed;
}

NewNPCSenseUpdateFlags CNewNPCSenseVision::UpdateNPC( CNewNPC* npc, TNoticedObjects& noticedObjects )
{
	if ( npc == nullptr )
	{
		return FLAG_NO_CHANGES;
	}

	NewNPCSenseUpdateFlags changed = FLAG_NO_CHANGES;

	if ( ActorTest( npc ) )
	{
		RangeTestResultData rangeTestResultData = RangeTest( npc );		
		if ( rangeTestResultData.m_result == RTR_Passed )
		{
			changed = OnNPCRangeTestPassed( noticedObjects, npc, rangeTestResultData.m_flags ) | changed;
		}
		// delayed tests have been already queued for future tests
	}

	return changed;
}

void CNewNPCSenseVision::CacheBoneIndex()
{
	// Reset
	m_boneIndex = -1;

	CAnimatedComponent* animated = m_npc->GetRootAnimatedComponent();
	if ( animated )
	{
		// Get skeleton data provider, it will help us find the bone by name
		const ISkeletonDataProvider* provider = animated->QuerySkeletonDataProvider();
		if ( provider )
		{
			// Find bone
			m_boneIndex = provider->FindBoneByName( TXT("head") );
			if( m_boneIndex == -1 )
			{
				// Bone not found
				WARN_GAME( TXT("CNewNPCSenseVision::CacheBoneIndex: Bone not found, actor '%ls'"), m_npc->GetName().AsChar() );
			}
		}
		else
		{
			// This is not normal not to have SkeletonData provider in animation component
			WARN_GAME( TXT("CNewNPCSenseVision::CacheBoneIndex: No skeleton data provider in AnimationComponent, actor '%ls'"), m_npc->GetName().AsChar() );
		}
	}
}

NewNPCSenseUpdateFlags CNewNPCSenseVision::OnPlayerRangeTestPassed( TNoticedObjects& noticedObjects, NewNPCNoticedObject::TFlags flags )
{
	CPlayer* player = GCommonGame->GetPlayer();
	RED_ASSERT( player != nullptr );
	Int32 objectIndex = -1;
	m_lastTimePlayerNoticed = GGame->GetEngineTime();
	NewNPCSenseUpdateFlags actorNoticed = OnActorNoticed( player, noticedObjects, flags, objectIndex );
	if ( actorNoticed )
	{
		OnPlayerNoticed( player );		
	}

	return actorNoticed;
}

NewNPCSenseUpdateFlags CNewNPCSenseVision::OnNPCRangeTestPassed( TNoticedObjects& noticedObjects, CActor* actor, NewNPCNoticedObject::TFlags flags )
{
	Int32 objectIndex = -1;
	return OnActorNoticed( actor, noticedObjects, flags, objectIndex );
}

Bool CNewNPCSenseVision::OnPlayerNoticed( CPlayer * player )
{
	CInterestPoint* ip = player->GetVisionInterestPoint();
	CReactionsManager* mgr = GCommonGame->GetReactionsManager();
	if ( ip && mgr )
	{		
		mgr->SendInterestPoint( m_npc, ip, THandle< CNode >( player ), 3.0f );
		return true;	
	}

	// No reaction
	return true;
}

NewNPCSenseUpdateFlags CNewNPCSenseVision::OnActorNoticed( CActor* actor, TNoticedObjects& noticedObjects, NewNPCNoticedObject::TFlags flags, Int32& objectIndex )
{
	RED_ASSERT( m_npc != actor );

	NewNPCNoticedObject* obj = nullptr;

	THandle< CActor > actorHandle( actor );

	Uint32 s = noticedObjects.Size();
	for ( Uint32 i = 0; i < s; i++ )
	{
		if ( noticedObjects[i].m_actorHandle == actorHandle )
		{
			obj = &noticedObjects[i];
			objectIndex = i;
		}
	}

	if ( !obj )
	{
		// create new entry
		AI_EVENT( m_npc, EAIE_Sense, EAIR_Success, TXT( "Actor noticed" ), String::Printf( TXT("NPC '%ls' - actor '%ls' noticed"), m_npc->GetName().AsChar(), actor->GetName().AsChar() ) );
		Uint32 newIdx = static_cast< Uint32 >( noticedObjects.Grow( 1 ) );
		new ( &noticedObjects[newIdx] ) NewNPCNoticedObject( actorHandle, GGame->GetEngineTime(), flags );
		objectIndex = s;
		return FLAG_NOTICED_OBJECTS_APPEARS;
	}
	else
	{
		Bool result = (obj->m_flags & flags ) == 0; // If no such flag set return true
		// update existing entry
		obj->m_lastNoticedTime = GGame->GetEngineTime();
		obj->m_flags |= flags;
		obj->UpdateLastNoticedPosition();
		obj->SetIsVisible( true );
		return result ? FLAG_VISIBILITY_FLAGS_CHANGED : FLAG_NO_CHANGES;
	}
}

CNewNPCSenseVision::RangeTestResultData CNewNPCSenseVision::RangeTest( CActor* actor ) const
{
	Bool inRange = false;
	SRangeTestData testData;
	testData.m_actorPos = actor->GetWorldPositionRef();
	testData.m_senseNearestPos = testData.m_actorPos.NearestPointOnEdge( m_npc->GetWorldPosition() , m_sensePosition );

	// First we check the absolute sense (since there could be no need to perform line of sight test).
	if ( IsValid( AIST_Absolute ) )
	{
		testData.m_senseParamsPrecalcs = &m_absoluteParamsPrecalcs;
		if ( RangeTest( testData ) )
		{
			if ( !m_absoluteParams->m_testLOS )
			{
				return RangeTestResultData( RTR_Passed, NewNPCNoticedObject::FLAG_DETECTION_ABSOLUTE );
			}
			inRange = true;
		}
	}

	// If we're here then either actor is not within absolute sense range
	// or additional line of sight test is needed.
	if ( !inRange && IsValid( AIST_Vision ) )
	{
		testData.m_senseParamsPrecalcs = &m_visionParamsPrecalcs;
		if ( RangeTest( testData ) )
		{
			if ( !m_visionParams->m_testLOS )
			{
				return RangeTestResultData( RTR_Passed, NewNPCNoticedObject::FLAG_DETECTION_VISION );
			}
			inRange = true;
		}
	}
	
	if ( !inRange )
	{
		return RangeTestResultData( RTR_Failed );
	}

	// If we're here then additional line if sight test is needed.
	// Always pass visibility test if activator is close enough (1m).
	if ( testData.m_distSqr < 1.0f )
	{
		return RangeTestResultData( RTR_Passed, NewNPCNoticedObject::FLAG_DETECTION_VISION );
	}

	// Finally, enqueue line of sight test
	Vector actorTestPos = actor->GetLOSTestPosition();
	RED_ASSERT( GCommonGame != nullptr && GCommonGame->GetNpcSensesManager() != nullptr );
	CNewNpcSensesManager* manager = GCommonGame->GetNpcSensesManager();
	VisibilityQueryId queryId = manager->SubmitQuery( m_npc, m_sensePosition, actorTestPos );
	m_delayedQueries.PushBack( SDelayedQuery( actor, queryId ) );
	return RangeTestResultData( RTR_Delayed, NewNPCNoticedObject::FLAG_DETECTION_VISION, queryId );
}

Bool CNewNPCSenseVision::RangeTest( SRangeTestData& testData ) const
{
	const SSenseParamsPrecals* precalcs = testData.m_senseParamsPrecalcs;
	RED_ASSERT( precalcs != nullptr );

	Vector diff = testData.m_actorPos - testData.m_senseNearestPos;
	Float rangeMax = precalcs->m_params->m_rangeMax;

	// first we check "lightweight" box range
	if ( Abs( diff.X ) > rangeMax || Abs( diff.Y ) > rangeMax || Abs( diff.Z ) > rangeMax )
	{
		return false;
	}

	// next, the exact range (squared)
	if ( testData.m_distSqr == 0.0f )
	{
		testData.m_distSqr = diff.SquareMag3();
	}
	if ( testData.m_distSqr < precalcs->m_rangeMinSqr || testData.m_distSqr > precalcs->m_rangeMaxSqr )
	{
		return false;
	}

	// finally, check cone angle
	if ( precalcs->m_cosHalfAngle > -0.99f )
	{
		diff.Normalize3();
		if ( Vector::Dot3( diff, m_senseDirection ) < precalcs->m_cosHalfAngle )
		{
			return false;
		}
	}

	return true;
}

Bool CNewNPCSenseVision::ActorTest( CActor* actor )
{
	if ( !actor )
	{
		return false;
	}
	if ( m_detectOnlyHostiles && m_npc->GetAttitude( actor ) != AIA_Hostile )
	{
		return false;
	}
	return actor->IsAlive();
}

void CNewNPCSenseVision::PerFramePrecalcs()
{
	Bool fromBone = false;

	// Get bone position
	if ( m_boneIndex != -1 )
	{
		CAnimatedComponent* animated = m_npc->GetRootAnimatedComponent();
		if ( animated )
		{	
			fromBone = true;
			m_sensePosition = animated->GetBoneMatrixWorldSpace( m_boneIndex ).GetTranslation();
		}
	}

	// Fallback
	if ( !fromBone )
	{
		// position
		m_sensePosition = m_npc->GetWorldPosition();
		if ( IsValid( AIST_Vision ) )
		{
			m_sensePosition.Z += m_visionParams->m_height;
		}
		else if ( IsValid( AIST_Absolute ) )
		{
			m_sensePosition.Z += m_absoluteParams->m_height;
		}
	}

	// direction
	m_senseDirection = m_npc->GetWorldForward();		

	m_detectOnlyHostiles = true;
	if ( IsValid( AIST_Absolute ) )
	{
		m_absoluteParamsPrecalcs.Set( m_absoluteParams );
		m_detectOnlyHostiles &= m_absoluteParams->m_detectOnlyHostiles;
	}
	if ( IsValid( AIST_Vision ) )
	{
		m_visionParamsPrecalcs.Set( m_visionParams );
		m_detectOnlyHostiles &= m_visionParams->m_detectOnlyHostiles;
	}
}

NewNPCSenseUpdateFlags CNewNPCSenseVision::ProcessDelayedQueries( TNoticedObjects& noticedObjects )
{
	RED_ASSERT( GCommonGame != nullptr && GCommonGame->GetNpcSensesManager() != nullptr );

    NewNPCSenseUpdateFlags changed = FLAG_NO_CHANGES;

	CNewNpcSensesManager* manager  = GCommonGame->GetNpcSensesManager();
	TDelayedQueries::iterator it = m_delayedQueries.Begin();
	while ( it != m_delayedQueries.End() )
	{
		CNewNpcSensesManager::EVisibilityQueryState queryState = manager->GetQueryState( it->m_queryId );
		// wrong job id or timed out, or result is false
		if ( queryState == CNewNpcSensesManager::QS_NotFound || queryState == CNewNpcSensesManager::QS_False )
		{
#ifndef RED_FINAL_BUILD
			SRaycastDebugData debugData;
			manager->GetQueryDebugData( it->m_queryId, debugData );
			m_raycastsDebugData.PushBack( debugData );
#endif
			it = m_delayedQueries.Erase( it );
		}
		else if ( queryState == CNewNpcSensesManager::QS_True )
		{
			CActor* actor = it->m_actor.Get();
			if ( actor != nullptr )
			{
				CNewNPC* testedNPC = Cast< CNewNPC >( actor );
				if ( ActorTest( testedNPC ) )
				{
					changed = OnNPCRangeTestPassed( noticedObjects, testedNPC, NewNPCNoticedObject::FLAG_DETECTION_VISION ) | changed;
				}

				CPlayer* player = Cast< CPlayer >( actor );
				if ( ActorTest( player ) )
				{
					changed = OnPlayerRangeTestPassed( noticedObjects, NewNPCNoticedObject::FLAG_DETECTION_VISION ) | changed;
				}
			}
#ifndef RED_FINAL_BUILD
			SRaycastDebugData debugData;
			manager->GetQueryDebugData( it->m_queryId, debugData );
			m_raycastsDebugData.PushBack( debugData );
#endif
			it = m_delayedQueries.Erase( it );
		}
		else
		{
			RED_ASSERT( queryState == CNewNpcSensesManager::QS_NotReady );
			++it;
		}
	}

#ifndef RED_FINAL_BUILD
	while ( m_raycastsDebugData.Size() > MAX_DEBUG_DATA_SIZE )
	{
		m_raycastsDebugData.PopFront();
	}
#endif

    return changed;
}

void CNewNPCSenseVision::GenerateDebugFragments( CRenderFrame* frame )
{
#ifndef RED_FINAL_BUILD
	TList< SRaycastDebugData >::iterator itEnd = m_raycastsDebugData.End();
	for ( TList< SRaycastDebugData >::iterator it = m_raycastsDebugData.Begin(); it != itEnd; ++it )
	{
		SRaycastDebugData& debugData = *it;
		Bool wasHit = debugData.m_hits.Size() > 0;
		frame->AddDebugLine( debugData.m_position, debugData.m_target, wasHit ? Color::RED : Color::GREEN, true );
		for ( Uint32 i = 0; i < debugData.m_hits.Size(); ++i )
		{
			frame->AddDebugSphere( debugData.m_hits[ i ], 0.3f, Matrix::IDENTITY, Color::RED, true );
		}
	}
#endif

	if ( frame != nullptr )
	{
		DrawDebug( frame, m_absoluteParams, AIST_Absolute );
		DrawDebug( frame, m_visionParams, AIST_Vision );
	}
}

void CNewNPCSenseVision::DrawDebug( CRenderFrame* frame, CAISenseParams* senseParams, EAISenseType senseType )
{
	if ( senseParams == nullptr || !senseParams->m_enabled )
	{
		return;
	}

	Matrix mat;
	//mat.V[3] = m_sensePosition;
	mat.V[3] = m_npc->GetWorldPosition();
	mat.V[1] = m_senseDirection;

	mat.V[2] = Vector::EZ;
	mat.V[0] = Vector::Cross( mat.V[1], mat.V[2], 0.0f );
	mat.V[1].W = 0.0f;
	mat.V[2].W = 0.0f;
	mat.V[3].W = 1.0f;

	frame->AddDebugAngledRange( mat, senseParams->m_height, senseParams->m_rangeMax, senseParams->m_rangeAngle, Color::LIGHT_GREEN, true );
	frame->AddDebugAngledRange( mat, senseParams->m_height, senseParams->m_rangeMin, senseParams->m_rangeAngle, Color::LIGHT_GREEN, true );
}
