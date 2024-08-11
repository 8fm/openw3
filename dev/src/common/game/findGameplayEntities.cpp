/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "findGameplayEntities.h"
#include "gameplayStorageAcceptors.h"

//////////////////////////////////////////////////////////////////////////
// stuff in this file was moved from gameplayStorage.cpp
//////////////////////////////////////////////////////////////////////////

using namespace FindGameplayEntities;

void funcFindGameplayEntitiesInRange( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	ASSERT( GGame->IsActive() );

	GET_PARAMETER_REF( TDynArray< THandle< CGameplayEntity > >, output, TDynArray< THandle< CGameplayEntity > > () );
	GET_PARAMETER( THandle< CNode >, centerNode, nullptr );
	GET_PARAMETER( Float, range, 15.f );
	GET_PARAMETER( Int32, maxResults, NumericLimits< Int32 >::Max() );
	GET_PARAMETER_OPT( CName, tag, CName::NONE );
	GET_PARAMETER_OPT( Uint32, flags, 0 );
	GET_PARAMETER_OPT( THandle< CGameplayEntity >, target, nullptr );
	GET_PARAMETER_OPT( CName, className, CName::NONE );
	FINISH_PARAMETERS;

	CNode* originNode = centerNode.Get();
	if ( originNode == nullptr )
	{
		originNode = GCommonGame->GetPlayer();
		if ( originNode == nullptr )
		{
			return;
		}
	}

	CGameplayStorage::SSearchParams params;
	params.m_origin = originNode->GetWorldPositionRef();
	params.m_range = range;
	params.m_flags = flags;
	params.m_maxResults = maxResults;
	params.m_target = target.Get();
	params.m_tag = tag;
	if ( className != CName::NONE )
	{
		params.m_class = SRTTI::GetInstance().FindClass( className );
	}

	Bool useNodeAsOrigin = true;
	if ( flags & FLAGMASK_OnlyActors && !originNode->IsA< CActor >() )
	{
		useNodeAsOrigin = false;
	}
	else if ( !originNode->IsA< CGameplayEntity >() )
	{
		useNodeAsOrigin = false;
	}

	GameplayStorageAcceptors::DefaultAcceptor acceptor;
	if ( !useNodeAsOrigin )
	{
		FindGameplayEntitiesInRange< const Vector >( output, originNode->GetWorldPositionRef(), params, acceptor );
	}
	else
	{
		FindGameplayEntitiesInRange< CGameplayEntity >( output, static_cast< CGameplayEntity& >( *originNode ), params, acceptor );
	}

}

void funcFindGameplayEntitiesInSphere( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	ASSERT( GGame->IsActive() );

	GET_PARAMETER_REF( TDynArray< THandle< CGameplayEntity > >, output, TDynArray< THandle< CGameplayEntity > > () );
	GET_PARAMETER( Vector, point, Vector::ZEROS );
	GET_PARAMETER( Float, range, 15.f );
	GET_PARAMETER( Int32, maxResults, NumericLimits< Int32 >::Max() );
	GET_PARAMETER_OPT( CName, tag, CName::NONE );
	GET_PARAMETER_OPT( Uint32, flags, 0 );
	GET_PARAMETER_OPT( THandle< CGameplayEntity >, target, nullptr );
	GET_PARAMETER_OPT( CName, className, CName::NONE );
	FINISH_PARAMETERS;

	CGameplayStorage::SSearchParams params;
	params.m_origin = point;
	params.m_range = range;
	params.m_flags = flags;
	params.m_maxResults = maxResults;
	params.m_target = target.Get();
	params.m_tag = tag;
	if ( className != CName::NONE )
	{
		params.m_class = SRTTI::GetInstance().FindClass( className );
	}

	GameplayStorageAcceptors::SphereAcceptor acceptor( point, range );
	FindGameplayEntitiesInRange< const Vector >( output, point, params, acceptor );
}

void funcFindGameplayEntitiesInCylinder( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	ASSERT( GGame->IsActive() );

	GET_PARAMETER_REF( TDynArray< THandle< CGameplayEntity > >, output, TDynArray< THandle< CGameplayEntity > > () );
	GET_PARAMETER( Vector, point, Vector::ZEROS );
	GET_PARAMETER( Float, range, 15.f );
	GET_PARAMETER( Float, height, 15.f );
	GET_PARAMETER( Int32, maxResults, NumericLimits< Int32 >::Max() );
	GET_PARAMETER_OPT( CName, tag, CName::NONE );
	GET_PARAMETER_OPT( Uint32, flags, 0 );
	GET_PARAMETER_OPT( THandle< CGameplayEntity >, target, nullptr );
	GET_PARAMETER_OPT( CName, className, CName::NONE );
	FINISH_PARAMETERS;

	CGameplayStorage::SSearchParams params;
	params.m_origin = point;
	params.m_range = Max( range, height );
	params.m_flags = flags;
	params.m_maxResults = maxResults;
	params.m_target = target.Get();
	params.m_tag = tag;
	if ( className != CName::NONE )
	{
		params.m_class = SRTTI::GetInstance().FindClass( className );
	}

	GameplayStorageAcceptors::CylinderAcceptor acceptor( point, range, height );
	FindGameplayEntitiesInRange< const Vector >( output, point, params, acceptor );
}

