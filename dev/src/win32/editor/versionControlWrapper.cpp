/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "../../common/core/versionControl.h"
#include "../../common/core/depot.h"

//RED_DEFINE_STATIC_NAME( UpdateSceneTree )
//RED_DEFINE_STATIC_NAME( FileReloadConfirm )

void CVersionControlWrapper::CopyFiles(CDirectory* dir, TDynArray<CResource*>& data)
{
	if(data.Empty()) return;

	TDynArray<String> names;

	for(TDynArray<CResource*>::iterator it = data.Begin(); it != data.End(); ++it)
	{
		CDiskFile* oldFile = (*it)->GetFile();
		names.PushBack( oldFile->GetFileName() );
	}

	CopyAsFiles(dir, data, names);
}

void CVersionControlWrapper::CopyAsFiles(CDirectory* dir, TDynArray<CResource*>& data, TDynArray<String>& names)
{
	ASSERT(data.Size()==names.Size());

	// You can't drag layer - if you want to load layer you have to load layer info too
	// If you load layer alone it will be damaged
	bool isLayer = false;
	for (Int32 i=(Int32)data.Size()-1; i>=0; i--)
	{
		if (data[i]->IsA<CLayer>())
		{
			data.Remove(data[i]);
			isLayer = true;
		}
	}
	if (isLayer)
	{
		wxString msg = wxT("\nLayers file couldn't be copied.\n\n");
		wxMessageDialog dialog( 0, msg, wxT("Warning"), wxOK | wxICON_WARNING );
		dialog.ShowModal();
	}

	if(data.Empty()) return;

	Uint32 namesIt = -1;

	for(TDynArray<CResource*>::iterator it = data.Begin(); it != data.End(); ++it)
	{
		namesIt++;

		CDiskFile* oldFile = (*it)->GetFile();
		String oldFileName = names[namesIt];

		String absNewPath = dir->GetAbsolutePath() + oldFileName;
		String newPath;
		GDepot->ConvertToLocalPath(absNewPath, newPath);

		// Check in version control
		String value;
		if ( GVersionControl->GetAttribute( absNewPath, TXT("action"), value) && value==TXT("delete"))
		{
			// Revet
			if( GVersionControl->RevertAbsolutePath( absNewPath ) )
			{
				LOG_EDITOR(TXT("Version control action, Revert file: %s"), newPath);
			}
			else
			{
				LOG_EDITOR(TXT("Version control action, Couldn't revert file: %s"), newPath);
			}

			// And override
			CDiskFile* newFile = GDepot->FindFile(newPath);

			if(!newFile->IsLocal() && !newFile->IsCheckedOut() && !newFile->CheckOut()) 
			{
				LOG_EDITOR(TXT("Version control action, Couldn't check out file: %s"), newFile->GetAbsolutePath());
			}
			else if (newFile->IsLocal())
			{
				if (newFile->IsLoaded()) newFile->Unload();
				if (!newFile->Delete())
				{
					LOG_EDITOR(TXT("Version control action, Couldn't delete local file: %s"), newFile->GetAbsolutePath());
				}
			}

			if ( !(oldFile->Copy(dir, oldFileName)) )
			{
				LOG_EDITOR(TXT("Version control action, Couldn't copy file: %s"), newPath);

				wxString msg = wxT("Couldn't copy file to target folder: ") + wxString::Format(_T("%s"), oldFileName.AsChar());

				// Error
				//++it;
				if (!UndoCopyFiles(dir, data, names, it)) msg += wxT("\n\nError: UndoCopyFiles");

				wxMessageBox(msg, wxT("Error"));

				return;
			}
			else
			{
				LOG_EDITOR(TXT("Version control action, Copy file: %s"), newPath);
			}

			dir->Repopulate();
		}
		else
		{
			CDiskFile* newFile = GDepot->FindFile(newPath);

			if ( newFile )
			{
				// Confirm override
				wxString msg = wxT("Do you want to override file: ") + wxString::Format(_T("%s"), newFile->GetFileName().AsChar() ) + wxT("?");

				wxMessageDialog dialog( 0, msg, wxT("Question"), wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION );
				if ( dialog.ShowModal() == wxID_YES ) 
				{
					if(!newFile->IsLocal())
					{
						if(!newFile->IsCheckedOut() && !newFile->CheckOut())
						{
							LOG_EDITOR(TXT("Couldn't check out file: %s"), oldFile->GetAbsolutePath());
						}
					}
					
					// Is loaded
					if (newFile->IsLoaded())
					{		
						String newAbsolutePath;
						GDepot->GetAbsolutePath( newAbsolutePath );
						newAbsolutePath += newPath;

						// Can't copy - have to be reloaded with new data
						if (!GFileManager->CopyFile(oldFile->GetAbsolutePath(), newAbsolutePath, true))
						{
							// Error

							wxString msg = wxT("Couldn't copy (with reload) file to target folder: ") + wxString::Format(_T("%s"), oldFileName.AsChar() );

							// Error
							//++it;
							if (!UndoCopyFiles(dir, data, names, it)) msg += wxT("\n\nError: UndoCopyFiles");

							wxMessageBox(msg, wxT("Error"));

							LOG_EDITOR(TXT("Couldn't copy file: %s"), newPath);

							return;
						}

						SEvents::GetInstance().QueueEvent( CNAME( FileReloadConfirm ), CreateEventData( newFile->GetResource() ) );
					}
					else
					{
						if (newFile->IsLocal())
						{
							if (!newFile->Delete()) 
							{
								LOG_EDITOR(TXT("Couldn't delete local file: %s - override"), newFile->GetAbsolutePath());
							}
						}

						if (newFile->IsLoaded()) 
							newFile->Unload();
						if ( !(oldFile->Copy(dir, oldFileName)) )
						{
							wxString msg = wxT("Couldn't copy file to target folder: ") + wxString::Format(_T("%s"), oldFileName.AsChar());

							// Error
							//++it;
							if (!UndoCopyFiles(dir, data, names, it)) msg += wxT("\n\nError: UndoCopyFiles");

							wxMessageBox(msg, wxT("Error"));

							LOG_EDITOR(TXT("Couldn't copy file: %s"), newPath);

							return;
						}
					}
					
					LOG_EDITOR(TXT("Copy file: %s"), newPath);
					dir->Repopulate();
				}
			}
			else
			{
				if ( !(oldFile->Copy(dir, oldFileName)) )
				{
					wxString msg = wxT("Couldn't copy file to target folder: ") + wxString::Format(_T("%s"), oldFileName.AsChar());

					// Error
					//++it;
					if (!UndoCopyFiles(dir, data, names, it)) msg += wxT("\n\nError: UndoCopyFiles");

					wxMessageBox(msg, wxT("Error"));

					LOG_EDITOR(TXT("Couldn't copy file: %s"), newPath);

					return;
				}
				else
				{
					LOG_EDITOR(TXT("Copy file: %s"), newPath);
				}
				dir->Repopulate();
			}
		}

		CDiskFile* newFile = GDepot->FindFile(newPath);
		newFile->SetChangelist( oldFile->GetChangelist() );

		if (newFile->IsCheckedIn())
		{
			newFile->SetLocal();
		}

		if ( !newFile->Add() )
		{
			wxString msg = wxT("Couldn't add moved file to version control: ") + wxString::Format(_T("%s"), newFile->GetFileName().AsChar());

			// Error
			//++it;
			if(!UndoCopyFiles(dir, data, names, it)) msg += wxT("\n\nError: UndoCopyFiles");

			wxMessageBox(msg, wxT("Error"));

			LOG_EDITOR(TXT("Couldn't add moved file: %s"), newPath);

			return;
		}
		else
		{
			LOG_EDITOR(TXT("Add moved file: %s"), newPath);
		}
	}

	SEvents::GetInstance().QueueEvent(CNAME( UpdateSceneTree ), NULL);
}

