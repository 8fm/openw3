/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "meshImpostor.h"
#include "game.h"
#include "layer.h"
#include "layerGroup.h"
#include "layerInfo.h"
#include "entity.h"

IMPLEMENT_ENGINE_CLASS( CImpostorMeshComponent );

void CImpostorMeshComponent::OnAttached( CWorld* world )
{	
	TBaseClass::OnAttached( world );
	m_groupToHide = NULL;
	if ( !GGame->IsActive() )
		return;

	CObject* entity = GetParent();
	if ( entity->IsA<CEntity>() )
	{
		CObject* layer = entity->GetParent();
		if ( entity->GetParent()->IsA<CLayer>() )
		{
			CLayer* layer = SafeCast<CLayer>( entity->GetParent() );
			if ( layer->GetLayerInfo() )
			{
				CLayerGroup* group = layer->GetLayerInfo()->GetLayerGroup();

				CFilePath path( m_layerGroupName );
				for ( Uint32 i = 0; i < path.GetDirectories().Size(); i++ )
				{
					if ( path.GetDirectories()[i] == TXT("..") )
					{
						group = group->GetParentGroup();
					} 
					else 
					{
						group = group->FindGroup( path.GetDirectories()[i] );
					}


					if ( group == NULL )
					{

						break;
					}
				}

				if ( group )
				{
					m_groupToHide = group->FindGroup( path.GetFileName() );
				}
				if ( m_groupToHide == NULL )
				{
					WARN_ENGINE( TXT("problem with finding group of the impostor %s, using path %s"), this->m_name.AsChar(), m_layerGroupName.AsChar() );
				}
			}
		}
	}
}

Float CImpostorMeshComponent::GetStreamingTransparency() const
{
/*	// No setting
	if ( m_groupToHide == NULL || !m_groupToHide->IsLoaded() )
	{
		return 0.0f;
	}

	// Fade in/out
	Float timeDiffLoadIn = (Float)Red::System::Clock::GetInstance().GetTimer().GetSeconds() - m_groupToHide->GetLastLoadInTime();
	Float timeDiffLoadOut = (Float)Red::System::Clock::GetInstance().GetTimer().GetSeconds() - m_groupToHide->GetLastLoadOutRequestTime();
	if ( timeDiffLoadIn < timeDiffLoadOut )
	{
		// Last request was load-in.
		Float multiplier = m_groupToHide->GetFadeInTime() != 0.0f ? 1.0f / m_groupToHide->GetFadeInTime() : 1000.0f;
		return Clamp<Float>( timeDiffLoadIn * multiplier, 0.f, 1.0f);
	}
	else
	{	
		// Last request was load-out
		Float multiplier = m_groupToHide->GetFadeOutTime() != 0.0f ? 1.0f / m_groupToHide->GetFadeOutTime() : 1000.0f;
		return 1.0f - Clamp<Float>( timeDiffLoadOut * multiplier, 0.f, 1.0f);
	}*/

	return 0.0f;
}

