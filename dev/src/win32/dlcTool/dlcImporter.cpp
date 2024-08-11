#include "build.h"
#include "resource.h"
#include <memory>
#include <sstream>
#include <vector>
#include <list>

#define HKCU_MAINREGISTERKEY TEXT("Software\\CD Projekt RED\\The Witcher 3")
#define HKLM_MAINREGISTERKEY TEXT("Software\\CD Projekt RED\\The Witcher 3")


namespace DLCTool
{
	///////////////////////////////////////////////////////////////////////////////

	const char* ids[] = {
		"id",
		"name",
		"install",
		"fileSize",
		"delete"
	};
	enum EParams
	{		
		EP_ID = 0,
		EP_Name,
		EP_InstallFile,
		EP_FileSize,
		EP_DeleteFile,
	};

	class ParseEntry
	{
	public:
		virtual ~ParseEntry() {}

		virtual bool parse( const std::string& line ) = 0;
	};

	template< typename T >
	class TParseEntry : public ParseEntry
	{
	public:
		virtual ~TParseEntry() {}
		virtual bool parse( const std::string& line ) { return false; }
	};

	class TParseIntEntry : public TParseEntry< UINT >
	{
	private:
		EParams		m_param;
		UINT&		m_outVal;

	public:
		TParseIntEntry( EParams param, UINT& outVal ) : m_param( param ), m_outVal( outVal ) {}

		bool parse( const std::string& line )
		{
			if ( line.substr( 0, strlen( ids[ m_param ] ) ) == ids[ m_param ] )
			{
				sscanf_s( line.substr( strlen( ids[ m_param ] ) ).c_str(), "=%d", &m_outVal );
				return true;
			}
			else
			{
				return false;
			}
		}
	};

	class TParseDWORDEntry : public TParseEntry< DWORD >
	{
	private:
		EParams		m_param;
		DWORD&		m_outVal;

	public:
		TParseDWORDEntry( EParams param, DWORD& outVal ) : m_param( param ), m_outVal( outVal ) {}

		bool parse( const std::string& line )
		{
			if ( line.substr( 0, strlen( ids[ m_param ] ) ) == ids[ m_param ] )
			{
				sscanf_s( line.substr( strlen( ids[ m_param ] ) ).c_str(), "=%ld", &m_outVal );
				return true;
			}
			else
			{
				return false;
			}
		}
	};

	class TParseArrDWORDEntry : public TParseEntry< std::vector< DWORD > >
	{
	private:
		EParams						m_param;
		std::vector< DWORD >&		m_outVal;

	public:
		TParseArrDWORDEntry( EParams param, std::vector< DWORD >& outVal ) : m_param( param ), m_outVal( outVal ) {}

		bool parse( const std::string& line )
		{
			if ( line.substr( 0, strlen( ids[ m_param ] ) ) == ids[ m_param ] )
			{
				DWORD val;
				sscanf_s( line.substr( strlen( ids[ m_param ] ) ).c_str(), "=%ld", &val );
				m_outVal.push_back( val );
				return true;
			}
			else
			{
				return false;
			}
		}
	};

	class TParseFloatEntry : public TParseEntry< float >
	{
	private:
		EParams		m_param;
		float&		m_outVal;

	public:
		TParseFloatEntry( EParams param, float& outVal ) : m_param( param ), m_outVal( outVal ) {}

		bool parse( const std::string& line )
		{
			if ( line.substr( 0, strlen( ids[ m_param ] ) ) == ids[ m_param ] )
			{
				sscanf_s( line.substr( strlen( ids[ m_param ] ) ).c_str(), "=%f", &m_outVal );
				return true;
			}
			else
			{
				return false;
			}
		}
	};

	class TParseStringEntry : public TParseEntry< std::wstring >
	{
	private:
		EParams				m_param;
		std::wstring&		m_outVal;

	public:
		TParseStringEntry( EParams param, std::wstring& outVal ) : m_param( param ), m_outVal( outVal ) {}

