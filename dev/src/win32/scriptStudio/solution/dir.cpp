/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "dir.h"
#include "../fileStubs.h"
#include "../app.h"
#include "file.h"

SolutionDir::SolutionDir( SolutionDir* parent, const wstring& solutionPath, const wstring& absolutePath, const wstring& name )
	: m_parent( parent )
	, m_name( name )
	, m_solutionPath( solutionPath )
	, m_absolutePath( absolutePath )
{
	ScanInternal();
	ScanVersionControl();
}

SolutionDir::~SolutionDir()
{
	FreeDirectories( m_directories );
	FreeFiles();
}

void SolutionDir::FreeDirectories( vector<SolutionDir*> & directories )
{
	// Delete directories
	for ( vector< SolutionDir* >::iterator i = directories.begin(); i != directories.end(); ++i )
	{
		delete *i;
	}

	directories.clear();
}

void SolutionDir::FreeFiles()
{
	m_files.clear();
}

void SolutionDir::Scan()
{
	ScanInternal();
	ScanVersionControl();
}

void SolutionDir::ScanInternal()
{
	FreeDirectories( m_directories );
	FreeFiles();

	WIN32_FIND_DATA	findData;

	// Find directories
	wstring dirPattern = m_absolutePath + wxT("*.");
	HANDLE handle = FindFirstFile( dirPattern.c_str(), &findData );
	if ( handle != INVALID_HANDLE_VALUE )
	{
		do 
		{
			// Go to subdirectory
			wstring fileName = findData.cFileName;
			if ( fileName != wxT(".") && fileName != wxT("..") )
			{
				wstring newAbsolutePath = m_absolutePath + fileName + wxT("\\");
				wstring newSolutionPath = m_solutionPath + fileName + wxT("\\");
				SolutionDir* newDir = new SolutionDir( this, newSolutionPath, newAbsolutePath, fileName );
				m_directories.push_back( newDir );
			}
		}
		while ( FindNextFile( handle, &findData ) );
	}

	// Find files
	wstring filePattern = m_absolutePath + wxT("*.ws");
	handle = FindFirstFile( filePattern.c_str(), &findData );
	if ( handle != INVALID_HANDLE_VALUE )
	{
		do 
		{
			// Create file wrapper
			wstring fileName = findData.cFileName;
			wstring newAbsolutePath = m_absolutePath + fileName;
			wstring newSolutionPath = m_solutionPath + fileName;

			SolutionFilePtr newFile = make_shared< SolutionFile >( this, newSolutionPath, newAbsolutePath, fileName );
			GStubSystem.Schedule( newFile );
			m_files.push_back( newFile );
		}
		while ( FindNextFile( handle, &findData ) );
	}
}

void SolutionDir::ScanVersionControl()
{
	// Retrieve list of files that are marked for deletion (and hence not available locally)
	VersionControl::Filelist* deletedFilesList = wxTheSSApp->GetVersionControl()->CreateFileList();

	if( deletedFilesList )
	{
		wxString absolutePath = m_absolutePath.c_str();

		wxTheSSApp->GetVersionControl()->GetDeletedFiles( deletedFilesList, absolutePath.mb_str() );

		const char** deletedFiles = deletedFilesList->Get();

		for( size_t i = 0; i < deletedFilesList->Size(); ++i )
		{
			wxString fileName = Red::System::StringSearchLast( deletedFiles[ i ], '\\' ) + 1;
			wstring newAbsolutePath = m_absolutePath + fileName.wc_str();
			wstring newSolutionPath = m_solutionPath + fileName.wc_str();

			SolutionFilePtr newFile = make_shared< SolutionFile >( this, newSolutionPath, newAbsolutePath, fileName.wc_str() );
			m_files.push_back( newFile );
		}

		wxTheSSApp->GetVersionControl()->DestroyFileList( deletedFilesList );
	}
}

bool SolutionDir::Empty() const
{
	return m_files.empty() && m_directories.empty();
}

SolutionDir* SolutionDir::CreateDir( const wstring& name )
{
	SolutionDir* dir = FindDir( name );
	if ( !dir )
	{
		wstring newAbsolutePath = m_absolutePath + name + wxT("\\");
		wstring newSolutionPath = m_solutionPath + name + wxT("\\");
		RED_VERIFY( wxMkdir( newAbsolutePath.c_str() ), TXT( "Couldn't create new directory %ls" ), newAbsolutePath.c_str() );
		dir = new SolutionDir( this, newSolutionPath, newAbsolutePath, name );
		m_directories.push_back( dir );
	}

	return dir;
}

SolutionFilePtr SolutionDir::CreateFile( const wstring& name )
{
	SolutionFilePtr file = FindFile( name );
	if ( !file )
	{
		wstring newAbsolutePath = m_absolutePath + name;
		wstring newSolutionPath = m_solutionPath + name;
		file = make_shared< SolutionFile >( this, newSolutionPath, newAbsolutePath, name );
		m_files.push_back( file );
	}

	return file;
}

SolutionDir* SolutionDir::FindDir( const wstring& name )
{
	const size_t numDirectories = m_directories.size();
	for ( size_t i = 0; i < numDirectories; ++i )
	{
		SolutionDir* subDir = m_directories[ i ];
		if( Red::System::StringCompareNoCase( subDir->GetName().c_str(), name.c_str() ) == 0 )
		{
			return subDir;
		}
	}

	return nullptr;
}

SolutionFilePtr SolutionDir::FindFile( const wstring& name )
{
	const size_t size = m_files.size();
	for ( size_t i = 0; i < size; ++i )
	{
		SolutionFilePtr file = m_files[ i ];

		if( Red::System::StringCompareNoCase( file->m_name.c_str(), name.c_str() ) == 0 )
		{
			return file;
		}
	}

	return nullptr;
}

SolutionFilePtr SolutionDir::FindFileRecurse( const wstring& path )
{
	size_t position = path.find( L'\\' );

	if( position != wstring::npos )
	{
		wstring directory = path.substr( 0, position );
		wstring remainingPath = path.substr( position + 1 );

		SolutionDir* dir = FindDir( directory );

		if( dir )
		{
			return dir->FindFileRecurse( remainingPath );
		}
		else
		{
			return nullptr;
		}
	}
	else
	{
		return FindFile( path );
	}
}

void SolutionDir::RefreshFilesStatus()
{
	for( SolutionDir* child : m_directories )
	{
		child->RefreshFilesStatus();
	}

	for( SolutionFilePtr file : m_files )
	{
		file->RefreshStatus();
	}
}

void SolutionDir::GetFiles( vector< SolutionFilePtr>& files )
{
	// Get local files
	const size_t numFiles = m_files.size();
	for ( size_t i = 0; i < numFiles; ++i )
	{
		SolutionFilePtr file = m_files[ i ];
		files.push_back( file );
	}

	// Recurse
	const size_t numDirectories = m_directories.size();
	for ( size_t i = 0; i < numDirectories; ++i )
	{
		SolutionDir* subDir = m_directories[ i ];
		subDir->GetFiles( files );
	}
}
