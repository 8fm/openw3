/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT

#include "string.h"
#include "changelist.h"
#include "diskFile.h"
#include "hashmap.h"
#include "set.h"

// results
#define SC_OK						1000
#define SC_CANCEL					1001
#define SC_ERROR					1002

// commands
#define SC_OVERWRITE				1003
#define SC_SAVE_AS					1004
#define SC_CHECK_OUT				1005
#define SC_FORCE					1006

// states
#define SC_CHECKED_OUT				2001
#define SC_NOT_CHECKED_OUT			2002
#define SC_SYNCED					2003
#define SC_SUBMITTED				3001
#define SC_REVERTED					4001
#define SC_DELETED					5001

// errors
#define SC_NOT_IN_DEPOT				6001
#define SC_WRITABLE					6002
#define SC_FILES_IDENTICAL			6003

class CDiskFile;

/// Source control config
struct SSourceControlSettings
{
	String	m_user;
	String	m_client;
	String	m_host;
	String	m_password;
	String	m_port;
	Bool	m_automaticChangelists;
};

/// Source control interface
class ISourceControl
{
	friend class CDiskFile;
	friend class CDirectory;

	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

protected:
	// Returns the given changelist's ID - only for internal use
	RED_INLINE const SChangelist::ID& GetChangelistID( const SChangelist& changeList ) const { return changeList.m_id; }

	// Constructs a changelist with the given ID - only for internal use
	RED_INLINE SChangelist ConstructChangelistWithID( const SChangelist::ID& id ) { return SChangelist( id ); }
	

public:	
	virtual ~ISourceControl() {};

	virtual Bool IsSourceControlDisabled() const { return true; }

	// Sets information about user, client, host, etc
	virtual void SetSettings( const SSourceControlSettings& /*settings*/ ) { }

	// Create a new changelist
	virtual Bool CreateChangelist( const String& /*name*/, SChangelist& /*changelist*/ ) { return false; }

	// Returns true if the passed changelist represents the default changelist
	virtual Bool IsDefaultChangelist( const SChangelist& /*changelist*/ ) const { return false; }

	// Returns true if the user has enabled automatic changelist creation
	virtual Bool AutomaticChangelistsEnabled() const { return false; }

	//
	// IMPORTANT: The file functions became protected so that they are called only from
	//	          the CDiskFile class which takes care of notifying the linked resource
	//	          (if any) for actions that would need extra resource-specific steps,
	//	          such as creating a new changelist or unloading/reloading data.
	//	          
	//	          Because of this, it is highly NOT recommended to change this back to
	//	          private, despite how tempting it might be.
	//
protected:

	// Inform whether given file was checked out
	virtual Bool GetStatus( CDiskFile& file, TDynArray<String>* = 0 ) { file.SetLocal(); return true; }

	// Inform whether file (given by path) was checked out by someone else
	virtual EOpenStatus GetOpenStatus( const String& /*file*/ ) { return OS_Unknown; }

	// Check out given file
	virtual Bool CheckOut( CDiskFile& /*file*/, Bool /*exclusive*/ = true ) { return false; }

	// Check out given file without notifying the user about the potential problems
	virtual Bool SilentCheckOut( CDiskFile& /*file*/, Bool /*exclusive*/ = true ) { return false; }

	// Submit given file
	virtual Bool Submit( CDiskFile& /*file*/ ) { return false; }

	// Submit given file with description
	virtual Bool Submit( CDiskFile& /*file*/, const String& /*description*/ ) { return false; }

	// Submit given directory
	virtual Bool Submit( CDirectory& /*directory*/ ) { return false; }

	// Submit given directory with description
	virtual Bool Submit( CDirectory& /*directory*/, const String& /*description*/ ) { return false; }

	// Revert given file to the state from the depot
	virtual Bool Revert( CDiskFile& /*file*/, Bool /*silent*/ = false ) { return false; }

	// Delete given file from the workspace and mark for deletion in the depot
	virtual Bool Delete( CDiskFile& /*resource*/, Bool /*confirm*/ = true) { return false; }

	// Rename/Move file
	virtual Bool Rename( CDiskFile& /*resource*/, const String& /*newAbsolutePath*/ ) { return false; }

