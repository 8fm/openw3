
#include "build.h"
#include "cutsceneEntity.h"
#include "cutscene.h"
#include "skeletalAnimatedComponent.h"
#include "skeleton.h"
#include "componentIterator.h"
#include "baseEngine.h"

IMPLEMENT_ENGINE_CLASS( CBgCutsceneEntity );

CBgCutsceneEntity::CBgCutsceneEntity()
{

}

#ifndef NO_EDITOR

void CBgCutsceneEntity::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	if ( property->GetName() == TXT("csTemplate") )
	{
		CheckCutsceneActors();
	}
}

#endif

Bool CBgCutsceneEntity::InitializeAnimatedEntity()
{
	return m_csTemplate.IsValid() && TBaseClass::InitializeAnimatedEntity();
}

void CBgCutsceneEntity::OnInitialized()
{
	const Float duration = m_csTemplate->GetDuration();
	if ( duration > 0.f )
	{
		static Float checkRandTime = 0.f;

		Float randTime = GEngine->GetRandomNumberGenerator().Get< Float >( 0.9f * duration );
		
		Uint32 counter = 0;
		while ( MAbs( randTime - checkRandTime ) < 0.1f && counter++ < 100 ) 
		{
			randTime = GEngine->GetRandomNumberGenerator().Get< Float >( 0.9f * duration );
		}

		checkRandTime = randTime;

		CSyncInfo info;
		info.m_prevTime = randTime;
		info.m_currTime = randTime;

		for ( ComponentIterator< CSkeletalAnimatedComponent > it( this ); it; ++it )
		{
			CSkeletalAnimatedComponent* comp = *it;

			comp->SyncTo( info );
		}
	}
	else
	{
		ASSERT( 0 );
	}

	if ( m_csTemplate->HasBoundingBoxes() )
	{
		TDynArray< Box > boxes;
		m_csTemplate->GetBoundingBoxes( boxes );

		Box box;
		m_box.Clear();

		const Uint32 size = boxes.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			box.AddBox( boxes[ i ] );
		}

		m_box.AddPoint( m_localToWorld.TransformPoint( box.Min ) );
		m_box.AddPoint( m_localToWorld.TransformPoint( box.Max ) );
		m_box.Min.SetW( 1.f );
		m_box.Max.SetW( 1.f );

		if ( !m_box.IsEmpty() )
		{
			m_state = 3;
		}
	}
	else
	{
		m_box = Box( GetWorldPositionRef(), 1.f );
	}
}

#ifndef NO_EDITOR

void CBgCutsceneEntity::CheckCutsceneActors()
{
	if ( m_csTemplate )
	{
		const TDynArray< CSkeletalAnimationSetEntry* >& animations = m_csTemplate->GetAnimations();
		for ( Uint32 i=0; i<animations.Size(); ++i )
		{
			// 1. Animation -> Component
			if ( animations[ i ] )
			{
				const CName& animName = animations[ i ]->GetName();

				CSkeletalAnimatedComponent* founded = NULL;

				for ( ComponentIterator< CSkeletalAnimatedComponent > it( this ); it; ++it )
				{
					CSkeletalAnimatedComponent* comp = *it;
					if ( comp->GetName() == animName.AsString() )
					{
						founded = comp;
						break;
					}
				}

				Bool createController = !founded || !founded->GetController() || !founded->GetController()->IsA< CSingleAnimationController >();

				if ( !founded )
				{
					// Create new component
					SComponentSpawnInfo componentSpawnInfo;
					componentSpawnInfo.m_name = animName.AsString();
					founded = SafeCast< CSkeletalAnimatedComponent >( CreateComponent( CSkeletalAnimatedComponent::GetStaticClass(), componentSpawnInfo ) );
				}

				// Set animset
				founded->SetAnimset( m_csTemplate.Get() );

				// Set skeleton
				/*const SCutsceneActorDef* actorDef = m_csTemplate->GetActorDefinition( founded->GetName() );
				if ( actorDef && actorDef->GetEntityTemplate() )
				{
					const CEntityTemplate* templ = actorDef->GetEntityTemplate();
					
					founded->SetSkeleton( skeleton );
				}
				else
				{
					ASSERT( actorDef );
				}*/
				
				if ( createController )
				{
					ASSERT( founded );

					CSingleAnimationController* controller = CreateObject< CSingleAnimationController >( founded );
					controller->SetAnimation( animName );
					founded->SetController( controller );
				}
			}
		}

		// 2. Component -> Animation
		TDynArray< CSkeletalAnimatedComponent* > toRemove;
		for ( ComponentIterator< CSkeletalAnimatedComponent > it( this ); it; ++it )
		{
			CSkeletalAnimatedComponent* comp = *it;
			const CName compName = CName( comp->GetName() );

			Bool found = false;

			const TDynArray< CSkeletalAnimationSetEntry* >& animations = m_csTemplate->GetAnimations();
			for ( Uint32 i=0; i<animations.Size(); ++i )
			{
				if ( animations[ i ] && animations[ i ]->GetName() == compName )
				{
					found = true;
					break;
				}
			}

			if ( !found )
			{
				toRemove.PushBack( comp );
			}
		}
		for ( Uint32 i=0; i<toRemove.Size(); ++i )
		{
			DestroyComponent( toRemove[ i ] );
		}
	}
}

#endif

//////////////////////////////////////////////////////////////////////////

#ifndef NO_EDITOR_ENTITY_VALIDATION

Bool CBgCutsceneEntity::OnValidate( TDynArray< String >& log ) const
{
	if ( !TBaseClass::OnValidate( log ) )
	{
		return false;
	}

	//...

	return true;
}

#endif
