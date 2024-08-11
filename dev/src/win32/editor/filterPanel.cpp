/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "filterPanel.h"
#include "../../common/core/configFileManager.h"
#include "../../common/core/configVarSystem.h"
#include "../../common/core/configVarLegacyWrapper.h"
#include "../../common/engine/viewport.h"
#include "../../common/engine/areaComponent.h"
#include "../../common/core/depot.h"

extern EShowFlags GShowGameFilter[];
extern EShowFlags GShowEditorFilter[];
extern EShowFlags GShowPreviewFilter[];
extern EShowFlags GShowRenderFilter[];
extern EShowFlags GShowPostProcessFilter[];
extern EShowFlags GShowPhysicsDebugFilter[];
extern EShowFlags GShowUmbraDebugFilter[];
extern EShowFlags GShowGameMask[];
extern EShowFlags GShowEditorMask[];
extern EShowFlags GShowPreviewMask[];

class wxClassObject : wxObject
{
public:
	wxClassObject( const String& type ) : m_class( type ) {}

	String m_class;
};

// static 
Bool CEdFilterPanel::IsInFilter( const EShowFlags* filter, Int32 value )
{
	for ( Uint32 i = 0; filter[ i ] < SHOW_MAX_INDEX; ++i )
	{
		if ( value == filter[ i ] )
		{
			return true;
		}
	}
	return false;
}

// static
void CEdFilterPanel::BuildFilterByAnd( TDynArray< EShowFlags >& filter, const EShowFlags* and1, const EShowFlags* and2 )
{
	filter.Clear();
	for ( Uint32 i = 0; and1[ i ] < SHOW_MAX_INDEX; ++i ) 
	{
		if ( CEdFilterPanel::IsInFilter( and2, and1[ i ] ) )
		{
			filter.PushBack( and1[ i ] );
		}
	}
	filter.PushBack( SHOW_MAX_INDEX );
}

// static
void CEdFilterPanel::BuildFilterByNot( TDynArray< EShowFlags >& filter, const EShowFlags* and1, const EShowFlags* and2 )
{
	filter.Clear();
	for ( Uint32 i = 0; and1[ i ] < SHOW_MAX_INDEX; ++i ) 
	{
		if ( !CEdFilterPanel::IsInFilter( and2, and1[ i ] ) )
		{
			filter.PushBack( and1[ i ] );
		}
	}
	filter.PushBack( SHOW_MAX_INDEX );
}

// static
void CEdFilterPanel::BuildFilterByAndNot( TDynArray< EShowFlags >& filter, const EShowFlags* and1, const EShowFlags* and2, const EShowFlags* and3 )
{
	filter.Clear();
	for ( Uint32 i = 0; and1[ i ] < SHOW_MAX_INDEX; ++i ) 
	{
		if ( CEdFilterPanel::IsInFilter( and2, and1[ i ] ) )
		{
			if ( !CEdFilterPanel::IsInFilter( and3, and1[ i ] ) )
			{
				filter.PushBack( and1[ i ] );
			}
		}
	}
	filter.PushBack( SHOW_MAX_INDEX );
}

// static
void CEdFilterPanel::BuildFilterByNotNot( TDynArray< EShowFlags >& filter, const EShowFlags* and1, const EShowFlags* and2, const EShowFlags* and3 )
{
	filter.Clear();
	for ( Uint32 i = 0; and1[ i ] < SHOW_MAX_INDEX; ++i ) 
	{
		if ( !CEdFilterPanel::IsInFilter( and2, and1[ i ] ) )
		{
			if ( !CEdFilterPanel::IsInFilter( and3, and1[ i ] ) )
			{
				filter.PushBack( and1[ i ] );
			}
		}
	}
	filter.PushBack( SHOW_MAX_INDEX );
}

void CEdFilterPanel::AddOptions( THashMap< Uint32, wxCheckBox* >& opts, wxSizer* sizer, const EShowFlags* filter )
{
	CEnum* c = SRTTI::GetInstance().FindEnum( CNAME( EShowFlags ) );

	TDynArray< CName > names;

	// Engine game options
	for ( Uint32 i=0; i<c->GetOptions().Size(); ++i )
	{
		CName name = c->GetOptions()[ i ];
		Int32 value( 0 );
		c->FindValue( name, value );
		if ( IsInFilter( filter, value ) )
		{
			names.PushBack( name );
		}
	}

	struct pred { bool operator()( const CName& a, const CName& b ) const { return a.AsString() < b.AsString(); } } pred;

	Sort( names.Begin(), names.End(), pred );

	for ( Uint32 i=0; i<names.Size(); ++i )
	{
		Int32 value( 0 );
		c->FindValue( names[ i ], value );
		AddOption( opts, sizer, names[ i ].AsString().AsChar()+5, static_cast< EShowFlags > ( value ) );
	}
}

