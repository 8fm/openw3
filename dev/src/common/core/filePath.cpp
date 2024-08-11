/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "filePath.h"

CFilePath::CFilePath()
{

}

CFilePath::CFilePath( const TChar* path )
:	m_path( path )
{
	Deconstruct();

	m_path.MakeLower();
}

CFilePath::CFilePath( const PathString& path )
:	m_path( path )
{
	Deconstruct();

	m_path.MakeLower();
}

CFilePath::CFilePath( const CFilePath& path )
:	m_path( path.m_path )
,	m_directories( path.m_directories )
,	m_filename( path.m_filename )
{

}

CFilePath::CFilePath( const CFilePath&& path )
:	m_path( std::move( path.m_path ) )
,	m_directories( std::move( path.m_directories ) )
,	m_filename( std::move( path.m_filename ) )
{

}

void CFilePath::operator=( const PathString& path )
{
	m_path = path;

	Deconstruct();

	m_path.MakeLower();
}

CFilePath::~CFilePath()
{

}

//////////////////////////////////////////////////////////////////////////
// Deconstruction
void CFilePath::Deconstruct()
{
	Uint32 firstCharAfterRoot = static_cast< Uint32 >( DeconstructRoot() );
	
	Uint32 firstCharAfterDirectories = DeconstructDirectories( firstCharAfterRoot );

	DeconstructFilename( firstCharAfterDirectories );

	ConformSeparators();
}

size_t CFilePath::DeconstructRoot() const
{
	// These sizes are returned like this so that you can see where the values come from
	// The actual contents are irrelevant as it's only the size that matters, which is resolved at compile time
	if( IsAbsoluteWindows() )
	{
		return Red::System::StringLengthCompileTime( "C:\\" );
	}
	else if( IsAbsoluteSamba() )
	{
		return Red::System::StringLengthCompileTime( "\\\\" );
	}
	else if( IsAbsolutePosix() )
	{
		return Red::System::StringLengthCompileTime( "/" );
	}
	else if( IsURL() )
	{
		return Red::System::StringLengthCompileTime( "http://" );
	}

	return 0;
}

Bool CFilePath::IsAbsoluteWindows() const
{
	static const TChar windowsDriveSansLetter[] = { RED_TEMPLATE_TXT( TChar, ':' ), RED_TEMPLATE_TXT( TChar, DIRECTORY_SEPARATOR_LITERAL ), RED_TEMPLATE_TXT( TChar, '\0' ) };

	// Skip over the drive letter
	return
		( m_path.GetLength() > Red::System::StringLengthCompileTime( "C:\\" ) ) &&
		(
			Red::System::MemoryCompare
			(
				&m_path[ 1 ],
				windowsDriveSansLetter,
				Red::System::StringLengthCompileTime( ":\\" ) * sizeof( TChar )
			) == 0
		);
}

Bool CFilePath::IsAbsolutePosix() const
{
	return ( m_path.GetLength() > 0 ) && ( m_path[ 0 ] == RED_TEMPLATE_TXT( TChar, DIRECTORY_SEPARATOR_LITERAL ) );
}

Bool CFilePath::IsAbsoluteSamba() const
{
	return
		( m_path.GetLength() > 1 ) &&
		( m_path[ 0 ] == RED_TEMPLATE_TXT( TChar, DIRECTORY_SEPARATOR_LITERAL ) ) &&
		( m_path[ 1 ] == RED_TEMPLATE_TXT( TChar, DIRECTORY_SEPARATOR_LITERAL ) );
}

Bool CFilePath::IsURL() const
{
	return
		( m_path.GetLength() > Red::System::StringLengthCompileTime( "http://" ) ) &&
		( Red::System::MemoryCompare( m_path.AsChar(), RED_TEMPLATE_TXT( TChar, "http://" ), Red::System::StringLengthCompileTime( "http://" ) * sizeof( TChar ) ) == 0 );
}