void funcFindGameplayEntitiesInCone( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	ASSERT( GGame->IsActive() );

	GET_PARAMETER_REF( TDynArray< THandle< CGameplayEntity > >, output, TDynArray< THandle< CGameplayEntity > > () );
	GET_PARAMETER( Vector, point, Vector::ZEROS );
	GET_PARAMETER( Float, coneDir, 0.f );
	GET_PARAMETER( Float, coneAngle, 90.f );
	GET_PARAMETER( Float, range, 15.f );
	GET_PARAMETER( Int32, maxResults, NumericLimits< Int32 >::Max() );
	GET_PARAMETER_OPT( CName, tag, CName::NONE );
	GET_PARAMETER_OPT( Uint32, flags, 0 );
	GET_PARAMETER_OPT( THandle< CGameplayEntity >, target, nullptr );
	GET_PARAMETER_OPT( CName, className, CName::NONE );
	FINISH_PARAMETERS;

	CGameplayStorage::SSearchParams params;
	params.m_origin = point;
	params.m_range = range;
	params.m_flags = flags;
	params.m_maxResults = maxResults;
	params.m_target = target.Get();
	params.m_tag = tag;
	if ( className != CName::NONE )
	{
		params.m_class = SRTTI::GetInstance().FindClass( className );
	}

	GameplayStorageAcceptors::ConeAcceptor acceptor( point, coneDir, coneAngle, range );
	FindGameplayEntitiesInRange< const Vector >( output, point, params, acceptor );
}

void funcFindGameplayEntitiesInBox( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	ASSERT( GGame->IsActive() );

	GET_PARAMETER_REF( TDynArray< THandle< CGameplayEntity > >, output, TDynArray< THandle< CGameplayEntity > > () );
	GET_PARAMETER( Vector, point, Vector::ZEROS );
	GET_PARAMETER( Box, boxLS, Box::EMPTY );
	GET_PARAMETER( Int32, maxResults, NumericLimits< Int32 >::Max() );
	GET_PARAMETER_OPT( CName, tag, CName::NONE );
	GET_PARAMETER_OPT( Uint32, flags, 0 );
	GET_PARAMETER_OPT( THandle< CGameplayEntity >, target, nullptr );
	GET_PARAMETER_OPT( CName, className, CName::NONE );
	FINISH_PARAMETERS;

	CGameplayStorage::SSearchParams params;
	params.m_origin = point;
	params.m_range = Max( Max( MAbs( boxLS.Min.X ), MAbs( boxLS.Min.Y ) ), Max( MAbs( boxLS.Max.X ), MAbs( boxLS.Max.Y ) ) );
	params.m_flags = flags;
	params.m_maxResults = maxResults;
	params.m_target = target.Get();
	params.m_tag = tag;
	if ( className != CName::NONE )
	{
		params.m_class = SRTTI::GetInstance().FindClass( className );
	}

	GameplayStorageAcceptors::BoxAcceptor acceptor( point, boxLS );
	FindGameplayEntitiesInRange< const Vector >( output, point, params, acceptor );
}

void funcFindGameplayEntitiesCloseToPoint( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	ASSERT( GGame->IsActive() );

	GET_PARAMETER_REF( TDynArray< THandle< CGameplayEntity > >, output, TDynArray< THandle< CGameplayEntity > > () );
	GET_PARAMETER( Vector, point, Vector::ZEROS );
	GET_PARAMETER( Float, range, 15.f );
	GET_PARAMETER( Int32, maxResults, NumericLimits< Int32 >::Max() );
	GET_PARAMETER_OPT( CName, tag, CName::NONE );
	GET_PARAMETER_OPT( Uint32, flags, 0 );
	GET_PARAMETER_OPT( THandle< CGameplayEntity >, target, nullptr );
	GET_PARAMETER_OPT( CName, className, CName::NONE );
	FINISH_PARAMETERS;

	CGameplayStorage::SSearchParams params;
	params.m_origin = point;
	params.m_range = range;
	params.m_flags = flags;
	params.m_maxResults = maxResults;
	params.m_tag = tag;
	params.m_target = target.Get();
	if ( className != CName::NONE )
	{
		params.m_class = SRTTI::GetInstance().FindClass( className );
	}

	GameplayStorageAcceptors::DefaultAcceptor acceptor;
	FindGameplayEntitiesInRange< const Vector >( output, point, params, acceptor );
}