void CEdFilterPanel::AddClassOptions( THashMap< CClass*, wxCheckBox* >& opts, THashMap< String, ClassTemplatesFlags >& choices, wxSizer* sizer )
{
	struct Collector
	{
		Collector( CEdFilterPanel* me )
			: m_this( me ) {}

		void Init( CClass* classObj )
		{
			m_classes.ClearFast();

			SRTTI::GetInstance().EnumClasses( classObj, m_classes );

			struct pred { bool operator()( const CClass* a, const CClass* b ) const { return a->GetName().AsString() < b->GetName().AsString(); } } pred;

			Sort( m_classes.Begin(), m_classes.End(), pred );
		}

		void AddOptions( THashMap< CClass*, wxCheckBox* >& opts, wxSizer* sizer )
		{
			for ( Uint32 i = 0, n = m_classes.Size(); i < n; ++i )
			{
				m_this->AddOption( opts, sizer, m_classes[i] );
			}
		}

		void AddOptions( THashMap< CClass*, wxCheckBox* >& opts, THashMap< String, ClassTemplatesFlags >& templatesOpts, THashMap< CClass*, TDynArray< String > >& entitiesPerClasses, wxSizer* sizer )
		{
			for ( Uint32 i = 0, n = m_classes.Size(); i < n; ++i )
			{
				CClass* currClass = m_classes[i];
				m_this->AddOption( opts, sizer, currClass );
				m_this->AddOption( templatesOpts, entitiesPerClasses.GetRef( currClass, TDynArray< String >() ), sizer, currClass );
			}
		}

		CEdFilterPanel*			m_this;
		TDynArray< CClass* >	m_classes;
	} collector( this );

	{
		// Add label
		wxStaticText * label = new wxStaticText( sizer->GetContainingWindow(), wxID_ANY, TXT("Area classes:"), wxDefaultPosition, wxDefaultSize, 0 );
		sizer->Add( label, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM | wxTOP, 5 );
	}

	// collect area templates
	THashMap< CClass*, TDynArray< String > > entitiesPerClasses;
	GatherAreaTemplates( GDepot->FindPath( TXT("gameplay\\areas\\") ), entitiesPerClasses );
	GatherAreaTemplates( GDepot->FindPath( TXT( "engine\\templates\\editor\\" ) ), entitiesPerClasses );

	collector.Init( CAreaComponent::GetStaticClass() );
	collector.AddOptions( opts, choices, entitiesPerClasses, sizer );

	// waypoints flags
	{
		// Add label
		wxStaticText * label = new wxStaticText( sizer->GetContainingWindow(), wxID_ANY, TXT("Waypoint classes:"), wxDefaultPosition, wxDefaultSize, 0 );
		sizer->Add( label, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM | wxTOP, 5 );
	}

	collector.Init( CWayPointComponent::GetStaticClass() );
	collector.AddOptions( opts, sizer );
}

void CEdFilterPanel::GatherAreaTemplates( CDirectory* dir, THashMap< CClass*, TDynArray< String > >& outEntitiesPerClasses ) const
{
	if ( !dir )
	{
		return;
	}

	for ( CResourceIterator< CEntityTemplate > entTemplate( dir, String::EMPTY, RIF_ReadOnly ); entTemplate; ++entTemplate )
	{
		CEntityTemplate* et = entTemplate.Get();
		if ( CEntity* ent = entTemplate->GetEntityObject() )
		{
			SEntityStreamingState worldState;
			ent->PrepareStreamingComponentsEnumeration( worldState, true, SWN_DoNotNotifyWorld );
			ent->ForceFinishAsyncResourceLoads();

			for ( Uint32 i = 0; i < ent->GetComponents().Size(); ++i )
			{
				if ( ent->GetComponents()[i]->IsA< CAreaComponent >() )
				{
					CClass* compClass = ent->GetComponents()[i]->GetClass();
					if ( outEntitiesPerClasses.KeyExist( compClass ) )
					{
						TDynArray< String >* templates = outEntitiesPerClasses.FindPtr( compClass );
						templates->PushBack( et->GetFile()->GetFileName() );
					}
					else
					{
						TDynArray< String > templates;
						templates.PushBack( et->GetFile()->GetFileName() );
						outEntitiesPerClasses.Insert( compClass, templates );
					}
					break;
				}
			}

			ent->FinishStreamingComponentsEnumeration( worldState );
		}
	}	
}

