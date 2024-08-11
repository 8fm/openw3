/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behaviorIncludes.h"
#include "../engine/actorInterface.h"
#include "../redMath/redmathbase.h"
#include "behaviorGraphOutput.h"
#include "animatedComponent.h"
#include "baseEngine.h"

IMPLEMENT_RTTI_ENUM( EBehaviorLod );
IMPLEMENT_RTTI_ENUM( EInterpolationType );
IMPLEMENT_RTTI_ENUM( EAxis );
IMPLEMENT_RTTI_ENUM( EBehaviorTransitionBlendMotion );
IMPLEMENT_RTTI_ENUM( ELookAtLevel );
IMPLEMENT_RTTI_ENUM( EActorAnimState );
IMPLEMENT_RTTI_ENUM( ESkeletonType );
IMPLEMENT_RTTI_ENUM( ECompressedPoseBlend );
IMPLEMENT_RTTI_ENUM( ESkeletalAnimationPoseType );
IMPLEMENT_RTTI_ENUM( EAnimationFps );
IMPLEMENT_RTTI_ENUM( EAdditiveType );
IMPLEMENT_RTTI_ENUM( EPoseDeformationLevel );
IMPLEMENT_RTTI_ENUM( ESkeletalAnimationStreamingType );
IMPLEMENT_ENGINE_CLASS( SBehaviorGraphBoneInfo );
IMPLEMENT_ENGINE_CLASS( SBehaviorGraphTrackInfo );
IMPLEMENT_ENGINE_CLASS( SBoneTransform );
IMPLEMENT_RTTI_BITFIELD( EActorLodFlag );

//////////////////////////////////////////////////////////////////////////

CSkeleton* IBehaviorGraphBonesPropertyOwner::GetBonesSkeleton( CAnimatedComponent* component ) const 
{ 
	return component->GetSkeleton(); 
}

//////////////////////////////////////////////////////////////////////////

void CInstancePropertiesBuilder::AddClassCaption( CClass* _class )
{
	m_data.PushBack( tDesc( String::EMPTY, _class->GetName().AsString() ) );
}

void CInstancePropertiesBuilder::AddData( const String& name, const String& data, Uint32 depth )
{
	if ( depth <= 1 )
	{
		m_data.PushBack( tDesc( name, data ) );
	}
	else if ( depth == 2 )
	{
		m_data.PushBack( tDesc( TXT("   ") + name, data ) );	
	}
	else
	{
		String temp = TXT("   ");
		for ( Uint32 i=1; i<depth; ++i )
		{
			temp.Append( TXT("   "), 3 );
		}

		m_data.PushBack( tDesc( temp + name, data ) );
	}
}

void CInstancePropertiesBuilder::ParseData( IRTTIType* type, const void* data, const String& name, Uint32 depth )
{
	if ( depth < 2 )
	{
		// Special stuff for array
		if ( type->GetType() == RT_Array )
		{
			CRTTIArrayType* arrayType = (CRTTIArrayType* ) type;
			const Uint32 size = arrayType->GetArraySize( data );

			String txt = String::Printf( TXT("Array [%i]"), size );
			AddData( name, txt, depth );

			for ( Uint32 i=0; i<size; i++ )
			{
				const void* elementData = arrayType->GetArrayElement( data, i );
				IRTTIType* innerArrayType = arrayType->GetInnerType();

				String elemName = String::Printf( TXT("   %i"), i );
				ParseData( innerArrayType, elementData, elemName, depth + 1 );
			}

			return;
		}

		// Pointer
		if ( type->GetType() == RT_Pointer )
		{
			const CRTTIPointerType* ptrType = (CRTTIPointerType*) type;
			IRTTIType* pointedType = ptrType->GetPointedType();
			const void* pointedData = ptrType->GetPointed( data );
			String txt = String::Printf( TXT("Pointer [%s]"), pointedType->GetName().AsString().AsChar() );

			AddData( name, txt, depth );

			if ( pointedData )
			{
				ParseData( pointedType, pointedData, pointedType->GetName().AsString(), depth + 1 );
			}

			return;
		}

		// Class
		if ( type->GetType() == RT_Class )
		{
			const CClass* classType = (CClass*) type;

			TDynArray< CProperty* > properties;
			classType->GetProperties( properties );	

			for ( Uint32 i=0; i<properties.Size(); i++ )
			{
				const void* propData = properties[i]->GetOffsetPtr( data );
				IRTTIType* propType = properties[i]->GetType();

				if ( Red::System::StringCompare( properties[i]->GetName().AsString().AsChar(), TXT("m_"), 2 ) == 0 )
				{
					ParseData( propType, propData, properties[i]->GetName().AsString().AsChar() + 2, depth + 1 );
				}
				else
				{
					ParseData( propType, propData, properties[i]->GetName().AsString(), depth + 1 );
				}
			}
			return;
		}
	}

	// Simple shit
	if ( data )
	{
		String txt;
		type->ToString( data, txt );
		AddData( name, txt, depth );
	}
	else
	{
		AddData( name, TXT("NULL"), depth );
	}
}

