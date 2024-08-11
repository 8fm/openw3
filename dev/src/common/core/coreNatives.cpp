/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "object.h"
#include "engineQsTransform.h"
#include "math.h"
#include "scriptStackFrame.h"

#include "../redMath/redmathbase.h"


//////////////////////////////////////////////
// Engine Qs Transform functions			//
//////////////////////////////////////////////


/************************************************************************/
/* Identity                                                             */
/************************************************************************/
void funct_Identity( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	RED_UNUSED( context );
	RED_UNUSED( result );

	GET_PARAMETER_REF( EngineQsTransform, trans, EngineQsTransform() );
	FINISH_PARAMETERS;
	trans.Identity();
}

void funct_SetIdentity( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	RED_UNUSED( context );

	FINISH_PARAMETERS;
	RETURN_STRUCT( EngineQsTransform, EngineQsTransform( EngineQsTransform::IDENTITY ) );
}

/************************************************************************/
/*  Build                                                                */
/************************************************************************/
void funct_BuiltTrans( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	RED_UNUSED( context );

	GET_PARAMETER( Vector, move, Vector::ZEROS );
	FINISH_PARAMETERS;
	EngineQsTransform trans( EngineQsTransform::IDENTITY );
	trans.SetPosition( move );
	RETURN_STRUCT( EngineQsTransform, trans );
}

void funct_BuiltRotQuat( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	RED_UNUSED( context );

	GET_PARAMETER( Vector, quat, Vector::ZERO_3D_POINT );
	FINISH_PARAMETERS;
	EngineQsTransform trans( EngineQsTransform::IDENTITY );
	trans.SetRotation( quat );
	RETURN_STRUCT( EngineQsTransform, trans );
}

void funct_BuiltRotAngles( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	RED_UNUSED( context );

	GET_PARAMETER( Float, pitch, 0.f );
	GET_PARAMETER( Float, roll, 0.f );
	GET_PARAMETER( Float, yaw, 0.f );
	FINISH_PARAMETERS;
#ifdef USE_HAVOK_ANIMATION
	EulerAngles angles( roll, pitch, yaw );

	hkQuaternion quat;
	EulerAnglesToHavokQuaternion( angles, quat );

	Vector rot = TO_CONST_VECTOR_REF( quat.m_vec );

	EngineQsTransform trans( EngineQsTransform::IDENTITY );
	trans.SetRotation( rot );
#else
	RedEulerAngles angles( roll, pitch, yaw );

	RedQuaternion quat = angles.ToQuaternion();

	EngineQsTransform trans( EngineQsTransform::IDENTITY );
	trans.SetRotation( reinterpret_cast< const Vector& >( quat.Quat ) );
#endif
	RETURN_STRUCT( EngineQsTransform, trans );
}

void funct_BuiltScale( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	RED_UNUSED( context );

	GET_PARAMETER( Vector, scale, Vector::ZEROS );
	FINISH_PARAMETERS;
	EngineQsTransform trans( EngineQsTransform::IDENTITY );
	trans.SetScale( scale );
	RETURN_STRUCT( EngineQsTransform, trans );
}

/************************************************************************/
/* Set                                                                  */
/************************************************************************/
void funct_Trans( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	RED_UNUSED( context );
	RED_UNUSED( result );

	GET_PARAMETER_REF( EngineQsTransform, trans, EngineQsTransform() );
	GET_PARAMETER( Vector, move, Vector::ZEROS );
	FINISH_PARAMETERS;
	trans.SetPosition( move );
}

void funct_RotQuat( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	RED_UNUSED( context );
	RED_UNUSED( result );

	GET_PARAMETER_REF( EngineQsTransform, trans, EngineQsTransform() );
	GET_PARAMETER( Vector, quat, Vector::ZERO_3D_POINT );
	FINISH_PARAMETERS;
	trans.SetRotation( quat );
}

void funct_Scale( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	RED_UNUSED( context );
	RED_UNUSED( result );

	GET_PARAMETER_REF( EngineQsTransform, trans, EngineQsTransform() );
	GET_PARAMETER( Vector, scale, Vector::ZEROS );
	FINISH_PARAMETERS;
	trans.SetScale( scale );
}

void funct_SetTrans( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	RED_UNUSED( context );

	GET_PARAMETER( EngineQsTransform, trans, EngineQsTransform() );
	GET_PARAMETER( Vector, move, Vector::ZEROS );
	FINISH_PARAMETERS;
	trans.SetPosition( move );
	RETURN_STRUCT( EngineQsTransform, trans );
}