CEdFilterPanel::CEdFilterPanel( wxWindow* parent, IViewport* viewport )
	: wxPanel( parent )
	, m_viewport( viewport )
{
	// Create sizer
	m_book = new wxTreebook( this, wxID_ANY );

	wxSizer* sizer;

	// Add common options tab
	sizer = AddPage( TXT("Common options"), true );	
	wxStaticText* commonRenderDistanceSliderText = new wxStaticText( sizer->GetContainingWindow(), wxID_ANY, TXT("VisualDebug max render distance [m]: (0 - unlimited)"), wxDefaultPosition, wxDefaultSize, 0 );
	sizer->Add( commonRenderDistanceSliderText, 0, wxALL | wxLEFT, 5 );
		
	m_commonRenderDistanceSlider = new wxSlider( sizer->GetContainingWindow(), wxID_ANY, 100, 0, 1000, wxDefaultPosition, wxSize( -1,-1 ), wxSL_TOP|wxSL_HORIZONTAL|wxSL_LABELS|wxSL_SELRANGE|wxSL_BOTH );
	sizer->Add( m_commonRenderDistanceSlider, 0, wxALL|wxEXPAND, 1 );

	m_commonRenderDistanceSlider->Connect( wxEVT_COMMAND_SLIDER_UPDATED, wxCommandEventHandler( CEdFilterPanel::OnFlagUpdate ), NULL, this );

	wxStaticText* commonRenderLineThicknessText = new wxStaticText( sizer->GetContainingWindow(), wxID_ANY, TXT("VisualDebug thickness of debug lines"), wxDefaultPosition, wxDefaultSize, 0 );
	sizer->Add( commonRenderLineThicknessText, 0, wxALL | wxLEFT, 5 );

	m_commonRenderLineThicknessSlider = new wxSlider( sizer->GetContainingWindow(), wxID_ANY, 25, 0, 100, wxDefaultPosition, wxSize( -1,-1 ), wxSL_TOP|wxSL_HORIZONTAL|wxSL_LABELS|wxSL_SELRANGE|wxSL_BOTH );
	sizer->Add( m_commonRenderLineThicknessSlider, 0, wxALL|wxEXPAND, 1 );

	m_commonRenderLineThicknessSlider->Connect( wxEVT_COMMAND_SLIDER_UPDATED, wxCommandEventHandler( CEdFilterPanel::OnFlagUpdate ), NULL, this );

	sizer = AddPage( TXT("Game options"), true );

	wxButton* exportBtn = new wxButton( sizer->GetContainingWindow(), wxID_ANY, TXT( "Export filter settings for game" ) );
	sizer->Add( exportBtn, 0, wxEXPAND | wxALL, 5 );
	exportBtn->Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CEdFilterPanel::OnExportFiltersForGame, this );

	sizer = AddPage( TXT("Render") );

	TDynArray< EShowFlags > filter;

	BuildFilterByAnd( filter, GShowGameFilter, GShowRenderFilter );
	AddOptions( m_gameOnlyFlags, sizer, filter.TypedData() );

	sizer = AddPage( TXT("Debug") );

	BuildFilterByNotNot( filter, GShowGameFilter, GShowRenderFilter, GShowPostProcessFilter );
	AddOptions( m_gameOnlyFlags, sizer, filter.TypedData() );
	{
		// Add label
		wxStaticText * label = new wxStaticText( sizer->GetContainingWindow(), wxID_ANY, TXT("Per class filters:"), wxDefaultPosition, wxDefaultSize, 0 );
		sizer->Add( label, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5 );
	}
	AddClassOptions( m_gameClassFlags, m_gameClassesTemplatesFlags, sizer );

	sizer = AddPage( TXT("Physics Debug") );

	AddOptions( m_gameOnlyFlags, sizer, GShowPhysicsDebugFilter );

	sizer = AddPage( TXT("Umbra Debug") );

	AddOptions( m_gameOnlyFlags, sizer, GShowUmbraDebugFilter );

	sizer = AddPage( TXT("Post-process") );

	BuildFilterByAnd( filter, GShowGameFilter, GShowPostProcessFilter );
	AddOptions( m_gameOnlyFlags, sizer, filter.TypedData() );
	{
		// Add label
		wxStaticText * label = new wxStaticText( sizer->GetContainingWindow(), wxID_ANY, TXT("Per class filters:"), wxDefaultPosition, wxDefaultSize, 0 );
		sizer->Add( label, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5 );
	}

	sizer = AddPage( TXT("Simulation") );

	// Simulation options
	AddSimulationOption( m_gameSimulationFlags, sizer, TXT("Scripts"), ES_Scripts );
	AddSimulationOption( m_gameSimulationFlags, sizer, TXT("Animation"), ES_Animation );
	AddSimulationOption( m_gameSimulationFlags, sizer, TXT("Particles"), ES_Particles );
	AddSimulationOption( m_gameSimulationFlags, sizer, TXT("Physics"), ES_Physics );
	AddSimulationOption( m_gameSimulationFlags, sizer, TXT("Rendering"), ES_Rendering );

	// Create group box
	sizer = AddPage( TXT("Editor options"), true );
	sizer = AddPage( TXT("Render") );

	// Engine editor options
	BuildFilterByAnd( filter, GShowEditorFilter, GShowRenderFilter );
	AddOptions( m_editorOnlyFlags, sizer, filter.TypedData() );
	
	sizer = AddPage( TXT("Debug") );

	BuildFilterByNotNot( filter, GShowEditorFilter, GShowRenderFilter, GShowPostProcessFilter );
	AddOptions( m_editorOnlyFlags, sizer, filter.TypedData() );
	{ // Add label
		wxStaticText * label = new wxStaticText( sizer->GetContainingWindow(), wxID_ANY, TXT("Per class filters:"), wxDefaultPosition, wxDefaultSize, 0 );
		sizer->Add( label, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5 );
	}
	AddClassOptions( m_editorClassFlags, m_editorClassesTemplatesFlags, sizer );

	sizer = AddPage( TXT("Physics Debug") );

	AddOptions( m_editorOnlyFlags, sizer, GShowPhysicsDebugFilter );

	sizer = AddPage( TXT("Umbra Debug") );

	AddOptions( m_editorOnlyFlags, sizer, GShowUmbraDebugFilter );

	sizer = AddPage( TXT("Post-process") );

	BuildFilterByAnd( filter, GShowEditorFilter, GShowPostProcessFilter );
	AddOptions( m_editorOnlyFlags, sizer, filter.TypedData() );
	{ // Add label
		wxStaticText * label = new wxStaticText( sizer->GetContainingWindow(), wxID_ANY, TXT("Per class filters:"), wxDefaultPosition, wxDefaultSize, 0 );
		sizer->Add( label, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5 );
	}

	sizer = AddPage( TXT("Simulation") );

	// Simulation options
	AddSimulationOption( m_editorSimulationFlags, sizer, TXT("Animation"), ES_Animation );
	AddSimulationOption( m_editorSimulationFlags, sizer, TXT("Rendering"), ES_Rendering );
	AddSimulationOption( m_editorSimulationFlags, sizer, TXT("Particles"), ES_Particles );
	// AddSimulationOption( m_editorSimulationFlags, sizer, TXT("Physics"), ES_Physics ); // tw > please, do not add this! Physics simulation in editor is always disabled

	// Create group box
	sizer = AddPage( TXT("Preview options"), true );
	sizer = AddPage( TXT("Render") );

	// Preview options
	BuildFilterByAnd( filter, GShowPreviewFilter, GShowRenderFilter );
	AddOptions( m_previewOnlyFlags, sizer, filter.TypedData() );
	
	sizer = AddPage( TXT("Debug") );

	BuildFilterByNot( filter, GShowPreviewFilter, GShowRenderFilter );
	AddOptions( m_previewOnlyFlags, sizer, filter.TypedData() );

	sizer = AddPage( TXT("Physics Debug") );

	AddOptions( m_previewOnlyFlags, sizer, GShowPhysicsDebugFilter );

	// Update flags
	LoadOptionsFromConfig();

	// Setup layout
	wxBoxSizer* mainsizer = new wxBoxSizer( wxVERTICAL );

	mainsizer->Add( m_book, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5 );
	
	SetSizer( mainsizer );
	Layout();
	Refresh();

	for( Uint32 i=0; i<m_book->GetPageCount(); ++i)
		m_book->ExpandNode( i );

	m_debugLinesThickness = (Float)(m_commonRenderLineThicknessSlider->GetValue());
	m_debugMaxDistance = (Float)(m_commonRenderDistanceSlider->GetValue());
	UpdateFlags();
}

