
#pragma once

#include "controlRigIncludes.h"
#include "../core/engineQsTransform.h"
#include "skeletalAnimationSet.h"

enum EVirtualAnimationTrack
{
	VAT_None		= -1,
	VAT_Base		= 0,
	VAT_Override	= 1,
	VAT_Additive	= 2,
	VAT_Last		= 3
};

struct VirtualAnimationID
{
	Int32					m_index;
	EVirtualAnimationTrack	m_track;

	RED_INLINE Bool operator ==( const VirtualAnimationID& a ) const
	{
		return m_index == a.m_index && m_track == a.m_track;
	}
};

struct VirtualAnimation
{
	DECLARE_RTTI_STRUCT( VirtualAnimation )

	CName	m_name;
	Float	m_time;
	Float	m_startTime;
	Float	m_endTime;
	Float	m_speed;
	Float	m_weight;
	Int32	m_track;
	Bool	m_useMotion;

	TDynArray< Int32 >						m_bones;
	TDynArray< Float >						m_weights;
	Int32									m_boneToExtract;

	TSoftHandle< CSkeletalAnimationSet >	m_animset;
	CSkeletalAnimationSetEntry*				m_cachedAnimation;
	Float									m_blendIn;
	Float									m_blendOut;

	VirtualAnimation();

	RED_INLINE Float GetDuration() const { return ( m_endTime - m_startTime ) / m_speed; }
};

BEGIN_CLASS_RTTI( VirtualAnimation );
	PROPERTY_RO( m_name, TXT( "Animation name" ) );
	PROPERTY_EDIT( m_time, TXT( "Virtual animation time" ) );
	PROPERTY_EDIT( m_startTime, TXT( "Animation start time (offset)" ) );
	PROPERTY_EDIT( m_endTime, TXT( "Animation end time" ) );
	PROPERTY_EDIT( m_speed, TXT( "Animation speed" ) );
	PROPERTY_EDIT( m_weight, TXT( "Animation weight" ) );
	PROPERTY_EDIT( m_useMotion, TXT( "Use motion for this animation" ) );
	PROPERTY_EDIT( m_boneToExtract, TXT( "Bone to extract - e.g. for characters is 'trajectory' bone" ) );
	PROPERTY( m_bones );
	PROPERTY( m_weights );
	PROPERTY_EDIT( m_blendIn, TXT( "" ) );
	PROPERTY_EDIT( m_blendOut, TXT( "" ) );
	PROPERTY( m_track );
	PROPERTY( m_animset );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

typedef Int32 VirtualAnimationMotionID;

struct VirtualAnimationMotion
{
	DECLARE_RTTI_STRUCT( VirtualAnimationMotion )

	Float			m_startTime;
	Float			m_endTime;
	Float			m_blendIn;
	Float			m_blendOut;

	VirtualAnimationMotion();

	RED_INLINE Float GetDuration() const { return m_endTime - m_startTime; }
};

BEGIN_CLASS_RTTI( VirtualAnimationMotion );
	PROPERTY_EDIT( m_startTime, TXT( "Start time" ) );
	PROPERTY_EDIT( m_endTime, TXT( "End time" ) );
	PROPERTY_EDIT( m_blendIn, TXT( "" ) );
	PROPERTY_EDIT( m_blendOut, TXT( "" ) );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

typedef Int32 VirtualAnimationPoseFKID;

struct VirtualAnimationPoseFK
{
	DECLARE_RTTI_STRUCT( VirtualAnimationPoseFK )

	Float							m_time;
	Vector							m_controlPoints;
	TDynArray< Int32 >				m_indices;
	TEngineQsTransformArray			m_transforms;

	VirtualAnimationPoseFK();
};

BEGIN_CLASS_RTTI( VirtualAnimationPoseFK );
	PROPERTY_EDIT( m_time, TXT( "Time" ) );
	PROPERTY_RO( m_controlPoints, TXT( "" ) );
	PROPERTY_RO( m_indices, TXT("") );
	PROPERTY_RO( m_transforms, TXT("") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

typedef Int32 VirtualAnimationPoseIKID;

struct VirtualAnimationPoseIK
{
	DECLARE_RTTI_STRUCT( VirtualAnimationPoseIK )

	Float						m_time;

	TDynArray< ETCrEffectorId > m_ids;
	TDynArray< Vector >			m_positionsMS;
	TDynArray< EulerAngles >	m_rotationsMS;
	TDynArray< Float >			m_weights;

	VirtualAnimationPoseIK();
};

BEGIN_CLASS_RTTI( VirtualAnimationPoseIK );
	PROPERTY_EDIT( m_time, TXT( "Time" ) );
	PROPERTY_RO( m_ids, TXT( "" ) );
	PROPERTY_RO( m_positionsMS, TXT( "" ) );
	PROPERTY_RO( m_rotationsMS, TXT( "" ) );
	PROPERTY_RO( m_weights, TXT( "" ) );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

struct VirtualAnimationLayer
{
	DECLARE_RTTI_STRUCT( VirtualAnimationLayer );

private:
	TDynArray< VirtualAnimation >	m_baseTrack;
	TDynArray< VirtualAnimation >	m_overrideTrack;
	TDynArray< VirtualAnimation >	m_additiveTrack;

public:
	const TDynArray< VirtualAnimation >& GetAnimations( EVirtualAnimationTrack track ) const;
};

BEGIN_CLASS_RTTI( VirtualAnimationLayer );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class IVirtualAnimationContainer
{
public:
	virtual const TDynArray< VirtualAnimation >& GetVirtualAnimations( EVirtualAnimationTrack track ) const = 0;
	virtual const TDynArray< VirtualAnimationMotion >& GetVirtualMotions() const = 0;
	virtual const TDynArray< VirtualAnimationPoseFK >& GetVirtualFKs() const = 0;
	virtual const TDynArray< VirtualAnimationPoseIK >& GetVirtualIKs() const = 0;

#ifndef NO_EDITOR
	virtual Bool AddAnimation( VirtualAnimation& anim, EVirtualAnimationTrack track ) = 0;
	virtual Bool RemoveAnimation( const VirtualAnimationID& animation ) = 0;
	virtual void SetAnimation( const VirtualAnimationID& animation , const VirtualAnimation& dest ) = 0;
#endif
};



