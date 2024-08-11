#include "build.h"

#include "horsePrediction.h"
#include "../../common/physics/physicsWorldUtils.h"
#include "../../common/physics/physicsWorld.h"
#include "../../common/core/mathUtils.h"
#include "../../common/engine/renderFrame.h"


IMPLEMENT_ENGINE_CLASS( SPredictionInfo );
IMPLEMENT_ENGINE_CLASS( CHorsePrediction );

const Float CHorsePrediction::Z_OFFSET = 0.7f;
const Float CHorsePrediction::SWEEP_RAD = 0.4f;
const Float CHorsePrediction::PROBE_RAD = 0.5f;

#ifndef RED_FINAL_BUILD
	TDynArray<CHorsePrediction*> CHorsePrediction::m_dbgFragments;
#endif

#define MAXCONTACTS 32

#define ULL 0	// upper most left
#define UL 1	// upper left
#define UR 2	// upper right
#define URR 3	// upper most right
#define LLL 4	// lower most left
#define LL 5	// lower left
#define LR 6	// lower right
#define LRR 7	// lower most right

const Vector CHorsePrediction::OFFSETS[] = {	Vector( PROBE_RAD * -3.f, 0.f, PROBE_RAD * 2.f + 0.2f ), Vector( -PROBE_RAD, 0.f, PROBE_RAD * 2.f + 0.2f ), Vector( PROBE_RAD, 0.f, PROBE_RAD * 2.f + 0.2f ), Vector( PROBE_RAD * 3.f, 0.f, PROBE_RAD * 2.f + 0.2f ),
												Vector( PROBE_RAD * -3.f, 0.f, PROBE_RAD + 0.2f ), Vector( -PROBE_RAD, 0.f, PROBE_RAD + 0.2f ), Vector( PROBE_RAD, 0.f, PROBE_RAD + 0.2f ), Vector( PROBE_RAD * 3.f, 0.f, PROBE_RAD + 0.2f ) };

RED_DECLARE_NAME( OnCorrectRout );

Float tmpAngle = 0.f;
Matrix tmpLTW = Matrix::IDENTITY;

Matrix YawToMatrix( Float Yaw )
{
	const Float cosYaw = MCos( DEG2RAD( Yaw ) );
	const Float sinYaw = MSin( DEG2RAD( Yaw ) );

	Matrix ret;
	ret.SetIdentity();
	ret.V[0].A[0] = cosYaw;
	ret.V[0].A[1] = sinYaw;
	ret.V[1].A[0] = -sinYaw;
	ret.V[1].A[1] = cosYaw;
	return ret;
}

