/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "meshTypeComponent.h"
#include "appearanceComponent.h"
#include "attachmentUtils.h"
#include "renderCommands.h"
#include "normalBlendComponent.h"
#include "normalBlendAttachment.h"
#include "hardAttachment.h"
#include "renderFrame.h"
#include "renderProxy.h"
#include "renderSkinningData.h"
#include "world.h"
#include "layer.h"
#include "fxDefinition.h"
#include "entityTemplate.h"
#include "entity.h"
#include "meshComponent.h"
#include "mesh.h"
#include "../core/dataError.h"
#include "meshSkinningAttachment.h"

IMPLEMENT_ENGINE_CLASS( CMeshTypeComponent );


CMeshTypeComponent::CMeshTypeComponent()
	: m_forceLODLevel( -1 )
	, m_forceAutoHideDistance( 0 )
	, m_shadowImportanceBias( MSIB_Default )
{
}

CMeshTypeComponent::~CMeshTypeComponent()
{
}


void CMeshTypeComponent::ApplyMeshColoring()
{
	CEntityTemplate* tpl = GetEntity() && GetEntity()->GetEntityTemplate() ? GetEntity()->GetEntityTemplate() : nullptr;
	if ( tpl )
	{
		CAppearanceComponent* appearanceComponent = CAppearanceComponent::GetAppearanceComponent( GetEntity() );
		CColorShift shift1;
		CColorShift shift2;
		bool tryNonAppearance = true;

		if ( appearanceComponent && tpl->FindColoringEntry( appearanceComponent->GetAppearance(), CName( GetName() ), shift1, shift2 ) )
		{
			tryNonAppearance = false;
		}

		if ( tryNonAppearance && !tpl->FindColoringEntry( CName( GetName() ), shift1, shift2 ) )
		{
			return;
		}

		Matrix m1, m2;
		shift1.CalculateColorShiftMatrix( m1 );
		shift2.CalculateColorShiftMatrix( m2 );
		SendColorShiftMatrices( m1, m2 );
	}
}

void CMeshTypeComponent::SendColorShiftMatrices( const Matrix& region0, const Matrix& region1 )
{
	if ( m_renderProxy )
	{
		( new CRenderCommand_UpdateColorShiftMatrices( m_renderProxy, region0, region1 ) )->Commit();
	}
}

void CMeshTypeComponent::OnAppearanceChanged( Bool added )
{
	// The component was just used in an appearance
	if ( added && GetParentAttachments().Empty() )
	{
		CMeshTypeResource* mesh = GetMeshTypeResource();
		if ( mesh )
		{
			mesh->ForceFullyLoad();

			CAnimatedComponent* animComponent = GetEntity()->GetRootAnimatedComponent();
			if ( animComponent && mesh && mesh->GetBoneCount() > 0 )
			{
				CAttachmentUtils::CreateSkinningAttachment( GetEntity(), animComponent, this );
			}
		}
	}
}

void CMeshTypeComponent::OnStreamIn()
{
	if ( IsUsedInAppearance() && GetParentAttachments().Empty() )
	{
		CMeshTypeResource* mesh = GetMeshTypeResource();
		if ( mesh )
		{
			mesh->ForceFullyLoad();

			CAnimatedComponent* animComponent = GetEntity()->GetRootAnimatedComponent();
			if ( animComponent && mesh && mesh->GetBoneCount() > 0 )
			{
				CAttachmentUtils::CreateSkinningAttachment( GetEntity(), animComponent, this );
			}
		}
	}

#ifndef NO_EDITOR
	Uint16 forceAutoHideDistance = GetEntity()->GetForceAutoHideDistance();
	if ( forceAutoHideDistance != 0 )
	{
		ForceAutoHideDistance( forceAutoHideDistance );
	}
#endif
}

Uint32 CMeshTypeComponent::GetMinimumStreamingDistance() const
{
	if ( HasForcedAutoHideDistance() )
	{
		return (Uint32)( Red::Math::MRound( ( (Float)m_forceAutoHideDistance ) * 1.1f ) );
	}

	// HACK++: make sure the mesh is loaded before we ask for the streaming distance...
	if ( IsA< CMeshComponent >() )
	{
		const CMeshComponent* mc = static_cast< const CMeshComponent* >( this );
		mc->GetMeshNow();
	}
	// HACK--

	CMeshTypeResource* mesh = GetMeshTypeResource();
	if ( mesh != nullptr )
	{
		return (Uint32)( Red::Math::MRound( mesh->GetAutoHideDistance()*1.1f ) );
	}
	else
	{
		return 0;
	}
}