wxSizer* CEdFilterPanel::AddPage( const String &name, bool mainPage )
{
	wxScrolledWindow* sw = new wxScrolledWindow( m_book );
	sw->EnableScrolling( true, true );
	sw->SetScrollRate( 1, 32 ); 
	wxSizer* sizer = new wxBoxSizer( wxVERTICAL );
	sw->SetSizer( sizer );

	if ( mainPage )
		m_book->AddPage( sw, name.AsChar() );
	else
		m_book->AddSubPage( sw, name.AsChar() );

	return sizer;
}

void CEdFilterPanel::AddOption( THashMap< Uint32, wxCheckBox* > &storage, wxSizer* sizer, const String& name, EShowFlags flag )
{
	// Add checkbox
	wxCheckBox* box = new wxCheckBox( sizer->GetContainingWindow(), wxID_ANY, name.AsChar(), wxDefaultPosition, wxDefaultSize, 0 );
	sizer->Add( box, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5 );
	box->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdFilterPanel::OnFlagUpdate ), NULL, this );

	// Add to flag map
	storage.Set( flag, box );
}


namespace
{
	String FormatClassName( const String & name )
	{
		if ( name.Empty() )
			return String::EMPTY;

		Char c = name[0];

		String res;
		res.Append( c ).MakeUpper();

		for ( Uint32 i = 1; i < name.GetLength(); ++i )
		{
			Char prev_c = c;
			c = name[i];

			if ( IsUpper( c ) && ! IsUpper( prev_c ) && ! IsWhiteSpace( prev_c ) )
				res.Append( TXT(' ') );
			else
				if ( IsNumber( c ) ^ IsNumber( prev_c ) )
					res.Append( TXT(' ') );

			if ( ! IsWhiteSpace( c ) || ! IsWhiteSpace( prev_c ) )
				res.Append( c );
		}

		return res;
	}
}

void CEdFilterPanel::AddOption( THashMap< CClass*, wxCheckBox* > &storage, wxSizer* sizer, CClass* type )
{
	// Add checkbox
	wxCheckBox* box = new wxCheckBox( sizer->GetContainingWindow(), wxID_ANY,
		FormatClassName( type->GetName().AsString().MidString(1) ).AsChar(), wxDefaultPosition, wxDefaultSize, 0 );
	sizer->Add( box, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 5 );
	box->SetValue( true );
	box->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdFilterPanel::OnFlagUpdate ), NULL, this );

	// Add to flag map
	storage.Set( type, box );
}

void CEdFilterPanel::AddOption( THashMap< String, ClassTemplatesFlags > &classesListBoxes, TDynArray< String >& templates, wxSizer* sizer, CClass* type )
{
	if ( !classesListBoxes.KeyExist( type->GetName().AsString() ) )
	{
		wxArrayString elements;
		for ( Uint32 i = 0; i < templates.Size(); ++i )
		{
			elements.Add( templates[i].AsChar() );
		}
		elements.Add( wxT("other templates") );
		elements.Add( wxT("non-templated") );

		wxCheckListBox* lBox = new wxCheckListBox( sizer->GetContainingWindow(), wxID_ANY, wxDefaultPosition, wxSize( 230, -1 ), elements, wxLB_NEEDED_SB );
		wxCheckBox* selectAllCBox = nullptr;

		if ( elements.Count() > 2 )
		{
			selectAllCBox = new wxCheckBox( sizer->GetContainingWindow(), wxID_ANY, wxT("Select all"), wxDefaultPosition, wxDefaultSize, wxCHK_3STATE );
			selectAllCBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdFilterPanel::OnSelectAll ), (wxObject*)(new wxClassObject( type->GetName().AsString() )), this );
			sizer->Add( selectAllCBox, 0, wxLEFT | wxRIGHT, 20 );
		}

		lBox->Connect( wxEVT_COMMAND_CHECKLISTBOX_TOGGLED, wxCommandEventHandler( CEdFilterPanel::OnFlagUpdate ), (wxObject*)(new wxClassObject( type->GetName().AsString() ) ), this );
		sizer->Add( lBox, 0, wxLEFT | wxRIGHT, 20 );

		ClassTemplatesFlags ctf( selectAllCBox, lBox );
		classesListBoxes.Set( type->GetName().AsString(), ctf );
	}
}

