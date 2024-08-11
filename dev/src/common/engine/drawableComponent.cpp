/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "drawableComponent.h"
#include "appearanceComponent.h"
#include "renderCommands.h"
#include "../core/scriptStackFrame.h"
#include "../core/objectIterator.h"
#include "../core/resourceUsage.h"
#include "clipMap.h"
#include "hardAttachment.h"
#include "renderVisibilityQuery.h"
#include "lightComponent.h"
#include "furComponent.h"
#include "renderProxy.h"
#include "world.h"
#include "layer.h"
#include "baseEngine.h"
#include "entity.h"

IMPLEMENT_ENGINE_CLASS( CDrawableComponent );
IMPLEMENT_RTTI_BITFIELD( ELightChannel );
IMPLEMENT_RTTI_BITFIELD( EDrawableFlags );
IMPLEMENT_RTTI_ENUM( ERenderingPlane );

#ifndef NO_EDITOR
Bool CDrawableComponent::m_forceNoAutohideDebug = false;
#endif

CDrawableComponent::CDrawableComponent()
	: m_drawableFlags( DF_IsVisible )
	, m_lightChannels( LC_Default )
	, m_renderingPlane( RPl_Scene )
{
	SetStreamed( true );
}

struct OldDrawableComponentProperties
{
	Bool	m_isVisible;
	Bool	m_isAcceptingLighting;
	Bool	m_isCastingShadows;

	void Reset()
	{
		m_isVisible = true;
		m_isAcceptingLighting = true;
		m_isCastingShadows = false;
	}

	Uint8 TranslateToDrawableFlags() const
	{
		Uint8 flags = 0;
		if ( m_isVisible ) flags |= DF_IsVisible;
		if ( m_isCastingShadows ) flags |= DF_CastShadows;
		if ( !m_isAcceptingLighting ) flags |= DF_NoLighting;
		return flags;
	}
};

void CDrawableComponent::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	// Refresh component in the render scene
	RefreshRenderProxies();
}

void CDrawableComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	PC_SCOPE_PIX( CDrawableComponent_OnAttached );

	InitFromAppearance();
	// Attach to rendering scene
	ConditionalAttachToRenderScene( world );
}

void CDrawableComponent::InitFromAppearance()
{
	PC_SCOPE_PIX( CDrawableComponent_InitFromAppearance );
	// Vertex collapse code
	CEntity* parent = GetEntity();
	if ( parent && parent->GetEntityTemplate() )
	{
		CAppearanceComponent* appearanceComponent = CAppearanceComponent::GetAppearanceComponent( parent );

		if ( appearanceComponent )
		{
			const CEntityAppearance* appearance = parent->GetEntityTemplate()->GetAppearance( appearanceComponent->GetAppearance(), true );

			if ( appearance && appearance->IsCollapsed(this) )
			{
				m_drawableFlags |= DF_Collapsed;
			}
		}
	}
}

void CDrawableComponent::OnDetached( CWorld* world )
{
	TBaseClass::OnDetached( world );

	// Detach from rendering scene
	ConditionalAttachToRenderScene( world );

	ASSERT( m_renderProxy == NULL );
}

// Extract master (topmost) entity from CNode chain
static CEntity* GetMasterEntity( CComponent* component )
{
	CEntity* masterEntity = component->GetEntity();
	CHardAttachment* transformParent = component->GetTransformParent();
	while ( transformParent != NULL )
	{
		// go level up
		CNode* parentNode = transformParent->GetParent(); 
		if( !parentNode ) break;

		transformParent = parentNode->GetTransformParent();

		// extract topmost entry on the way
		if ( parentNode->AsComponent() )
		{
			masterEntity = parentNode->AsComponent()->GetEntity();
		}
		else if ( parentNode->AsEntity() )
		{
			masterEntity = parentNode->AsEntity();
		}
		else
		{
			continue;
		}

		// Stop search at the first master entity candidate, to avoid 
		// potential problems with dynamic attachments (mounting player to horse etc).
		if ( masterEntity && masterEntity->IsMasterEntity() )
		{
			break;
		}
	}

	return masterEntity;
}

