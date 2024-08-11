/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 *
 * [1] Wrappers to make it easier to implement math using Havok and Red math.
 * [2] Utility functions for drawing debug info.
 * [3] Interpolation utils
 * [4] Sample helpers
 */

#pragma once

#include "../engine/renderFrame.h"
#include "animMath.h"
#include "renderFrame.h"
#include "cacheBehaviorGraphOutput.h"
#include "skeletalAnimation.h"
#include "skeletalAnimationEntry.h"
#include "behaviorGraphOutput.h"

//////////////////////////////////////////////////////////////////////////

RED_FORCE_INLINE static AnimFloat Sqrt( const AnimFloat& val )
{
#ifdef USE_HAVOK_ANIMATION
	return hkMath::sqrt( val );
#else
	return Red::Math::MSqrt( val );
#endif
}

RED_FORCE_INLINE static AnimFloat GetLength3( const AnimVector4& from )
{
#ifdef USE_HAVOK_ANIMATION
	return from.length3();
#else
	return from.Length3();
#endif
}

RED_FORCE_INLINE static AnimFloat GetDist3( const AnimVector4& a, const AnimVector4& b )
{
#ifdef USE_HAVOK_ANIMATION
	AnimVector4 c;
	c.setSub4( a, b );
	return c.length3();
#else
	return Sub( a, b ).Length3();
#endif
}

RED_FORCE_INLINE static void MulVector( AnimVector4& vector, const AnimFloat& mul )
{
#ifdef USE_HAVOK_ANIMATION
	vector.mul4( mul );
#else
	SetMul( vector,  mul );
#endif
}

RED_FORCE_INLINE static void AddVector( AnimVector4& vector, const AnimVector4& add )
{
#ifdef USE_HAVOK_ANIMATION
	vector.add4( add );
#else
	SetAdd( vector, add );
#endif
}

RED_FORCE_INLINE static void AddVectorMul( AnimVector4& vector, const AnimVector4& add, const AnimFloat& mul )
{
#ifdef USE_HAVOK_ANIMATION
	vector.addMul4( mul, add );
#else
	SetAdd( vector, Mul( add, mul ) );
#endif
}

RED_FORCE_INLINE static void SubVectorMul( AnimVector4& vector, const AnimVector4& sub, const AnimFloat& mul )
{
#ifdef USE_HAVOK_ANIMATION
	vector.subMul4( mul, sub );
#else
	SetSub( vector, Mul ( sub,  mul ) );
#endif
}

RED_FORCE_INLINE static void SetCross( AnimVector4& dest, const AnimVector4& a, const AnimVector4& b )
{
#ifdef USE_HAVOK_ANIMATION
	dest.setCross( a, b );
#else
	dest = Cross( a, b );
#endif
}

RED_FORCE_INLINE static AnimFloat Dot3( const AnimVector4& a, const AnimVector4& b )
{
#ifdef USE_HAVOK_ANIMATION
	return a.dot3( b );
#else
	return RedMath::SIMD::Dot3( a, b );
#endif
}

RED_FORCE_INLINE static AnimVector4 PointToPoint( const AnimVector4& from, const AnimVector4& to )
{
#ifdef USE_HAVOK_ANIMATION
	hkVector4 ret;
	ret.setSub4( to, from );
	return ret;
#else
	return Sub( to, from );
#endif
}

RED_FORCE_INLINE static void Normalize3( AnimVector4& vector )
{
#ifdef USE_HAVOK_ANIMATION
	vector.normalize3();
#else
	vector.Normalize3();
#endif
}

RED_FORCE_INLINE static void SetMulTransform( AnimQsTransform& dest, const AnimQsTransform& a, const AnimQsTransform& b )
{
#ifdef USE_HAVOK_ANIMATION
	dest.setMul( a, b );
#else
	dest.SetMul( a, b );
#endif
}

/** dest = a * b^-1 */
RED_FORCE_INLINE static void SetMulMulInverseTransform( AnimQsTransform& dest, const AnimQsTransform& a, const AnimQsTransform& b )
{
#ifdef USE_HAVOK_ANIMATION
	dest.setMulMulInverse( a, b );
#else
	dest.SetMulMulInverse( a, b );
#endif
}

/** dest = a^-1 * b */
RED_FORCE_INLINE static void SetMulInverseMulTransform( AnimQsTransform& dest, const AnimQsTransform& a, const AnimQsTransform& b )
{
#ifdef USE_HAVOK_ANIMATION
	dest.setMulInverseMul( a, b );
#else
	dest.SetMulInverseMul( a, b );
#endif
}

