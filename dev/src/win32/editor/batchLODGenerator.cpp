/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#include "Build.h"
#include "batchLODGenerator.h"
#include "undoMeshEditor.h"
#include "../../common/core/jobGenericJobs.h"
#include "../../common/core/loadingJobManager.h"
#include "../../common/engine/mesh.h"
#include "../../common/core/directory.h"
#include "../../common/core/garbageCollector.h"
#include "../../common/core/feedback.h"

wxDEFINE_EVENT( wxEVT_LOD_GENERATOR_JOB_STARTED, wxCommandEvent );
wxDEFINE_EVENT( wxEVT_LOD_GENERATOR_JOB_ENDED,   wxCommandEvent );
wxDEFINE_EVENT( wxEVT_LOD_GENERATOR_MESSAGE,     wxCommandEvent );

BEGIN_EVENT_TABLE( CEdAddLODDialog, wxDialog )
	EVT_RADIOBUTTON( XRCID("BySliderRB"), CEdAddLODDialog::OnBySliderRB )
	EVT_RADIOBUTTON( XRCID("ByEditRB"), CEdAddLODDialog::OnByEditRB )
	EVT_SLIDER( XRCID("ReductionSlider"), CEdAddLODDialog::OnReductionSlider )
END_EVENT_TABLE()

CEdAddLODDialog::CEdAddLODDialog( wxWindow* parent, Bool editMode )
{
	wxXmlResource::Get()->LoadDialog( this, parent, wxT("AddLODDialog") );

	if ( editMode )
	{
		SetTitle( TXT("Edit LOD") );
	}

	m_reductionSlider		= XRCCTRL( *this, "ReductionSlider", wxSlider );
	m_reductionEdit			= XRCCTRL( *this, "ReductionEdit",   wxTextCtrl );
	m_distance				= XRCCTRL( *this, "Distance",        wxTextCtrl );
	m_faceLimit				= XRCCTRL( *this, "FaceLimit",       wxTextCtrl );
	m_removeSkinning		= XRCCTRL( *this, "RemoveSkinning",  wxCheckBox );
	m_insertAsChoice		= XRCCTRL( *this, "InsertAsChoice",  wxChoice );
	m_recalculateNormals	= XRCCTRL( *this, "RecalculateNormals",  wxCheckBox );
	m_hardEdgeAngle			= XRCCTRL( *this, "HardEdgeAngle",  wxSpinCtrl );
	m_geometryImportance	= XRCCTRL( *this, "GeometryImportance",  wxSlider );
	m_textureImportance		= XRCCTRL( *this, "TextureImportance",  wxSlider );
	m_materialImportance	= XRCCTRL( *this, "MaterialImportance",  wxSlider );
	m_groupImportance		= XRCCTRL( *this, "GroupImportance",  wxSlider );
	m_vertexColorImportance	= XRCCTRL( *this, "VertexColorImportance",  wxSlider );
	m_shadingImportance		= XRCCTRL( *this, "ShadingImportance",  wxSlider );

	m_reductionEdit->Disable();
	XRCCTRL( *this, "BySliderRB", wxRadioButton )->SetValue( 1 );
}

CEdAddLODDialog::~CEdAddLODDialog()
{
}