void funct_SetRotQuat( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	RED_UNUSED( context );

	GET_PARAMETER( EngineQsTransform, trans, EngineQsTransform() );
	GET_PARAMETER( Vector, quat, Vector::ZERO_3D_POINT );
	FINISH_PARAMETERS;
	trans.SetRotation( quat );
	RETURN_STRUCT( EngineQsTransform, trans );
}

void funct_SetScale( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	RED_UNUSED( context );

	GET_PARAMETER( EngineQsTransform, trans, EngineQsTransform() );
	GET_PARAMETER( Vector, scale, Vector::ZEROS );
	FINISH_PARAMETERS;
	trans.SetScale( scale );
	RETURN_STRUCT( EngineQsTransform, trans );
}

/************************************************************************/
/* Get                                                                  */
/************************************************************************/
void funct_GetTrans( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	RED_UNUSED( context );

	GET_PARAMETER( EngineQsTransform, trans, EngineQsTransform() );
	FINISH_PARAMETERS;
	RETURN_STRUCT( Vector, trans.GetPosition() );
}

void funct_GetRotQuat( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	RED_UNUSED( context );

	GET_PARAMETER( EngineQsTransform, trans, EngineQsTransform() );
	FINISH_PARAMETERS;
	RETURN_STRUCT( Vector, trans.GetRotation() );
}

void funct_GetScale( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	RED_UNUSED( context );

	GET_PARAMETER( EngineQsTransform, trans, EngineQsTransform() );
	FINISH_PARAMETERS;
	RETURN_STRUCT( Vector, trans.GetScale() );
}

/************************************************************************/
/* Mul                                                                  */
/************************************************************************/
void funct_SetMul( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	RED_UNUSED( context );

	GET_PARAMETER( EngineQsTransform, a, EngineQsTransform() );
	GET_PARAMETER( EngineQsTransform, b, EngineQsTransform() );
	FINISH_PARAMETERS;
#ifdef USE_HAVOK_ANIMATION
	hkQsTransform a1, b1, r1;
	EngineToHkQsTransform( a, a1 );
	EngineToHkQsTransform( b, b1 );

	r1.setMul( a1, b1 );

	EngineQsTransform r;
	HkToEngineQsTransform( r1, r );
#else
	RedQsTransform a1;
	RedQsTransform b1; 
	RedQsTransform r1;
	a1 = reinterpret_cast< const RedQsTransform& >( a );
	b1 = reinterpret_cast< const RedQsTransform& >( b );
	r1.SetMul( a1, b1 );

	EngineQsTransform r = reinterpret_cast< const EngineQsTransform& >( r1 );
#endif
	RETURN_STRUCT( EngineQsTransform, r );
}

void funct_SetMulMulInv( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	RED_UNUSED( context );

	GET_PARAMETER( EngineQsTransform, a, EngineQsTransform() );
	GET_PARAMETER( EngineQsTransform, b, EngineQsTransform() );
	FINISH_PARAMETERS;

#ifdef USE_HAVOK_ANIMATION
	hkQsTransform a1, b1, r1;
	EngineToHkQsTransform( a, a1 );
	EngineToHkQsTransform( b, b1 );

	r1.setMulMulInverse( a1, b1 );

	EngineQsTransform r;
	HkToEngineQsTransform( r1, r );
#else
	RedQsTransform a1;
	RedQsTransform b1; 
	RedQsTransform r1;
	a1 = reinterpret_cast< const RedQsTransform& >( a );
	b1 = reinterpret_cast< const RedQsTransform& >( b );
	r1.SetMulMulInverse( a1, b1 );

	EngineQsTransform r = reinterpret_cast< const EngineQsTransform& >( r1 );
#endif
	RETURN_STRUCT( EngineQsTransform, r );
}

void funct_SetMulInvMul( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	RED_UNUSED( context );

	GET_PARAMETER( EngineQsTransform, a, EngineQsTransform() );
	GET_PARAMETER( EngineQsTransform, b, EngineQsTransform() );
	FINISH_PARAMETERS;
#ifdef USE_HAVOK_ANIMATION
	hkQsTransform a1, b1, r1;
	EngineToHkQsTransform( a, a1 );
	EngineToHkQsTransform( b, b1 );

	r1.setMulInverseMul( a1, b1 );

	EngineQsTransform r;
	HkToEngineQsTransform( r1, r );
#else
	RedQsTransform a1;
	RedQsTransform b1;
	RedQsTransform r1;
	a1 = reinterpret_cast< const RedQsTransform& >( a );
	b1 = reinterpret_cast< const RedQsTransform& >( b );
	r1.SetMulInverseMul( a1, b1 );
	EngineQsTransform r = reinterpret_cast< const EngineQsTransform& >( r1 );
#endif
	RETURN_STRUCT( EngineQsTransform, r );
}

