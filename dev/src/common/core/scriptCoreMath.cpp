/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "scriptingSystem.h"
#include "scriptStackFrame.h"
#include "mathUtils.h"

#include "../redMath/random/noise.h"
#include "../redMath/redmathbase.h"

/////////////////////////////////////////////
// Math functions
/////////////////////////////////////////////

void funcRand( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	Int32 randomNumber = 0;

	randomNumber = GScriptingSystem->GetRandomNumberGenerator().Get< Int32 >();

	RETURN_INT( randomNumber );
}

void funcRandRange( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, max, 0 );
	GET_PARAMETER_OPT( Int32, min, 0 );
	FINISH_PARAMETERS;

	Int32 randomNumber = 0;

	randomNumber = GScriptingSystem->GetRandomNumberGenerator().Get< Int32 >( min , max );

	RETURN_INT( randomNumber );
}

void funcRandDifferent( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, lastValue, 0 );
	GET_PARAMETER_OPT( Int32, range, GScriptingSystem->GetRandomNumberGenerator().Max< Int32 >() );
	FINISH_PARAMETERS;

	Int32 randomNumber = 0;

	if( range <= 1 )
	{
		randomNumber = 0;
	}
	else if( range == 2 )
	{
		randomNumber = ( ( lastValue + 1 ) % 2 );
	}
	else
	{
		do
		{
			randomNumber = GScriptingSystem->GetRandomNumberGenerator().Get< Int32 >( range );
		}
		while( randomNumber == lastValue );
	}

	RETURN_INT( randomNumber );
}

void funcRandF( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_FLOAT( GScriptingSystem->GetRandomNumberGenerator().Get< Float >() );
}

void funcRandRangeF( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, max, 1.0f );
	GET_PARAMETER( Float, min, 0.0f );
	FINISH_PARAMETERS;

	RETURN_FLOAT( (min < max) ? GScriptingSystem->GetRandomNumberGenerator().Get< Float >( min , max ) : min );
}

void funcRandNoiseF( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, seed, 0 );
	GET_PARAMETER( Float, max, 0.0f );
	GET_PARAMETER_OPT( Float, min, 0.0f );
	FINISH_PARAMETERS;

	Red::Math::Random::Generator< Red::Math::Random::Noise > noise;

	noise.Seed( seed );

	RETURN_FLOAT( noise.Get< Float >( max, min ) );
}

void funcAbs( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, range, 0 );
	FINISH_PARAMETERS;

	RETURN_INT( Abs( range ) );
}

void funcMin( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, a, 0 );
	GET_PARAMETER( Int32, b, 0 );
	FINISH_PARAMETERS;

	RETURN_INT( Min<Int32>( a, b ) );
}

void funcMax( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, a, 0 );
	GET_PARAMETER( Int32, b, 0 );
	FINISH_PARAMETERS;

	RETURN_INT( Max<Int32>( a, b ) );
}

void funcClamp( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, v, 0 );
	GET_PARAMETER( Int32, a, 0 );
	GET_PARAMETER( Int32, b, 0 );
	FINISH_PARAMETERS;

	RETURN_INT( Clamp<Int32>( v, a, b ) );
}

void funcDeg2Rad( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, deg, 0.0f );
	FINISH_PARAMETERS;

	RETURN_FLOAT( DEG2RAD( deg ) );
}

void funcRad2Deg( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, rad, 0.0f );
	FINISH_PARAMETERS;

	RETURN_FLOAT( RAD2DEG( rad ) );
}

void funcAbsF( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, a, 0.0f );
	FINISH_PARAMETERS;

	RETURN_FLOAT( Abs<Float>( a ) );
}

void funcSinF( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, a, 0.0f );
	FINISH_PARAMETERS;

	RETURN_FLOAT( sinf( a ) );
}

void funcAsinF( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, a, 0.0f );
	FINISH_PARAMETERS;

	RETURN_FLOAT( asinf( a ) );
}

void funcCosF( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, a, 0.0f );
	FINISH_PARAMETERS;

	RETURN_FLOAT( cosf( a ) );
}

