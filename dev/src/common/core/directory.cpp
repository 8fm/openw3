/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "directory.h"
#include "resource.h"
#include "filePath.h"
#include "depot.h"

namespace DepotHelpers
{
	RED_INLINE const Uint32 CalculateFileHash( const Char* fileName, const Uint32 length )
	{
		return String::SimpleHash( fileName, length + 1 );
	}

	RED_INLINE const Uint32 CalculateFileHash( const String& fileName )
	{
		Uint32 hash = 0;
		fileName.SimpleHash(hash);
		return hash;
	}

	Bool ValidatePath( const Char* path, const Bool canContainFileName )
	{
#ifdef RED_PLATFORM_WINPC
		if ( !path )
		{
			RED_HALT( "Path string pased to a function is NULL" );
			return false;
		}

		const Char* dotPos = nullptr;
		const Char* cur = path;
		while ( *cur )
		{
			if ( *cur == '/' )
			{
				RED_HALT( "Path string '%ls' is using invalid path separator. This is no longer allowed. FIX IT.", path );
				return false;
			}

			else if ( *cur >= 'A' && *cur <= 'Z' )
			{
				RED_HALT( "Path string '%ls' is using upper case letters. This is no longer allowed. FIX IT.", path );
				return false;
			}

			else if ( *cur == '.' )
			{
				if ( !canContainFileName )
				{
					RED_HALT( "Path string '%ls' contains dot which is not an expected character here. This is no longer allowed. FIX IT.", path );
				}

				dotPos = cur;
			}

			++cur;
		}

		// do we contain a file name ? (file name + extension)
		if ( !canContainFileName || !dotPos )
		{
			// we don't have file name so the final character should be path separator
			if ( cur > path && cur[-1] != '\\' )
			{
				RED_HALT( "Path string '%ls' does not end with path separator. FIX IT.", path );
				return false;
			}
		}
#else
		RED_UNUSED(path);
		RED_UNUSED(canContainFileName);
#endif
		return true;
	}


	Bool ValidatePathString( const Char* path, const Uint32 length /*= (const Uint32)-1*/ )
	{
#ifdef RED_PLATFORM_WINPC
		const Char* cur = path;
		while ( *cur && (Int32)((cur-path) < (Int32)length) )	
		{
			if ( *cur == '/' )
			{
				RED_HALT( "Path string '%ls' is using invalid path separator. This is no longer allowed. FIX IT.", path );
				return false;
			}

			if ( *cur >= 'A' && *cur <= 'Z' )
			{
				RED_HALT( "Path string '%ls' is using upper case letters. This is no longer allowed. FIX IT.", path );
				return false;
			}

			++cur;
		}
#else
		RED_UNUSED(path);
		RED_UNUSED(length);
#endif
		return true;
	}
}

//*************************************
//
CDirectory::CDirectory( const Char* name, const Uint32 nameLength, CDirectory* parent )
	: m_parent( parent )
	, m_checkedOut( 0 )
	, m_name( name, (nameLength == (Uint32)-1) ? Red::StringLength(name) : nameLength )
	, m_populated( false )
	, m_overridePath( nullptr )
{}

//*************************************
//

CDirectory::~CDirectory()
{
	m_directories.ClearPtr();
	m_files.ClearPtr();
}

//*************************************
//

void CDirectory::GetAbsolutePath( String& str ) const
{
	if ( m_overridePath )
	{
		// use override path is we have one
		str = *m_overridePath;
	}
	else
	{
		// get base of the path from the parent directory
		if ( m_parent )
		{
			m_parent->GetAbsolutePath( str );
		}

		// append current directory name
		str += m_name;
		str += TXT("\\");
	}
}

//*************************************
//

void CDirectory::GetDepotPath( String& str ) const
{
	// get base of the path from the parent directory
	if ( m_parent )
	{
		m_parent->GetDepotPath( str );
	}

	// append current directory name
	str += m_name;
	str += TXT("\\");
}

// Use a mutex because NeedChildren might call this from multiple threads
static Red::Threads::CMutex RepopulateMutex;

//*************************************
//
void CDirectory::Repopulate( Bool deep /*=true*/ )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > mutex( RepopulateMutex );

	if( m_populated )
	{
		// Another thread beat us, move along.
		return;
	}
	RepopulateDirectory( deep );
}

void CDirectory::ForceRepopulate()
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > mutex( RepopulateMutex );
	RepopulateDirectory( false );
}

