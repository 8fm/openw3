#include "build.h"
#include "gameSaveFileNoSeeks.h"
#include "../core/fileSkipableBlock.h"
#include "gameSaveFile.h"
#include "gameDataStorage.h"

#define VALUE_MAGIC_TAG			Uint16( ( 'L' << 8 ) + 'V' )
#define PROPERTY_MAGIC_TAG		Uint16( ( 'P' << 8 ) + 'O' )
#define STORAGE_STREAM_TAG		Uint16( ( 'S' << 8 ) + 'S' )
#define BLOCK_START_TAG			Uint16( ( 'S' << 8 ) + 'B' )
#define SAVE_END_TAG			Uint16( ( 'C' << 8 ) + 'S' )
#define CNAME_MAPPING_TAG		Uint16( ( 'M' << 8 ) + 'N' )
#define ROOT_BLOCKS_TAG			Uint16( ( 'B' << 8 ) + 'R' )

#define GAMESAVE_LOG( txt, ... )		RED_LOG( GameSave, txt, ##__VA_ARGS__ )
#define GAMESAVE_WARN( txt, ... )		RED_LOG( GameSave, txt, ##__VA_ARGS__ )
#define GAMESAVE_ERROR( txt, ... )		RED_LOG( GameSave, txt, ##__VA_ARGS__ )

// Use this define to test for invalid / unexpected data by injecting random stuff into a save game
//#define ENABLE_RANDOM_EXTRA_DATA_FOR_TESTING

#ifdef ENABLE_RANDOM_EXTRA_DATA_FOR_TESTING
__declspec( thread ) int stack_depth = 0;
void InjectRandomData(CGameFileSaverNoSeeks* writer)
{
	if( stack_depth > 0 )
	{
		return;
	}
	stack_depth++;

	int r = rand() % 10000;
	if( r < 200 )
	{
		CName newName( String::Printf( TXT( "rand_debug_block_%d" ), r ) );
		writer->BeginBlock( newName );
		writer->EndBlock( newName );
	}
	else if( r < 400 )
	{
		CName randomValue1( TXT( "RandomVal1" ) );
		const AnsiChar v[] = "Some random test data";
		writer->WriteRawAnsiValue( randomValue1, v, sizeof( v ) );
	}
	else if( r < 600 )
	{
		CName randomValue2( TXT( "RandomVal2" ) );
		const AnsiChar v[] = "ARRRRSE";
		writer->WriteRawAnsiValue( randomValue2, v, sizeof( v ) );
	}
	else if( r < 800 )
	{
		CName randomValue3( TXT( "RandomVal3" ) );
		const AnsiChar v[] = "Bleh";
		writer->WriteRawAnsiValue( randomValue3, v, sizeof( v ) );
	}
	else if( r < 1000 )
	{
		CName newName( String::Printf( TXT( "rand_debug_block_%d" ), r ) );
		writer->BeginBlock( newName );

		CName randomValue4( TXT( "RandomVal4" ) );
		const AnsiChar v[] = "MEEERP";
		writer->WriteRawAnsiValue( randomValue4, v, sizeof( v ) );

		writer->EndBlock( newName );
	}

	stack_depth--;
}
#endif

// Rather than saving a header, a footer is used instead to avoid seeks during write
struct CSaveGameFooter
{
	Uint32 m_cnameRemappingDataOffset;
	Uint32 m_rootBlockDataOffset;
};

CGameFileSaverNoSeeks::CGameFileSaverNoSeeks( IFileEx* saveDataFile )
	: m_saveDataFile( saveDataFile )
{
	RED_FATAL_ASSERT( m_saveDataFile, "File saver must be passed a valid stream" );
	RED_FATAL_ASSERT( !m_saveDataFile->IsSkipBlockDataInline(), "File should NOT accept inline skip block data" );
	RED_FATAL_ASSERT( m_saveDataFile->QuerySkippableBlockCache(), "File must have skippable block cache interface" );
}

CGameFileSaverNoSeeks::~CGameFileSaverNoSeeks()
{
	RED_FATAL_ASSERT( m_openBlockStack.Empty(), "Some save-data blocks were not closed! Save will be uncomplete/corrupt" );
	delete m_saveDataFile;
}