void funcAcosF( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, a, 0.0f );
	FINISH_PARAMETERS;

	RETURN_FLOAT( acosf( a ) );
}

void funcTanF( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, a, 0.0f );
	FINISH_PARAMETERS;

	RETURN_FLOAT( tanf( a ) );
}

void funcAtanF( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, a, 0.0f );
	GET_PARAMETER( Float, b, 0.0f );
	FINISH_PARAMETERS;

	RETURN_FLOAT( atan2f( a, b ) );
}

void funcExpF( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, a, 0.0f );
	FINISH_PARAMETERS;

	RETURN_FLOAT( expf( a ) );
}

void funcPowF( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, a, 0.0f );
	GET_PARAMETER( Float, x, 0.0f );
	FINISH_PARAMETERS;

	RETURN_FLOAT( powf( a, x ) );
}

void funcLogF( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, a, 0.0f );
	FINISH_PARAMETERS;

	RETURN_FLOAT( logf( a ) );
}

void funcSqrtF( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, a, 0.0f );
	FINISH_PARAMETERS;

	RETURN_FLOAT( sqrtf( a ) );
}

void funcSqrF( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, a, 0.0f );
	FINISH_PARAMETERS;

	RETURN_FLOAT( a * a );
}

void funcCalcSeed( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< IScriptable>, objHanlde, NULL );
	FINISH_PARAMETERS;

	IScriptable* obj = objHanlde.Get();

	RETURN_INT( Int32( (uintptr_t)(void*)(obj) ) );
}

void funcMinF( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, a, 0.0f );
	GET_PARAMETER( Float, b, 0.0f );
	FINISH_PARAMETERS;

	RETURN_FLOAT( Min<Float>( a, b ) );
}

void funcMaxF( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, a, 0.0f );
	GET_PARAMETER( Float, b, 0.0f );
	FINISH_PARAMETERS;

	RETURN_FLOAT( Max<Float>( a, b ) );
}

void funcClampF( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, v, 0.0f );
	GET_PARAMETER( Float, a, 0.0f );
	GET_PARAMETER( Float, b, 0.0f );
	FINISH_PARAMETERS;

	RETURN_FLOAT( Clamp<Float>( v, a, b ) );
}

void funcLerpF( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, alpha, 0.0f );
	GET_PARAMETER( Float, a, 0.0f );
	GET_PARAMETER( Float, b, 0.0f );
	GET_PARAMETER_OPT( Bool, clamp, false );
	FINISH_PARAMETERS;

	Float ret = ::Lerp<Float>( alpha, a, b );
	if ( clamp && ret < a ) ret = a;
	if ( clamp && ret > b ) ret = b;
	RETURN_FLOAT( ret );
}

void funcCeilF( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, a, 0.0f );
	FINISH_PARAMETERS;

	RETURN_INT( (int)ceilf( a ) );
}

void funcFloorF( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, a, 0.0f );
	FINISH_PARAMETERS;

	RETURN_INT( (int)MFloor( a ) );
}

void funcRoundF( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, a, 0.0f );
	FINISH_PARAMETERS;

	RETURN_INT( (int)a );
}

void funcRoundFEx( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, a, 0.0f );
	FINISH_PARAMETERS;

	RETURN_INT( (int)MRound( a ) );
}

void funcReinterpretIntAsFloat( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, var, 0 );
	FINISH_PARAMETERS;

	RETURN_FLOAT( *(Float*)(&var) );
}

/////////////////////////////////////////////
// Angle functions
/////////////////////////////////////////////

void funcAngleNormalize( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, angle, 0.0f );
	FINISH_PARAMETERS;

	RETURN_FLOAT( EulerAngles::NormalizeAngle( angle ) );
}

void funcAngleDistance( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, target, 0.0f );
	GET_PARAMETER( Float, current, 0.0f );
	FINISH_PARAMETERS;
	
	Float delta = EulerAngles::AngleDistance( current, target );
	RETURN_FLOAT( delta );
}

