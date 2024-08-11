
#include "build.h"
#include "animatedEntity.h"
#include "skeletalAnimatedComponent.h"
#include "animationManager.h"
#include "drawableComponent.h"
#include "skeleton.h"
#include "componentIterator.h"

IMPLEMENT_ENGINE_CLASS( CAnimatedEntity );

CAnimatedEntity::CAnimatedEntity()
	: m_state( 0 )
{

}

void CAnimatedEntity::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	if ( InitializeAnimatedEntity() )
	{
		GAnimationManager->AddAsync( this );
	}
}

void CAnimatedEntity::OnAttachFinished( CWorld* world )
{
	TBaseClass::OnAttachFinished( world );

	if ( m_state > 0 )
	{
		// Force pose after initialization (then we know times for animations)
		ForceFirstFrame();
	}
}

void CAnimatedEntity::OnDetached( CWorld* world )
{
	if ( m_state > 0 )
	{
		GAnimationManager->RemoveAsync( this );
	}

	Deinitialize();

	TBaseClass::OnDetached( world );
}

Bool CAnimatedEntity::InitializeAnimatedEntity()
{
	ASSERT( ::SIsMainThread() ) ;
	ASSERT( m_state == 0 );

	Uint32 num = 0;
	for ( ComponentIterator< CSkeletalAnimatedComponent > it( this ); it; ++it )
	{
		CSkeletalAnimatedComponent* comp = *it;

		// Force initialization
		comp->Initialize();

		if ( !comp->IsValid() )
		{
			return false;
		}

		num++;
	}

	if ( num == 0 )
	{
		return false;
	}
	else if ( num == 1 )
	{
		m_state = 1;
	}

	m_state = 2;

	OnInitialized();

	return true;
}

void CAnimatedEntity::Deinitialize()
{
	m_state = 0;

	for ( ComponentIterator< CSkeletalAnimatedComponent > it( this ); it; ++it )
	{
		CSkeletalAnimatedComponent* comp = *it;

		comp->Deinitialize();
	}
}

void CAnimatedEntity::OnInitialized()
{
	for ( ComponentIterator< CSkeletalAnimatedComponent > it( this ); it; ++it )
	{
		CSkeletalAnimatedComponent* comp = *it;

		comp->RandSync();
	}

	m_box = Box( GetWorldPositionRef(), 1.f );

	TBaseClass::OnInitialized();
}

EAsyncAnimPriority CAnimatedEntity::GetPriority() const
{
	return AAP_Low;
}

Box CAnimatedEntity::GetBox() const
{
	return m_box;
}

void CAnimatedEntity::DoAsyncTick( Float dt )
{
	// It is not solution, only temp
	if ( IsDetaching() || !IsAttached() )
	{
		return;
	}

	// Animations
	if ( m_state == 1 )
	{
		CSkeletalAnimatedComponent* root = static_cast< CSkeletalAnimatedComponent* >( m_components[ 0 ] );

		root->UpdateAsync( dt );
		root->CalcBox( m_box );
	}
	else if ( m_state == 2 )
	{
		m_box.Clear();

		for ( ComponentIterator< CSkeletalAnimatedComponent > it( this ); it; ++it )
		{
			CSkeletalAnimatedComponent* sac = *it;

			sac->UpdateAsync( dt );

			Box tempBox;
			sac->CalcBox( tempBox );

			m_box.AddBox( tempBox );
		}

		ASSERT( !m_box.IsEmpty() );
	}
	else if ( m_state == 3 )
	{
		for ( ComponentIterator< CSkeletalAnimatedComponent > it( this ); it; ++it )
		{
			CSkeletalAnimatedComponent* sac = *it;

			sac->UpdateAsync( dt );
		}
	}

	// Components
	/*for ( ComponentIterator< CDrawableComponent > it( this ); it; ++it )
	{
		CDrawableComponent* comp = *it;
		if ( comp->UsesAutoUpdateTransform() )
		{
			comp->OnUpdateTransform();
		}
	}*/
}

void CAnimatedEntity::ForceFirstFrame()
{
	DoAsyncTick( 0.f );
}