void CGameFileSaverNoSeeks::BeginBlock( CName name )
{
	if( m_openBlockStack.Empty() )
	{
		BlockData newRoot( name, static_cast< Uint32 >( m_saveDataFile->GetOffset() ) );
		m_rootBlocks.PushBack( newRoot );
	}

	Uint16 magic = BLOCK_START_TAG;
	Uint32 startOffset = static_cast< Uint32 >( m_saveDataFile->GetOffset() );

	*m_saveDataFile << magic;
	*m_saveDataFile << name;

	BlockData newBlock( name, startOffset );
	m_openBlockStack.PushBack( newBlock );

#ifdef ENABLE_RANDOM_EXTRA_DATA_FOR_TESTING
	InjectRandomData( this );
#endif
}

void CGameFileSaverNoSeeks::EndBlock( CName name )
{
#ifdef ENABLE_RANDOM_EXTRA_DATA_FOR_TESTING
	InjectRandomData( this );
#endif

	RED_FATAL_ASSERT( m_openBlockStack.Size() > 0, "Begin/End block mismatch" );

	BlockData endBlock = m_openBlockStack.PopBack();
	RED_FATAL_ASSERT( endBlock.m_name == name, "EndBlock name mismatch" );

	Uint32 blockEndOffset = static_cast< Uint32 >( m_saveDataFile->GetOffset() );

	// Manually adds a skipable block cache entry here, so the entire block can be skipped if it is bad
	RED_FATAL_ASSERT( m_saveDataFile->QuerySkippableBlockCache(), "No skippable block cache found. Wrong IFile" );
	m_saveDataFile->QuerySkippableBlockCache()->RegisterSkippableBlockOffset( endBlock.m_skipOffset, blockEndOffset - endBlock.m_skipOffset );
}

void CGameFileSaverNoSeeks::WriteRawAnsiValue( CName name, const AnsiChar* data, Uint32 size )
{
#ifdef ENABLE_RANDOM_EXTRA_DATA_FOR_TESTING
	InjectRandomData( this );
#endif

	RED_FATAL_ASSERT( !m_openBlockStack.Empty(), "Values should always be written in a block" );

	CFileSkipableBlock skipBlock( *m_saveDataFile );
	{
		Uint16 valMagic = VALUE_MAGIC_TAG;
		CName typeV = CNAME( String );

		*m_saveDataFile << valMagic;
		*m_saveDataFile << name;
		*m_saveDataFile << typeV;			// Write raw ansi data as String

		Int32 byteCountNegative = -Int32( size ); 
		*m_saveDataFile << CCompressedNumSerializer( byteCountNegative );
		m_saveDataFile->Serialize( ( void* ) data, size * sizeof( AnsiChar ) );
	}

#ifdef ENABLE_RANDOM_EXTRA_DATA_FOR_TESTING
	InjectRandomData( this );
#endif
}

void CGameFileSaverNoSeeks::WriteValue( CName name, IRTTIType* type, const void* data )
{
#ifdef ENABLE_RANDOM_EXTRA_DATA_FOR_TESTING
	InjectRandomData( this );
#endif

	RED_FATAL_ASSERT( !m_openBlockStack.Empty(), "Values should always be written in a block" );

	CFileSkipableBlock skipBlock( *m_saveDataFile );
	{
		Uint16 valMagic = VALUE_MAGIC_TAG;
		CName typeV = type->GetName();

		*m_saveDataFile << valMagic;
		*m_saveDataFile << name;
		*m_saveDataFile << typeV;

		type->Serialize( *m_saveDataFile, const_cast< void* >( data ) );
	}

#ifdef ENABLE_RANDOM_EXTRA_DATA_FOR_TESTING
	InjectRandomData( this );
#endif
}

void CGameFileSaverNoSeeks::WriteProperty( void* object, CProperty* prop )
{
#ifdef ENABLE_RANDOM_EXTRA_DATA_FOR_TESTING
	InjectRandomData( this );
#endif

	RED_FATAL_ASSERT( !m_openBlockStack.Empty(), "Values should always be written in a block" );
	RED_FATAL_ASSERT( object, "Cannot save data for null object!" );

	Uint16 propMagic = PROPERTY_MAGIC_TAG;
	CName name = prop->GetName();
	IRTTIType* type = prop->GetType();
	void* data = prop->GetOffsetPtr( object );
	CName typeV = type->GetName();

	RED_FATAL_ASSERT( type, "Cannot save data for property with null type!" );
	RED_FATAL_ASSERT( data, "Cannot save data for property with null data!" );

	CFileSkipableBlock skipBlock( *m_saveDataFile );
	{
		*m_saveDataFile << propMagic;
		*m_saveDataFile << name;	
		*m_saveDataFile << typeV;
		type->Serialize( *m_saveDataFile, data );
	}

#ifdef ENABLE_RANDOM_EXTRA_DATA_FOR_TESTING
	InjectRandomData( this );
#endif
}

