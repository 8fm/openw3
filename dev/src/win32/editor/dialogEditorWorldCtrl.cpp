
#include "build.h"
#include "dialogEditorWorldCtrl.h"
#include "dialogEditor.h"
#include "../../common/engine/renderCommands.h"
#include "../../common/engine/clipMap.h"
#include "../../common/core/feedback.h"
#include "../../common/engine/viewport.h"
#include "../../common/engine/tagManager.h"
#include "../../common/engine/staticMeshComponent.h"
#include "../../common/engine/renderProxy.h"
#include "../../common/engine/worldIterators.h"

#ifdef DEBUG_SCENES_2
#pragma optimize("",off)
#endif

CEdSceneWorldCtrl::CEdSceneWorldCtrl()
	: m_editor( nullptr )
	, m_previewWorld( nullptr )
	, m_mode( M_Preview )
	, m_prevHook( nullptr )
	, m_orgHook( nullptr )
{

}

void CEdSceneWorldCtrl::Init( CEdSceneEditor* editor, CWorld* previewWorld )
{
	ASSERT( !m_editor );
	m_editor = editor;
	m_previewWorld = previewWorld;
}

CWorld* CEdSceneWorldCtrl::GetWorld() const
{
	return m_mode == M_Preview ? m_previewWorld : GetGameWorld();
}

CWorld* CEdSceneWorldCtrl::GetGameWorld() const
{
	ASSERT( GGame );
	ASSERT( GGame->GetActiveWorld() );
	return GGame->GetActiveWorld();
}

void CEdSceneWorldCtrl::ToggleMode()
{
	if ( IsPreviewMode() )
	{
		SetGameMode();
	}
	else
	{
		SetPreviewMode();
	}
}

void CEdSceneWorldCtrl::SetPreviewMode()
{
	if ( m_mode != M_Preview )
	{
		m_editor->OnWorldCtrl_PreModeChanged();

		m_mode = M_Preview;

		if ( m_orgHook )
		{
			GGame->GetViewport()->SetViewportHook( m_orgHook );
		}
		else
		{
			GGame->GetViewport()->SetViewportHook( m_prevHook );
		}
		
		GGame->GetViewport()->RestoreSize();

		m_orgHook = nullptr;
		m_prevHook = nullptr;

		m_editor->OnWorldCtrl_PostModeChanged();
	}
}

void CEdSceneWorldCtrl::SetGameMode()
{
	if ( m_mode != M_Game )
	{
		m_editor->OnWorldCtrl_PreModeChanged();

		m_mode = M_Game;

		ASSERT( !m_prevHook );
		m_orgHook = nullptr;
		m_prevHook = GGame->GetViewport()->GetViewportHook();

		GGame->GetViewport()->SetViewportHook( this );
		GGame->GetViewport()->AdjustSizeWithCachets( wxTheFrame->GetWorldEditPanel()->GetViewportCachetAspectRatio() );

		m_editor->OnWorldCtrl_PostModeChanged();
	}
}

void CEdSceneWorldCtrl::SetGameplayMode()
{
	if ( m_mode != M_Gameplay )
	{
		m_editor->OnWorldCtrl_PreModeChanged();

		m_mode = M_Gameplay;

		ASSERT( !m_prevHook );
		m_orgHook = GGame->GetViewport()->GetViewportHook();
		m_prevHook = wxTheFrame->GetWorldEditPanel();

		GGame->GetViewport()->SetViewportHook( this );
		GGame->GetViewport()->AdjustSizeWithCachets( wxTheFrame->GetWorldEditPanel()->GetViewportCachetAspectRatio() );

		m_editor->OnWorldCtrl_PostModeChanged();
	}
}

Bool CEdSceneWorldCtrl::IsPreviewMode() const
{
	return m_mode == M_Preview;
}

Bool CEdSceneWorldCtrl::IsGameMode() const
{
	return m_mode == M_Game;
}

Bool CEdSceneWorldCtrl::IsGameplayMode() const
{
	return m_mode == M_Gameplay;
}