void CDrawableComponent::RefreshEntityGroupBinding()
{
	if ( m_renderProxy != NULL )
	{
		// Extract the rendering entity group from the master entity
		CEntity* masterEntity = GetMasterEntity( this );
		IRenderEntityGroup* entityGroup = masterEntity ? masterEntity->GetRenderEntityGroup() : NULL;

		// Bind/Unbind the render group
		( new CRenderCommand_BindEntityGroupToProxy( entityGroup, m_renderProxy ) )->Commit();
	}
}

void CDrawableComponent::RelinkProxy( SUpdateTransformContext* context )
{
	// Update in scene
	// PARTICLE COMPONENT HAS A MODIFIED VERSION OF THIS CODE. REMEMBER TO MAKE CHANGES IN BOTH FILES.
	if ( IsAttached() && m_renderProxy )
	{
		// Relink the position and bounding box of this proxy on the rendering thread
		RenderProxyUpdateInfo info;
		info.m_boundingBox = &m_boundingBox;

		if ( IsCameraTransformComponentWithoutRotation() || IsCameraTransformComponentWithRotation() )
		{
			// Ok, this logic is strange and kinda hacky: if there is any attachment, it should be special kind of attachment, like
			// sky transform, moon or sun or sth and it's transform is already local one to the camera - so matrix calculated like 
			// transform local * this local is ok; if there is no local transform, we store local transform matrix in local to world (HACK :( )
			// not to use extra memory
			if ( !m_transformParent )
			{
				CalcLocalTransformMatrix( m_localToWorld );
			}
		}
		info.m_localToWorld = &m_localToWorld;

		if ( context )
		{
			context->m_skinningContext.AddCommand_Relink( m_renderProxy, info );
		}
		else
		{
			( new CRenderCommand_RelinkProxy( m_renderProxy, info ) )->Commit();
		}
	}
}

void CDrawableComponent::OnParentAttachmentAdded( IAttachment* attachment )
{
	TBaseClass::OnParentAttachmentAdded( attachment );

	RefreshEntityGroupBinding();
}

void CDrawableComponent::OnParentAttachmentBroken( IAttachment* attachment )
{
	TBaseClass::OnParentAttachmentBroken( attachment );

	RefreshEntityGroupBinding();
}

void CDrawableComponent::OnUpdateTransformComponent( SUpdateTransformContext& context, const Matrix& prevLocalToWorld )
{
	PC_SCOPE( CDrawableComponent );

	// Update transform
	TBaseClass::OnUpdateTransformComponent( context, prevLocalToWorld );

	m_drawableFlags &= ~DF_MissedUpdateTransform;

#ifndef NO_EDITOR
	if( m_drawableFlags & DF_ScheduledRenderingResourceChanged )
	{
		OnRenderingResourceChanged();
		m_drawableFlags &= ~DF_ScheduledRenderingResourceChanged;
	}
#endif
	RelinkProxy( &context );
}

void CDrawableComponent::OnInitializeProxy()
{
	// Create default from this component
	RenderProxyInitInfo info;
	info.m_component = this;

	// For visibility testing assigned visibility query to the created proxy ( can be NULL )
	info.m_visibilityQuery = GetEntity()->GetVisibilityQuery();

	// Set the initial entity group of the entity we are created from
	CEntity* masterEntity = GetMasterEntity( this );
	if ( NULL != masterEntity )
	{
		info.m_entityGroup = masterEntity->GetRenderEntityGroup();
	}

	// Vertex collapse code
	info.m_usesVertexCollapse = UsesVertexCollapsed();
	
	// Create proxy
	m_renderProxy = GRender->CreateProxy( info );
	
	// Handle color variant cases
	OnColorVariantChanged();

	// Refresh internal engine coloring
	RefreshEngineColoring();
}

void CDrawableComponent::OnSelectionChanged()
{
	TBaseClass::OnSelectionChanged();

	// Send command to render thread
	if ( m_renderProxy )
	{
		const Bool isSelected = IsSelected();
		( new CRenderCommand_SetSelectionFlag( m_renderProxy, isSelected ) )->Commit();
	}
}

void CDrawableComponent::OnGenerateEditorHitProxies( CHitProxyMap& map )
{
	TBaseClass::OnGenerateEditorHitProxies( map );

#ifndef NO_COMPONENT_GRAPH
	// Send command to render thread
	if ( m_renderProxy )
	{
		const CHitProxyID& id = GetHitProxyID();
		( new CRenderCommand_UpdateHitProxyID( m_renderProxy, id ) )->Commit();
	}
#endif
}