void CGameFileSaverNoSeeks::AddStorageStream( IGameDataStorage* storageStream )
{
	RED_FATAL_ASSERT( !m_openBlockStack.Empty(), "Values should always be written in a block" );

#ifdef ENABLE_RANDOM_EXTRA_DATA_FOR_TESTING
	InjectRandomData( this );
#endif

	Uint16 magic = STORAGE_STREAM_TAG;
	Uint32 size = storageStream ? storageStream->GetSize() : 0;

	CFileSkipableBlock skipBlock( *m_saveDataFile );
	{
		*m_saveDataFile << magic;
		*m_saveDataFile << size;
		if( size != 0 )
		{
			m_saveDataFile->Serialize( const_cast< void* >( storageStream->GetData() ), size );
		}
	}

#ifdef ENABLE_RANDOM_EXTRA_DATA_FOR_TESTING
	InjectRandomData( this );
#endif
}

void CGameFileSaverNoSeeks::Finalize()
{
	// Root block data
	Uint32 rootBlockOffset = static_cast< Uint32 >( m_saveDataFile->GetOffset() );
	Uint16 rootMagic = ROOT_BLOCKS_TAG;
	Uint32 arraySize = m_rootBlocks.Size();
	*m_saveDataFile << rootMagic;
	*m_saveDataFile << arraySize;
	for( auto rootBlock : m_rootBlocks )
	{
		CName rootName = rootBlock.m_name;
		Uint32 rootOffset = rootBlock.m_skipOffset;
		*m_saveDataFile << rootName;
		*m_saveDataFile << rootOffset;
	}

	// CName remapping data
	Uint32 cnameRemappingOffset = static_cast< Uint32 >( m_saveDataFile->GetOffset() );
	Uint16 cnameMappingTag = CNAME_MAPPING_TAG;
	*m_saveDataFile << cnameMappingTag;
	SGameSessionManager::GetInstance().GetCNamesRemapper().Save( m_saveDataFile );
	GAMESAVE_LOG( TXT( "CName remapping data took %d bytes in save-game" ), static_cast< Uint32 >( m_saveDataFile->GetOffset() ) - cnameRemappingOffset );

	CSaveGameFooter m_footerData;
	m_footerData.m_cnameRemappingDataOffset = cnameRemappingOffset;
	m_footerData.m_rootBlockDataOffset = rootBlockOffset;
	*m_saveDataFile << m_footerData.m_cnameRemappingDataOffset;
	*m_saveDataFile << m_footerData.m_rootBlockDataOffset;

	Uint16 saveCompleteMagic = SAVE_END_TAG;
	*m_saveDataFile << saveCompleteMagic;

	GAMESAVE_LOG( TXT( "Save game completed. Root block data took %d bytes, CName remapping took %d bytes, actual data took %d bytes" ),
			cnameRemappingOffset - rootBlockOffset,
			m_saveDataFile->GetOffset() - cnameRemappingOffset,
			rootBlockOffset );
}

void CGameFileSaverNoSeeks::Close()
{
	m_saveDataFile->Close();
}

const void* CGameFileSaverNoSeeks::GetData() const
{
	RED_FATAL_ASSERT( false, "Do not use CGameSplitFileSaver::GetData - Not supported" );
	return nullptr;
}

Uint32 CGameFileSaverNoSeeks::GetDataSize() const
{
	return static_cast< Uint32 >( m_saveDataFile->GetSize() );
}

Uint32 CGameFileSaverNoSeeks::GetDataCapacity() const
{
	RED_FATAL_ASSERT( false, "Do not use CGameSplitFileSaver::GetDataCapacity - Not supported" );
	return 0;
}

//////////////////////////////////////////////////////////////////////////////

