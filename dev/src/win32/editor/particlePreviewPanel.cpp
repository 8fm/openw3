/**
 * Copyright © 2008 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"
#include "particleEditor.h"
#include "particlePreviewPanel.h"
#include "editorExternalResources.h"
#include "viewportWidgetMoveAxis.h"
#include "../../common/engine/renderCommands.h"
#include "../../common/core/depot.h"
#include "../../common/engine/particleSystem.h"
#include "../../common/engine/particleEmitter.h"
#include "../../common/engine/viewport.h"
#include "../../common/engine/dynamicLayer.h"
#include "../../common/engine/material.h"
#include "../../common/engine/materialDefinition.h"
#include "../../common/engine/worldTick.h"
#include "../../common/engine/fonts.h"

#define ID_MENU_ENTITY 100

BEGIN_EVENT_TABLE( CEdParticlePreviewPanel, CEdPreviewPanel )
	EVT_MENU( ID_MENU_ENTITY, CEdParticlePreviewPanel::OnLoadEntity )
END_EVENT_TABLE()

CEdParticlePreviewPanel::CEdParticlePreviewPanel( wxWindow *parent, CEdParticleEditor* particleEditor )
	: CEdPreviewPanel( parent, true )
	, m_particleEditor( particleEditor )
	, m_particleComponent( NULL )
	, m_entity( NULL )
	, m_targetEntity( NULL )
	, m_timeMultiplier( 1.0f )
	, m_currentAveragedTime( 0.0f )
	, m_averagedTime( 0.0f )
	, m_averagedCount( 0 )
	, m_showHelpers( false )
{
	// Set default camera orientation and load default template
	SetCameraPosition( Vector( 2, 2, 1 ) );
	SetCameraRotation( EulerAngles( 0, -10, 135 ) );
	GetViewport()->SetRenderingMode( RM_Shaded );

	// Load particle entity
	LoadEntity( ENTITY_PARTICLES );
	SpawnTargetEntity();
	// tick world to make the newly loaded entities visible
	Tick( 0.01f );
	// Assign target
	m_particleComponent->SetTarget( m_targetEntity );

	// Init wx control
	m_timeMultiplierControl.Init( parent->GetParent(), TXT("time"), 0.0f, 4.0f, 1.0f, &m_timeMultiplier );

	// Register as event listener
	SEvents::GetInstance().RegisterListener( CNAME( SelectionChanged ), this );

	m_widgetManager->AddWidget( TXT("Move"), new CViewportWidgetMoveAxis( Vector::EX, Color::RED ) );
	m_widgetManager->AddWidget( TXT("Move"), new CViewportWidgetMoveAxis( Vector::EY, Color::GREEN ) );
	m_widgetManager->AddWidget( TXT("Move"), new CViewportWidgetMoveAxis( Vector::EZ, Color::BLUE ) );
	m_widgetManager->AddWidget( TXT("Move"), new CViewportWidgetMovePlane( Vector::EY, Vector::EZ, Color::LIGHT_RED ) );
	m_widgetManager->AddWidget( TXT("Move"), new CViewportWidgetMovePlane( Vector::EX, Vector::EZ, Color::LIGHT_GREEN ) );
	m_widgetManager->AddWidget( TXT("Move"), new CViewportWidgetMovePlane( Vector::EX, Vector::EY, Color::LIGHT_BLUE ) );

	GetSelectionManager()->Select( m_targetEntity );
	m_widgetManager->EnableWidgets( false );
}

CEdParticlePreviewPanel::~CEdParticlePreviewPanel()
{
	// Unregister from all events
	SEvents::GetInstance().UnregisterListener( this );
}

void CEdParticlePreviewPanel::Reload()
{
	ASSERT(m_particleComponent);

	m_particleComponent->SetParticleSystem( m_particleEditor->GetParticleSystem() );

	Tick( 0.01f );
}

void CEdParticlePreviewPanel::HandleContextMenu( Int32 x, Int32 y )
{
	wxMenu menu;
	menu.Append( ID_MENU_ENTITY, TXT("Load entity...") );
	PopupMenu( &menu, x, y );
}

void CEdParticlePreviewPanel::OnLoadEntity( wxCommandEvent& event )
{
	CEdFileDialog dlg;
	dlg.SetMultiselection( false );
	dlg.AddFormat( ResourceExtension< CEntityTemplate >(), TXT( "Entity files" ) );

	String rootPath;
	GDepot->GetAbsolutePath( rootPath );
	dlg.SetDirectory( rootPath );

	if ( dlg.DoOpen( (HWND)GetHandle() ) )
	{				
		String localDepotPath;
		if ( !GDepot->ConvertToLocalPath( dlg.GetFile(), localDepotPath ) )
		{
			WARN_EDITOR( TXT("Couldn't convert '%s' to local depot path!"), dlg.GetFile().AsChar() );
		}
		else
		{
			LoadEntity( localDepotPath );
		}
	}
}

void CEdParticlePreviewPanel::LoadEntity( const String &filename )
{
	EntitySpawnInfo einfo;
	einfo.m_name = TXT("PreviewEntity");
	einfo.m_template = LoadResource< CEntityTemplate>( filename );
	m_entity = GetPreviewWorld()->GetDynamicLayer()->CreateEntitySync( einfo );

	if ( m_entity )
	{
		TDynArray< CComponent* > components;
		CollectEntityComponents( m_entity, components );

		m_particleComponent = NULL;
		for( Uint32 i = 0; i < components.Size(); ++i )
		{
			if ( components[i]->IsA( CParticleComponent::GetStaticClass() ) )
			{
				ASSERT( !m_particleComponent && "More then one particle component in %s, using first one only!", filename.AsChar() );
				m_particleComponent = SafeCast< CParticleComponent >( components[i] );
			}
		}

		if ( m_particleComponent )
		{
			m_particleComponent->SetParticleSystem( m_particleEditor->GetParticleSystem() );
		}
	}
	else
	{
		WARN_EDITOR( TXT("Failed to load %s entity"), filename.AsChar() );
	}	
}

void CEdParticlePreviewPanel::SpawnTargetEntity()
{
	EntitySpawnInfo einfo;
	einfo.m_name = TXT("TargetEntity");
	m_targetEntity = GetPreviewWorld()->GetDynamicLayer()->CreateEntitySync( einfo );
	m_targetEntity->SetPosition( Vector( 0.0f, 0.0f, 3.0f ) );
	m_targetEntity->ScheduleUpdateTransformNode();
}

void CEdParticlePreviewPanel::OnViewportTick( IViewport* view, Float timeDelta )
{
	// Tick world
	Tick( timeDelta );

	// Tick panel, we don't scale delta here, because we want fly speed not changed
	CEdRenderingPanel::OnViewportTick( view, timeDelta );
}

void CEdParticlePreviewPanel::OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
{
	if ( m_particleComponent )
	{
		// Generate preview fragments
		CEdPreviewPanel::OnViewportGenerateFragments( view, frame );

		// Average time
		m_averagedTime += static_cast< Float >( m_particleComponent->GetLastSimulationTime() );
		m_averagedCount += 1;

		// Reset once a while
		if ( m_averagedCount > 10 )
		{
			m_currentAveragedTime = m_averagedTime / (Float) m_averagedCount;
			m_averagedTime = 0.0f;
			m_averagedCount = 0;
		}

		const Uint32 lineHeight = 1.2f*m_font->GetLineDist();
		Uint32 x = 20;
		Uint32 y = 20;

		// Simulation time
		frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("Tick %1.1fus"), m_currentAveragedTime * 1000000.0f );
		y += lineHeight;

		// Distance from camera to particles
		const Float distance = m_particleComponent->GetPosition().DistanceTo( frame->GetFrameInfo().m_camera.GetPosition() );
		frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("Distance: %1.2fm"), distance );
		y += lineHeight;


		if ( m_showHelpers )
		{
			frame->AddDebugSphere( m_targetEntity->GetWorldPosition(), 0.1f, Matrix::IDENTITY, Color( 0,0,255,255 ) );
		}

		//// Get particle instance data
		//for ( TParticleInstanceMap::const_iterator it=data.Begin(); it!=data.End(); ++it )
		//{
		//	CParticleEmitter* emitter = (*it).m_first;

		//	if ( emitter->IsEnabled() )
		//	{
		//		IParticleData* instanceData = (*it).m_second;
		//		if ( instanceData )
		//		{
		//			IParticleBuffer* bufferData = instanceData->GetParticleBufferInterface();
		//			if ( bufferData )
		//			{
		//				const Int32 current = bufferData->GetNumParticles();
		//				const Int32 max = bufferData->GetMaxParticles();
		//				const Color color = emitter->GetEditorColor();
		//				frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("%i/%i"), current, max );
		//				frame->AddDebugScreenFormatedText( x+50, y, color, TXT("%s"), emitter->GetEditorName().AsChar() );
		//				y += 15;
		//			}
		//		}
		//	}
		//}

		/*// Show time multiplier
		String textToShow = String::Printf( TXT("Time (x %.2f): %.2f"), m_timeMultiplier, m_particleComponent->GetLocalTime() );
		m_font->Print( frame, (Int32)m_font->GetLineDist(), (Int32)m_font->GetLineDist(), 0.5f, textToShow );

		// Emitters particle count
		TDynArray< Int32 > particleCount = m_particleComponent->GetParticleCount();
		TDynArray< CParticleEmitter* > emitters = m_particleComponent->GetParticleSystem()->GetEmitters();
		for ( Uint32 i = 0; i < emitters.Size(); ++i )
		{
			Uint32 size = emitters[i]->GetParticleMaxCount()*emitters[i]->GetParticleDescription().GetParticleSize();
			String textToShow = String::Printf( TXT("%s: %d (%d bytes)"), emitters[i]->GetName().AsChar(), particleCount[i], size );
			m_font->Print( frame, (Int32)m_font->GetLineDist(), (i+2)*(Int32)m_font->GetLineDist(), 0.5f, textToShow );
		}*/
	}
}

