/**
 * Copyright © 2009 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "entityPreviewPanel.h"
#include "effectTracksEditor.h"
#include "effectEditor.h"
#include "effectProperties.h"
#include "effectBoneListSelection.h"
#include "propertiesPage.h"
#include "../../common/engine/worldIterators.h"

BEGIN_EVENT_TABLE( CEdEffectEditor, wxPanel )
	EVT_TOOL( XRCID("RestartTool"), CEdEffectEditor::OnRestartEffect )
	EVT_TOOL( XRCID("PlayTool"), CEdEffectEditor::OnPlayEffect )
	EVT_TOOL( XRCID("PauseTool"), CEdEffectEditor::OnPauseEffect )
	EVT_TOOL( XRCID("StopTool"), CEdEffectEditor::OnStopEffect )
	EVT_TOOL( XRCID("ForceStopTool"), CEdEffectEditor::OnForceStopEffect )
	EVT_UPDATE_UI( wxID_ANY, CEdEffectEditor::OnUpdateUI )
END_EVENT_TABLE()

CEdEffectEditor::CEdEffectEditor( wxWindow* parent, CEntity* entity, CEntityTemplate* templ, CFXDefinition* fxDefinition )
	: m_fxDefinition( fxDefinition )
	, m_curveEditor( nullptr )
	, m_propertyEditor( nullptr )
	, m_effectTracksEditor( nullptr )
	, m_template( templ )
	, m_entity( entity )
	, m_isPlaying( false )
{
	String animName = m_fxDefinition->GetAnimationName().AsString();
	
	wxXmlResource::Get()->LoadPanel( this, parent, TEXT("EffectPanel") );

	// Create Curve Editor and place it in CurvePanel
	{
		wxPanel* panel = XRCCTRL( *this, "CurvePanel", wxPanel );
		wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
		m_curveEditor = new CEdEffectCurveEditor( panel );
		sizer->Add( m_curveEditor, 1, wxEXPAND, 0 );
		panel->SetSizer( sizer );
		panel->Layout();
	}

	// Create Effect Tracks Editor and place it in TrackPanel
	{
		wxPanel* panel = XRCCTRL( *this, "TrackPanel", wxPanel );
		wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
		m_effectTracksEditor = new CEdEffectTracksEditor( panel, m_fxDefinition, m_curveEditor->GetSidePanelWidth(), m_curveEditor, this );
		sizer->Add( m_effectTracksEditor, 1, wxEXPAND, 0 );
		panel->SetSizer( sizer );
		panel->Layout();
	}

	// Create Properties Editor and place it in PropertyPanel
	{
		wxPanel* panel = XRCCTRL( *this, "PropertyPanel", wxPanel );
		wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
		PropertiesPageSettings settings;
		m_propertyEditor = new CEdEffectEditorProperties( panel, settings, nullptr, m_entity.Get() );
		m_propertyEditor->SetObject( m_fxDefinition );
		sizer->Add( m_propertyEditor, 1, wxEXPAND );
		panel->SetSizer( sizer );
		panel->Layout();
	}

	// Link synchronization between track editor and curve editor
	m_curveEditor->SetSynchEditor( m_effectTracksEditor );
	m_effectTracksEditor->SetSynchEditor( m_curveEditor );

	// Update and finalize layout
	Layout();
	Show();

	// Register event listener
	SEvents::GetInstance().RegisterListener( CNAME( EffectTicked ), this );
	SEvents::GetInstance().RegisterListener( CNAME( EffectDestroyed ), this );
	SEvents::GetInstance().RegisterListener( CNAME( EditorPropertyPostChange ), this );

	// Load configuration options
	LoadOptionsFromConfig();

	// Update bone editor
	CEffectBoneListSelection::SetEntity( entity );
}

CEdEffectEditor::~CEdEffectEditor()
{
	// Stop effect
    if ( m_entity.Get() )
    {
	    m_entity.Get()->DestroyAllEffects();
    }

	// Unregister listener
	SEvents::GetInstance().UnregisterListener( this );

	// Save configuration options
	SaveOptionsToConfig();

	// Update bone editor
	CEffectBoneListSelection::SetEntity( nullptr );
}

void CEdEffectEditor::SetObjectToEdit( CObject* object )
{
	m_propertyEditor->SetObject( object );
}

void CEdEffectEditor::RefreshProperties()
{
	m_propertyEditor->RefreshValues();
}

void CEdEffectEditor::SetAnimationLength( Float length )
{
	// Update animation bar
	m_effectTracksEditor->SetAnimationLength( length );
}

void CEdEffectEditor::ForceStopEffects()
{
	wxCommandEvent fakeEvent;
	OnForceStopEffect( fakeEvent );
}

void CEdEffectEditor::SetEntity( CEntity* entity )
{
	ForceStopEffects();
	m_entity = entity;
	CEffectBoneListSelection::SetEntity( entity );
}

void CEdEffectEditor::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	if ( name == CNAME( EffectTicked ) || name == CNAME( EffectDestroyed ) )
	{
		const SEditorEffectMessage& msg = GetEventData< SEditorEffectMessage >( data );
		if ( msg.m_entity == m_entity.Get() && msg.m_definition == m_fxDefinition )
		{
			if ( name == CNAME( EffectTicked ) )
			{
				const Float effectTime = msg.m_effectTime;
				m_effectTracksEditor->SetEffectTime( effectTime );
			}
		}
	}
	else if ( name == CNAME( EditorPropertyPostChange ) )
	{
		if ( m_effectTracksEditor )
		{
			m_effectTracksEditor->UpdateBars();
		}
	}
}

void CEdEffectEditor::GetPreviewEntities( TDynArray< CEntity* >& entities )
{
	// Use the given entity if we have one
	if ( m_entity.Get() )
	{
		entities.PushBack( m_entity.Get() );
	}

	// We should edit an entity with template to preview effect in the world
	if ( m_template.Get() )
	{
		// If 'preview on all entities' option is select play the effect on the entities in the world :)
		wxCheckBox* box = XRCCTRL( *this, "PreviewOnAllEntities", wxCheckBox );
		if ( box->IsChecked() )
		{
			// Collect entities from world
			CWorld* world = GGame->GetActiveWorld();
			if ( world )
			{
				for ( WorldAttachedEntitiesIterator it( world ); it; ++it )
				{
					CEntity* entity = *it;
					if ( entity && entity->GetEntityTemplate() )
					{
						if ( entity->GetEntityTemplate()->IsBasedOnTemplate( m_template.Get() ) )
						{
							entities.PushBack( entity );
						}
					}
				}
			}
		}
	}
}

void CEdEffectEditor::OnRestartEffect( wxCommandEvent& evnet )
{
	if ( m_fxDefinition )
	{
		// Get preview entities
		TDynArray< CEntity* > previewEntities;
		GetPreviewEntities( previewEntities );

		// Restart effect
		CName effectName = m_fxDefinition->GetName();
		for ( Uint32 i=0; i<previewEntities.Size(); i++ )
		{
			CEntity* entity = previewEntities[i];
			entity->DestroyAllEffects();
			entity->PlayEffectPreview( m_fxDefinition );
		}
	}
}

void CEdEffectEditor::OnPlayEffect( wxCommandEvent& evnet )
{
	if ( m_fxDefinition )
	{
		// Get preview entities
		TDynArray< CEntity* > previewEntities;
		GetPreviewEntities( previewEntities );

		// Resume effect
		CName effectName = m_fxDefinition->GetName();
		for ( Uint32 i=0; i<previewEntities.Size(); i++ )
		{
			CEntity* entity = previewEntities[i];
			if ( entity->PlayEffectPreview( m_fxDefinition ) )
			{
				m_isPlaying = true;
			}
		}
	}
}

void CEdEffectEditor::OnPauseEffect( wxCommandEvent& evnet )
{
	if ( m_fxDefinition )
	{
		// Get preview entities
		TDynArray< CEntity* > previewEntities;
		GetPreviewEntities( previewEntities );

		// Resume effect
		CName effectName = m_fxDefinition->GetName();
		for ( Uint32 i=0; i<previewEntities.Size(); i++ )
		{
			CEntity* entity = previewEntities[i];
			if ( entity->PauseEffect( m_fxDefinition, true ) )
			{
				m_isPlaying = false;
			}
		}
	}
}

void CEdEffectEditor::OnStopEffect( wxCommandEvent& evnet )
{
	if ( m_fxDefinition )
	{
		// Get preview entities
		TDynArray< CEntity* > previewEntities;
		GetPreviewEntities( previewEntities );

		// Resume effect
		CName effectName = m_fxDefinition->GetName();
		for ( Uint32 i=0; i<previewEntities.Size(); i++ )
		{
			CEntity* entity = previewEntities[i];
			entity->StopEffect( m_fxDefinition );
		}
	}
}

void CEdEffectEditor::OnForceStopEffect( wxCommandEvent& evnet )
{
	if ( m_fxDefinition )
	{
		// Get preview entities
		TDynArray< CEntity* > previewEntities;
		GetPreviewEntities( previewEntities );

		// Resume effect
		CName effectName = m_fxDefinition->GetName();
		for ( Uint32 i=0; i<previewEntities.Size(); i++ )
		{
			CEntity* entity = previewEntities[i];
			entity->DestroyAllEffects();
		}
	}
}

void CEdEffectEditor::OnUpdateUI( wxUpdateUIEvent& event )
{
	Bool isBoundToAnimation = m_fxDefinition->IsBoundToAnimation();
	Bool hasEntity = m_entity.Get() != nullptr;

	wxToolBar* toolbar = XRCCTRL( *this, "ToolBar", wxToolBar );

	if ( hasEntity && m_isPlaying && !m_entity.Get()->IsPlayingEffect( m_fxDefinition ) )
	{
		m_isPlaying = false; // the effect stopped playing in the meantime
	}

	toolbar->EnableTool( XRCID("RestartTool"),   !isBoundToAnimation && hasEntity );
	toolbar->EnableTool( XRCID("PlayTool"),      !isBoundToAnimation && hasEntity && !m_isPlaying );
	toolbar->EnableTool( XRCID("PauseTool"),     !isBoundToAnimation && hasEntity && m_isPlaying );
	toolbar->EnableTool( XRCID("StopTool"),      !isBoundToAnimation && hasEntity && m_isPlaying );
	toolbar->EnableTool( XRCID("ForceStopTool"), !isBoundToAnimation && hasEntity && m_isPlaying );
}

void CEdEffectEditor::SaveOptionsToConfig()
{
	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	CConfigurationScopedPathSetter pathSetter( config, TXT("/Frames/EffectEditor") );

	wxPoint offset = m_curveEditor->GetOffset();
	Vector scale = m_effectTracksEditor->GetScale();
	bool infinityCheckBox = false;//m_infinityCheckBox->GetValue();
	int loopCountSpinCtrl = 0;//m_loopCountSpinCtrl->GetValue();

	config.Write( TXT("OffsetX"), offset.x );
	config.Write( TXT("OffsetY"), offset.y );
	config.Write( TXT("ScaleX"), scale.X );
	config.Write( TXT("ScaleY"), scale.Y );
	config.Write( TXT("InfinityCheckBox"), (Int32)infinityCheckBox );
	config.Write( TXT("LoopCountSpinCtrl"), loopCountSpinCtrl );

	wxSplitterWindow* firstSplitter = XRCCTRL( *this, "firstSplitter", wxSplitterWindow );
	wxSplitterWindow* secondSplitter = XRCCTRL( *this, "secondSplitter", wxSplitterWindow );

	config.Write( TXT("firstSplitterSash"), firstSplitter->GetSashPosition() );
	config.Write( TXT("secondSplitterSash"), secondSplitter->GetSashPosition() );
}

void CEdEffectEditor::LoadOptionsFromConfig()
{
	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	CConfigurationScopedPathSetter pathSetter( config, TXT("/Frames/EffectEditor") );

	wxPoint offset( 0, 0 );
	Vector scale( 0, 0, 0 );
	bool infinityCheckBox = true;
	int loopCountSpinCtrl = 1;

	offset.x = config.Read( TXT("OffsetX"), 0 );
	offset.y = config.Read( TXT("OffsetY"), 0 );
	scale.X = config.Read( TXT("ScaleX"), 0.04f );
	scale.Y = config.Read( TXT("ScaleY"), 0.0f );
	infinityCheckBox = config.Read( TXT("InfinityCheckBox"), 1 ) != 0;
	loopCountSpinCtrl = config.Read( TXT("LoopCountSpinCtrl"), 1 );

	wxSplitterWindow* firstSplitter = XRCCTRL( *this, "firstSplitter", wxSplitterWindow );
	wxSplitterWindow* secondSplitter = XRCCTRL( *this, "secondSplitter", wxSplitterWindow );

	firstSplitter->SetSashPosition( config.Read( TXT("firstSplitterSash"), firstSplitter->GetSashPosition() ) );
	secondSplitter->SetSashPosition( config.Read( TXT("secondSplitterSash"), secondSplitter->GetSashPosition() ) );

	m_effectTracksEditor->Synchronize( offset, scale );
	m_curveEditor->Synchronize( offset, scale );
}

void CEdEffectEditor::CopySelection()
{
	m_effectTracksEditor->CopySelectedTrackItem();
}

void CEdEffectEditor::PasteSelection()
{
	m_effectTracksEditor->PasteCopiedTrackItem();
}

//////////////////////////////////////////////////////////////////////////

CEdEffectCurveEditor::CEdEffectCurveEditor( wxWindow* parent )
{
	// Load designed panel from resource
	wxXmlResource::Get()->LoadPanel( this, parent, TEXT("CurveEditorPanel") );

	// Set default states for buttons
	wxToolBar* tb = XRCCTRL( *this, "curveEditorControlToolBar1", wxToolBar );
	tb->ToggleTool( XRCID( "showControlPoints" ), true );

	// Connect time and value controls and set default values
	XRCCTRL( *this, "time", wxTextCtrl )->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( CEdCurveEditor::OnTimeValueChanged ), NULL, this );
	XRCCTRL( *this, "value", wxTextCtrl )->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( CEdCurveEditor::OnTimeValueChanged ), NULL, this );
	XRCCTRL( *this, "absoluteChange", wxCheckBox )->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdCurveEditor::OnAbsoluteChanged ), NULL, this );
	ClearTimeValue();

	m_absoluteMove = XRCCTRL( *this, "absoluteChange", wxCheckBox )->IsChecked();

	// Create Curve Editor Canvas panel
	wxPanel* rp = XRCCTRL( *this, "CurveEditorCanvas", wxPanel );
	wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );		
	m_curveEditorCanvas = new CEdEffectCurveEditorCanvas( rp, this );
	m_curveEditorCanvas->SetParentCurveEditor( this );
	GetCanvas()->SetXIsUnlocked( true );
	GetCanvas()->SetYIsUnlocked( true );
	sizer1->Add( m_curveEditorCanvas, 1, wxEXPAND, 0 );
	rp->SetSizer( sizer1 );		
	rp->Layout();

	// Update and finalize layout
	Layout();
	Show();
}

void CEdEffectCurveEditor::Synchronize( wxPoint offset, const Vector& s )
{
	Vector scale = s;
	wxPoint oldOffset = m_curveEditorCanvas->GetOffset();
	offset.y = oldOffset.y;
	m_curveEditorCanvas->SetOffset( offset );
	scale.Y = m_curveEditorCanvas->GetCurveScale().Y;
	m_curveEditorCanvas->SetCurveScale( scale );
	m_curveEditorCanvas->Repaint( true );
}