//////////////////////////////////////////////////////////////////////////

namespace BehaviorDebugUtils 
{
	void LogMsg( const String& desc )
	{
		BEH_LOG( desc.AsChar() );
	}

	void LogVector( const Vector& vec )
	{
		String msg = String::Printf( TXT("[ %1.3f; %1.3f; %1.3f ]"), vec.A[ 0 ], vec.A[ 1 ], vec.A[ 2 ] );
		BEH_LOG( msg.AsChar() );
	}

	void LogVector( const String& desc, const Vector& vec )
	{
		String msg = String::Printf( TXT("%16s [ %1.3f; %1.3f; %1.3f ]"), desc.AsChar(), vec.A[ 0 ], vec.A[ 1 ], vec.A[ 2 ] );
		BEH_LOG( msg.AsChar() );
	}

	void LogMatrix( const Matrix& mat )
	{
		LogVector( mat.V[ 0 ] );
		LogVector( mat.V[ 1 ] );
		LogVector( mat.V[ 2 ] );
		LogVector( mat.V[ 3 ] );
	}

	void LogMatrix( const String& desc, const Matrix& mat )
	{
		String s1 = desc + TXT("(0)");
		String s2 = desc + TXT("(1)");
		String s3 = desc + TXT("(2)");
		String s4 = desc + TXT("(3)");

		LogVector( s1, mat.V[ 0 ] );
		LogVector( s2, mat.V[ 1 ] );
		LogVector( s3, mat.V[ 2 ] );
		LogVector( s4, mat.V[ 3 ] );
	}
}

//////////////////////////////////////////////////////////////////////////

namespace BehaviorUtils
{
	void RotateBoneMS( const AnimQsTransform& parent, AnimQsTransform& child, Float angle, const AnimVector4& axis )
	{
#ifdef USE_HAVOK_ANIMATION
		hkQsTransform childMS; childMS.setMul( parent, child );

		hkQsTransform rotationRequested( hkQsTransform::IDENTITY );
		rotationRequested.m_rotation = hkQuaternion( axis, angle );

		rotationRequested.setMul( rotationRequested, childMS );

		child.setMulInverseMul( parent, rotationRequested );
#else
		RedQsTransform childMS; 
		childMS.SetMul( parent, child );

		RedQsTransform rotationRequested( RedQsTransform::IDENTITY );
		rotationRequested.Rotation = RedQuaternion( axis, angle );

		rotationRequested.SetMul( rotationRequested, childMS );

		child.SetMulInverseMul( parent, rotationRequested );
#endif
	}

	void RotateBoneLS( AnimQsTransform& bone, Float angle, const AnimVector4& axis )
	{
		AnimQsTransform rotationRequested( AnimQsTransform::IDENTITY );
#ifdef USE_HAVOK_ANIMATION
		rotationRequested.m_rotation = hkQuaternion( axis, angle );
		bone.setMul( bone, rotationRequested );
#else
		rotationRequested.Rotation = RedQuaternion( axis, angle );
		bone.SetMul( bone, rotationRequested );
#endif
	}