CGameFileLoaderNoSeeks::CGameFileLoaderNoSeeks( IFile* saveDataFile, Uint32 gameVersion, Uint32 saveVersion )
	: m_saveDataFile( saveDataFile )
	, m_saveVersion( saveVersion )
	, m_gameVersion( gameVersion )
{
	// Read the footer data
	Uint32 initialOffset = static_cast< Uint32 >( m_saveDataFile->GetOffset() );

	Uint32 footerOffset = static_cast< Uint32 >( m_saveDataFile->GetSize() ) - ( sizeof( Uint16 ) + sizeof( Uint32 ) * 2 );
	m_saveDataFile->Seek( footerOffset );

	CSaveGameFooter footerData;
	*m_saveDataFile << footerData.m_cnameRemappingDataOffset;
	*m_saveDataFile << footerData.m_rootBlockDataOffset;

	Uint16 saveCompleteMagic = 0;
	*m_saveDataFile << saveCompleteMagic;
	if( saveCompleteMagic  != SAVE_END_TAG )
	{
		GAMESAVE_WARN( TXT( "Incomplete save game - expected save-end tag" ) );
		m_saveDataFile->Seek( initialOffset );
		return ;
	}

	if( !LoadCNameRemapping( footerData.m_cnameRemappingDataOffset ) )
	{
		GAMESAVE_WARN( TXT( "Failed to load CName remapping data" ) );
		m_saveDataFile->Seek( initialOffset );
		return;
	}

	if( !LoadRootblockData( footerData.m_rootBlockDataOffset ) )
	{
		GAMESAVE_WARN( TXT( "Failed to load root block offset data" ) );
		m_saveDataFile->Seek( initialOffset );
		return;
	}

	m_saveDataFile->Seek( initialOffset );
}

CGameFileLoaderNoSeeks::~CGameFileLoaderNoSeeks()
{
	delete m_saveDataFile;
}

Bool CGameFileLoaderNoSeeks::LoadCNameRemapping( Uint32 fileOffset )
{
	m_saveDataFile->Seek( fileOffset );
	Uint16 remapMagic = 0;
	*m_saveDataFile << remapMagic;
	if( remapMagic != CNAME_MAPPING_TAG )
	{
		GAMESAVE_WARN( TXT( "Incomplete save game - expected CName remapping" ) );
		return false;
	}
	SGameSessionManager::GetInstance().GetCNamesRemapper().Load( m_saveDataFile );

	return true;
}

Bool CGameFileLoaderNoSeeks::LoadRootblockData( Uint32 fileOffset )
{
	m_saveDataFile->Seek( fileOffset );

	Uint16 rootMagic = 0;
	*m_saveDataFile << rootMagic;
	if( rootMagic != ROOT_BLOCKS_TAG )
	{
		GAMESAVE_WARN( TXT( "Expected root block tag" ) );
		return false;
	}

	Uint32 rootBlockCount = 0;
	*m_saveDataFile << rootBlockCount;
	m_rootBlockOffsets.Reserve( rootBlockCount );
	for( Uint32 r = 0; r < rootBlockCount; ++r )
	{
		CName rootName;
		Uint32 rootOffset = 0;
		*m_saveDataFile << rootName;
		*m_saveDataFile << rootOffset;
		m_rootBlockOffsets.Insert( rootName, rootOffset );
	}

	return true;
}

Uint32 CGameFileLoaderNoSeeks::GetGameVersion() const
{
	return m_gameVersion;
}

Uint32 CGameFileLoaderNoSeeks::GetSaveVersion() const
{
	return m_saveVersion;
}

void CGameFileLoaderNoSeeks::BeginBlock( CName blockName )
{
	// If we are trying to begin a root block, we have the advantage of seeking to the correct place
	if( m_openBlockStack.Empty() )
	{
		Uint32 rootOffset = 0;
		if( m_rootBlockOffsets.Find( blockName, rootOffset ) )
		{
			m_saveDataFile->Seek( rootOffset );
		}
		else
		{
			GAMESAVE_WARN( TXT( "Failed to find root block '%ls' in roots list. Attempting manual load" ), blockName.AsChar() );
		}
	}

	// We skip backwards if the block wasn't found so we don't miss other data
	Uint32 initialOffset = static_cast< Uint32 >( m_saveDataFile->GetOffset() );

	Uint16 magic = 0;
	CName readBlockName;
	CFileSkipableBlock skipBlock( *m_saveDataFile );
	{
		*m_saveDataFile << magic;
		if( magic != BLOCK_START_TAG )
		{
			GAMESAVE_WARN( TXT( "Expected block start '%ls'" ), blockName.AsChar() );
			m_saveDataFile->Seek( initialOffset );		// Block data not found, skip back to ensure we don't miss something
			return;
		}
		*m_saveDataFile << readBlockName;
		if( blockName != readBlockName )
		{
			GAMESAVE_WARN( TXT( "Expected opening block '%ls' but instead found '%ls'" ), blockName.AsChar(), readBlockName.AsChar() );
			m_saveDataFile->Seek( initialOffset );		// Wrong block, skip back so something might read the data properly later
			return;
		}
		OpenBlock newBlock( readBlockName, static_cast< Uint32 >( skipBlock.GetEndOffset() ) );
		m_openBlockStack.PushBack( newBlock );
	}
}

