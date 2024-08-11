/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT

#include "directory.h"

#include "versionControl.h"
#include "resource.h"

//////////////////////////////////////////////////////////////////////////
// Iterators

// Generic iterator for functions with 0 parameters
template < void (CDirectory::*fun)( CDiskFile* ) >
RED_INLINE void CDirectory::IterateFiles()
{
	NeedChildren();
	for ( CDiskFile* file : m_files )
	{
		(this->*fun)( file );
	}
}

// Generic iterator for functions with 1 mutable parameter
template < void (CDirectory::*fun)( CDiskFile*, TDynArray< CDiskFile* >& ) >
RED_INLINE void CDirectory::IterateFiles( TDynArray< CDiskFile* >& files )
{
	NeedChildren();
	for ( CDiskFile* file : m_files )
	{
		(this->*fun)( file, files );
	}
}

// Generic iterator for functions with 1 mutable parameter and string parameter
template < void (CDirectory::*fun)( CDiskFile*, const String& str, TDynArray< CDiskFile* >& ) >
RED_INLINE void CDirectory::IterateFiles( const String& str, TDynArray< CDiskFile* >& files )
{
	NeedChildren();
	for ( CDiskFile* file : m_files )
	{
		(this->*fun)( file, str, files );
	}
}

//////////////////////////////////////////////////////////////////////////
// Private methods
RED_INLINE void CDirectory::GetFileStatus( CDiskFile* file )
{
	file->GetStatus();
}

RED_INLINE void CDirectory::GetEmpty( CDiskFile* file, TDynArray< CDiskFile* >& files )
{
	if ( GFileManager->GetFileSize( file->GetAbsolutePath() ) == 0 )
	{
		files.PushBack( file );
	}
}

RED_INLINE void CDirectory::GetCheckedOut( CDiskFile* file, TDynArray< CDiskFile* >& files )
{
	if ( file->IsCheckedOut() || file->IsDeleted() )
	{
		files.PushBack( file );
	}
}

RED_INLINE void CDirectory::GetWithMatchingName( CDiskFile* file, const String& phrase, TDynArray< CDiskFile* >& files )
{
	if ( file->GetFileName().ContainsSubstring( phrase ) )
	{
		files.PushBack( file );
	}
}

RED_INLINE void CDirectory::RemoveThumbnail( CDiskFile* file )
{
	file->RemoveThumbnail();
}

RED_INLINE void CDirectory::SilentCheckOut( CDiskFile* file )
{
	GVersionControl->SilentCheckOut( *file );
}

RED_INLINE void CDirectory::AddToVersionControl( CDiskFile* file )
{
	ASSERT( file, TXT("NULL passed for the file in CDirectory::AddToVersionControl") );
	file->Add();
}

RED_INLINE void CDirectory::Reload( CDiskFile* file )
{
	CResource* res = file->GetResource();
	if ( res )
	{
		res->Reload( m_confirm );
	}
}

//////////////////////////////////////////////////////////////////////////
// Public methods

void CDirectory::Search( const String& phrase, TDynArray< CDiskFile* >& result )
{
	IterateFiles< &CDirectory::GetWithMatchingName >( phrase, result );

	// Explore subdirectories
	for ( CDirectory* dir : m_directories )
	{
		dir->Search( phrase, result );
	}
}

Bool CDirectory::DeleteChildDirectory( CDirectory* dir )
{
	// Remove from list
	m_directories.Remove( dir );

	// Delete directory files
	/*copy*/ CDirectoryFileList files = dir->GetFiles();
	for ( CDiskFile* file : files )
	{
		if ( file->IsLocal() )
		{
			if ( !file->Delete( false ) )
			{
				return false;
			}
		}
		else if ( file->IsCheckedOut() )
		{
			if ( file->Revert() && file->Delete( false, false ) )
			{
				if ( !file->Submit() )
				{
					return false;
				}
			}
			else
			{
				return false;
			}
		}
		else
		{
			if ( !file->Delete( false, false ) )
			{
				return false;
			}
		}
	}

	// Delete subdirectories
	/*copy*/ CDirectoryDirList dirs = dir->GetDirectories();
	for ( CDirectory* childDir : dirs )
	{
		if ( !dir->DeleteChildDirectory( childDir) )
		{
			return false;
		}
	}

	// Delete physical manifestation
	const String absolutePath = GetAbsolutePath();
	if ( !GSystemIO.RemoveDirectory( absolutePath.AsChar() ) )
	{
		return false;
	}

	// Delete
	dir->m_parent = nullptr;
	delete dir;
	return true;
}

void CDirectory::GetCheckedOut( TDynArray< CDiskFile* >& files )
{
	IterateFiles< &CDirectory::GetCheckedOut >( files );

	// Explore subdirectories
	for ( CDirectory* dir : m_directories )
	{
		dir->GetCheckedOut( files );
	}
}

