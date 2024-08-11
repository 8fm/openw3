/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderHelpers.h"
#include "renderProxyBrushFace.h"
#include "renderProxyMesh.h"
#include "renderProxyApex.h"
#include "renderProxyPointLight.h"
#include "renderProxyParticles.h"
#include "renderProxyFlare.h"
#include "renderProxyDecal.h"
#include "renderProxyStripe.h"
#include "renderProxyFur.h"
#include "renderProxySpeedTree.h"
#include "renderProxySpotLight.h"
#include "renderProxyDimmer.h"
#include "renderProxyMorphedMesh.h"
#include "renderProxySwarm.h"
#include "../engine/morphedMeshComponent.h"
#include "../engine/destructionSystemComponent.h"
#include "../engine/clothComponent.h"
#include "../engine/furComponent.h"
#include "../engine/decalComponent.h"
#include "../engine/dimmerComponent.h"
#include "../engine/stripeComponent.h"
#include "../engine/pointLightComponent.h"
#include "../engine/spotLightComponent.h"
#include "../engine/flareComponent.h"
#include "../engine/swarmRenderComponent.h"
#include "../engine/destructionComponent.h"
#include "renderProxyDestructionMesh.h"

IRenderProxy* CRenderInterface::CreateProxy( const RenderProxyInitInfo& info )
{
	// Cooked mesh
	if ( info.m_packedData )
	{
		const auto type = info.m_packedData->GetType();
		if ( type == RPT_Mesh )
		{
			return new CRenderProxy_Mesh( info );
		}
		else if ( type == RPT_Dimmer )
		{
			return new CRenderProxy_Dimmer( info );
		}
		else if ( type == RPT_SSDecal )
		{
			return new CRenderProxy_Decal( info );
		}
		else if ( type == RPT_PointLight )
		{
			return new CRenderProxy_PointLight( info );
		}
		else if ( type == RPT_SpotLight )
		{
			return new CRenderProxy_SpotLight( info );
		}
		else
		{
			return nullptr;
		}
	}

	// We should have component given
	if ( info.m_component )
	{
		// Fur
		if ( const CFurComponent* furCmp = Cast< CFurComponent >( info.m_component ) )
		{
			if ( furCmp->IsUsingFur() )
			{
				return new CRenderProxy_Fur( info );
			}
			else
			{
				return new CRenderProxy_Mesh( info );
			}
		}

		// Mesh or destruction system 
		// (destruction system is rendered like skinned mesh with just a few exceptions,
		// so creating separate proxy class for it doesn't make any sense)
		//if ( info.m_component->IsA< CMeshComponent >() || 
		if ( info.m_component->IsA< CMeshComponent >() && !info.m_component->IsA< CFurComponent >() )
		{
			return new CRenderProxy_Mesh( info );
		}

#ifdef USE_APEX
		if ( info.m_component->IsA< CDestructionSystemComponent >() || info.m_component->IsA< CClothComponent >() )
		{
			return new CRenderProxy_Apex( info );
		}
#endif
		if ( info.m_component->IsA< CDestructionComponent >()  )
		{
			const CDestructionComponent* component = Cast< CDestructionComponent >( info.m_component );
			if( component->IsFractured() )
			{
				return new CRenderProxy_DestructionMesh( info );
			}
			else
			{
				return new CRenderProxy_Mesh( info );
			}
		}
		// Point light
		if ( info.m_component->IsA< CPointLightComponent >() )
		{
			return new CRenderProxy_PointLight( info );
		}

		// Spot light
		if ( info.m_component->IsA< CSpotLightComponent >() )
		{
			return new CRenderProxy_SpotLight( info );
		}

		// Particle emitter
		if ( info.m_component->IsA< CParticleComponent >() )
		{
			return new CRenderProxy_Particles( info );
		}

		// Flare
		if ( info.m_component->IsA< CFlareComponent >() )
		{
			return new CRenderProxy_Flare( info );
		}

		// Decal
		if ( info.m_component->IsA< CDecalComponent >() )
		{
			return new CRenderProxy_Decal( info );
		}

		// Stripe
		if ( info.m_component->IsA< CStripeComponent >() )
		{
			return new CRenderProxy_Stripe( info );
		}

		// Dimmer
		if ( info.m_component->IsA< CDimmerComponent >() )
		{
			return new CRenderProxy_Dimmer( info );
		}

		// Morph
		if ( info.m_component->IsA< CMorphedMeshComponent >() )
		{
			return new CRenderProxy_MorphedMesh( info );
		}

		// Swarm
		if ( info.m_component->IsA< CSwarmRenderComponent >() )
		{
			return new CRenderProxy_Swarm( info );
		}
	}

	// No proxy created
	return NULL;
}

#ifdef USE_SPEED_TREE

IRenderObject* CRenderInterface::CreateSpeedTreeProxy()
{
	return new CRenderProxy_SpeedTree();
}

#endif
