
/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "fileDlg.h"

#include "../../common/redSystem/crt.h"
#include "../../common/core/depot.h"

// TODO: find a better way for this..
#define MAX_STATIC_STRING	2048
#define MAX_FILE_STRING		65536

CEdFileDialog::CEdFileDialog()
{
	// Setup dialog parameters
	Red::System::MemorySet( &m_settings, 0, sizeof(m_settings) );
	m_settings.lStructSize = sizeof(m_settings);
	m_settings.lpstrFile = new TCHAR [ MAX_FILE_STRING ];
	m_settings.nMaxFile = MAX_FILE_STRING;
	m_settings.lpstrInitialDir = new TCHAR [ MAX_STATIC_STRING ];
	m_settings.lpstrFilter = new TCHAR [ MAX_STATIC_STRING ];
	m_settings.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_ENABLESIZING | OFN_NONETWORKBUTTON | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	// Reset directory to the base data directory
	SetDirectory( GFileManager->GetDataDirectory() );

	// Reset format list
	ClearFormats(); 
}

CEdFileDialog::~CEdFileDialog()
{
	delete [] m_settings.lpstrFile;
	delete [] m_settings.lpstrInitialDir;
	delete [] m_settings.lpstrFilter; 
}

void CEdFileDialog::ClearFormats()
{
	m_formats.Clear();
}

void CEdFileDialog::AddFormat( const String &extension, const String &description )
{
	AddFormat( CFileFormat( extension, description ) );
}

void CEdFileDialog::AddFormat( const CFileFormat& format )
{
	m_formats.PushBack( format );
}

void CEdFileDialog::AddFormats( const TDynArray<CFileFormat>& formats )
{
	m_formats.PushBack( formats );
}

void CEdFileDialog::SetMultiselection( Bool multiselection )
{
	if ( multiselection )
	{
		m_settings.Flags |= OFN_ALLOWMULTISELECT;
	}
	else
	{
		m_settings.Flags &= ~OFN_ALLOWMULTISELECT;  
	}
}

void CEdFileDialog::SetDirectory( const String &directory )
{
	Red::System::StringCopy( (Char*)m_settings.lpstrInitialDir, directory.AsChar(), MAX_STATIC_STRING-1 );
}

void CEdFileDialog::SetIniTag(const String &tag)
{
	m_iniTag = tag;
}

const TDynArray< String > &CEdFileDialog::GetFiles() const
{
	return m_files;
}

const String& CEdFileDialog::GetFile() const
{
	return m_files.Size() ? m_files[0] : String::EMPTY;
}


const String& CEdFileDialog::GetFileDirectory() const
{
	return m_directory;
}

const TDynArray< String > &CEdFileDialog::GetFileNames() const
{
	return m_filenames;
}

const String& CEdFileDialog::GetFileName() const
{
	return m_filenames.Size() ? m_filenames[0] : String::EMPTY;
}


