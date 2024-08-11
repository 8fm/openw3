/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"
#include "spawnPointManager.h"
#include "../engine/pathlibWorld.h"
#include "../engine/areaComponent.h"

Bool SpawnPointManager::IsSpawnPointAppropriate( const Vector &testPoint, Bool isAppear )
{
	if ( IsPointReachable( testPoint ) && IsPointFree( testPoint ) )
	{
		if ( IsPointSeenByPlayer( testPoint ) == isAppear )
		{
			return true;
		}
	}

	return false;
}

Bool SpawnPointManager::IsPointSeenByPlayer( const Vector &testPoint )
{
	return GGame->GetActiveWorld()->GetCameraDirector()->IsPointInView( testPoint );
}

Bool SpawnPointManager::IsPointReachable( const Vector &testPoint )
{
	//if ( CWorld *world = GGame->GetActiveWorld() )
	//{
	//return world->GetPathEngineWorld()->IsPositionValid( PEAT_Actor, testPoint );
	//}

	//return false;

	// always return true, because IsPointFree() works now for both methods
	return true;
}

Bool SpawnPointManager::IsPointFree( const Vector &testPoint )
{
	if ( CWorld *world = GGame->GetActiveWorld() )
	{
		Vector outPos;
		return world->GetPathLibWorld()->TestLocation( testPoint.AsVector3(), PathLib::CT_DEFAULT );
	}

	return true;
}

Vector SpawnPointManager::GetRandomPointInDisc( const Vector &center, Float maxRadius )
{
	Float randAngle = GEngine->GetRandomNumberGenerator().Get< Float >( 360.0f );
	Float randRadius = GEngine->GetRandomNumberGenerator().Get< Float >( maxRadius );

	EulerAngles rotAngle( 0, 0, randAngle );
	const Vector &rotVec = rotAngle.TransformVector( Vector::EY );

	Vector res = center + ( rotVec * randRadius );

	return res;
}

Vector SpawnPointManager::GetRandomPointInAnnulus( const Vector &center, Float minRadius, Float maxRadius )
{
	ASSERT( minRadius <= maxRadius );

	Float randAngle = GEngine->GetRandomNumberGenerator().Get< Float >( 360.0f );
	Float randRadius = GEngine->GetRandomNumberGenerator().Get< Float >( minRadius , maxRadius );

	EulerAngles rotAngle( 0, 0, randAngle );
	const Vector &rotVec = rotAngle.TransformVector( Vector::EY );

	Vector res = center + ( rotVec * randRadius );

	return res;
}

Int32 SpawnPointManager::GetPointsInAnnulus( const Vector &center, Float minRadius, Float maxRadius,
										    Float stepRadius, Float stepAngle, TDynArray< Vector > &resultPoints /* out */ )
{
	ASSERT( minRadius <= maxRadius );
	ASSERT( stepAngle > 0 );

	// case if minRadius == maxRadius
	if ( stepRadius <= 0 ) stepRadius = 1.0f;

	Int32 result = 0;

	for ( Float angle = 0; angle < 360; angle += stepAngle )
	{
		for ( Float radius = minRadius; radius <= maxRadius; radius += stepRadius )
		{
			EulerAngles rotAngle( 0, 0, angle );
			const Vector &rotVec = rotAngle.TransformVector( Vector::EY );
			resultPoints.PushBack( center + ( rotVec * radius ) );
			++result;
		}
	}

	return result;
}

Vector SpawnPointManager::GetRandomPointInCircle( const Vector &center, Float radius )
{
	Float randAngle = GEngine->GetRandomNumberGenerator().Get< Float >( 360.0f );

	EulerAngles rotAngle( 0, 0, randAngle );
	const Vector &rotVec = rotAngle.TransformVector( Vector::EY );

	Vector res = center + ( rotVec * radius );

	return res;
}

Vector SpawnPointManager::GetRandomPointInPie( const Vector &center, Float radius, Float yawAngleBeg, Float yawAngleEnd )
{
	Float randAngle = GEngine->GetRandomNumberGenerator().Get< Float >( yawAngleBeg , yawAngleEnd );

	EulerAngles rotAngle( 0, 0, randAngle );
	const Vector &rotVec = rotAngle.TransformVector( Vector::EY );

	Vector res = center + ( rotVec * radius );

	return res;
}

Vector SpawnPointManager::GetPointInCircle( const Vector &center, Float radius, Float angle )
{
	EulerAngles rotAngle( 0, 0, angle );
	const Vector &rotVec = rotAngle.TransformVector( Vector::EY );

	Vector res = center + ( rotVec * radius );

	return res;
}

Bool SpawnPointManager::GetRandomReachablePoint( const Vector &center, Float maxRadius, Vector &spawnPoint /* out */ )
{
	Vector randPoint = GetRandomPointInDisc( center, maxRadius );
	Float randRadius = maxRadius - center.DistanceTo( randPoint );
	return GetFreeReachablePoint( randPoint, randRadius, spawnPoint );
}