void funcAngleApproach( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, target, 0.0f );
	GET_PARAMETER( Float, current, 0.0f );
	GET_PARAMETER( Float, step, 0.0f );
	FINISH_PARAMETERS;

	Float delta = EulerAngles::AngleDistance( current, target );

	// Move
	if ( MAbs( delta ) > MAbs( step ) )
	{
		Float deltaSign = delta >= 0.0f ? 1.0f : -1.0f;
		current = EulerAngles::NormalizeAngle( current + step * deltaSign );
	}
	else
	{
		current = target; 
	} 

	// Return final angle
	RETURN_FLOAT( current );
}

/////////////////////////////////////////////
// Vector functions
/////////////////////////////////////////////

void funcVecDot2D( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, a, Vector::ZEROS );
	GET_PARAMETER( Vector, b, Vector::ZEROS );
	FINISH_PARAMETERS;

	RETURN_FLOAT( Vector::Dot2( a, b ) );
}

void funcVecDot( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, a, Vector::ZEROS );
	GET_PARAMETER( Vector, b, Vector::ZEROS );
	FINISH_PARAMETERS;

	RETURN_FLOAT( Vector::Dot3( a, b ) );
}

void funcVecCross( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, a, Vector::ZEROS );
	GET_PARAMETER( Vector, b, Vector::ZEROS );
	FINISH_PARAMETERS;

	RETURN_STRUCT( Vector, Vector::Cross( a, b, 1.0f ) );
}

void funcVecLength2D( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, a, Vector::ZEROS );
	FINISH_PARAMETERS;

	RETURN_FLOAT( a.Mag2() );
}

void funcVecLength( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, a, Vector::ZEROS );
	FINISH_PARAMETERS;

	RETURN_FLOAT( a.Mag3() );
}

void funcVecLengthSquared( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, a, Vector::ZEROS );
	FINISH_PARAMETERS;

	RETURN_FLOAT( a.SquareMag3() );
}

void funcVecNormalize2D( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, a, Vector::ZEROS );
	FINISH_PARAMETERS;

	RETURN_STRUCT( Vector, a.Normalized2() );
}

void funcVecNormalize( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, a, Vector::ZEROS );
	FINISH_PARAMETERS;

	RETURN_STRUCT( Vector, a.Normalized3() );
}

void funcVecRand2D( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	Vector ret;
	do 
	{
		Float x = GScriptingSystem->GetRandomNumberGenerator().Get< Float >( -1.0f , 1.0f );
		Float y = GScriptingSystem->GetRandomNumberGenerator().Get< Float >( -1.0f , 1.0f );
		ret = Vector( x, y, 0.0f );
	}
	while ( Vector::Near3( ret, Vector::ZEROS ) );

	RETURN_STRUCT( Vector, ret.Normalized2() );
}

void funcVecRand( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	Vector ret;
	do 
	{
		Float x = GScriptingSystem->GetRandomNumberGenerator().Get< Float >( -1.0f , 1.0f );
		Float y = GScriptingSystem->GetRandomNumberGenerator().Get< Float >( -1.0f , 1.0f );
		Float z = GScriptingSystem->GetRandomNumberGenerator().Get< Float >( -1.0f , 1.0f );
		ret = Vector( x, y, z );
	}
	while ( Vector::Near3( ret, Vector::ZEROS ) );

	RETURN_STRUCT( Vector, ret.Normalized3() );
}

void funcVecMirror( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, dir, Vector::ZEROS );
	GET_PARAMETER( Vector, normal, Vector::ZEROS );
	FINISH_PARAMETERS;

	Float d = 2.0f * Vector::Dot3( dir, normal );
	Vector ret = dir - ( normal * d );

	RETURN_STRUCT( Vector, ret );
}

void funcVecDistance( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, a, Vector::ZEROS );
	GET_PARAMETER( Vector, b, Vector::ZEROS );
	FINISH_PARAMETERS;

	RETURN_FLOAT( a.DistanceTo( b ) );
}

