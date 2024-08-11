/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "hashmap.h"
#include "string.h"
#include "memory.h"

/// Manager responsible for collecting status about file loading, used mostly in editor for PC
class CFileLoadingStats
{
public:
	/// Per file stats
	struct FileStats
	{
		DECLARE_STRUCT_MEMORY_ALLOCATOR( MC_Engine );

		String		m_filePath;				//!< Path to file
		Float		m_openTime;				//!< Time spent opening file
		Float		m_tableLoadingTime;		//!< Time of loading file tables
		Float		m_objectCreationTime;	//!< Time needed for creating all objects
		Float		m_deserializationTime;	//!< Time spent serializing file content
		Float		m_postLoadTime;			//!< Time spent on post loading
		Uint32		m_numBlocks;			//!< Number of blocks serialized
		Uint32		m_bytesRead;			//!< Total bytes read from file
		CName		m_className;			//!< RTTI class name. Derived resources classes can have the same extension, so record class here.

		//! Constructor
		FileStats( const String& filePath );
	};

	/// Per class status
	struct ClassStats
	{
		DECLARE_STRUCT_MEMORY_ALLOCATOR( MC_Engine );

		const CClass*	m_class;				//!< Class object
		Uint32			m_numObjects;			//!< Number of created objects
		Float			m_creationTime;			//!< Time it took to create objects of this class
		Float			m_deserializationTime;	//!< Time it took to deserialize objects
		Float			m_postLoadTime;			//!< Time spent on post loading
		Uint32			m_numBlocks;			//!< Number of blocks serialized
		Uint32			m_bytesRead;			//!< Total number of bytes read in objects of this class

		//! Constructor
		ClassStats( const CClass* objectClass );
	};

private:
	typedef Red::Threads::CMutex CMutex;
	typedef Red::Threads::CScopedLock< CMutex > CScopedLock;

private:
	THashMap< String, FileStats* >			m_files;		//!< All loaded files
	THashMap< const CClass*, ClassStats* >	m_classes;		//!< All loaded classes
	mutable CMutex							m_mutex;

public:
	CFileLoadingStats();
	~CFileLoadingStats();

	//! Reset stats
	void Clear();

	//! Get file stats for given file, creates if not found
	FileStats* GetFileStats( const String& file, Bool create = true );

	//! Get file stats for given file, creates if not found
	ClassStats* GetClassStats( const CClass* objectClass, Bool create = true );

	THashMap< String, FileStats* > GetFileStatsCopy() const;
	THashMap< const CClass*, ClassStats* > GetClassStatsCopy() const;

	//! Dump stats
	void Dump() const;
};

// FIle stats manager singleton
typedef TSingleton< CFileLoadingStats > SFileLoadingStats;
