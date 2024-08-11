/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "exporter.h"

CSSExporter::CSSExporter( const wxString& source, const wxString& dest )
:	m_source( source )
,	m_dest( dest )
{

}

CSSExporter::~CSSExporter()
{

}

bool CSSExporter::IsDestinationEmpty() const
{
	wxDiskspaceSize_t destSize = 0;

	if( wxGetDiskSpace( m_dest, &destSize ) )
	{
		return destSize > 0;
	}

	// Probably doesn't exist?
	return true;
}

void CSSExporter::Export()
{
	wxDir dir( m_source );
	dir.Traverse( *this );
}

wxString CSSExporter::ConvertPath( const wxString& file, const wxString& source, const wxString& dest )
{
	wxFileName fixer( file );

	return ConvertPath( fixer, source, dest );
}

wxString CSSExporter::ConvertPath( wxFileName& fixer, const wxString& source, const wxString& dest )
{
	fixer.MakeRelativeTo( source );
	fixer.Normalize( wxPATH_NORM_ALL, dest );

	return fixer.GetFullPath();
}

wxDirTraverseResult CSSExporter::OnDir( const wxString& dir )
{
	wxString dest = ConvertPath( dir, m_source, m_dest );

	wxFileName::Mkdir( dest, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL );

	return wxDIR_CONTINUE;
}

wxDirTraverseResult CSSExporter::OnFile( const wxString& file )
{
	wxString dest = ConvertPath( file, m_source, m_dest );

	wxCopyFile( file, dest, true );

	return wxDIR_CONTINUE;
}

//////////////////////////////////////////////////////////////////////////
//
CSSDiffExporter::CSSDiffExporter( const wxString& source, const wxString& comp, const wxString& dest )
:	CSSExporter( source, dest )
,	m_comp( comp )
,	m_fileA( nullptr )
,	m_fileB( nullptr )
,	m_bufferSizeA( 0 )
,	m_bufferSizeB( 0 )
{

}

CSSDiffExporter::~CSSDiffExporter()
{
	if( m_fileA )
	{
		free( m_fileA );
	}

	if( m_fileB )
	{
		free( m_fileB );
	}
}

wxDirTraverseResult CSSDiffExporter::OnDir( const wxString& dir )  
{
	return wxDIR_CONTINUE;
}

wxDirTraverseResult CSSDiffExporter::OnFile( const wxString& file )
{
	wxString compFile = ConvertPath( file, m_source, m_comp );

	int fileSizeA;
	int fileSizeB;

	if( ReadFile( file.wc_str(), &m_fileA, m_bufferSizeA, fileSizeA ) &&	ReadFile( compFile.wc_str(), &m_fileB, m_bufferSizeB, fileSizeB ) )
	{
		if( fileSizeA != fileSizeB || Red::System::MemoryCompare( m_fileA, m_fileB, fileSizeA ) != 0 )
		{
			wxFileName fixer( file );
			ConvertPath( fixer, m_source, m_dest );
			wxString path = fixer.GetPath( wxPATH_GET_VOLUME );
			wxFileName::Mkdir( path, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL );

			return CSSExporter::OnFile( file );
		}
	}

	return wxDIR_CONTINUE;
}

bool CSSDiffExporter::ReadFile( const wchar_t* path, void** buffer, int& bufferSize, int& fileSize )
{
	void* targetBuffer = *buffer;
	int targetSize = bufferSize;

	if( !targetBuffer )
	{
		targetSize = 1024 * 128;
		targetBuffer = realloc( targetBuffer, targetSize );
	}

	FILE* file = nullptr;
	errno_t result = _wfopen_s( &file, path, L"r" );
	if( result == 0 )
	{
		int readSize = fread_s( targetBuffer, targetSize, 1, targetSize, file );

		while( feof( file ) == 0 )
		{
			// There's more of the file to be read (buffer must not be big enough)
			targetSize = targetSize * 2;
			targetBuffer = realloc( targetBuffer, targetSize );

			Red::System::Uint8* copyPoint = static_cast< Red::System::Uint8* >( targetBuffer ) + readSize;
			int sizeToRead = targetSize - readSize;
			readSize += fread_s( copyPoint, sizeToRead, 1, sizeToRead, file );
		}

		*buffer = targetBuffer;
		bufferSize = targetSize;
		fileSize = readSize;

		fclose( file );

		return true;
	}

	return false;
}
