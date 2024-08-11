
#include "build.h"
#include "animationController.h"
#include "skeletalAnimationSet.h"
#include "playedAnimation.h"
#include "skeletalAnimatedComponent.h"
#include "skeleton.h"
#include "baseEngine.h"

IMPLEMENT_ENGINE_CLASS( IAnimationController );
IMPLEMENT_ENGINE_CLASS( CSingleAnimationController );
IMPLEMENT_ENGINE_CLASS( CSequentialAnimationController );
IMPLEMENT_ENGINE_CLASS( CRandomAnimationController );
IMPLEMENT_ENGINE_CLASS( CRandomWithWeightAnimationController );

//////////////////////////////////////////////////////////////////////////

SAnimationControllerPose::SAnimationControllerPose( CPoseHandle pose )
	: m_pose( pose )
{}

SAnimationControllerPose::~SAnimationControllerPose()
{}

//////////////////////////////////////////////////////////////////////////

IAnimationController::IAnimationController()
	: m_skeleton( NULL )
	, m_set( NULL )
	, m_collectEvents( false )
{

}

IAnimationController::~IAnimationController()
{

}

Bool IAnimationController::Init( const CSkeleton* skeleton, const CSkeletalAnimationSet* set )
{
	m_skeleton = skeleton;
	m_set = set;

	return m_skeleton && m_set;
}

void IAnimationController::Destroy()
{

}

void IAnimationController::CollectEvents( Bool flag )
{
	m_collectEvents = flag;
}

Bool IAnimationController::IsCollectingEvents() const
{
	return m_collectEvents;
}

//////////////////////////////////////////////////////////////////////////

CSingleAnimationController::CSingleAnimationController()
	: m_animation( NULL )
{

}

Bool CSingleAnimationController::Init( const CSkeleton* skeleton, const CSkeletalAnimationSet* set )
{
	if ( m_animation )
	{
		ASSERT( !m_animation );

		StopAnimation();
	}

	Bool ret = IAnimationController::Init( skeleton, set );
	if ( ret )
	{
		const CSkeletalAnimationSetEntry* animation = m_set->FindAnimation( m_animationName );
		if ( animation && animation->GetAnimation() )
		{
			m_animation = new CPlayedSkeletalAnimation( animation, m_skeleton );
			Bool ok = m_animation->Play( true, 1.f, 0.f, 0.f, SAT_Normal, false );
			if ( !ok )
			{
				StopAnimation();
			}
			return ok;
		}
	}
	return false;
}

void CSingleAnimationController::StopAnimation()
{
	if ( m_animation )
	{
		delete m_animation;
		m_animation = NULL;
	}
}

void CSingleAnimationController::Destroy()
{
	StopAnimation();
}

Bool CSingleAnimationController::Update( Float timeDelta )
{
	if ( m_animation )
	{
		m_animation->Update( timeDelta );
		return true;
	}
	return false;
}

Bool CSingleAnimationController::Sample( const CSkeletalAnimatedComponent * ac, SAnimationControllerPose& pose ) const
{
	ASSERT( pose.m_pose );

	if ( m_animation )
	{
		Bool ret = m_animation->Sample( *pose.m_pose );
		if ( ret && IsCollectingEvents() )
		{
			m_animation->CollectEvents( ac, *pose.m_pose );
		}
		return ret;
	}

	return false;
}

void CSingleAnimationController::SyncTo( const CSyncInfo& info )
{
	if ( m_animation )
	{
		m_animation->SynchronizeTo( info );
	}
}

Bool CSingleAnimationController::GetSyncInfo( CSyncInfo& info )
{
	if ( m_animation )
	{
		m_animation->GetSyncInfo( info );
		return true;
	}
	return false;
}

void CSingleAnimationController::RandSync()
{
	if ( m_animation )
	{
		const Float duration = m_animation->GetDuration();
		if ( duration > 0.f )
		{
			const Float time = GEngine->GetRandomNumberGenerator().Get< Float >( duration );

			CSyncInfo info;
			info.m_currTime = time;
			info.m_prevTime = time;

			m_animation->SynchronizeTo( info );
		}
	}
}

void CSingleAnimationController::CalcBox( Box& box ) const
{
	if ( m_animation && m_animation->HasBoundingBox() )
	{
		box = m_animation->GetBoundingBox();
	}
	else
	{
		IAnimationController::CalcBox( box );
	}
}

//////////////////////////////////////////////////////////////////////////

CSequentialAnimationController::CSequentialAnimationController()
	: m_animation( NULL )
	, m_currIndex( 0 )
	, m_startingOffsetRange( 0.f )
	, m_startingOffsetBias( 0.f )
{

}

Bool CSequentialAnimationController::Init( const CSkeleton* skeleton, const CSkeletalAnimationSet* set )
{
	Bool ret = IAnimationController::Init( skeleton, set );
	if ( ret )
	{
		ret = PlayNextAnimation();

		if ( ret && ( m_startingOffsetBias > 0.f || m_startingOffsetRange > 0.f ) )
		{
			ASSERT( m_animation );

			const Float duration = m_animation->GetDuration();

			m_animation->SetTime( Min( 0.99f * duration, duration * m_startingOffsetBias + GEngine->GetRandomNumberGenerator().Get< Float >( duration * m_startingOffsetRange ) ) );
		}
	}
	return ret;
}