	void RotateBoneRefS( const AnimQsTransform& ref, AnimQsTransform& bone, Float angle, const AnimVector4& axis )
	{
		AnimQsTransform rotationRequested( AnimQsTransform::IDENTITY );
#ifdef USE_HAVOK_ANIMATION
		rotationRequested.m_rotation = hkQuaternion( axis, angle );
		//rotationRequested.m_rotation.setMul( rotationRequested.m_rotation, ref.m_rotation );
		bone.setMul( bone, rotationRequested );
#else
		rotationRequested.Rotation = RedQuaternion( axis, angle );
		//rotationRequested.m_rotation.setMul( rotationRequested.m_rotation, ref.m_rotation );
		bone.SetMul( bone, rotationRequested );
#endif
	}

	void LogQuaternion( const String& label, const AnimQuaternion& quat )
	{
		EulerAngles angles;
#ifdef USE_HAVOK_ANIMATION
		HavokQuaternionToEulerAngles( quat, angles );
#else
		RedMatrix4x4 mat = BuildFromQuaternion( quat.Quat );
		RedEulerAngles temp = mat.ToEulerAnglesFull();
		angles = reinterpret_cast< const EulerAngles& >( temp );
#endif
		BEH_LOG( TXT("%s '%ls'"), label.AsChar(), ToString( angles ).AsChar() );
	}

	void CartesianFromSpherical( AnimVector4& sphericalInCartesianOut )
	{
#ifdef USE_HAVOK_ANIMATION
		const hkReal rad = sphericalInCartesianOut( 0 );
		const hkReal lon = sphericalInCartesianOut( 1 );
		const hkReal lat = sphericalInCartesianOut( 2 );
		const hkReal rct = rad * hkMath::cos( lat );

		sphericalInCartesianOut.set(
			rct * hkMath::cos( lon ),
			rct * hkMath::sin( lon ),
			rad * hkMath::sin( lat ) );
#else
		const Float rad = sphericalInCartesianOut.X;
		const Float lon = sphericalInCartesianOut.Y;
		const Float lat = sphericalInCartesianOut.Z;
		const Float rct = rad * Red::Math::MCos( lat );

		sphericalInCartesianOut.Set(
			rct * Red::Math::MCos( lon ),
			rct * Red::Math::MSin( lon ),
			rad * Red::Math::MSin( lat ), 0.0f );
#endif
	}

	void SphericalFromCartesian( AnimVector4& cartesianInSphericalOut )
	{
#ifdef USE_HAVOK_ANIMATION
		const hkReal eps = 1.0e-4f;

		const hkReal rad = cartesianInSphericalOut.length3();

		if ( rad < eps )
		{
			cartesianInSphericalOut.setZero4();
			return;
		}

		const hkReal x = cartesianInSphericalOut( 0 );
		const hkReal y = cartesianInSphericalOut( 1 );
		const hkReal z = cartesianInSphericalOut( 2 );

		const hkReal lat = hkMath::asin( z / rad );
		const hkReal lon = hkMath_atan2fApproximation( y, x );

		cartesianInSphericalOut.set( rad, lon, lat );
#else
		const Float eps = 1.0e-4f;

		const Float rad = cartesianInSphericalOut.Length3();

		if ( rad < eps )
		{
			cartesianInSphericalOut.SetZeros();
			return;
		}

		const Float x = cartesianInSphericalOut.X;
		const Float y = cartesianInSphericalOut.Y;
		const Float z = cartesianInSphericalOut.Z;

		const Float lat = Red::Math::MAsin( z / rad );
		const Float lon = Red::Math::MATan2( y, x );

		cartesianInSphericalOut.Set( rad, lon, lat, 0.0f );
#endif
	}

namespace BlendingUtils
{

