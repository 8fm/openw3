
#pragma once

#include "behaviorGraphOutput.h"
#include "../core/engineQsTransform.h"
#include "skeleton.h"

struct SMimicTrackPose
{
	DECLARE_RTTI_STRUCT( SMimicTrackPose )

	CName				m_name;
	TDynArray< Float >	m_tracks;
	TDynArray< Int32 >	m_mapping;
};

BEGIN_CLASS_RTTI( SMimicTrackPose );
	PROPERTY_RO( m_name, String::EMPTY );
	PROPERTY_RO( m_tracks, String::EMPTY );
	PROPERTY_RO( m_mapping, String::EMPTY );
END_CLASS_RTTI();

class CMimicFace : public CResource
{
	DECLARE_ENGINE_RESOURCE_CLASS( CMimicFace, CResource, "w3fac", "Mimic FAC" );	

	typedef TEngineQsTransformArray TMimicPose;

protected:
	TDynArray< SMimicTrackPose >	m_mimicTrackPoses;
	TDynArray< SMimicTrackPose >	m_mimicFilterPoses;
	THandle< CSkeleton >			m_mimicSkeleton;
	THandle< CSkeleton >			m_floatTrackSkeleton;
	TDynArray< TMimicPose >			m_mimicPoses;
	TDynArray< Int32 >				m_mapping;
	Int32							m_neckIndex;
	Int32							m_headIndex;
	TDynArray< Vector >				m_normalBlendAreas;

#ifndef NO_RESOURCE_IMPORT

public:
	struct FactoryInfo : public CResource::FactoryInfo< CMimicFace >
	{				
		TDynArray< SMimicTrackPose >	m_mimicTrackPoses;
		TDynArray< SMimicTrackPose >	m_mimicFilterPoses;
		AnimQsTransform*				m_poses;
		CSkeleton::FactoryInfo			m_mimicSkeleton;
		CSkeleton::FactoryInfo			m_floatTrackSkeleton;
		Int32							m_numPoses;
		Int32							m_numBones;

		TDynArray< Int32 >				m_mapper;

		Vector*							m_areas;
	};

	static CMimicFace* Create( const FactoryInfo& data );

#endif

public:
	CSkeleton* GetSkeleton() const;
	CSkeleton* GetFloatTrackSkeleton() const;

public:
	Uint32 GetMimicTrackPosesNum() const;
	Int32 FindMimicTrackPose( const CName& name ) const;
	const CName& GetMimicTrackPoseName( const Int32 poseIndex ) const;

	Uint32 GetMimicFilterPosesNum() const;
	Int32 FindMimicFilterPose( const CName& name ) const;
	const CName& GetMimicTrackFilterName( const Int32 filterIndex ) const;

	void GetNeckAndHead( Int32& neck, Int32& head ) const;

	void GetNormalBlendAreasNormalized( TDynArray< Vector >& areas ) const;

public: // Use with care!
	Bool AddTrackPose( Int32 poseNum, Float weight, SBehaviorGraphOutput& output ) const;
	Bool AddFilterPose( Int32 poseNum, Float weight, SBehaviorGraphOutput& output ) const;

	Bool GetMimicPose( Uint32 num, SBehaviorGraphOutput& pose ) const;
	void AddMimicPose( SBehaviorGraphOutput& mainPose, const SBehaviorGraphOutput& additivePose, Float weight ) const;
};

BEGIN_CLASS_RTTI( CMimicFace );
	PARENT_CLASS( CResource );
	PROPERTY_RO( m_mimicSkeleton, TXT("Mimic skeleton") );
	PROPERTY_RO( m_floatTrackSkeleton, TXT("Float track skeleton") );
	PROPERTY_RO( m_mimicPoses, TXT("Mimic poses") );
	PROPERTY_RO( m_mapping, TXT("Mapping") );
	PROPERTY_RO( m_mimicTrackPoses, TXT("Track poses") );
	PROPERTY_RO( m_mimicFilterPoses, TXT("Filter poses") );
	PROPERTY_RO( m_normalBlendAreas, TXT("Normal blend areas") );
	PROPERTY_RO( m_neckIndex, TXT("Neck index") );
	PROPERTY_RO( m_headIndex, TXT("Head index") );
END_CLASS_RTTI();

/*
Gives access to track and filter poses from custom, category and common CMimicFace objects just as if they were in one CMimicFace.

When searching for a pose with given name, custom poses are searched first, then category poses and then common
poses - this means that common poses are overridden by category poses which are in turn overridden by custom poses.

CExtendedMimics is lightweight and it's ok to create/destroy it frequently.
*/
class CExtendedMimics
{
public:
	CExtendedMimics( CMimicFace* customMimics, CMimicFace* categoryMimics = nullptr );
	~CExtendedMimics();

	// compiler generated cctor is ok
	// compiler generated op= is ok

	CMimicFace* GetCustomMimics() const;
	CMimicFace* GetCategoryMimics() const;
	CMimicFace* GetCommonMimics() const;

	Uint32 GetNumTrackPoses() const;
	Int32 FindTrackPose( const CName& name ) const;
	const CName& GetTrackPoseName( const Int32 trackPoseIndex ) const;
	Bool ApplyTrackPose( Int32 trackPoseIndex, Float weight, SBehaviorGraphOutput& output ) const;

	Uint32 GetNumFilterPoses() const;
	Int32 FindFilterPose( const CName& name ) const;
	const CName& GetFilterPoseName( const Int32 filterPoseIndex ) const;
	Bool ApplyFilterPose( Int32 filterPoseIndex, Float weight, SBehaviorGraphOutput& output ) const;

private:
	void TranslateTrackPoseIndex( Int32 trackPoseIndex, Int32& outIndex, const CMimicFace*& outMimics ) const;
	void TranslateFilterPoseIndex( Int32 filterPoseIndex, Int32& outIndex, const CMimicFace*& outMimics ) const;

	CMimicFace* m_customMimics;		// Custom mimics. Not owned by CExtendedMimics. Must not be nullptr.
	CMimicFace* m_categoryMimics;	// Category mimics. Not owned by CExtendedMimics. May be nullptr.
	CMimicFace* m_commonMimics;		// Common mimics. Not owned by CExtendedMimics. May be nullptr.
};

/*
Returns CMimicFace* used as custom mimics.

\return CMimicFace* used as custom mimics. This is never nullptr.
*/
RED_INLINE CMimicFace* CExtendedMimics::GetCustomMimics() const
{
	return m_customMimics;
}

/*
Returns CMimicFace* used as category mimics.

\return CMimicFace* used as category mimics. May be nullptr.
*/
RED_INLINE CMimicFace* CExtendedMimics::GetCategoryMimics() const
{
	return m_categoryMimics;
}

/*
Returns CMimicFace* used as common mimics.

\return CMimicFace* used as common mimics. May be nullptr.
*/
RED_INLINE CMimicFace* CExtendedMimics::GetCommonMimics() const
{
	return m_commonMimics;
}