/************************************************************************/
/* Based on two transform                                               */
/************************************************************************/

void funct_SetInterpolate( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	RED_UNUSED( context );

	GET_PARAMETER( EngineQsTransform, a, EngineQsTransform() );
	GET_PARAMETER( EngineQsTransform, b, EngineQsTransform() );
	GET_PARAMETER( Float, w, 0.f );
	FINISH_PARAMETERS;
#ifdef USE_HAVOK_ANIMATION
	hkQsTransform a1, b1, r1;
	EngineToHkQsTransform( a, a1 );
	EngineToHkQsTransform( b, b1 );

	r1.setInterpolate4( a1, b1, w );

	EngineQsTransform r;
	HkToEngineQsTransform( r1, r );
#else
	RedQsTransform a1;
	RedQsTransform b1;
	RedQsTransform r1;
	a1 = reinterpret_cast< const RedQsTransform& >( a );
	b1 = reinterpret_cast< const RedQsTransform& >( b );
	r1.Lerp( a1, b1, w );
	EngineQsTransform r = reinterpret_cast< const EngineQsTransform& >( r1 );
#endif
	RETURN_STRUCT( EngineQsTransform, r );
}

void funct_IsEqual( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	RED_UNUSED( context );

	GET_PARAMETER( EngineQsTransform, a, EngineQsTransform() );
	GET_PARAMETER( EngineQsTransform, b, EngineQsTransform() );
	GET_PARAMETER_OPT( Float, eps, 0.01f );
	FINISH_PARAMETERS;
#ifdef USE_HAVOK_ANIMATION
	hkQsTransform a1, b1;
	EngineToHkQsTransform( a, a1 );
	EngineToHkQsTransform( b, b1 );

	RETURN_BOOL( a1.isApproximatelyEqual( b1, eps ) );
#else
	RedQsTransform a1;
	RedQsTransform b1;
	a1 = reinterpret_cast< const RedQsTransform& >( a );
	b1 = reinterpret_cast< const RedQsTransform& >( b );

	RETURN_BOOL( a1.IsAlmostEqual( b1, eps ) );
#endif
}

/************************************************************************/
/* Based on one transform                                               */
/************************************************************************/

void funct_Inv( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	RED_UNUSED( context );
	RED_UNUSED( result );

	GET_PARAMETER_REF( EngineQsTransform, a, EngineQsTransform() );
	FINISH_PARAMETERS;
#ifdef USE_HAVOK_ANIMATION
	hkQsTransform a1;
	hkQsTransform r1;
	EngineToHkQsTransform( a, a1 );

	r1.setInverse( a1 );

	HkToEngineQsTransform( r1, a );
#else
	RedQsTransform a1;
	RedQsTransform r1;
	a1 = reinterpret_cast< const RedQsTransform& >( a );

	r1.SetInverse( a1 );
	a = reinterpret_cast< const EngineQsTransform& >( r1 );
#endif
}

void funct_SetInv( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	RED_UNUSED( context );

	GET_PARAMETER( EngineQsTransform, a, EngineQsTransform() );
	FINISH_PARAMETERS;
#ifdef USE_HAVOK_ANIMATION
	hkQsTransform a1, r1;
	EngineToHkQsTransform( a, a1 );

	r1.setInverse( a1 );

	EngineQsTransform r;
	HkToEngineQsTransform( r1, r );
#else
	RedQsTransform a1; 
	RedQsTransform r1;
	a1 = reinterpret_cast< const RedQsTransform& >( a );

	r1.SetInverse( a1 );

	EngineQsTransform r = reinterpret_cast< const EngineQsTransform& >( r1 );
#endif
	RETURN_STRUCT( EngineQsTransform, r );
}

void funct_NormalizeQuat( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	RED_UNUSED( context );
	RED_UNUSED( result );

	GET_PARAMETER_REF( EngineQsTransform, a, EngineQsTransform() );
	FINISH_PARAMETERS;
#ifdef USE_HAVOK_ANIMATION
	hkQsTransform a1;
	EngineToHkQsTransform( a, a1 );

	a1.m_rotation.normalize();

	EngineQsTransform r;
	HkToEngineQsTransform( a1, a );
#else
	RedQsTransform a1;
	a1 = reinterpret_cast< const RedQsTransform& >( a );

	a1.Rotation.Normalize();

	a = reinterpret_cast< const EngineQsTransform& >( a1 );
#endif
}