void funcVecDistanceSquared( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, a, Vector::ZEROS );
	GET_PARAMETER( Vector, b, Vector::ZEROS );
	FINISH_PARAMETERS;

	RETURN_FLOAT( a.DistanceSquaredTo( b ) );
}

void funcVecDistance2D( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, a, Vector::ZEROS );
	GET_PARAMETER( Vector, b, Vector::ZEROS );
	FINISH_PARAMETERS;

	RETURN_FLOAT( a.DistanceTo2D( b ) );
}

void funcVecDistanceSquared2D( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, a, Vector::ZEROS );
	GET_PARAMETER( Vector, b, Vector::ZEROS );
	FINISH_PARAMETERS;

	RETURN_FLOAT( a.DistanceSquaredTo2D( b ) );
}

void funcVecDistanceToEdge( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, pt, Vector::ZEROS );
	GET_PARAMETER( Vector, a, Vector::ZEROS );
	GET_PARAMETER( Vector, b, Vector::ZEROS );
	FINISH_PARAMETERS;

	RETURN_FLOAT( pt.DistanceToEdge( a, b ) );
}

void funcVecNearestPointOnEdge( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, pt, Vector::ZEROS );
	GET_PARAMETER( Vector, a, Vector::ZEROS );
	GET_PARAMETER( Vector, b, Vector::ZEROS );
	FINISH_PARAMETERS;

	RETURN_STRUCT( Vector, pt.NearestPointOnEdge( a, b ) );
}

void funcVecToRotation( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, dir, Vector::ZEROS );
	FINISH_PARAMETERS;

	RETURN_STRUCT( EulerAngles, dir.ToEulerAngles() );
}

void funcVecHeading( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, dir, Vector::ZEROS );
	FINISH_PARAMETERS;

	RETURN_FLOAT( dir.ToEulerAngles().Yaw );
}

void funcVecFromHeading( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, heading, 0.0f );
	FINISH_PARAMETERS;

	const Float cosYaw = MCos( DEG2RAD( heading ) );
	const Float sinYaw = MSin( DEG2RAD( heading ) );
	const Vector ret( -sinYaw, cosYaw, 0.0f );

	RETURN_STRUCT( Vector, ret );
}

void funcVecTransform( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Matrix, m, Matrix::IDENTITY );
	GET_PARAMETER( Vector, point, Vector::ZEROS );
	FINISH_PARAMETERS;

	RETURN_STRUCT( Vector, m.TransformPoint( point ) );
}

void funcVecRotateAxis( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( RedVector4, vector, RedVector4::ZEROS );
	GET_PARAMETER( RedVector4, axis, RedVector4::ZEROS );
	GET_PARAMETER( Float, angle, 0.0f );
	FINISH_PARAMETERS;

	AxisRotateVector( vector, axis, angle );
	RETURN_STRUCT( Vector, Vector( vector.X, vector.Y, vector.Z, vector.W ) );
}

void funcVecApproach( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, vector, Vector::ZEROS );
	GET_PARAMETER( Vector, target, Vector::ZEROS );
	GET_PARAMETER( Float, angle, 0.0f );
	FINISH_PARAMETERS;

	RedVector4	redVector( vector.X, vector.Y, vector.Z );
	RedVector4	redTarget( target.X, target.Y, target.Z );

	AxisRotateVector( redVector, Cross( redVector, redTarget ), Min( angle, MathUtils::VectorUtils::GetAngleDegBetweenVectors( vector, target ) ) );

	vector.Set3( redVector.X, redVector.Y, redVector.Z );
	RETURN_STRUCT( Vector, vector );
}

void funcVecTransformDir( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Matrix, m, Matrix::IDENTITY  );
	GET_PARAMETER( Vector, dir, Vector::ZEROS );
	FINISH_PARAMETERS;

	RETURN_STRUCT( Vector, m.TransformVector( dir ) );
}

