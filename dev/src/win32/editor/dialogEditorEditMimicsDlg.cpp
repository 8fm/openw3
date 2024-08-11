// Copyright © 2013 CD Projekt Red. All Rights Reserved.

#include "build.h"
#include "dialogEditorEditMimicsDlg.h"
#include "dialogEditor.h"
#include <memory>

#include "../../common/core/depot.h"
#include "../../common/engine/mimicFac.h"
#include "../../common/engine/mimicComponent.h"

// =================================================================================================
namespace {
// =================================================================================================

// functions in this namespace
String GetAbsPathFaceEdDir();
String GetAbsPathFaceEdApp();
Bool ReimportMimicFace( CMimicFace* mimicFace );
Bool EditImportFileInFaceEd( const CMimicFace* mimicFace );

/*
Returns absolute path to FaceEd directory.

\return Absolute path to FaceEd directory.
*/
String GetAbsPathFaceEdDir()
{
	String faceEdPath;
	GDepot->GetAbsolutePath( faceEdPath );
	faceEdPath = faceEdPath.StringBefore( TXT( "\\" ), true );
	faceEdPath = faceEdPath.StringBefore( TXT( "\\" ), true );
	faceEdPath += TXT( "\\bin\\tools\\faceed" );
	
	return faceEdPath;
}

/*
Returns absolute path to FaceEd application.

\return Absolute path to FaceEd application.
*/
String GetAbsPathFaceEdApp()
{
	String faceEdPath = GetAbsPathFaceEdDir();
	faceEdPath += TXT( "\\faceed.exe" );

	return faceEdPath;
}

/*
Reimports mimic face.

\param mimicFace Mimic face to reimport. Must not be nullptr.
\return True - mimic face reimported successfully. False - otherwise.
Possible causes:
1. Couldn't check out mimic face file.
2. Couldn't find suitable importer.
3. Couldn't reimport mimic face file.
4. Couldn't save reimported mimic face file.
*/
Bool ReimportMimicFace( CMimicFace* mimicFace )
{
	ASSERT( mimicFace );

	if( mimicFace->MarkModified() )
	{
		// get suitable importer
		CFilePath importFilePath( mimicFace->GetImportFile() );
		IImporter* importer = IImporter::FindImporter( mimicFace->GetClass(), importFilePath.GetExtension() );

		if( importer )
		{
			IImporter::ImportOptions options;
			options.m_existingResource = mimicFace;
			options.m_parentObject = mimicFace->GetParent();
			options.m_sourceFilePath = mimicFace->GetImportFile();
			
			importer->PrepareForImport( importFilePath.GetFileName(), options );
			CResource* imported = importer->DoImport( options );

			if( imported )
			{
				// old resource and imported resource should point to the same location
				ASSERT( imported == mimicFace );

				if( imported->Save() )
				{
					return true;
				}
			}
		}
	}

	return false;
}

/*
Starts FaceEd with import file of specified mimics face.

\param mimicFace Mimic face whose import file is to be edited in FaceEd. Must not be nullptr.
\return True - success. False - couldn't start FaceEd.
*/
Bool EditImportFileInFaceEd( const CMimicFace* mimicFace )
{
	ASSERT( mimicFace );

	const String appName = GetAbsPathFaceEdApp();
	const Uint32 appNameLen = appName.GetLength();
	const String& importFileName = mimicFace->GetImportFile();

	// Construct command line.
	std::unique_ptr< wchar_t[] > cmdLine( new wchar_t[ appNameLen + 1 + importFileName.GetLength() + 1 ] ); // // + 1 for space, + 1 for null
	wcscpy( cmdLine.get(), appName.AsChar() );
	cmdLine[ appNameLen ] = L' ';
	wcscpy( cmdLine.get() + appNameLen + 1, importFileName.AsChar() );

	STARTUPINFO si;
	ZeroMemory( &si, sizeof( si ) );
	si.cb = sizeof( si );

	PROCESS_INFORMATION pi;
	ZeroMemory( &pi, sizeof( pi ) );

	BOOL status = CreateProcessW( appName.AsChar(), cmdLine.get(), NULL, NULL, FALSE, 0, NULL, GetAbsPathFaceEdDir().AsChar(), &si, &pi );

	if( status != 0 )
	{
		// Close process and thread handles as we won't need them.
		CloseHandle( pi.hProcess );
		CloseHandle( pi.hThread );
		return true;
	}
	else
	{
		// Couldn't start FaceEd.
		return false;
	}
}

// =================================================================================================
} // unnamed namespace
// =================================================================================================