void funct_BlendNormalize( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	RED_UNUSED( context );
	RED_UNUSED( result );

	GET_PARAMETER_REF( EngineQsTransform, a, EngineQsTransform() );
	GET_PARAMETER( Float, w, 0.f );
	FINISH_PARAMETERS;
#ifdef USE_HAVOK_ANIMATION
	hkQsTransform a1;
	EngineToHkQsTransform( a, a1 );

	a1.blendNormalize( w );

	EngineQsTransform r;
	HkToEngineQsTransform( a1, a );
#else
	RedQsTransform a1;
	a1 = reinterpret_cast< const RedQsTransform& >( a );

	a1.BlendNormalize( w );

	a = reinterpret_cast< const EngineQsTransform& >( a1 );
#endif
}

void funct_IsOk( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	RED_UNUSED( context );

	GET_PARAMETER( EngineQsTransform, a, EngineQsTransform() );
	FINISH_PARAMETERS;
#ifdef USE_HAVOK_ANIMATION
	hkQsTransform a1;
	EngineToHkQsTransform( a, a1 );

	RETURN_BOOL( a1.isOk() );
#else
	RedQsTransform a1;
	a1 = reinterpret_cast< const RedQsTransform& >( a );

	RETURN_BOOL( a1.IsOk() );
#endif
}

//////////////////////////////////////////////
// Quaternion functions						//
//////////////////////////////////////////////

void funcq_SetIdentity( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	RED_UNUSED( context );

	FINISH_PARAMETERS;
	RETURN_STRUCT( Vector, Vector::ZERO_3D_POINT );
}

void funcq_Identity( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	RED_UNUSED( context );
	RED_UNUSED( result );

	GET_PARAMETER_REF( Vector, a, Vector::ZERO_3D_POINT );
	FINISH_PARAMETERS;
	a = Vector::ZERO_3D_POINT;
}

void funcq_SetInv( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	RED_UNUSED( context );

	GET_PARAMETER( Vector, a, Vector::ZERO_3D_POINT );
	FINISH_PARAMETERS;
#ifdef USE_HAVOK_ANIMATION
	hkQuaternion a1, r1;
	a1.m_vec = TO_CONST_HK_VECTOR_REF( a );

	r1.setInverse( a1 );

	Vector r = TO_CONST_VECTOR_REF( r1.m_vec );
#else
	RedQuaternion a1;
	RedQuaternion r1;
	a1.Quat = reinterpret_cast< const RedVector4& >( a );

	r1.SetInverse( a1 );

	Vector r = reinterpret_cast< const Vector& >( r1.Quat );
#endif
	RETURN_STRUCT( Vector, r );
}

void funcq_Inv( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	RED_UNUSED( context );
	RED_UNUSED( result );

	GET_PARAMETER_REF( Vector, a, Vector::ZERO_3D_POINT );
	FINISH_PARAMETERS;
#ifdef USE_HAVOK_ANIMATION
	hkQuaternion a1, r1;
	a1.m_vec = TO_CONST_HK_VECTOR_REF( a );

	r1.setInverse( a1 );

	a = TO_CONST_VECTOR_REF( r1.m_vec );
#else
	RedQuaternion a1;
	RedQuaternion r1;
	a1.Quat = reinterpret_cast< const RedVector4& >( a );

	r1.SetInverse( a1 );

	a = reinterpret_cast< const Vector& >( r1.Quat );
#endif
}

void funcq_SetNormalize( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	RED_UNUSED( context );

	GET_PARAMETER( Vector, a, Vector::ZERO_3D_POINT );
	FINISH_PARAMETERS;
#ifdef USE_HAVOK_ANIMATION
	hkQuaternion a1, r1;
	a1.m_vec = TO_CONST_HK_VECTOR_REF( a );

	r1.normalize();

	Vector r = TO_CONST_VECTOR_REF( r1.m_vec );
#else
	RedQuaternion a1;
	RedQuaternion r1;
	a1.Quat = reinterpret_cast< const RedVector4& >( a );

	r1.Normalize();

	Vector r = reinterpret_cast< const Vector& >( r1.Quat );
#endif
	RETURN_STRUCT( Vector, r );
}

