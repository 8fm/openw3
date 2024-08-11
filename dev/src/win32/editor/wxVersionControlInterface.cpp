#include "build.h"
#include "../../common/engine/inputDeviceManager.h"
#include "../../common/engine/inputEditorInterface.h"
#include "wxVersionControlInterface.h"

#define ERROR_NOT_OPERATIONAL "Version control system is not operational; try using some other version control system"
#define ERROR_DOUBLE_CHECKOUT "File is already checked out"
#define ERROR_NOT_CHECKED_OUT "File wasn't checked out, therefore cannot be saved/submitted/reverted."
#define ERROR_CHECKOUT "Failed to check out this file."
#define ERROR_SUBMIT "Failed to submit this file."
#define ERROR_LOCAL "This file is local. You can't use version control operations."
#define ERROR_LOADED "This file is already loaded and cannot be deleted."
#define ERROR_DELETE "Failed to delete this file."
#define ERROR_SYNC_REQUIRED "There is a new version of this file, sync before proceding."

#define QUESTION_CHECKOUT "You want to edit file, that wasn't checked out. You won't be able to save it. Do you wish to check out it first?"
#define QUESTION_CONNECTION "It seems that connection with P4 cannot be established. This may cause tools that are dependant of version control to be not responsive. Do you wish to disable version control?"
#define QUESTION_REVERT "Discard your changes to the selected objects?"
#define QUESTION_FORCE_SYNC "File cannot be synce, because it is writable/checked out. Do you wish to force sync?"

class CScopedTemporaryUnfullscreenization
{
public:
	CScopedTemporaryUnfullscreenization()
	{
		m_refullscreenize = wxTheFrame->IsInFullscreenMode();
		Toggle();
	}

	~CScopedTemporaryUnfullscreenization()
	{
		Toggle();
	}

	void Toggle()
	{
		if ( m_refullscreenize )
		{
			wxTheFrame->ToggleFullscreen();
		}
	}

private:
	Bool	m_refullscreenize;
};

void CEdVersionControlInterface::OnNotOperational()
{
	CScopedTemporaryUnfullscreenization nization;
	wxMessageDialog dialog(0, wxT(ERROR_NOT_OPERATIONAL), wxT("Error"), wxOK | wxICON_ERROR );
	dialog.ShowModal();
}

Int32 CEdVersionControlInterface::OnNoConnection()
{
	CScopedTemporaryUnfullscreenization nization;
	wxMessageDialog dialog(0, wxT(QUESTION_CONNECTION), wxT("Question"), wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION);
	if ( dialog.ShowModal() == wxID_YES )
	{
		return SC_OK;
	}
	else
	{
		return SC_CANCEL;
	}
}

Bool CEdVersionControlInterface::OnSaveFailure( CDiskFile &file )
{ 
	CScopedTemporaryUnfullscreenization nization;
	CEdSaveDialog dialog( NULL, file.GetAbsolutePath() );
	switch ( dialog.ShowModal() )
	{
	case SC_CHECK_OUT:
		if ( file.CheckOut() )
		{
			return file.Save( file.GetAbsolutePath() );
		}
		return false;
	case SC_SAVE_AS:
		return file.Save( dialog.GetPath() );
	case SC_OVERWRITE:
		file.Overwrite();
		return file.Save( file.GetAbsolutePath() );
	default:
		return false;
	}
}

Int32 CEdVersionControlInterface::OnParallelUsers( const TDynArray< CDiskFile* > &fileList, const TDynArray< String > &users, Bool exclusiveAccess )
{
	if ( users.Empty() )
		return SC_OK;

	String question = TXT("Files:\n");
	for ( Uint32 i = 0, n = fileList.Size(); i < n; ++i )
	{
		question += fileList[ i ]->GetDepotPath() + TXT("\n");
	}

	if ( exclusiveAccess )
	{
		question += TXT( "cannot be checked out, because it is already checked out by:\n" );
	}
	else
	{
		question += TXT( "allready checked out. Check out non-exclusively?\nCurrent users:\n" );
	}

	for ( Uint32 i = 0; i < users.Size(); i++ ){
		question += users[i] + TXT("\n");
	}

	CScopedTemporaryUnfullscreenization nization;
	
	wxMessageDialog dialog( 0, question.AsChar(), wxT("Error"), exclusiveAccess ? wxOK | wxICON_ERROR  : wxYES_NO | wxICON_WARNING );
	Int32 dialogResult = dialog.ShowModal();
	if ( !exclusiveAccess && dialogResult == wxID_YES )
	{
		return SC_OK;
	}
	return SC_CANCEL;
}