wxBEGIN_EVENT_TABLE( CEdEditMimicsDlg, wxDialog )

	EVT_COMBOBOX( XRCID("comboActors"), CEdEditMimicsDlg::OnActorSelected )
	EVT_BUTTON( XRCID("btnClose"), CEdEditMimicsDlg::OnBtnClose )
	EVT_CLOSE( CEdEditMimicsDlg::OnClose )

	EVT_BUTTON( XRCID("btnEditCustomMimics"), CEdEditMimicsDlg::OnBtnEditCustomMimics )
	EVT_BUTTON( XRCID("btnEditCategoryMimics"), CEdEditMimicsDlg::OnBtnEditCategoryMimics )
	EVT_BUTTON( XRCID("btnEditCommonMimics"), CEdEditMimicsDlg::OnBtnEditCommonMimics )

	EVT_BUTTON( XRCID("btnReimportCustomMimics"), CEdEditMimicsDlg::OnBtnReimportCustomMimics )
	EVT_BUTTON( XRCID("btnReimportCategoryMimics"), CEdEditMimicsDlg::OnBtnReimportCategoryMimics )
	EVT_BUTTON( XRCID("btnReimportCommonMimics"), CEdEditMimicsDlg::OnBtnReimportCommonMimics )

wxEND_EVENT_TABLE()

/*
Ctor.

\param sceneEditor Scene editor that is a parent of this dialog. Must not be nulltpr.
*/
CEdEditMimicsDlg::CEdEditMimicsDlg( CEdSceneEditor* sceneEditor )
: m_sceneEditor( sceneEditor )
, m_textCtrlCustomMimicsImportFile( nullptr )
, m_textCtrlCustomMimicsFacFile( nullptr )
, m_btnEditCustomMimics( nullptr )
, m_btnReimportCustomMimics( nullptr )
, m_textCtrlCategoryMimicsImportFile( nullptr )
, m_textCtrlCategoryMimicsFacFile( nullptr )
, m_btnEditCategoryMimics( nullptr )
, m_btnReimportCategoryMimics( nullptr )
, m_textCtrlCommonMimicsImportFile( nullptr )
, m_textCtrlCommonMimicsFacFile( nullptr )
, m_btnEditCommonMimics( nullptr )
, m_btnReimportCommonMimics( nullptr )
, m_comboActors( nullptr )
{
	ASSERT( sceneEditor );

	wxXmlResource::Get()->LoadDialog( this, sceneEditor, "DialogEditMimics" );
	SetupControls();
	UpdateUI();
}

/*
Dtor.
*/
CEdEditMimicsDlg::~CEdEditMimicsDlg()
{}