void CSequentialAnimationController::Destroy()
{
	StopAnimation();
}

Bool CSequentialAnimationController::Update( Float timeDelta )
{
	if ( m_animation )
	{
		const Float currTime = m_animation->GetTime();
		const Float duration = m_animation->GetDuration();
		const Float speed = m_animation->GetSpeed();

		Float nextTime = timeDelta * speed + currTime;
		if ( nextTime > duration )
		{
			Float rest = duration - nextTime;

			if ( PlayNextAnimation() )
			{
				ASSERT( m_animation );
				m_animation->Update( rest );
			}
			else
			{
				return false;
			}
		}
		else
		{
			m_animation->Update( timeDelta );
		}

		return true;
	}
	return false;
}

Bool CSequentialAnimationController::Sample( const CSkeletalAnimatedComponent * ac, SAnimationControllerPose& pose ) const
{
	ASSERT( pose.m_pose );
	ASSERT( m_animation );

	if ( m_animation )
	{
		Bool ret = m_animation->Sample( *pose.m_pose );
		if ( ret && IsCollectingEvents() )
		{
			m_animation->CollectEvents( ac, *pose.m_pose );
		}
		return ret;
	}

	return false;
}

Bool CSequentialAnimationController::PlayNextAnimation()
{
	CName animName;

	if ( CalcNextAnim( animName ) )
	{
		StopAnimation();

		return PlayAnimation( animName );
	}

	return false;
}

Bool CSequentialAnimationController::PlayAnimation( const CName& animName )
{
	const CSkeletalAnimationSetEntry* animation = m_set->FindAnimation( animName );
	if ( animation && animation->GetAnimation() )
	{
		m_animation = new CPlayedSkeletalAnimation( animation, m_skeleton );
		Bool ok = m_animation->Play( false, 1.f, 0.f, 0.f, SAT_Normal, false );
		if ( !ok )
		{
			StopAnimation();
		}
		else if ( HasSpeedTable() )
		{
			m_animation->SetSpeed( RandSpeed() );
		}

		return ok;
	}
	return false;
}

void CSequentialAnimationController::StopAnimation()
{
	if ( m_animation )
	{
		delete m_animation;
		m_animation = NULL;
	}
}

Bool CSequentialAnimationController::CalcNextAnim( CName& animName )
{
	Uint32 animSize = m_animations.Size();
	if ( animSize == 0 )
	{
		return false;
	}

	if ( m_currIndex + 1 < animSize )
	{
		++m_currIndex;
	}
	else
	{
		m_currIndex = 0;
	}

	animName = m_animations[ m_currIndex ];

	return true;
}

Bool CSequentialAnimationController::HasSpeedTable() const
{
	return m_speeds.Size() > 0;
}

Float CSequentialAnimationController::RandSpeed() const
{
	Uint32 index = GEngine->GetRandomNumberGenerator().Get< Uint32 >( m_speeds.SizeInt() );
	return m_speeds[ index ];
}

void CSequentialAnimationController::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	if ( property->GetName() == TXT("speeds") )
	{
		for ( Uint32 i=0; i<m_speeds.Size(); ++i )
		{
			if ( MAbs( m_speeds[ i ] ) < 0.0001f )
			{
				m_speeds[ i ] = 1.f;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////

Bool CRandomAnimationController::Init( const CSkeleton* skeleton, const CSkeletalAnimationSet* set )
{
	m_generator.Setup( 0, m_animations.Size() );

	return CSequentialAnimationController::Init( skeleton, set );
}

Bool CRandomAnimationController::CalcNextAnim( CName& animName )
{
	Uint32 animSize = m_animations.Size();
	if ( animSize == 0 )
	{
		return false;
	}

	m_currIndex = m_generator.Rand();

	animName = m_animations[ m_currIndex ];

	return true;
}

//////////////////////////////////////////////////////////////////////////

void CRandomWithWeightAnimationController::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	if ( property->GetName() == TXT("weights") || property->GetName() == TXT("animations") )
	{
		m_weights.Resize( m_animations.Size() );
		
		for ( Uint32 i=0; i<m_weights.Size(); ++i )
		{
			if ( MAbs( m_weights[ i ] ) < 0.0001f )
			{
				m_weights[ i ] = 1.f;
			}
		}
	}
}

Bool CRandomWithWeightAnimationController::CalcNextAnim( CName& animName )
{
	if ( m_weights.Empty() || m_animations.Empty() )
	{
		return false;
	}

	m_currIndex = GetRandomIndex( m_weights );

	animName = m_animations[ m_currIndex ];

	return true;
}

Uint32 CRandomWithWeightAnimationController::GetRandomIndex( const TDynArray< Float > &weights )
{
	ASSERT( !weights.Empty() );

	Float weightsSum = 0.0f;
	for ( TDynArray< Float >::const_iterator ci = weights.Begin(); ci != weights.End(); ++ci )
	{
		weightsSum += *ci;
	}

	Float randomFloat = GEngine->GetRandomNumberGenerator().Get< Float >( weightsSum );

	for ( Uint32 index = 0; index < weights.Size(); ++index )
	{
		randomFloat -= weights[ index ];
		if ( randomFloat <= 0 )
		{
			return index;
		}
	}

	// Should never reach here
	return 0;
}