Int32 CEdVersionControlInterface::OnParallelUsers( CDiskFile &file, const TDynArray< String > &users, Bool exclusiveAccess )
{
	if ( users.Empty() )
		return SC_OK;

	String question =
		file.GetDepotPath() +
		(exclusiveAccess
			? TXT( "\ncannot be checked out, because it is already checked out by:\n" )
			: TXT( "\allready checked out. Check out non-exclusively?\nCurrent users:\n" )
		);
	for ( Uint32 i = 0; i < users.Size(); i++ ){
		question += users[i];
		question += TXT("\n");
	}

	CScopedTemporaryUnfullscreenization nization;

	wxMessageDialog dialog( 0, question.AsChar(), wxT("Error"), exclusiveAccess ? wxOK | wxICON_ERROR  : wxYES_NO | wxICON_WARNING );
	Int32 dialogResult = dialog.ShowModal();
	if ( !exclusiveAccess && dialogResult == wxID_YES )
	{
		return SC_OK;
	}
	return SC_CANCEL;
}

//void CEdVersionControlInterface::OnCaptureLost(wxMouseCaptureLostEvent &event)
//{
//	LOG_EDITOR(TXT("Lost capture"));
//}

Int32 CEdVersionControlInterface::OnCheckOutRequired( const String &path, const TDynArray< String > &users )
{
	String question = TXT("You want to edit ") + path + TXT(" that wasn't checked out. You won't be able to save it. Do you wish to check out it first?");

	CScopedTemporaryUnfullscreenization nization;

	wxMessageDialog dialog(0,question.AsChar(), wxT("Question"), wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION);

	RECT clipCursorRect;
	::GetClipCursor( &clipCursorRect );
	::ClipCursor( NULL );


	IInputDeviceManager* inputDeviceManager = GEngine ? GEngine->GetInputDeviceManager() : nullptr; 

	if ( inputDeviceManager && inputDeviceManager->GetEditorInterface() )
	{
		inputDeviceManager->GetEditorInterface()->SetAssertHookInputCaptureOverride( true );
	}
	
	Int32 res = dialog.ShowModal();

	ClipCursor( &clipCursorRect );

	if ( inputDeviceManager && inputDeviceManager->GetEditorInterface() )
	{
		inputDeviceManager->GetEditorInterface()->SetAssertHookInputCaptureOverride( false );
	}

	return res == wxID_YES ? SC_OK : SC_CANCEL;
}

void CEdVersionControlInterface::OnDoubleCheckOut()
{
	CScopedTemporaryUnfullscreenization nization;
	wxMessageDialog dialog(0, wxT(ERROR_DOUBLE_CHECKOUT), wxT("Error"), wxOK | wxICON_ERROR);
	dialog.ShowModal();
}

void CEdVersionControlInterface::OnNotEdited( const CDiskFile &file )
{
	CScopedTemporaryUnfullscreenization nization;
	String error = file.GetFileName() + TXT(" is not checked out or marked for addition/deletion, therefore cannot be reverted");
	wxMessageDialog dialog(0, error.AsChar(), wxT("Error"), wxOK | wxICON_ERROR);
	dialog.ShowModal();
}

Int32 CEdVersionControlInterface::OnSubmit( String &description, CDiskFile &file )
{
	TDynArray< CDiskFile * > files;
	TSet< CDiskFile * > chosen;
	files.PushBack( &file );
	chosen.Insert( &file );

	CScopedTemporaryUnfullscreenization nization;
	CEdSubmitDialog dialog( NULL, description, files, chosen );
	if ( dialog.ShowModal() != wxID_OK )
	{
		return SC_CANCEL;
	}
	return SC_OK;
}

Int32 CEdVersionControlInterface::OnMultipleSubmit(String &description, const TDynArray< CDiskFile * > &files, 
											   TSet< CDiskFile * > &chosen)
{
	CScopedTemporaryUnfullscreenization nization;
	CEdSubmitDialog dialog( NULL, description, files, chosen );
	if ( dialog.ShowModal() != wxID_OK )
		return SC_CANCEL;
	return SC_OK;
}