void CDrawableComponent::OnColorVariantChanged()
{
}

void CDrawableComponent::SetVisible( Bool visible )
{
	// Change visibility type
	if ( visible != IsVisible() )
	{
		// Change visibility flag
		if ( visible )
		{
			m_drawableFlags |= DF_IsVisible;
		}
		else
		{
			m_drawableFlags &= ~DF_IsVisible;
		}

		// Full proxy refresh
		RefreshRenderProxies();
	}
}

void CDrawableComponent::SetForceNoAutohide( Bool forceNoAutohide )
{
	// Change visibility type
	if ( forceNoAutohide != IsForceNoAutohide() )
	{
		// Change visibility flag
		if ( forceNoAutohide )
		{
			m_drawableFlags |= DF_ForceNoAutohide;
		}
		else
		{
			m_drawableFlags &= ~DF_ForceNoAutohide;
		}

		// Full proxy refresh
		RefreshRenderProxies();
	}
}

void CDrawableComponent::ToggleVisibility( Bool visible )
{
	// Pass to base class
	TBaseClass::ToggleVisibility( visible );

	if ( IsVisible() )
	{
		SetProxiesVisible( visible );
	}
}

#ifndef NO_EDITOR
Bool CDrawableComponent::RemoveOnCookedBuild()
{
	return UseWithSimplygonOnly();
}
#endif

void CDrawableComponent::SetCastingShadows( Bool enable, Bool enableForLocalLightsOnly /* = false */ )
{
	Bool refreshNeeded = false;

	// Change flag
	if ( enable != IsCastingShadows()  )
	{
		// Change
		if ( enable )
		{
			m_drawableFlags |= DF_CastShadows;			
		}
		else
		{
			m_drawableFlags &= ~DF_CastShadows;			
		}
		refreshNeeded = true;
	}

	if ( enableForLocalLightsOnly != IsCastingShadowsFromLocalLightsOnly() || refreshNeeded )
	{
		// Change
		if ( enableForLocalLightsOnly )
		{
			m_drawableFlags |= DF_CastShadowsFromLocalLightsOnly;
			m_drawableFlags &= ~DF_CastShadows;			
		}
		else
		{
			m_drawableFlags &= ~DF_CastShadowsFromLocalLightsOnly;
			m_drawableFlags |= DF_CastShadows;
		}

		refreshNeeded = true;
	}

	// Full proxy refresh
	if( refreshNeeded ) RefreshRenderProxies();	
}

void CDrawableComponent::SetLocalWindSimulation( Bool enable )
{
	if ( enable != IsLocalWindSimulation() )
	{
		// Change
		if ( enable )
		{
			m_drawableFlags |= DF_LocalWindSimulation;
		}
		else
		{
			m_drawableFlags &= ~DF_LocalWindSimulation;
		}

		// Full proxy refresh
		RefreshRenderProxies();
	}
}

void CDrawableComponent::SetUsesVertexCollapsed( bool collapsed )
{
	if ( collapsed != UsesVertexCollapsed() )
	{
		// Change
		if ( collapsed )
		{
			m_drawableFlags |= DF_Collapsed;
		}
		else
		{
			m_drawableFlags &= ~DF_Collapsed;
		}

		// Full proxy refresh
		RefreshRenderProxies();
	}
}

void CDrawableComponent::SetNoDisolves( Bool noDisolves )
{
	const Bool isEnabled = ( 0 != ( m_drawableFlags & DF_NoDissolves ) );
	if ( noDisolves != isEnabled )
	{
		// Change
		if ( noDisolves )
		{
			m_drawableFlags |= DF_NoDissolves;
		}
		else
		{
			m_drawableFlags &= ~DF_NoDissolves;
		}

		// Full proxy refresh
		RefreshRenderProxies();
	}
}

void CDrawableComponent::SetTwoSided( Bool twoSided )
{
	const Bool isAlreadyTwoSided = IsTwoSided();
	if ( twoSided != isAlreadyTwoSided )
	{
		if ( twoSided )
		{
			m_drawableFlags |= DF_ForceTwoSided;
		}
		else
		{
			m_drawableFlags &= ~DF_ForceTwoSided;
		}

		// Refresh proxies
		RefreshRenderProxies();
	}
}