	// Save file
	virtual Bool Save( CDiskFile& file ) { return file.Save(file.GetAbsolutePath()); }

	// Prepare file for editing
	virtual Bool Edit( CDiskFile& /*file*/ ) { return true; }

	// Add file to the version control system
	virtual Bool Add( CDiskFile& /*file*/, const SChangelist& /*changelist*/ = SChangelist::DEFAULT ) { return true; }

	// Lock file in the version control system
	virtual Bool Lock( CDiskFile& /*file*/ ) { return true; }

	// Get latest version of an object from the version control system
	virtual Bool GetLatest( CDiskFile& /*file*/ ){ return false; }

	// Get latest version of an object from the version control system
	virtual Bool GetLatest( CDirectory& /*directory*/, Bool /*confirm*/ = 0 ){ return false; }

	//
	// The methods below do not act on individual files so they can be in GVersionControl
	// however any subclass should make sure that the resources in CDiskFiles are notified
	// about changes
	// 
public:
	// Get latest version of the specified path from the version control system
	virtual Bool GetLatest( const String& /*absoluteFilePath*/, Bool /*confirm*/ = 0  ){ return false; }

	// Check out given file of the specified path from the version control system
	virtual Bool CheckOut( const String& /*absoluteFilePath*/, const SChangelist& /*changeList*/ = SChangelist::DEFAULT, Bool /*exclusive*/ = true  ) { return false; }

	// This one is so that we can check perforce in general on a path to a file and not only CFile or CDirectory
	virtual Bool DoesFileExist( const String& /*absoluteFilePath*/ ) { return false; }
	
	// Get a list of folders inside the given path
	virtual Bool DoesFolderExist( const String& /*absolutePath*/ ) { return false; }

	// Check out file list
	virtual Bool EnsureCheckedOut( const TDynArray< CDiskFile* >& /*fileList*/, Bool /*exclusive*/ = true ) { return false; }

	// Return all files opened for edit, delete, etc.
	virtual Bool Opened( TDynArray< CDiskFile* >& /*files*/ ){ return false; }

	// Return all files in a given folder
	virtual Bool GetListOfFiles( const String& /*folderPath*/, TDynArray< String >& /*files*/ ) { return false; }

	// Return file's history
	virtual Bool FileLog( CDiskFile& /*file*/, TDynArray< THashMap< String, String > >& ){ return false; }

	// Return the name of the user to last edit the specified file
	virtual Bool FileLastEditedBy( CDiskFile& /*file*/, String& /*user*/ ) { return false; }
	virtual Bool FileLastEditedBy( const String& /*absoluteFilePath*/, String& /*user*/ ) { return false; }

	// Retrieve all possible views
	virtual Bool GetViews( TDynArray< String >& /*views*/ ){ return false; }

	// version control attributes - used to keep additional information about the file
	virtual Bool GetAttribute( const String& /*absFileName*/, const String& /*name*/, String& /*value*/ ) { return false; }

	// version control attributes - used to keep additional information about the file
	virtual Bool GetAttribute( CDiskFile& /*file*/, const String& /*name*/, String& /*value*/ ) { return false; }

	// version control attributes - used to keep additional information about the file
	virtual Bool SetAttribute( const String& /*absFileName*/, const String& /*name*/, String& /*value*/ ) { return false; }

	// version control attributes - used to keep additional information about the file
	virtual Bool SetAttribute( CDiskFile& /*file*/, const String& /*name*/, String& /*value*/ ) { return false; }

	// Submit given files
	virtual Bool Submit( TDynArray< CDiskFile * >& /*files*/ ) { return false; }

	// Submit given files with description
	virtual Bool Submit( TDynArray< CDiskFile * >& /*files*/, const String& /*description*/ ) { return false; }

	// Submit given files
	virtual Bool Submit( TDynArray< CDiskFile * >& /*files*/, TSet< CDiskFile * >& ) { return false; }

	// Submit given files with description
	virtual Bool Submit( TDynArray< CDiskFile * >& /*files*/, TSet< CDiskFile * >&, const String& /*description*/ ) { return false; }

	// Submit given changelist (provided it's not the default one)
	virtual Bool Submit( const SChangelist& ) { return false; }

