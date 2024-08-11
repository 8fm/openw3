/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#ifndef __FILE_PATH_H__
#define __FILE_PATH_H__

#include "string.h"
#include "dynarray.h"
#include "filesys.h"
#include "memory.h"

class CFilePath
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

public:
	typedef Char TChar;
	typedef TString< TChar > PathString;

	CFilePath();
	explicit CFilePath( const TChar* path );
	explicit CFilePath( const PathString& path );
	explicit CFilePath( const CFilePath& path );
	explicit CFilePath( const CFilePath&& path );

	void operator=( const PathString& path );

	~CFilePath();

	PathString GetExtension() const;
	PathString GetFileName() const;
	PathString GetFileNameWithExt() const;

	RED_INLINE Bool HasFilename() const;
	RED_INLINE Bool HasExtension() const;
	RED_INLINE Uint32 GetNumberOfDirectories() const;
	RED_INLINE Bool GetDirectory( Uint32 directoryIndex, TChar* buffer, Uint32 bufferSize ) const;
	RED_INLINE Bool GetFilenameWithExt( TChar* buffer, Uint32 bufferSize ) const;

	TDynArray< PathString > GetDirectories() const;
	void GetDirectories( TDynArray< PathString >& directories ) const;

	PathString GetPathString( Bool trailingSeparator = false ) const;
	RED_INLINE const PathString& ToString() const;

	Bool IsRelative() const;

	void SetFileName( const PathString& filename );
	void SetExtension( const PathString& extension );

	void PopFile();

	RED_INLINE void PopDirectory();

	void PushDirectory( const PathString& directory );

	void Normalize();

	// conform path(directory) name to standard (lower case, same path separator)
	// the additional temporary memory is allocated only if the initial path does not conform to the standard
	// USE ONLY ON THE DEPOT PATH (not tested with absolute paths)
	static const String& ConformPath( const String& depotPath, String& tempFile );
	static const StringAnsi& ConformPath( const StringAnsi& depotPath, StringAnsi& tempFile );

	static void GetConformedPath( const String& depotPath, String& outPath );
	static void GetConformedPath( const StringAnsi& depotPath, StringAnsi& outPath );

private:
	void Deconstruct();

	// Returns the length of the drive
	size_t DeconstructRoot() const;

	// "C:\"
	Bool IsAbsoluteWindows() const;

	// "/"
	Bool IsAbsolutePosix() const;

	// "\\"
	Bool IsAbsoluteSamba() const;

	// "HTTP://"
	Bool IsURL() const;

	Uint32 CountDirectories( Uint32 start ) const;

	Uint32 DeconstructDirectories( Uint32 start );

	void DeconstructFilename( Uint32 start );

	void ConformSeparators();

	Bool IsCurrentDirectoryShortcut( Uint32 index ) const;
	Bool IsParentDirectoryShortcut( Uint32 index ) const;
	void CollapseDirectories( Uint32 start, Uint32 end );

private:
	struct Section
	{
		Uint32 offset;
		Uint32 length;

		RED_INLINE Section();
	};

private:
	static const Char InvalidPathSeparator = '/';
	static const Char ValidPathSeparator = '\\';

	PathString m_path;
	TDynArray< Section > m_directories;
	Section m_filename;
	Section m_extension;
};

#include "filePath.inl"

#endif // __FILE_PATH_H__