Bool CEdAddLODDialog::Execute( SLODPresetDefinition& definition, Int32 numLods, Int32& lodIdx )
{
	m_reductionEdit->SetValue( ToString( 100.f * definition.m_reduction ).AsChar() );
	m_distance->SetValue( ToString( definition.m_distance ).AsChar() );
	m_faceLimit->SetValue( ToString( definition.m_chunkFaceLimit ).AsChar() );
	m_removeSkinning->SetValue( definition.m_removeSkinning );
	m_recalculateNormals->SetValue( definition.m_recalculateNormals );
	m_hardEdgeAngle->SetValue( definition.m_hardEdgeAngle );

	if ( numLods >= 0 )
	{
		m_insertAsChoice->Clear();

		for ( Int32 i = 0; i <= numLods; ++i )
		{
			m_insertAsChoice->AppendString( ToString( i ).AsChar() );
		}

		m_insertAsChoice->Select( lodIdx == -1 ? numLods : lodIdx );
	}
	else
	{
		XRCCTRL( *this, "InsertAsPanel", wxPanel )->Hide();
		DoLayoutAdaptation();
	}

	if ( ShowModal() == wxID_OK )
	{
		Float d;
		FromString< Float >( String( m_reductionEdit->GetValue() ), d );
		definition.m_reduction = Clamp( d / 100.f, 0.f, 1.f );
		FromString< Float >( String( m_distance->GetValue() ), definition.m_distance );
		FromString< Uint32 >( String( m_faceLimit->GetValue() ), definition.m_chunkFaceLimit );
		definition.m_removeSkinning = m_removeSkinning->GetValue();
		definition.m_recalculateNormals = m_recalculateNormals->GetValue();
		definition.m_hardEdgeAngle = m_hardEdgeAngle->GetValue();
		definition.m_geometryImportance = m_geometryImportance->GetValue() / 2.f;
		definition.m_textureImportance = m_textureImportance->GetValue() / 2.f;
		definition.m_materialImportance = m_materialImportance->GetValue() / 2.f;
		definition.m_groupImportance = m_groupImportance->GetValue() / 2.f;
		definition.m_vertexColorImportance = m_vertexColorImportance->GetValue() / 2.f;
		definition.m_shadingImportance = m_shadingImportance->GetValue() / 2.f;

		if ( numLods >= 0 )
		{
			lodIdx = m_insertAsChoice->GetSelection();
		}

		return true;
	}
	else
	{
		return false;
	}
}

Bool CEdAddLODDialog::Execute( SLODPresetDefinition& definition )
{
	Int32 dummyLodIdx = -1;
	return Execute( definition, -1, dummyLodIdx );
}

void CEdAddLODDialog::OnBySliderRB( wxCommandEvent& event )
{
	m_reductionSlider->Enable();
	m_reductionEdit->Disable();
}

void CEdAddLODDialog::OnByEditRB( wxCommandEvent& event )
{
	m_reductionSlider->Disable();
	m_reductionEdit->Enable();
}

void CEdAddLODDialog::OnReductionSlider( wxCommandEvent& event )
{
	Float r = 100.f / m_reductionSlider->GetValue();
	m_reductionEdit->SetValue( ToString( r ).AsChar() );
}

// ------------------------------------------------------------------

class CEdBatchLodGenerator::CJobGenerateLOD : public CJobLoadResource
{
public:
	CJobGenerateLOD( CEdBatchLodGenerator* dialog, const String& resourceToLoad )
		: CJobLoadResource( resourceToLoad )
		, m_dialog( dialog )
		{}

	~CJobGenerateLOD()
	{
	}

private:
	CEdBatchLodGenerator* m_dialog;
	Bool m_wasLoaded;

	void SyncEvent( wxEventType type, const String& text )
	{
		wxCommandEvent* event = new wxCommandEvent( type );
		event->SetClientData( this );
		event->SetString( text.AsChar() );
		m_dialog->QueueEvent( event );
	}

	Bool PrepareInMainThread( CDiskFile* file )
	{
		Bool success = true;

		RunInMainThread( [ file, &success ](){

			file->GetStatus();

			if ( !file->IsLocal() && !file->IsCheckedOut() )
			{
				if ( !file->SilentCheckOut() )
				{
					success = false;
					return;
				}
			}

			// Call MarkModified to "disarm" the possibility of accessing the perforce API from another thread
			if ( !file->MarkModified() )
			{
				success = false;
			}
		} );

		return success;
	}

	Bool SaveInMainThread( CDiskFile* file )
	{
		Bool success = true;

		RunInMainThread( [ file, &success ]() {
			if ( !file->Save() )
			{
				success = false;
			}
		} );

		return success;
	}

	void Unload()
	{
		if ( !m_wasLoaded ) // do not unload meshes that had been loaded previously
		{
			if ( CResource* res = GetResource() )
			{
				res->RemoveFromRootSet();
				if ( CDiskFile* file = res->GetFile() )
				{
					file->Unload();
				}
			}

			m_loadedResource = nullptr; // release the resource
		}
	}

	EJobResult Failed( const String& message )
	{
		Unload();
		SyncEvent( wxEVT_LOD_GENERATOR_JOB_ENDED, message );
		return JR_Failed;
	}

	EJobResult Succeeded()
	{
		Unload();
		SyncEvent( wxEVT_LOD_GENERATOR_JOB_ENDED, TXT("") );
		return JR_Finished;
	}

