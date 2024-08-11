/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "packageManager.h"
#include "memory.h"
#include "string.h"
#include "datetime.h"

class CDirectory;

/// Packed directory
/// This is uncompressed structure for storing and keeping depot files
class CDiskPackage
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Depot );

protected:
	struct FileHeader
	{
		Uint32		m_magic;
		Uint32		m_numFiles;
		Uint32		m_listOffset;
	};

	struct FileEntry
	{
		String		m_path;		//!< Path relative to package root
		Uint32		m_offset;	//!< File placement offset
		Uint32		m_size;		//!< File size
		CDateTime	m_time;		//!< File time stamp
	};

	typedef TDynArray< FileEntry >	TFiles;
	TFiles							m_files;					//!< Files stored in disk package

	String							m_packageDepotDirectory;	//!< Depot path of the directory this package was installed in
	String							m_packageFileName;			//!< Depot path to package file

	IFile*							m_handle;					//!< File access handle

	Red::Threads::CMutex			m_mutex;					//!< Access mutex (only one thread can read at once)

public:
	//! Get package file path
	RED_INLINE const String& GetPackageFileName() const { return m_packageFileName; }

public:
	CDiskPackage( const String& packageFileName );
	virtual ~CDiskPackage();

	//! Installs package, returns false if format is invalid

#if defined( RED_PLATFORM_WINPC ) && ! defined( RED_FINAL_BUILD )
	Bool Install( CDirectory* installDirectory, Bool isAbsolutePath = false );
#else	
	Bool Install( CDirectory* installDirectory );
#endif

	//! Create package file reader
	IFile* CreateReader( Int32 packageFileIndex );

	//! Get last modification time for given file
	const CDateTime& GetFileTime( Int32 packageFileIndex );
};