void CDirectory::RemoveThumbnails()
{
	if ( m_populated )
	{
		IterateFiles< &CDirectory::RemoveThumbnail >();

		// Explore subdirectories
		for ( CDirectory* dir : m_directories )
		{
			dir->RemoveThumbnails();
		}
	}
}

void CDirectory::GetStatus()
{
	IterateFiles< &CDirectory::GetFileStatus >();
}

void CDirectory::CheckedOutInc()
{
	if ( !m_checkedOut && m_parent )
	{
		m_parent->CheckedOutInc();
	}

	m_checkedOut++;
}

void CDirectory::CheckedOutDec()
{
	m_checkedOut--;

	if ( !m_checkedOut && m_parent )
	{
		m_parent->CheckedOutDec();
	}
}

Bool CDirectory::Sync()
{
	return GVersionControl->GetLatest( *this );
}

void CDirectory::CheckOut()
{
	IterateFiles< &CDirectory::SilentCheckOut >();

	// Recurse
	for ( CDirectory* dir : m_directories )
	{
		dir->CheckOut();
	}
}

Bool CDirectory::Submit()
{
	// Submit directory
	return GVersionControl->Submit( *this );
}

Bool CDirectory::Submit( const String& description )
{
	// Submit directory with description
	return GVersionControl->Submit( *this, description );
}

Bool CDirectory::Revert()
{
	NeedChildren();

	// Get all opened files
	TDynArray< CDiskFile* > opened;
	GVersionControl->Opened( opened );

	// Get the absolute path
	String absolutePath;
	GetAbsolutePath( absolutePath );

	// Get files that beings with directory name
	TDynArray< CDiskFile* > files;
	for ( Uint32 i = 0; i < opened.Size(); i++ )
	{
		if ( opened[ i ]->GetAbsolutePath().BeginsWith( absolutePath ) )
		{
			files.PushBack( opened[ i ] );
		}
	}

	// Revert files
	return GVersionControl->Revert( files );
}

void CDirectory::Add()
{
	IterateFiles< &CDirectory::AddToVersionControl >();

	// Recurse
	for ( CDirectory* dir : m_directories )
	{
		dir->Add();
	}
}

void CDirectory::Reload( Bool confirm, Bool recursive /* = true */ )
{
	m_confirm = confirm;
	IterateFiles< &CDirectory::Reload >();

	if( recursive )
	{
		for ( CDirectory* dir : m_directories )
		{
			dir->Reload( confirm, recursive );
		}
	}
}

Bool CDirectory::Rename( const String& newName )
{
	ASSERT( GetParent(), TXT("Cannot rename a parentless directory") );
	if ( !GetParent() )
	{
		return false;
	}

	NeedChildren();

	// Unload all files and keep a list with versioncontrol added files
	const TFiles files = m_files;
	TDynArray< CDiskFile* > revAddLater;
	for ( auto it=files.Begin(); it != files.End(); ++it )
	{
		if ( (*it)->IsAdded() )
		{
			revAddLater.PushBack( *it );
			GVersionControl->Revert( *(*it), true );
		}
		else if ( !(*it)->IsLocal() ) // Abort if there are versioncontrolled files
		{
			return false;
		}
		(*it)->Unload();
	}

	// Rename directory on disk
	String oldPath = GetParent()->GetAbsolutePath() + m_name;
	String newPath = GetParent()->GetAbsolutePath() + newName;
	if ( !GSystemIO.MoveFile( oldPath.AsChar(), newPath.AsChar() ) )
	{
		return false;
	}

	// Set new directory name
	m_name = newName;

	// Add files back to version control if necessary
	for ( auto it=revAddLater.Begin(); it != revAddLater.End(); ++it )
	{
		(*it)->Add();
	}

	return true;
}

void CDirectory::CleanupMissingFiles()
{
	// Deleting non existent files
	TDynArray< CDiskFile* > toDelete;
	IterateFiles< &CDirectory::GetEmpty >( toDelete );

	// Delete files from depot directory
	for ( Uint32 i = 0; i < toDelete.Size(); ++i )
	{
		// We're not interested in removing any entries that aren't loose on the disk
		if ( toDelete[ i ]->IsLooseFile() )
		{
			// Discard loaded resource
			if ( toDelete[ i ]->IsLoaded() )
			{
				CResource* res = toDelete[ i ]->GetResource();
				toDelete[ i ]->Bind( NULL );
				res->Discard();
			}

			// Delete file from tree
			DeleteFile( *toDelete[ i ] );
		}
	}
}

#endif // NO_FILE_SOURCE_CONTROL_SUPPORT
