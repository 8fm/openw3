#include "build.h"
#include "redTelemetryServiceImplWin32.h"
#include "fileSys.h"

#include "../../../internal/telemetry/telemetryInclude.h"

//////////////////////////////////////////////////////////////////////////
//! This function is used only in Debug mode to use assert inside the telemetry lib
void telemetryAssert( bool exp, Telemetry::const_utf8 msg )
{
	RED_ASSERT( exp, ANSI_TO_UNICODE( msg ) );
	RED_UNUSED( exp );
	RED_UNUSED( msg );
}

//////////////////////////////////////////////////////////////////////////
//! This function is used only in Debug mode to use logger inside the telemetry lib
void telemetryDebug( Telemetry::const_utf8 format, ... )
{
	static AnsiChar formattedAnsiMessage[1024];

	va_list arglist;
	va_start( arglist, format );
	Red::System::VSNPrintF( formattedAnsiMessage, ARRAY_COUNT(formattedAnsiMessage), format, arglist );
	va_end (arglist);

	RED_LOG( RED_LOG_CHANNEL(Telemetry), TXT( "%hs" ), formattedAnsiMessage );
}

//! Implementation of the file read callback for ordinary files
class CRedTelemetryReadCallBack : public Telemetry::IFileReadCallBack
{
public:

	//! construct from filename
	CRedTelemetryReadCallBack(const Char* filePtah)
		: m_file( NULL )
	{
		// open file
		String absoluteFilePath = GFileManager->GetBaseDirectory() + TXT( "xdkconfig\\" );
		absoluteFilePath += filePtah;
		m_file = GFileManager->CreateFileReader( absoluteFilePath, FOF_AbsolutePath|FOF_Buffered );
		
	}

	//! destructor
	virtual ~CRedTelemetryReadCallBack()
	{
		if ( m_file )
			delete m_file;
	}

	//! Reads an amount of bytes from the file.
	virtual int read(void* buffer, int sizeToRead)
	{
		if (!m_file)
			return 0;

		Uint64 oldOffset = m_file->GetOffset();
		m_file->Serialize( buffer, sizeToRead );
		return (int)( m_file->GetOffset() - oldOffset );
	}

	//! Returns size of file in bytes
	virtual int getSize()
	{
		if (!m_file)
			return 0;

		return (int)m_file->GetSize();
	}

private:

	IFile* m_file;

}; // end class CFileReadCallBack


class CRedTelemetryFileManager: public Telemetry::IFileReadCallBackManager
{
public:
	virtual Telemetry::IFileReadCallBack* openFile( Telemetry::const_utf8 filePath )
	{
		return new CRedTelemetryReadCallBack( ANSI_TO_UNICODE( filePath ) );
	}

	virtual void closeFile( Telemetry::IFileReadCallBack* file )
	{
		if( file )
		{
			delete file;
		}
	}
};

//////////////////////////////////////////////////////////////////////////
//! This function is used to construct one string with proper max length
void ConstructTagsString( const TDynArray<String>& tags, String& tagsString )
{
	tagsString.Clear();
	TDynArray<String>::const_iterator end = tags.End();
	for ( TDynArray<String>::const_iterator i = tags.Begin(); i != end; ++i )
	{
		size_t predictedSize = i->GetLength() + tagsString.GetLength();
		if( tagsString.GetLength() > 0 )
		{
			predictedSize += 1;
		}
		if( predictedSize > 255 )
		{
			RED_LOG( RED_LOG_CHANNEL(Telemetry), TXT( "Can not add %s tag. Tags string will be to long (max tags string length can be 255 characters)!!!" ), i->AsChar() );
		}
		else
		{
			if( tagsString.GetLength() > 0 )
			{
				tagsString += TXT(" ");
			}	

			tagsString += (*i).ToLower();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
//! This object is used to set memory allocators
class CRedTelemetryServiceMemoryAllocator : public Telemetry::IMemory
{
public:
	virtual void* alloc( size_t size )
	{
		return RED_MEMORY_ALLOCATE_ALIGNED( MemoryPool_Default, MC_Package, size, 16 );
	}

	virtual void* realloc( void* mem, size_t size )
	{
		return RED_MEMORY_REALLOCATE_ALIGNED( MemoryPool_Default, mem, MC_Package, size, 16 );
	}

	virtual void free( void* mem )
	{
		RED_MEMORY_FREE( MemoryPool_Default, MC_Package, mem );
	}
};

CRedTelemetryServiceImplWin32::CRedTelemetryServiceImplWin32() 
{
	// In debug mode one can set assert function
#if defined( NO_TELEMETRY_DEBUG )
	Telemetry::CTelemetryInterfaceManager::Init( (CRedTelemetryServiceMemoryAllocator*) new( RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Package, sizeof( CRedTelemetryServiceMemoryAllocator ) ) )CRedTelemetryServiceMemoryAllocator(),
		(CRedTelemetryFileManager*) new( RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Package, sizeof( CRedTelemetryFileManager ) ) )CRedTelemetryFileManager() );
#else
	Telemetry::CTelemetryInterfaceManager::Init( (CRedTelemetryServiceMemoryAllocator*) new( RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Package, sizeof( CRedTelemetryServiceMemoryAllocator ) ) )CRedTelemetryServiceMemoryAllocator(),
		(CRedTelemetryFileManager*) new( RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Package, sizeof( CRedTelemetryFileManager ) ) )CRedTelemetryFileManager() , 
		telemetryAssert,
		telemetryDebug );