TEdShortcutArray *CEdFilterPanel::GetAccelerators()
{
    if (m_shortcuts.Empty())
    {
        m_mapIndexToFlag.Clear();

        THashMap< Uint32, wxCheckBox*	>::iterator OPT_curr = m_gameOnlyFlags.Begin(),
                                            OPT_last = m_gameOnlyFlags.End();
        for (; OPT_curr != OPT_last; ++OPT_curr)
        {
            m_shortcuts.PushBack(SEdShortcut(TXT("Accelerators\\Filters\\Game\\") + OPT_curr->m_second->GetLabel(),
                wxAcceleratorEntry(0,0, ID_ACCEL_FILTER_FIRST + m_mapIndexToFlag.Size())) );
            m_mapIndexToFlag.PushBack(OPT_curr->m_first);
        }

        OPT_curr = m_gameSimulationFlags.Begin();
        OPT_last = m_gameSimulationFlags.End();
        for (; OPT_curr != OPT_last; ++OPT_curr)
        {
            m_shortcuts.PushBack(SEdShortcut(TXT("Accelerators\\Filters\\Game Simulation\\") + OPT_curr->m_second->GetLabel(),
                wxAcceleratorEntry(0,0, ID_ACCEL_FILTER_FIRST + m_mapIndexToFlag.Size())) );
            m_mapIndexToFlag.PushBack(OPT_curr->m_first);
        }

        OPT_curr = m_editorOnlyFlags.Begin();
        OPT_last = m_editorOnlyFlags.End();
        for (; OPT_curr != OPT_last; ++OPT_curr)
        {
            m_shortcuts.PushBack(SEdShortcut(TXT("Accelerators\\Filters\\Editor\\") + OPT_curr->m_second->GetLabel(),
                wxAcceleratorEntry(0,0, ID_ACCEL_FILTER_FIRST + m_mapIndexToFlag.Size())) );
            m_mapIndexToFlag.PushBack(OPT_curr->m_first);
        }

        OPT_curr = m_editorSimulationFlags.Begin();
        OPT_last = m_editorSimulationFlags.End();
        for (; OPT_curr != OPT_last; ++OPT_curr)
        {
            m_shortcuts.PushBack(SEdShortcut(TXT("Accelerators\\Filters\\Editor Simulation\\") + OPT_curr->m_second->GetLabel(),
                wxAcceleratorEntry(0,0, ID_ACCEL_FILTER_FIRST + m_mapIndexToFlag.Size())) );
            m_mapIndexToFlag.PushBack(OPT_curr->m_first);
        }

		OPT_curr = m_previewOnlyFlags.Begin();
		OPT_last = m_previewOnlyFlags.End();
		for (; OPT_curr != OPT_last; ++OPT_curr)
		{
			m_shortcuts.PushBack(SEdShortcut(TXT("Accelerators\\Filters\\Preview\\") + OPT_curr->m_second->GetLabel(),
				wxAcceleratorEntry(0,0, ID_ACCEL_FILTER_FIRST + m_mapIndexToFlag.Size())) );
			m_mapIndexToFlag.PushBack(OPT_curr->m_first);
		}
	}

	ASSERT( m_mapIndexToFlag.Size() < ID_ACCEL_FILTER_LAST - ID_ACCEL_FILTER_FIRST, TXT("Too many filters for the accelerator table - increase the ID_ACCEL_FILTER_LAST value") );

    return &m_shortcuts;
}

void CEdFilterPanel::OnAccelFilter( wxCommandEvent& event )
{
    int nId  = event.GetId() - ID_ACCEL_FILTER_FIRST;
    if (nId < 0) return;
    
    wxCheckBox* chk = NULL;

    if (nId < static_cast<int>( m_gameOnlyFlags.Size() ))
    {
        chk = *m_gameOnlyFlags.FindPtr( m_mapIndexToFlag[nId] );
    }
    else if (nId < static_cast<int>( m_gameOnlyFlags.Size() + m_gameSimulationFlags.Size() ))
    {
        chk = *m_gameSimulationFlags.FindPtr( m_mapIndexToFlag[nId] );
    }
    else if (nId < static_cast<int>( m_gameOnlyFlags.Size() + m_gameSimulationFlags.Size() + m_editorOnlyFlags.Size() ))
    {
        chk = *m_editorOnlyFlags.FindPtr( m_mapIndexToFlag[nId] );
    }
    else if (nId < static_cast<int>( m_gameOnlyFlags.Size() + m_gameSimulationFlags.Size() + m_editorOnlyFlags.Size() + m_editorSimulationFlags.Size() ))
    {
        chk = *m_editorSimulationFlags.FindPtr( m_mapIndexToFlag[nId] );
    }
	else if (nId < static_cast<int>( m_gameOnlyFlags.Size() + m_gameSimulationFlags.Size() + m_editorOnlyFlags.Size() + m_editorSimulationFlags.Size() + m_previewOnlyFlags.Size() ))
	{
		chk = *m_previewOnlyFlags.FindPtr( m_mapIndexToFlag[nId] );
	}
    else
        return;

    chk->SetValue(!chk->GetValue());
    UpdateFlags();
}

Bool CEdFilterPanel::GetSimulationEditorFlag( Uint32 flag )
{
	wxCheckBox** box = m_editorSimulationFlags.FindPtr( flag );
	if ( box )
	{
		ASSERT( *box );
		return (*box)->IsChecked();
	}

	return false;
}

void CEdFilterPanel::AddSimulationOption( THashMap< Uint32, wxCheckBox* > &storage, wxSizer* sizer, const String& name, Uint32 flag )
{
	wxCheckBox* box = new wxCheckBox( sizer->GetContainingWindow(), wxID_ANY, name.AsChar(), wxDefaultPosition, wxDefaultSize, 0 );
	sizer->Add( box, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5 );
	box->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdFilterPanel::OnChangeSimulationOption ), NULL, this );
	storage.Set( flag, box );
}

void CEdFilterPanel::OnChangeSimulationOption( wxCommandEvent& event )
{
	UpdateSimulationOptions( GGame->IsActive() );
}

CEdFilterPanel::~CEdFilterPanel()
{
    SaveOptionsToConfig();
}