/** dest = a*(1.0 - blend) + b*blend, blend goes from 0 (a) to 1 (b) */
RED_FORCE_INLINE static void BlendTwoTransforms( AnimQsTransform& dest, const AnimQsTransform& a, const AnimQsTransform& b, const AnimFloat& blend )
{
#ifdef USE_HAVOK_ANIMATION
	hkQsTransform out;
	out.setZero();
	out.blendAddMul( a, 1.0f - blend );
	out.blendAddMul( b, blend );
	out.blendNormalize();
	dest = out;
#else
	RedQsTransform out;
	out.SetZero();
	out.BlendAddMul( a, 1.0f - blend );
	out.BlendAddMul( b, blend );
	out.BlendNormalize();
	dest = out;
#endif
}

RED_FORCE_INLINE static AnimVector4 GetTranslation( const AnimQsTransform& from )
{
#ifdef USE_HAVOK_ANIMATION
	return from.getTranslation();
#else
	return from.GetTranslation();
#endif
}

RED_FORCE_INLINE static void SetTranslation( AnimQsTransform& transform, const AnimVector4& translation )
{
#ifdef USE_HAVOK_ANIMATION
	transform.setTranslation( translation );
#else
	transform.SetTranslation( translation );
#endif
}

RED_FORCE_INLINE static void TransformVectorNoScale( AnimVector4& dest, const AnimQsTransform& transform, const AnimVector4& src )
{
#ifdef USE_HAVOK_ANIMATION
	dest.setRotatedDir( transform.m_rotation, src );
#else
	dest.RotateDirection( transform.Rotation, src );
#endif
}

RED_FORCE_INLINE static void TransformLocationNoScale( AnimVector4& dest, const AnimQsTransform& transform, const AnimVector4& src )
{
#ifndef USE_HAVOK_ANIMATION
	dest.RotateDirection( transform.Rotation, src );
	SetAdd( dest, transform.Translation );
#endif
}

RED_FORCE_INLINE static void InverseTransformLocationNoScale( AnimVector4& dest, const AnimQsTransform& transform, const AnimVector4& src )
{
#ifndef USE_HAVOK_ANIMATION
	SetSub( dest, transform.Translation );
	dest.InverseRotateDirection( transform.Rotation, dest );
#endif
}

RED_FORCE_INLINE static Matrix AnimQsTransformToMatrix( const AnimQsTransform& transform )
{
	Matrix mat;
#ifdef USE_HAVOK_ANIMATION
	HavokTransformToMatrix_Renormalize( transform, &mat );
#else
	RedMatrix4x4 conversionMatrix = transform.ConvertToMatrixNormalized();
	mat = reinterpret_cast< const Matrix& >( conversionMatrix );
#endif
	return mat;
}

RED_FORCE_INLINE static AnimQsTransform MatrixToAnimQsTransform( const Matrix& matrix )
{
	AnimQsTransform ret;
#ifdef USE_HAVOK_ANIMATION
	MatrixToHavokQsTransform( matrix, ret );
#else
	RedMatrix4x4 conversionMatrix;
	conversionMatrix = reinterpret_cast< const RedMatrix4x4& >( matrix );
	ret.Set( conversionMatrix );
#endif
	return ret;
}

#define	ANIM_QS_TRANSFORM_TO_ENGINE_QS_TRANSFORM_REF( trans ) reinterpret_cast< EngineQsTransform& > ( trans )
#define	ENGINE_QS_TRANSFORM_TO_ANIM_QS_TRANSFORM_REF( trans ) reinterpret_cast< AnimQsTransform& > ( trans )
#define	ANIM_QS_TRANSFORM_TO_CONST_ENGINE_QS_TRANSFORM_REF( trans ) reinterpret_cast< const EngineQsTransform& > ( trans )
#define	ENGINE_QS_TRANSFORM_TO_CONST_ANIM_QS_TRANSFORM_REF( trans ) reinterpret_cast< const AnimQsTransform& > ( trans )

#define	ANIM_VECTOR_TO_VECTOR_REF( vec ) reinterpret_cast< Vector& > ( vec )
#define	VECTOR_TO_ANIM_VECTOR_REF( vec ) reinterpret_cast< AnimVector4& > ( vec )
#define	ANIM_VECTOR_TO_CONST_VECTOR_REF( vec ) reinterpret_cast< const Vector& > ( vec )
#define	VECTOR_TO_ANIM_CONST_VECTOR_REF( vec ) reinterpret_cast< const AnimVector4& > ( vec )

