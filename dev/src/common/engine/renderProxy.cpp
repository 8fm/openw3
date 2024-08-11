/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "renderProxy.h"
#include "mesh.h"
#include "drawableComponent.h"
#include "lightComponent.h"
#include "dimmerComponent.h"
#include "meshEnum.h"
#include "meshTypeResource.h"

//------

RenderProxyInitData::RenderProxyInitData( ERenderProxyType type, const Uint32 sizeofData )
{
#ifndef RED_FINAL_BUILD
	Red::MemorySet( this, 0xFE, sizeofData );
#endif
	m_type = type;
}

RenderProxyMeshInitData::RenderProxyMeshInitData()
	: RenderProxyInitData( RPT_Mesh, sizeof(RenderProxyMeshInitData) )
{
}

RenderProxyDimmerInitData::RenderProxyDimmerInitData()
	: RenderProxyInitData( RPT_Dimmer, sizeof(RenderProxyDimmerInitData) )
{
}

RenderProxyDecalInitData::RenderProxyDecalInitData()
	: RenderProxyInitData( RPT_SSDecal, sizeof(RenderProxyDecalInitData) )
{
}

RenderProxyLightInitData::RenderProxyLightInitData( ERenderProxyType type, const Uint32 sizeofData )
	: RenderProxyInitData( type, sizeofData )
{
}

RenderProxyPointLightInitData::RenderProxyPointLightInitData()
	: RenderProxyLightInitData( RPT_PointLight, sizeof(RenderProxyPointLightInitData) )
{
}

RenderProxySpotLightInitData::RenderProxySpotLightInitData()
	: RenderProxyLightInitData( RPT_SpotLight, sizeof(RenderProxySpotLightInitData) )
{
}

//------

RenderProxyInitInfo::RenderProxyInitInfo()
	: m_component( nullptr )
	, m_packedData( nullptr )
	, m_visibilityQuery( 0 )
	, m_entityGroup( nullptr )
	, m_usesVertexCollapse( false )
{
}

Box RenderProxyInitInfo::ExtractBoundingBox() const
{
	// From component
	if ( m_component )
	{
		if ( m_component->IsA< CBoundedComponent >() )
		{
			const CDrawableComponent* dc = static_cast< const CDrawableComponent* >( m_component );
			return dc->GetBoundingBox();
		}

		if ( m_component->IsA< CLightComponent >() )
		{
			const CLightComponent* dc = static_cast< const CLightComponent* >( m_component );
			return Box( dc->GetWorldPosition(), dc->GetRadius() );
		}

		if ( m_component->IsA< CDimmerComponent >() )
		{
			const CDimmerComponent* dc = static_cast< const CDimmerComponent* >( m_component );
			return dc->GetBoundingBox();
		}
	}

	// From cooked mesh
	if ( m_packedData && m_packedData->GetType() == RPT_Mesh )
	{
		const RenderProxyMeshInitData* packedMeshData = static_cast< const RenderProxyMeshInitData* >( m_packedData );
		const Box localBox = packedMeshData->m_mesh->GetBoundingBox();
		return m_packedData->m_localToWorld.TransformBox( localBox );
	}
	else if ( m_packedData && m_packedData->GetType() == RPT_PointLight )
	{
		const RenderProxyPointLightInitData* packedLightData = static_cast< const RenderProxyPointLightInitData* >( m_packedData );
		return Box( packedLightData->m_localToWorld.GetTranslation(), packedLightData->m_radius );
	}
	else if ( m_packedData && m_packedData->GetType() == RPT_SpotLight )
	{
		const RenderProxySpotLightInitData* packedLightData = static_cast< const RenderProxySpotLightInitData* >( m_packedData );
		return Box( packedLightData->m_localToWorld.GetTranslation(), packedLightData->m_radius );
	}
	else if ( m_packedData && m_packedData->GetType() == RPT_Dimmer )
	{
		const Box unitBox( Vector::ZEROS, 1.0f );
		return m_packedData->m_localToWorld.TransformBox( unitBox );
	}
	else if ( m_packedData && m_packedData->GetType() == RPT_SSDecal )
	{
		const Box unitBox( Vector::ZEROS, 0.5f );
		return m_packedData->m_localToWorld.TransformBox( unitBox );
	}

	// None 
	return Box();
}