/*Int32 CEdVersionControlInterface::OnMultipleRevert()
{
wxMessageDialog dialog(0, wxT(QUESTION_REVERT), wxT("Question"), wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION);
if ( dialog.ShowModal() == wxID_YES )
{
return SC_OK;
}
else
{
return SC_CANCEL;
}*/

void CEdVersionControlInterface::OnFailedCheckOut()
{
	CScopedTemporaryUnfullscreenization nization;
	wxMessageDialog dialog(0, wxT(ERROR_CHECKOUT), wxT("Error"), wxOK | wxICON_ERROR);
	dialog.ShowModal();
}

void CEdVersionControlInterface::OnFailedSubmit()
{
	CScopedTemporaryUnfullscreenization nization;
	wxMessageDialog dialog(0, wxT(ERROR_SUBMIT), wxT("Error"), wxOK | wxICON_ERROR);
	dialog.ShowModal();	
}

void CEdVersionControlInterface::OnLocalFile()
{
	CScopedTemporaryUnfullscreenization nization;
	wxMessageDialog dialog(0, wxT(ERROR_LOCAL), wxT("Error"), wxOK | wxICON_ERROR);
	dialog.ShowModal();
}

void CEdVersionControlInterface::OnFailedDelete()
{
	CScopedTemporaryUnfullscreenization nization;
	wxMessageDialog dialog(0, wxT(ERROR_DELETE), wxT("Error"), wxOK | wxICON_ERROR);
	dialog.ShowModal();
}

void CEdVersionControlInterface::OnLoadedDelete()
{
	CScopedTemporaryUnfullscreenization nization;
	wxMessageDialog dialog(0, wxT(ERROR_LOADED), wxT("Error"), wxOK | wxICON_ERROR);
	dialog.ShowModal();
}

void CEdVersionControlInterface::OnSyncRequired()
{
	CScopedTemporaryUnfullscreenization nization;
	wxMessageDialog dialog(0, wxT(ERROR_SYNC_REQUIRED), wxT("Error"), wxOK | wxICON_ERROR);
	dialog.ShowModal();
}

Int32 CEdVersionControlInterface::OnSyncFailed()
{
	CScopedTemporaryUnfullscreenization nization;
	wxMessageDialog dialog(0, wxT(QUESTION_FORCE_SYNC), wxT("Question"), wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION);
	if ( dialog.ShowModal() == wxID_YES )
	{
		return SC_FORCE;
	}
	else
	{
		return SC_CANCEL;
	}	
}

#define QUESTION_DELETE "You are about to delete a file. Are you sure?"
#define QUESTION_MODIFIED_DELETE "You want to delete file, that was modified. Are you sure?"

Bool CEdVersionControlInterface::OnConfirmModifiedDelete()
{
	CScopedTemporaryUnfullscreenization nization;
	wxMessageDialog dialog( 0, wxT(QUESTION_MODIFIED_DELETE), wxT("Question"), wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION );
	if ( dialog.ShowModal() == wxID_NO )
		return false;
	return true;
}

Bool CEdVersionControlInterface::OnConfirmDelete()
{
	CScopedTemporaryUnfullscreenization nization;
	wxMessageDialog dialog( 0, wxT(QUESTION_DELETE), wxT("Question"), wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION );
	if ( dialog.ShowModal() == wxID_NO )
		return false;
	return true;
}

void CEdVersionControlInterface::OnFileLog(CDiskFile &file, TDynArray< THashMap< String, String > > &history )
{
	CScopedTemporaryUnfullscreenization nization;
	CEdFileHistoryDialog dialog( NULL, file.GetFileName(), history );
	dialog.ShowModal();
}

#define FILES_IDENTICAL "Files are identical."

void CEdVersionControlInterface::OnFilesIdentical()
{
	CScopedTemporaryUnfullscreenization nization;
	wxMessageDialog dialog(0, wxT(FILES_IDENTICAL), wxT("Warning"), wxOK | wxICON_WARNING);
	dialog.ShowModal();
}

Bool CEdVersionControlInterface::OnConfirmRevert() 
{ 
	CScopedTemporaryUnfullscreenization nization;
	wxMessageDialog dialog( 0, wxT(QUESTION_REVERT), wxT("Question"), wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION );
	if ( dialog.ShowModal() == wxID_NO )
		return false;
	return true;
}