RED_FORCE_INLINE static AnimQuaternion EulerAnglesToAnimQuat( const EulerAngles& rotation )
{
#ifdef USE_HAVOK_ANIMATION
	hkQuaternion out; out.setZero4();
	RED_ASSERT(false);
#else
	Vector vector = rotation.ToQuat();
	RedQuaternion out;
	out.Quat = reinterpret_cast< const RedVector4& >( vector );
#endif
	return out;
}

RED_FORCE_INLINE static AnimVector4 VectorToAnimVector( const Vector& vector )
{
#ifdef USE_HAVOK_ANIMATION
	hkVector4 out; out.setZero4();
	COPY_VECTOR_TO_HK_VECTOR( vector, out );
#else
	RedVector4 out = reinterpret_cast< const RedVector4& >( vector );
#endif
	return out;
}

RED_FORCE_INLINE static Vector AnimVectorToVector( const AnimVector4& vector )
{
#ifdef USE_HAVOK_ANIMATION
	hkVector4 vec = vector;
	Vector& outVec = TO_VECTOR_REF( vec );
	return outVec;
#else
	RedVector4 vec = vector;
	return reinterpret_cast< Vector& >( vec );
#endif
}

RED_FORCE_INLINE static void AddYawRotationToAnimQsTransform( AnimQsTransform& transform, const AnimFloat& byYaw )
{
	AnimFloat radByYaw = DEG2RAD( byYaw );
	RedQuaternion rotate = RedQuaternion( RedVector4( 0.0f, 0.0f, 1.0f, 0.0f ), radByYaw );
	RedQuaternion currentRotation = transform.GetRotation();
	currentRotation = RedQuaternion::Mul( currentRotation, rotate );
	transform.SetRotation( currentRotation );
}

RED_FORCE_INLINE static void SetYawRotationToAnimQsTransform( AnimQsTransform& transform, const AnimFloat& yawToSet )
{
	AnimFloat radYawToSet = DEG2RAD( yawToSet );
	RedQuaternion rotate = RedQuaternion( RedVector4( 0.0f, 0.0f, 1.0f, 0.0f ), radYawToSet );
	transform.SetRotation( rotate );
}

/** calculate anim transform by using Location ( translation ), Forward direction, Side direction */
RED_FORCE_INLINE static void CalculateAnimTransformLFS( AnimQsTransform& out, const AnimVector4& translation, const AnimVector4& fwd, const AnimVector4& side, const AnimVector4 & fwdAxis, const AnimVector4 & sideAxis, const AnimVector4 & bendAxis )
{
#ifdef USE_HAVOK_ANIMATION
	ASSERT( fwd.isNormalized3(), TXT( "Forward dir should be normalized" ) );
	ASSERT( side.isNormalized3(), TXT( "Side dir should be normalized" ) );
#else
	ASSERT( fwd.IsNormalized3(RedFloat1(0.0015f)), TXT( "Forward dir should be normalized" ) );
	ASSERT( side.IsNormalized3(RedFloat1(0.0015f)), TXT( "Side dir should be normalized" ) );
#endif
	ASSERT( abs( RedMath::SIMD::Dot3( fwd, side ) ) < 0.05f, TXT( "Forward and Side dirs should be orthogonal") );

	SetTranslation( out, translation );
	AnimVector4 bend;
	SetCross( bend, side, fwd );

#ifdef USE_HAVOK_ANIMATION
	hkRotation hkR;
	hkR.setCols( fwd, bend, side ); // TODO ignores axes!
	out.setRotation( hkR );
	out.setScale( hkVector4( 1.0f, 1.0f, 1.0f, 1.0f ) );
#else
	RedMatrix3x3 redM33;
	redM33.SetCols( Add(Add(Mul(fwd.AsVector3(), fwdAxis.X), Mul(bend.AsVector3(), bendAxis.X)), Mul(side.AsVector3(), sideAxis.X)),
					Add(Add(Mul(fwd.AsVector3(), fwdAxis.Y), Mul(bend.AsVector3(), bendAxis.Y)), Mul(side.AsVector3(), sideAxis.Y)),
					Add(Add(Mul(fwd.AsVector3(), fwdAxis.Z), Mul(bend.AsVector3(), bendAxis.Z)), Mul(side.AsVector3(), sideAxis.Z)));
	RedQuaternion redQ;
	redQ.ConstructFromMatrix( redM33 );
	out.SetRotation( redQ );
	out.SetScale( AnimVector4( 1.0f, 1.0f, 1.0f, 1.0f ) );
#endif
}

//////////////////////////////////////////////////////////////////////////