	virtual EJobResult Process()
	{
		SyncEvent( wxEVT_LOD_GENERATOR_JOB_STARTED, String::Printf( TXT("Processing: %s"), m_resourceToLoad.AsChar() ) );

		m_wasLoaded = GetResource() && GetResource()->GetFile()->IsLoaded();

		// Perform the loading
		if ( __super::Process() != JR_Finished )
		{
			return Failed( TXT("ERROR: cannot load file") );
		}

		if ( CMesh* mesh = Cast< CMesh >( GetResource() ) )
		{
			CDiskFile* file = mesh->GetFile();

			if ( !PrepareInMainThread( file ) )
			{
				return Failed( TXT( "ERROR: cannot checkout file" ) );
			}

			// Note: preset is not synchronized, it's assumed that it is only modified before issuing jobs
			String errorMsg;
			Bool success = GenerateLODsForMesh( mesh, m_dialog->m_currentPreset, errorMsg, 
				[ this ]( const String& msg ) {
					SyncEvent( wxEVT_LOD_GENERATOR_MESSAGE, msg );
				} );

			if ( !success )
			{
				return Failed( errorMsg );
			}

			if ( !SaveInMainThread( file ) )
			{
				return Failed( TXT( "ERROR: cannot save file" ) );
			}

			return Succeeded();
		}
		else
		{
			return Failed( TXT("ERROR: resource is not CMesh") );
		}
	}
};

BEGIN_EVENT_TABLE( CEdBatchLodGenerator, wxDialog )

	EVT_CHOICE( XRCID("Presets"), CEdBatchLodGenerator::OnPresetChanged )
	EVT_BUTTON( XRCID("SavePreset"), CEdBatchLodGenerator::OnSavePreset )
	EVT_BUTTON( XRCID("RemovePreset"), CEdBatchLodGenerator::OnRemovePreset )

	EVT_BUTTON( XRCID("AddLOD"), CEdBatchLodGenerator::OnAddLOD )
	EVT_BUTTON( XRCID("EditLOD"), CEdBatchLodGenerator::OnEditLOD )
	EVT_BUTTON( XRCID("RemoveLOD"), CEdBatchLodGenerator::OnRemoveLOD )
	EVT_LISTBOX( XRCID("LODList"), CEdBatchLodGenerator::OnLODSelected )
	EVT_LISTBOX_DCLICK( XRCID("LODList"), CEdBatchLodGenerator::OnEditLOD )
	EVT_TEXT( XRCID("FaceLimit"), CEdBatchLodGenerator::OnFaceLimitChanged )

	EVT_BUTTON( XRCID("Go"), CEdBatchLodGenerator::OnStartStop )
	EVT_BUTTON( XRCID("Close"), CEdBatchLodGenerator::OnClose )

	EVT_COMMAND( wxID_ANY, wxEVT_LOD_GENERATOR_JOB_STARTED, CEdBatchLodGenerator::OnJobStarted )
	EVT_COMMAND( wxID_ANY, wxEVT_LOD_GENERATOR_MESSAGE,     CEdBatchLodGenerator::OnJobMessage )
	EVT_COMMAND( wxID_ANY, wxEVT_LOD_GENERATOR_JOB_ENDED,   CEdBatchLodGenerator::OnJobEnded )

	EVT_UPDATE_UI( wxID_ANY, CEdBatchLodGenerator::OnUpdateUI )
END_EVENT_TABLE()


CEdBatchLodGenerator::CEdBatchLodGenerator( wxWindow* parent, const CContextMenuDir& contextMenuDir )
	: m_contextMenuDir( contextMenuDir )
	, m_running( false )
	, m_currentPresetDirty( false )
	, m_internalSet( false )
	, m_currentPresetIdx( -1 )
	, m_singleMeshToProcess( nullptr )
	, m_undoManager( nullptr )
{
	CommonInit( parent );
}

CEdBatchLodGenerator::CEdBatchLodGenerator( wxWindow* parent, CMesh* mesh, CEdUndoManager* undoManager )
	: m_contextMenuDir()
	, m_running( false )
	, m_currentPresetDirty( false )
	, m_internalSet( false )
	, m_currentPresetIdx( -1 )
	, m_singleMeshToProcess( mesh )
	, m_undoManager( undoManager )
{
	CommonInit( parent );

	XRCCTRL( *this, "ProgressPanel", wxPanel )->Hide();
	DoLayoutAdaptation();
}

