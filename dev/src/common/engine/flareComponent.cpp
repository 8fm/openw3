/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "flareComponent.h"
#include "renderCommands.h"
#include "world.h"
#include "../core/dataError.h"
#include "../core/resourceUsage.h"
#include "fxDefinition.h"
#include "entity.h"
#include "materialInstance.h"
#include "layer.h"
#include "utils.h"

IMPLEMENT_ENGINE_CLASS( CFlareComponent );


CFlareComponent::CFlareComponent()
	: m_material( NULL )
{
}

void CFlareComponent::OnAttached( CWorld* world )
{
	// Pass to base class
	TBaseClass::OnAttached( world );

	// Add to editor fragments group
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_FlaresData );
}

void CFlareComponent::OnDetached( CWorld* world )
{
	// Pass to base class
	TBaseClass::OnDetached( world );

	// Remove from editor fragments group
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_FlaresData );
}

Uint32 CFlareComponent::GetMinimumStreamingDistance() const
{
	return (Uint32)Red::Math::MRound( m_parameters.m_flareRadius*1.1f );
}

void CFlareComponent::OnUpdateBounds()
{
	TBaseClass::OnUpdateBounds();

	const Float maxRadius = m_parameters.m_occlusionExtent; //< we only need occlusion extent since we introduces 'activeFlares' list in the scene
	m_boundingBox = GetLocalToWorld().TransformBox( Box ( Vector (-maxRadius, -maxRadius, -maxRadius), Vector (maxRadius, maxRadius, maxRadius) ) );
}

void CFlareComponent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag )
{
	// Pass to base class
	TBaseClass::OnGenerateEditorFragments( frame, flag );

	// Generate bounding boxes
	if ( flag == SHOW_FlaresData && IsVisible() )
	{
		// Draw boxes
		frame->AddDebugBox( GetBoundingBox(),	Matrix::IDENTITY, Color::YELLOW );
	}
}

Bool CFlareComponent::GetEffectParameterValue( CName paramName, EffectParameterValue &value /* out */ ) const
{
	return false;
}

Bool CFlareComponent::SetEffectParameterValue( CName paramName, const EffectParameterValue &value )
{
	if ( m_renderProxy )
	{
		if ( paramName == CNAME( MeshEffectScalar0 ) && value.IsFloat() )
		{
			const Float paramValue = value.GetFloat();
			const Vector paramVectorValue( paramValue, paramValue, paramValue, paramValue );
			( new CRenderCommand_UpdateEffectParameters( m_renderProxy, paramVectorValue, 0 ) )->Commit();
			return true;
		}

		if ( paramName == CNAME( MeshEffectScalar1 ) && value.IsFloat() )
		{
			const Float paramValue = value.GetFloat();
			const Vector paramVectorValue( paramValue, paramValue, paramValue, paramValue );
			( new CRenderCommand_UpdateEffectParameters( m_renderProxy, paramVectorValue, 1 ) )->Commit();
			return true;
		}

		if ( paramName == CNAME( MeshEffectScalar2 ) && value.IsFloat() )
		{
			const Float paramValue = value.GetFloat();
			const Vector paramVectorValue( paramValue, paramValue, paramValue, paramValue );
			( new CRenderCommand_UpdateEffectParameters( m_renderProxy, paramVectorValue, 2 ) )->Commit();
			return true;
		}

		if ( paramName == CNAME( MeshEffectScalar3 ) && value.IsFloat() )
		{
			const Float paramValue = value.GetFloat();
			const Vector paramVectorValue( paramValue, paramValue, paramValue, paramValue );
			( new CRenderCommand_UpdateEffectParameters( m_renderProxy, paramVectorValue, 3 ) )->Commit();
			return true;
		}


		if ( paramName == CNAME( MeshEffectColor ) && value.IsColor() )
		{
			const Vector paramVectorValue = value.GetColor().ToVector();
			( new CRenderCommand_UpdateEffectParameters( m_renderProxy, paramVectorValue, 4 ) )->Commit();
			return true;
		}
	}

	return false;
}

void CFlareComponent::EnumEffectParameters( CFXParameters &effectParams /* out */ )
{
	effectParams.AddParameter< Float >( CNAME( MeshEffectScalar0 ) );
	effectParams.AddParameter< Float >( CNAME( MeshEffectScalar1 ) );
	effectParams.AddParameter< Float >( CNAME( MeshEffectScalar2 ) );
	effectParams.AddParameter< Float >( CNAME( MeshEffectScalar3 ) );
	effectParams.AddParameter< Color >( CNAME( MeshEffectColor ) );
}

#ifndef NO_DATA_VALIDATION
void CFlareComponent::OnCheckDataErrors( Bool isInTemplate ) const
{
	// Pass to base class
	TBaseClass::OnCheckDataErrors( isInTemplate );

	// No material set
	if ( !m_material )
	{
		DATA_HALT( DES_Minor, CResourceObtainer::GetResource( this ), TXT("Rendering"), TXT("Flare component '%ls' in entity '%ls' has missing material"), GetName().AsChar(), GetEntity() ? GetEntity()->GetName().AsChar() : TXT("null") );
	}

	// Shader is gone
	if ( m_material && !m_material->GetMaterialDefinition() )
	{
		DATA_HALT( DES_Minor, CResourceObtainer::GetResource( this ), TXT("Rendering"), TXT("Flare component '%ls' in entity '%ls' uses invalid or missing shader"), GetName().AsChar(), GetEntity() ? GetEntity()->GetName().AsChar() : TXT("null") );
	}
}
#endif // NO_DATA_VALIDATION

#ifndef NO_RESOURCE_USAGE_INFO
void CFlareComponent::CollectResourceUsage( class IResourceUsageCollector& collector, const Bool isStremable ) const
{
	TBaseClass::CollectResourceUsage( collector, isStremable );

	if ( isStremable && m_material )
		collector.ReportResourceUsage( m_material.Get() );
}

#endif