		bool parse( const std::string& line )
		{
			if ( line.substr( 0, strlen( ids[ m_param ] ) ) == ids[ m_param ] )
			{
				char tmpStr[255];
				sscanf_s( line.substr( strlen( ids[ m_param ] ) ).c_str(), "=%s", &tmpStr );

				std::string str( tmpStr );
				m_outVal = std::wstring( str.begin(), str.end() );
				return true;
			}
			else
			{
				return false;
			}
		}
	};

	class TParseArrStringEntry : public TParseEntry< std::vector< std::wstring > >
	{
	private:
		EParams								m_param;
		std::vector< std::wstring >&		m_outVal;

	public:
		TParseArrStringEntry( EParams param, std::vector< std::wstring >& outVal ) : m_param( param ), m_outVal( outVal ) {}

		bool parse( const std::string& line )
		{
			if ( line.substr( 0, strlen( ids[ m_param ] ) ) == ids[ m_param ] )
			{
				char tmpStr[255];
				sscanf_s( line.substr( strlen( ids[ m_param ] ) ).c_str(), "=%s", &tmpStr );

				std::string str( tmpStr );
				m_outVal.push_back( std::wstring( str.begin(), str.end() ) );
				return true;
			}
			else
			{
				return false;
			}
		}
	};

	class Parser
	{
	private:
		std::vector< ParseEntry* >		m_entries;

	public:
		~Parser()
		{
			for ( std::vector< ParseEntry* >::iterator it = m_entries.begin(); it != m_entries.end(); ++it )
			{
				delete *it;
			}
			m_entries.clear();
		}

		void add( ParseEntry* entry )
		{
			if ( entry != NULL )
			{
				m_entries.push_back( entry );
			}
		}

		void parse( std::string data )
		{
			UINT parsersCount = static_cast< UINT >( m_entries.size() );
			std::string line;
			while( data.empty() == false )
			{
				std::size_t pos = data.find_first_of( "\n" );
				line = data.substr( 0, pos );
				if ( line.length() < data.length() )
				{
					data = data.substr( line.length() + 1 );
				}
				else
				{
					data.clear();
				}

				for ( UINT i = 0; i < parsersCount; ++i )
				{
					if ( m_entries[i]->parse( line ) )
					{
						break;
					}
				}
			}
		}
	};

	///////////////////////////////////////////////////////////////////////////////

	void CreateArchive( const char* iniFile, const char* outputFile )
	{
		std::wstring wstrIniFileName( iniFile, iniFile + strlen( iniFile ) );
		std::wstring wstrOutputFileName( outputFile, outputFile + strlen( outputFile ) );

		// parse the ini file
		DLCTool::UpdateTaskInfo( DLCTool::String_Parsing_Ini);
		DLCTool::UpdateTaskProgress( 0, 1 );
		RawFileReader* reader = new RawFileReader( wstrIniFileName.c_str() );
		UINT fileSize = reader->GetSize();

		char* iniDataPtr = new char[ fileSize + 1 ];
		reader->Serialize( iniDataPtr, fileSize );
		iniDataPtr[fileSize] = 0;
		std::string iniContents( iniDataPtr );
		delete [] iniDataPtr;

		Parser parser;
		std::vector< std::wstring > installedFiles;
		std::vector< std::wstring > deletedFiles;
		std::list< DWORD > fileSizes;
		parser.add( new TParseArrStringEntry( EP_InstallFile, installedFiles ) );
		parser.add( new TParseArrStringEntry( EP_InstallFile, deletedFiles ) );
		parser.parse( iniContents );

		WCHAR moduleName[ 1024 ];
		GetModuleFileNameW( GetModuleHandle(NULL), moduleName, ARRAYSIZE(moduleName) );
		installedFiles.push_back( moduleName );
		DLCTool::UpdateTaskProgress( 1, 1 );
		
		// go through the installed files and attach them to the archive,
		// starting from the end of the list
		RawFileWriter* writer = new RawFileWriter( wstrOutputFileName.c_str() );

		DLCTool::UpdateTaskInfo( DLCTool::String_Adding_Files);
		UINT filesCount = static_cast< UINT >( installedFiles.size() );
		for ( int i = filesCount - 1; i >= 0; --i )
		{
			RawFileReader* moduleReader = new RawFileReader( installedFiles[i].c_str() );
			UINT moduleSize = moduleReader->GetSize();
			if ( moduleSize > 0 )
			{
				char* moduleContents = new char[ moduleSize ];
				moduleReader->Serialize( moduleContents, moduleSize );
				delete moduleReader;

				writer->Serialize( moduleContents, moduleSize );
				delete [] moduleContents;
				moduleContents = NULL;
			}

			// append info about the sizes of particular files ( except for the last file, which is this module )
			if ( i != filesCount - 1 )
			{
				fileSizes.push_front( moduleSize );
			}

			DLCTool::UpdateTaskProgress( filesCount - i, filesCount );
		}

		for ( std::list< DWORD >::const_iterator it = fileSizes.begin(); it != fileSizes.end(); ++it )
		{
			char sizeStr[255];
			sprintf_s( sizeStr, "\n%s=%ld", ids[ EP_FileSize ], *it );
			iniContents += sizeStr;
		}

		// at the end - attach the layout info
		{
			DLCTool::UpdateTaskInfo( DLCTool::String_Adding_Ini );
			DLCTool::UpdateTaskProgress( 0, 1 );
			UINT iniLength = static_cast< UINT >( iniContents.length() );
			writer->Serialize( (void *)( iniContents.c_str() ), iniLength );

			// and add the layout size
			writer->Serialize( &iniLength, sizeof( UINT ) );
			DLCTool::UpdateTaskProgress( 1, 1 );
		}

		// cleanup
		delete writer;
	}