Bool CVersionControlWrapper::UndoCopyFiles(CDirectory* dir, TDynArray<CResource*>& data, TDynArray<String>& names, TDynArray<CResource*>::iterator end)
{
	ASSERT(data.Size()==names.Size());

	Uint32 namesIt = -1;

	Bool res = true;
	String msg = String::EMPTY;

	for(TDynArray<CResource*>::iterator it = data.Begin(); it != end; ++it)
	{
		namesIt++;

		CDiskFile* oldFile = (*it)->GetFile();

		String oldFileName = names[namesIt];
		String absNewPath = dir->GetAbsolutePath() + oldFileName;
		String newPath;
		GDepot->ConvertToLocalPath(absNewPath, newPath);
		CDiskFile* newFile = GDepot->FindFile(newPath);

		if ( newFile )
		{
			if (newFile->IsLoaded()) newFile->Unload();

			msg = TXT("File found, ");
			if ( newFile->IsLocal() )
			{
				msg += TXT("is local, ");
				if (!newFile->Delete()) res = false;
			}
			else if (newFile->IsCheckedOut())
			{
				msg += TXT("is check out, ");
				if (!newFile->Revert()) res = false;
				if (!newFile->Delete(false)) res = false;
			}
			else if (newFile->IsAdded())
			{
				msg += TXT("is added, ");
				if (!newFile->Revert()) res = false;
				if (!newFile->Delete()) res = false;
			}
			else
			{
				msg += TXT("version control, ");
				if( newFile->Delete( false, false ) )
				{
					res = false;
				}
			}
		}
		else
		{
			msg = TXT("File not found");
			res = false;
		}
	}

	LOG_EDITOR(TXT("Undo copy file - %s"), msg);
	SEvents::GetInstance().QueueEvent(CNAME( UpdateSceneTree ), NULL);

	return res;
}