void CMeshTypeComponent::OnColorVariantChanged()
{
	// Pass to base class
	TBaseClass::OnColorVariantChanged();

	// Apply coloring values to mesh
	ApplyMeshColoring();
}

void CMeshTypeComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	PC_SCOPE_PIX( CMeshTypeComponent_OnAttached );

	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_Bboxes );

	ApplyMeshColoring();
}

void CMeshTypeComponent::OnDetached( CWorld* world )
{
	TBaseClass::OnDetached( world );

	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_Bboxes );
}


void CMeshTypeComponent::OnInitializeProxy()
{
	if ( GetMeshTypeResource() )
	{
		// Pass to base class so we get a proxy created
		TBaseClass::OnInitializeProxy();

		// Apply color variance
		ApplyMeshColoring();

		// HACK: After creating a new proxy, we need to check if we're attached to any CNormalBlendComponent, and notify it that the proxies have
		// changed, so it can re-apply normal blend material.
		for ( TList< IAttachment* >::const_iterator attIter = GetParentAttachments().Begin(); attIter != GetParentAttachments().End(); ++attIter )
		{
			IAttachment* att = *attIter;
			if ( att->IsA< CNormalBlendAttachment >() )
			{
				SafeCast< CNormalBlendComponent >( att->GetParent() )->OnRenderProxiesChanged();
			}
		}
	}
}


void CMeshTypeComponent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag )
{
	// Pass to base class
	TBaseClass::OnGenerateEditorFragments( frame, flag );

	// Generate bounding boxes
	if ( flag == SHOW_Bboxes && IsVisible() )
	{
		frame->AddDebugBox( m_boundingBox, Matrix::IDENTITY, Color::LIGHT_YELLOW );
	}
}


void CMeshTypeComponent::ForceLODLevel( Int32 forceLODLevel )
{
	if ( m_forceLODLevel != forceLODLevel )
	{
		// Set new flag
		m_forceLODLevel = forceLODLevel;
		RefreshRenderProxies();
	}
}

void CMeshTypeComponent::ForceAutoHideDistance( Uint16 forceAutoHideDistance )
{
	forceAutoHideDistance = Min( forceAutoHideDistance, (Uint16) Red::Math::MRound( GetMaxAutohideDistance() ) );

	// add entry to mark that every instance has its own value
	CEntityTemplate* entityTemplate = GetEntity()->GetEntityTemplate();
	if ( entityTemplate )
	{
		entityTemplate->AddInstancePropertyEntry( CName( GetName() ), CName( TXT("forceAutoHideDistance") ) );
	}

	if ( m_forceAutoHideDistance != forceAutoHideDistance )
	{
		m_forceAutoHideDistance = forceAutoHideDistance;
		RefreshRenderProxies();
	}
}

Bool CMeshTypeComponent::IsSkinned() const
{
	return m_transformParent && m_transformParent->ToSkinningAttachment();
}

Bool CMeshTypeComponent::UsesAutoUpdateTransform()
{
	// If we are attached via skinning attachment to not automatically update transform
	if ( m_transformParent && m_transformParent->ToSkinningAttachment() )
	{
		return false;
	}

	if( m_renderProxy == nullptr )
	{
		m_drawableFlags |= DF_MissedUpdateTransform;
	}

	// Auto update transform
	return m_renderProxy != nullptr;
}


void CMeshTypeComponent::OnUpdateBounds()
{
	// This method is called VERY often, so please do not submit this line uncommented:
	//PC_SCOPE( MeshUpdateBounds );

	// Calculate from mesh
	CMeshTypeResource* mesh = GetMeshTypeResource();
	if ( mesh )
	{
		// No skinning, use static bounding box. If we're skinned our bounding box is set by the skinning attachment,
		// so we should do nothing.
		if ( !IsSkinned() )
		{
			// Use default bounding box from mesh
			m_boundingBox = GetLocalToWorld().TransformBox( mesh->GetBoundingBox() );
		}
	}
	else
	{
		// Use default implementation
		TBaseClass::OnUpdateBounds();
	}
}

Bool CMeshTypeComponent::HasForcedAutoHideDistance() const
{
	return m_forceAutoHideDistance != 0.0f;
}

Float CMeshTypeComponent::GetAutoHideDistance() const
{
	 return HasForcedAutoHideDistance() ? m_forceAutoHideDistance : ( GetMeshTypeResource() ? GetMeshTypeResource()->GetAutoHideDistance() : GetDefaultAutohideDistance() );
}

