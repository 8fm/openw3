/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "filePath.h"
#include "configVar.h"
#include "configVarLegacyWrapper.h"

namespace Config
{
	namespace Legacy
	{

		//-------

		CConfigLegacySection::CConfigLegacySection( CConfigLegacyFile* file )
			: m_file( file )
		{
		}

		CConfigLegacySection::~CConfigLegacySection()
		{
		}

		void CConfigLegacySection::Clear()
		{
			// Remove all data
			m_items.Clear();

			// Mark file as modified
			m_file->MarkModified();
		}

		Bool CConfigLegacySection::ReadValue( const String& key, String& value ) const
		{
			// Find value
			const TDynArray< String >* foundValues = m_items.FindPtr( key );
			if ( foundValues && foundValues->Size() )
			{
				value = (*foundValues)[0];
				return true;
			}

			// Not found
			return false;
		}

		Bool CConfigLegacySection::ReadValues( const String& key, TDynArray< String > & values ) const
		{
			// Find value
			const TDynArray< String >* foundValues = m_items.FindPtr( key );
			if ( foundValues && foundValues->Size() )
			{
				values = *foundValues;
				return true;
			}

			// Not found
			return false;
		}

		Bool CConfigLegacySection::WriteValue( const String& key, const String& value, Bool append )
		{
			// Empty key
			if ( key.Empty() )
			{
				return false;
			}

			// Do not append empty values
			if ( append && value.Empty() )
			{
				return false;
			}

			// Get the values
			TDynArray< String >* values = m_items.FindPtr( key );
			if ( values )
			{
				// Append only non empty values
				if ( append )
				{
					// Append new value
					values->PushBack( value );
				}
				else
				{
					// Emit value
					values->Resize( 1 );
					(*values)[0] = value;
				}
			}
			else
			{
				// Add new value in section
				TDynArray< String > values( 1 );
				values[0] = value;
				m_items.Insert( key, values );
			}

			// Mark file as modified
			m_file->MarkModified();
			return true;
		}

		Bool CConfigLegacySection::RemoveValues( const String& key )
		{
			// Remove whole key from config
			if ( !m_items.Erase( key ) )
			{
				return false;
			}

			// Mark file as modified
			m_file->MarkModified();
			return true;
		}

		Bool CConfigLegacySection::RemoveValue( const String& key, String& value )
		{
			// Find key array
			TDynArray< String >* values = m_items.FindPtr( key );
			if ( !values )
			{
				return false;
			}

			// Remove value
			if ( !values->Remove( value ) )
			{
				return false;
			}

			// Last value remove, remove the whole definition
			if ( !values->Size() )
			{
				m_items.Erase( key );
			}

			// Mark file as modified
			m_file->MarkModified();
			return true;
		}

		//-------

		CConfigLegacyFile::CConfigLegacyFile( const String& shortName, const String& filePath )
			: m_filePath( filePath )
			, m_shortName( shortName )
			, m_isModified( false )
		{
		}

		CConfigLegacyFile::~CConfigLegacyFile()
		{
			m_sections.ClearPtr();
		}

		void CConfigLegacyFile::MarkModified()
		{
			m_isModified = true;
		}

		Bool CConfigLegacyFile::Read( const String& filePath )
		{
			// Load to string
			String configString;
			if ( !GFileManager->LoadFileToString( filePath, configString, true ) )
			{
				return false;
			}

			// Empty config file
			if ( configString.Empty() )
			{
				return false;
			}

			// Remove existing crap
			m_sections.ClearPtr();

			// Parse lines
			CConfigLegacySection* section = NULL;
			const Char* cur = configString.AsChar();
			while ( *cur )
			{
				Char ch = *cur;

				// Parse white space && non-ansi chars
				if ( ch <= 32 || ch > 127 )
				{
					cur++;
					continue;
				}

				// Section name
				if ( ch == '[' )
				{
					cur++;

					String sectionName;
					while ( *cur )
					{
						if ( *cur <= 32 )
						{
							WARN_CORE( TXT("Error parsing config file '%ls'"), filePath.AsChar() );
							return false;
						}

						if ( *cur == ']' )
						{
							cur++;
							break;
						}

						// Config name
						sectionName += String::Chr( *cur++ );
					}

					// Create section
					section = GetSection( sectionName, true );
					if ( !section )
					{
						WARN_CORE( TXT("Error parsing config file '%ls': no section '%ls'"), filePath.AsChar(), sectionName.AsChar() );
						return false;
					}

					continue;
				}

				// Key=Value pair, parse till the end of line
				Bool isKey = true;
				String key, value;
				while ( *cur )
				{
					if ( *cur < 32 )
					{
						break;
					}

					if ( *cur == '=' && isKey )
					{
						isKey = false;
						cur++;
						continue;
					}

					// Parse key or value name
					if ( isKey )
					{
						key += String::Chr( *cur );
					}
					else
					{
						value += String::Chr( *cur );
					}

					cur++;
				}

				// Empty key
				if ( key.Empty() )
				{
					WARN_CORE( TXT("Error parsing config file '%ls': empty key"), filePath.AsChar() );
					return false;
				}

				// Add to section
				if ( !section )
				{
					WARN_CORE( TXT("Error parsing config file '%ls': key '%ls' without section"), filePath.AsChar(), key.AsChar() );
					return false;
				}

				// Add to values
				section->WriteValue( key, value, true );

			}

			// Parsed
			m_isModified = false;
			return true;
		}