/*
Performs initial setup of dialog controls.
*/
void CEdEditMimicsDlg::SetupControls()
{
	m_comboActors = XRCCTRL( *this, "comboActors", wxComboBox );
	ASSERT( m_comboActors );

	// get custom mimics controls
	m_textCtrlCustomMimicsImportFile = XRCCTRL( *this, "textCtrlCustomMimicsImportFile", wxTextCtrl );
	m_btnEditCustomMimics = XRCCTRL( *this, "btnEditCustomMimics", wxButton );
	m_textCtrlCustomMimicsFacFile = XRCCTRL( *this, "textCtrlCustomMimicsFacFile", wxTextCtrl );
	m_btnReimportCustomMimics = XRCCTRL( *this, "btnReimportCustomMimics", wxButton );

	ASSERT( m_textCtrlCustomMimicsImportFile );
	ASSERT( m_btnEditCustomMimics );
	ASSERT( m_textCtrlCustomMimicsFacFile );
	ASSERT( m_btnReimportCustomMimics );

	// get category mimics controls
	m_textCtrlCategoryMimicsImportFile = XRCCTRL( *this, "textCtrlCategoryMimicsImportFile", wxTextCtrl );
	m_btnEditCategoryMimics = XRCCTRL( *this, "btnEditCategoryMimics", wxButton );
	m_textCtrlCategoryMimicsFacFile = XRCCTRL( *this, "textCtrlCategoryMimicsFacFile", wxTextCtrl );
	m_btnReimportCategoryMimics = XRCCTRL( *this, "btnReimportCategoryMimics", wxButton );

	ASSERT( m_textCtrlCategoryMimicsImportFile );
	ASSERT( m_btnEditCategoryMimics );
	ASSERT( m_textCtrlCategoryMimicsFacFile );
	ASSERT( m_btnReimportCategoryMimics );

	// get common mimics controls
	m_textCtrlCommonMimicsImportFile = XRCCTRL( *this, "textCtrlCommonMimicsImportFile", wxTextCtrl );
	m_btnEditCommonMimics = XRCCTRL( *this, "btnEditCommonMimics", wxButton );
	m_textCtrlCommonMimicsFacFile = XRCCTRL( *this, "textCtrlCommonMimicsFacFile", wxTextCtrl );
	m_btnReimportCommonMimics = XRCCTRL( *this, "btnReimportCommonMimics", wxButton );
	
	ASSERT( m_textCtrlCommonMimicsImportFile );
	ASSERT( m_btnEditCommonMimics );
	ASSERT( m_textCtrlCommonMimicsFacFile );
	ASSERT( m_btnReimportCommonMimics );

	// Fill combobox with names of actors.
	const THashMap< CName, THandle< CEntity > >& actorMap = m_sceneEditor->OnEditMimicsDialog_GetActorMap();
	for( auto it = actorMap.Begin(), end = actorMap.End(); it != end; ++it )
	{
		m_comboActors->Append( it->m_first.AsChar() );
	}

	// Select first actor.
	if( !m_comboActors->IsListEmpty() )
	{
		m_comboActors->SetSelection( 0 );
	}
}

/*
Updates UI.
*/
void CEdEditMimicsDlg::UpdateUI()
{
	const CMimicFace* customMimics = nullptr;
	const CMimicFace* categoryMimics = nullptr;
	const CMimicFace* commonMimics = nullptr;

	CActor* actor = GetSelectedActor();
	if( actor )
	{
		CMimicComponent* mimicComponent = actor->GetMimicComponent();
		if( mimicComponent )
		{
			CExtendedMimics exMimics = mimicComponent->GetExtendedMimics();
			customMimics = exMimics.GetCustomMimics();
			categoryMimics = exMimics.GetCategoryMimics();
			commonMimics = exMimics.GetCommonMimics();
		}
	}

	UpdateCustomMimicsUI( customMimics );
	UpdateCategoryMimicsUI( categoryMimics );
	UpdateCommonMimicsUI( commonMimics );
}

/*
Updates custom mimics UI.

\param customMimics Custom mimic face. May be nullptr.
*/
void CEdEditMimicsDlg::UpdateCustomMimicsUI( const CMimicFace* customMimics )
{
	if( customMimics )
	{
		EnableCustomMimicsControls( true );
		m_textCtrlCustomMimicsFacFile->SetValue( customMimics->GetFile()->GetDepotPath().AsChar() );
		m_textCtrlCustomMimicsImportFile->SetValue( customMimics->GetImportFile().AsChar() );
	}
	else
	{
		EnableCustomMimicsControls( false );
		m_textCtrlCustomMimicsFacFile->SetValue( "" );
		m_textCtrlCustomMimicsImportFile->SetValue( "" );
	}
}