void CDrawableComponent::SetForcedHighestLOD( Bool allowed )
{
	const Bool isAlreadyAllowed = IsForcedHighestLOD();
	if ( allowed != isAlreadyAllowed )
	{
		if ( allowed )
		{
			m_drawableFlags |= DF_ForceHighestLOD;
		}
		else
		{
			m_drawableFlags &= ~DF_ForceHighestLOD;
		}

		// Refresh proxies
		RefreshRenderProxies();
	}
}

void CDrawableComponent::SetDrawableFlags( Uint32 flags )
{
	if ( m_drawableFlags != flags )
	{
		m_drawableFlags = flags;

		//Refresh render proxies
		RefreshRenderProxies();
	}
}

void CDrawableComponent::SetLightChannels( Uint8 channels )
{
	if ( channels != m_lightChannels )
	{
		m_lightChannels = channels;
		if (m_renderProxy != nullptr)
		{
			( new CRenderCommand_SetProxyLightChannels( m_renderProxy, m_lightChannels, (Uint8)(~(LC_DynamicObject | LC_ForwardShaded)) ) )->Commit();
		}
	}
}

void CDrawableComponent::EnableLightChannels( Bool enable, Uint8 channels )
{
	if ( enable )
	{
		SetLightChannels( m_lightChannels | channels );
	}
	else
	{
		SetLightChannels( m_lightChannels & ~channels );
	}
}

void CDrawableComponent::EnableDrawableFlags( Bool enable, Uint32 flags )
{
	if( enable == true )
	{
		SetDrawableFlags( m_drawableFlags | flags );
	}
	else
	{
		SetDrawableFlags( m_drawableFlags & ~flags );
	}
}

Bool CDrawableComponent::AreLightChannelsEnabled( Uint8 channels )
{
	return ( m_lightChannels & channels ) != 0;
}

void CDrawableComponent::RefreshEngineColoring()
{
	IMeshColoringScheme* meshColoringScheme = GEngine->GetMeshColoringScheme();
	if ( meshColoringScheme && m_renderProxy )
	{
		m_renderProxy->SetSelectionColor( meshColoringScheme->GetMeshSelectionColor( this ) );
	}
}

void CDrawableComponent::ConditionalAttachToRenderScene( CWorld* world )
{
	CEntity* entity = GetEntity();

	// Should add to scene?
	const Bool shouldAdd = CanAttachToRenderScene();

	// Did render proxy already exist?
	const Bool renderProxyExisted = m_renderProxy != NULL;

	// Should perform shit on dissolves?
	const Bool shouldPerformProxyAttachesOnDissolve = ShouldPerformProxyAttachesOnDissolve();

	// Destroy current proxy
	if ( m_renderProxy )
	{
		// Detach if attached
		if ( world->GetRenderSceneEx() )
		{
			if ( !shouldAdd && shouldPerformProxyAttachesOnDissolve )
			{
				( new CRenderCommand_SetAutoFade( world->GetRenderSceneEx(), m_renderProxy, FT_FadeOutAndDestroy ) )->Commit();
			}
			else
			{
				// Detach proxy from rendering scene
				( new CRenderCommand_RemoveProxyFromScene( world->GetRenderSceneEx(), m_renderProxy ) )->Commit();
			}
		}

		// Inform entity that rendering proxy has been detached
		entity->OnProxyDetached( this );

		// Free proxy
		m_renderProxy->Release();
		m_renderProxy = NULL;
	}

	// Add to scene
	if ( shouldAdd )
	{
		// Create proxy, let each drawable create one
		OnInitializeProxy();

		// Release render proxy object
		if ( m_renderProxy )
		{
			// Attach to scene
			( new CRenderCommand_AddProxyToScene( world->GetRenderSceneEx(), m_renderProxy ) )->Commit();

			// If proxy didn't exist, fade in if needed
			// NOTE: this will only work for proxies that are in the scene so add it first :)
			if ( !renderProxyExisted && shouldPerformProxyAttachesOnDissolve )
			{
				( new CRenderCommand_SetAutoFade( world->GetRenderSceneEx(), m_renderProxy, FT_FadeInStart ) )->Commit();
			}

			// Inform entity that rendering proxy has been attached
			entity->OnProxyAttached( this, m_renderProxy );

			if ( MissedUpdateTransform() )
			{
				ForceUpdateTransformNodeAndCommitChanges();
			}
		}
	}
}