void CGameFileLoaderNoSeeks::EndBlock( CName blockName )
{
	RED_ASSERT( m_openBlockStack.Size() != 0, TXT( "Begin / End block mismatch! Please debug this" ) );
	if( m_openBlockStack.Size() == 0 )
	{
		GAMESAVE_ERROR( TXT( "Ending block when nothing is on the loading stack" ) );
		return;
	}

	if( m_openBlockStack.Back().m_name != blockName )
	{
		GAMESAVE_WARN( TXT( "Expected end of block '%ls', but instead found '%ls' on the stack" ), blockName.AsChar(), m_openBlockStack.Back().m_name.AsChar() );
		// Don't seek; there may be other data to read first
		return;	
	}

	// Upon ending a block, always seek to the end in case extra data exists that we don't read
	m_saveDataFile->Seek( m_openBlockStack.Back().m_dataEndOffset );	
	m_openBlockStack.PopBack();
}

Bool CGameFileLoaderNoSeeks::ValueTypeIsCompatible( IRTTIType* requestedType, CName actualTypeName )
{
	if( requestedType->GetName() == actualTypeName )
	{
		return true;
	}
	else
	{
		// We can handle type names changing if their 'signature' is still the same (i.e. the data is compatible)
		IRTTIType* oldType = SRTTI::GetInstance().FindType( actualTypeName );
		if( !oldType )
		{
			GAMESAVE_WARN( TXT( "Type '%ls' not found when requesting data of type '%ls'" ), actualTypeName.AsChar(), requestedType->GetName().AsChar() );
			return false;
		}
		if( oldType && oldType->GetType() == RT_Array && requestedType->GetType() == RT_Array )
		{
			// Arrays are compatible as long as their internal type is the same
			CRTTIArrayType* newArrayType = static_cast< CRTTIArrayType* > ( requestedType );
			CRTTIArrayType* oldArrayType = static_cast< CRTTIArrayType* > ( oldType );
			if ( newArrayType->GetInnerType() != oldArrayType->GetInnerType() )
			{
				GAMESAVE_WARN( TXT("Array inner values are not compatible: '%ls' vs '%ls'"), newArrayType->GetName().AsChar(), oldArrayType->GetName().AsChar() );
				return false;
			}
			else
			{
				return true;
			}
		}
		// Anything else is bad
		GAMESAVE_WARN( TXT( "Incompatible data types - '%ls' vs '%ls" ), requestedType->GetName().AsChar(), actualTypeName.AsChar() );
		return false;
	}
}

void CGameFileLoaderNoSeeks::ReadValue( CName name, IRTTIType* type, const void* data, CObject* defaultParent )
{
	RED_ASSERT( !m_openBlockStack.Empty(), TXT( "Values should always be read in a block" ) );

	// Set default parent object of save data reader so it can create and instantiate objects correctly
	CGameStorageReader::ScopedDefault scopedDefaultParent( static_cast< CGameStorageReader* >( m_saveDataFile ), defaultParent );

	Uint16 valMagic = 0;
	CName typeNameFromData, valueName;
	Uint32 initialOffset = static_cast< Uint32 >( m_saveDataFile->GetOffset() );
	CFileSkipableBlock skipBlock( *m_saveDataFile );
	{
		*m_saveDataFile << valMagic;
		if( valMagic != VALUE_MAGIC_TAG )
		{
			GAMESAVE_WARN( TXT( "Expected value tag, but none found ('%ls')" ), name.AsChar() );
			m_saveDataFile->Seek( initialOffset );		// No value in stream, seek back to ensure next data is read properly
			return;
		}

		*m_saveDataFile << valueName;
		if( valueName != name )
		{
			GAMESAVE_WARN( TXT( "Expected value '%ls', but instead found '%ls'" ), name.AsChar(), valueName.AsChar() );
			m_saveDataFile->Seek( initialOffset );		// Seek back in case something else wants this value
			return;
		}

		*m_saveDataFile << typeNameFromData;
		if( !ValueTypeIsCompatible( type, typeNameFromData ) )
		{
			GAMESAVE_WARN( TXT( "Value types are not compatible for '%ls' - expected '%ls', but found '%ls'" ), name.AsChar(), type->GetName().AsChar(), typeNameFromData.AsChar() );
			skipBlock.Skip();							// We can't do anything else with this value but skip it
			return;
		}

		type->Serialize( *m_saveDataFile, const_cast< void* >( data ) );
		skipBlock.Skip();		// Always seek to end of data in case serialisation didn't leave us in a good state
	}
}