Matrix RenderProxyInitInfo::ExtractLocalToWorld() const
{
	// From component
	if ( m_component )
	{
		return m_component->GetLocalToWorld();
	}
	else if ( m_packedData )
	{
		return m_packedData->m_localToWorld;
	}

	// None
	return Matrix::IDENTITY;
}

Matrix RenderProxyInitInfo::ExtractLocalMatrix() const
{
	// From component
	if ( m_component )
	{
		Matrix mat;
		m_component->CalcLocalTransformMatrix( mat );
		return mat;
	}
	else if ( m_packedData )
	{
		return m_packedData->m_localToWorld;
	}

	// None
	return Matrix::IDENTITY;
}

Uint8 RenderProxyInitInfo::ExtractRenderingPlane() const
{
	// From component
	if ( m_component && m_component->IsA< CDrawableComponent >() )
	{
		const CDrawableComponent* drawableComponent = static_cast< const CDrawableComponent* >( m_component );
		return (Uint8)drawableComponent->GetRenderingPlane();
	}
	else if ( m_packedData && m_packedData->GetType() == RPT_Mesh )
	{
		const RenderProxyMeshInitData* packedMeshData = static_cast< const RenderProxyMeshInitData* >( m_packedData );
		return packedMeshData->m_renderingPlane;
	}

	return 0;
}

Bool RenderProxyInitInfo::IsCameraTransform() const
{
	// From component
	if ( m_component && m_component->IsA< CDrawableComponent >() )
	{
		const CDrawableComponent* drawableComponent = static_cast< const CDrawableComponent* >( m_component );
		return drawableComponent->IsCameraTransformComponentWithoutRotation() || drawableComponent->IsCameraTransformComponentWithRotation();
	}
	else if ( m_packedData && m_packedData->GetType() == RPT_Mesh )
	{
		const RenderProxyMeshInitData* packedMeshData = static_cast< const RenderProxyMeshInitData* >( m_packedData );
		return packedMeshData->m_cameraTransformRotate || packedMeshData->m_cameraTransformTranslate;
	}

	return false;
}

Bool RenderProxyInitInfo::IsCameraTransformWithRotation() const
{
	// From component
	if ( m_component && m_component->IsA< CDrawableComponent >() )
	{
		const CDrawableComponent* drawableComponent = static_cast< const CDrawableComponent* >( m_component );
		return drawableComponent->IsCameraTransformComponentWithRotation();
	}
	else if ( m_packedData && m_packedData->GetType() == RPT_Mesh )
	{
		const RenderProxyMeshInitData* packedMeshData = static_cast< const RenderProxyMeshInitData* >( m_packedData );
		return packedMeshData->m_cameraTransformRotate;
	}


	return false;
}

Bool RenderProxyInitInfo::IsCameraTransformWithoutRotation() const
{
	// From component
	if ( m_component && m_component->IsA< CDrawableComponent >() )
	{
		const CDrawableComponent* drawableComponent = static_cast< const CDrawableComponent* >( m_component );
		return drawableComponent->IsCameraTransformComponentWithoutRotation();
	}
	else if ( m_packedData && m_packedData->GetType() == RPT_Mesh )
	{
		const RenderProxyMeshInitData* packedMeshData = static_cast< const RenderProxyMeshInitData* >( m_packedData );
		return packedMeshData->m_cameraTransformTranslate;
	}

	return false;
}

Bool RenderProxyInitInfo::IsCastingShadowsFromLocalLightsOnly() const
{
	// From component
	if ( m_component && m_component->IsA< CDrawableComponent >() )
	{
		const CDrawableComponent* drawableComponent = static_cast< const CDrawableComponent* >( m_component );
		return drawableComponent->IsCastingShadowsFromLocalLightsOnly();
	}

	return false;
}

RenderProxyUpdateInfo::RenderProxyUpdateInfo()
	: m_boundingBox( nullptr )
	, m_localToWorld( nullptr )
{
}

IRenderProxy::IRenderProxy( ERenderProxyType type )
	: m_type( type )
{
}

IRenderProxy::~IRenderProxy()
{
}
