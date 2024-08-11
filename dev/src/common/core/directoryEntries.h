/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "sortedmap.h"

//-----

/// File list
class CDirectoryFileList
{
public:
	typedef Uint32 TFileHash;
	typedef TSortedMap< TFileHash, CDiskFile*, DefaultCompareFunc< Uint32 >, MC_Depot >		TFiles;

	RED_INLINE CDirectoryFileList()
		: m_rehashNeeded( 0 )
	{}

	// Cleanup
	void Clear();

	// Cleanup and delete
	void ClearPtr();

	// Add to list
	void Add( CDiskFile* file, const Bool batch );

	// Delete from list
	void Remove( CDiskFile* file );

	// Find file entry by name
	CDiskFile* Find( const String& fileName ) const;

	// Find file entry by name
	CDiskFile* Find( const Char* fileName, const Uint32 length ) const;

	// Iterator wrapper
	class const_iterator
	{
	public:
		RED_INLINE const_iterator() {};			
		RED_INLINE const_iterator( const TFiles::const_iterator& it ) : m_iterator( it ) {};
		RED_INLINE CDiskFile* operator*() const { return (*m_iterator).m_second; }
		RED_INLINE Bool operator==( const const_iterator& it ) const { return m_iterator == it.m_iterator; }
		RED_INLINE Bool operator!=( const const_iterator& it ) const { return m_iterator != it.m_iterator; }
		RED_INLINE void operator++() { ++m_iterator; }
		RED_INLINE void operator++( Int32 ) { ++m_iterator; }

	private:
		TFiles::const_iterator	m_iterator;
	};

	// Iteration ranges
	RED_INLINE const_iterator Begin() const { EnsureSorted(); return const_iterator( m_files.Begin() ); }
	RED_INLINE const_iterator End() const { EnsureSorted(); return const_iterator( m_files.End() ); }

	// Calculate file hash
	RED_INLINE static const TFileHash CalcHash( const String& fileName )
	{
		Uint32 hash = 0;
		fileName.SimpleHash( hash );
		return hash;
	}

	// Calculate file hash from C string
	RED_INLINE static const TFileHash CalcHash( const Char* fileName, const Uint32 length )
	{
		return String::SimpleHash( fileName, length );
	}

	// Basic access
	RED_INLINE const Uint32 Size() const
	{
		return m_files.Size();
	}

	// Shrink data
	RED_INLINE void Shrink()
	{
		return m_files.Shrink();
	}

private:
	mutable TFiles		m_files;
	mutable Uint8		m_rehashNeeded;

	RED_INLINE void EnsureSorted() const
	{
		if ( m_rehashNeeded )
		{
			m_files.Resort();
			m_rehashNeeded = 0;
		}
	}
};

// range for
RED_FORCE_INLINE CDirectoryFileList::const_iterator begin( const CDirectoryFileList& map ) { return map.Begin(); }
RED_FORCE_INLINE CDirectoryFileList::const_iterator end( const CDirectoryFileList& map ) { return map.End(); }

//-----

/// Directory list
class CDirectoryDirList
{
public:
	typedef Uint32 TDirectoryHash;
	typedef TSortedMap< TDirectoryHash, CDirectory*, DefaultCompareFunc< Uint32 >, MC_Depot >		TDirectories;

	RED_INLINE CDirectoryDirList()
		: m_rehashNeeded( 0 )
	{}

	// Cleanup
	void Clear();

	// Cleanup and delete
	void ClearPtr();

	// Add to list
	void Add( CDirectory* file, const Bool batch );

	// Delete from list
	void Remove( CDirectory* file );

	// Find directory entry by name
	CDirectory* Find( const String& directoryName ) const;

	// Find directory entry by name
	CDirectory* Find( const Char* directoryName, const Uint32 length ) const;

	// Iterator wrapper
	class const_iterator
	{
	public:
		RED_INLINE const_iterator() {};			
		RED_INLINE const_iterator( const TDirectories::const_iterator& it ) : m_iterator( it ) {};
		RED_INLINE CDirectory* operator*() const { return (*m_iterator).m_second; }
		RED_INLINE Bool operator==( const const_iterator& it ) const { return m_iterator == it.m_iterator; }
		RED_INLINE Bool operator!=( const const_iterator& it ) const { return m_iterator != it.m_iterator; }
		RED_INLINE void operator++() { ++m_iterator; }
		RED_INLINE void operator++( Int32 ) { ++m_iterator; }

	private:
		TDirectories::const_iterator	m_iterator;
	};

	// Iteration ranges
	RED_INLINE const_iterator Begin() const { EnsureSorted(); return const_iterator( m_directories.Begin() ); }
	RED_INLINE const_iterator End() const { EnsureSorted(); return const_iterator( m_directories.End() ); }

	// Calculate file hash
	RED_INLINE static const TDirectoryHash CalcHash( const String& directoryName )
	{
		Uint32 hash = 0;
		directoryName.SimpleHash( hash );
		return hash;
	}

	// Calculate file hash from C string
	RED_INLINE static const TDirectoryHash CalcHash( const Char* directoryName, const Uint32 length )
	{
		return String::SimpleHash( directoryName, length );
	}

	// Basic access
	RED_INLINE const Uint32 Size() const
	{
		return m_directories.Size();
	}

	// Shrink data
	RED_INLINE void Shrink()
	{
		return m_directories.Shrink();
	}

private:
	mutable TDirectories	m_directories;
	mutable Uint8			m_rehashNeeded;

	RED_INLINE void EnsureSorted() const
	{
		if ( m_rehashNeeded )
		{
			m_directories.Resort();
			m_rehashNeeded = 0;
		}
	}
};

// range for
RED_FORCE_INLINE CDirectoryDirList::const_iterator begin( const CDirectoryDirList& map ) { return map.Begin(); }
RED_FORCE_INLINE CDirectoryDirList::const_iterator end( const CDirectoryDirList& map ) { return map.End(); }

//-----