/** Blend a with b, preserving a.alpha and using b.alpha as blend factor for RGB */
RED_FORCE_INLINE static Color QuiteSpecificBlendColor( Color a, Color b )
{
	Float useA = 1.0f - ( ( Float )b.A ) / 255.0f;
	Float useB = ( ( Float )b.A ) / 255.0f;
	return Color( 	( Uint8 )( ( Float )a.R * useA + ( Float )b.R * useB ),
		( Uint8 )( ( Float )a.G * useA + ( Float )b.G * useB ),
		( Uint8 )( ( Float )a.B * useA + ( Float )b.B * useB ),
		a.A );
}

/** draw debug matrix */
RED_FORCE_INLINE static void DrawDebugMatrix( const Matrix& m, Float axisSize, Uint8 alpha, CRenderFrame* frame, Color colorOverride = Color( 0,0,0,0 ) )
{
	frame->AddDebugLine( m.GetTranslation(), m.GetTranslation() + m.GetAxisX() * axisSize, QuiteSpecificBlendColor( Color( 255,0,0,alpha ), colorOverride ), true );
	frame->AddDebugLine( m.GetTranslation(), m.GetTranslation() + m.GetAxisY() * axisSize, QuiteSpecificBlendColor( Color( 0,255,0,alpha ), colorOverride ), true );
	frame->AddDebugLine( m.GetTranslation(), m.GetTranslation() + m.GetAxisZ() * axisSize, QuiteSpecificBlendColor( Color( 0,0,255,alpha ), colorOverride ), true );
}

//////////////////////////////////////////////////////////////////////////

template < class T >
RED_FORCE_INLINE T BlendToWithBlendTime( const T& from, const T& to, Float blendTime, Float t )
{
	const T from2to = to - from;
	const Float coef = t / ( blendTime );
	return from + from2to * Min( coef, 1.0f );
}

template < class T >
RED_FORCE_INLINE T BlendToWithBlendTimeLimit( const T& from, const T& to, Float blendTime, Float t, Float clampLimit = 0.5f )
{
	const T from2to = to - from;
	const Float coef = t / ( blendTime );
	return from + from2to * Min( coef, Min( 1.0f, clampLimit ) );
}

RED_FORCE_INLINE static Float BlendOnOffWithSpeedBasedOnTime( Float from, Float to, Float blendTime, Float t )
{
	const Float from2to = to - from;
	const Float speed = blendTime != 0.0f? 1.0f / blendTime : 1000000.0f; // should give in real-time gameplay instant switch
	return from2to >= 0.0f? from + Min( speed * t, from2to ) : from + Max ( -speed * t, from2to );
}

RED_FORCE_INLINE static Float BlendToWithSpeed( Float from, Float to, Float speed, Float t )
{
	const Float from2to = to - from;
	return from2to >= 0.0f? from + Min( speed * t, from2to ) : from + Max ( -speed * t, from2to );
}

// check how far would we get if we would use blend time and how far if we would use speed
// and use minimal value (we will then have nice blend at the end but we will have control over maximal speed of rotation)

RED_FORCE_INLINE static Float BlendToWithBlendTimeAndSpeed( Float from, Float to, Float blendTime, Float speed, Float t )
{
	const Float from2to = to - from;
	Float from2toDist = Abs( from2to );
	if ( from2toDist != 0.0f )
	{
		Float coef = Clamp( t / blendTime, 0.0f, 1.0f );
		Float covDist = speed * t;
		Float maxChange = Min( coef * from2toDist, covDist );
		return from + from2to * maxChange / from2toDist;
	}
	else
	{
		return from;
	}
}

RED_INLINE static Vector BlendToWithBlendTimeAndSpeed( const Vector2& from, const Vector2& to, Float blendTime, Float speed, Float t )
{
	const Vector2 from2to = to - from;
	Float from2toDist = from2to.Mag();
	if ( from2toDist != 0.0f )
	{
		Float coef = Clamp( t / blendTime, 0.0f, 1.0f );
		Float covDist = speed * t;
		Float maxChange = Min( coef * from2toDist, covDist );
		return from + from2to * maxChange / from2toDist;
	}
	else
	{
		return from;
	}
}

RED_FORCE_INLINE static Bool SampleCompressedPoseOrFallback( const CSkeletalAnimation* skAnimation, SBehaviorGraphOutput &output, const CSkeleton* skeleton )
{
	if ( skAnimation->SampleCompressedPose( output.m_numBones, output.m_outputPose, output.m_numFloatTracks, output.m_floatTracks, skeleton ) )
	{
		return true;
	}
	else
	{
		return skAnimation->SampleFallback( output.m_numBones, output.m_numFloatTracks, output.m_outputPose,  output.m_floatTracks );
	}
}