void funcq_Normalize( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	RED_UNUSED( context );

	GET_PARAMETER_REF( Vector, a, Vector::ZERO_3D_POINT );
	FINISH_PARAMETERS;
#ifdef USE_HAVOK_ANIMATION
	hkQuaternion a1, r1;
	a1.m_vec = TO_CONST_HK_VECTOR_REF( a );

	r1.normalize();

	Vector r = TO_CONST_VECTOR_REF( r1.m_vec );
#else
	RedQuaternion a1;
	RedQuaternion r1;
	a1.Quat = reinterpret_cast< const RedVector4& >( a );

	r1.Normalize();

	Vector r = reinterpret_cast< const Vector& >( r1.Quat );
#endif
	RETURN_STRUCT( Vector, r );
}

void funcq_SetMul( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	RED_UNUSED( context );

	GET_PARAMETER( Vector, a, Vector::ZERO_3D_POINT );
	GET_PARAMETER( Vector, b, Vector::ZERO_3D_POINT );
	FINISH_PARAMETERS;
#ifdef USE_HAVOK_ANIMATION
	hkQuaternion a1, b1, r1;
	a1.m_vec = TO_CONST_HK_VECTOR_REF( a );
	b1.m_vec = TO_CONST_HK_VECTOR_REF( b );

	r1.setMul( a1, b1 );

	Vector r = TO_CONST_VECTOR_REF( r1.m_vec );
#else
	RedQuaternion a1;
	RedQuaternion b1;
	RedQuaternion r1;
	a1.Quat = reinterpret_cast< const RedVector4& >( a );
	b1.Quat = reinterpret_cast< const RedVector4& >( b );

	r1.SetMul( a1, b1 );

	Vector r = reinterpret_cast< const Vector& >( r1.Quat );
#endif
	RETURN_STRUCT( Vector, r );
}

void funcq_SetMulMulInv( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	RED_UNUSED( context );

	GET_PARAMETER( Vector, a, Vector::ZERO_3D_POINT );
	GET_PARAMETER( Vector, b, Vector::ZERO_3D_POINT );
	FINISH_PARAMETERS;
#ifdef USE_HAVOK_ANIMATION
	hkQuaternion a1, b1, r1;
	a1.m_vec = TO_CONST_HK_VECTOR_REF( a );
	b1.m_vec = TO_CONST_HK_VECTOR_REF( b );

	r1.setMulInverse( a1, b1 );

	Vector r = TO_CONST_VECTOR_REF( r1.m_vec );
#else
	RedQuaternion a1;
	RedQuaternion b1;
	RedQuaternion r1;
	a1.Quat = reinterpret_cast< const RedVector4& >( a );
	b1.Quat = reinterpret_cast< const RedVector4& >( b );

	r1.SetMulInverse( a1, b1 );

	Vector r = reinterpret_cast< const Vector& >( r1.Quat );
#endif
	RETURN_STRUCT( Vector, r );
}

void funcq_SetMulInvMul( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	RED_UNUSED( context );

	GET_PARAMETER( Vector, a, Vector::ZERO_3D_POINT );
	GET_PARAMETER( Vector, b, Vector::ZERO_3D_POINT );
	FINISH_PARAMETERS;
#ifdef USE_HAVOK_ANIMATION
	hkQuaternion a1, b1, r1;
	a1.m_vec = TO_CONST_HK_VECTOR_REF( a );
	b1.m_vec = TO_CONST_HK_VECTOR_REF( b );

	r1.setInverseMul( a1, b1 );

	Vector r = TO_CONST_VECTOR_REF( r1.m_vec );
#else
	RedQuaternion a1;
	RedQuaternion b1;
	RedQuaternion r1;
	a1.Quat = reinterpret_cast< const RedVector4& >( a );
	b1.Quat = reinterpret_cast< const RedVector4& >( b );

	r1.SetInverseMul( a1, b1 );

	Vector r = reinterpret_cast< const Vector& >( r1.Quat );
#endif
	RETURN_STRUCT( Vector, r );
}

void funcq_SetShortestRotation( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	RED_UNUSED( context );

	GET_PARAMETER( Vector, from, Vector::ZERO_3D_POINT );
	GET_PARAMETER( Vector, to, Vector::ZERO_3D_POINT );
	FINISH_PARAMETERS;
#ifdef USE_HAVOK_ANIMATION
	hkVector4 v1, v2;
	hkQuaternion r1;
	v1 = TO_CONST_HK_VECTOR_REF( from );
	v2 = TO_CONST_HK_VECTOR_REF( to );

	v1.normalize3();
	v2.normalize3();

	r1.setShortestRotation( v1, v2 );

	Vector r = TO_CONST_VECTOR_REF( r1.m_vec );
#else
	RedVector4 v1;
	RedVector4 v2;
	RedQuaternion r1;
	v1 = reinterpret_cast< const RedVector4& >( from );
	v2 = reinterpret_cast< const RedVector4& >( to );

	v1.Normalize3();
	v2.Normalize3();

	r1.SetShortestRotation( v1, v2 );

	Vector r = reinterpret_cast< const Vector& >( r1.Quat );
#endif
	RETURN_STRUCT( Vector, r );
}