Bool CEdSceneWorldCtrl::CanUseGameMode() const
{
	return GGame && GGame->GetActiveWorld() && GGame->GetViewport();
}

void CEdSceneWorldCtrl::OnViewportCalculateCamera( IViewport* view, CRenderCamera &camera )
{
	if ( !m_editor->OnWorldCtrl_CalculateCamera( view, camera ) )
	{
		m_prevHook->OnViewportCalculateCamera( view, camera );
	}
}

void CEdSceneWorldCtrl::OnViewportTick( IViewport* view, Float timeDelta )
{
	m_editor->OnWorldCtrl_ViewportTick( timeDelta );

	m_prevHook->OnViewportTick( view, timeDelta );
}

void CEdSceneWorldCtrl::OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
{
	m_editor->OnWorldCtrl_GenerateFragments( view, frame );

	m_prevHook->OnViewportGenerateFragments( view, frame );
}

Bool CEdSceneWorldCtrl::OnViewportTrack( const CMousePacket& packet )					
{
	m_editor->OnWorldCtrl_CameraMoved(); 
	return m_prevHook->OnViewportTrack( packet ); 
}

Bool CEdSceneWorldCtrl::OnViewportMouseMove( const CMousePacket& packet )				
{ 
	m_editor->OnWorldCtrl_CameraMoved(); 
	return m_prevHook->OnViewportMouseMove( packet ); 
}

Bool CEdSceneWorldCtrl::OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )	
{ 
	if ( m_editor->OnWorldCtrl_ViewportInput( view, key, action, data ) )
	{
		return true;
	}
	return m_prevHook->OnViewportInput( view, key, action, data ); 
}

Bool CEdSceneWorldCtrl::OnViewportClick( IViewport* view, Int32 button, Bool state, Int32 x, Int32 y )					
{ 
	//m_editor->OnWorldCtrl_CameraMoved(); 
	return m_prevHook->OnViewportClick( view, button, state, x, y ); 
}

CRenderFrame* CEdSceneWorldCtrl::OnViewportCreateFrame( IViewport *view )				
{ 
	return m_prevHook->OnViewportCreateFrame( view ); 
}

void CEdSceneWorldCtrl::OnViewportRenderFrame( IViewport *view, CRenderFrame *frame )	
{ 
	m_prevHook->OnViewportRenderFrame( view, frame ); 
}

void CEdSceneWorldCtrl::OnViewportSetDimensions ( IViewport* view )					
{ 
	m_prevHook->OnViewportSetDimensions( view ); 
}

//////////////////////////////////////////////////////////////////////////

CEdSceneWorldProxyRelinker::CEdSceneWorldProxyRelinker()
	: m_isLinked( false )
	, m_terrainProxy( NULL )
{

}

CEdSceneWorldProxyRelinker::~CEdSceneWorldProxyRelinker()
{

}


void CEdSceneWorldProxyRelinker::Destroy( CWorld* world )
{
	if ( m_isLinked )
	{
		Unlink( world );
	}
}