void CEdFilterPanel::WriteFlagsToConfig( CConfigurationManager &config, THashMap< Uint32, wxCheckBox* > &storage )
{
	for ( THashMap< Uint32, wxCheckBox* >::iterator i = storage.Begin(); i != storage.End(); ++i )
	{
		String value = ( i->m_second->GetValue() ? TXT("True") : TXT("False") );
		config.Write(i->m_second->GetLabel().wc_str(), value);
	}
}

void CEdFilterPanel::WriteFlagsToConfig( CConfigurationManager &config, THashMap< CClass*, wxCheckBox*	> &storage )
{
	for ( THashMap< CClass*, wxCheckBox*>::iterator i = storage.Begin(); i != storage.End(); ++i )
	{
		String value = ( i->m_second->GetValue() ? TXT("True") : TXT("False") );
		config.Write(i->m_first->GetName().AsString(), value);
	}
}

void CEdFilterPanel::WriteFlagsToConfig( CConfigurationManager &config, THashMap< String, ClassTemplatesFlags > &storage )
{
	for ( THashMap< String, ClassTemplatesFlags >::iterator i = storage.Begin(); i != storage.End(); ++i )
	{
		if ( i->m_second.m_selectAllCBox )
		{
			wxCheckBoxState state = i->m_second.m_selectAllCBox->Get3StateValue();
			String value = TXT("unchecked");
			if ( state == wxCHK_CHECKED )
			{
				value = TXT("checked");
			}
			else if ( state == wxCHK_UNDETERMINED )
			{
				value = TXT("undetermined");
			}
			config.Write( i->m_first + TXT("SelectAll"), value );
		}
		for ( Uint32 ind = 0; ind < i->m_second.GetTemplatesCount(); ++ind )
		{
			String value = ( i->m_second.IsTemplateChecked( ind ) ? TXT("True") : TXT("False") );
			config.Write( i->m_first + i->m_second.GetTemplateName( ind ), value );
		}
	}
}

void CEdFilterPanel::SaveOptionsToConfig()
{
	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();

	{
		CConfigurationScopedPathSetter pathSetter( config, TXT("/Filter/Game") );
		WriteFlagsToConfig( config, m_gameOnlyFlags );
		WriteFlagsToConfig( config, m_gameClassFlags );
		WriteFlagsToConfig( config, m_gameClassesTemplatesFlags );
	}

	{
		CConfigurationScopedPathSetter pathSetter( config, TXT("/Filter/Editor") );
		WriteFlagsToConfig( config, m_editorOnlyFlags );
		WriteFlagsToConfig( config, m_editorClassFlags );
		WriteFlagsToConfig( config, m_editorClassesTemplatesFlags );
	}

	{
		CConfigurationScopedPathSetter pathSetter( config, TXT("/Simulation/Editor") );
		WriteFlagsToConfig( config, m_editorSimulationFlags );
	}

	{
		CConfigurationScopedPathSetter pathSetter( config, TXT("/Simulation/Game") );
		WriteFlagsToConfig( config, m_gameSimulationFlags );
	}

	{
		CConfigurationScopedPathSetter pathSetter( config, TXT("/Filter/Preview") );
		WriteFlagsToConfig( config, m_previewOnlyFlags );
	}
		
	config.Write( TXT("/Filter/Common/maxRenderDistance"), m_commonRenderDistanceSlider->GetValue() );
	config.Write( TXT("/Filter/Common/lineThickness"), m_commonRenderLineThicknessSlider->GetValue() );
}

void CEdFilterPanel::UpdateSimulationOptions( bool game )
{
	if( game )
	{
		for( THashMap< Uint32, wxCheckBox* >::iterator it=m_gameSimulationFlags.Begin(); it!=m_gameSimulationFlags.End(); ++it )
		{
			GEngine->SetActiveSubsystem( it->m_first, it->m_second->GetValue() ); 
		}
	}
	else
	{
		for( THashMap< Uint32, wxCheckBox* >::iterator it=m_editorSimulationFlags.Begin(); it!=m_editorSimulationFlags.End(); ++it )
		{
			GEngine->SetActiveSubsystem( it->m_first, it->m_second->GetValue() ); 
		}
	}
}

void CEdFilterPanel::GetFlagsFromConfig( CConfigurationManager &config, THashMap< Uint32, wxCheckBox*	> &storage, const EShowFlags* defaultFlags )
{
	for ( THashMap< Uint32, wxCheckBox* >::iterator i = storage.Begin(); i != storage.End(); ++i )
	{
		// setting flag's value based on defaults from the viewport and
		// information stored in configuration ini file
		Bool check;
		String result;
		if ( config.Read( i->m_second->GetLabel().wc_str(), &result ) )
		{
			check = ( result == TXT("True") );
		}
		else
		{
			if ( defaultFlags )
			{
				check = CEdFilterPanel::IsInFilter( defaultFlags, i->m_first );
			}
			else
			{
				check = true;
			}
		}
		i->m_second->SetValue( check );
	}
}

void CEdFilterPanel::GetFlagsFromConfig( CConfigurationManager &config, THashMap< CClass*, wxCheckBox*	> &storage )
{
	for ( THashMap< CClass*, wxCheckBox* >::iterator i = storage.Begin(); i != storage.End(); ++i )
	{
		Bool check = true;
		String result;
		if ( config.Read( i->m_first->GetName().AsString(), &result ) )
		{
			check = ( result == TXT("True") );
		}
		i->m_second->SetValue( check );
	}
}