void funcq_SetShortestRotationDamped( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	RED_UNUSED( context );

	GET_PARAMETER( Vector, from, Vector::ZERO_3D_POINT );
	GET_PARAMETER( Vector, to, Vector::ZERO_3D_POINT );
	GET_PARAMETER( Float, w, 0.f );
	FINISH_PARAMETERS;
#ifdef USE_HAVOK_ANIMATION
	hkVector4 v1, v2;
	hkQuaternion r1;
	v1 = TO_CONST_HK_VECTOR_REF( from );
	v2 = TO_CONST_HK_VECTOR_REF( to );

	v1.normalize3();
	v2.normalize3();

	r1.setShortestRotationDamped( w, v1, v2 );

	Vector r = TO_CONST_VECTOR_REF( r1.m_vec );
#else
	RedVector4 v1;
	RedVector4 v2;
	RedQuaternion r1;
	v1 = reinterpret_cast< const RedVector4& >( from );
	v2 = reinterpret_cast< const RedVector4& >( to );

	v1.Normalize3();
	v2.Normalize3();

	r1.SetShortestRotationDamped( w, v1, v2 );

	Vector r = reinterpret_cast< const Vector& >( r1.Quat );
#endif
	RETURN_STRUCT( Vector, r );
}

void funcq_SetAxisAngle( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	RED_UNUSED( context );

	GET_PARAMETER( Vector, vec, Vector::ZERO_3D_POINT );
	GET_PARAMETER( Float, angle, 0.f );
	FINISH_PARAMETERS;
#ifdef USE_HAVOK_ANIMATION
	hkVector4 v1;
	hkQuaternion r1;
	v1 = TO_CONST_HK_VECTOR_REF( vec );

	v1.normalize3();

	r1.setAxisAngle( v1, angle );

	Vector r = TO_CONST_VECTOR_REF( r1.m_vec );
#else
	RedVector4 v1;
	RedQuaternion r1;
	v1 = reinterpret_cast< const RedVector4& >( vec );

	v1.Normalize3();

	r1.SetAxisAngle( v1, angle );

	Vector r = reinterpret_cast< const Vector& >( r1.Quat );
#endif
	RETURN_STRUCT( Vector, r );
}

void funcq_RemoveAxisComponent( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	RED_UNUSED( context );
	RED_UNUSED( result );

	GET_PARAMETER_REF( Vector, quat, Vector::ZERO_3D_POINT );
	GET_PARAMETER( Vector, vec, Vector::ZERO_3D_POINT );
	FINISH_PARAMETERS;
#ifdef USE_HAVOK_ANIMATION
	hkVector4 v1;
	hkQuaternion r1;
	r1.m_vec = TO_CONST_HK_VECTOR_REF( quat );
	v1 = TO_CONST_HK_VECTOR_REF( vec );

	r1.removeAxisComponent( v1 );

	quat = TO_CONST_VECTOR_REF( r1.m_vec );
#else
	RedVector4 v1;
	RedQuaternion r1;
	r1.Quat = reinterpret_cast< const RedVector4& >( quat );
	v1 = reinterpret_cast< const RedVector4& >( vec );

	r1.RemoveAxisComponent( v1 );

	quat = reinterpret_cast< const Vector&>( r1.Quat );
#endif
}

void funcq_DecomposeAxis( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	RED_UNUSED( context );

	GET_PARAMETER( Vector, quat, Vector::ZERO_3D_POINT );
	GET_PARAMETER( Vector, vec, Vector::ZERO_3D_POINT );
	FINISH_PARAMETERS;
#ifdef USE_HAVOK_ANIMATION
	hkVector4 v1;
	hkQuaternion r1, rest;
	Float angle;

	r1.m_vec = TO_CONST_HK_VECTOR_REF( quat );
	v1 = TO_CONST_HK_VECTOR_REF( vec );

	r1.decomposeRestAxis( v1, rest, angle );
#else
	RedVector4 v1;
	RedQuaternion r1;
	RedQuaternion rest;
	Float angle;

	r1.Quat = reinterpret_cast< const RedVector4& >( quat );
	v1 = reinterpret_cast< const RedVector4& >( vec );

	r1.DecomposeRestAxis( v1, rest, angle );
#endif
	RETURN_FLOAT( angle );
}