		Bool CConfigLegacyFile::Write()
		{
			const Uint32 fileFlags = FOF_Buffered|FOF_AbsolutePath;
			IFile* saver = GFileManager->CreateFileWriter( m_filePath, fileFlags );
			if ( !saver )
			{
				LOG_CORE( TXT( "Failed to open config file '%ls' for writing" ), m_filePath.AsChar() );
				return false;
			}

			AnsiChar newLine[] = "\r\n";
			const MemSize newLineLen = Red::System::StringLength( newLine );
			AnsiChar formatString[ 512 ] = {'\0'};	// avoid allocating thousands of times

			for ( auto i=m_sections.Begin(); i!=m_sections.End(); ++i )
			{
				// Section name
				size_t charsWritten = Red::System::SNPrintF( formatString, ARRAY_COUNT( formatString ), "[%ls]\r\n", i->m_first.AsChar() );
				saver->Serialize( formatString, charsWritten );

				// Section values
				const auto& values = i->m_second->GetItems();
				for ( auto j=values.Begin(); j!=values.End(); ++j )
				{
					// Values
					for ( Uint32 k=0; k<j->m_second.Size(); k++ )
					{
						charsWritten = Red::System::SNPrintF( formatString, ARRAY_COUNT( formatString ), "%ls=%ls\r\n", j->m_first.AsChar(), j->m_second[k].AsChar() );
						saver->Serialize( formatString, charsWritten );
					}

				}

				// Separation
				saver->Serialize( newLine, newLineLen );
			}

			delete saver;
			m_isModified = false;

			return true;
		}

		CConfigLegacySection* CConfigLegacyFile::GetSection( const String& name, Bool createIfNotFound/*=false*/ )
		{
			// Empty section names are not supported
			if ( name.Empty() )
			{
				return NULL;
			}

			// Find existing section
			CConfigLegacySection* section = NULL;
			if ( !m_sections.Find( name, section ) )
			{
				// Create new section
				if ( createIfNotFound )
				{
					section = new CConfigLegacySection( this );
					m_sections.Insert( name, section );
					m_isModified = true;
				}
			}

			// Return created or found section
			return section;
		}

		//---------

		CConfigLegacyManager::CConfigLegacyManager()
		{
		}

		CConfigLegacyManager::~CConfigLegacyManager()
		{
			m_files.ClearPtr();
		}

		void CConfigLegacyManager::Initialize( const String& configDir )
		{
			if ( m_configDir != configDir )
			{
				m_files.ClearPtr();
				m_configDir = configDir;
			}
		}

		void CConfigLegacyManager::Shutdown()
		{
			// Flush modified only
			Save( false );
		}

		void CConfigLegacyManager::SetAlias( const Char* aliasName, const Char* trueName )
		{
			// Flush files
			Save( false );

			// Cleanup current files
			m_files.ClearPtr();

			// Set alias
			for ( auto alias : m_aliases )
			{
				if ( alias.m_first == aliasName )
				{
					alias.m_second = trueName;
					return;
				}
			}

			// Add new alias
			m_aliases.PushBack( TPair<String,String>( aliasName, trueName ) );
		}