Bool CEdFileDialog::DoOpen(HWND owner, Bool getDirFromIni/* =false  */)
{
	// Load last directory path from ".ini" file
	if (getDirFromIni && !m_iniTag.Empty())
	{
		CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
		String identifier = TXT("/FileDialog/") + m_iniTag + TXT("/Open/");
		CConfigurationScopedPathSetter pathSetter( config, identifier );

		String depotPath;
		GDepot->GetAbsolutePath( depotPath );

		String dir = config.Read( TXT("Dir"), depotPath );
		dir = dir.ToLower();

		SetDirectory(dir);
	}

	// Remember current directory
	Char tempDir[ MAX_STATIC_STRING ];
	GetCurrentDirectory( MAX_STATIC_STRING, tempDir );

	// Update settings
	UpdateFilters( true );
	m_files.Clear();
	m_filenames.Clear();
	m_directory = String::EMPTY;
	Red::System::StringCopy( m_settings.lpstrFile, wxT(""), MAX_FILE_STRING );
	m_settings.hwndOwner = owner;
	m_settings.lpstrTitle = ( m_settings.Flags & OFN_ALLOWMULTISELECT ) ? TXT("Open files") : TXT("Open file");

	// Show dialog
	if ( !GetOpenFileName( &m_settings ))
	{
		// Canceled by user
		return false;
	}

	// Restore original directory
	GetCurrentDirectory( MAX_STATIC_STRING, (TCHAR *)m_settings.lpstrInitialDir );
	SetCurrentDirectory( tempDir );  

	// Parse file names
	if ( m_settings.Flags & OFN_ALLOWMULTISELECT )
	{
		Char* pos = m_settings.lpstrFile;
		String newFile;

		// Parse file names
		for (;;)
		{
			if  ( !*pos )
			{
				if ( newFile.GetLength() )
				{
					m_files.PushBack( newFile );
					newFile = String::EMPTY;
					pos++;      
				}
				else
				{
					break;
				}

				continue;
			}

			newFile += String::Chr( *pos );
			pos++;
		}

		// More than one file then we must deal with the path given in the first entry
		if ( m_files.Size() > 1 )
		{
			String mainPath = m_files[0];
			m_files.Erase( m_files.Begin() );

			for ( Uint32 i=0; i<m_files.Size(); i++ )
			{
				m_files[i] = mainPath + TXT("\\") + m_files[i];
			}
		}
	}
	else
	{
		// Single file only
		m_files.PushBack( m_settings.lpstrFile );
	}  

	ExtractDirectoryAndFilenames();

	// Get extension index
	Int32 index = m_settings.nFilterIndex - 1;
	if ( m_formats.Size() > 1 )
	{
		// Account for "all formats"
		index--;
	}

	// Update selected extension
	if ( index >= 0 && index < (Int32)m_formats.Size() )
	{
		m_fileFormat = m_formats[ index ];
	}
	else
	{
		m_fileFormat = CFileFormat( String::EMPTY, String::EMPTY );
	}

	if (getDirFromIni && !m_files.Empty() && !m_iniTag.Empty())
	{
		// Save selected directory

		CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
		String identifier = TXT("/FileDialog/") + m_iniTag + TXT("/Open/");
		CConfigurationScopedPathSetter pathSetter( config, identifier );

		String dir = m_files[0];
		dir = dir.StringBefore(TXT("\\"),true) + TXT("\\");

		config.Write( TXT("Dir"), dir );

		// Reset
		m_iniTag = String::EMPTY;
	}

	return true; 
}

Bool CEdFileDialog::DoSave(HWND owner, const Char* defaultFileName/* =NULL */, Bool getDirFromIni/* =false  */)
{
	// Load last directory path from ".ini" file
	if (getDirFromIni && !m_iniTag.Empty())
	{
		CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
		String identifier = TXT("/FileDialog/") + m_iniTag + TXT("/Save/");
		CConfigurationScopedPathSetter pathSetter( config, identifier );

		String depotPath;
		GDepot->GetAbsolutePath( depotPath );

		String dir = config.Read( TXT("Dir"), depotPath );
		dir = dir.ToLower();

		SetDirectory(dir);
	}

	// Remember current directory
	Char tempDir[ MAX_STATIC_STRING ];
	GetCurrentDirectory( MAX_STATIC_STRING, tempDir );

	// Update settings
	UpdateFilters( false );
	m_files.Clear();
	m_filenames.Clear();
	m_directory = String::EMPTY;
	m_settings.hwndOwner = owner;
	m_settings.lpstrTitle = TXT("Save file");   

	// Copy initial file name
	if ( defaultFileName )
	{
		// File name
		Red::System::StringCopy( m_settings.lpstrFile, defaultFileName, MAX_FILE_STRING );

		// Get default extension
		if ( m_fileFormat.GetExtension().Empty() )
		{
			m_fileFormat = m_formats[0];
		}

		// Append extension to file name
		if ( !m_fileFormat.GetExtension().Empty() )
		{
			Red::System::StringConcatenate( m_settings.lpstrFile, TXT("."), MAX_FILE_STRING );
			Red::System::StringConcatenate( m_settings.lpstrFile, m_fileFormat.GetExtension().AsChar(), MAX_FILE_STRING );
		}   
	}
	else
	{
		// No default file name given
		Red::System::StringCopy( m_settings.lpstrFile, wxT(""), MAX_FILE_STRING );  
	}  

	// Show dialog
	if ( !GetSaveFileName( &m_settings ))
	{
		// Canceled by user
		return false;
	}

	// Get extension
	Int32 index = m_settings.nFilterIndex - 1;

	// Update
	if ( index >= 0 && index < (Int32)m_formats.Size() )
	{
		m_fileFormat = m_formats[ index ];
	}
	else
	{
		// Custom extension given
		m_fileFormat = CFileFormat( String::EMPTY, String::EMPTY );
	}

	// Get file
	String file = m_settings.lpstrFile; 

	// Append extension if not given
	if ( !file.ContainsSubstring( TXT(".") ) )
	{
		file += TXT(".");
		file += m_fileFormat.GetExtension();
	}

	// Add file to list  
	m_files.PushBack( file );

	ExtractDirectoryAndFilenames();

	// Restore directory
	GetCurrentDirectory( MAX_STATIC_STRING, (Char*)m_settings.lpstrInitialDir );
	SetCurrentDirectory( tempDir );

	if (getDirFromIni && !m_files.Empty() && !m_iniTag.Empty())
	{
		// Save selected directory

		CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
		String identifier = TXT("/FileDialog/") + m_iniTag + TXT("/Save/");;
		CConfigurationScopedPathSetter pathSetter( config, identifier );

		String dir = m_files[0];
		dir = dir.StringBefore(TXT("\\"),true) + TXT("\\");

		config.Write( TXT("Dir"), dir );

		// Reset
		m_iniTag = String::EMPTY;
	}

	return true; 
}

