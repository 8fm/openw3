
#include "build.h"
#include "animationGameParams.h"

IMPLEMENT_ENGINE_CLASS( CSkeletalAnimationTrajectoryParam );

void CSkeletalAnimationTrajectoryParam::OnSerialize( IFile& file )
{
	ISkeletalAnimationSetEntryParam::OnSerialize( file );

	if ( file.IsGarbageCollector() )
	{
		return;
	}

	m_data.Serialize( file );
}

Bool CSkeletalAnimationTrajectoryParam::IsParamValid() const
{
	return m_data.m_pointsMSO.Size() > m_data.m_syncFrame;
}

Vector CSkeletalAnimationTrajectoryParam::GetSyncPointMS() const
{
	ASSERT( IsParamValid() );

	return m_data.m_pointsMSO[ m_data.m_syncFrame ];
}

#ifndef NO_EDITOR

void CSkeletalAnimationTrajectoryParam::Init( AnimationTrajectoryData& data )
{
	m_data = data;
}

#endif

const AnimationTrajectoryData& CSkeletalAnimationTrajectoryParam::GetData() const
{
	return m_data;
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_RTTI_ENUM( EAnimationAttackType );
IMPLEMENT_ENGINE_CLASS( CSkeletalAnimationAttackTrajectoryParam );

CSkeletalAnimationAttackTrajectoryParam::CSkeletalAnimationAttackTrajectoryParam()
	: m_type( AAT_None )
	, m_hitDuration( 0.1f )
	, m_postHitEnd( 0.f )
	, m_slowMotionStart( 0.f )
	, m_slowMotionEnd( 0.f )
	, m_dampOutEnd( 0.f )
	, m_slowMotionTimeFactor( 0.01f )
{
	
}

void CSkeletalAnimationAttackTrajectoryParam::OnSerialize( IFile& file )
{
	ISkeletalAnimationSetEntryParam::OnSerialize( file );

	if ( file.IsGarbageCollector() )
	{
		return;
	}

	m_dataL.Serialize( file );
	m_dataR.Serialize( file );
}

Bool CSkeletalAnimationAttackTrajectoryParam::IsParamValid() const
{
	return ( m_dataL.m_pointsMSO.Size() > m_dataL.m_syncFrame || m_dataR.m_pointsMSO.Size() > m_dataR.m_syncFrame ); // && m_type != AAT_None;
}

Bool CSkeletalAnimationAttackTrajectoryParam::GetSyncPointLeftMS( Vector& point ) const
{
	const Bool isValid = m_dataL.m_pointsMSO.Size() > m_dataL.m_syncFrame;
	if ( isValid )
	{
		point = m_dataL.m_pointsMSO[ m_dataL.m_syncFrame ];
	}
	return isValid;
}

Bool CSkeletalAnimationAttackTrajectoryParam::GetSyncPointRightMS( Vector& point ) const
{
	const Bool isValid = m_dataR.m_pointsMSO.Size() > m_dataR.m_syncFrame;
	if ( isValid )
	{
		point = m_dataR.m_pointsMSO[ m_dataR.m_syncFrame ];
	}
	return isValid;
}

#ifndef NO_EDITOR

void CSkeletalAnimationAttackTrajectoryParam::Init( AnimationTrajectoryData& dataL, AnimationTrajectoryData& dataR )
{
	m_dataL = dataL;
	m_dataR = dataR;
}

#endif

const AnimationTrajectoryData& CSkeletalAnimationAttackTrajectoryParam::GetDataL() const
{
	return m_dataL;
}

const AnimationTrajectoryData& CSkeletalAnimationAttackTrajectoryParam::GetDataR() const
{
	return m_dataR;
}

void CSkeletalAnimationAttackTrajectoryParam::GetTagId( CName& tagId ) const
{
	tagId = m_tagId;
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CSkeletalAnimationHitParam );

CSkeletalAnimationHitParam::CSkeletalAnimationHitParam()
	: m_pointMS( Vector::ZEROS )
	, m_directionMS( Vector::ZEROS )
{

}

void CSkeletalAnimationHitParam::OnSerialize( IFile& file )
{
	ISkeletalAnimationSetEntryParam::OnSerialize( file );

	if ( file.IsGarbageCollector() )
	{
		return;
	}

	file << m_pointMS;
	file << m_directionMS;
	file << m_boneName;
}

Bool CSkeletalAnimationHitParam::IsParamValid() const
{
	return m_pointMS.W > 0.f;
}

const Vector& CSkeletalAnimationHitParam::GetPointMS() const
{
	return m_pointMS;
}

const Vector& CSkeletalAnimationHitParam::GetDirectionMS() const
{
	return m_directionMS;
}

#ifndef NO_EDITOR

void CSkeletalAnimationHitParam::Init( const CSkeletalAnimationHitParam::InitData& init )
{
	m_pointMS = init.m_pointMS;
	m_directionMS = init.m_directionMS;
	m_boneName = init.m_boneName;
}

#endif

//////////////////////////////////////////////////////////////////////////