Uint32 CFilePath::CountDirectories( Uint32 start ) const
{
	Uint32 numSeparators = 0;

	const Uint32 pathLength = m_path.GetLength();
	for( Uint32 i = start; i < pathLength; ++i )
	{
		if( CFileManager::IsDirectorySeparator( m_path[ i ] ) )
		{
			++numSeparators;
		}
	}

	return numSeparators;
}

Uint32 CFilePath::DeconstructDirectories( Uint32 start )
{
	Uint32 numDirectories = CountDirectories( start );
	Uint32 currentDirectory = 0;

	m_directories.Resize( numDirectories );

	Uint32 directoryStart = start;

	const Uint32 pathLength = m_path.GetLength();
	for( Uint32 i = start; i < pathLength; ++i )
	{
		if( CFileManager::IsDirectorySeparator( m_path[ i ] ) )
		{
			m_directories[ currentDirectory ].offset = directoryStart;
			m_directories[ currentDirectory ].length = i - directoryStart;

			directoryStart = i + 1;
			++currentDirectory;
		}
	}

	return directoryStart;
}

void CFilePath::DeconstructFilename( Uint32 start )
{
	m_filename.offset = start;

	Uint32 pathLength = m_path.GetLength();
	if( start < pathLength )
	{
		size_t position;

		// We need to search from the right as it's valid for a filename to contain many dots, but it's only the rightmost we care about
		if( m_path.FindCharacter( RED_TEMPLATE_TXT( TChar, '.' ), position, static_cast< size_t >( start ), pathLength, true ) )
		{
			m_filename.length = static_cast< Uint32 >( position ) - start;
			
			m_extension.offset = static_cast< Uint32 >( position ) + 1;
			m_extension.length = pathLength - m_extension.offset;
		}
		else
		{
			m_filename.length = pathLength - start;

			m_extension.offset = pathLength;
			m_extension.length = 0;
		}
	}
	else
	{
		m_filename.length = 0;
	}
}

void CFilePath::ConformSeparators()
{
	TChar src = RED_TEMPLATE_TXT( TChar, ALTERNATIVE_DIRECTORY_SEPARATOR_LITERAL );
	TChar target = RED_TEMPLATE_TXT( TChar, DIRECTORY_SEPARATOR_LITERAL );

	m_path.ReplaceAll( src, target );

	if( m_directories.Size() > 0 )
	{
		// Remove trailing slash
		m_path.TrimRight( target );
	}
}

//////////////////////////////////////////////////////////////////////////
// Accessors
CFilePath::PathString CFilePath::GetExtension() const
{
	if( m_extension.length > 0 )
	{
		return &m_path[ m_extension.offset ];
	}

	return PathString::EMPTY;
}

CFilePath::PathString CFilePath::GetFileName() const
{
	if( m_filename.length > 0 )
	{
		return PathString( &m_path[ m_filename.offset ], m_filename.length );
	}

	return PathString::EMPTY;
}

CFilePath::PathString CFilePath::GetFileNameWithExt() const
{
	PathString filename( std::move( GetFileName() ) );

	if( m_extension.length > 0 )
	{
		// Expand the extension section to utilise the dot that should already be a part of the path
		RED_ASSERT( m_extension.offset > 0 && m_path[ m_extension.offset - 1 ] == RED_TEMPLATE_TXT( TChar, '.' ) );
		filename.Append( &m_path[ m_extension.offset - 1 ], m_extension.length + 1 );
	}

	return filename;
}

TDynArray< CFilePath::PathString > CFilePath::GetDirectories() const
{
	TDynArray< PathString > directories;
	GetDirectories( directories );
	return directories;
}

void CFilePath::GetDirectories( TDynArray< PathString >& directories ) const
{
	directories.Resize( m_directories.Size() );

	for( Uint32 i = 0; i < m_directories.Size(); ++i )
	{
		directories[ i ].Set( &m_path[ m_directories[ i ].offset ], m_directories[ i ].length );
	}
}