/*
Updates category mimics UI.

\param categoryMimics Category mimic face. May be nullptr.
*/
void CEdEditMimicsDlg::UpdateCategoryMimicsUI( const CMimicFace* categoryMimics )
{
	if( categoryMimics )
	{
		EnableCategoryMimicsControls( true );
		m_textCtrlCategoryMimicsFacFile->SetValue( categoryMimics->GetFile()->GetDepotPath().AsChar() );
		m_textCtrlCategoryMimicsImportFile->SetValue( categoryMimics->GetImportFile().AsChar() );
	}
	else
	{
		EnableCategoryMimicsControls( false );
		m_textCtrlCategoryMimicsFacFile->SetValue( "" );
		m_textCtrlCategoryMimicsImportFile->SetValue( "" );
	}
}

/*
Updates common mimics UI.

\param commonMimics Common mimic face. May be nullptr.
*/
void CEdEditMimicsDlg::UpdateCommonMimicsUI( const CMimicFace* commonMimics )
{
	if( commonMimics )
	{
		EnableCommonMimicsControls( true );
		m_textCtrlCommonMimicsFacFile->SetValue( commonMimics->GetFile()->GetDepotPath().AsChar() );
		m_textCtrlCommonMimicsImportFile->SetValue( commonMimics->GetImportFile().AsChar() );
	}
	else
	{
		EnableCommonMimicsControls( false );
		m_textCtrlCommonMimicsFacFile->SetValue( "" );
		m_textCtrlCommonMimicsImportFile->SetValue( "" );
	}
}

/*
Returns actor that is currently selected in actors combobox.

\return Actor that is currently selected in actors combobox. Nullptr may be returned.
*/
CActor* CEdEditMimicsDlg::GetSelectedActor() const
{
	if( m_comboActors->IsListEmpty() )
	{
		return nullptr;
	}

	CName actorCName( m_comboActors->GetStringSelection().wc_str() );
	CActor* actor = m_sceneEditor->OnEditMimicsDialog_GetSceneActor( actorCName );
	return actor;
}

/*
Gets selected actor custom mimics.

\return Selected actor custom mimics. Nullptr may be returned (no actor, actor has no mimic component, no custom mimics set).
*/
CMimicFace* CEdEditMimicsDlg::GetSelectedActorCustomMimics() const
{
	CMimicFace* customMimics = nullptr;
	CActor* actor = GetSelectedActor();
	if( actor )
	{
		CMimicComponent* mimicComponent = actor->GetMimicComponent();
		if( mimicComponent )
		{
			CExtendedMimics exMimics = mimicComponent->GetExtendedMimics();
			customMimics = exMimics.GetCustomMimics();
		}
	}

	return customMimics;
}

/*
Gets selected actor category mimics.

\return Selected actor category mimics. Nullptr may be returned (no actor, actor has no mimic component, no category mimics set).
*/
CMimicFace* CEdEditMimicsDlg::GetSelectedActorCategoryMimics() const
{
	CMimicFace* categoryMimics = nullptr;
	CActor* actor = GetSelectedActor();
	if( actor )
	{
		CMimicComponent* mimicComponent = actor->GetMimicComponent();
		if( mimicComponent )
		{
			CExtendedMimics exMimics = mimicComponent->GetExtendedMimics();
			categoryMimics = exMimics.GetCategoryMimics();
		}
	}

	return categoryMimics;
}

/*
Gets selected actor common mimics.

\return Selected actor common mimics. Nullptr may be returned (no actor, actor has no mimic component, no common mimics set).
*/
CMimicFace* CEdEditMimicsDlg::GetSelectedActorCommonMimics() const
{
	CMimicFace* commonMimics = nullptr;
	CActor* actor = GetSelectedActor();
	if( actor )
	{
		CMimicComponent* mimicComponent = actor->GetMimicComponent();
		if( mimicComponent )
		{
			CExtendedMimics exMimics = mimicComponent->GetExtendedMimics();
			commonMimics = exMimics.GetCommonMimics();
		}
	}

	return commonMimics;
}

/*
Enables or disables custom mimics controls.

\param state True - controls are to be enabled. False - controls are to be disabled.
*/
void CEdEditMimicsDlg::EnableCustomMimicsControls( Bool state )
{
	m_btnEditCustomMimics->Enable( state );
	m_btnReimportCustomMimics->Enable( state );
}