void CEdBatchLodGenerator::CommonInit( wxWindow* parent )
{
	wxXmlResource::Get()->LoadDialog( this, parent, wxT("BatchLODGenerator") );

	m_presetsChoice = XRCCTRL( *this, "Presets",      wxChoice );
	m_savePreset    = XRCCTRL( *this, "SavePreset",   wxButton );
	m_removePreset  = XRCCTRL( *this, "RemovePreset", wxButton );
				    
	m_addLodBtn     = XRCCTRL( *this, "AddLOD",    wxButton );
	m_editLodBtn    = XRCCTRL( *this, "EditLOD",   wxButton );   
	m_removeLodBtn  = XRCCTRL( *this, "RemoveLOD", wxButton );   
	m_lodList       = XRCCTRL( *this, "LODList",   wxListBox );
				    
	m_faceLimitEdit = XRCCTRL( *this, "FaceLimit", wxTextCtrlEx );
				    
	m_progress      = XRCCTRL( *this, "Progress",  wxGauge );
	m_log           = XRCCTRL( *this, "Log",       wxListBox );
				    
	m_startStopBtn  = XRCCTRL( *this, "Go",        wxButton );
	m_closeBtn      = XRCCTRL( *this, "Close",     wxButton );

	m_editLodBtn->Disable();
	m_removeLodBtn->Disable();

	m_logFilePath = GFileManager->GetBaseDirectory() + TXT("BatchLODGenerator.log");

	LoadOptionsFromConfig();
}

CEdBatchLodGenerator::~CEdBatchLodGenerator()
{
	ASSERT( m_jobs.Empty() );
}

SLODPreset CEdBatchLodGenerator::GetDefaultPreset() const
{
	SLODPreset preset;
	preset.m_faceLimit = 300;
	preset.m_definitions.PushBack( SLODPresetDefinition( 0.5,   10, 0, false ) );
	preset.m_definitions.PushBack( SLODPresetDefinition( 0.25,  30, 0, false ) );
	preset.m_definitions.PushBack( SLODPresetDefinition( 0.125, 50, 0, false ) );
	preset.m_name = TXT("Default");
	return preset;
}

Int32 CEdBatchLodGenerator::FindPresetIndexByName( const String& name )
{
	auto presetIt = FindIf( m_presets.Begin(), m_presets.End(), 
		[ &name ]( const SLODPreset& preset ) { return preset.m_name == name; }
		);

	return ( presetIt != m_presets.End() ) ? presetIt - m_presets.Begin() : -1;
}

void CEdBatchLodGenerator::SetPresetDirty()
{
	m_currentPresetDirty = true;
	if ( m_currentPresetIdx != -1 )
	{
		m_presetsChoice->SetString( m_currentPresetIdx, ( m_presets[ m_currentPresetIdx ].m_name + TXT(" *") ).AsChar() );
	}
}

void CEdBatchLodGenerator::SetPresetClean()
{
	m_currentPresetDirty = false;
	if ( m_currentPresetIdx != -1 )
	{
		m_presetsChoice->SetString( m_currentPresetIdx, m_presets[ m_currentPresetIdx ].m_name.AsChar() );
	}
}

Bool CEdBatchLodGenerator::Execute()
{
	Bool result = ( ShowModal() == wxID_OK );
	SaveOptionsToConfig();
	return result;
}

void CEdBatchLodGenerator::OnStartStop( wxCommandEvent& event )
{
	if ( !m_running )
	{
		SaveOptionsToConfig(); // In case something goes wrong ;)
		UpdatePresetDisplay();
		GenerateLODs();
	}
	else
	{
		StopLODGeneration();
	}
}

void CEdBatchLodGenerator::OnClose( wxCommandEvent& event )
{
	EndDialog( wxID_CANCEL );
}

void CEdBatchLodGenerator::AfterPresetChanged()
{
	SetPresetClean(); // Either the previous preset is discarded or saved - it will be clean now

	m_currentPresetIdx = m_presetsChoice->GetSelection();

	if ( m_currentPresetIdx != -1 )
	{
		m_currentPreset = m_presets[ m_currentPresetIdx ];
	}

	UpdatePresetDisplay();
}