CFilePath::PathString CFilePath::GetPathString( Bool trailingSeparator ) const
{
	if( m_filename.length > 0 )
	{
		if( m_filename.offset == 0 )
		{
			return PathString::EMPTY;
		}
		else
		{
			Uint32 offset = ( trailingSeparator )? m_filename.offset : m_filename.offset - 1;
			return PathString( m_path.AsChar(), offset );
		}
		return PathString( m_path.AsChar(), m_filename.offset );
	}
	else
	{
		if( m_path.GetLength() > 0 && trailingSeparator )
		{
			// -2 to account for '.' and DIRECTORY_SEPARATOR_LITERAL.
			Uint32 offset = ( m_extension.length > 0 )? m_extension.offset - 2 : m_path.GetLength();
			PathString path( m_path.AsChar(), offset + 1 );
			path[ offset ] = RED_TEMPLATE_TXT( TChar, DIRECTORY_SEPARATOR_LITERAL );
			return path;
		}
		else
		{
			// -2 to account for '.' and DIRECTORY_SEPARATOR_LITERAL.
			Uint32 dirOffset = ( m_directories.Size() > 0 )? 1 : 0;
			Uint32 offset = ( m_extension.length > 0 )? m_extension.offset - 1 - dirOffset : m_path.GetLength();
			PathString path( m_path.AsChar(), offset );
			return path;
		}
	}
}

Bool CFilePath::IsRelative() const
{
	// If path is not initialized, then return true (technically it could refer to the current directory)
	// Otherwise, if the first character of the path is a dot, then it must either be "this directory" or "parent directory"
	// Finally, check if it's just a filename
	return
		( m_path.GetLength() == 0 ) ||
		( m_directories.Size() == 0 ) ||
		( !IsAbsoluteWindows() && !IsAbsoluteSamba() && !IsAbsolutePosix() && !IsURL() );
}

void CFilePath::SetFileName( const PathString& filename )
{
	// We will need to append a trailing separator if there are any directories and no filename
	if( m_directories.Size() > 0 )
	{
		size_t newFilenameLength = filename.GetLength();

		// +1 for the directory separator, +1 for '.' if needed, +1 for the null terminator
		size_t length = 1 + newFilenameLength + ( m_extension.length > 0 ? m_extension.length + 1 : 0 ) + 1;
		size_t size = length * sizeof( TChar );
		TChar* intermediary = static_cast< TChar* >( RED_ALLOCA( size ) );

		// Insert the divider between the final directory and the file
		intermediary[ 0 ] = RED_TEMPLATE_TXT( TChar, DIRECTORY_SEPARATOR_LITERAL );
		intermediary[ 1 ] = RED_TEMPLATE_TXT( TChar, '\0' );

		// Remove the original filename.
		size_t resizedLength = m_path.Size( ) - m_filename.length;

		// Remove the original directory separator if any.
		if( m_filename.length > 0 || m_extension.length > 0 )
			--resizedLength;

		// Append the filename to the directory separator
		Red::System::StringConcatenate( intermediary, filename.AsChar(), length, newFilenameLength );

		// Add the filename to the path
		if( m_extension.length > 0 )
		{
			Red::System::StringConcatenate( intermediary, TXT( "." ), length, 1 );
			Red::System::StringConcatenate( intermediary, GetExtension().AsChar(), length, m_extension.length );

			// Remove the original '.' and extension.
			resizedLength = resizedLength - 1 - m_extension.length;
		}

		// Remove original stuff and append new, -1 for null terminator.
		m_path.Resize( resizedLength );
		m_path.Append( intermediary, length - 1 );
	}
	else
	{
		// straight up swap, there are no directories to complicate matters
		m_path.Replace( GetFileName(), filename );
	}

	// Update the position of the extension
	m_extension.offset += filename.GetLength() - m_filename.length;

	// Update the length of the filename
	m_filename.length = filename.GetLength();
	if( m_filename.length > 0 )
	{
		m_path.MakeLower( m_filename.offset, m_filename.offset + m_filename.length );
	}

	// Check to make sure we haven't broken anything
	RED_ASSERT( filename.EqualsNC( GetFileName() ) == true );
}