void CDirectory::RepopulateDirectory( Bool deep )
{
	const Bool hadFilesBefore = m_files.Size() > 0;

	const String depotPath = GetDepotPath();

	// Scan files
	{
		String fileSearchPath = GetAbsolutePath() + TXT("*.*");
		CSystemFindFile fileEnumerator = fileSearchPath.AsChar();
		while ( fileEnumerator )
		{
			if ( !fileEnumerator.IsDirectory() )
			{
				// always convert the names from the file system to lower case
				String fileName = fileEnumerator.GetFileName();
				fileName.MakeLower();

				// directory was empty - just add files with no checks
				if ( !hadFilesBefore )
				{
					CDiskFile* looseFile = new CDiskFile( this, fileName, nullptr );
					AddFile( looseFile, /*batch*/ true );
					++fileEnumerator;
					continue;
				}

				// check if given file already exists
				CDiskFile* alreadyAddedFile = m_files.Find( fileName.AsChar(), (Uint32)-1 );
				if ( alreadyAddedFile )
				{
					// replace non-loose file with a loose file
					if ( !alreadyAddedFile->IsLooseFile() )
					{
						m_files.Remove( alreadyAddedFile );
					}
					else
					{
						++fileEnumerator;
						continue;;
					}
				}

				// Re add as a normal file
				CDiskFile* looseFile = new CDiskFile( this, fileName, nullptr );
				AddFile( looseFile, /*batch*/ false ); // WE ARE INSERTING WITH SORT into the list
			}

			++fileEnumerator;
		}
	}

	// Scan directories
	TDynArray< CDirectory* > discoveredDirectories;
	{
		String dirSearchPath = GetAbsolutePath() + TXT("*.");
		CSystemFindFile dirEnumerator = dirSearchPath.AsChar();
		while ( dirEnumerator )
		{
			if ( dirEnumerator.IsDirectory() )
			{
				// always convert the names from the file system to lower case
				String dirName = dirEnumerator.GetFileName();
				dirName.MakeLower();

				// Important on the PS4 when running from local disk!
				// Otherwise infinite recursion on the current directory.
				if ( dirName == TXT(".") || dirName == TXT("..") )
				{
					++dirEnumerator;
					continue;
				}

				// hack... - skip some of the directories
				if (this == GDepot)
				{
					if ( dirName == TXT("tmp") || dirName == TXT("speech") )
					{
						WARN_CORE( TXT("Skipping directory '%ls' from depot enumeration"), dirName.AsChar() );
						++dirEnumerator;
						continue;
					}
				}

				// create local dir, will create it only if it does not exist yet
				CDirectory* dir = CreateNewDirectory( dirName );
				if ( dir && deep )
				{
					discoveredDirectories.PushBack( dir );
				}
			}

			++dirEnumerator;
		}
	}

#ifdef RED_PLATFORM_WINPC
#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT
	if ( hadFilesBefore && GIsEditor )
	{
		CleanupMissingFiles();
	}
#endif
#endif

	// Save some memory by shrinking arrays
	m_files.Shrink();
	m_directories.Shrink();

	// Recurse to sub directories
	if ( deep )
	{
		for ( CDirectory* dir : discoveredDirectories )
		{
			dir->Repopulate();
		}
	}

	// Mark as populated
	m_populated = true;
}

//*************************************
//
CDirectory* CDirectory::FindLocalDirectory( const String& name ) const
{
	DepotHelpers::ValidatePathString( name.AsChar() );

	NeedChildren();
	return m_directories.Find( name );
}

//*************************************
//
CDirectory* CDirectory::FindLocalDirectory( const Char* name ) const
{
	DepotHelpers::ValidatePathString( name );

	NeedChildren();
	return m_directories.Find( name, (Uint32)-1 );
}

//*************************************
//
CDirectory* CDirectory::FindLocalDirectory( const Char* name, Uint32 length ) const
{
	DepotHelpers::ValidatePathString( name, length );

	NeedChildren();
	return m_directories.Find( name, length );
}

//*************************************
//
CDiskFile* CDirectory::FindLocalFile( const Char* filename ) const
{
	DepotHelpers::ValidatePathString( filename );

	NeedChildren();
	return m_files.Find( filename, (Uint32)-1 );
}

//*************************************
//
CDiskFile* CDirectory::FindLocalFile( const Char* filename, Uint32 length ) const
{
	DepotHelpers::ValidatePathString( filename );

	NeedChildren();
	return m_files.Find( filename, length );
}