void CEdBatchLodGenerator::OnPresetChanged( wxCommandEvent& event )
{
	if ( m_currentPresetDirty && m_currentPresetIdx != -1 )
	{
		if ( YesNo( TXT("Save changes to the current preset?") ) )
		{
			m_presets[ m_currentPresetIdx ] = m_currentPreset;
			SaveOptionsToConfig();
		}
	}

	AfterPresetChanged();
}

void CEdBatchLodGenerator::OnSavePreset( wxCommandEvent& event )
{
	String name = m_currentPreset.m_name;
	String oldName = name;

	if ( InputBox( this, TXT("Save preset"), TXT("Preset name"), name ) )
	{
		m_currentPreset.m_name = name;

		Int32 presetIdx = FindPresetIndexByName( name );

		if ( presetIdx != -1 )
		{
			if ( presetIdx == m_currentPresetIdx )
			{
				m_presets[ presetIdx ] = m_currentPreset;
			}
			else
			{
				if ( YesNo( TXT("Preset with this name already exists. Overwrite?") ) )
				{
					m_presets[ presetIdx ] = m_currentPreset;
					m_presetsChoice->SetSelection( presetIdx );
					AfterPresetChanged();
				}
				else
				{
					return;
				}
			}
		}
		else
		{
			// Create new preset
			m_presets.PushBack( m_currentPreset );
			m_presets.Back().m_name = name;
	  		m_presetsChoice->Append( name.AsChar() );

			m_presetsChoice->SetSelection( m_presets.Size()-1 );
			AfterPresetChanged();
		}

		SetPresetClean();
		SaveOptionsToConfig();
	}
}

void CEdBatchLodGenerator::OnRemovePreset( wxCommandEvent& event )
{
	if ( YesNo( TXT("Are you sure to remove this preset?") ) )
	{
		ASSERT ( m_currentPresetIdx != -1 );
		m_presets.RemoveAt( m_currentPresetIdx );
		m_presetsChoice->Delete( m_currentPresetIdx );

		m_currentPresetIdx = m_presets.Empty() ? -1 : 0;

		m_presetsChoice->SetSelection( m_currentPresetIdx );
		AfterPresetChanged();

		SaveOptionsToConfig();
	}
}

void CEdBatchLodGenerator::UpdatePresetDisplay()
{
	TDynArray< SLODPresetDefinition >& definitions = m_currentPreset.m_definitions;

	Sort( 
		definitions.Begin(), definitions.End(),
		[]( const SLODPresetDefinition& d1, const SLODPresetDefinition& d2 ) { return d1.m_distance < d2.m_distance; }
	);

	m_lodList->Freeze();
	m_lodList->Clear();

	for ( Uint32 i=0; i<definitions.Size(); ++i )
	{
		m_lodList->Append(
			wxString::Format( TXT("LOD %i - dist: %3.1f, red: %3.3f, limit: %d%s%s"), 
				i+1, definitions[i].m_distance, definitions[i].m_reduction, definitions[i].m_chunkFaceLimit,
				definitions[i].m_recalculateNormals ? String::Printf(TXT(", recalculate normals (%d)"), (Int32)definitions[i].m_hardEdgeAngle).AsChar() : TXT(""),
				definitions[i].m_removeSkinning ? TXT(", no skinning") : TXT("")
				)
			);
	}

	m_lodList->Thaw();

	{
		Red::System::ScopedFlag< Bool > sc( m_internalSet = true, false );
		m_faceLimitEdit->SetValue( ToString( m_currentPreset.m_faceLimit ).AsChar() );	
	}
}

void CEdBatchLodGenerator::OnAddLOD( wxCommandEvent& event )
{
	TDynArray< SLODPresetDefinition >& definitions = m_currentPreset.m_definitions;

	SLODPresetDefinition definition;
	if ( CEdAddLODDialog( this ).Execute( definition )  )
	{
		SetPresetDirty();
		definitions.PushBack( definition );
		UpdatePresetDisplay();
	}
}

void CEdBatchLodGenerator::OnEditLOD( wxCommandEvent& event )
{
	TDynArray< SLODPresetDefinition >& definitions = m_currentPreset.m_definitions;

	int lodIdx = m_lodList->GetSelection();
	ASSERT ( lodIdx != -1 && lodIdx < static_cast<int>( definitions.Size() ) );

	SLODPresetDefinition& definition = definitions[ lodIdx ];
	if ( CEdAddLODDialog( this, true ).Execute( definition ) )
	{
		SetPresetDirty();
		UpdatePresetDisplay();
	}
}

