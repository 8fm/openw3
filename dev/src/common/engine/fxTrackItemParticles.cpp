/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "fxTrackItemParticles.h"
#include "particleSystem.h"
#include "animatedComponent.h"
#include "particleComponent.h"
#include "renderProxy.h"
#include "entity.h"
#include "layer.h"

IMPLEMENT_ENGINE_CLASS( CFXTrackItemParticles );

CFXTrackItemParticles::CFXTrackItemParticles()
	: CFXTrackItemCurveBase( 3, CNAME( Particles ) )
	, m_spawner( nullptr )
{
}

/// Runtime player for particles
class CFXTrackItemParticlesPlayData : public IFXTrackItemPlayData
{
public:
	const CFXTrackItemParticles*						m_trackItem;		//!< Data
	TDynArray< CParticleComponent* >					m_particles;		//!< Controlled particles component
	THandle< CEntity >									m_entity;			//!< Created entity
    Bool												m_asyncLoad;		//!< Is the resource being loaded asynchronously
	TSoftHandle< CParticleSystem >						m_particleSystem;	//!< ParticleSystem resource


public:
	CFXTrackItemParticlesPlayData( const CFXTrackItemParticles* trackItem, const TDynArray< CParticleComponent* >& pc, CEntity* entity, bool asyncLoad )
		: IFXTrackItemPlayData( !pc.Empty() ? pc[0] : NULL , trackItem )
		, m_trackItem( trackItem )
		, m_particles( pc )
		, m_entity( entity )
        , m_asyncLoad( asyncLoad )
	{
        ASSERT( m_trackItem );
		m_particleSystem = trackItem->GetParticleSystemResource();
	};

	~CFXTrackItemParticlesPlayData()
	{
		// Destroy entity
		CEntity* entity = m_entity.Get();
		if ( entity )
		{
			entity->Destroy();
			m_entity = NULL;
		}
		m_particleSystem.Release();
	}

	virtual void OnStop() {}

	virtual void OnTick( const CFXState& fxState, Float timeDelta )
	{
        if ( m_asyncLoad )
        {
            if ( m_particleSystem.GetAsync() != BaseSoftHandle::ALR_InProgress )
            {
                m_asyncLoad = false;
                CParticleSystem* ps = m_particleSystem.Get();
                if ( ps )
                {
                    for ( Uint32 i = 0; i < m_particles.Size(); ++i )
                    {
                        CParticleComponent* pc = m_particles[ i ];
                        pc->SetParticleSystem( ps );
                        pc->RefreshRenderProxies();
                    }
                }
                else
                {
                    return;
                }
            }
            else
            {
                return;
            }
        }

		for ( Uint32 i = 0; i < m_particles.Size(); ++i )
		{
			CParticleComponent* pc = m_particles[i];
			if ( pc )
			{
				// Evaluate curve value
				const Float globalEmission = m_trackItem->GetCurveValue( 0, fxState.GetCurrentTime() );
				const Float alpha = m_trackItem->GetCurveValue( 1, fxState.GetCurrentTime() );
				const Float size = m_trackItem->GetCurveValue( 2, fxState.GetCurrentTime() );
				pc->SetGlobalEmissionScale( globalEmission );

				CParticleComponent::SEffectInfo info;
				info.m_alpha = alpha;
				info.m_size = size;
				pc->SetEffectInfo( info );
			}

		}
	}

	CEntity* GetEffectEntity() override
	{
		return m_entity.Get();
	}

#ifndef NO_DEBUG_PAGES
	virtual void CollectBoundingBoxes( Box& box ) const
	{
		for ( Uint32 i = 0; i < m_particles.Size(); ++i )
		{
			CParticleComponent* pc = m_particles[i];
			if ( pc )
			{
				pc->GetRenderSideBoundingBox( box );
			}
		}
	}

	virtual void GetDescription( TDynArray< String >& descriptionLines ) const
	{
		descriptionLines.PushBack( TXT("PARTICLES") );
		String newLine;
		for ( Uint32 i=0; i<m_particles.Size(); ++i )
		{
			CParticleComponent* pc = m_particles[i];
			IRenderProxy* proxy = pc->GetRenderProxy();
			if ( proxy )
			{
				proxy->GetDescription( descriptionLines );
			}
		}
	}

	virtual Bool ValidateOptimization( TDynArray< String >* commentLines ) const
	{
		Bool result = true;
		for ( Uint32 i=0; i<m_particles.Size(); ++i )
		{
			CParticleComponent* pc = m_particles[i];
			IRenderProxy* proxy = pc->GetRenderProxy();
			if ( proxy )
			{
				if ( !proxy->ValidateOptimization( commentLines ) )
				{
					result = false;
				}
			}
		}

		return result;
	}
#endif
};