Bool SpawnPointManager::GetRandomReachablePoint( const Vector &center, Float minRadius, Float maxRadius, Vector &spawnPoint /* out */ )
{
	Vector randPoint = GetRandomPointInAnnulus( center, minRadius, maxRadius );
	Float randRadius = maxRadius - center.DistanceTo( randPoint );
	return GetFreeReachablePoint( randPoint, randRadius, spawnPoint );
}

Bool SpawnPointManager::GetRandomReachablePoint( const Vector &center, Float minRadius, Float maxRadius, CAreaComponent *area, Vector &spawnPoint /* out */ )
{
	TDynArray< Vector > randPoints;
	GetPointsInAnnulus( center, minRadius, maxRadius, (maxRadius - minRadius) / 3.0f, 90.0f, randPoints );
	
	while ( !randPoints.Empty() )
	{
		Vector randPoint;
		Vector result;

		Uint32 pointIndex = GEngine->GetRandomNumberGenerator().Get< Uint32 >( randPoints.Size() );
		ASSERT( pointIndex < randPoints.Size() );
		randPoint = randPoints[ pointIndex ];

		if ( GetFreeReachablePoint( randPoint, 1.0f, result ) )
		{
			if ( area->TestPointOverlap( result ) )
			{
				spawnPoint = result;
				return true;
			}
		}

		randPoints.EraseFast( randPoints.Begin() + pointIndex );
	}

	return false;
}

Bool SpawnPointManager::GetFreeReachablePoint( const Vector &center, Float maxRadius, Vector &spawnPoint /* out */ )
{
	if ( CWorld *world = GGame->GetActiveWorld() )
	{
		return world->GetPathLibWorld()->FindSafeSpot( PathLib::INVALID_AREA_ID, center, maxRadius, 0.1f, spawnPoint.AsVector3() );
	}

	return false;
}

void SpawnPointManager::GenerateSpawnPoints( const Vector &center, Float maxRadius, TDynArray< Vector > &spawnPoints /* out */ )
{
	ASSERT( maxRadius > 0 );

	const Float STEP = 5.0f;

	// vertical line
	for ( Float x = center.X - maxRadius; x < center.X + maxRadius; x += STEP )
	{
		spawnPoints.PushBack( Vector( x, center.Y, center.Z ) );
	}

	// horizontal line
	for ( Float y = center.Y - maxRadius; y < center.Y + maxRadius; y += STEP )
	{
		spawnPoints.PushBack( Vector( center.X, y, center.Z ) );
	}
}

void funcIsPointSeenByPlayer( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, testPoint, Vector::ZEROS );
	FINISH_PARAMETERS;

	RETURN_BOOL( SpawnPointManager::IsPointSeenByPlayer( testPoint ) );
}

void funcIsPointFree( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, testPoint, Vector::ZEROS );
	FINISH_PARAMETERS;

	RETURN_BOOL( SpawnPointManager::IsPointFree( testPoint ) );
}

void funcGetRandomReachablePoint( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, center, Vector::ZEROS );
	GET_PARAMETER( Float, minRadius, 0.0f );
	GET_PARAMETER( Float, maxRadius, 1.0f );
	GET_PARAMETER_REF( Vector, spawnPoint, Vector::ZEROS );
	FINISH_PARAMETERS;

	RETURN_BOOL( SpawnPointManager::GetRandomReachablePoint( center, minRadius, maxRadius, spawnPoint ) );
}

void funcGetRandomReachablePointWithinArea( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, center, Vector::ZEROS );
	GET_PARAMETER( Float, minRadius, 0.0f );
	GET_PARAMETER( Float, maxRadius, 1.0f );
	GET_PARAMETER( THandle< CAreaComponent >, areaComponent, THandle< CAreaComponent >() );
	GET_PARAMETER_REF( Vector, spawnPoint, Vector::ZEROS );
	FINISH_PARAMETERS;

	CAreaComponent *area = areaComponent.Get();
	if ( area )
	{
		RETURN_BOOL( SpawnPointManager::GetRandomReachablePoint( center, minRadius, maxRadius, area, spawnPoint ) );
	}
	else
	{
		RETURN_BOOL( false );
	}
}

void funcGetFreeReachablePoint( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, center, Vector::ZEROS );
	GET_PARAMETER( Float, maxRadius, 0.0f );
	GET_PARAMETER_REF( Vector, spawnPoint, Vector::ZEROS );
	FINISH_PARAMETERS;

	RETURN_BOOL( SpawnPointManager::GetFreeReachablePoint( center, maxRadius, spawnPoint ) );
}

void RegisterSpawnPointManagerFunctions()
{
	NATIVE_GLOBAL_FUNCTION( "IsPointSeenByPlayer", funcIsPointSeenByPlayer );
	NATIVE_GLOBAL_FUNCTION( "IsPointFree", funcIsPointFree );
	NATIVE_GLOBAL_FUNCTION( "GetRandomReachablePoint", funcGetRandomReachablePoint );
	NATIVE_GLOBAL_FUNCTION( "GetRandomReachablePointWithinArea", funcGetRandomReachablePointWithinArea );
	NATIVE_GLOBAL_FUNCTION( "GetFreeReachablePoint", funcGetFreeReachablePoint );
}