void funcVecTransformH( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Matrix, m, Matrix::IDENTITY );
	GET_PARAMETER( Vector, point, Vector::ZEROS );
	FINISH_PARAMETERS;

	Vector p = m.TransformVectorWithW( point );
	if ( p.A[3] ) p /= p.A[3];
	RETURN_STRUCT( Vector, p );
}

void funcVecGetAngleBetween( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, from, Vector::ZEROS );
	GET_PARAMETER( Vector, to, Vector::ZEROS );
	FINISH_PARAMETERS;

	RETURN_FLOAT( MathUtils::VectorUtils::GetAngleDegBetweenVectors( from, to ) );
}

void funcVecGetAngleDegAroundAxis( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, dirA, Vector::ZEROS );
	GET_PARAMETER( Vector, dirB, Vector::ZEROS );
	GET_PARAMETER( Vector, axis, Vector::EZ );
	FINISH_PARAMETERS;

	RETURN_FLOAT( MathUtils::VectorUtils::GetAngleDegAroundAxis( dirA, dirB, axis ) );
}

void funcVecProjectPointToPlane( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, p1, Vector::ZEROS );
	GET_PARAMETER( Vector, p2, Vector::ZEROS );
	GET_PARAMETER( Vector, p3, Vector::ZEROS );
	GET_PARAMETER( Vector, toProject, Vector::ZEROS );
	FINISH_PARAMETERS;

	Vector res = Plane( p1, p2, p3 ).Project( toProject );
	RETURN_STRUCT( Vector, res );
}

/////////////////////////////////////////////
// EulerAngles functions
/////////////////////////////////////////////

void funcRotX( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( EulerAngles, rot, EulerAngles::ZEROS );
	FINISH_PARAMETERS;

	Vector ret;
	rot.ToAngleVectors( NULL, &ret, NULL );
	RETURN_STRUCT( Vector, ret );
}

void funcRotY( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( EulerAngles, rot, EulerAngles::ZEROS );
	FINISH_PARAMETERS;

	Vector ret;
	rot.ToAngleVectors( &ret, NULL, NULL );
	RETURN_STRUCT( Vector, ret );
}

void funcRotZ( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( EulerAngles, rot, EulerAngles::ZEROS );
	FINISH_PARAMETERS;

	Vector ret;
	rot.ToAngleVectors( NULL, NULL, &ret );
	RETURN_STRUCT( Vector, ret );
}

void funcRotForward( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( EulerAngles, rot, EulerAngles::ZEROS );
	FINISH_PARAMETERS;

	Vector ret;
	rot.ToAngleVectors( &ret, NULL, NULL );
	RETURN_STRUCT( Vector, ret );
}

void funcRotRight( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( EulerAngles, rot, EulerAngles::ZEROS );
	FINISH_PARAMETERS;

	Vector ret;
	rot.ToAngleVectors( NULL, &ret, NULL );
	RETURN_STRUCT( Vector, ret );
}

void funcRotUp( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( EulerAngles, rot, EulerAngles::ZEROS );
	FINISH_PARAMETERS;

	Vector ret;
	rot.ToAngleVectors( NULL, NULL, &ret );
	RETURN_STRUCT( Vector, ret );
}

void funcRotToMatrix( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( EulerAngles, rot, EulerAngles::ZEROS );
	FINISH_PARAMETERS;

	RETURN_STRUCT( Matrix, rot.ToMatrix() );
}

void funcRotAxes( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( EulerAngles, rot, EulerAngles::ZEROS );
	GET_PARAMETER_REF( Vector, forward, Vector::ZEROS );
	GET_PARAMETER_REF( Vector, right, Vector::ZEROS );
	GET_PARAMETER_REF( Vector, up, Vector::ZEROS );
	FINISH_PARAMETERS;

	rot.ToAngleVectors( &forward, &right, &up );

	RETURN_VOID();
}

