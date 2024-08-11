/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "fxTrackItemFlare.h"
#include "fxTrackGroup.h"
#include "fxTrackItem.h"
#include "flareComponent.h"
#include "animatedComponent.h"
#include "renderProxy.h"
#include "entity.h"
#include "layer.h"

IMPLEMENT_ENGINE_CLASS( CFXTrackItemFlare );

CFXTrackItemFlare::CFXTrackItemFlare()
	: CFXTrackItemCurveBase( 1 ), m_material( NULL )
{
}

/// Runtime player for flare
class CFXTrackItemFlarePlayData : public IFXTrackItemPlayData
{
public:
	const CFXTrackItemFlare*							m_trackItem;		//!< Data
	THandle< CEntity >									m_entity;			//!< Created entity
	THandle< CFlareComponent >							m_flare;			//!< Controlled flare component

public:
	CFXTrackItemFlarePlayData( const CFXTrackItemFlare* trackItem, CFlareComponent* fc, CEntity* entity )
		: IFXTrackItemPlayData( fc, trackItem )
		, m_trackItem( trackItem )
		, m_flare( fc )
		, m_entity( entity )
	{
	};

	~CFXTrackItemFlarePlayData()
	{	
		// Destroy entity
		CEntity* entity =  m_entity.Get();
		if ( entity )
		{
			entity->Destroy();
			m_entity = NULL;
		}
	}

	virtual void OnStop() {}

	virtual void OnTick( const CFXState& fxState, Float timeDelta )
	{
		if ( m_flare )
		{
			// Evaluate curve value
			const Float intensity = m_trackItem->GetCurveValue( fxState.GetCurrentTime() );

			EffectParameterValue effectValue;
			effectValue.SetFloat( intensity );

			m_flare->SetEffectParameterValue(CNAME(MeshEffectScalar0), effectValue);
		}
	}
};

IFXTrackItemPlayData* CFXTrackItemFlare::OnStart( CFXState& fxState ) const
{
	// No spawner
	if ( !m_spawner )
	{
		WARN_ENGINE( TXT("No spawner for flare track '%ls' in effect '%ls' in '%ls'"), GetName().AsChar(), fxState.GetDefinition()->GetName().AsString().AsChar(), fxState.GetEntity()->GetFriendlyName().AsChar() );
		return NULL;
	}

	// Create flare entity
	CEntity* entity = NULL;
	EntitySpawnInfo einfo;
	//einfo.m_name = TXT("CFXTrackItemFlare");
	if ( !m_spawner->Calculate( fxState.GetEntity(), einfo.m_spawnPosition, einfo.m_spawnRotation, 0 ) )
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

	if(!entity)
	{
		entity = fxState.CreateDynamicEntity( einfo );
		if(!entity)
		{
			WARN_ENGINE( TXT("Unable to create flare for track '%ls' in effect '%ls' in '%ls'"), GetName().AsChar(), fxState.GetDefinition()->GetName().AsString().AsChar(), fxState.GetEntity()->GetFriendlyName().AsChar() );
			return NULL;
		}
	}

	//Create flare component
	CFlareComponent* fc = Cast< CFlareComponent >(entity ->CreateComponent( ClassID< CFlareComponent >(), SComponentSpawnInfo()));
	if ( !fc )
	{
		ASSERT( fc && "Flare component not created" );
		entity->Destroy();
		return NULL;
	}

	//Initialize material & params
	fc->SetMaterial(m_material.Get());
	fc->SetFlareParameters(m_parameters);
	fc->RefreshRenderProxies();

	//Initialize intensity
	const Float intensity = GetCurveValue(fxState.GetCurrentTime());
	EffectParameterValue effectValue;
	effectValue.SetFloat( intensity );
	fc->SetEffectParameterValue(CNAME(MeshEffectScalar0), effectValue);

	if( m_spawner->IsPositionFinal() || fxState.GetBoneName().Empty() )
	{
		// Update post spawn
		m_spawner->PostSpawnUpdate( fxState.GetEntity(), fc, 0 );
	}

	// Make sure all transforms are up to date
	entity->ForceUpdateTransformNodeAndCommitChanges();
	fc->ForceUpdateTransformNodeAndCommitChanges();

	// Done
	return new CFXTrackItemFlarePlayData( this, fc, entity );
}

Bool CFXTrackItemFlare::UsesComponent( const CName& componentName ) const
{
	return m_spawner && m_spawner->UsesComponent( componentName );
}

void CFXTrackItemFlare::SetName( const String& name )
{
}

String CFXTrackItemFlare::GetName() const
{
	return TXT("Flare");
}

String CFXTrackItemFlare::GetCurveName( Uint32 i /*= 0 */ ) const
{
	switch( i )
	{
	case 0:
		return TXT("intensity");
	default:
		return String::EMPTY;
	}
}

void CFXTrackItemFlare::SetSpawner(IFXSpawner* spawner)
{
	if ( m_spawner && m_spawner != spawner )
	{
		m_spawner->Discard();
	}

	m_spawner = spawner;
	m_spawner->SetParent( this );
}