	// Revert given file to the state from the depot
	virtual Bool Revert( TDynArray< CDiskFile * >& /*files*/ ) { return false; }

	// Revert files from given changelist to the state from the depot (with the option of reverting unchanged files only)
	virtual Bool Revert( const SChangelist& /*changelist*/, Bool /*unchangedOnly*/ = false ) { return false; }

	// Revert given file to the state from the depot using an absolute path
	// NOTE: make sure you *REALLY* need this because this method will not
	// notify any CResource about it being reverted
	virtual Bool RevertAbsolutePath( const String& /*absFileName*/ ) { return false; }

	// Delete given file from the workspace and mark for deletion in the depot
	virtual Bool Delete( TDynArray< CDiskFile * >& /*files*/, Bool /*confirm*/ = true ) { return false; }

public:
	// Get file status string
	static const Char* StatusString( CDiskFile* file )
	{
		if ( file->IsLocal() )
		{
			return TXT( "Local" );
		}
		else if ( file->IsCheckedIn() )
		{
			return TXT( "Checked in" );
		}
		else if ( file->IsCheckedOut() )
		{
			return TXT( "Checked out" );
		}
		else if ( file->IsDeleted() )
		{
			return TXT( "Deleted" );
		}
		else if ( file->IsAdded() )
		{
			return TXT( "Added" );
		}
		else
		{
			return TXT( "Unknown" );
		}
	}
};

/// Editor side of version control interface
class IVersionControlEditorInterface
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

public:
	virtual ~IVersionControlEditorInterface() {};

public:
	//! Not working
	virtual void OnNotOperational()=0;

	// Resolves situation, when connection could not be established
	virtual Int32 OnNoConnection()=0;

	// Resolves situation, when some files on list are already checked out by some other users
	virtual Int32 OnParallelUsers( const TDynArray< CDiskFile* > &fileList, const TDynArray< String > &users, Bool exclusiveAccess )=0;

	// Resolves situation, when file is checked out already by some other users
	virtual Int32 OnParallelUsers( CDiskFile &file, const TDynArray< String > &users, Bool exclusiveAccess )=0;

	// Resolves situation, when file cannot be saved due to lack of check out
	virtual Bool OnSaveFailure( CDiskFile &file )=0;

	// Resolves situation, when file must be checked out
	virtual Int32 OnCheckOutRequired( const String &path, const TDynArray< String > &users )=0;

	// Resolves situation, when file was already checked out
	virtual void OnDoubleCheckOut()=0;

	// Resolves situation, when file was not checked out before other operation
	virtual void OnNotEdited( const CDiskFile &file )=0;

	// Resolves situation, when single file is about to be submitted
	virtual Int32 OnSubmit( String &description, CDiskFile &resource )=0;

	// Resolves situation, when multiple files are about to be submitted
	virtual Int32 OnMultipleSubmit( String &, const TDynArray< CDiskFile * > &, TSet< CDiskFile * > &)=0;

	// Resolves situation, when file could not be submitted
	virtual void OnFailedSubmit()=0;

	// Resolves situation, when file could not be checked out
	virtual void OnFailedCheckOut()=0;

	// Resolves situation, when file is local
	virtual void OnLocalFile()=0;

	// Resolves situation, when loaded asset is about to be deleted
	virtual void OnLoadedDelete()=0;

	// Resolves situation, when asset could not be deleted
	virtual void OnFailedDelete()=0;

	// Resolves situation, when asset has to be synced
	virtual void OnSyncRequired()=0;

	// Resolves situation, when asset cannot be synced due to writable/checked out state
	virtual Int32 OnSyncFailed()=0;

	// Confirm deletion of modified file
	virtual Bool OnConfirmModifiedDelete()=0;

	// Confirm deletion of a file
	virtual Bool OnConfirmDelete()=0;

	// Log file history
	virtual void OnFileLog( CDiskFile &file, TDynArray< THashMap< String, String > > &history )=0;

	// Files are identical
	virtual void OnFilesIdentical()=0;

	// Confirm revert of a file
	virtual Bool OnConfirmRevert()=0;
};

// Source control interface
extern ISourceControl *GVersionControl;

#endif