void funcRotDot( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( EulerAngles, a, EulerAngles::ZEROS );
	GET_PARAMETER_OPT( EulerAngles, b, EulerAngles::ZEROS );
	FINISH_PARAMETERS;

	Vector forwardA = Vector::ZEROS;
	a.ToAngleVectors( &forwardA, NULL, NULL );

	Vector forwardB = Vector::ZEROS;
	b.ToAngleVectors( &forwardB, NULL, NULL );

	RETURN_FLOAT( Vector::Dot3( forwardA, forwardB ) );
}

void funcRotRand( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, rotMin, 0.0f );
	GET_PARAMETER( Float, rotMax, 0.0f );
	FINISH_PARAMETERS;

	EulerAngles rot	= EulerAngles::ZEROS;
	rot.Pitch		= GScriptingSystem->GetRandomNumberGenerator().Get< Float >( rotMin , rotMax );
	rot.Yaw			= GScriptingSystem->GetRandomNumberGenerator().Get< Float >( rotMin , rotMax );

	RETURN_STRUCT( EulerAngles, rot );
}

/////////////////////////////////////////////
// Matrix functions
/////////////////////////////////////////////

void funcMatrixIdentity( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_STRUCT( Matrix, Matrix::IDENTITY );
}

void funcMatrixBuiltTranslation( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, move, Vector::ZEROS );
	FINISH_PARAMETERS;

	Matrix mat = Matrix::IDENTITY;
	mat.SetTranslation( move );
	RETURN_STRUCT( Matrix, mat );
}

void funcMatrixBuiltRotation( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( EulerAngles, rot, EulerAngles::ZEROS );
	FINISH_PARAMETERS;

	RETURN_STRUCT( Matrix, rot.ToMatrix() );
}

void funcMatrixBuiltScale( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, scale, Vector::ZEROS );
	FINISH_PARAMETERS;

	Matrix mat = Matrix::IDENTITY;
	mat.SetScale33( scale );
	RETURN_STRUCT( Matrix, mat );
}

void funcMatrixBuiltPreScale( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, scale, Vector::ZEROS );
	FINISH_PARAMETERS;

	Matrix mat = Matrix::IDENTITY;
	mat.SetPreScale33( scale );
	RETURN_STRUCT( Matrix, mat );
}

void funcMatrixBuiltTRS( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( Vector, move, Vector::ZEROS );
	GET_PARAMETER_OPT( EulerAngles, rot, EulerAngles::ZEROS );
	GET_PARAMETER_OPT( Vector, scale, Vector::ONES );
	FINISH_PARAMETERS;

	Matrix t = Matrix::IDENTITY;
	t.SetTranslation( move );

	Matrix r = rot.ToMatrix();

	Matrix s = Matrix::IDENTITY;
	s.SetScale33( scale );

	Matrix ret = s * r * t;
	RETURN_STRUCT( Matrix, ret );
}

void funcMatrixBuiltRTS( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( EulerAngles, rot, EulerAngles::ZEROS );
	GET_PARAMETER_OPT( Vector, move, Vector::ZEROS );
	GET_PARAMETER_OPT( Vector, scale, Vector::ONES );
	FINISH_PARAMETERS;

	Matrix t = Matrix::IDENTITY;
	t.SetTranslation( move );

	Matrix r = rot.ToMatrix();

	Matrix s = Matrix::IDENTITY;
	s.SetScale33( scale );

	Matrix ret = s * t * r;
	RETURN_STRUCT( Matrix, ret );
}

void funcMatrixBuildFromDirectionVector( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, dirVec, Vector::ZEROS );
	FINISH_PARAMETERS;

	Matrix ret;
	ret.BuildFromDirectionVector( dirVec );
	RETURN_STRUCT( Matrix, ret );
}

void funcMatrixGetTranslation( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Matrix, mat, Matrix::IDENTITY );
	FINISH_PARAMETERS;

	RETURN_STRUCT( Vector, mat.GetTranslation() );
}

void funcMatrixGetAxisX( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Matrix, mat, Matrix::IDENTITY );
	FINISH_PARAMETERS;

	RETURN_STRUCT( Vector, mat.GetAxisX() );
}