SPredictionInfo CHorsePrediction::CollectPredictionInfo( CNode* ent, Float testDistance, Float inputDir, Bool checkWater )
{
	SPredictionInfo output = { 0.f, 0.f };

	CPhysicsWorld* physics = nullptr;
	if ( !GGame->GetActiveWorld()->GetPhysicsWorld( physics ) || !ent )
	{
		return output;
	}

	const Matrix ltw = YawToMatrix( -inputDir ) * ent->GetLocalToWorld();

	Vector pos = ltw.GetTranslation();
	const Vector axisX = ltw.GetAxisX();
	Vector axisY = ltw.GetAxisY();

	if( checkWater )
	{
		STATIC_GAME_ONLY CPhysicsEngine::CollisionMask collisionType = GPhysicEngine->GetCollisionTypeBit( CNAME( Static ) );
		STATIC_GAME_ONLY CPhysicsEngine::CollisionMask include = GPhysicEngine->GetCollisionGroupMask( collisionType );

		Vector axisArr[3] = { axisY, axisY, axisY };
		Float results[3] = { FLT_MAX, FLT_MAX, FLT_MAX };

		axisArr[0].AsVector2() = MathUtils::GeometryUtils::Rotate2D( axisArr[0].AsVector2(), DEG2RAD( 45.f ) );
		axisArr[2].AsVector2() = MathUtils::GeometryUtils::Rotate2D( axisArr[2].AsVector2(), DEG2RAD( -45.f ) );

		for( Uint32 i = 0; i < 3; ++i )
		{
			Vector start = pos + axisArr[i] * testDistance;
			start.Z += 2.f;

			Vector end = start;
			end.Z -= 10.f;

			SPhysicsContactInfo info;
			if( physics->RayCastWithSingleResult( start, end, include, 0, info ) == TRV_Hit )
			{
				results[i] = info.m_position.Z;

#ifndef RED_FINAL_BUILD
				m_dbgHeightTrace[i] = info.m_position;
#endif
			}
		}

		output.m_leftGroundLevel = results[0];
		output.m_frontGroundLevel = results[1];
		output.m_rightGroundLevel = results[2];
	}

	STATIC_GAME_ONLY CPhysicsEngine::CollisionMask collisionType = GPhysicEngine->GetCollisionTypeBit( CNAME( Camera ) );
	STATIC_GAME_ONLY CPhysicsEngine::CollisionMask include = GPhysicEngine->GetCollisionGroupMask( collisionType );

	// TEST FORWARD
	SPhysicsContactInfo contactInfo[MAXCONTACTS];
	Vector start = pos;
	start.Z += Z_OFFSET;

	// UNCOMMENT FOR IT TO WORK
	Uint32 results = 0; //physics->SphereSweepTestWithMultipleResults( start, start + axisY * testDistance, SWEEP_RAD, include, 0, contactInfo, MAXCONTACTS ); 

	Float ratio = 1.f;
	Float normalZ = 0.f;
	for ( Uint32 i = 0; i < results; ++i )
	{
		const SPhysicsContactInfo& info = contactInfo[ i ];

		ASSERT( info.m_distance >= 0.f );

		if( info.m_distance < ratio )
		{
			ratio = info.m_distance;
			normalZ = info.m_normal.Z;
			output.m_normalYaw = RAD2DEG( -atan2f( info.m_normal.X, info.m_normal.Y ) );
		}
	}

	// CHECK IF MOVING DOWNHILL
	/*if( results == 0 )
	{
		const Vector start2 = start + axisY;
		SPhysicsContactInfo info;
		if( physics->RayCastWithSingleResult( start2, start2 + Vector::EZ * -5.f, include, 0, info ) )
		{
			if( info.m_distance > 0.14f )
			{
				Vector temp = start2;
				temp.Z -= info.m_distance * 5.f - Z_OFFSET;
				axisY = (temp - start).Normalized3();

#ifndef RED_FINAL_BUILD
				m_dbgSweep[0] = start;
				m_dbgSweep[1] = start + axisY * testDistance;
#endif
				results = physics->SphereSweepTestWithMultipleResults( start, start + axisY * testDistance, SWEEP_RAD, include, 0, contactInfo, MAXCONTACTS );

				ratio = 1.f;
				for ( Uint32 i = 0; i < results; ++i )
				{
					const SPhysicsContactInfo& info = contactInfo[ i ];

					ASSERT( info.m_distance >= 0.f );

					if( info.m_distance < ratio )
					{
						ratio = info.m_distance;
						output.m_normalYaw = RAD2DEG( -atan2f( info.m_normal.X, info.m_normal.Y ) );
					}
				}
			}
		}
	}*/

	// CHECK IF WE HAVE AN ELEVATION
	if( normalZ > 0.71f )
	{
		const Vector lowerPos = axisY * (ratio * testDistance);
		start.Z += 1.f;
		const ETraceReturnValue retVal = physics->SphereSweepTestWithMultipleResults( start, start + axisY * testDistance, SWEEP_RAD, include, 0, contactInfo, results, MAXCONTACTS ); 

		ratio = 1.f;
		for ( Uint32 i = 0; i < results; ++i )
		{
			const SPhysicsContactInfo& info = contactInfo[ i ];

			ASSERT( info.m_distance >= 0.f );

			if( info.m_distance < ratio )
			{
				ratio = info.m_distance;
				normalZ = info.m_normal.Z;
				output.m_normalYaw = RAD2DEG( -atan2f( info.m_normal.X, info.m_normal.Y ) );
			}
		}

		if( results > 0 )
		{
			Vector upperPos = axisY * (ratio * testDistance);
			upperPos.Z += 1.f;

			//const Float Pitch = RAD2DEG( atan2f( -dir.Z, sqrtf( dir.X*dir.X + dir.Y*dir.Y )) );
			const Vector dir = (upperPos - lowerPos).Normalized3();
			if( dir.Z < 0.71f )
			{
				const Float angle = DEG2RAD( ent->GetWorldRotation().Yaw - inputDir );
				const Float factor = 1.f - dir.Z * dir.Z;
				const Float cosYaw = MCos( angle );
				const Float sinYaw = MSin( angle );
				const Float y = MSqrt( cosYaw * cosYaw * factor );
				const Float x = MSqrt( sinYaw * sinYaw * factor );

				axisY = Vector( sinYaw >= 0.f ? -x : x, cosYaw < 0.f ? -y : y, dir.Z );

				ASSERT( MAbs( axisY.Mag3() - 1.f ) < 0.01f );

#ifndef RED_FINAL_BUILD
				m_dbgSweep[0] = start;
				m_dbgSweep[1] = start + axisY * testDistance;
#endif

				const ETraceReturnValue retVal = physics->SphereSweepTestWithMultipleResults( start, start + axisY * testDistance, SWEEP_RAD, include, 0, contactInfo, results, MAXCONTACTS ); 

				ratio = 1.f;
				for ( Uint32 i = 0; i < results; ++i )
				{
					const SPhysicsContactInfo& info = contactInfo[ i ];

					ASSERT( info.m_distance >= 0.f );

					if( info.m_distance < ratio )
					{
						ratio = info.m_distance;
						output.m_normalYaw = RAD2DEG( -atan2f( info.m_normal.X, info.m_normal.Y ) );
					}
				}
			}
		}
	}

	/*Uint32 tests = 0;
	while( results > 0 && normalZ > 0.71f && tests < 3 )
	{
		const Float angle = DEG2RAD( ent->GetWorldRotation().Yaw - inputDir );
		const Float crossNormZ = 1.f - normalZ;
		const Float factor = 1.f - crossNormZ * crossNormZ;
		const Float cosYaw = MCos( angle );
		const Float sinYaw = MSin( angle );
		const Float y = MSqrt( cosYaw * cosYaw * factor );
		const Float x = MSqrt( sinYaw * sinYaw * factor );

		axisY = Vector( sinYaw >= 0.f ? -x : x, cosYaw < 0.f ? -y : y, crossNormZ );


		const Vector temp( -sinYaw, cosYaw, 0.0f );

		ASSERT( MAbs( axisY.Mag3() - 1.f ) < 0.01f );

#ifndef RED_FINAL_BUILD
		m_dbgSweep[0] = start;
		m_dbgSweep[1] = start + axisY * testDistance;
#endif

		results = physics->SphereSweepTestWithMultipleResults( start, start + axisY * testDistance, SWEEP_RAD, include, 0, contactInfo, MAXCONTACTS ); 

		ratio = 1.f;
		for ( Uint32 i = 0; i < results; ++i )
		{
			const SPhysicsContactInfo& info = contactInfo[ i ];

			ASSERT( info.m_distance >= 0.f );

			if( info.m_distance < ratio )
			{
				ratio = info.m_distance;
				if( info.m_normal.Z < normalZ )
				{
					normalZ = info.m_normal.Z;
				}
				output.m_normalYaw = RAD2DEG( -atan2f( info.m_normal.X, info.m_normal.Y ) );
			}
		}
	}*/

	// FINALLY COMPUTE THE TURNING ANGLE
	if( ratio < 1.f )
	{
		output.m_distanceToCollision = Max( ratio * testDistance, 0.1f );
		const Float offsetY = output.m_distanceToCollision + PROBE_RAD * 0.5f;
		for( Uint32 i = 0; i < PROBES_NUM; ++i )
		{
			Vector position = pos;
			position += axisX * OFFSETS[i].X + axisY * offsetY;
			position.Z += OFFSETS[i].Z;

#ifndef RED_FINAL_BUILD
			m_dbgProbes[i] = position;
#endif

			m_hits[i] = 0.f;

			SPhysicsOverlapInfo overlapInfo[MAXCONTACTS];
			Uint32 results = 0;
			const ETraceReturnValue retVal = physics->SphereOverlapWithMultipleResults( position, PROBE_RAD, include, 0, overlapInfo, results, MAXCONTACTS );
			for( Uint32 j = 0; j < results; ++j )
			{
				if( m_hits[i] > overlapInfo[j].m_penetration )
				{
					m_hits[i] = overlapInfo[j].m_penetration;
				}
			}
		}

		// TURNING
		const Float leftHit = Min( m_hits[UL], m_hits[LL] ) + Min( m_hits[ULL], m_hits[LLL] );
		const Float rightHit = Min( m_hits[UR], m_hits[LR] ) + Min( m_hits[URR], m_hits[LRR] );

		Float hitFactor;

		if( MAbs(inputDir) < 10.f )
		{
			hitFactor = m_prevTurn == PT_None ? leftHit - rightHit : ( m_prevTurn == PT_Right ? leftHit : -rightHit );
		}
		else
		{
			m_prevTurn = inputDir < 0.f ? PT_Left : PT_Right;

			if( MAbs( leftHit - rightHit ) > 0.3f )
			{
				hitFactor = leftHit < rightHit ? leftHit : -rightHit;
			}
			else
			{
				hitFactor = inputDir < 0.f ? -leftHit - rightHit : leftHit + rightHit;
			}
		}

		const Float angle = Clamp( 120.f / output.m_distanceToCollision, 30.f, 90.f );

		if( output.m_distanceToCollision < 3.5f && MAbs(hitFactor) < 0.2f && m_hits[UL] < -0.1f && m_hits[UR] < -0.1f )
		{
			output.m_turnAngle = inputDir > 0.f ? -angle : angle;
		}
		else
		{
			output.m_turnAngle = hitFactor * angle;
		}

		m_prevTurn = hitFactor < 0.f ? PT_Right : ( hitFactor > 0 ? PT_Left : PT_None );

		// dbg shit
		tmpAngle = output.m_turnAngle;
		tmpLTW = ltw;
	}
	else
	{
		Red::System::MemoryZero( m_hits, sizeof( m_hits ) );
		m_prevTurn = PT_None;
	}

	output.m_turnAngle -= inputDir;

	return output;
}