//*************************************
//
CDiskFile* CDirectory::FindLocalFile( const String& filename ) const
{
	DepotHelpers::ValidatePathString( filename.AsChar() );

	NeedChildren();
	return m_files.Find( filename );
}

//*************************************
//

void CDirectory::FindResourcesByExtension( const String& extension, TDynArray< String >& resourcesPaths, Bool recursive, Bool fullFilePath ) const
{
	NeedChildren();

	for ( CDiskFile* file : m_files )
	{
		if ( file->GetFileName().EndsWith( extension ) == true )
		{
			resourcesPaths.PushBackUnique( fullFilePath ? file->GetDepotPath() : file->GetFileName() );
		}
	}

	if ( recursive )
	{
		for ( CDirectory* dir : m_directories )
		{
			dir->FindResourcesByExtension( extension, resourcesPaths, true, fullFilePath );
		}
	}
}

//*************************************
//
CDiskFile* CDirectory::CreateNewFile( const Char* fileName )
{
	DepotHelpers::ValidatePathString( fileName );

	// File already exists ?
	if ( FindLocalFile( fileName ) )
	{
		ERR_CORE( TXT("File '%ls' already exists in directory '%ls'. Not adding."), fileName, GetDepotPath().AsChar() );
		return nullptr;
	}

	// Create file entry
	CDiskFile* diskFile = new CDiskFile( this, fileName );
	AddFile( diskFile, /* batchAdd*/ false );
	return diskFile;
}

//*************************************
//
CDiskFile* CDirectory::CreateNewFile( const String& fileName )
{
	DepotHelpers::ValidatePathString( fileName.AsChar() );

	CDiskFile* diskFile = new CDiskFile( this, fileName );
	AddFile( diskFile, /* batchAdd*/ false );
	return diskFile;
}

//*************************************
//
void CDirectory::MarkAsPopulated( const Bool recursive )
{
	m_populated = true;

	if ( recursive )
	{
		for ( CDirectory* dir : m_directories )
		{
			dir->MarkAsPopulated( true );
		}
	}
}

//*************************************
//
void CDirectory::AddFile( CDiskFile* file, const Bool batchAdd /*= false*/ )
{
	DepotHelpers::ValidatePathString( file->GetFileName().AsChar() );

	// add to file map
	m_files.Add( file, batchAdd );

	// map file to the entry in dependency cache
	if ( file->m_depCacheIndex == 0 )
	{
		file->m_depCacheIndex = GDepot->MapFileToDependencyCache( file );
	}
}

//*************************************

void CDirectory::DeleteFile( CDiskFile& file )
{
	NeedChildren();
	m_files.Remove( &file );
}

//*************************************
//
CDirectory* CDirectory::CreateNewDirectory( const String& name, const Bool batch /*= false*/ )
{
	return CreateNewDirectory( name.AsChar() );
}

//*************************************
//
CDirectory* CDirectory::CreateNewDirectory( const Char* dirName, const Bool batch /*= false*/  )
{
	return CreateNewDirectory( dirName, (Uint32) -1 );
}

//*************************************
//
CDirectory* CDirectory::CreateNewDirectory( const Char* name, const Uint32 length, const Bool batch /*= false*/  )
{
	DepotHelpers::ValidatePathString( name, length );

	// get existing
	CDirectory* dir = m_directories.Find( name, length );
	if ( !dir )
	{
		// create new one
		dir = new CDirectory( name, length, this );
		m_directories.Add( dir, batch );
	}

	return dir;
}

//*************************************
//
CDirectory* CDirectory::CreatePath( const String& path )
{
	return CreatePath( path.AsChar() );
}

//*************************************
//
CDirectory* CDirectory::CreatePath( const Char* path, const Char** fileNamePtr /*= nullptr*/ )
{
	DepotHelpers::ValidatePath( path, true );

	CDirectory* dir = this;

	// split the path and create the directories
	const Char* start = path;
	const Char* cur = path;
	while ( *cur )
	{
		if ( *cur == '\\' )
		{
			const Uint32 length = (const Uint32) (cur - start);
			if ( !length )
				return nullptr;

			// create sub directory
			dir = dir->CreateNewDirectory( start, length );
			if ( !dir )
				return nullptr;

			// start new segment
			start = cur+1;
		}

		++cur;
	}

	// the rest is the file name
	if ( fileNamePtr )
		*fileNamePtr = start;

	// return final directory
	return dir;
}