void funcMatrixGetAxisY( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Matrix, mat, Matrix::IDENTITY );
	FINISH_PARAMETERS;

	RETURN_STRUCT( Vector, mat.GetAxisY() );
}

void funcMatrixGetAxisZ( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Matrix, mat, Matrix::IDENTITY );
	FINISH_PARAMETERS;

	RETURN_STRUCT( Vector, mat.GetAxisZ() );
}

void funcMatrixGetDirectionVector( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Matrix, mat, Matrix::IDENTITY );
	FINISH_PARAMETERS;

	RETURN_STRUCT( Vector, mat.GetAxisY() );
}

void funcMatrixGetRotation( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Matrix, mat, Matrix::IDENTITY );
	FINISH_PARAMETERS;

	RETURN_STRUCT( EulerAngles, mat.ToEulerAnglesFull() );
}

void funcMatrixGetScale( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Matrix, mat, Matrix::IDENTITY );
	FINISH_PARAMETERS;

	RETURN_STRUCT( Vector, mat.GetScale33() );
}

void funcMatrixGetInverted( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Matrix, mat, Matrix::IDENTITY );
	FINISH_PARAMETERS;

	RETURN_STRUCT( Matrix, mat.Inverted() );
}


void funcSphereIntersectRay( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Sphere, sphere, Sphere( Vector::ZEROS, 0.0f ) );
	GET_PARAMETER( Vector, orign, Vector::ZEROS );
	GET_PARAMETER( Vector, dir, Vector::ZEROS );
	GET_PARAMETER_REF( Vector, enterPoint, Vector::ZEROS );
	GET_PARAMETER_REF( Vector, exitPoint, Vector::ZEROS );
	FINISH_PARAMETERS;

	RETURN_INT( sphere.IntersectRay( orign, dir, enterPoint, exitPoint ) );
}

void funcSphereIntersectEdge( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Sphere, sphere, Sphere( Vector::ZEROS, 0.0f ) );
	GET_PARAMETER( Vector, a, Vector::ZEROS );
	GET_PARAMETER( Vector, b, Vector::ZEROS );
	GET_PARAMETER_REF( Vector, ip0, Vector::ZEROS );
	GET_PARAMETER_REF( Vector, ip1, Vector::ZEROS );
	FINISH_PARAMETERS;

	RETURN_INT( sphere.IntersectEdge( a, b, ip0, ip1 ) );
}

void funcInt8ToInt( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int8, i, 0 );
	FINISH_PARAMETERS;

	RETURN_INT( Int32( i ) );
}

void funcIntToInt8( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, i, 0 );
	FINISH_PARAMETERS;

	RETURN_INT( Int8( i ) );
}

void funcIntToUint64( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, i, 0 );
	FINISH_PARAMETERS;

	RETURN_UINT64( Uint64( i ) );
}

void funcUint64ToInt( IScriptable*,  CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Uint64, i, 0 );
	FINISH_PARAMETERS;

	RETURN_INT( Int32( i ) );
}


