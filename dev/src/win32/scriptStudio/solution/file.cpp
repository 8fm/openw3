/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "file.h"

#include "../fileStubs.h"
#include "../app.h"
#include "../documentView.h"


SolutionFile::SolutionFile( SolutionDir* dir, const wstring& solutionPath, const wstring& absolutePath, const wstring& name )
	: m_dir( dir )
	, m_name( name )
	, m_solutionPath( solutionPath )
	, m_absolutePath( absolutePath )
	, m_document( nullptr )
	, m_documentEx( nullptr )
	, m_stubs( nullptr )
	, m_isModified( false )
	, m_ignoreChanges( false )
{
	// Create stub manager for this file and request update
	m_stubs = new SSFileStub( solutionPath );
}

SolutionFile::~SolutionFile()
{
	if( m_stubs )
	{
		GStubSystem.Unregister( m_stubs );

		delete m_stubs;
		m_stubs = nullptr;
	}
}

bool SolutionFile::MarkModified()
{
	// Mark as modified
	m_isModified = true;
	return true;
}

bool SolutionFile::CancelModified()
{
	// Mark as not modified
	m_isModified = false;
	return true;
}

wxString SolutionFile::GetText()
{
	if ( m_document )
	{
		return m_document->GetText();
	}
	else
	{
		// Load file
		wxFile readFile( m_absolutePath.c_str() );
		if ( readFile.IsOpened() )
		{
			// Load string from file
			const wxFileOffset size = readFile.Length();
			char* rawText = (char*) RED_ALLOCA( size + 2 );
			readFile.Read( rawText, size );

			// Null terminate
			rawText[ size + 0 ] = 0;
			rawText[ size + 1 ] = 0;

			wxBOM bom = wxConvAuto::DetectBOM( rawText, size );

			wxString text;

			switch( bom )
			{
			case wxBOM_None:
			case wxBOM_Unknown:
				text = wxString( rawText, wxConvLocal );
				break;

			default:
				text = wxString( rawText, wxConvAuto() );
			}

			return text;
		}
	}

	// No text in file
	return wxEmptyString;
}

bool SolutionFile::IsModified() const
{
	return m_isModified;
}

bool SolutionFile::Add()
{
	Red::System::Uint32 size = m_absolutePath.length() + 1;
	Red::System::AnsiChar* buffer = static_cast< Red::System::AnsiChar* >( alloca( size * sizeof( Red::System::AnsiChar ) ) );

	Red::System::StringConvert( buffer, m_absolutePath.c_str(), size );

	return wxTheSSApp->GetVersionControl()->Add( buffer );
}

bool SolutionFile::CheckOut()
{
	Red::System::Uint32 size = m_absolutePath.length() + 1;
	Red::System::AnsiChar* buffer = static_cast< Red::System::AnsiChar* >( alloca( size * sizeof( Red::System::AnsiChar ) ) );

	Red::System::StringConvert( buffer, m_absolutePath.c_str(), size );

	return wxTheSSApp->GetVersionControl()->Checkout( buffer );
}

bool SolutionFile::CheckIn()
{
	Red::System::Uint32 size = m_absolutePath.length() + 1;
	Red::System::AnsiChar* buffer = static_cast< Red::System::AnsiChar* >( alloca( size * sizeof( Red::System::AnsiChar ) ) );

	Red::System::StringConvert( buffer, m_absolutePath.c_str(), size );

	return wxTheSSApp->GetVersionControl()->Submit( buffer, "test" );
}

bool SolutionFile::Revert()
{
	Red::System::Uint32 size = m_absolutePath.length() + 1;
	Red::System::AnsiChar* buffer = static_cast< Red::System::AnsiChar* >( alloca( size * sizeof( Red::System::AnsiChar ) ) );

	Red::System::StringConvert( buffer, m_absolutePath.c_str(), size );

	return wxTheSSApp->GetVersionControl()->Revert( buffer );
}

void SolutionFile::RefreshStatus()
{
	Red::System::Uint32 size = m_absolutePath.length() + 1;
	Red::System::AnsiChar* buffer = static_cast< Red::System::AnsiChar* >( alloca( size * sizeof( Red::System::AnsiChar ) ) );

	Red::System::StringConvert( buffer, m_absolutePath.c_str(), size );

	m_status = wxTheSSApp->GetVersionControl()->GetStatus( buffer );

	// Update editor
	if ( m_document )
	{
		const bool isReadOnly = !CanModify();
		m_document->SetReadOnly( isReadOnly );
	}
}

bool SolutionFile::IsReadOnlyFlagSet() const
{
	DWORD attributes = GetFileAttributes( m_absolutePath.c_str() );

	if( attributes != INVALID_FILE_ATTRIBUTES )
	{
		return ( attributes & FILE_ATTRIBUTE_READONLY ) != 0;
	}

	return true;
}

bool SolutionFile::ClearReadOnlyFlag() const
{
	DWORD attributes = GetFileAttributes( m_absolutePath.c_str() );

	if( attributes != INVALID_FILE_ATTRIBUTES )
	{
		attributes &= ~FILE_ATTRIBUTE_READONLY;
		return SetFileAttributes( m_absolutePath.c_str(), attributes ) != 0;
	}

	return false;
}

bool SolutionFile::Save()
{
	if ( m_document && m_document->Save() )
	{
		m_isModified = false;
		return true;
	}

	return false;
}

void SolutionFile::UpdateModificationTime()
{
	wxFileName fn( m_absolutePath.c_str() );
	m_modificationTime = fn.GetModificationTime();
}

bool SolutionFile::CheckModificationTime()
{
	if ( !m_modificationTime.IsValid() )
	{
		return true;
	}
	wxFileName fn( m_absolutePath.c_str() );
	wxDateTime dt = fn.GetModificationTime();
	return ( dt == m_modificationTime );
}

bool SolutionFile::ExistsOnDisk() const
{
	return wxFileExists( m_absolutePath.c_str() );
}