
#include "build.h"
#include "virtualAnimation.h"

IMPLEMENT_ENGINE_CLASS( VirtualAnimation );

VirtualAnimation::VirtualAnimation() 
	: m_time( 0.f)
	, m_startTime( 0.f )
	, m_endTime( 1.f )
	, m_speed( 1.f )
	, m_track( 0 )
	, m_useMotion( true )
	, m_animset( NULL )
	, m_cachedAnimation( NULL ) 
	, m_blendIn( 0.f )
	, m_blendOut( 0.f )
	, m_weight( 1.f )
	, m_boneToExtract( 1 )
{

}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( VirtualAnimationMotion );

VirtualAnimationMotion::VirtualAnimationMotion()
	: m_startTime( 0.f )
	, m_endTime( 1.f )
	, m_blendIn( 0.f )
	, m_blendOut( 0.f )
{

}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( VirtualAnimationPoseFK );

VirtualAnimationPoseFK::VirtualAnimationPoseFK()
	: m_time( 0.f )
	, m_controlPoints( 1.f, 0.f, -1.f, 0.0f )
{

}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( VirtualAnimationPoseIK );

VirtualAnimationPoseIK::VirtualAnimationPoseIK()
	: m_time( 0.f )
{

}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( VirtualAnimationLayer );

const TDynArray< VirtualAnimation >& VirtualAnimationLayer::GetAnimations( EVirtualAnimationTrack track ) const
{
	return m_baseTrack;
}

//////////////////////////////////////////////////////////////////////////