Bool CMeshTypeComponent::GetEffectParameterValue( CName paramName, EffectParameterValue& outValue ) const
{
	if ( paramName == CNAME( MeshEffectScalar0 ) )
	{
		outValue.SetFloat( m_defaultEffectParams.X );
		return true;
	}

	if ( paramName == CNAME( MeshEffectScalar1 ) )
	{
		outValue.SetFloat( m_defaultEffectParams.Y );
		return true;
	}

	if ( paramName == CNAME( MeshEffectScalar2 ) )
	{
		outValue.SetFloat( m_defaultEffectParams.Z );
		return true;
	}

	if ( paramName == CNAME( MeshEffectScalar3 ) )
	{
		outValue.SetFloat( m_defaultEffectParams.W );
		return true;
	}

	if ( paramName == CNAME( MeshEffectColor ) )
	{
		outValue.SetColor( m_defaultEffectColor );
		return true;
	}

	return false;
}

Bool CMeshTypeComponent::SetEffectParameterValue( CName paramName, const EffectParameterValue& value )
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

void CMeshTypeComponent::EnumEffectParameters( CFXParameters &outEffectParams )
{
	outEffectParams.AddParameter< Float >( CNAME( MeshEffectScalar0 ) );
	outEffectParams.AddParameter< Float >( CNAME( MeshEffectScalar1 ) );
	outEffectParams.AddParameter< Float >( CNAME( MeshEffectScalar2 ) );
	outEffectParams.AddParameter< Float >( CNAME( MeshEffectScalar3 ) );
	outEffectParams.AddParameter< Color >( CNAME( MeshEffectColor ) );
}

void CMeshTypeComponent::OnUpdateSkinning( const ISkeletonDataProvider* provider, IRenderSkinningData* renderSkinningData, const Box& boxMS, const Matrix& l2w, SMeshSkinningUpdateContext& skinningContext )
{
	RED_ASSERT( renderSkinningData, TXT( "No render skinning data?" ) );

	// Update mesh components bounding box from animated component data
	const Box boxWS = l2w.TransformBox( boxMS );
	SetBoundingBox( boxWS );
	// update component's l2w
	m_localToWorld = l2w;

	// Send new bounding box
	RenderProxyUpdateInfo updateInfo;
	updateInfo.m_localToWorld = &l2w;
	updateInfo.m_boundingBox = &boxWS;

	renderSkinningData->AdvanceWrite();

	// Send new data to render thread
	skinningContext.AddCommand_SkinningDataAndRelink( m_renderProxy, renderSkinningData, updateInfo );
}

void CMeshTypeComponent::OnUpdateTransformWithoutSkinning( const Box& boxMS, const Matrix& l2w, SMeshSkinningUpdateContext& skinningContext )
{
	// Update mesh components bounding box from animated component data
	const Box boxWS = l2w.TransformBox( boxMS );
	SetBoundingBox( boxWS );
	// update component's l2w
	m_localToWorld = l2w;

	// Send new bounding box
	RenderProxyUpdateInfo updateInfo;
	updateInfo.m_localToWorld = &l2w;
	updateInfo.m_boundingBox = &boxWS;

	// Send new data to render thread
	skinningContext.AddCommand_Relink( m_renderProxy, updateInfo );
}

#ifndef NO_EDITOR
void CMeshTypeComponent::UpdateRenderDistanceParams( const SMeshRenderParams& params )
{
	if ( m_renderProxy )
	{
		( new CRenderCommand_UpdateMeshRenderParams( m_renderProxy, params ) )->Commit();
	}
}
#endif

#ifndef NO_DATA_VALIDATION
void CMeshTypeComponent::OnCheckDataErrors( Bool isInTemplate ) const
{
	TBaseClass::OnCheckDataErrors( isInTemplate );

	// forces sync loading !!
	/*if ( CMesh * mesh = Cast< CMesh >( GetMeshTypeResource() ) )
	{
		const auto& handle = mesh->GetVerti();
		if ( (*handle).Empty() )
		{
			DATA_HALT( DES_Major, mesh, TXT("Rendering"), TXT( "Looks like mesh in MeshTypeComponent '%ls' has no vertices!"), 
				GetName().AsChar() );
		}
	}*/
}
#endif

Bool CMeshTypeComponent::IsRenderingReady() const
{
	if ( CMeshTypeResource* res = GetMeshTypeResource() )
	{
		return res->IsRenderingReady();
	}

	return true; // no resource == "ready" to not block on empty components
}

void CMeshTypeComponent::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	if ( property->GetName() == CNAME( forceAutoHideDistance ) )
	{
		ForceAutoHideDistance( m_forceAutoHideDistance );
	}
}