void CVersionControlWrapper::CreateLinksForFiles(CDirectory* dir, TDynArray<CResource*>& data)
{
	if(data.Empty()) return;

	TDynArray<String> names;

	for(TDynArray<CResource*>::iterator it = data.Begin(); it != data.End(); ++it)
	{
		CDiskFile* oldFile = (*it)->GetFile();
		names.PushBack( oldFile->GetFileName() );
	}

	CreateLinksForFiles(dir, data, names);
}

void CVersionControlWrapper::CreateLinksForFiles(CDirectory* dir, TDynArray<CResource*>& data, TDynArray<String>& names)
{
	ASSERT(data.Size()==names.Size());

	Uint32 namesIt = -1;

	for(TDynArray<CResource*>::iterator it = data.Begin(); it != data.End(); ++it)
	{
		namesIt++;

		CDiskFile* oldFile = (*it)->GetFile();

		String oldFileName = oldFile->GetFileName();
		String absNewPath = dir->GetAbsolutePath() + names[namesIt];
		String newPath;
		GDepot->ConvertToLocalPath(absNewPath, newPath);

		// Create link file
		String absLinkPath = oldFile->GetDirectory()->GetAbsolutePath() + oldFileName + TXT(".link");
		String linkPath = oldFile->GetDirectory()->GetDepotPath() + oldFileName + TXT(".link");

		// First try to find file - for safety reason
		CDiskFile* linkFile = GDepot->FindFile(linkPath);
		if ( linkFile == NULL )
		{
			CDirectory* oldDir = oldFile->GetDirectory();
			linkFile = GDepot->CreateNewFile( linkPath.AsChar() );
			oldDir->Repopulate();
		}
		else
		{
			linkFile->SilentCheckOut();
		}

		// Fill link file
		GFileManager->SaveStringToFile(absLinkPath, newPath);

		if (!linkFile->IsAdded())
		{
			if (linkFile->IsCheckedIn())
			{
				linkFile->SetLocal();
			}

			linkFile->SetChangelist( oldFile->GetChangelist() );
			// File don't exist, add file
			if ( !linkFile->Add() )
			{
				wxString msg = wxT("Couldn't add link file to version control: ") + wxString::Format(_T("%s"), linkFile->GetFileName().AsChar());

				// Error
				if(!UndoCopyFiles(dir, data, names, data.End()))	msg += wxT("\n\nError: UndoCopyFiles");
				//++it;
				if(!UndoCreateLinksForFiles(data, it))				msg += wxT("\n\nError: UndoCreateLinksForFiles");

				wxMessageBox(msg, wxT("Error"));

				LOG_EDITOR(TXT("Couldn't add link file: %s"), linkFile->GetAbsolutePath());

				return;
			}
			else
			{
				LOG_EDITOR(TXT("Add link file: %s"), linkFile->GetAbsolutePath());
			}
		}
		else if (!linkFile->IsLocal())
		{
			// File exists, edit it
			if( !linkFile->CheckOut() )
			{
				wxString msg = wxT("Couldn't check out link file: ") + wxString::Format(_T("%s"), linkFile->GetFileName().AsChar());

				// Error
				if(!UndoCopyFiles(dir, data, names, data.End()))	msg += wxT("\n\nError: UndoCopyFiles");
				//++it;
				if(!UndoCreateLinksForFiles(data, it))				msg += wxT("\n\nError: UndoCreateLinksForFiles");

				wxMessageBox(msg, wxT("Error"));

				LOG_EDITOR(TXT("Couldn't check out link file: %s"), linkFile->GetAbsolutePath());

				return;
			}
			else
			{
				LOG_EDITOR(TXT("Check out link file: %s"), linkFile->GetAbsolutePath());
			}
		}
	}
}