	void BlendTwoTransforms( const RedQsTransform& transformA, const RedQsTransform& transformB, Float weight, RedQsTransform& transformOut )
	{
		// TODO: this should be optimized once RedMath will be converted to SIMD properly
		transformOut.Translation = RedVector4::Lerp(
			transformA.Translation,
			transformB.Translation, weight);

		// Make sure we blend the closest representation of the quaternion; flip if necessary
		RedVector4 bRot = transformB.Rotation.Quat;
		if ( Dot(transformA.Rotation.Quat, transformB.Rotation.Quat) < 0.0f )
			SetMul( bRot, RedFloat4(-1.0f) );

		transformOut.Rotation = RedVector4::Lerp(
			transformA.Rotation.Quat,
			bRot, weight);

		transformOut.Scale = RedVector4::Lerp(
			transformA.Scale,
			transformB.Scale, weight);

		// Check Rotation
		{
			const Float quatNorm = transformOut.Rotation.Quat.Length4();
			if (quatNorm < FLT_EPSILON)
			{
				// no rotations blended (or cancelled each other) -> set to identity
				transformOut.Rotation.SetIdentity();
			}
			else
			{
				// normalize
				transformOut.Rotation.Normalize();
			}
		}
	}

	void BlendPosesNormal( SBehaviorGraphOutput& output, const SBehaviorGraphOutput& a, const SBehaviorGraphOutput& b, Float alpha )
	{
#ifdef USE_HAVOK_ANIMATION
		hkaSkeletonUtils::blendPoses( output.m_numBones, a.m_outputPose, b.m_outputPose, alpha, output.m_outputPose );
#else
		const Uint32 numBones = output.m_numBones;
		for ( Uint32 i=0; i<numBones; ++i )
		{
			BlendTwoTransforms( a.m_outputPose[i], b.m_outputPose[i], alpha, output.m_outputPose[i] );
		}

		const Uint32 numTracks = Min( output.m_numFloatTracks, Min( a.m_numFloatTracks, b.m_numFloatTracks ) );
		for ( Uint32 i=0; i<numTracks; ++i )
		{
			output.m_floatTracks[i] = Lerp( alpha, a.m_floatTracks[i], b.m_floatTracks[i] );
		}
#endif
	}

	void BlendAdditive( SBehaviorGraphOutput& output, const SBehaviorGraphOutput& pose, const SBehaviorGraphOutput& additive, Float weight, EAdditiveType type )
	{
		if ( type == AT_Local )
		{
			BlendAdditive_Local( output, pose, additive, weight );
		}
		else if ( type == AT_Ref )
		{
			BlendAdditive_Ref( output, pose, additive, weight );
		}
		else
		{
			ASSERT( 0 );
		}
	}

	void BlendAdditive_Local( SBehaviorGraphOutput& output, const SBehaviorGraphOutput& pose, const SBehaviorGraphOutput& additive, Float weight )
	{
		if ( weight < 1.f )
		{
#ifdef USE_HAVOK_ANIMATION
			hkSimdReal w = weight;
			const hkQsTransform ident( hkQsTransform::IDENTITY );

			const Uint32 size = output.m_numBones;
			for ( Uint32 i=0; i<size;  ++i )
			{
				hkQsTransform temp;
				temp.setInterpolate4( ident, additive.m_outputPose[ i ], w );
				output.m_outputPose[ i ].setMul( pose.m_outputPose[ i ], temp );
			}
#else
			Float w = weight;
			const RedQsTransform ident( RedQsTransform::IDENTITY );

			const Uint32 size = output.m_numBones;
			for ( Uint32 i=0; i<size;  ++i )
			{
				RedQsTransform temp;
				temp.Lerp( ident, additive.m_outputPose[ i ], w );
				output.m_outputPose[ i ].SetMul( pose.m_outputPose[ i ], temp );
			}
#endif
		}
		else
		{

			const Uint32 size = output.m_numBones;
			for ( Uint32 i=0; i<size;  ++i )
			{
#ifdef USE_HAVOK_ANIMATION
				output.m_outputPose[ i ].setMul( pose.m_outputPose[ i ], additive.m_outputPose[ i ] );
#else
				output.m_outputPose[ i ].SetMul( pose.m_outputPose[ i ], additive.m_outputPose[ i ] );
#endif
			}
		}

		for( Uint32 i=0; i<output.m_numFloatTracks; ++i )
		{
			output.m_floatTracks[i] = pose.m_floatTracks[i] + weight * additive.m_floatTracks[i];
		}
	}

	void BlendAdditive_Ref( SBehaviorGraphOutput& output, const SBehaviorGraphOutput& pose, const SBehaviorGraphOutput& additive, Float weight )
	{
		output.SetAddMul( pose, additive, weight );
	}