void CEdFileDialog::UpdateFilters( Bool all )
{
	Bool addedMultiFilter = false;
	String filterString;

	if ( !m_formats.Size() )
	{
		// No filters
		filterString += TXT("All files [*.*]");
		filterString.PushBack( 0 );
		filterString += TXT("*.*");;
		filterString.PushBack( 0 );
		filterString.PushBack( 0 );
	}
	else if ( m_formats.Size() == 1 )
	{
		// One filter
		filterString += m_formats[0].GetDescription();
		filterString += TXT(" [*.");
		filterString += m_formats[0].GetExtension();
		filterString += TXT("]");
		filterString.PushBack( 0 );
		filterString += TXT("*.");
		filterString += m_formats[0].GetExtension();
		filterString.PushBack( 0 );
		filterString.PushBack( 0 );
	}
	else
	{
		// Many filters
		if ( all )
		{
			addedMultiFilter = true;
			filterString += TXT("All supported formats");
			filterString += TXT(" [");

			for ( Uint32 i=0; i<m_formats.Size(); i++ )
			{
				if ( i != 0 )
				{
					filterString += TXT(";");
				}
				filterString += TXT("*.");
				filterString += m_formats[i].GetExtension().AsChar();
			}

			filterString += TXT("]");
			filterString.PushBack( 0 );

			for ( Uint32 i=0; i<m_formats.Size(); i++ )
			{
				if ( i != 0 ) 
				{
					filterString += TXT(";");
				}

				filterString += TXT("*.");
				filterString += m_formats[i].GetExtension();
			}

			filterString.PushBack( 0 );
		}

		for ( Uint32 i=0; i<m_formats.Size(); i++ )
		{
			filterString += m_formats[i].GetDescription();
			filterString += TXT(" [");
			filterString += TXT("*.");
			filterString += m_formats[i].GetExtension();
			filterString += TXT("]");
			filterString.PushBack( 0 );
			filterString += TXT("*.");
			filterString += m_formats[i].GetExtension().AsChar();
			filterString.PushBack( 0 );
		}

		filterString.PushBack( 0 );   
	}

	// Update filter index
	Int32 find = -1;

	// Find filter
	for ( Uint32 i=0; i<m_formats.Size(); i++ )
	{
		const CFileFormat &format = m_formats[i];
		if ( format.GetDescription() == m_fileFormat.GetDescription() && format.GetExtension() == m_fileFormat.GetExtension() )
		{
			find = i;
			break;
		}
	}

	// Set filter index  
	if ( find == -1 )
	{
		m_settings.nFilterIndex = 1;
	}
	else
	{
		if ( addedMultiFilter )
		{
			m_settings.nFilterIndex = 2 + find;
		}
		else
		{
			m_settings.nFilterIndex = 1 + find;
		}
	}  

	// Copy buffer
	Red::System::MemoryCopy( (Char*)m_settings.lpstrFilter, filterString.AsChar(), filterString.Size() * sizeof(Char) ); 
}


void CEdFileDialog::ExtractDirectoryAndFilenames()
{
	m_filenames.Resize( m_files.Size() );
	m_directory = String::EMPTY;

	if ( m_files.Size() > 0 )
	{
		// We assume the directory is the same for everything. After all, how could it be otherwise?
		size_t lastSlashIndex = 0;
		m_files[0].FindCharacter( TXT('\\'), lastSlashIndex, true );
		m_directory = m_files[0].LeftString( lastSlashIndex );

		for ( Uint32 i = 0; i < m_files.Size(); ++i )
		{
			ASSERT( m_files[0].LeftString( lastSlashIndex ) == m_directory );
			m_filenames[i] = m_files[i].RightString( m_files[i].Size() - lastSlashIndex - 2 );
		}
	}
}
