/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../core/engineQsTransform.h"
#include "animMath.h"
#include "../core/resource.h"

struct SBehaviorGraphOutput;

class CMimicFaces : public CResource
{
	DECLARE_ENGINE_RESOURCE_CLASS( CMimicFaces, CResource, "w2faces", "Mimic faces" );	

public:
	typedef TEngineQsTransformArray TMimicPose;

protected:
	THandle< CSkeleton >	m_mimicSkeleton;
	TDynArray< TMimicPose >	m_mimicPoses;
	TDynArray< Int32 >		m_mapping;
	Int32					m_neckIndex;
	Int32					m_headIndex;
	TDynArray< Vector >		m_normalBlendAreas;

#ifndef NO_RESOURCE_IMPORT

public:
	struct FactoryInfo : public CResource::FactoryInfo< CMimicFaces >
	{
#ifdef USE_HAVOK_ANIMATION
		hkaSkeleton*	m_skeleton;
		hkaAnimation*	m_animation;
#endif

		AnimQsTransform*  m_poses;
		Int32				m_numPoses;
		Int32				m_numBones;
		TDynArray<Int32>	m_mapper;
		Vector*			m_areas;
	};

	static CMimicFaces* Create( const FactoryInfo& data );

#endif

public:
	CSkeleton*			GetMimicSkeleton() const;
	Uint32				GetMimicPoseNum() const;
	const TMimicPose	GetMimicPose( Uint32 pose ) const;
	RED_INLINE const TDynArray<Vector>& GetNormalBlendAreas() const { return m_normalBlendAreas; }
	void				GetNeckAndHead( Int32& neck, Int32& head ) const;

	// Use with care!
	Bool				GetMimicPose( Uint32 num, SBehaviorGraphOutput& pose ) const;

	void				AddPose( SBehaviorGraphOutput& mainPose, const SBehaviorGraphOutput& additivePose, Float weight ) const;
};

BEGIN_CLASS_RTTI( CMimicFaces );
	PARENT_CLASS( CResource );
	PROPERTY( m_mimicSkeleton );
	PROPERTY( m_mimicPoses );
	PROPERTY( m_mapping );
	PROPERTY( m_neckIndex );
	PROPERTY( m_headIndex );
	PROPERTY( m_normalBlendAreas );
END_CLASS_RTTI();
