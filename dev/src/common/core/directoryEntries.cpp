/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "diskFile.h"
#include "directory.h"
#include "directoryEntries.h"

//---

void CDirectoryFileList::Clear()
{
	m_files.Clear();
}

void CDirectoryFileList::ClearPtr()
{
	m_files.ClearPtr();
}

void CDirectoryFileList::Add( CDiskFile* file, const Bool batch )
{
	const TFileHash hash = CalcHash( file->GetFileName() );

	if ( batch )
	{
		// check file system collisions
		if ( !GFileManager->IsReadOnly() )
		{
			for ( auto it = m_files.Begin(); it != m_files.End(); ++it )
			{
				RED_FATAL_ASSERT( (*it).m_first != hash, "Fatal key collision '%ls' vs '%ls'", (*it).m_second->GetFileName().AsChar(), file->GetFileName().AsChar() );
			}
		}

		m_files.BulkInsert( hash, file );
		m_rehashNeeded = 1;
	}
	else
	{
		m_files.Insert( hash, file );
	}
}

void CDirectoryFileList::Remove( CDiskFile* file )
{
	const TFileHash hash = CalcHash( file->GetFileName() );

	if ( m_rehashNeeded )
	{
		m_rehashNeeded = 0;
		m_files.Resort();
	}

	TFiles::iterator it = m_files.Find( hash );
	if ( it != m_files.End() )
	{
		m_files.Erase( it );
	}
}

CDiskFile* CDirectoryFileList::Find( const String& fileName ) const
{
	const TFileHash hash = CalcHash( fileName );

	if ( m_rehashNeeded )
	{
		m_rehashNeeded = 0;
		m_files.Resort();
	}

	CDiskFile* file = nullptr;
	m_files.Find( hash, file );
	return file;
}

CDiskFile* CDirectoryFileList::Find( const Char* fileName, const Uint32 length ) const
{
	const TFileHash hash = CalcHash( fileName, length );

	if ( m_rehashNeeded )
	{
		m_rehashNeeded = 0;
		m_files.Resort();
	}

	CDiskFile* file = nullptr;
	m_files.Find( hash, file );
	return file;
}

//---

void CDirectoryDirList::Clear()
{
	m_directories.Clear();
}

void CDirectoryDirList::ClearPtr()
{
	m_directories.ClearPtr();
}

void CDirectoryDirList::Add( CDirectory* dir, const Bool batch )
{
	const TDirectoryHash hash = CalcHash( dir->GetName() );

	if ( batch )
	{
		m_directories.BulkInsert( hash, dir );
		m_rehashNeeded = 1;
	}
	else
	{
		m_directories.Insert( hash, dir );
	}
}

void CDirectoryDirList::Remove( CDirectory* dir )
{
	const TDirectoryHash hash = CalcHash( dir->GetName() );

	if ( m_rehashNeeded )
	{
		m_rehashNeeded = 0;
		m_directories.Resort();
	}

	TDirectories::iterator it = m_directories.Find( hash );
	if ( it != m_directories.End() )
	{
		m_directories.Erase( it );
	}
}

CDirectory* CDirectoryDirList::Find( const String& directoryName ) const
{
	const TDirectoryHash hash = CalcHash( directoryName );

	if ( m_rehashNeeded )
	{
		m_rehashNeeded = 0;
		m_directories.Resort();
	}

	CDirectory* dir = nullptr;
	m_directories.Find( hash, dir );
	return dir;
}

CDirectory* CDirectoryDirList::Find( const Char* directoryName, const Uint32 length ) const
{
	const TDirectoryHash hash = CalcHash( directoryName, length );

	if ( m_rehashNeeded )
	{
		m_rehashNeeded = 0;
		m_directories.Resort();
	}

	CDirectory* dir = nullptr;
	m_directories.Find( hash, dir );
	return dir;
}

//---