	void BlendPosesNormal( AnimQsTransform* bonesA, const AnimQsTransform* bonesB, Uint32 num, Float weight )
	{
#ifdef USE_HAVOK_ANIMATION
		const hkSimdReal hkWeight = weight;
		for ( Uint32 i=0; i<num; ++i )
		{
			bonesA[ i ].blendAddMul( bonesB[ i ], hkWeight );
		}
#else
		const Float hkWeight = weight;
		for ( Uint32 i=0; i<num; ++i )
		{
			bonesA[ i ].BlendAddMul( bonesB[ i ], hkWeight );
		}
#endif
	}

	void BlendTracksNormal( Float* tracksA, const Float* tracksB, Uint32 num, Float weight )
	{
		for ( Uint32 i=0; i<num; ++i )
		{
			tracksA[ i ] += weight * tracksB[ i ];
		}
	}

	void BlendPosesNormal( SBehaviorGraphOutput& poseA, const SBehaviorGraphOutput& poseB, Float weight )
	{
		BlendPosesNormal( poseA.m_outputPose, poseB.m_outputPose, poseA.m_numBones, weight );
		BlendTracksNormal( poseA.m_floatTracks, poseB.m_floatTracks, poseA.m_numFloatTracks, weight );

#ifdef USE_HAVOK_ANIMATION
		poseA.m_deltaReferenceFrameLocal.blendAddMul( poseB.m_deltaReferenceFrameLocal, weight );
#else
		poseA.m_deltaReferenceFrameLocal.BlendAddMul( poseB.m_deltaReferenceFrameLocal, weight );
#endif
	}

	void BlendPartialPosesNormal( AnimQsTransform* bonesA, const AnimQsTransform* bonesB, Uint32 num, Float weight, const TDynArray< Int32 >& bones )
	{
		const Uint32 size = bones.Size();
		const Int32 numInt = (Int32)num;

		ASSERT( num < size );
#ifdef USE_HAVOK_ANIMATION
		const hkSimdReal hkWeight = weight;
#else
		const Float hkWeight = weight;
#endif
		for ( Uint32 i=0; i<size; ++i )
		{
			const Int32 boneIdx = bones[ i ];

			ASSERT( boneIdx != -1 );
			ASSERT( boneIdx < numInt );

			if ( boneIdx != -1 && boneIdx < numInt )
			{
#ifdef USE_HAVOK_ANIMATION
				bonesA[ boneIdx ].blendAddMul( bonesB[ boneIdx ], hkWeight );
#else
				bonesA[ boneIdx ].BlendAddMul( bonesB[ boneIdx ], hkWeight );
#endif
			}
		}
	}

	void BlendPartialPosesOverride( AnimQsTransform* bonesA, AnimQsTransform* bonesB, Uint32 num, Float weight, const TDynArray< Int32 >& bones )
	{
		const Uint32 size = bones.Size();
		const Int32 numInt = (Int32)num;

		ASSERT( num < size );
		const AnimFloat hkWeight = weight;
		for ( Uint32 i=0; i<size; ++i )
		{
			const Int32 boneIdx = bones[ i ];

			ASSERT( boneIdx != -1 );
			ASSERT( boneIdx < numInt );

			if ( boneIdx != -1 && boneIdx < numInt )
			{
#ifdef USE_HAVOK_ANIMATION
				bonesA[ boneIdx ].setInterpolate4( bonesA[ boneIdx ], bonesB[ boneIdx ], hkWeight );
#else
				bonesA[ boneIdx ].Lerp( bonesA[ boneIdx ], bonesB[ boneIdx ], hkWeight );
#endif
			}
		}
	}