void CEdBatchLodGenerator::OnRemoveLOD( wxCommandEvent& event )
{
	TDynArray< SLODPresetDefinition >& definitions = m_currentPreset.m_definitions;

	int lodIdx = m_lodList->GetSelection();
	ASSERT ( lodIdx != -1 && lodIdx < static_cast<int>( definitions.Size() ) );

	SetPresetDirty();
	definitions.RemoveAt( lodIdx );
	UpdatePresetDisplay();
}

void CEdBatchLodGenerator::OnLODSelected( wxCommandEvent& event )
{
}

void CEdBatchLodGenerator::OnFaceLimitChanged( wxCommandEvent& event )
{
	if ( !m_internalSet )
	{
		SetPresetDirty();
	}

	FromString( String( m_faceLimitEdit->GetValue() ), m_currentPreset.m_faceLimit );
}

void CEdBatchLodGenerator::OnUpdateUI( wxUpdateUIEvent& event )
{
	//SGarbageCollector::GetInstance().Tick();

	Bool presetSelected = m_currentPresetIdx != -1;
	Bool lodSelected    = m_lodList->GetSelection() != -1;

	m_presetsChoice->Enable( !m_running );
	m_savePreset   ->Enable( !m_running && ( m_currentPresetDirty || m_presetsChoice->IsEmpty() ) );
	m_removePreset ->Enable( !m_running && presetSelected );

	m_addLodBtn    ->Enable( !m_running );
	m_editLodBtn   ->Enable( !m_running && lodSelected );
	m_removeLodBtn ->Enable( !m_running && lodSelected );
	m_lodList      ->Enable( !m_running );
	m_faceLimitEdit->Enable( !m_running );

	m_closeBtn     ->Enable( !m_running );
}

bool CEdBatchLodGenerator::IsSupportedFile( CDiskFile* file ) const
{
	return file->GetDepotPath().EndsWith( ResourceExtension< CMesh >() );
}

void CEdBatchLodGenerator::ScanDir( CDirectory* dir )
{
	for ( CDiskFile* file : dir->GetFiles() )
	{
		if ( IsSupportedFile( file ) )
		{
			m_filesToProcess.PushBack( file->GetDepotPath() );
		}
	}

	for ( CDirectory* child : dir->GetDirectories() )
	{
		ScanDir( child );
	}

}

void CEdBatchLodGenerator::GenerateLODs()
{
	ASSERT ( m_jobs.Empty() );

	if ( m_singleMeshToProcess != nullptr )
	{
		// SYNCHRONOUS - directly given mesh

		Hide();

		GFeedback->BeginTask( TXT("Generating LODs"), false );
		{
			if ( m_undoManager )
			{
				CUndoMeshChunksChanged::CreateStep( *m_undoManager, m_singleMeshToProcess, TXT("generate LODs") );
			}

			String errorMsg;
			Bool success = GenerateLODsForMesh( m_singleMeshToProcess, m_currentPreset, errorMsg, 
				[ ]( const String& msg ) {
					LOG_EDITOR( msg.AsChar() );
				} );

			if ( !success )
			{
				GFeedback->ShowError( errorMsg.AsChar() );
			}
		}
		GFeedback->EndTask();

		EndModal( wxID_OK );
	}
	else
	{
		// ASYNCHRONOUS - multiple file from given directories

		GFeedback->BeginTask( TXT("Scanning directories"), false );

		m_filesToProcess.Clear();
		
		for ( auto dirI = m_contextMenuDir.GetDirs().Begin(); dirI != m_contextMenuDir.GetDirs().End(); ++dirI )
		{
			ScanDir( *dirI );
		}

		for ( auto fileI = m_contextMenuDir.GetFiles().Begin(); fileI != m_contextMenuDir.GetFiles().End(); ++fileI )
		{
			if ( IsSupportedFile( *fileI ) )
			{
				m_filesToProcess.PushBack( (*fileI)->GetDepotPath() );
			}
		}

		GFeedback->EndTask();

		m_startStopBtn->SetLabel( TXT("Stop") );

		m_log->Clear();
		GFileManager->DeleteFile( m_logFilePath );

		for ( auto pathI = m_filesToProcess.Begin(); pathI != m_filesToProcess.End(); ++pathI )
		{
			m_jobs.PushBack( new CJobGenerateLOD( this, *pathI ) );
		}

		m_running = true;

		for ( auto job : m_jobs )
		{
			SJobManager::GetInstance().Issue( job );
		}
	}
}