void CEdFilterPanel::GetFlagsFromConfig( CConfigurationManager &config, THashMap< String, ClassTemplatesFlags	> &storage )
{
	for ( THashMap< String, ClassTemplatesFlags >::iterator i = storage.Begin(); i != storage.End(); ++i )
	{
		String result;
		if ( i->m_second.m_selectAllCBox )
		{
			wxCheckBoxState state = wxCHK_UNCHECKED;
			if ( config.Read( i->m_first + TXT("SelectAll"), &result ) )
			{
				if ( result == TXT("checked") )
				{
					state = wxCHK_CHECKED;
				}
				else if ( result == TXT("undetermined") )
				{
					state = wxCHK_UNDETERMINED;
				}
			}
			i->m_second.m_selectAllCBox->Set3StateValue( state );
		}

		Bool check = true;
		for ( Uint32 ind = 0; ind < i->m_second.GetTemplatesCount(); ++ind )
		{
			if ( config.Read( i->m_first + i->m_second.GetTemplateName( ind ), &result ) )
			{
				check = ( result == TXT("True") );
			}
			i->m_second.CheckTemplate( ind, check );
		}
	}
}

void CEdFilterPanel::LoadOptionsFromConfig()
{
	// Get mask
	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();

	{
		CConfigurationScopedPathSetter pathSetter( config, TXT("/Filter/Game") );
		GetFlagsFromConfig( config, m_gameOnlyFlags, GShowGameMask );
		GetFlagsFromConfig( config, m_gameClassFlags );
		GetFlagsFromConfig( config, m_gameClassesTemplatesFlags );
	}

	{
		CConfigurationScopedPathSetter pathSetter( config, TXT("/Filter/Editor") );
		GetFlagsFromConfig( config, m_editorOnlyFlags, GShowEditorMask );
		GetFlagsFromConfig( config, m_editorClassFlags );
		GetFlagsFromConfig( config, m_editorClassesTemplatesFlags );
	}

	{
		CConfigurationScopedPathSetter pathSetter( config, TXT("/Simulation/Editor") );
		GetFlagsFromConfig( config, m_editorSimulationFlags, NULL );
	}

	{
		CConfigurationScopedPathSetter pathSetter( config, TXT("/Simulation/Game") );
		GetFlagsFromConfig( config, m_gameSimulationFlags, NULL );
	}

	{
		CConfigurationScopedPathSetter pathSetter( config, TXT("/Filter/Preview") );
		GetFlagsFromConfig( config, m_previewOnlyFlags, GShowPreviewMask );
	}
	
	Int32 val = 100;		
	m_commonRenderDistanceSlider->SetValue( config.Read( TXT("/Filter/Common/maxRenderDistance"), val ) );
	m_commonRenderLineThicknessSlider->SetValue( config.Read( TXT("/Filter/Common/lineThickness"), val ) );

	UpdateFlags();
	UpdateSimulationOptions( false );
}

void CEdFilterPanel::OnFlagUpdate( wxCommandEvent& event )
{
	wxClassObject* userData = (wxClassObject*)( event.m_callbackUserData );
	if ( userData && !userData->m_class.Empty() )
	{
		THashMap< String, ClassTemplatesFlags > classTemplates = GGame->IsActive() ? m_gameClassesTemplatesFlags : m_editorClassesTemplatesFlags;
		if ( ClassTemplatesFlags* ctf = classTemplates.FindPtr( userData->m_class ) )
		{
			if ( wxCheckListBox* lBox = wxDynamicCast( event.GetEventObject(), wxCheckListBox ) )
			{
				Bool anythingSelected = ctf->IsAnyTemplateChecked();
				Bool anythingUnselected = ctf->IsAnyTemplateUnchecked();

				wxCheckBoxState newState = wxCHK_UNDETERMINED;
				if ( !anythingSelected )
				{
					newState = wxCHK_UNCHECKED;
				}
				else if ( !anythingUnselected )
				{
					newState = wxCHK_CHECKED;
				}

				ctf->CheckSelectAllBox( newState );
			}
		}
	}

	UpdateFlags();
}

void CEdFilterPanel::OnSelectAll( wxCommandEvent& event )
{
	wxClassObject* userData = (wxClassObject*)( event.m_callbackUserData );
	if ( userData && !userData->m_class.Empty() )
	{
		THashMap< String, ClassTemplatesFlags > classTemplates = GGame->IsActive() ? m_gameClassesTemplatesFlags : m_editorClassesTemplatesFlags;
		if ( ClassTemplatesFlags* ctf = classTemplates.FindPtr( userData->m_class ) )
		{
			ctf->CheckAllTemplates( event.IsChecked() );
		}
	}
	UpdateFlags();
}

