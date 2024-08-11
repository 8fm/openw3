/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "dynarray.h"
#include "sortedMap.h"
#include "hashmap.h"
#include "diskFile.h"
#include "directoryEntries.h"

// List of files in directory
typedef CDirectoryFileList TFiles;
typedef CDirectoryDirList TDirs;

/// Directory - collection of files and directories
class CDirectory : public IDepotObject
{
protected:
	// Default constructor for mocking.
	CDirectory() {}

	//////////////////////////////////////////////////////////////////////////
	// Constructors / Destructors
public:
	CDirectory( const Char* name, const Uint32 nameLength, CDirectory* parent );
	virtual ~CDirectory();

	//////////////////////////////////////////////////////////////////////////
	// Accessors
public:
	//! Get child directories
	RED_INLINE const TDirs& GetDirectories() const;

	//! Get directory files
	RED_INLINE const TFiles& GetFiles() const;

	//! Inform whether contains any checked out files
	RED_INLINE Bool IsCheckedOut() const;

	//! Get parent directory
	RED_INLINE CDirectory* GetParent() const;

	//! Get absolute directory path
	RED_INLINE String GetAbsolutePath() const;

	//! Get depot directory path
	RED_INLINE String GetDepotPath() const;

	//! Returns name of the directory
	RED_INLINE const String& GetName() const;

	//! Is this directory populated ?
	RED_INLINE const Bool IsPopulated() const;

public:
	//! Get absolute directory path
	virtual void GetAbsolutePath( String& str ) const;

	//! Get directory depot path
	virtual void GetDepotPath( String& str ) const;

	// Convert absolute path to local directory path, returns false if path is not under this directory
	Bool ConvertToLocalPath( const String& absolutePath, String& localPath );

	//---

	// Repopulate directory from physical data directory with list of sub directories and files
	// NOTE: this is only for loose files
	void Repopulate( Bool deep = true );
	void ForceRepopulate();

	// Mark directory as populated
	void MarkAsPopulated( const Bool recursive );

	// Add file object to this directory, low level, no checks
	void AddFile( CDiskFile* file, const Bool batchAdd = false );

	// Deletes file from a directory
	void DeleteFile( CDiskFile& file );

	// Add new file
	CDiskFile* CreateNewFile( const String& fileName );
	CDiskFile* CreateNewFile( const Char* fileName );

	// Create directory on disk
	Bool CreateOnDisk() const;

	// Create sub directory
	CDirectory* CreateNewDirectory( const String& name, const Bool batch = false );
	CDirectory* CreateNewDirectory( const Char* name, const Bool batch = false );
	CDirectory* CreateNewDirectory( const Char* name, const Uint32 length, const Bool batch = false );

	// Create path
	CDirectory* CreatePath( const String& path );
	CDirectory* CreatePath( const Char* path, const Char** fileNamePtr = nullptr );

	// Find path (chain of sub directories)
	CDirectory* FindPath( const String& path ) const;
	CDirectory* FindPath( const Char *path, const Char** fileNamePtr = nullptr ) const;

	// Collect files
	void CollectFiles( TDynArray< class CDiskFile* >& outFiles, const String& extensionFilter, const Bool recursive = true, const Bool onlyPopulated = true ) const;

	// Find all resources with given extension
	void FindResourcesByExtension( const String& extension, TDynArray< String >& resourcesPaths, Bool recursive = true, Bool fullFilePath = true ) const;

	// Remove all mapped the content from this directory
	void Unmap();

	// Remap the directory location to a new absolute location
	// Fill remove current disk files/directories and create new structure
	virtual void Remap( const String& newAbsolutePath, const Bool forceRescan = true );

	//////////////////////////////////////////////////////////////////////////
	// Search

	// Find sub directory
	CDirectory* FindLocalDirectory( const String& name ) const;
	CDirectory* FindLocalDirectory( const Char* name ) const;
	CDirectory* FindLocalDirectory( const Char* name, Uint32 length ) const;