void CFilePath::SetExtension( const PathString& extension )
{
	RED_FATAL_ASSERT( !extension.ContainsCharacter( RED_TEMPLATE_TXT( TChar, '.' ) ), "extension must not contain any dots ('.')" );

	if( m_extension.length == 0 )
	{
		// +1 for the null terminator and +1 for the extension's dot
		size_t length = extension.GetLength() + 2;
		size_t size = length * sizeof( TChar );
		TChar* intermediary = static_cast< TChar* >( RED_ALLOCA( size ) );

		intermediary[ 0 ] = RED_TEMPLATE_TXT( TChar, '.' );
		intermediary[ 1 ] = RED_TEMPLATE_TXT( TChar, '\0' );

		// Append the extension
		Red::System::StringConcatenate( intermediary, extension.AsChar(), length );

		m_extension.offset = m_path.GetLength() + 1;
		m_extension.length = extension.GetLength();

		m_path.Append( intermediary, m_extension.length + 1 );
	}
	else
	{
		m_path.Replace( GetExtension(), extension, true );

		m_extension.length = extension.GetLength();
	}

	if( m_extension.length > 0 )
	{
		m_path.MakeLower( m_extension.offset, m_extension.offset + m_extension.length );
	}
}

void CFilePath::PopFile()
{
	if( m_filename.length > 0 )
	{
		Uint32 pathDecrement = ( m_directories.Size() > 0 ) ? 1 : 0;
		m_path.Replace( &m_path[ m_filename.offset - pathDecrement ], TXT(""), true );
		m_extension.offset = m_path.GetLength() + 1;
		m_extension.length = 0;
		m_filename.offset = m_path.GetLength() + 1;
		m_filename.length = 0;
	}
}

Bool CFilePath::IsCurrentDirectoryShortcut( Uint32 index ) const
{
	RED_ASSERT( index < m_directories.Size(), TXT( "Invalid directory index specified %u/%u" ), index, m_directories.Size() );

	Section directory = m_directories[ index ];

	return directory.length == Red::System::StringLengthCompileTime( "." ) &&
		Red::System::StringCompare( &m_path[ directory.offset ], RED_TEMPLATE_TXT( TChar, "." ), Red::System::StringLengthCompileTime( "." ) ) == 0;
}

Bool CFilePath::IsParentDirectoryShortcut( Uint32 index ) const
{
	RED_ASSERT( index < m_directories.Size(), TXT( "Invalid directory index specified %u/%u" ), index, m_directories.Size() );
	
	Section directory = m_directories[ index ];
	
	return directory.length == Red::System::StringLengthCompileTime( ".." ) &&
		Red::System::StringCompare( &m_path[ directory.offset ], RED_TEMPLATE_TXT( TChar, ".." ), Red::System::StringLengthCompileTime( ".." ) ) == 0;
}

