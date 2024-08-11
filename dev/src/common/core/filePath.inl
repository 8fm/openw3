/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#ifndef __FILE_PATH_INL__
#define __FILE_PATH_INL__

//////////////////////////////////////////////////////////////////////////
RED_INLINE Bool CFilePath::HasFilename() const
{
	return m_filename.length > 0;
}

RED_INLINE Bool CFilePath::HasExtension() const
{
	return m_extension.length > 0;
}

RED_INLINE Uint32 CFilePath::GetNumberOfDirectories() const
{
	return m_directories.Size();
}
	
RED_INLINE Bool CFilePath::GetDirectory( Uint32 directoryIndex, TChar* buffer, Uint32 bufferSize ) const
{
	RED_FATAL_ASSERT( directoryIndex < m_directories.Size(), "Invalid directory index specified %u/%u", directoryIndex, m_directories.Size() );
	return Red::System::StringCopy( buffer, &m_path[ m_directories[ directoryIndex ].offset ], bufferSize, m_directories[ directoryIndex ].length );
}

RED_INLINE Bool CFilePath::GetFilenameWithExt( TChar* buffer, Uint32 bufferSize ) const
{
	Uint32 sizeToCopy = m_filename.length;
		
	if( m_extension.length > 0 )
	{
		sizeToCopy += m_extension.length + 1;
	}

	return Red::System::StringCopy( buffer, &m_path[ m_filename.offset ], bufferSize, sizeToCopy );
}

RED_INLINE void CFilePath::PopDirectory()
{
	RED_FATAL_ASSERT( m_directories.Size() > 0, "Cannot pop: No directories left" );

	Uint32 lastDirectoryIndex = m_directories.Size() - 1;
	CollapseDirectories( lastDirectoryIndex, lastDirectoryIndex );
}

RED_INLINE const CFilePath::PathString& CFilePath::ToString() const
{
	return m_path;
}

//////////////////////////////////////////////////////////////////////////
RED_INLINE CFilePath::Section::Section()
:	offset( 0 )
,	length( 0 )
{
}

#endif // __FILE_PATH_INL__