void funcq_SetSlerp( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	RED_UNUSED( context );

	GET_PARAMETER( Vector, a, Vector::ZERO_3D_POINT );
	GET_PARAMETER( Vector, b, Vector::ZERO_3D_POINT );
	GET_PARAMETER( Float, w, 0.f );
	FINISH_PARAMETERS;
#ifdef USE_HAVOK_ANIMATION
	hkQuaternion a1, b1, r1;
	a1.m_vec = TO_CONST_HK_VECTOR_REF( a );
	b1.m_vec = TO_CONST_HK_VECTOR_REF( b );

	r1.setSlerp( a1, b1, w );

	Vector r = TO_CONST_VECTOR_REF( r1.m_vec );
#else
	RedQuaternion a1;
	RedQuaternion b1;
	RedQuaternion r1;
	a1.Quat = reinterpret_cast< const RedVector4& >( a );
	b1.Quat = reinterpret_cast< const RedVector4& >( b );

	r1.SetSlerp( a1, b1, w );

	Vector r = reinterpret_cast< const Vector& >( r1.Quat );
#endif
	RETURN_STRUCT( Vector, r );
}

void funcq_GetAngle( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	RED_UNUSED( context );

	GET_PARAMETER( Vector, a, Vector::ZERO_3D_POINT );
	FINISH_PARAMETERS;
#ifdef USE_HAVOK_ANIMATION
	hkQuaternion a1;
	a1.m_vec = TO_CONST_HK_VECTOR_REF( a );

	RETURN_FLOAT( a1.getAngle() );
#else
	RedQuaternion a1;
	a1.Quat = reinterpret_cast< const RedVector4& >( a );

	RETURN_FLOAT( a1.GetAngle() );
#endif
}

void funcq_GetAxis( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	RED_UNUSED( context );

	GET_PARAMETER( Vector, a, Vector::ZERO_3D_POINT );
	FINISH_PARAMETERS;
#ifdef USE_HAVOK_ANIMATION
	hkQuaternion a1;
	a1.m_vec = TO_CONST_HK_VECTOR_REF( a );

	hkVector4 vec;
	a1.getAxis( vec );

	Vector r = TO_CONST_VECTOR_REF( vec );
#else
	RedQuaternion a1;
	a1.Quat = reinterpret_cast< const RedVector4& >( a );

	RedVector4 vec = a1.GetAxis();

	Vector r = reinterpret_cast< const Vector& >( vec );
#endif
	RETURN_STRUCT( Vector, r );
}

//////////////////////////////////////////////
// Vector functions							//
//////////////////////////////////////////////

void funcv_SetInterpolate( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	RED_UNUSED( context );

	GET_PARAMETER( Vector, a, Vector::ZERO_3D_POINT );
	GET_PARAMETER( Vector, b, Vector::ZERO_3D_POINT );
	GET_PARAMETER( Float, w, 0.f );
	FINISH_PARAMETERS;

	Vector r = Vector::Interpolate( a, b, w );

	RETURN_STRUCT( Vector, r );
}

void funcv_SetRotatedDir( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	RED_UNUSED( context );

	GET_PARAMETER( Vector, quat, Vector::ZERO_3D_POINT );
	GET_PARAMETER( Vector, vec, Vector::ZERO_3D_POINT );
	FINISH_PARAMETERS;
#ifdef USE_HAVOK_ANIMATION
	hkQuaternion quat1;
	hkVector4 vec1, r1;

	quat1.m_vec = TO_CONST_HK_VECTOR_REF( quat );
	vec1 = TO_CONST_HK_VECTOR_REF( vec );

	r1.setRotatedDir( quat1, vec1 );

	Vector r = TO_CONST_VECTOR_REF( r1 );
#else
	RedQuaternion quat1;
	RedVector4 vec1;
	RedVector4 r1;

	quat1.Quat = reinterpret_cast< const RedVector4& >( quat );
	vec1 = reinterpret_cast< const RedVector4& >( vec );

	r1.RotateDirection( quat1, vec1 );

	Vector r = reinterpret_cast< const Vector& >( r1 );
#endif
	RETURN_STRUCT( Vector, r );
}