#endif

	m_libInterface = (Telemetry::CTelemetryInterfaceManager*)new( RED_MEMORY_ALLOCATE_ALIGNED( MemoryPool_Default, MC_Package, sizeof( Telemetry::CTelemetryInterfaceManager ), 16 ) ) Telemetry::CTelemetryInterfaceManager();
	m_dumpedData = nullptr;
	m_immediatePost = false;
}

CRedTelemetryServiceImplWin32::~CRedTelemetryServiceImplWin32()
{
	delete m_libInterface;
	m_libInterface = nullptr;
	if( m_dumpedData )
	{
		RED_MEMORY_FREE( MemoryPool_Default, MC_Package, m_dumpedData );
		m_dumpedData = nullptr;
	}
}
;
void CRedTelemetryServiceImplWin32::Configure( const SRedTelemetryServiceConfig& config, Telemetry::EBackendTelemetry backendName )
{
	m_config = config;

	if( m_libInterface )
	{
		m_libInterface->SetDelegate( this );
		m_libInterface->AddBackend( backendName );
		m_libInterface->SetBatchAggregationTime( m_config.batchSeconds );
		m_libInterface->SetMaximumBatchCount( m_config.batchCount );

		if( config.useDefaultPlayerConfig )
		{
			m_libInterface->ConfigTelemetry( m_config.gameId, m_config.gameVersion, m_config.platform,
				UNICODE_TO_ANSI( m_config.language.AsChar() ),
				m_config.userCategory,
				m_config.curlTimeout,
				m_config.googleTrackingID.Empty() ? NULL : UNICODE_TO_ANSI( m_config.googleTrackingID.AsChar() ) );
		}
		else
		{
			m_libInterface->ConfigTelemetry( m_config.gameId, m_config.gameVersion, m_config.platform,
				UNICODE_TO_ANSI( m_config.deviceId.AsChar() ), 
				UNICODE_TO_ANSI( m_config.language.AsChar() ),
				m_config.userCategory,
				m_config.curlTimeout,
				m_config.googleTrackingID.Empty() ? NULL : UNICODE_TO_ANSI( m_config.googleTrackingID.AsChar() ) );
		}

		m_libInterface->ConfigTelemetryHTTPService( backendName, UNICODE_TO_ANSI( m_config.serverHost.AsChar() ), m_config.serverPort );
	}
}

Bool CRedTelemetryServiceImplWin32::StartCollecting()
{
	return m_libInterface->StartCollecting();
}

Bool CRedTelemetryServiceImplWin32::StartSession( const String& playerId, const String& parentSessionId )
{
	AnsiChar* strPlayerId = NULL;
	if( !playerId.Empty() )
		strPlayerId = UNICODE_TO_ANSI( playerId.AsChar() );

	AnsiChar* strParentSessionId = NULL;
	if( !parentSessionId.Empty() )
		strParentSessionId = UNICODE_TO_ANSI( parentSessionId.AsChar() );

	String tagsString;

	ConstructTagsString( m_sessionTags, tagsString );

	AnsiChar* strSessionTags = NULL;
	strSessionTags = UNICODE_TO_ANSI( tagsString.AsChar() );

	return m_libInterface->StartSession( strPlayerId, strParentSessionId, strSessionTags );
}