void CHorsePrediction::GenerateDebugFragments( CRenderFrame* frame )
{
#ifndef RED_FINAL_BUILD
	/*for( Uint32 i = 0; i < PROBES_NUM; ++i )
	{
		frame->AddDebugSphere( m_dbgProbes[i], PROBE_RAD, Matrix::IDENTITY, m_hits[i] < 0.f ? Color::RED : Color::BLUE );
	}*/

	//frame->AddDebugFatLine( m_dbgSweep[0], m_dbgSweep[1], Color::RED, 0.1f );

	for( Uint32 i = 0; i < 3; ++i )
	{
		frame->AddDebugSphere( m_dbgHeightTrace[i], 0.2f, Matrix::IDENTITY, Color::RED );
	}

	//frame->AddDebugFatLine( tmpLTW.GetTranslationRef(), tmpLTW.GetTranslationRef() + tmpLTW.TransformVector( EulerAngles::YawToVector( tmpAngle ) ), Color::RED, 0.1f, false );
	//frame->AddDebugArrow( tmpLTW, EulerAngles::YawToVector( tmpAngle ), 1.f, Color::RED );
#endif
}


// SCRIPTS

void CHorsePrediction::funcCollectPredictionInfo( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle<CNode>, node, NULL );
	GET_PARAMETER( Float, distance, 1.f );
	GET_PARAMETER( Float, direction, 0.f );
	GET_PARAMETER( Bool, checkWater, false );
	FINISH_PARAMETERS;

	RETURN_STRUCT( SPredictionInfo, CollectPredictionInfo( node.Get(), distance, direction, checkWater ) );
}