void CEdParticlePreviewPanel::Tick( Float timeDelta )
{
	CWorldTickInfo info( m_previewWorld, timeDelta );
	info.m_updatePhysics = true;
	m_previewWorld->Tick( info );
}

Bool CEdParticlePreviewPanel::ShouldDrawGrid() const
{
	// Use emitter grid setting
	if ( m_particleComponent && m_particleComponent->GetParticleSystem() )
	{
		CParticleSystem* system = m_particleComponent->GetParticleSystem();
		return system->m_previewShowGrid;
	}

	// Fallback
	return CEdPreviewPanel::ShouldDrawGrid();
}

Color CEdParticlePreviewPanel::GetClearColor() const
{
	// Use emitter preview color
	if ( m_particleComponent && m_particleComponent->GetParticleSystem() )
	{
		CParticleSystem* system = m_particleComponent->GetParticleSystem();
		return system->m_previewBackgroundColor;
	}

	// Fallback
	return CEdPreviewPanel::GetClearColor();
}

void CEdParticlePreviewPanel::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	// Selection has changed
	if ( name == CNAME( SelectionChanged ) )
	{
		typedef CSelectionManager::SSelectionEventData SEventData;
		const SEventData& eventData = GetEventData< SEventData >( data );
		if ( eventData.m_world == m_previewWorld )
		{
			TDynArray< CNode* > nodes;
			GetSelectionManager()->GetSelectedNodes( nodes );
			m_widgetManager->EnableWidgets( nodes.Size() > 0 );
		}
	}
}