void CRedTelemetryServiceImplWin32::Update( Bool immediatePost )
{
	m_libInterface->Update( immediatePost );
}

Bool CRedTelemetryServiceImplWin32::StopSession()
{
	return m_libInterface->StopSession();
}

Bool CRedTelemetryServiceImplWin32::StopCollecting()
{
	return m_libInterface->StopCollecting();
}

IFile* CRedTelemetryServiceImplWin32::GetDumpedDataHandler()
{
	if( m_dumpedData == nullptr )
	{
#ifdef RED_PLATFORM_DURANGO
		String pathToDumpFile = TXT("t:\\");
		pathToDumpFile += m_config.GetDumpFileName();
#else
		String pathToDumpFile = GFileManager->GetUserDirectory();
		pathToDumpFile += m_config.GetDumpFileName();
#endif // RED_PLATFORM_DURANGO
		m_dumpedData = GFileManager->CreateFileReader( pathToDumpFile, FOF_Buffered|FOF_AbsolutePath );
	}
	return m_dumpedData;
}

Bool CRedTelemetryServiceImplWin32::IsTransferInProgress() const
{
	return m_libInterface->IsOperationInProgress();
}

void CRedTelemetryServiceImplWin32::Log( const String& eventName, const String& eventCategory )
{
	m_libInterface->Log( UNICODE_TO_ANSI( eventName.AsChar() ), 
						UNICODE_TO_ANSI( eventCategory.AsChar() ) );
}

void CRedTelemetryServiceImplWin32::LogL( const String& eventName, const String& eventCategory, const String& eventLabel )
{
	m_libInterface->LogL( UNICODE_TO_ANSI( eventName.AsChar() ),
						UNICODE_TO_ANSI( eventCategory.AsChar() ), 
						UNICODE_TO_ANSI( eventLabel.AsChar() ) );
}

void CRedTelemetryServiceImplWin32::LogV(const String& name, const String& category, Int32 value )
{
	m_libInterface->LogV( UNICODE_TO_ANSI( name.AsChar() ), 
						UNICODE_TO_ANSI( category.AsChar() ),
						value );
}

void CRedTelemetryServiceImplWin32::LogV(const String& name, const String& category, const String& value )
{
	m_libInterface->LogV( UNICODE_TO_ANSI( name.AsChar() ), 
						UNICODE_TO_ANSI( category.AsChar() ),
						UNICODE_TO_ANSI( value.AsChar() ) );
}

void CRedTelemetryServiceImplWin32::LogV_WS(const String& name, const String& category, const String& value )
{
	m_libInterface->LogV( UNICODE_TO_ANSI( name.AsChar() ), 
		UNICODE_TO_ANSI( category.AsChar() ),
		(void*)value.AsChar(), value.GetLength()*sizeof(Char) );
}

void CRedTelemetryServiceImplWin32::LogVL( const String& eventName, const String& eventCategory, const String& eventValue, const String& eventLabel )
{
	m_libInterface->LogVL( UNICODE_TO_ANSI( eventName.AsChar() ), 
						UNICODE_TO_ANSI( eventCategory.AsChar() ), 
						UNICODE_TO_ANSI( eventValue.AsChar() ),
						UNICODE_TO_ANSI( eventLabel.AsChar() ) );
}

void CRedTelemetryServiceImplWin32::LogVL( const String& eventName, const String& eventCategory, Int32 eventValue, const String& eventLabel )
{
	m_libInterface->LogVL( UNICODE_TO_ANSI( eventName.AsChar() ), 
						UNICODE_TO_ANSI( eventCategory.AsChar() ), 
						eventValue,
						UNICODE_TO_ANSI( eventLabel.AsChar() ) );

}
void CRedTelemetryServiceImplWin32::LogVL_WS( const String& eventName, const String& eventCategory, const String& eventValue, const String& eventLabel )
{
	m_libInterface->LogVL( UNICODE_TO_ANSI( eventName.AsChar() ), 
		UNICODE_TO_ANSI( eventCategory.AsChar() ), 
		(void*)eventValue.AsChar(), eventValue.GetLength()*sizeof(Char),
		UNICODE_TO_ANSI( eventLabel.AsChar() ) );
}

void CRedTelemetryServiceImplWin32::LogEx	( const String& ex )
{
	m_libInterface->SendExceptionCategoryW( "w3exception", ex.AsChar() );
}