		void CConfigLegacyManager::Save( Bool flushAll /*=false*/ )
		{
			// Not initialized
			if ( m_configDir.Empty() )
			{
				ERR_CORE( TXT("Legacy config was not yet initialized") );
				return;
			}

#if defined( RED_PLATFORM_WINPC )
			// Save all configuration files
			for ( TDynArray< CConfigLegacyFile* >::iterator i=m_files.Begin(); i!=m_files.End(); ++i )
			{
				CConfigLegacyFile* file = *i;
				if ( file->IsModified() || flushAll )
				{
					file->Write();
				}
			}
#else
			RED_UNUSED( flushAll );			
#endif
		}

		CConfigLegacyFile* CConfigLegacyManager::GetFile( const Char* fileName )
		{
			// Not initialized
			if ( m_configDir.Empty() )
			{
				ERR_CORE( TXT("Legacy config was not yet initialized") );
				return NULL;
			}

			// No category given
			if ( !fileName || !fileName[0] )
			{
				return NULL;
			}

			// Search for existing file
			for ( Uint32 i=0; i<m_files.Size(); i++ )
			{
				CConfigLegacyFile* file = m_files[i];
				if ( file->GetShortName().EqualsNC( fileName ) )
				{
					return file;
				}
			}

			// Translate file name using alias table
			const Char* finalFileName = fileName;
			for ( auto iter = m_aliases.Begin(), end = m_aliases.End(); iter != end; ++iter)
			{
				if ( iter->m_first == fileName )
				{
					finalFileName = iter->m_first.AsChar();
					break;
				}
			}

			// Invalid category name :)
			Char fullConfigFilePath[ 256 ];
			Red::System::SNPrintF( fullConfigFilePath, 256, TXT("%ls%ls.ini"), m_configDir.AsChar(), finalFileName );

			// Add to list
			CConfigLegacyFile* file = new CConfigLegacyFile( finalFileName, fullConfigFilePath );
			m_files.PushBack( file );

			// Load base content
			CFilePath baseFilePath( fullConfigFilePath );
			baseFilePath.PushDirectory( TXT("base") );
			Bool contentLoaded = file->Read( baseFilePath.ToString() );

			// Load normal content
			contentLoaded |= file->Read( fullConfigFilePath );

			// stats
			if ( contentLoaded )
			{
				if ( finalFileName != fileName )
				{
					LOG_CORE( TXT("Loaded legacy config from '%ls' (actually %ls)"), fileName, finalFileName );
				}
				else
				{
					LOG_CORE( TXT("Loaded legacy config from '%ls'"), fileName );
				}
			}

			// return loaded/created file
			return file;
		}

		CConfigLegacyFile* CConfigLegacyManager::GetFileWithAbsolutePathAndExtension( const Char* absolutePath, const Char* fileName, const Char* extension )
		{
			// No category given
			if ( !fileName || !fileName[0] )
			{
				return NULL;
			}

			// Search for existing file
			for ( Uint32 i=0; i<m_files.Size(); i++ )
			{
				CConfigLegacyFile* file = m_files[i];
				if ( file->GetShortName().EqualsNC( fileName ) )
				{
					return file;
				}
			}

			// Translate file name using alias table
			const Char* finalFileName = fileName;
			for ( auto iter = m_aliases.Begin(), end = m_aliases.End(); iter != end; ++iter)
			{
				if ( iter->m_first == fileName )
				{
					finalFileName = iter->m_first.AsChar();
					break;
				}
			}

			// Invalid category name :)
			Char fullConfigFilePath[ 256 ];
			Red::System::SNPrintF( fullConfigFilePath, 256, TXT("%ls%ls.%ls"), absolutePath, finalFileName, extension );

			// Add to list
			CConfigLegacyFile* file = new CConfigLegacyFile( finalFileName, fullConfigFilePath );
			m_files.PushBack( file );

			// Load base content
			CFilePath baseFilePath( fullConfigFilePath );
			baseFilePath.PushDirectory( TXT("base") );
			Bool contentLoaded = file->Read( baseFilePath.ToString() );

			// Load normal content
			contentLoaded |= file->Read( fullConfigFilePath );

			// stats
			if ( contentLoaded )
			{
				if ( finalFileName != fileName )
				{
					LOG_CORE( TXT("Loaded legacy config from '%ls' (actually %ls)"), fileName, finalFileName );
				}
				else
				{
					LOG_CORE( TXT("Loaded legacy config from '%ls'"), fileName );
				}
			}

			// return loaded/created file
			return file;
		}

	} // Legacy

} // Config