void CFilePath::PushDirectory( const PathString& directory )
{
	// +1 for the null terminator
	// +1 for the separator for the new directory
	// +1 for the potential second separator for the other side of the new directory
	size_t maxPathSize = m_path.GetLength() + directory.GetLength() + 3;
	TChar* newPath = static_cast< TChar* >( RED_ALLOCA( sizeof( TChar ) * maxPathSize ) );
	newPath[ 0 ] = RED_TEMPLATE_TXT( TChar, '\0' );

	Bool filenameExists = m_filename.length > 0;
	Bool hasDirectories = m_directories.Size() > 0;

	if( filenameExists && hasDirectories )
	{
		Uint32 frontHalfOfExistingPathToCopy = m_filename.offset;

		// Existing directories
		Red::System::StringConcatenate( newPath, &m_path[ 0 ], maxPathSize, frontHalfOfExistingPathToCopy );

		// New Directory
		Red::System::StringConcatenate( newPath, directory.AsChar(), maxPathSize );

		// New separator between new directory and filename
		Red::System::StringConcatenate( newPath, RED_TEMPLATE_TXT( TChar, DIRECTORY_SEPARATOR_LITERAL_STRING ), maxPathSize );

		// Filename
		Red::System::StringConcatenate( newPath, &m_path[ m_filename.offset ], maxPathSize );
	}
	else if( !filenameExists && hasDirectories )
	{
		Uint32 frontHalfOfExistingPathToCopy = m_directories.Back().offset + m_directories.Back().length + 1;

		// Existing directories
		Red::System::StringConcatenate( newPath, &m_path[ 0 ], maxPathSize, frontHalfOfExistingPathToCopy );

		// New separator between existing directories and new directory
		Red::System::StringConcatenate( newPath, RED_TEMPLATE_TXT( TChar, DIRECTORY_SEPARATOR_LITERAL_STRING ), maxPathSize );

		// New Directory
		Red::System::StringConcatenate( newPath, directory.AsChar(), maxPathSize );

		// Trailing separator to indicate that this is a directory not a filename
		Red::System::StringConcatenate( newPath, RED_TEMPLATE_TXT( TChar, DIRECTORY_SEPARATOR_LITERAL_STRING ), maxPathSize );
	}
	else if( filenameExists && !hasDirectories )
	{
		Uint32 frontHalfOfExistingPathToCopy = m_filename.offset;

		// Drive, if there is one
		Red::System::StringConcatenate( newPath, &m_path[ 0 ], maxPathSize, frontHalfOfExistingPathToCopy );

		// New Directory
		Red::System::StringConcatenate( newPath, directory.AsChar(), maxPathSize );

		// New separator between new directory and filename
		Red::System::StringConcatenate( newPath, RED_TEMPLATE_TXT( TChar, DIRECTORY_SEPARATOR_LITERAL_STRING ), maxPathSize );

		// Filename
		Red::System::StringConcatenate( newPath, &m_path[ m_filename.offset ], maxPathSize );
	}
	else if( m_path.GetLength() > 0 )
	{
		// Drive
		Red::System::StringConcatenate( newPath, &m_path[ 0 ], maxPathSize );

		// New Directory
		Red::System::StringConcatenate( newPath, directory.AsChar(), maxPathSize );

		// Trailing separator to indicate that this is a directory not a filename
		Red::System::StringConcatenate( newPath, RED_TEMPLATE_TXT( TChar, DIRECTORY_SEPARATOR_LITERAL_STRING ), maxPathSize );
	}
	else
	{
		// New Directory
		Red::System::StringConcatenate( newPath, directory.AsChar(), maxPathSize );

		// Trailing separator to indicate that this is a directory not a filename
		Red::System::StringConcatenate( newPath, RED_TEMPLATE_TXT( TChar, DIRECTORY_SEPARATOR_LITERAL_STRING ), maxPathSize );
	}

	m_path.Set( newPath );

	m_directories.ClearFast();
	Deconstruct();

	m_path.MakeLower();
}

void CFilePath::Normalize()
{
	Bool finished = false;

	do
	{
		const Uint32 numDirectories = m_directories.Size();
		for( Uint32 i = 1; i < numDirectories; ++i )
		{
			// Have we found a "."?
			if( IsCurrentDirectoryShortcut( i ) )
			{
				CollapseDirectories( i, i );
				break;
			}
			else if( IsParentDirectoryShortcut( i ) && !IsParentDirectoryShortcut( i - 1 ) )
			{
				CollapseDirectories( i - 1, i );
				break;
			}
			else if( i == numDirectories - 1 )
			{
				finished = true;
			}
		}

	} while( !finished && m_directories.Size() > 1 );
}

