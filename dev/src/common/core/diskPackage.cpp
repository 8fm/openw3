/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "directory.h"
#include "diskPackage.h"
#include "fileSys.h"
#include "fileLatentLoadingToken.h"

CDiskPackage::CDiskPackage( const String& packageFileName )
	: m_packageFileName( packageFileName )
	, m_handle( nullptr )
{
	// spin lock
	m_mutex.SetSpinCount( 5000 ); // experimental value
}

#if defined( RED_PLATFORM_WINPC ) && ! defined( RED_FINAL_BUILD )
Bool CDiskPackage::Install( CDirectory* installDirectory, Bool isAbsolutePath /*= false*/ )
#else
Bool CDiskPackage::Install( CDirectory* installDirectory )
#endif
{
#if defined( RED_PLATFORM_WINPC ) && ! defined( RED_FINAL_BUILD )
	const Uint32 openFlags = FOF_Buffered | ( isAbsolutePath ? FOF_AbsolutePath : 0 );
#else
	const Uint32 openFlags = FOF_Buffered;
#endif

	// open the pack file
	IFile* packFile = GFileManager->CreateFileReader( m_packageFileName.AsChar(), openFlags );
	if ( nullptr == packFile )
	{
		WARN_CORE( TXT("DirectoryPackage '%ls' could not be opened"), m_packageFileName.AsChar() );
		return false;
	}

	// load file header
	FileHeader header;
	*packFile << header.m_magic;
	*packFile << header.m_numFiles;
	*packFile << header.m_listOffset;

	// not a pack file
	if ( header.m_magic != 'PACK' )
	{
		const Bool isEmpty = packFile->GetSize() < 1;
		if ( isEmpty )
		{
			WARN_CORE( TXT("DirectoryPackage '%ls' is zero bytes. Package file or was not copied over or created properly"), m_packageFileName.AsChar() );
		}
		else
		{
			WARN_CORE( TXT("DirectoryPackage '%ls' is not a valid package file or was not created properly"), m_packageFileName.AsChar() );
		}
		delete packFile;
		return false;
	}

	// load file list
	packFile->Seek( header.m_listOffset );
	m_files.Resize( header.m_numFiles );
	for ( Uint32 i=0; i<header.m_numFiles; ++i )
	{
		FileEntry& entry = m_files[i];
		*packFile << entry.m_path;
		*packFile << entry.m_offset;
		*packFile << entry.m_size;
		*packFile << entry.m_time;
	}

	// stats
	LOG_CORE( TXT("DirectoryPackage '%ls' loaded with %d files"), m_packageFileName.AsChar(), m_files.Size() );

	// create the disk package entries
	for ( Uint32 i=0; i<m_files.Size(); ++i )
	{
		const String& filePath = m_files[i].m_path;

		// create a directory, mark it as unpopulated to support it being split among multiple packs
		CDirectory* dir = installDirectory->CreatePath( filePath );
		if ( nullptr != dir )
		{
			// extract file name (no directory name)
			const String fileName = filePath.StringAfter( TXT("\\"), true );

			// add package file entry
			//CDiskFile* file = new CDiskFile( dir, fileName, CDiskFile::Type_PackageEntry, i, this );
			//dir->AddFile( file );
		}
	}

	// save the pack file handle
	m_packageDepotDirectory = installDirectory->GetDepotPath();
	m_handle = packFile;
	return true;
}

CDiskPackage::~CDiskPackage()
{
	if ( nullptr != m_handle )
	{
		delete m_handle;
		m_handle = nullptr;
	}
}

class FPackageFileReader : public IFile
{
protected:
	String					m_name;
	IFile*					m_handle;
	Uint64					m_baseOffset;
	Uint64					m_currentOffset;
	Uint64					m_fileSize;
	Red::Threads::CMutex*	m_handleLock;

public:
	FPackageFileReader( const String& name, IFile* handle, const Uint64 baseOffset, const Uint64 currentOffset, const Uint64 fileSize, Red::Threads::CMutex* lock )
		: IFile( FF_FileBased | FF_Reader )
		, m_handle( handle )
		, m_handleLock( lock )
		, m_name( name )
		, m_fileSize( fileSize )
		, m_baseOffset( baseOffset )
		, m_currentOffset( currentOffset )
	{
	}