void CGameFileLoaderNoSeeks::ReadProperty( void* object, CClass* theClass, CObject* defaultParent )
{
	RED_FATAL_ASSERT( !m_openBlockStack.Empty(), "Properties should always be written in a block" );
	RED_FATAL_ASSERT( object, "Cannot save data for null object!" );
	RED_FATAL_ASSERT( theClass, "Null class!?" );

	Uint16 propMagic = 0;
	CName name;
	CName typeV;
	Uint32 initialOffset = static_cast< Uint32 >( m_saveDataFile->GetOffset() );
	CFileSkipableBlock skipBlock( *m_saveDataFile );
	{
		*m_saveDataFile << propMagic;
		if( propMagic != PROPERTY_MAGIC_TAG )
		{
			GAMESAVE_WARN( TXT( "Expected property '%ls', but no tag found" ), name.AsChar() );
			m_saveDataFile->Seek( initialOffset );		// Skip back, something else may want this data
			return;
		}

		*m_saveDataFile << name;
		CProperty* prop = theClass->FindProperty( name );
		if( !prop )
		{
			GAMESAVE_WARN( TXT( "Property '%ls' not found in class '%ls'" ), name.AsChar(), theClass->GetName().AsChar() );
			skipBlock.Skip();		// Can't do anything else but skip this property
			return;
		}

		*m_saveDataFile << typeV;
		if( !ValueTypeIsCompatible( prop->GetType(), typeV ) )
		{
			GAMESAVE_WARN( TXT( "Property types are not compatible ('%ls' - %ls vs %ls)" ), name.AsChar(), prop->GetType()->GetName().AsChar(), typeV.AsChar() );
			skipBlock.Skip();		// Can't do anything else but skip this property
			return;
		}

		// everything is correct, read the value directly
		{
			CGameStorageReader::ScopedDefault scopedDefaultParent( static_cast< CGameStorageReader* >( m_saveDataFile ), defaultParent );
			void* destData = prop->GetOffsetPtr( object );
			prop->GetType()->Serialize( *m_saveDataFile, destData );
			skipBlock.Skip();		// Seek to end of data in case anything went wrong in Serialize
		}
	}
}

IGameDataStorage* CGameFileLoaderNoSeeks::ExtractDataStorage()
{
	Uint16 magic = STORAGE_STREAM_TAG;
	Uint32 size = 0;

	CFileSkipableBlock skipBlock( *m_saveDataFile );
	{
		*m_saveDataFile << magic;
		if( magic != STORAGE_STREAM_TAG )
		{
			GAMESAVE_WARN( TXT( "Expected data storage, but no tag found" ) );
			skipBlock.Skip();
			return nullptr;
		}

		*m_saveDataFile << size;
		if( size != 0 )
		{
            // use default pool for "big" (root level) storages
            IGameDataStorage* newStorage = CGameDataStorage< MC_Gameplay, MemoryPool_Default >::Create( size, size );

			RED_FATAL_ASSERT( newStorage, "Failed to instantiate new data storage object" );
			RED_FATAL_ASSERT( newStorage->GetData(), "Failed to instantiate new data storage object buffer" );
			m_saveDataFile->Serialize( const_cast< void* >( newStorage->GetData() ), size );
			return newStorage;
		}
	}

	return nullptr;
}

void CGameFileLoaderNoSeeks::SkipDataStorage()
{
	CFileSkipableBlock skipBlock( *m_saveDataFile );
	skipBlock.Skip();
}