void CFilePath::CollapseDirectories( Uint32 start, Uint32 end )
{
	RED_ASSERT( start < m_directories.Size(), TXT( "Invalid start directory index specified %u/%u" ), start, m_directories.Size() );
	RED_ASSERT( end < m_directories.Size(), TXT( "Invalid end directory index specified %u/%u" ), end, m_directories.Size() );
	RED_ASSERT( start <= end );

	size_t maxPathLength = m_path.GetLength() + 1;
	TChar* newPath = static_cast< TChar* >( RED_ALLOCA( sizeof( TChar ) * maxPathLength ) );
	newPath[ 0 ] = RED_TEMPLATE_TXT( TChar, '\0' );

	if( start > 0 )
	{
		Red::System::StringConcatenate( newPath, &m_path[ 0 ], maxPathLength, m_directories[ start ].offset );
	}

	Uint32 trailingSlashSkip = 0;
	if( HasFilename() || HasExtension() )
	{
		++trailingSlashSkip;
	}

	Uint32 secondHalfStart = m_directories[ end ].offset + m_directories[ end ].length + trailingSlashSkip;
	Red::System::StringConcatenate( newPath, &m_path[ secondHalfStart ], maxPathLength );

	m_path.Set( newPath );

	m_directories.ClearFast();
	Deconstruct();
}

const String& CFilePath::ConformPath( const String& thePath, String& tempPath )
{
	// lower case only, one type of path separator
	Bool isConformed = true;
	const Uint32 strLen = thePath.GetLength();
	for ( Uint32 i=0; i<strLen; ++i )
	{
		const Char ch = thePath[i];

		// upper case
		if ( ch >= 'A' && ch <= 'Z' )
		{
			isConformed = false;
			break;
		}

		// invalid path separator
		if ( ch == InvalidPathSeparator )
		{
			isConformed = false;
			break;
		}
	}

	// path is already conformed, do nothing
	if ( isConformed )
		return thePath;

	// copy for modification
	tempPath = thePath;

	// make lower case + modify the path separators
	for ( Uint32 i=0; i<strLen; ++i )
	{
		const Char ch = tempPath[i];

		if ( ch >= 'A' && ch <= 'Z' )
		{
			tempPath[i] = (ch - 'A') + 'a';
			continue;
		}

		if ( ch == InvalidPathSeparator )
		{
			tempPath[i] = ValidPathSeparator;
			continue;
		}
	}

	// return modified string
	return tempPath;
}

const StringAnsi& CFilePath::ConformPath( const StringAnsi& thePath, StringAnsi& tempPath )
{
	// lower case only, one type of path separator
	Bool isConformed = true;
	const Uint32 strLen = thePath.GetLength();
	for ( Uint32 i=0; i<strLen; ++i )
	{
		const AnsiChar ch = thePath[i];

		// upper case
		if ( ch >= 'A' && ch <= 'Z' )
		{
			isConformed = false;
			break;
		}

		// invalid path separator
		if ( ch == InvalidPathSeparator )
		{
			isConformed = false;
			break;
		}
	}

	// path is already conformed, do nothing
	if ( isConformed )
		return thePath;

	// copy for modification
	tempPath = thePath;

	// make lower case + modify the path separators
	for ( Uint32 i=0; i<strLen; ++i )
	{
		const AnsiChar ch = tempPath[i];

		if ( ch >= 'A' && ch <= 'Z' )
		{
			tempPath[i] = (ch - 'A') + 'a';
			continue;
		}

		if ( ch == InvalidPathSeparator )
		{
			tempPath[i] = ValidPathSeparator;
			continue;
		}
	}

	// return modified string
	return tempPath;
}

void CFilePath::GetConformedPath( const String& depotPath, String& outPath )
{
	outPath = ConformPath( depotPath, outPath );
}

void CFilePath::GetConformedPath( const StringAnsi& depotPath, StringAnsi& outPath )
{
	outPath = ConformPath( depotPath, outPath );
}