IFXTrackItemPlayData* CFXTrackItemParticles::OnStart( CFXState& fxState ) const
{
    // No spawner
    if ( !m_spawner )
    {
        WARN_ENGINE( TXT("No spawner for particles track '%ls' in effect '%ls' in '%ls'"), GetName().AsChar(), fxState.GetDefinition()->GetName().AsString().AsChar(), fxState.GetEntity()->GetFriendlyName().AsChar() );
        return NULL;
    }

	// Create particles entity
	CEntity* entity = NULL;

	// Get the amount of particles to spawn
	TDynArray< Uint32 > indices;
	Uint32 amountOfPC = m_spawner->AmountOfPC( fxState.GetEntity(), indices );
	
	// Limit the number of spawned particle components to some sane number
	if ( amountOfPC > 16 )
	{
		amountOfPC = 16;
	}

	// This table will hold handles to created particle components
	TDynArray< CParticleComponent* > particleComponents;
	particleComponents.Resize( amountOfPC );

	// Create particle components
	for ( Uint32 i = 0; i < amountOfPC; ++i )
	{
		// Calculate spawn position and rotation
		EntitySpawnInfo einfo;
		//einfo.m_name = TXT("CFXTrackItemParticles");
		if ( !m_spawner->Calculate( fxState.GetEntity(), einfo.m_spawnPosition, einfo.m_spawnRotation, indices[i] ) )
		{
			WARN_ENGINE( TXT("Invalid spawner for particles track '%ls' in effect '%ls' in '%ls'"), GetName().AsChar(), fxState.GetDefinition()->GetName().AsString().AsChar(), fxState.GetEntity()->GetFriendlyName().AsChar() );
			
			if ( entity )
			{
				entity->Destroy();
			}
			
			return NULL;
		}

		// If set, use bone name for effect position
		if( ! m_spawner->IsPositionFinal() && ! fxState.GetBoneName().Empty() )
		{
			CAnimatedComponent* animatedComponent = fxState.GetEntity()->GetRootAnimatedComponent();
			if( animatedComponent != NULL )
			{
				Int32 bone = animatedComponent->FindBoneByName( fxState.GetBoneName() );
				if( bone >= 0 )
				{
					Matrix worldSpace = animatedComponent->GetBoneMatrixWorldSpace( bone );
					einfo.m_spawnPosition = worldSpace.GetTranslation();

					//LOG_ENGINE( TXT( "LOGSHIT: using effect bone '%ls'" ), fxState.GetBoneName().AsChar() );
				}
				else
				{
					WARN_ENGINE( TXT( "Used bone name '%ls' for effect, but it is not present in '%ls'" ),
						fxState.GetBoneName().AsString().AsChar(), animatedComponent->GetFriendlyName().AsChar() );
				}
			}
			else
			{
				WARN_ENGINE( TXT( "Used bone name for effect, but animated component is NULL" ) );
			}
		}

		// Create dynamic entity for effect
		if ( !entity )
		{
			entity = fxState.CreateDynamicEntity( einfo );
			if ( !entity )
			{
				WARN_ENGINE( TXT("Unable to create particles for track '%ls' in effect '%ls' in '%ls'"), GetName().AsChar(), fxState.GetDefinition()->GetName().AsString().AsChar(), fxState.GetEntity()->GetFriendlyName().AsChar() );
				return NULL;
			}
		}

		// Create particles component
		CParticleComponent* pc = Cast< CParticleComponent >( entity->CreateComponent( ClassID< CParticleComponent >(), SComponentSpawnInfo() ) );

		// Not created, should not happen
		if ( !pc )
		{
			ASSERT( pc && "Particles component not created" );
			entity->Destroy();
			return NULL;
		}

		if( m_spawner->IsPositionFinal() || fxState.GetBoneName().Empty() )
		{
			// Update post spawn
			m_spawner->PostSpawnUpdate( fxState.GetEntity(), pc, indices[i] );
		}

		// Make sure all transforms are up to date
		entity->ForceUpdateTransformNodeAndCommitChanges();
		pc->ForceUpdateTransformNodeAndCommitChanges();

		// Initialize emission scale
		const Float val = GetCurveValue( fxState.GetCurrentTime() );
		pc->SetGlobalEmissionScale( val );

		// Update targetting
		pc->SetTarget( fxState.GetTargetNode(), fxState.GetTargetBone() );

		// Remember handle
		particleComponents[ i ] = pc;
	}

	ASSERT(amountOfPC == particleComponents.Size());

	// Create effect wrapper if particle components were created
	// Try to avoid calling GetAsync if we have no components
	if ( !particleComponents.Empty() )
	{
		CParticleSystem* ps = NULL;

		if ( fxState.GetDefinition()->IsStayInMemory() )
		{
			// If particle system is already preloaded, set it here, so it will be seen by reachability marker. 
			// This prevents unloading some very common particle systems (like aard).

			BaseSoftHandle::EAsyncLoadingResult state = m_particleSystem.GetAsync();
			if ( state == BaseSoftHandle::ALR_Loaded )
			{
				ps = m_particleSystem.Get();
				ASSERT( ps );

				for ( Uint32 i = 0; i < amountOfPC; ++i )
				{
					particleComponents[ i ]->SetParticleSystem( ps ); 
				}
			}
		}

		// Done
		return new CFXTrackItemParticlesPlayData( this, particleComponents, entity, ( ps == NULL ) ? true : false );
	}

	// Not created
	return NULL;
}

void CFXTrackItemParticles::PrefetchResources(TDynArray< TSoftHandle< CResource > >& requiredResources) const
{
	requiredResources.PushBack( (TSoftHandle< CResource >&)m_particleSystem );
}

void CFXTrackItemParticles::SetName( const String& name )
{
}

String CFXTrackItemParticles::GetName() const
{
	return TXT("Particles");
}

String CFXTrackItemParticles::GetCurveName( Uint32 i /*= 0 */ ) const
{
	switch( i )
	{
	case 0:
		return TXT("emission scale");
	case 1:
		return TXT("alpha");
	case 2:
		return TXT("size");
	default:
		return String::EMPTY;
	}
}

Bool CFXTrackItemParticles::UsesComponent( const CName& componentName ) const
{
	return m_spawner && m_spawner->UsesComponent( componentName );
}

const TSoftHandle< CParticleSystem >& CFXTrackItemParticles::GetParticleSystemResource() const
{
    return m_particleSystem;
}

void CFXTrackItemParticles::SetSpawner( IFXSpawner* spawner )
{
	if ( m_spawner && m_spawner != spawner )
	{
		m_spawner->Discard();
	}

	m_spawner = spawner;
	m_spawner->SetParent( this );
}