Bool CVersionControlWrapper::UndoCreateLinksForFiles(TDynArray<CResource*>& data, TDynArray<CResource*>::iterator end)
{
	Bool res = true;
	String msg = String::EMPTY;

	data[0]->GetFile()->GetDirectory()->Repopulate();

	for(TDynArray<CResource*>::iterator it = data.Begin(); it != end; ++it)
	{
		CDiskFile* oldFile = (*it)->GetFile();

		String oldFileName = oldFile->GetFileName();
		String linkPath = oldFile->GetDirectory()->GetDepotPath() + oldFileName + TXT(".link");

		CDiskFile* linkFile = GDepot->FindFile(linkPath);

		if ( linkFile )
		{
			msg = TXT("File found, ");
			if ( linkFile->IsLocal() )
			{
				msg += TXT("is local, ");
				if (!linkFile->Delete()) res = false;
			}
			else if (linkFile->IsCheckedOut())
			{
				msg += TXT("is checked out, ");
				if (!linkFile->Revert()) res = false;
				if (!linkFile->Delete(false)) res = false;
			}
			else if (linkFile->IsAdded())
			{
				msg += TXT("is added, ");
				if (!linkFile->Revert()) res = false;
				if (!linkFile->Delete()) res = false;
			}
			else if (linkFile->IsCheckedIn())
			{
				msg += TXT("is checked in, ");
				linkFile->SetAdded();
				if (!linkFile->Revert()) res = false;
				if (!linkFile->Delete()) res = false;
			}
			else
			{
				msg += TXT("version control, ");
				if( !linkFile->Delete( false, false ) )
				{
					res = false;
				}
			}
		}
		else
		{
			msg = TXT("File not found ");
			res = false;
		}
	}

	LOG_EDITOR(TXT("Undo copy file - %s"), msg);
	return res;
}

void CVersionControlWrapper::DeleteFiles(TDynArray<CResource*>& data)
{
	Bool res = true;

	for(TDynArray<CResource*>::iterator it = data.Begin(); it != data.End(); ++it)
	{
		CDiskFile* oldFile = (*it)->GetFile();

		oldFile->Unload();
		if ( oldFile->IsLocal() )
		{
			if (!oldFile->Delete(false)) { res = false; LOG_EDITOR(TXT("Couldn't delete original file %s - local"), oldFile->GetAbsolutePath()); }
		}
		else if (oldFile->IsCheckedOut())
		{
			if (!oldFile->Revert()) { res = false; LOG_EDITOR(TXT("Couldn't revert original file %s"), oldFile->GetAbsolutePath()); }
			if (!oldFile->Delete(false)) { res = false; LOG_EDITOR(TXT("Couldn't delete original file - check out %s"), oldFile->GetAbsolutePath()); }
		}
		else if (oldFile->IsAdded())
		{
			if (!oldFile->Revert()) { res = false; LOG_EDITOR(TXT("Couldn't revert original file %s"), oldFile->GetAbsolutePath()); }
			if (!oldFile->Delete(false)) { res = false; LOG_EDITOR(TXT("Couldn't delete original file - added %s"), oldFile->GetAbsolutePath()); }
		}
		else
		{
			if( !oldFile->Delete( false, false ) )
			{ 
				if (oldFile->IsCheckedIn())
				{
					oldFile->SetLocal();
					if (!oldFile->Delete()) { res = false; LOG_EDITOR(TXT("Couldn't delete orginal file %s - check in"), oldFile->GetAbsolutePath()); }
				}
				else
				{
					res = false; LOG_EDITOR(TXT("Couldn't delete original file %s - version control"), oldFile->GetAbsolutePath()); 
				}
			}
		}
	}

	if (!res)
	{
		wxString msg = wxT("Couldn't delete original file.");
		wxMessageBox(msg, wxT("Error"));
	}

	SEvents::GetInstance().QueueEvent(CNAME( UpdateSceneTree ), NULL);
}