void CEdSceneWorldProxyRelinker::Link( CWorld* world, const CStoryScene* scene )
{
	if ( GGame == NULL || GGame->GetActiveWorld() == NULL )
	{
		GFeedback->ShowMsg( TXT("Log"), TXT("Cannot find active world in main panel. Do you open any world?") );
		return;
	}

	ASSERT( !m_isLinked );
	if ( m_isLinked )
	{
		return;
	}

	m_isLinked = true;

	const TDynArray< CStorySceneDialogsetInstance* >& dialogsets = scene->GetDialogsetInstances();

	TDynArray< String > logs;

	for ( auto iter = dialogsets.Begin(); iter != dialogsets.End(); ++iter )
	{
		TDynArray< CNode* > taggedNodes;
		GGame->GetActiveWorld()->GetTagManager()->CollectTaggedNodes( (*iter)->GetPlacementTag() , taggedNodes, BCTO_MatchAll );

		if ( taggedNodes.Size() == 0 )
		{
			logs.PushBack( (*iter)->GetPlacementTag().ToString() );
		}

		for ( Int32 j=taggedNodes.SizeInt()-1; j>=0; --j )
		{
			world->GetTagManager()->AddNode( taggedNodes[j], (*iter)->GetPlacementTag() );
		}
	}

	if ( logs.Size() > 0 )
	{
		String msg;
		for ( Uint32 i=0; i<logs.Size(); ++i )
		{
			msg += logs[ i ];
			msg += TXT("\n");
		}
		GFeedback->ShowError( TXT("Cannot find\n [%s] \ntagged place(s)"), msg.AsChar() );
	}

	CClipMap* terra = GGame->GetActiveWorld()->GetTerrain();
	if( terra )
	{
		// little hacky way to display new terrain in preview without all of the logic
		m_terrainProxy = terra->GetTerrainProxy();
		if ( m_terrainProxy != NULL )
		{
			m_terrainProxy->AddRef();

			( new CRenderCommand_SetTerrainProxyToScene( world->GetRenderSceneEx(), m_terrainProxy ) )->Commit();
		}

		//CClipMap* terraClone = Cast<CClipMap>(terra->Clone( m_previewWorld ));
		//m_previewWorld->SetTerrain( terraClone );
	}

	for ( WorldAttachedComponentsIterator it( GGame->GetActiveWorld() ); it; ++it )
	{
		CComponent *component = *it;
		CDrawableComponent* drawableComponent = Cast< CDrawableComponent >( component );
		if ( drawableComponent == NULL )
		{
			continue;
		}

		RenderProxyInitInfo info;
		info.m_component = drawableComponent;

		// Create proxy
		IRenderProxy* componentRenderProxy = GRender->CreateProxy( info );

		if ( componentRenderProxy != NULL )
		{
			componentRenderProxy->AddRef();
			( new CRenderCommand_AddProxyToScene( world->GetRenderSceneEx(), componentRenderProxy ) )->Commit();
			m_renderProxies.PushBack( componentRenderProxy );
		}
	}
}

void CEdSceneWorldProxyRelinker::Unlink( CWorld* world )
{
	ASSERT( m_isLinked );

	if ( !m_isLinked )
	{
		return;
	}

	m_isLinked = false;

	if ( m_terrainProxy != NULL )
	{
		( new CRenderCommand_RemoveTerrainProxyFromScene( world->GetRenderSceneEx(), m_terrainProxy ) )->Commit();
		m_terrainProxy->Release();
		m_terrainProxy = NULL;
	}
	// Clear render proxies
	for ( TDynArray< IRenderProxy* >::iterator renderProxyIter = m_renderProxies.Begin(); renderProxyIter != m_renderProxies.End(); ++renderProxyIter )
	{
		( new CRenderCommand_RemoveProxyFromScene( world->GetRenderSceneEx(), *renderProxyIter ) )->Commit();
		(*renderProxyIter)->Release();
	}
	m_renderProxies.Clear();

	world->GetTagManager()->Clear();

	// Clear renderer collision filter
	if ( GGame->GetActiveWorld() != NULL )
	{
		for ( WorldAttachedComponentsIterator it( GGame->GetActiveWorld() ); it; ++it )
		{
			CComponent *component = *it;
			if ( component->IsA< CStaticMeshComponent >() == true )
			{
				world->GetEditorFragmentsFilter().UnregisterEditorFragment( component, SHOW_Collision );
			}
		}
	}

	if ( world->GetTerrain() )
	{
		world->GetTerrain()->ClearTerrainProxy();
		world->SetTerrain( NULL );
	}
}

Bool CEdSceneWorldProxyRelinker::ToggleLinkProcess( CWorld* world, const CStoryScene* scene )
{
	if ( m_isLinked )
	{
		Unlink( world );
		return false;
	}
	else
	{
		Link( world, scene );
		return true;
	}
}

Bool CEdSceneWorldProxyRelinker::IsLinked() const
{
	return m_isLinked;
}

#ifdef DEBUG_SCENES_2
#pragma optimize("",on)
#endif