void funcFindActorsAtLine( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	ASSERT( GGame->IsActive() );

	GET_PARAMETER( Vector, startPos, Vector::ZEROS );
	GET_PARAMETER( Vector, endPos, Vector::ZEROS );
	GET_PARAMETER( Float, radius, 0.05f );
	GET_PARAMETER_REF( TDynArray< SRaycastHitResult >, hitResults, TDynArray< SRaycastHitResult>() );
	GET_PARAMETER_OPT( TDynArray< CName >, collisionTypeNames, TDynArray< CName >() );
	FINISH_PARAMETERS;

	Uint32 actorsTaken = 0;

#ifdef USE_PHYSX
	Bool characterTest = false;
	if ( collisionTypeNames.Empty() )
	{
		characterTest = true;
	}
	else
	{
		const Uint32 colSize = collisionTypeNames.Size();
		for ( Uint32 i = 0; i < colSize; ++i )
		{
			if ( collisionTypeNames[i] == CNAME( Character ) )
			{
				characterTest = true;
				break;
			}
		}
	}

	if ( !characterTest )
		RETURN_BOOL( false );

	const Uint32 maxResults = 20;
	CActor* foundActorsAround[ maxResults ];
	Int32 resultsCount = GCommonGame->GetActorsManager()->CollectActorsAtLine( startPos, endPos, radius, foundActorsAround, maxResults );

	for ( Int32 no = 0; no < resultsCount; ++no )
	{
		CActor* actor = foundActorsAround[ no ];
		if ( !actor )
			continue;

		CMovingPhysicalAgentComponent* mpac = Cast< CMovingPhysicalAgentComponent >( actor->GetMovingAgentComponent() );
		if ( mpac && mpac->GetPhysicalCharacter() )
		{
			// init
			AACylinder cylinder;
			Uint32 hits = 0;
			Vector hitPos( FLT_MAX, FLT_MAX, FLT_MAX );

			// check its virtual controllers
			if ( mpac->GetPhysicalCharacter()->GetCharacterController() )
			{
				typedef TDynArray< CVirtualCharacterController > VCCArray;
#ifdef USE_PHYSX
				const VCCArray& virtualControllers = mpac->GetPhysicalCharacter()->GetCharacterController()->GetVirtualControllers();
				VCCArray::const_iterator itEnd = virtualControllers.End();
				for ( VCCArray::const_iterator it = virtualControllers.Begin(); it != itEnd; ++it )
				{
					if ( it->IsEnabled() )
					{
						cylinder.m_positionAndRadius = it->GetGlobalPosition();
						cylinder.m_positionAndRadius.W = it->GetCurrentRadius();
						cylinder.m_height = it->GetCurrentHeight();

						Vector locaHitPos;
						if ( cylinder.IntersectRay( startPos, endPos-startPos, locaHitPos ) )
						{
							if ( startPos.DistanceSquaredTo( locaHitPos ) < startPos.DistanceSquaredTo( hitPos ) )
							{
								hitPos = locaHitPos;
							}

							++hits;

							// next v-controller
							continue;
						}
					}
				}
#endif
			}

			// than check physical controller
			const Vector& worldPosition = actor->GetWorldPositionRef();
			if ( mpac->GetPhysicalCharacter()->GetCollisionControllerExtents( cylinder, worldPosition ) )
			{
				Vector localHitPos;
				if ( cylinder.IntersectRay( startPos, endPos-startPos, localHitPos ) )
				{
					if ( startPos.DistanceSquaredTo( localHitPos ) < startPos.DistanceSquaredTo( hitPos ) )
					{
						hitPos = localHitPos;
					}
					++hits;
				}
			}

			// if sth was hit
			if ( hits )
			{
				// call scripts handler
				THandle< CComponent > componentHandle( mpac );
				Vector normal = startPos-endPos;
				normal.Normalize3();

				SRaycastHitResult hitResult;
				hitResult.m_position = hitPos;
				hitResult.m_normal = normal;
				hitResult.m_component = componentHandle;
				hitResult.m_actorShapeIndex = SActorShapeIndex( 0, 0 );
				hitResult.m_distance = (hitPos-startPos).Mag3();
				hitResult.m_wrapper = nullptr;
				hitResult.m_physicalMaterial = nullptr;
				hitResults.PushBack( hitResult );

				++actorsTaken;
			}	
		}
	}
#endif // USE_PHYSX

	RETURN_BOOL( actorsTaken!=0 );
}

void RegisterActorsStorageScriptFunctions()
{
	NATIVE_GLOBAL_FUNCTION( "FindGameplayEntitiesInRange", funcFindGameplayEntitiesInRange );
	NATIVE_GLOBAL_FUNCTION( "FindGameplayEntitiesInSphere", funcFindGameplayEntitiesInSphere );
	NATIVE_GLOBAL_FUNCTION( "FindGameplayEntitiesInCylinder", funcFindGameplayEntitiesInCylinder );
	NATIVE_GLOBAL_FUNCTION( "FindGameplayEntitiesInCone", funcFindGameplayEntitiesInCone );
	NATIVE_GLOBAL_FUNCTION( "FindGameplayEntitiesInBox", funcFindGameplayEntitiesInBox );
	NATIVE_GLOBAL_FUNCTION( "FindGameplayEntitiesCloseToPoint", funcFindGameplayEntitiesCloseToPoint );
	NATIVE_GLOBAL_FUNCTION( "FindActorsAtLine", funcFindActorsAtLine );
}