	~FPackageFileReader()
	{
	}

	// Serialize data buffer of given size
	virtual void Serialize( void* buffer, size_t size )
	{
		m_handleLock->Acquire();

		m_handle->Seek( m_currentOffset );
		m_handle->Serialize( buffer, size );
		m_currentOffset += size;

		m_handleLock->Release();
	}

	// Get position in file stream
	virtual Uint64 GetOffset() const
	{
		return m_currentOffset - m_baseOffset;
	}

	// Get size of the file stream
	virtual Uint64 GetSize() const
	{
		return m_fileSize;
	}

	// Seek to file position
	virtual void Seek( Int64 offset )
	{
		m_currentOffset = m_baseOffset + offset;
	}

	// Get debug file name
	virtual const Char *GetFileNameForDebug() const
	{
		return m_name.AsChar();
	}

	// Create latent loading token for current file position
	virtual IFileLatentLoadingToken* CreateLatentLoadingToken( Uint64 currentOffset );
};

/// Reading token
class PackageLatentLoadingToken : public IFileLatentLoadingToken
{
protected:
	String					m_name;
	IFile*					m_handle;
	Uint64					m_baseOffset;
	Uint64					m_currentOffset;
	Uint64					m_fileSize;
	Red::Threads::CMutex*	m_handleLock;

public:
	//! Constructor
	PackageLatentLoadingToken( const String& name, IFile* handle, const Uint64 baseOffset, const Uint64 currentOffset, const Uint64 fileSize, Red::Threads::CMutex* lock )
		: IFileLatentLoadingToken( currentOffset - baseOffset )
		, m_handle( handle )
		, m_handleLock( lock )
		, m_name( name )
		, m_fileSize( fileSize )
		, m_baseOffset( baseOffset )
		, m_currentOffset( currentOffset )
	{}

	virtual IFileLatentLoadingToken* Clone() const
	{
		return new PackageLatentLoadingToken( m_name, m_handle, m_baseOffset, m_currentOffset, m_fileSize, m_handleLock );
	}

	//! Describe loading token
	virtual String GetDebugInfo() const
	{
		return String::Printf( TXT("PackageFile '%ls', offset %") RED_PRIWu64, m_name.AsChar(), m_currentOffset );
	}

	//! Resume loading
	virtual IFile* Resume( Uint64 relativeOffset )
	{
		return new FPackageFileReader( m_name, m_handle, m_baseOffset, m_currentOffset + relativeOffset, m_fileSize, m_handleLock );
	}
};

IFileLatentLoadingToken* FPackageFileReader::CreateLatentLoadingToken( Uint64 currentOffset )
{
	return new PackageLatentLoadingToken( m_name, m_handle, m_baseOffset, m_baseOffset + currentOffset, m_fileSize, m_handleLock );
}

IFile* CDiskPackage::CreateReader( Int32 packageFileIndex )
{
	// get the file entry
	if ( packageFileIndex < 0 || packageFileIndex >= (Int32) m_files.Size() )
	{
		return nullptr;
	}

	// get the file entry
	const FileEntry& entry = m_files[ packageFileIndex ];
	const String fullDepotPath = m_packageDepotDirectory + entry.m_path;
	return new FPackageFileReader( fullDepotPath, m_handle, entry.m_offset, entry.m_offset, entry.m_size, &m_mutex );
}

const CDateTime& CDiskPackage::GetFileTime( Int32 packageFileIndex )
{
	// get the file entry
	if ( packageFileIndex < 0 || packageFileIndex >= (Int32) m_files.Size() )
	{
		return CDateTime::INVALID;
	}

	// return file time
	const FileEntry& entry = m_files[ packageFileIndex ];
	return entry.m_time;
}