//dex++: is this a dynamic geometry?
Bool CDrawableComponent::IsDynamicGeometryComponent() const
{
	// by default proxy is dynamic if has the flag set
	return 0 != (m_drawableFlags & DF_DynamicGeometry);
}
//dex--

Bool CDrawableComponent::ShouldPerformProxyAttachesOnDissolve() const
{
	if ( m_drawableFlags & DF_NoDissolves )
	{
		return false; 
	}

	if ( GetEntity()->CheckDynamicFlag( EDF_DisableAllDissolves ) )
	{
		return false;
	}

	if ( GGame->IsBlackscreen() || GGame->IsLoadingScreenShown() )
	{
		return false; 
	}

	// If we're destroying and weren't visible, don't need to dissolve. If we're adding, we don't care about the entity's
	// visibility, we always want to fade.
	const ERenderVisibilityResult visibilityResult = GetEntity()->GetLastFrameVisibility();
	if ( visibilityResult == RVR_NotVisible && m_renderProxy != nullptr )
	{
		return false;
	}

	if ( GRender->IsDeviceLost() )
	{
		return false;
	}

	return true;
}


Bool CDrawableComponent::CanAttachToRenderScene() const
{
	// Attach is used in appearance and visible
	Bool shouldCreateProxy = 
		// Local visibility flag should be true
		( IsVisible() || ( ( IsCastingShadows() || IsCastingShadowsFromLocalLightsOnly() ) && IsCastingShadowsEvenIfNotVisible() ) )
		// We must be attached to the world
		&& IsAttached()
		// All parent objects should also be marked as visible in game
		&& !IsHiddenInGame()
		// If device is lost, do not perform attaches
		&& !GRender->IsDeviceLost();

	// Return accumulated flag
	return shouldCreateProxy;
}

void CDrawableComponent::OnRenderingResourceChanged()
{
	RefreshRenderProxies();
}

void CDrawableComponent::OnRefreshVisibilityFlag()
{
	TBaseClass::OnRefreshVisibilityFlag();
	RefreshRenderProxies();
}

void CDrawableComponent::OnVisibilityForced()
{
	if ( IsAttached() && GetLayer()->IsAttached() )
	{
		CEntity* entity = GetEntity();
		CWorld* attachedWorld = GetLayer()->GetWorld();
		
		// Should add to scene?
		const Bool shouldAdd = CanAttachToRenderScene();

		if ( shouldAdd == false && m_renderProxy != NULL )
		{
			// Destroy current proxy
			if ( attachedWorld->GetRenderSceneEx() )
			{
				// Detach proxy from rendering scene
				( new CRenderCommand_RemoveProxyFromScene( attachedWorld->GetRenderSceneEx(), m_renderProxy ) )->Commit();
			}
			
			// Inform entity that rendering proxy has been detached
			entity->OnProxyDetached( this );

			// Free proxy
			m_renderProxy->Release();
			m_renderProxy = NULL;
		}
		else if ( shouldAdd == true && m_renderProxy == NULL )
		{
			// Add to scene
			// Create proxy, let each drawable create one
			OnInitializeProxy();

			if ( m_renderProxy )
			{
				// Attach to scene
				( new CRenderCommand_AddProxyToScene( attachedWorld->GetRenderSceneEx(), m_renderProxy ) )->Commit();

				// Inform entity that rendering proxy has been attached
				entity->OnProxyAttached( this, m_renderProxy );
			}
		}

		if( m_renderProxy && m_drawableFlags & DF_MissedUpdateTransform )
		{
			ScheduleUpdateTransformNode();
			m_drawableFlags &= ~DF_MissedUpdateTransform;
		}
	}
}