void CRedTelemetryServiceImplWin32::SetCommonStatValue( const String& name, Float value )
{
	m_libInterface->SetCommonParam( UNICODE_TO_ANSI( name.AsChar() ), value );
}

void CRedTelemetryServiceImplWin32::SetCommonStatValue( const String& name, Int32 value )
{
	m_libInterface->SetCommonParam( UNICODE_TO_ANSI( name.AsChar() ), value );
}

void CRedTelemetryServiceImplWin32::SetCommonStatValue( const String& name, const String& value )
{
	m_libInterface->SetCommonParam( UNICODE_TO_ANSI( name.AsChar() ), UNICODE_TO_ANSI( value.AsChar() ) );
}

bool CRedTelemetryServiceImplWin32::AppendDataToLocalStorage( const void* const data, unsigned int dataSize )
{
	// TODO: create file consistently on all platforms
	bool success = false;
	if( dataSize > 0 )
	{
#ifdef RED_PLATFORM_DURANGO
		String pathToDumpFile = TXT("t:\\");
		pathToDumpFile += m_config.GetDumpFileName();
#else
		String pathToDumpFile = GFileManager->GetUserDirectory();
		pathToDumpFile += m_config.GetDumpFileName();
#endif // RED_PLATFORM_DURANGO

		IFile* file = GFileManager->CreateFileWriter( pathToDumpFile, FOF_Buffered|FOF_AbsolutePath, static_cast<Uint32>( dataSize ) );
		if( file )
		{
			file->Serialize( const_cast<void*>( data ), static_cast<Uint32>( dataSize ) );
			success = true;
			delete file;
		}
	}

	return success;
}

unsigned int CRedTelemetryServiceImplWin32::DataSize()
{
	Uint64 size = 0;
	if( GetDumpedDataHandler() )
		size = GetDumpedDataHandler()->GetSize();

	return static_cast< unsigned int >( size );
}

bool CRedTelemetryServiceImplWin32::ReadDataToBuffer( void* buffer )
{
	bool success = false;

	if( GetDumpedDataHandler() )
	{
		GetDumpedDataHandler()->Serialize( buffer, static_cast<Uint32>( DataSize() ) ); 
		success = true;
		delete m_dumpedData;
		m_dumpedData = NULL;
	}

	return success;
}

void CRedTelemetryServiceImplWin32::DeleteCachedData()
{
#ifdef RED_PLATFORM_DURANGO
	String pathToDumpFile = TXT("t:\\");
	pathToDumpFile += m_config.GetDumpFileName();
#else
	String pathToDumpFile = GFileManager->GetUserDirectory();
	pathToDumpFile += m_config.GetDumpFileName();
#endif // RED_PLATFORM_DURANGO

	GFileManager->DeleteFile( pathToDumpFile );
}

void CRedTelemetryServiceImplWin32::SetExternalSessionId( Telemetry::EBackendTelemetry backendName, const String& extSessionId )
{
	m_libInterface->SetExternalSessionId(backendName, UNICODE_TO_ANSI( extSessionId.AsChar() ) );
}

const String& CRedTelemetryServiceImplWin32::GetSessionId( Telemetry::EBackendTelemetry backendName )
{
	m_sessionIdRedString = ANSI_TO_UNICODE(m_libInterface->GetSessionId( backendName ));
	return m_sessionIdRedString;
}

void CRedTelemetryServiceImplWin32::GetTime( double& time, unsigned long long& qpf, unsigned long long& qpc )
 {
	 m_libInterface->GetTime( time, qpf, qpc );
 }

void CRedTelemetryServiceImplWin32::SetImmediatePost( bool val )
{
	m_immediatePost = val;
	m_libInterface->SetImmediatePost( m_immediatePost );
}

Bool CRedTelemetryServiceImplWin32::GetImmediatePost()
{
	return m_immediatePost;
}

Bool CRedTelemetryServiceImplWin32::RemoveSessionTag( const String& tag )
{
	return m_sessionTags.Remove( tag );
}

void CRedTelemetryServiceImplWin32::AddSessionTag( const String& tag )
{
	if( tag.Empty() == false )
	{
		m_sessionTags.PushBackUnique( tag );
	}
	
}

void CRedTelemetryServiceImplWin32::RemoveAllSessionTags()
{
	m_sessionTags.Clear();
}
