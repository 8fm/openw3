#include "build.h"

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

void CVersionControlInterface::OnNotOperational()
{
	ERR(TXT2(ERROR_NOT_OPERATIONAL));
}

Int CVersionControlInterface::OnNoConnection()
{
	ERR(TXT2(QUESTION_CONNECTION) TXT(" y/n:"));
	if ( m_silentMode )
	{
		ERR( TXT("Silent mode, default: No") );
		return SC_CANCEL;
	}

	Char c;
	scanf("%C",&c);
	if (c=='y')
	{
		ERR(TXT("Yes"));
		return SC_OK;
	}
	else if (c=='n')
	{
		ERR(TXT("No"));
		return SC_CANCEL;
	}
	else
	{
		ERR(TXT("Answer not recognise, default: No"));
		return SC_CANCEL;
	}
}

Bool CVersionControlInterface::OnSaveFailure( CDiskFile &file )
{ 
	ERR(TXT(" Cannot be saved because it was not checked out. What do you wish to do?"));
	ERR(TXT(" 1 - Check out"));
	ERR(TXT(" 2 - Overwrite"));
	Char c;

	if ( m_silentMode )
	{
		ERR( TXT("Silent mode, default: 1") );
		c = '1';
	}
	else
	{
		scanf("%C",&c);
	}

	if (c=='1')
	{
		ERR(TXT("Check out"));
		if ( file.CheckOut() )
		{
			return file.Save( file.GetAbsolutePath() );
		}
		return false;
	}
	else if (c=='2')
	{
		ERR(TXT("Overwrite"));
		file.Overwrite();
		return file.Save( file.GetAbsolutePath() );
	}

	return false;
}

Int CVersionControlInterface::OnParallelUsers( const TDynArray< String > &users )
{
	if ( users.Empty() )
		return SC_OK;

	String question = TXT( "This asset cannot be checked out, because it is already checked out by:\n" );
	for ( Uint i = 0; i < users.Size(); i++ ){
		question += users[i];
		question += TXT("\n");
	}
	ERR(TXT("%s"), question.AsChar());

	return SC_CANCEL;
}

//void CVersionControlInterface::OnCaptureLost(wxMouseCaptureLostEvent &event)
//{
//	LOG(TXT("Lost capture"));
//}

Int CVersionControlInterface::OnCheckOutRequired( const String &path, const TDynArray< String > &users )
{
	String question = TXT("You want to edit ") + path + TXT(" that wasn't checked out. You won't be able to save it. Do you wish to check out it first? y/n: ");
	ERR(TXT("%s"), question.AsChar());

	if ( m_silentMode )
	{
		ERR( TXT("Silent mode, default: Yes") );
		return SC_OK;
	}

	Char c;
	scanf("%C",&c);

	EInputCaptureMode mode = GGame->GetViewport()->GetInputCaptureMode();
	if ( mode != ICM_None )
	{
		GGame->GetViewport()->CaptureInput( ICM_None );
	}
	if ( c=='y' )
	{
		ERR(TXT("yes"));
		GGame->GetViewport()->CaptureInput( mode );
		return SC_OK;
	}
	else if ( c=='n' )
	{
		ERR(TXT("no"));
		GGame->GetViewport()->CaptureInput( mode );
		return SC_CANCEL;
	}
	else
	{
		ERR(TXT("Answer not recognise, default: No"));
		GGame->GetViewport()->CaptureInput( mode );
		return SC_CANCEL;
	}


	return SC_CANCEL;
}

void CVersionControlInterface::OnDoubleCheckOut()
{
	ERR(TXT2(ERROR_DOUBLE_CHECKOUT));
}

void CVersionControlInterface::OnNotEdited( const CDiskFile &file )
{
	String error = file.GetFileName() + TXT(" is not checked out or marked for addition/deletion, therefore cannot be reverted");
	ERR(TXT("%s"), error.AsChar());
}

Int CVersionControlInterface::OnSubmit( String &description, CDiskFile &file )
{
	description = TXT("Resource linker");
	return SC_OK;
}

Int CVersionControlInterface::OnMultipleSubmit(String &description, const TDynArray< CDiskFile * > &files, 
											   TSet< CDiskFile * > &chosen)
{
	description = TXT("Resource linker");
	return SC_OK;
}

/*Int CVersionControlInterface::OnMultipleRevert()
{
ERR(TXT2(QUESTION_REVERT) TXT(", y/n: "));
if ( m_silentMode )
{
ERR( TXT("Silent mode, default: Yes") );
return SC_OK;
}

Char c;
scanf("%C",&c);
if (c=='y')
{
ERR(TXT("yes"));
return SC_OK;
}
else if (c=='n')
{
ERR(TXT("no"));
return SC_CANCEL;
}
else
{
ERR(TXT("Answer not recognise, default: No"));
return SC_CANCEL;
}*/

void CVersionControlInterface::OnFailedCheckOut()
{
	ERR(TXT2(ERROR_CHECKOUT));
}

void CVersionControlInterface::OnFailedSubmit()
{
	ERR(TXT2(ERROR_SUBMIT));	
}

void CVersionControlInterface::OnLocalFile()
{
	ERR(TXT2(ERROR_LOCAL));
}

void CVersionControlInterface::OnFailedDelete()
{
	ERR(TXT2(ERROR_DELETE));
}

void CVersionControlInterface::OnLoadedDelete()
{
	ERR(TXT2(ERROR_LOADED));
}

void CVersionControlInterface::OnSyncRequired()
{
	ERR(TXT2(ERROR_SYNC_REQUIRED));
}

Int CVersionControlInterface::OnSyncFailed()
{
	ERR(TXT2(QUESTION_FORCE_SYNC) TXT(", y/n: "));

	if ( m_silentMode )
	{
		ERR( TXT("Silent mode, default: Yes") );
		return SC_FORCE;
	}

	Char c;
	scanf("%C",&c);
	if (c=='y')
	{
		ERR(TXT("yes"));
		return SC_FORCE;
	}
	else if (c=='n')
	{
		ERR(TXT("no"));
		return SC_CANCEL;
	}
	else
	{
		ERR(TXT("Answer not recognise, default: No"));
		return SC_CANCEL;
	}
}

Bool CVersionControlInterface::OnConfirmModifiedDelete()
{
	WARN(TXT("Delete file, that was modified"));
	return true;
}

Bool CVersionControlInterface::OnConfirmDelete()
{
	return true;
}

void CVersionControlInterface::OnFileLog(CDiskFile &file, TDynArray< THashMap< String, String > > &history )
{
}

void CVersionControlInterface::OnFilesIdentical() {}

Bool CVersionControlInterface::OnConfirmRevert()
{
	ERR(TXT2(QUESTION_REVERT) TXT(", y/n: "));
	if ( m_silentMode )
	{
		ERR( TXT("Silent mode, default: Yes") );
		return true;
	}

	Char c;
	scanf("%C",&c);
	if (c=='y')
	{
		ERR(TXT("yes"));
		return true;
	}
	else if (c=='n')
	{
		ERR(TXT("no"));
		return false;
	}
	else
	{
		ERR(TXT("Answer not recognise, default: No"));
		return false;
	}
}