//! Sample method helper to do blend with compressed pose in one place
//! Updates blend timer too!
RED_FORCE_INLINE static Bool UpdateAndSampleBlendWithCompressedPose( const CSkeletalAnimationSetEntry* animation, Float timeDelta, Float& blendTimer, Float localTime, SBehaviorSampleContext& context, SBehaviorGraphOutput &output, const CSkeleton* skeleton )
{
	const CSkeletalAnimation* skAnimation = animation->GetAnimation();
	Uint32 bonesAvailable;
	Uint32 bonesAlwaysLoaded;
	EAnimationBufferDataAvailable abda = skAnimation->GetAnimationBufferDataAvailable( output.m_numBones, bonesAvailable, bonesAlwaysLoaded );

	Float blendLength = BehaviorUtils::GetTimeFromCompressedBlend( animation->GetCompressedPoseBlend() );

	if ( abda != ABDA_All && blendTimer == 0.0f )
	{
		// we just started and we don't have full animation - have to blend!
		blendTimer = blendLength;
	}
	Float useCompressedBlend = blendLength != 0.0f? blendTimer / blendLength : 0.0f;

	Bool ret = false;
	if ( useCompressedBlend == 0.0f && abda == ABDA_All )
	{
		// we've already blended
		if ( output.m_numBones != bonesAvailable )
		{
			// ugh? for some reason we don't have all! just fill with compressed pose and then try to fill rest with animation
			ret |= SampleCompressedPoseOrFallback( skAnimation, output, skeleton );
		}

		// no need to blend
		ret |= skAnimation->Sample(	localTime, 
			bonesAvailable,
			output.m_numFloatTracks,
			output.m_outputPose, 
			output.m_floatTracks  );

		ASSERT( ret );
	}
	else if ( abda == ABDA_None )
	{
		// just get compressed pose
		ret |= SampleCompressedPoseOrFallback( skAnimation, output, skeleton );
	}
	else
	{
		// blending
		if ( abda == ABDA_All )
		{
			// everything is loaded, start blending
			blendTimer = Max( 0.0f, blendTimer - timeDelta );
		}

		if ( useCompressedBlend == 1.0f )
		{
			// not blending yet?
			ret |= SampleCompressedPoseOrFallback( skAnimation, output, skeleton );
			if ( bonesAvailable > 0 )
			{
				ret |= skAnimation->Sample(	localTime, 
					bonesAvailable,
					output.m_numFloatTracks,
					output.m_outputPose, 
					output.m_floatTracks );
			}

			ASSERT( ret );
		}
		else
		{
			// blend!
			CCacheBehaviorGraphOutput cachePose1( context );
			CCacheBehaviorGraphOutput cachePose2( context );

			SBehaviorGraphOutput* temp1 = cachePose1.GetPose();
			SBehaviorGraphOutput* temp2 = cachePose2.GetPose();

			if ( temp1 && temp2 )
			{
				// temp1 is our "everything we have"
				if ( output.m_numBones != bonesAvailable )
				{
					// ugh? for some reason we don't have all! just fill with compressed pose and then try to fill rest with animation
					ret |= SampleCompressedPoseOrFallback( skAnimation, *temp1, skeleton );
				}

				// no need to blend
				ret |= skAnimation->Sample(	localTime, 
					bonesAvailable,
					temp1->m_numFloatTracks,
					temp1->m_outputPose, 
					temp1->m_floatTracks  );

				// temp2 is "what we've got before"
				ret |= SampleCompressedPoseOrFallback( skAnimation, *temp2, skeleton );

				// copy bonesAlwaysLoaded amount of bones from temp1 to temp2
				if ( bonesAlwaysLoaded > 0 )
				{
					Red::System::MemoryCopy( temp2->m_outputPose, temp1->m_outputPose, sizeof(AnimQsTransform) * bonesAlwaysLoaded );
				}

				temp1->Touch();
				temp2->Touch();

				output.SetInterpolate( *temp1, *temp2, useCompressedBlend );

				ASSERT( ret );
			}
		}
	}

#ifdef DEBUG_CORRUPT_TRANSFORMS
	{
		const Uint32 num = output.m_numBones;
		for ( Uint32 i=0; i<num; ++i )
		{
			RED_ASSERT( output.m_outputPose[i].Rotation.Quat.IsOk(), TXT("Animation rotation data appears to be corrupt!") );
			RED_ASSERT( output.m_outputPose[i].Translation.IsOk(), TXT("Animation translation data appears to be corrupt!") );
			RED_ASSERT( output.m_outputPose[i].Scale.IsOk(), TXT("Animation scale data appears to be corrupt!") );
		}
	}
#endif

	return ret;
}