void CEdParticlePreviewPanel::SetTargetVisible( Bool flag )
{
	m_widgetManager->EnableWidgets( flag );
	m_showHelpers = flag;
}

void GatherAllExistingParticleComponentsWithParticleSystem( TDynArray< CParticleComponent* >& matchingComponents, const CParticleSystem* ps )
{
	matchingComponents.Clear();

	for ( ObjectIterator<CParticleComponent> it; it; ++it )
	{
		CParticleComponent* pc = (*it);
		if ( pc->GetParticleSystem() == ps )
		{
			matchingComponents.PushBack( pc );
		}
	}
}

void CEdParticlePreviewPanel::OnUpdateEmitter( CParticleEmitter* emitter )
{
	ASSERT( m_particleComponent );
	ASSERT( emitter );

	CParticleSystem* ps = emitter->FindParent< CParticleSystem >();
	ASSERT( ps );
	ASSERT( ps == m_particleComponent->GetParticleSystem() );

	// Collect all components using the same particle system. This will update any particle components, regardless of window, world, etc., not just the one in preview.
	TDynArray< CParticleComponent* > components;
	GatherAllExistingParticleComponentsWithParticleSystem( components, ps );
		
	if ( !emitter->IsEnabled() )
	{
		// Render element with corresponding render emitter will be removed from particles render proxy, nothing happens if removed already
		for ( Uint32 i=0; i<components.Size(); ++i )
		{
			if ( components[i]->GetRenderProxy() )
			{
				( new CRenderCommand_RemoveParticleEmitter( components[i]->GetRenderProxy(), emitter->GetUniqueId() ) )->Commit();
			}
		}
	}
	else
	{
		IMaterial* material = emitter->GetMaterial();
		IRenderResource* renderMaterialDefinition = NULL;
		IRenderResource* renderMaterialParameters = NULL;
		IRenderResource* renderEmitter = NULL;
		if ( material )
		{
			IMaterialDefinition* materialDefinition = material->GetMaterialDefinition();
			if ( materialDefinition )
			{
				// Rebuild render resource
				emitter->CreateRenderResource();
				renderEmitter = emitter->GetRenderResource();
				renderMaterialDefinition = materialDefinition->GetRenderResource();
				renderMaterialParameters = material->GetRenderResource();
			}
		}

		if ( renderEmitter && renderMaterialDefinition && renderMaterialParameters )
		{
			for ( Uint32 i=0; i<components.Size(); ++i )
			{
				if ( components[i]->GetRenderProxy() )
				{
					( new CRenderCommand_UpdateCreateParticleEmitter( components[i]->GetRenderProxy(), renderEmitter, renderMaterialDefinition, renderMaterialParameters, emitter->GetEnvColorGroup() ) )->Commit();
				}
			}
		}
		else
		{
			for ( Uint32 i=0; i<components.Size(); ++i )
			{
				if ( components[i]->GetRenderProxy() )
				{
					( new CRenderCommand_RemoveParticleEmitter( components[i]->GetRenderProxy(), emitter->GetUniqueId() ) )->Commit();
				}
			}
		}
	}
}

void CEdParticlePreviewPanel::OnRemoveEmitter( CParticleEmitter * emitter )
{
	ASSERT( m_particleComponent );
	ASSERT( emitter );

	CParticleSystem* ps = emitter->FindParent< CParticleSystem >();
	ASSERT( ps );
	ASSERT( ps == m_particleComponent->GetParticleSystem() );

	// Collect all components using the same particle system. This will update any particle components, regardless of window, world, etc., not just the one in preview.
	TDynArray< CParticleComponent* > components;
	GatherAllExistingParticleComponentsWithParticleSystem( components, ps );

	for ( Uint32 i=0; i<components.Size(); ++i )
	{
		if ( components[i]->GetRenderProxy() )
		{
			( new CRenderCommand_RemoveParticleEmitter( components[i]->GetRenderProxy(), emitter->GetUniqueId() ) )->Commit();
		}
	}
}