void CDrawableComponent::RecreateProxiesOfRenderableComponents()
{
	// Can't use only ObjectIterator here, because recreating some render proxies may require temporary CObjects to be
	// instantiated, which can cause a freeze.
	{
		TDynArray< CDrawableComponent* > drawableComponents;
		drawableComponents.Reserve( 200000 );

		for ( ObjectIterator< CDrawableComponent > it; it; ++it )
		{
			drawableComponents.PushBack( *it );
		}

		for ( CDrawableComponent* dc : drawableComponents )
		{
			dc->OnRenderingResourceChanged();
		}
	}

	for ( ObjectIterator<CLightComponent> it; it; ++it )
	{
		(*it)->RefreshInRenderSceneIfAttached();
	}
#ifndef NO_EDITOR
	for ( ObjectIterator<CClipMap> it; it; ++it )
	{
		(*it)->ClearTerrainProxy();
	}

	for ( ObjectIterator<CWorld> it; it; ++it )
	{
		(*it)->RefreshSceneSkyboxParams();
		(*it)->RefreshLensFlareParams();
	}
#endif
}

void CDrawableComponent::RecreateProxiesOfFurComponents()
{
	// Can't use only ObjectIterator here, because recreating some render proxies may require temporary CObjects to be
	// instantiated, which can cause a freeze. We only recreate fur proxies as settings don't affect other proxies.
	{
		TDynArray< CFurComponent* > furComponents;
		furComponents.Reserve( 50 );

		for ( ObjectIterator< CFurComponent > it; it; ++it )
		{
			furComponents.PushBack( *it );
		}

		for ( CFurComponent* fc : furComponents )
		{
			fc->OnRenderingResourceChanged();
		}
	}
}

void CDrawableComponent::RenderingSelectionColorChangedInEditor()
{
	for ( ObjectIterator<CDrawableComponent> it; it; ++it )
	{
		(*it)->RefreshEngineColoring();
	}
}

IRenderProxyInterface* CDrawableComponent::QueryRenderProxyInterface()
{
	return static_cast< IRenderProxyInterface* >( this );
}

const IRenderProxyInterface* CDrawableComponent::QueryRenderProxyInterface() const
{
	return static_cast< const IRenderProxyInterface* >( this );
}

Uint32 CDrawableComponent::GetNumberOfRenderProxies() const
{
	return m_renderProxy ? 1 : 0;
}

IRenderProxy* CDrawableComponent::GetRenderProxy( Uint32 i )
{
	ASSERT( i == 0 );
	return m_renderProxy;
}

void CDrawableComponent::SetProxiesVisible( Bool flag )
{
	if ( m_renderProxy )
	{
		m_renderProxy->SetVisible( flag );
	}
}

void CDrawableComponent::RefreshRenderProxies()
{
	// Reattach
	if ( IsAttached() && GetLayer()->IsAttached() )
	{
		CWorld* attachedWorld = GetLayer()->GetWorld();
		ConditionalAttachToRenderScene( attachedWorld );
	}
	else
	{
		ASSERT( !m_renderProxy );
	}
}

#ifndef NO_RESOURCE_USAGE_INFO

RED_DEFINE_NAME( CastingShadows );
RED_DEFINE_NAME( NoAutoHide );

void CDrawableComponent::CollectResourceUsage( class IResourceUsageCollector& collector, const Bool isStremable ) const
{
	TBaseClass::CollectResourceUsage( collector, isStremable );

	if ( IsCastingShadows() )
		collector.ReportComponentFlag( CNAME( CastingShadows ), true );

	if ( IsForceNoAutohide() )
		collector.ReportComponentFlag( CNAME( NoAutoHide ), true );
}

#endif // NO_RESOURCE_USAGE_INFO

void CDrawableComponent::funcIsVisible( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_BOOL( IsVisible() );
}

void CDrawableComponent::funcSetVisible( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, flag, false );
	FINISH_PARAMETERS;
	SetVisible( flag );
}

void CDrawableComponent::funcSetCastingShadows( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, flag, false );
	FINISH_PARAMETERS;
	SetCastingShadows( flag );
}

void CDrawableComponent::funcEnableLightChannels( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, enable, false );
	GET_PARAMETER( Uint32, channels, 0 );
	FINISH_PARAMETERS;
	EnableLightChannels( enable, static_cast< Uint8 >( channels ) );
	RETURN_VOID();
}

void CDrawableComponent::funcAreLightChannelsEnabled( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Uint32, channels, 0 );
	FINISH_PARAMETERS;
	RETURN_BOOL( AreLightChannelsEnabled( static_cast< Uint8 >( channels ) ) );
}
