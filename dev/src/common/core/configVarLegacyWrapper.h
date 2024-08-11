/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "sortedmap.h"
#include "hashmap.h"

/// LEGACY CONFIG IS VERY HEAVY AND PAIN IN THE ASS TO USE
/// PLEASE CONSIDER USING TConfigVar directly
/// Those classes are here because we have a game to ship and not everything can be moved to config vars ATM

namespace Config
{
	namespace Legacy
	{
		class CConfigLegacyFile;
		class CConfigLegacySection;
		class CConfigLegacyManager;

		/// Legacy config keys
		/// NOTE: this matches previous interface
		typedef THashMap< String, TDynArray< String > >	TConfigLegacySectionKeys;
		typedef TSortedMap< String, CConfigLegacySection* > TConfigLegacySections;
		typedef TDynArray< CConfigLegacyFile* > TConfigLegacyFiles;
		typedef TDynArray< TPair< String, String > > TConfigLegacyAliases;

		/// Legacy config section
		/// NOTE: this matches previous interface
		class CConfigLegacySection
		{
			DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

		public:
			//! Get the configuration items
			RED_INLINE const TConfigLegacySectionKeys& GetItems() const { return m_items; }

		public:
			CConfigLegacySection( CConfigLegacyFile* file );
			~CConfigLegacySection();

			//! Clear section
			void Clear();

			//! Read value ( the first one if multiple are defined )
			Bool ReadValue( const String& key, String& value ) const;

			//! Read all values for given key
			Bool ReadValues( const String& key, TDynArray< String > & values ) const;

			//! Write value
			Bool WriteValue( const String& key, const String& value, Bool append );

			//! Remove all values for entry
			Bool RemoveValues( const String& key );

			//! Remove entry form config
			Bool RemoveValue( const String& key, String& value );

		protected:
			TConfigLegacySectionKeys		m_items;		//!< Items in this section
			CConfigLegacyFile*				m_file;			//!< File this section is in
		};

		/// Legacy config file
		/// NOTE: this matches previous interface
		class CConfigLegacyFile
		{
			DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

		public:
			//! Get config file path to the ini file
			RED_INLINE const String& GetFilePath() const { return m_filePath; }

			//! Get short name
			RED_INLINE const String& GetShortName() const { return m_shortName; }

			//! Is this file modified
			RED_INLINE Bool IsModified() const { return m_isModified; }

		public:
			CConfigLegacyFile( const String& shortName, const String& filePath );
			~CConfigLegacyFile();

			//! Mark configuration file as modified
			void MarkModified();

			//! Load configuration, will append keys
			Bool Read( const String& filePath );

			//! Write configuration file
			Bool Write();

		public:
			//! Get section
			CConfigLegacySection* GetSection( const String& name, Bool createIfNotFound=false );

			//! Get sections
			RED_INLINE const TConfigLegacySections& GetSections() const { return m_sections; }

		private:
			//! Load configuration
			Bool Parse( const String& filePath );

			String						m_shortName;		//!< Short config file name
			String						m_filePath;			//!< Path to configuration file
			TConfigLegacySections		m_sections;			//!< Sections in the config file
			Bool						m_isModified;		//!< File is modified ( needs to be saved )
		};

		/// Configuration files
		class CConfigLegacyManager
		{
			DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

		public:
			CConfigLegacyManager();
			~CConfigLegacyManager();

			//! Initialize
			void Initialize( const String& configDir );

			//! Shutdown
			void Shutdown();

			//! Set config alias
			void SetAlias( const Char* alias, const Char* file );

			//! Save configuration files
			void Save( Bool flushAll = false );

			//! Get configuration file for given category
			CConfigLegacyFile* GetFile( const Char* category );

			//! Get configuration file for given category
			const CConfigLegacyFile* GetFile( const Char* category ) const;

			//! Get configuration file for given category in given absolute path and with given file extension
			CConfigLegacyFile* GetFileWithAbsolutePathAndExtension( const Char* absolutePath, const Char* fileName, const Char* extension );

		public:
			//! Load parameter
			template< class T >
			Bool ReadParam( const Char* category, const String& section, const String& key, T& param )
			{
				CConfigLegacyFile* configFile = GetFile( category );
				if ( configFile )
				{
					CConfigLegacySection* configSection = configFile->GetSection( section );
					if ( configSection )
					{
						String value;
						if ( configSection->ReadValue( key, value ) )
						{
							return ParseValue( value, param );
						}
					}
				}

				// Not found
				return false;
			}

			//! Save parameter
			template< class T >
			Bool WriteParam( const Char* category, const Char*section, const Char* key, const T& param )
			{
				CConfigLegacyFile* configFile = GetFile( category );
				if ( configFile )
				{
					CConfigLegacySection* configSection = configFile->GetSection( section, true );
					if ( configSection )
					{
						String value = ToString<T>( param );
						return configSection->WriteValue( key, value, false );
					}
				}

				// Not found
				return false;
			}

		protected:
			String					m_configDir;		//!< Directory with main configuration files
			TConfigLegacyFiles		m_files;			//!< Configuration files
			TConfigLegacyAliases	m_aliases;			//!< Config aliases

			RED_INLINE Bool ParseValue( const String& in, String& out ) const
			{
				const Char* stream = in.AsChar();
				return GParseString( stream, out );
			}

			template< typename T >
			RED_INLINE Bool ParseValue( const String& in, T& out ) const
			{
				return FromString< T >( in, out );
			}
		};

		template<>
		RED_INLINE Bool CConfigLegacyManager::ParseValue( const String& in, Bool& out ) const
		{
			if ( in.EqualsNC( TXT("true") ) )
			{
				out = true;
				return true;
			}
			else if ( in.EqualsNC( TXT("false") ) )
			{
				out = false;
				return true;
			}

			Int32 number = 0;
			if ( !ParseValue<Int32>( in, number ) )
				return false;

			out = (number != 0);
			return true;
		}


	} // Legacy

} // Config