void CEdBatchLodGenerator::StopLODGeneration()
{
	for ( Int32 i = m_jobs.SizeInt()-1; i >= 0; --i )
	{
		ILoadJob* job = m_jobs[i];
		if ( !job->HasEnded() )
		{
			job->Cancel();
			job->Release();
			m_jobs.RemoveAt( i );
		}
	}

	m_startStopBtn->SetLabel( TXT("Stopping...") );
	m_startStopBtn->Disable();
}

void CEdBatchLodGenerator::Log( const wxString& message )
{
	if ( !message.IsEmpty() )
	{
		GFileManager->SaveStringToFile( m_logFilePath, String( message.c_str() ) + TXT("\r\n"), true );

		m_log->Append( message );
		m_log->SetFirstItem( m_log->GetCount()-1 ); // wx's EnsureVisible is not implemented under MSW :(
	}
}

void CEdBatchLodGenerator::OnJobStarted( wxCommandEvent& event )
{
	CJobGenerateLOD* job = static_cast< CJobGenerateLOD* >( event.GetClientData() );
	Log( event.GetString() );
}

void CEdBatchLodGenerator::OnJobMessage( wxCommandEvent& event )
{
	Log( event.GetString() );
}

void CEdBatchLodGenerator::OnJobEnded( wxCommandEvent& event )
{
	CJobGenerateLOD* job = static_cast< CJobGenerateLOD* >( event.GetClientData() );

	Log( event.GetString() );

	Bool jobRemoved = m_jobs.Remove( job );
	ASSERT ( jobRemoved );
	job->Release();

	if ( !m_filesToProcess.Empty() )
	{
		m_progress->SetValue( 100 - 100 * m_jobs.Size() / m_filesToProcess.Size() );
	}

	if ( m_jobs.Empty() )
	{
		// All jobs ended
		m_progress->SetValue( 0 );
		m_startStopBtn->SetLabel( TXT("Go!") );
		m_startStopBtn->Enable();
		m_running = false;

		Log( TXT("------ FINISHED ------") );
		wxMessageBox( TXT("Generating LODs finished"), TXT("Finished"), wxOK|wxICON_INFORMATION, this );
	}
}

/*virtual*/ 
void CEdBatchLodGenerator::LoadOptionsFromConfig() /*override*/
{
    CUserConfigurationManager& config = SUserConfigurationManager::GetInstance();

	m_presetsChoice->Freeze();
	m_presetsChoice->Clear();
	m_presets.Clear();

	{
		CConfigurationScopedPathSetter pathSetter( config, TXT("/BatchLODGenerator") );

		Int32 sizeX = config.Read( TXT("SizeX"),  GetSize().GetX() );
		Int32 sizeY = config.Read( TXT("SizeY"), GetSize().GetY() );
		SetSize( sizeX, sizeY );

		Int32 numOfPresets = config.Read( TXT("NumberOfPresets"), 0 );

		if ( numOfPresets != 0 )
		{
			Uint32 curPreset = config.Read( TXT("CurrentPreset"), 0 );

			for ( Int32 presetIdx = 0; presetIdx < numOfPresets; ++presetIdx )
			{
				String presetPath = String::Printf( TXT("Preset%i/"), presetIdx );

				SLODPreset preset;
				preset.m_name      = config.Read( presetPath + TXT("Name"),         TXT("") );
				preset.m_faceLimit = config.Read( presetPath + TXT("FaceLimit"),    0 );
				Int32 numOfLODs    = config.Read( presetPath + TXT("NumberOfLODs"), 0 );

				for ( Int32 lodIdx = 0; lodIdx < numOfLODs; ++lodIdx )
				{
					String lodPath = presetPath + String::Printf( TXT("LOD%i/"), lodIdx+1 );
					SLODPresetDefinition lod;
					lod.m_distance				= config.Read( lodPath + TXT("Distance"),  0.0f );
					lod.m_reduction				= config.Read( lodPath + TXT("Reduction"), 0.0f );
					lod.m_chunkFaceLimit		= config.Read( lodPath + TXT("FaceLimit"), 0.0f );
					lod.m_removeSkinning		= config.Read( lodPath + TXT("RemoveSkinning"), 0 ) != 0;
					lod.m_recalculateNormals	= config.Read( lodPath + TXT("RecalculateNormals"), 0 ) != 0;
					lod.m_hardEdgeAngle			= config.Read( lodPath + TXT("HardEdgeAngle"), 80.0f );
					lod.m_geometryImportance	= config.Read( lodPath + TXT("GeometryImportance"), 1.0f );
					lod.m_textureImportance		= config.Read( lodPath + TXT("TextureImportance"), 1.0f );
					lod.m_materialImportance	= config.Read( lodPath + TXT("MaterialImportance"), 1.0f );
					lod.m_groupImportance		= config.Read( lodPath + TXT("GroupImportance"), 1.0f );
					lod.m_vertexColorImportance	= config.Read( lodPath + TXT("VertexColorImportance"), 1.0f );
					lod.m_shadingImportance		= config.Read( lodPath + TXT("ShadingImportance"), 1.0f );
					preset.m_definitions.PushBack( lod );
				}

				m_presets.PushBack( preset );
			}

			// Fill up the preset choice drop-down

			for ( Uint32 presetIdx = 0; presetIdx < m_presets.Size(); ++presetIdx )
			{
				m_presetsChoice->Append( m_presets[ presetIdx ].m_name.AsChar() );
			}

			m_presetsChoice->SetSelection( curPreset );
			AfterPresetChanged();
		}
		else
		{
			// Prepare the default preset
			m_currentPreset = GetDefaultPreset();
		}

		m_currentPresetDirty = false;
	}

	m_presetsChoice->Thaw();

	SetPresetClean();
	UpdatePresetDisplay();
}