void CEdFilterPanel::UpdateFlags()
{
	m_viewport->ClearRenderingMask( SHOW_ALL_FLAGS );
	m_viewport->SetRenderingMask( GetViewportFlags( GGame->IsActive() ? VFT_GAME : VFT_EDITOR ) );
	
	m_viewport->ClearClassRenderingExclusion();
	THashMap< CClass*, wxCheckBox* >& classesToExclude = GGame->IsActive() ? m_gameClassFlags : m_editorClassFlags;
	THashMap< CClass*, wxCheckBox* >::iterator
		currClass = classesToExclude.Begin(),
		lastClass = classesToExclude.End();

	THashMap< String, ClassTemplatesFlags > & templatesToExclude = GGame->IsActive() ? m_gameClassesTemplatesFlags : m_editorClassesTemplatesFlags;

	for ( ; currClass != lastClass; ++currClass )
	{
		Bool checked = currClass->m_second->IsChecked();
		m_viewport->SetClassRenderingExclusion( currClass->m_first, !checked );

		if ( ClassTemplatesFlags* classTemplate = templatesToExclude.FindPtr( currClass->m_first->GetName().AsString() ) )
		{
			classTemplate->Enable( checked );

			Uint32 count = classTemplate->GetTemplatesCount();
			for ( Uint32 i = 0; i < count; ++i )
			{
				Bool selected = checked && classTemplate->IsTemplateChecked( i );
				String et = classTemplate->GetTemplateName( i );

				if ( i >= count - 2 )
				{
					et = et + String::Printf( TXT(" %ls"), currClass->m_first->GetName().AsString().AsChar() );
				}

				m_viewport->SetTemplateRenderingExclusion( et, !selected );
			}
		}
	}

	THashMap< CClass*, wxCheckBox* >& classesToUpdateUI = GGame->IsActive() ? m_editorClassFlags : m_gameClassFlags;
	THashMap< String, ClassTemplatesFlags > & templatesToUpdateUI = GGame->IsActive() ? m_editorClassesTemplatesFlags : m_gameClassesTemplatesFlags;
	currClass = classesToUpdateUI.Begin();
	lastClass = classesToUpdateUI.End();
	for ( ; currClass != lastClass; ++currClass )
	{
		Bool checked = currClass->m_second->IsChecked();

		if ( ClassTemplatesFlags* ctf = templatesToUpdateUI.FindPtr( currClass->m_first->GetName().AsString() ) )
		{
			ctf->Enable( checked );
		}
	}

	m_debugMaxDistance = (Float)(m_commonRenderDistanceSlider->GetValue());
	m_debugMaxDistance = ( m_debugMaxDistance < 0.01 ) ? 100000.f : m_debugMaxDistance;
	m_debugMaxDistance *= m_debugMaxDistance;

	m_debugLinesThickness = (Float)m_commonRenderLineThicknessSlider->GetValue()*0.001;

	for( Uint32 i=0; i<m_previewPanels.Size(); ++i )
	{
		UpdatePreviewPanel(m_previewPanels[i]);
	}
	m_viewport->SetRenderingDebugOptions( VDCommon_MaxRenderingDistance, m_debugMaxDistance );
	m_viewport->SetRenderingDebugOptions( VDCommon_DebugLinesThickness, m_debugLinesThickness );
}

const EShowFlags* CEdFilterPanel::GetViewportFlags( EViewportFlagsType type ) const
{
	// Merge flags
	static TDynArray< EShowFlags > flags;
	flags.Clear();

	const THashMap< Uint32, wxCheckBox* >* map = &m_gameOnlyFlags;
	if ( type == VFT_EDITOR )
	{
		map = &m_editorOnlyFlags;
	}
	else if ( type == VFT_PREVIEW )
	{
		map = &m_previewOnlyFlags;
	}

	for ( THashMap< Uint32, wxCheckBox* >::const_iterator i = map->Begin(); i != map->End(); ++i )
	{
		if ( i->m_second->GetValue() )
		{
			flags.PushBack( static_cast< EShowFlags > ( i->m_first ) );
		}
	}

	flags.PushBack( SHOW_MAX_INDEX );
	return flags.TypedData();
}

void CEdFilterPanel::SetViewportFlag( EViewportFlagsType type, EShowFlags flag, Bool b )
{
	const THashMap< Uint32, wxCheckBox* >* map = &m_gameOnlyFlags;
	if ( type == VFT_EDITOR )
	{
		map = &m_editorOnlyFlags;
	}
	else if ( type == VFT_PREVIEW )
	{
		map = &m_previewOnlyFlags;
	}
	
	THashMap< Uint32, wxCheckBox* >::const_iterator it = map->Find( flag );
	if ( it != map->End() )
	{
		if ( it->m_second->GetValue() != b )
		{
			it->m_second->SetValue( b );
			UpdateFlags();
		}
	}
}

void CEdFilterPanel::RegisterPreviewPanel( CEdPreviewPanel* panel )
{
	m_previewPanels.PushBackUnique( panel );
}

void CEdFilterPanel::UnregisterPreviewPanel( CEdPreviewPanel* panel )
{
	m_previewPanels.Remove( panel );
}

void CEdFilterPanel::OnExportFiltersForGame( wxCommandEvent& event )
{
	ExportFiltersForGame();
}

void CEdFilterPanel::ExportFiltersForGame()
{
	CEnum* showFlagEnum = SRTTI::GetInstance().FindEnum( CNAME( EShowFlags ) );

	RED_MESSAGE( "Using legacy config, please convert" );
	Config::Legacy::CConfigLegacyFile* filterConfig = SConfig::GetInstance().GetLegacy().GetFile( TXT( "filters" ) );

	Config::Legacy::CConfigLegacySection* section = filterConfig->GetSection( TXT( "Show" ), true );
	
	for( THashMap< Uint32, wxCheckBox* >::const_iterator iter = m_gameOnlyFlags.Begin(); iter!= m_gameOnlyFlags.End(); ++iter )
	{
		CName name;
		if( showFlagEnum->FindName( iter->m_first, name ) )
		{
			String outValue = ToString( iter->m_second->GetValue() );

			section->WriteValue( name.AsString(), outValue, false );
		}
	}

	filterConfig->Write();
}

void CEdFilterPanel::UpdatePreviewPanel( CEdPreviewPanel* p )
{
	if ( !p ) return;
	p->GetViewport()->ClearRenderingMask( SHOW_ALL_FLAGS );
	p->GetViewport()->SetRenderingMask( GetViewportFlags( VFT_PREVIEW ) );
	p->GetViewport()->SetRenderingDebugOptions( VDCommon_MaxRenderingDistance, m_debugMaxDistance );
	p->GetViewport()->SetRenderingDebugOptions( VDCommon_DebugLinesThickness, m_debugLinesThickness );
}