void CVersionControlWrapper::DeleteLinksForFiles(CDirectory*dir, TDynArray<CResource*>& data)
{
	Bool res = true;

	for(TDynArray<CResource*>::iterator it = data.Begin(); it != data.End(); ++it)
	{
		CDiskFile* oldFile = (*it)->GetFile();

		String oldFileName = oldFile->GetFileName();
		String absNewPath = dir->GetAbsolutePath() + oldFileName;
		String newPath;
		GDepot->ConvertToLocalPath(absNewPath, newPath);

		CDiskFile* oldLinkFile = GDepot->FindFile(newPath + TXT(".link"));
		if (oldLinkFile)
		{
			if (oldLinkFile->IsLocal())
			{
				if (!oldLinkFile->Delete()) { res = false; LOG_EDITOR(TXT("Couldn't delete link file %s - local"), oldFile->GetAbsolutePath()); }
			}
			else if (oldLinkFile->IsAdded())
			{
				if (!oldLinkFile->Revert()) { res = false; LOG_EDITOR(TXT("Couldn't revert link file %s"), oldFile->GetAbsolutePath()); }
				if (!oldLinkFile->Delete()) { res = false; LOG_EDITOR(TXT("Couldn't delete link file %s - added"), oldFile->GetAbsolutePath()); }
			}
			else
			{
				if( !oldLinkFile->Delete( false, false ) )
				{ 
					if (oldLinkFile->IsCheckedIn())
					{
						oldLinkFile->SetLocal();
						if (!oldLinkFile->Delete()) { res = false; LOG_EDITOR(TXT("Couldn't delete orginal file %s - check in"), oldLinkFile->GetAbsolutePath()); }
					}
					else
					{
						res = false; LOG_EDITOR(TXT("Couldn't delete link file %s - version control"), oldFile->GetAbsolutePath());
					} 
				}
			}
		}
	}

	if (!res)
	{
		wxString msg = wxT("Couldn't delete link file.");
		wxMessageBox(msg, wxT("Error"));
	}
}

void CVersionControlWrapper::MoveFiles(CDirectory* dir, TDynArray<CResource*>& data)
{
	if(data.Empty()) return;

	// Copy
	CopyFiles(dir, data);

	// Create links
	TDynArray<String> names;
	for(TDynArray<CResource*>::iterator it = data.Begin(); it != data.End(); ++it)
	{
		CDiskFile* file = (*it)->GetFile();
		names.PushBack( file->GetFileName() );
	}
	CreateLinksForFiles(dir, data, names);

	// Delete unnecessary links
	DeleteLinksForFiles(dir, data);

	// Delete old files
	DeleteFiles(data);
}

void CVersionControlWrapper::RenameFiles(TDynArray<CResource*>& data, TDynArray<String>& names)
{
	ASSERT(data.Size()==names.Size());

	if(data.Empty()) return;
	
	CDirectory* dir = data[0]->GetFile()->GetDirectory();

	// Copy with new names
	CopyAsFiles(dir, data, names);

	// Create links
	CreateLinksForFiles(dir, data, names);

	// Delete old files
	DeleteFiles(data);
}