	void BlendPartialPosesOverride( AnimQsTransform* bonesA, AnimQsTransform* bonesB, Uint32 num, Float weight, const TDynArray< Int32 >& bones, const TDynArray< Float >& boneWeights )
	{
		const Uint32 size = bones.Size();
		const Int32 numInt = (Int32)num;

		ASSERT( num > size );
		const AnimFloat hkWeight = weight;
		for ( Uint32 i=0; i<size; ++i )
		{
			const AnimFloat boneWeight = hkWeight * boneWeights[ i ];
			const Int32 boneIdx = bones[ i ];

			ASSERT( boneIdx != -1 );
			ASSERT( boneIdx < numInt );

			if ( boneIdx != -1 && boneIdx < numInt )
			{
#ifdef USE_HAVOK_ANIMATION
				bonesA[ boneIdx ].setInterpolate4( bonesA[ boneIdx ], bonesB[ boneIdx ], boneWeight );
#else
				bonesA[ boneIdx ].Lerp( bonesA[ boneIdx ], bonesB[ boneIdx ], boneWeight );
#endif
			}
		}
	}

	void SetPoseZero( SBehaviorGraphOutput& pose )
	{
		SetPoseZero( pose.m_outputPose, pose.m_numBones );
		SetTracksZero( pose.m_floatTracks, pose.m_numFloatTracks );

#ifdef USE_HAVOK_ANIMATION
		pose.m_deltaReferenceFrameLocal.setZero();
#else
		pose.m_deltaReferenceFrameLocal.SetZero();
#endif
	}

	void SetPoseIdentity( SBehaviorGraphOutput& pose )
	{
		SetPoseIdentity( pose.m_outputPose, pose.m_numBones );
		SetTracksZero( pose.m_floatTracks, pose.m_numFloatTracks );

#ifdef USE_HAVOK_ANIMATION
		pose.m_deltaReferenceFrameLocal.setIdentity();
#else
		pose.m_deltaReferenceFrameLocal.SetIdentity();
#endif
	}

	void RenormalizePoseRotations( SBehaviorGraphOutput& pose )
	{
		RenormalizePoseRotations( pose.m_outputPose, pose.m_numBones );
	}

	void RenormalizePose( SBehaviorGraphOutput& pose, Float accWeight, Bool renormalizeTracks )
	{
		RenormalizePose( pose.m_outputPose, pose.m_numBones, accWeight );

		if ( renormalizeTracks )
		{
			RenormalizeTracks( pose.m_floatTracks, pose.m_numFloatTracks, accWeight );
		}

#ifdef USE_HAVOK_ANIMATION
		const hkSimdReal invWeight = HK_SIMD_REAL(1.0f) / (hkSimdReal) accWeight;

		pose.m_deltaReferenceFrameLocal.m_translation.mul4( invWeight );
		pose.m_deltaReferenceFrameLocal.m_scale.mul4( invWeight );
		pose.m_deltaReferenceFrameLocal.m_rotation.normalize();
#else
		const Float invWeight = 1.0f / accWeight;

		SetMul( pose.m_deltaReferenceFrameLocal.Translation, invWeight );
		SetMul( pose.m_deltaReferenceFrameLocal.Scale, invWeight );
		pose.m_deltaReferenceFrameLocal.Rotation.Normalize();		
#endif
	}

	void SetPoseZero( AnimQsTransform* bones, Uint32 num )
	{
		for ( Uint32 i=0; i<num; ++i )
		{
#ifdef USE_HAVOK_ANIMATION
			bones[ i ].setZero();
#else
			bones[ i ].SetZero();
#endif
		}
	}

	void SetTracksZero( Float* tracks, Uint32 num )
	{
		for ( Uint32 i=0; i<num; ++i )
		{
			tracks[ i ] = 0.f;
		}
	}

	void SetPoseIdentity( AnimQsTransform* bones, Uint32 num )
	{
		for ( Uint32 i=0; i<num; ++i )
		{
#ifdef USE_HAVOK_ANIMATION
			bones[ i ].setIdentity();
#else
			bones[ i ].SetIdentity();
#endif
		}
	}

