/**
 * Copyright © 2008 CD Projekt Red. All Rights Reserved.
 */
#pragma once

// This class is not used at all
#if 0

class CSkeletonMapper : public CResource
{
	DECLARE_ENGINE_RESOURCE_CLASS( CSkeletonMapper, CResource, "w2rigm", "Skeleton Mapper" );	

protected:
	String									m_skeletonA;
	String									m_skeletonB;
#ifdef USE_HAVOK_ANIMATION
	THavokDataBuffer< hkaSkeletonMapper >	m_ragdollToHighResMapper;
	THavokDataBuffer< hkaSkeletonMapper >	m_highResToRagdollMapper;
#endif
private:
	CSkeletonMapper();

public:
	void OnSerialize( IFile &file );

public:
	struct FactoryInfo : public CResource::FactoryInfo< CSkeletonMapper >
	{				
		String					m_skeletonA;
		String					m_skeletonB;
#ifdef USE_HAVOK_ANIMATION
		hkaSkeletonMapper*		m_ragdollToHighResMapper;
		hkaSkeletonMapper*		m_highResToRagdollMapper;
#endif
	};

	Int32 NumBonesLowRes() const;
	Int32 NumBonesHighRes() const;

	void MapHighToLow( Matrix* boneTransformsHighRes, Matrix* boneTransformsLowRes ) const;
	void MapLowToHigh( Matrix* boneTransformsLowRes, Matrix* boneTransformsHighRes ) const;

	static CSkeletonMapper* Create( const FactoryInfo& data );

public:
#ifdef USE_HAVOK_ANIMATION
	const hkaSkeletonMapper*	GetRagdollToHighResMapper() const;
	hkaSkeletonMapper*			GetRagdollToHighResMapper();

	const hkaSkeletonMapper*	GetHighResToRagdollMapper() const;
	hkaSkeletonMapper*			GetHighResToRagdollMapper();
#endif
	const String&				GetSkeletonNameA() const { return m_skeletonA; }
	String&						GetSkeletonNameA() { return m_skeletonA; }
	const String&				GetSkeletonNameB() const  { return m_skeletonB; }
	String&						GetSkeletonNameB()  { return m_skeletonB; }
};

BEGIN_CLASS_RTTI( CSkeletonMapper );
	PARENT_CLASS( CResource );
	PROPERTY_RO( m_skeletonA, TXT("Name of Skeleton A") );
	PROPERTY_RO( m_skeletonB, TXT("Name of Skeleton B") );
END_CLASS_RTTI();

#endif

//////////////////////////////////////////////////////////////////////////

class CSkeleton2SkeletonMapper : public CObject
{
	DECLARE_ENGINE_CLASS( CSkeleton2SkeletonMapper, CObject, 0 );

	//						m_skeletonA; skeletonA is object's parent
	THandle< CSkeleton >	m_skeletonB;
	CName					m_pelvisBoneName;

	TDynArray< Int32 >		m_mappingA2B;	// A2B and B2A have the same data but different order
	TDynArray< Int32 >		m_mappingB2A;

	Float					m_motionScale;

	Bool					m_skeletonsAreSimilar;

public:
	CSkeleton2SkeletonMapper();

	virtual void OnPropertyPostChange( IProperty* property );

public:
	const CSkeleton* GetSkeletonA() const;
	RED_INLINE const CSkeleton* GetSkeletonB() const			{ return m_skeletonB.Get(); }

	RED_INLINE const TDynArray< Int32 > GetMappingA2B() const	{ return m_mappingA2B; }
	RED_INLINE const TDynArray< Int32 > GetMappingB2A() const	{ return m_mappingB2A; }

	RED_INLINE Float GetMotionScaleA2B() const				{ return m_motionScale; }
	RED_INLINE Float GetMotionScaleB2A() const				{ ASSERT( m_motionScale > 0.f ); return 1.f / m_motionScale; }

	RED_INLINE Bool AreSkeletonsSimilar() const				{ return m_skeletonsAreSimilar; }

public:
	Bool MapPoseA2B( const SBehaviorGraphOutput& poseA, SBehaviorGraphOutput& poseB ) const;
	Bool MapPoseB2A( SBehaviorGraphOutput& poseA, const SBehaviorGraphOutput& poseB ) const;

	Bool MapPoseFullA2B( const SBehaviorGraphOutput& poseA, SBehaviorGraphOutput& poseB ) const;
	Bool MapPoseFullB2A( SBehaviorGraphOutput& poseA, const SBehaviorGraphOutput& poseB ) const;

	Bool MapPoseA2B( const SBehaviorGraphOutput& poseA, SBehaviorGraphOutput& poseB, Float weight ) const;
	Bool MapPoseB2A( SBehaviorGraphOutput& poseA, const SBehaviorGraphOutput& poseB, Float weight ) const;

	Bool MapPoseFullA2B( const SBehaviorGraphOutput& poseA, SBehaviorGraphOutput& poseB, Float weight ) const;
	Bool MapPoseFullB2A( SBehaviorGraphOutput& poseA, const SBehaviorGraphOutput& poseB, Float weight ) const;
};

BEGIN_CLASS_RTTI( CSkeleton2SkeletonMapper );
	PARENT_CLASS( CObject );
	PROPERTY_EDIT( m_skeletonB, TXT("") );
	PROPERTY_EDIT( m_pelvisBoneName, TXT("") );
	PROPERTY_EDIT( m_motionScale, TXT("") );
	PROPERTY_RO( m_mappingA2B, TXT("") );
	PROPERTY_RO( m_mappingB2A, TXT("") );
	PROPERTY_RO( m_skeletonsAreSimilar, TXT("") );
END_CLASS_RTTI();