#define NATIVE_MATH( x )	\
	NATIVE_GLOBAL_FUNCTION( #x, func##x );

void ExportCoreMathNatives()
{
	// Scalar natives
	NATIVE_MATH( Rand );
	NATIVE_MATH( RandRange );
	NATIVE_MATH( RandF );
	NATIVE_MATH( RandRangeF );
	NATIVE_MATH( RandNoiseF );
	
	NATIVE_MATH( RandDifferent );
	NATIVE_MATH( Abs );
	NATIVE_MATH( Min );
	NATIVE_MATH( Max );
	NATIVE_MATH( Clamp );
	NATIVE_MATH( Deg2Rad );
	NATIVE_MATH( Rad2Deg );
	NATIVE_MATH( AbsF );
	NATIVE_MATH( SinF );
	NATIVE_MATH( AsinF );
	NATIVE_MATH( CosF );
	NATIVE_MATH( AcosF );
	NATIVE_MATH( TanF );
	NATIVE_MATH( AtanF );
	NATIVE_MATH( ExpF );
	NATIVE_MATH( PowF );
	NATIVE_MATH( LogF );
	NATIVE_MATH( SqrtF );
	NATIVE_MATH( SqrF );
	NATIVE_MATH( CalcSeed );
	NATIVE_MATH( MinF );
	NATIVE_MATH( MaxF );
	NATIVE_MATH( ClampF );
	NATIVE_MATH( LerpF );
	NATIVE_MATH( CeilF );
	NATIVE_MATH( FloorF );
	NATIVE_MATH( RoundF );
	NATIVE_MATH( RoundFEx );
	NATIVE_MATH( ReinterpretIntAsFloat );

	// Angle functions
	NATIVE_MATH( AngleNormalize );
	NATIVE_MATH( AngleDistance );
	NATIVE_MATH( AngleApproach );

	// Vector natives
	NATIVE_MATH( VecDot2D );
	NATIVE_MATH( VecDot );
	NATIVE_MATH( VecCross );
	NATIVE_MATH( VecLength2D );
	NATIVE_MATH( VecLength );
	NATIVE_MATH( VecLengthSquared );
	NATIVE_MATH( VecNormalize2D );
	NATIVE_MATH( VecNormalize );
	NATIVE_MATH( VecRand2D );
	NATIVE_MATH( VecRand );
	NATIVE_MATH( VecMirror );
	NATIVE_MATH( VecDistance );
	NATIVE_MATH( VecDistanceSquared );
	NATIVE_MATH( VecDistance2D );
	NATIVE_MATH( VecDistanceSquared2D );
	NATIVE_MATH( VecDistanceToEdge );
	NATIVE_MATH( VecNearestPointOnEdge );
	NATIVE_MATH( VecToRotation );
	NATIVE_MATH( VecHeading );
	NATIVE_MATH( VecFromHeading );
	NATIVE_MATH( VecTransform );
	NATIVE_MATH( VecTransformDir );
	NATIVE_MATH( VecTransformH );
	NATIVE_MATH( VecGetAngleBetween );
	NATIVE_MATH( VecGetAngleDegAroundAxis );
	NATIVE_MATH( VecProjectPointToPlane );
	NATIVE_MATH( VecRotateAxis );
	NATIVE_MATH( VecApproach );

	// EulerAngles natives
	NATIVE_MATH( RotX );
	NATIVE_MATH( RotY );
	NATIVE_MATH( RotZ );
	NATIVE_MATH( RotForward );
	NATIVE_MATH( RotRight );
	NATIVE_MATH( RotUp );
	NATIVE_MATH( RotToMatrix );
	NATIVE_MATH( RotAxes );
	NATIVE_MATH( RotRand );
	NATIVE_MATH( RotDot );

	// Matrix natives
	NATIVE_MATH( MatrixIdentity );
	NATIVE_MATH( MatrixBuiltTranslation );
	NATIVE_MATH( MatrixBuiltRotation );
	NATIVE_MATH( MatrixBuiltScale );
	NATIVE_MATH( MatrixBuiltPreScale );
	NATIVE_MATH( MatrixBuiltTRS );
	NATIVE_MATH( MatrixBuiltRTS );
	NATIVE_MATH( MatrixBuildFromDirectionVector );
	NATIVE_MATH( MatrixGetTranslation );
	NATIVE_MATH( MatrixGetRotation );
	NATIVE_MATH( MatrixGetScale );
	NATIVE_MATH( MatrixGetAxisX );
	NATIVE_MATH( MatrixGetAxisY );
	NATIVE_MATH( MatrixGetAxisZ );
	NATIVE_MATH( MatrixGetDirectionVector );
	NATIVE_MATH( MatrixGetInverted );

	// Sphere functions
	NATIVE_MATH( SphereIntersectRay );
	NATIVE_MATH( SphereIntersectEdge );

	// Conversions
	NATIVE_MATH( Int8ToInt );
	NATIVE_MATH( IntToInt8 );
	NATIVE_MATH( IntToUint64 );
	NATIVE_MATH( Uint64ToInt );
}