	///////////////////////////////////////////////////////////////////////////////

	void ExtractArchive( const WCHAR* gameDirectory )
	{
		WCHAR moduleName[ 1024 ];
		GetModuleFileNameW( GetModuleHandle(NULL), moduleName, ARRAYSIZE(moduleName) );
		HANDLE hModuleFile = ::CreateFile( moduleName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL );
		DWORD dataOffset = 0;
		if ( hModuleFile != INVALID_HANDLE_VALUE )
		{
			dataOffset = GetFileSize( hModuleFile, NULL );
			CloseHandle( hModuleFile );
		}

		// read the layout size
		DLCTool::UpdateTaskInfo( DLCTool::String_Reading_Layout );
		DLCTool::UpdateTaskProgress( 0, 1 );

		dataOffset -= sizeof( UINT );
		RawFileReader* sizeReader = new RawFileReader( GetModuleHandle( NULL ), dataOffset );
		UINT layoutSize = 0;
		sizeReader->Serialize( &layoutSize, sizeof( layoutSize ) );
		delete sizeReader;

		// read the layout
		dataOffset -= layoutSize;
		RawFileReader* layoutReader = new RawFileReader( GetModuleHandle( NULL ), dataOffset );
		char* iniDataPtr = new char[ layoutSize + 1 ];
		layoutReader->Serialize( iniDataPtr, layoutSize );
		iniDataPtr[layoutSize] = 0;
		std::string iniContents( iniDataPtr );
		delete [] iniDataPtr;
		delete layoutReader;

		// parse the layout and create proper commands
		Parser parser;
		std::vector< std::wstring >			installedFiles;
		std::vector< std::wstring >			deletedFiles;
		std::vector< DWORD >				fileSizes;
		UINT								regId;
		std::wstring						regName;
		parser.add( new TParseArrStringEntry( EP_InstallFile, installedFiles ) );
		parser.add( new TParseArrStringEntry( EP_DeleteFile, deletedFiles ) );
		parser.add( new TParseArrDWORDEntry( EP_FileSize, fileSizes ) );
		parser.add( new TParseIntEntry( EP_ID, regId ) );
		parser.add( new TParseStringEntry( EP_Name, regName ) );
		parser.parse( iniContents );
		DLCTool::UpdateTaskProgress( 1, 1 );


		// start installing the files
		DLCTool::UpdateTaskInfo( DLCTool::String_Installing_Files );
		UINT filesCount = static_cast< UINT >( installedFiles.size() );
		for ( UINT i = 0; i < filesCount; ++i )
		{
			DWORD fileSize = fileSizes[i];
			dataOffset -= fileSize;
			RawFileReader* reader = new RawFileReader( GetModuleHandle( NULL ), dataOffset );

			char* fileData = new char[ fileSize ];
			reader->Serialize( fileData, fileSize );
			delete reader;

			std::wstring filePath( gameDirectory );
			filePath += L"\\";
			filePath += installedFiles[i];
			RawFileWriter* writer = new RawFileWriter( filePath.c_str() );
			writer->Serialize( fileData, fileSize );
			delete writer;

			delete [] fileData;

			DLCTool::UpdateTaskProgress( i + 1, filesCount );
		}

		// delete files
		DLCTool::UpdateTaskInfo( DLCTool::String_Deleting_Files );
		filesCount = static_cast< UINT >( deletedFiles.size() );
		for ( UINT i = 0; i < filesCount; ++i )
		{
			std::wstring filePath( gameDirectory );
			filePath += L"\\";
			filePath += deletedFiles[i];
			DeleteFileW( filePath.c_str() );
			DLCTool::UpdateTaskProgress( i + 1, filesCount );
		}

		// update the registry
		{
			DLCTool::UpdateTaskInfo( DLCTool::String_Updating_Registry );
			DLCTool::UpdateTaskProgress( 0, 1 );

			HKEY gameCurrentUserKey;
			REGSAM registryAccess = KEY_SET_VALUE;
			bool result = (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, HKCU_MAINREGISTERKEY, 0, registryAccess, &gameCurrentUserKey)) && (gameCurrentUserKey != NULL);
			if ( !result )
			{
				return;
			}

			RegScopedKey downloadsKey( gameCurrentUserKey, TEXT("Downloads") );

			struct Internal
			{

				void Append( BYTE** base, const std::wstring& addition )
				{
					// String size
					unsigned short len = static_cast< unsigned short >( addition.length() );
					unsigned short size = len * static_cast< unsigned short >( sizeof(TCHAR) );
					memcpy_s(*base, sizeof(size), &size, sizeof(size));
					*base += sizeof(size);
					// String
					if (size > 0)
					{
						memcpy_s(*base, size, addition.c_str(), size);
						*base += sizeof(TCHAR) * len;
					}
				}

				void Append ( BYTE** base, UINT addition)
				{
					// UINT
					memcpy_s(*base, sizeof(addition), &addition, sizeof(addition));
					*base += sizeof(addition);
				}
				void Append ( BYTE** base, unsigned long long addition)
				{
					// ULONGLONG
					memcpy_s(*base, sizeof(addition), &addition, sizeof(addition));
					*base += sizeof(addition);
				}

			};

			std::auto_ptr< BYTE > buffer ( new BYTE [ 2048 ] );
			BYTE* ptr = buffer.get();
			Internal __internal;
			__internal.Append(&ptr, regId);
			__internal.Append(&ptr, regName);
			__internal.Append(&ptr, std::wstring());
			__internal.Append(&ptr, (UINT)1); // m_type
			__internal.Append(&ptr, (unsigned long long)1 ); // version
			__internal.Append(&ptr, (unsigned long long)1 ); // required version
			__internal.Append(&ptr, (DWORD64)0); // checksum
			__internal.Append(&ptr, (UINT)4); // m_state: Installed
			__internal.Append(&ptr, std::wstring() );

			int size = PtrToInt( (void*)( ptr - buffer.get() ) );

			std::auto_ptr<TCHAR> bytes(new TCHAR [ 100 ] );
			ZeroMemory(bytes.get(), 100 * sizeof(TCHAR));
			_i64tow_s((unsigned long)regId, bytes.get(), 100, 10);
			if ( ERROR_SUCCESS != ::RegSetValueEx( downloadsKey.m_hKey, bytes.get(), 0, REG_BINARY, buffer.get(), size) )
			{
				ThrowError( String_Info_RegistryUpdateFailure );
			}

			DLCTool::UpdateTaskProgress( 1, 1 );
		}
	}

	// -c $(TargetDir)\test.ini test.exe

	///////////////////////////////////////////////////////////////////////////////

} // DLCTool
