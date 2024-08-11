/**
 * Copyright © 2008 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "behaviorGraphOutput.h"
#include "skeletonMapper.h"
#include "skeleton.h"

// class CSkeletonMapper is not used in the whole engine
// class was original needed to map between hi res and lo res skeletons
// in Havok (mapping for ragdolls basically)
#if 0

CSkeletonMapper::CSkeletonMapper()	
{
}

void CSkeletonMapper::OnSerialize( IFile &file )
{
	TBaseClass::OnSerialize( file );
#ifdef USE_HAVOK_ANIMATION
	file << m_skeletonA;
	file << m_skeletonB;
	m_ragdollToHighResMapper.Serialize( file );
	m_highResToRagdollMapper.Serialize( file );
#else
	//not used
#endif
}

Int32 CSkeletonMapper::NumBonesLowRes() const
{
#ifdef USE_HAVOK_ANIMATION
	const hkaSkeleton* skeletonLow  = m_highResToRagdollMapper.GetHavokObject()->m_mapping.m_skeletonB;
	return skeletonLow->m_numBones;
#else
	//not used
	return 0;
#endif
}

Int32 CSkeletonMapper::NumBonesHighRes() const
{
#ifdef USE_HAVOK_ANIMATION
	const hkaSkeleton* skeletonHigh = m_highResToRagdollMapper.GetHavokObject()->m_mapping.m_skeletonA;
	return skeletonHigh->m_numBones;
#else
	//not used
	return 0;
#endif
}

void CSkeletonMapper::MapHighToLow( Matrix* boneTransformsHighRes, Matrix* boneTransformsLowRes ) const
{
#ifdef USE_HAVOK_ANIMATION
	const hkaSkeleton* skeletonHigh = m_highResToRagdollMapper.GetHavokObject()->m_mapping.m_skeletonA;
	const hkaSkeleton* skeletonLow  = m_highResToRagdollMapper.GetHavokObject()->m_mapping.m_skeletonB;

	hkLocalBuffer<hkQsTransform> transformsHigh( skeletonHigh->m_numBones );
	hkLocalBuffer<hkQsTransform> transformsLow( skeletonLow->m_numBones );

	HK_ALIGN16( Float tempMatrix[16] );
	for( int i = 0; i < skeletonHigh->m_numBones; ++i )
	{
		boneTransformsHighRes[i].GetRowMajor( tempMatrix );
		transformsHigh[i].set4x4ColumnMajor( tempMatrix );
	}

	m_highResToRagdollMapper.GetHavokObject()->mapPose( transformsHigh.begin(), skeletonLow->m_referencePose, transformsLow.begin(), hkaSkeletonMapper::CURRENT_POSE );

	for( int i = 0; i < skeletonLow->m_numBones; ++i )
	{
		transformsLow[i].get4x4ColumnMajor( tempMatrix );
		boneTransformsLowRes[i].SetRowMajor( tempMatrix );
	}
#else
	//not used
#endif
}

void CSkeletonMapper::MapLowToHigh( Matrix* boneTransformsLowRes, Matrix* boneTransformsHighRes ) const
{
#ifdef USE_HAVOK_ANIMATION
	const hkaSkeleton* skeletonLow  = m_ragdollToHighResMapper.GetHavokObject()->m_mapping.m_skeletonA;
	const hkaSkeleton* skeletonHigh = m_ragdollToHighResMapper.GetHavokObject()->m_mapping.m_skeletonB;

	hkLocalBuffer<hkQsTransform> transformsLow( skeletonLow->m_numBones );
	hkLocalBuffer<hkQsTransform> transformsHigh( skeletonHigh->m_numBones );

	HK_ALIGN16( Float tempMatrix[16] );
	for( int i = 0; i < skeletonLow->m_numBones; ++i )
	{
		boneTransformsLowRes[i].GetColumnMajor( tempMatrix );
		transformsLow[i].set4x4ColumnMajor( tempMatrix );
	}

	m_ragdollToHighResMapper.GetHavokObject()->mapPose( transformsLow.begin(), skeletonHigh->m_referencePose, transformsHigh.begin(), hkaSkeletonMapper::CURRENT_POSE );

	for( int i = 0; i < skeletonHigh->m_numBones; ++i )
	{
		transformsHigh[i].get4x4ColumnMajor( tempMatrix );
		boneTransformsHighRes[i].SetColumnMajor( tempMatrix );
	}
#else
	//not used
#endif
}

#ifdef USE_HAVOK_ANIMATION
const hkaSkeletonMapper* CSkeletonMapper::GetRagdollToHighResMapper() const
{
	return m_ragdollToHighResMapper.GetHavokObject();
}
#endif

#ifdef USE_HAVOK_ANIMATION
hkaSkeletonMapper* CSkeletonMapper::GetRagdollToHighResMapper()
{
	return m_ragdollToHighResMapper.GetHavokObject();
}
#endif

#ifdef USE_HAVOK_ANIMATION
const hkaSkeletonMapper* CSkeletonMapper::GetHighResToRagdollMapper() const
{
	return m_highResToRagdollMapper.GetHavokObject();
}
#endif

#ifdef USE_HAVOK_ANIMATION
hkaSkeletonMapper* CSkeletonMapper::GetHighResToRagdollMapper()
{
	return m_highResToRagdollMapper.GetHavokObject();
}
#endif

CSkeletonMapper* CSkeletonMapper::Create( const FactoryInfo& data )
{
	CSkeletonMapper* obj = NULL;

	if ( data.m_reuse )
	{
		obj = data.m_reuse;			
	}
	else
	{
		obj = ::CreateObject< CSkeletonMapper >( data.m_parent );
	}			

	obj->m_skeletonA = data.m_skeletonA;
	obj->m_skeletonB = data.m_skeletonB;
#ifdef USE_HAVOK_ANIMATION
	obj->m_ragdollToHighResMapper.CopyFromContainer( data.m_ragdollToHighResMapper, &hkaSkeletonMapperClass );	
	obj->m_highResToRagdollMapper.CopyFromContainer( data.m_highResToRagdollMapper, &hkaSkeletonMapperClass );	
#else
	//not used
#endif
	return obj;
}

IMPLEMENT_ENGINE_CLASS( CSkeletonMapper );

#endif

//////////////////////////////////////////////////////////////////////////

namespace
{
	void FindSkeletonsMapping( const CSkeleton* skeletonA, const CSkeleton* skeletonB, TDynArray< Int32 >& outMappingA2B )
	{
		if ( skeletonA && skeletonB )
		{
			const Int32 sizeA = skeletonA->GetBonesNum();
			for ( Int32 i=0; i<sizeA; ++i )
			{
				const Uint32 boneIdxA = (Uint32)i;

				const AnsiChar* boneName = skeletonA->GetBoneNameAnsi( boneIdxA );

				ASSERT( boneName );
				if ( boneName )
				{
					//dex++
					const Int32 boneIdxB = skeletonB->FindBoneByName( boneName );
					//dex--

					if ( boneIdxB != -1 )
					{
						outMappingA2B.PushBack( boneIdxB );
					}
					else
					{
						outMappingA2B.PushBack( -1 );
					}
				}
			}
		}
	}

	Bool CalcAreSkeletonsSimilar( const TDynArray< Int32 >& mappingA, const TDynArray< Int32 >& mappingB )
	{
		if ( mappingA.Size() != mappingB.Size() )
		{
			return false;
		}

		for ( Uint32 i=0; i<mappingA.Size(); ++i )
		{
			if ( mappingA[ i ] != mappingB[ i ] )
			{
				return false;
			}
		}

		return true;
	}

	void MapA2B( const SBehaviorGraphOutput& poseA, SBehaviorGraphOutput& poseB, const TDynArray< Int32 >& mapping, const CSkeleton* skeletonB )
	{
		ASSERT( poseA.m_outputPose != poseB.m_outputPose );

		const AnimQsTransform* refPose = skeletonB->GetReferencePoseLS();

		const Uint32 size = mapping.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			const Int32 idx = mapping[ i ];

			ASSERT( idx < (Int32)poseA.m_numBones );
			ASSERT( idx < (Int32)poseB.m_numBones );
			ASSERT( idx < skeletonB->GetBonesNum() );
			ASSERT( i < (Int32)poseA.m_numBones );

			if ( idx != -1 )
			{
#ifdef USE_HAVOK_ANIMATION
				poseB.m_outputPose[ idx ].m_rotation = poseA.m_outputPose[ i ].m_rotation;
				poseB.m_outputPose[ idx ].m_translation = refPose[ idx ].m_translation;
#else
				// what about scale?
				poseB.m_outputPose[ idx ].Rotation = poseA.m_outputPose[ i ].Rotation;
				poseB.m_outputPose[ idx ].Translation = refPose[ idx ].Translation;
#endif
			}
		}		
	}

	void MapA2BFull( const SBehaviorGraphOutput& poseA, SBehaviorGraphOutput& poseB, const TDynArray< Int32 >& mapping, const CSkeleton* skeletonB )
	{
		ASSERT( poseA.m_outputPose != poseB.m_outputPose );

		const AnimQsTransform* refPose = skeletonB->GetReferencePoseLS();

		const Uint32 size = mapping.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			const Int32 idx = mapping[ i ];

			ASSERT( idx < (Int32)poseA.m_numBones );
			ASSERT( idx < (Int32)poseB.m_numBones );
			ASSERT( idx < skeletonB->GetBonesNum() );
			ASSERT( i < (Int32)poseA.m_numBones );

			if ( idx != -1 )
			{
#ifdef USE_HAVOK_ANIMATION
				poseB.m_outputPose[ idx ].m_rotation = poseA.m_outputPose[ i ].m_rotation;
				poseB.m_outputPose[ idx ].m_translation = refPose[ idx ].m_translation;
#else
				poseB.m_outputPose[ idx ].Rotation = poseA.m_outputPose[ i ].Rotation;
				poseB.m_outputPose[ idx ].Translation = refPose[ idx ].Translation;

#endif
			}
			else
			{
#ifdef USE_HAVOK_ANIMATION
				poseB.m_outputPose[ idx ].m_rotation = refPose[ idx ].m_rotation;
				poseB.m_outputPose[ idx ].m_translation = refPose[ idx ].m_translation;
#else
				poseB.m_outputPose[ idx ].Rotation = refPose[ idx ].Rotation;
				poseB.m_outputPose[ idx ].Translation = refPose[ idx ].Translation;
#endif
			}
		}
	}
};

IMPLEMENT_ENGINE_CLASS( CSkeleton2SkeletonMapper );

CSkeleton2SkeletonMapper::CSkeleton2SkeletonMapper()
	: m_motionScale( 1.f )
	, m_skeletonsAreSimilar( false )
	, m_pelvisBoneName( CNAME( pelvis ) )
{

}

const CSkeleton* CSkeleton2SkeletonMapper::GetSkeletonA() const			
{ 
	return static_cast< const CSkeleton* >( GetParent() ); 
}

Bool CSkeleton2SkeletonMapper::MapPoseA2B( const SBehaviorGraphOutput& poseA, SBehaviorGraphOutput& poseB ) const
{
	if ( AreSkeletonsSimilar() )
	{
		// TODO
	}
	
	MapA2B( poseA, poseB, m_mappingA2B, GetSkeletonB() );

#ifdef USE_HAVOK_ANIMATION
	poseB.m_deltaReferenceFrameLocal.m_translation.mul4( GetMotionScaleA2B() );
#endif

	return true;
}

Bool CSkeleton2SkeletonMapper::MapPoseB2A( SBehaviorGraphOutput& poseA, const SBehaviorGraphOutput& poseB ) const
{
	if ( AreSkeletonsSimilar() )
	{
		// TODO
	}

	MapA2B( poseB, poseA, m_mappingB2A, GetSkeletonA() );

#ifdef USE_HAVOK_ANIMATION
	poseA.m_deltaReferenceFrameLocal.m_translation.mul4( GetMotionScaleB2A() );
#endif

	return true;
}

Bool CSkeleton2SkeletonMapper::MapPoseFullA2B( const SBehaviorGraphOutput& poseA, SBehaviorGraphOutput& poseB ) const
{
	if ( AreSkeletonsSimilar() )
	{
		// TODO
	}

	MapA2BFull( poseA, poseB, m_mappingA2B, GetSkeletonB() );

#ifdef USE_HAVOK_ANIMATION
	poseB.m_deltaReferenceFrameLocal.m_translation.mul4( GetMotionScaleA2B() );
#endif

	return true;
}

Bool CSkeleton2SkeletonMapper::MapPoseFullB2A( SBehaviorGraphOutput& poseA, const SBehaviorGraphOutput& poseB ) const
{
	if ( AreSkeletonsSimilar() )
	{
		// TODO
	}

	MapA2BFull( poseB, poseA, m_mappingB2A, GetSkeletonA() );

#ifdef USE_HAVOK_ANIMATION
	poseA.m_deltaReferenceFrameLocal.m_translation.mul4( GetMotionScaleB2A() );
#endif

	return true;
}

Bool CSkeleton2SkeletonMapper::MapPoseA2B( const SBehaviorGraphOutput& poseA, SBehaviorGraphOutput& poseB, Float weight ) const
{
	// TODO
	return MapPoseA2B( poseA, poseB );
}

Bool CSkeleton2SkeletonMapper::MapPoseB2A( SBehaviorGraphOutput& poseA, const SBehaviorGraphOutput& poseB, Float weight ) const
{
	// TODO
	return MapPoseB2A( poseA, poseB );
}

Bool CSkeleton2SkeletonMapper::MapPoseFullA2B( const SBehaviorGraphOutput& poseA, SBehaviorGraphOutput& poseB, Float weight ) const
{
	// TODO
	return MapPoseFullA2B( poseA, poseB );
}

Bool CSkeleton2SkeletonMapper::MapPoseFullB2A( SBehaviorGraphOutput& poseA, const SBehaviorGraphOutput& poseB, Float weight ) const
{
	// TODO
	return MapPoseFullB2A( poseA, poseB );
}

void CSkeleton2SkeletonMapper::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	if ( !GetParent()->IsA< CSkeleton >() )
	{
		HALT( "CSkeleton2SkeletonMapper's parent has to be CSkeleton" );

		m_skeletonB = NULL;
		m_mappingA2B.Clear();
		m_mappingB2A.Clear();
		m_motionScale = 1.f;
	}

	if ( property->GetName() == TXT("skeletonB") || property->GetName() == TXT("pelvisBoneName") )
	{
		m_mappingA2B.Clear();
		m_mappingB2A.Clear();

		const CSkeleton* skeletonA = GetSkeletonA();
		const CSkeleton* skeletonB = m_skeletonB.Get();

		FindSkeletonsMapping( skeletonA, skeletonB, m_mappingA2B );
		FindSkeletonsMapping( skeletonB, skeletonA, m_mappingB2A );

		m_motionScale = 1.f;
		m_skeletonsAreSimilar = false;

		if ( m_pelvisBoneName != CName::NONE )
		{
			const Int32 pelvisInxA = skeletonA->FindBoneByName( m_pelvisBoneName );
			const Int32 pelvisInxB = skeletonB->FindBoneByName( m_pelvisBoneName );

			if ( pelvisInxA != -1 && pelvisInxB != -1 )
			{
				const Bool hasPelvisA = pelvisInxA < skeletonA->GetBonesNum();
				const Bool hasPelvisB = pelvisInxB < skeletonB->GetBonesNum();

				if ( hasPelvisA && hasPelvisB )
				{
					const AnimQsTransform pelvisA = skeletonA->GetBoneMS( pelvisInxA );
					const AnimQsTransform pelvisB = skeletonB->GetBoneMS( pelvisInxB );

					// Hmm is this ok?
#ifdef USE_HAVOK_ANIMATION
					const Float zA = pelvisA.m_translation( 2 );
					const Float zB = pelvisB.m_translation( 2 );
#else
					const Float zA = pelvisA.Translation.Z;
					const Float zB = pelvisB.Translation.Z;
#endif
					m_motionScale = zA > 0.f && zB > 0.f ? zB / zA : 1.f;
				}
			}
		}

		m_skeletonsAreSimilar = CalcAreSkeletonsSimilar( m_mappingA2B, m_mappingB2A );
	}
}