//*************************************
//
CDirectory* CDirectory::FindPath( const String& path ) const
{
	DepotHelpers::ValidatePath( path.AsChar(), true );
	return FindPath( path.AsChar() );
}

//*************************************
//
CDirectory* CDirectory::FindPath( const Char *path, const Char** fileNamePtr /*= nullptr*/ ) const
{
	DepotHelpers::ValidatePath( path, true );

	CDirectory* dir = const_cast< CDirectory* >( this );

	// split the path and create the directories
	const Char* start = path;
	const Char* cur = path;
	while ( *cur )
	{
		if ( *cur == '\\' )
		{
			const Uint32 length = (const Uint32) (cur - start);
			if ( !length )
				return nullptr;

			// create sub directory
			dir = dir->FindLocalDirectory( start, length );
			if ( !dir )
				return nullptr;

			// start new segment
			start = cur+1;
		}

		++cur;
	}

	// the rest is the file name
	if ( fileNamePtr )
		*fileNamePtr = start;

	// return final directory
	return dir;
}

//*************************************
//
void CDirectory::CollectFiles( TDynArray< class CDiskFile* >& outFiles, const String& extensionFilter, const Bool recursive /*= true*/, const Bool onlyPopulated /*= true*/ ) const
{
	if ( !onlyPopulated )
		NeedChildren();

	// collect files
	for ( CDiskFile* file : m_files )
	{
		if ( extensionFilter.Empty() || file->GetFileName().EndsWith( extensionFilter ) )
		{
			outFiles.PushBack( file );
		}
	}

	// recurse to sub directories
	if ( recursive )
	{
		for ( CDirectory* dir : m_directories )
		{
			dir->CollectFiles( outFiles, extensionFilter, recursive, onlyPopulated );
		}
	}
}

//*************************************
//
void CDirectory::Unmap()
{
	// unmap sub directories
	for ( CDirectory* subDir : m_directories )
	{
		subDir->Unmap();
		delete subDir;
	}

	// unload files
	for ( CDiskFile* file : m_files )
	{
		file->Unload();
		delete file;
	}

	// cleanup
	m_directories.Clear();
	m_files.Clear();

	// mark as unpopulated
	m_populated = false;
}

//*************************************
// Create directory on disk
void CDirectory::Remap( const String& newAbsolutePath, const Bool forceRescan /*= true*/ )
{
	String currentOverride;
	if ( m_overridePath )
		currentOverride = *m_overridePath;

	if ( currentOverride != newAbsolutePath )
	{
		// cleanup current stuff
		Unmap();

		// delete current override mapping
		delete m_overridePath;
		m_overridePath = nullptr;

		// map to new location
		if ( !newAbsolutePath.Empty() )
		{
			m_overridePath = new String( newAbsolutePath );
		}

		// populate with stuff at the new location
		Repopulate( forceRescan );
	}
}

//*************************************
// Create directory on disk
Bool CDirectory::CreateOnDisk() const
{
	// Create directory on local disk
	const String absoluteDirPath = GetAbsolutePath();
	const Bool success = GFileManager->CreatePath( absoluteDirPath );
	return success;
}

//*************************************
//
Bool CDirectory::ConvertToLocalPath( const String& absolutePath, String& localPath )
{
	// Strip absolute path of this directory
	StringBuffer<512> absolutePathBuf( absolutePath );

	StringBuffer<512> directoryAbsolutePathBuf( GetAbsolutePath().AsChar() );

	absolutePathBuf.ToLower();
	directoryAbsolutePathBuf.ToLower();
	absolutePathBuf[ directoryAbsolutePathBuf.Size() ] = 0;

	// find if it begins with 
	if ( absolutePathBuf == directoryAbsolutePathBuf )
	{
		localPath = absolutePath.ToLower().StringAfter( GetAbsolutePath().ToLower() );
		return true;
	}

	// Not a local path
	return false;
}

//*************************************
//
String CDirectory::GetNameFromDepotPath( const String& depotPath ) const
{
	String path;
	if ( depotPath.GetLength() && depotPath[ depotPath.GetLength() - 1 ] == '\\' )
	{
		path = depotPath.LeftString( depotPath.GetLength() - 1 );
	}
	else
	{
		path = depotPath;
	}
	size_t index;
	path.FindSubstring( TXT("\\"), index, true );

	return path.RightString( ( ( path.GetLength() - index ) - 1 ) );
}