/*
Enables or disables category mimics controls.

\param state True - controls are to be enabled. False - controls are to be disabled.
*/
void CEdEditMimicsDlg::EnableCategoryMimicsControls( Bool state )
{
	m_btnEditCategoryMimics->Enable( state );
	m_btnReimportCategoryMimics->Enable( state );
}

/*
Enables or disables common mimics controls.

\param state True - controls are to be enabled. False - controls are to be disabled.
*/
void CEdEditMimicsDlg::EnableCommonMimicsControls( Bool state )
{
	m_btnEditCommonMimics->Enable( state );
	m_btnReimportCommonMimics->Enable( state );
}

/*
Called when actor is selected in actor combobox.
*/
void CEdEditMimicsDlg::OnActorSelected( wxCommandEvent& event )
{
	UpdateUI();
}

/*
Handles "Close" button.
*/
void CEdEditMimicsDlg::OnBtnClose( wxCommandEvent& event )
{
	Close( true );
}

/*
Handles close event.
*/
void CEdEditMimicsDlg::OnClose( wxCloseEvent& event )
{
	Destroy();
}

/*
Handles "edit custom mimics" button.
*/
void CEdEditMimicsDlg::OnBtnEditCustomMimics( wxCommandEvent& event )
{
	CMimicFace* customMimics = GetSelectedActorCustomMimics();
	ASSERT( customMimics ); // otherwise "edit custom mimics" button is disabled so we should not be here

	EditImportFileInFaceEd( customMimics );
}

/*
Handles "edit category mimics" button.
*/
void CEdEditMimicsDlg::OnBtnEditCategoryMimics( wxCommandEvent& event )
{
	CMimicFace* categoryMimics = GetSelectedActorCategoryMimics();
	ASSERT( categoryMimics ); // otherwise "edit category mimics" button is disabled so we should not be here

	EditImportFileInFaceEd( categoryMimics );
}

/*
Handles "edit common mimics" button.
*/
void CEdEditMimicsDlg::OnBtnEditCommonMimics( wxCommandEvent& event )
{
	CMimicFace* commonMimics = GetSelectedActorCommonMimics();
	ASSERT( commonMimics ); // otherwise "edit common mimics" button is disabled so we should not be here

	EditImportFileInFaceEd( commonMimics );
}

/*
Handles "reimport custom mimics" button.
*/
void CEdEditMimicsDlg::OnBtnReimportCustomMimics( wxCommandEvent& event )
{
	CMimicFace* customMimics = GetSelectedActorCustomMimics();
	ASSERT( customMimics ); // otherwise "reimport custom mimics" button is disabled so we should not be here

	Bool reimported = ReimportMimicFace( customMimics );
	if( reimported )
	{
		wxMessageBox("Custom mimics reimported.");
	}
	else
	{
		wxMessageBox("Failed to reimport custom mimics.");
	}
}

/*
Handles "reimport category mimics" button.
*/
void CEdEditMimicsDlg::OnBtnReimportCategoryMimics( wxCommandEvent& event )
{
	CMimicFace* categoryMimics = GetSelectedActorCategoryMimics();
	ASSERT( categoryMimics ); // otherwise "reimport category mimics" button is disabled so we should not be here

	Bool reimported = ReimportMimicFace( categoryMimics );
	if( reimported )
	{
		wxMessageBox("Category mimics reimported.");
	}
	else
	{
		wxMessageBox("Failed to reimport category mimics.");
	}
}

/*
Handles "reimport common mimics" button.
*/
void CEdEditMimicsDlg::OnBtnReimportCommonMimics( wxCommandEvent& event )
{
	CMimicFace* commonMimics = GetSelectedActorCommonMimics();
	ASSERT( commonMimics ); // otherwise "reimport custom mimics" button is disabled so we should not be here

	Bool reimported = ReimportMimicFace( commonMimics );
	if( reimported )
	{
		wxMessageBox("Common mimics reimported.");
	}
	else
	{
		wxMessageBox("Failed to reimport common mimics.");
	}
}