	// Find disk file, file name should be given with extension
	CDiskFile* FindLocalFile( const String& fileName ) const;
	CDiskFile* FindLocalFile( const Char* fileName ) const;
	CDiskFile* FindLocalFile( const Char* filename, Uint32 length ) const;

private:
	// Returns name from internal data
	String GetNameFromDepotPath( const String& depotPath ) const;
	void RepopulateDirectory( Bool deep );

	//////////////////////////////////////////////////////////////////////////
	// Source control operations

#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT
public:
	// Search the directory
	void Search( const String& phrase, TDynArray< CDiskFile* >& result );

	// Deletes a subdirectory
	Bool DeleteChildDirectory( CDirectory* dir );

	// Fetch all checked out resources from this directory and subdirectories
	void GetCheckedOut( TDynArray< CDiskFile* >& files );

	// Remove thumbnails
	void RemoveThumbnails();

	// Verifies if files in this directory are checked out
	void GetStatus();

	// Increase number of checked out files/subdirectories
	void CheckedOutInc();

	// Decrease number of checked out files/subdirectories
	void CheckedOutDec();

	// Sync the directory with the one in the version control system
	Bool Sync();

	// Check out the whole directory from the version control system
	void CheckOut();

	// Submit all checked out files from the directory to the version control system
	Bool Submit();

	// Submit all checked out files from the directory to the version control system with description
	Bool Submit( const String& description );

	// Revert all checked out files from the directory
	Bool Revert();

	// Adds all local file from the directory
	void Add();

	// Reload the directory
	void Reload( Bool confirm, Bool recursive = true );

	// Rename the directory
	Bool Rename( const String& newName );

	// Remove entries for missing (non-existing) files from this directory
	void CleanupMissingFiles();

private:
	// Generic iterator for functions with 0 parameters
	template < void (CDirectory::*fun)( CDiskFile* ) >
	RED_INLINE void IterateFiles();

	// Generic iterator for functions with 1 mutable parameter
	template < void (CDirectory::*fun)( CDiskFile*, TDynArray< CDiskFile* >& ) >
	RED_INLINE void IterateFiles( TDynArray< CDiskFile* >& files );

	// Generic iterator for functions with 1 mutable parameter and string parameter
	template < void (CDirectory::*fun)( CDiskFile*, const String& str, TDynArray< CDiskFile* >& ) >
	RED_INLINE void IterateFiles( const String& str, TDynArray< CDiskFile* >& files );

	RED_INLINE void	GetFileStatus		( CDiskFile* file );
	RED_INLINE void	GetEmpty			( CDiskFile* file, TDynArray< CDiskFile* >& files );
	RED_INLINE void	GetCheckedOut		( CDiskFile* file, TDynArray< CDiskFile* >& files );
	RED_INLINE void	GetWithMatchingName	( CDiskFile* file, const String& phrase, TDynArray< CDiskFile* >& files );
	RED_INLINE void	RemoveThumbnail		( CDiskFile* file );
	RED_INLINE void	SilentCheckOut		( CDiskFile* file );
	RED_INLINE void	AddToVersionControl	( CDiskFile* file );
	RED_INLINE void	Reload				( CDiskFile* file );
#endif // NO_FILE_SOURCE_CONTROL_SUPPORT

protected:	
	//! Populates the directory, if necessary
	RED_INLINE void NeedChildren() const;

	CDirectoryFileList				m_files;		//!< Serialized files on disk
	CDirectoryDirList				m_directories;	//!< Child directories
	CDirectory*						m_parent;		//!< Parent directory

	String							m_name;			//!< Directory name
	String*							m_overridePath;	//!< Override path - only set if actually used

	Uint32							m_confirm:1;		//!< State variable used by Reload method
	Uint32							m_populated:1;		//!< Set to true once this directory has been populated
	Uint32							m_subDirsNum:8;		//!< Number of subdirectories
	Int32							m_checkedOut;		//!< Number of checked out files/subdirs

	friend class CResource;
	friend class CDiskFile;
};

namespace DepotHelpers
{
	extern Bool ValidatePathString( const Char* path, const Uint32 length = (const Uint32)-1 );
}

#include "directory.inl"