void funcv_SetTransformedPos( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	RED_UNUSED( context );

	GET_PARAMETER( EngineQsTransform, trans, EngineQsTransform() );
	GET_PARAMETER( Vector, vec, Vector::ZERO_3D_POINT );
	FINISH_PARAMETERS;
#ifdef USE_HAVOK_ANIMATION
	hkQsTransform trans1;
	hkVector4 vec1, r1;

	EngineToHkQsTransform( trans, trans1 );
	vec1 = TO_CONST_HK_VECTOR_REF( vec );

	r1.setTransformedPos( trans1, vec1 );

	Vector r = TO_CONST_VECTOR_REF( r1 );
#else
	RedQsTransform trans1;
	RedVector4 vec1;
	RedVector4 r1;

	trans1 = reinterpret_cast< const RedQsTransform& >( trans );
	vec1 = reinterpret_cast< const RedVector4& >( vec );

	r1.SetTransformedPos( trans1, vec1 );

	Vector r = reinterpret_cast< const Vector& >( r1 );
#endif
	RETURN_STRUCT( Vector, r );
}

void funcv_ZeroElement( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	RED_UNUSED( context );
	RED_UNUSED( result );

	GET_PARAMETER_REF( Vector, a, Vector::ZERO_3D_POINT );
	GET_PARAMETER( Int32, i, 0 );
	FINISH_PARAMETERS;

	a.ZeroElement( i );
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#define ENGINE_NATIVE_MATH( x )	\
	NATIVE_GLOBAL_FUNCTION( #x, func##x );

void RegisterEngineScriptFunctions()
{
	// Engine Qs Transform
	ENGINE_NATIVE_MATH( t_Identity );
	ENGINE_NATIVE_MATH( t_SetIdentity );
	ENGINE_NATIVE_MATH( t_BuiltTrans );
	ENGINE_NATIVE_MATH( t_BuiltRotQuat );
	ENGINE_NATIVE_MATH( t_BuiltRotAngles );
	ENGINE_NATIVE_MATH( t_BuiltScale );
	ENGINE_NATIVE_MATH( t_Trans );
	ENGINE_NATIVE_MATH( t_RotQuat );
	ENGINE_NATIVE_MATH( t_Scale );
	ENGINE_NATIVE_MATH( t_SetTrans );
	ENGINE_NATIVE_MATH( t_SetRotQuat );
	ENGINE_NATIVE_MATH( t_SetScale );
	ENGINE_NATIVE_MATH( t_GetTrans );
	ENGINE_NATIVE_MATH( t_GetRotQuat );
	ENGINE_NATIVE_MATH( t_GetScale );
	ENGINE_NATIVE_MATH( t_SetMul );
	ENGINE_NATIVE_MATH( t_SetMulMulInv );
	ENGINE_NATIVE_MATH( t_SetMulInvMul );
	ENGINE_NATIVE_MATH( t_SetInterpolate );
	ENGINE_NATIVE_MATH( t_IsEqual );
	ENGINE_NATIVE_MATH( t_SetInv );
	ENGINE_NATIVE_MATH( t_Inv );
	ENGINE_NATIVE_MATH( t_NormalizeQuat );
	ENGINE_NATIVE_MATH( t_BlendNormalize );
	ENGINE_NATIVE_MATH( t_IsOk );

	// Quaternion
	ENGINE_NATIVE_MATH( q_SetIdentity );
	ENGINE_NATIVE_MATH( q_Identity );
	ENGINE_NATIVE_MATH( q_SetInv );
	ENGINE_NATIVE_MATH( q_Inv );
	ENGINE_NATIVE_MATH( q_SetNormalize );
	ENGINE_NATIVE_MATH( q_Normalize );
	ENGINE_NATIVE_MATH( q_SetMul );
	ENGINE_NATIVE_MATH( q_SetMulMulInv );
	ENGINE_NATIVE_MATH( q_SetMulInvMul );
	ENGINE_NATIVE_MATH( q_SetShortestRotation );
	ENGINE_NATIVE_MATH( q_SetShortestRotationDamped );
	ENGINE_NATIVE_MATH( q_SetAxisAngle );
	ENGINE_NATIVE_MATH( q_RemoveAxisComponent );
	ENGINE_NATIVE_MATH( q_DecomposeAxis );
	ENGINE_NATIVE_MATH( q_SetSlerp );
	ENGINE_NATIVE_MATH( q_GetAngle );
	ENGINE_NATIVE_MATH( q_GetAxis );

	// Vector
	ENGINE_NATIVE_MATH( v_SetInterpolate );
	ENGINE_NATIVE_MATH( v_SetRotatedDir );
	ENGINE_NATIVE_MATH( v_SetTransformedPos );
	ENGINE_NATIVE_MATH( v_ZeroElement );
}