	void RenormalizePoseRotations( AnimQsTransform* poseOut, Uint32 numTransforms )
	{
#ifdef USE_HAVOK_ANIMATION
		// Code from havok
		// now normalize 4 quaternions at once
		hkQsTransform* blockStart = poseOut;
		unsigned numTransformsOver4 = numTransforms/4;
		for (unsigned i=0; i< numTransformsOver4; i++)
		{
			hkVector4 dots;
			hkVector4Util::dot4_4vs4(blockStart[0].m_rotation.m_vec, blockStart[0].m_rotation.m_vec,
				blockStart[1].m_rotation.m_vec, blockStart[1].m_rotation.m_vec,
				blockStart[2].m_rotation.m_vec, blockStart[2].m_rotation.m_vec,
				blockStart[3].m_rotation.m_vec, blockStart[3].m_rotation.m_vec,
				dots);
			hkVector4 inverseSqrtDots;
			inverseSqrtDots.setSqrtInverse4(dots);

			blockStart[0].m_rotation.m_vec.mul4(inverseSqrtDots.getSimdAt(0));
			blockStart[1].m_rotation.m_vec.mul4(inverseSqrtDots.getSimdAt(1));
			blockStart[2].m_rotation.m_vec.mul4(inverseSqrtDots.getSimdAt(2));
			blockStart[3].m_rotation.m_vec.mul4(inverseSqrtDots.getSimdAt(3));

			blockStart += 4;
		}

		unsigned leftovers = numTransforms%4;
		for (unsigned j=0; j<leftovers; j++)
		{
			blockStart[j].m_rotation.normalize();
		}
#else
		// Code from havok
		// now normalize 4 quaternions at once
		RedQsTransform* blockStart = poseOut;
		unsigned numTransformsOver4 = numTransforms/4;
		for (unsigned i=0; i< numTransformsOver4; i++)
		{
			RedVector4 dots(Dot(blockStart[0].Rotation.Quat, blockStart[0].Rotation.Quat),
				Dot(blockStart[1].Rotation.Quat, blockStart[1].Rotation.Quat),
				Dot(blockStart[2].Rotation.Quat, blockStart[2].Rotation.Quat),
				Dot(blockStart[3].Rotation.Quat, blockStart[3].Rotation.Quat) );

			RedVector4 inverseSqrtDots;
			inverseSqrtDots.Set( Red::Math::MRsqrt(dots.X),
				Red::Math::MRsqrt(dots.Y),
				Red::Math::MRsqrt(dots.Z),
				Red::Math::MRsqrt(dots.W) );


			SetMul( blockStart[0].Rotation.Quat, inverseSqrtDots.X );
			SetMul( blockStart[1].Rotation.Quat, inverseSqrtDots.Y );
			SetMul( blockStart[2].Rotation.Quat, inverseSqrtDots.Z );
			SetMul( blockStart[3].Rotation.Quat, inverseSqrtDots.W );

			blockStart += 4;
		}

		unsigned leftovers = numTransforms%4;
		for (unsigned j=0; j<leftovers; j++)
		{
			blockStart[j].Rotation.Normalize();
		}
#endif
	}

	void RenormalizePose( AnimQsTransform* poseOut, Uint32 numTransforms, Float accWeight )
	{
#ifdef USE_HAVOK_ANIMATION
		// Code from havok
		const hkSimdReal invWeight = HK_SIMD_REAL(1.0f) / (hkSimdReal) accWeight;

		for (unsigned i=0; i < numTransforms; i++)
		{
			poseOut[i].m_translation.mul4(invWeight);
			poseOut[i].m_scale.mul4(invWeight);
		}
#else
		// Code from havok
		const Float invWeight = 1.0f / accWeight;

		for (Uint32 i=0; i < numTransforms; i++)
		{
			SetMul( poseOut[i].Translation, invWeight );
			SetMul( poseOut[i].Scale, invWeight );
		}
#endif

		RenormalizePoseRotations( poseOut, numTransforms );
	}

	void RenormalizeTracks( Float* tracks, Uint32 num, Float accWeight )
	{
		for (unsigned i=0; i < num; i++)
		{
			tracks[i] /= accWeight;
		}
	}
}

Float RandF( const Float val )
{
	return val > 0.f ? GEngine->GetRandomNumberGenerator().Get( val ) : 0.f;
}

Float RandF( const Float minVal, Float maxVal )
{
	return maxVal > minVal ? GEngine->GetRandomNumberGenerator().Get( minVal, maxVal ) : minVal;
}

}

//////////////////////////////////////////////////////////////////////////