/*virtual*/ 
void CEdBatchLodGenerator::SaveOptionsToConfig() /*override*/
{
    CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();

	{
		CConfigurationScopedPathSetter pathSetter( config, TXT("/BatchLODGenerator") );

		config.Write( TXT("SizeX"),  GetSize().GetX() );
		config.Write( TXT("SizeY"), GetSize().GetY() );

		config.Write( TXT("NumberOfPresets"), m_presets.SizeInt() );
		if ( m_presets.SizeInt() > 0 )
		{
			config.Write( TXT("CurrentPreset"), m_currentPresetIdx );
			for ( Int32 presetIdx = 0; presetIdx < m_presets.SizeInt(); ++presetIdx )
			{
				String presetPath = String::Printf( TXT("Preset%i/"), presetIdx );

				const SLODPreset& preset = m_presets[ presetIdx ];
				config.Write( presetPath + TXT("Name"),         preset.m_name );
				config.Write( presetPath + TXT("FaceLimit"),    static_cast< Int32 >( preset.m_faceLimit ) );
				config.Write( presetPath + TXT("NumberOfLODs"), static_cast< Int32 >( preset.m_definitions.SizeInt() ) );

				for ( Int32 lodIdx = 0; lodIdx < preset.m_definitions.SizeInt(); ++lodIdx )
				{
					String lodPath = presetPath + String::Printf( TXT("LOD%i/"), lodIdx+1 );
					const SLODPresetDefinition& lod = preset.m_definitions[ lodIdx ];
					config.Write( lodPath + TXT("Distance"),  lod.m_distance  );
					config.Write( lodPath + TXT("Reduction"), lod.m_reduction );
					config.Write( lodPath + TXT("FaceLimit"), static_cast< Int32 >( lod.m_chunkFaceLimit ) );
					config.Write( lodPath + TXT("RemoveSkinning"), lod.m_removeSkinning );
					config.Write( lodPath + TXT("RecalculateNormals"), lod.m_recalculateNormals );
					config.Write( lodPath + TXT("HardEdgeAngle"), static_cast< Int32 >( lod.m_hardEdgeAngle ) );
					config.Write( lodPath + TXT("GeometryImportance"), lod.m_geometryImportance );
					config.Write( lodPath + TXT("TextureImportance"), lod.m_textureImportance );
					config.Write( lodPath + TXT("MaterialImportance"), lod.m_materialImportance );
					config.Write( lodPath + TXT("GroupImportance"), lod.m_groupImportance );
					config.Write( lodPath + TXT("VertexColorImportance"), lod.m_vertexColorImportance );
					config.Write( lodPath + TXT("ShadingImportance"), lod.m_shadingImportance );
				}
			}
		}
	}
}