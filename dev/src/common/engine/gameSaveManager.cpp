/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "gameSaveManager.h"
#include "compressedFile.h"
#include "../core/memoryFileReader.h"
#include "../engine/localizationManager.h"
#include "gameSaveFileNoSeeks.h"
#include "gameSession.h"
#include "game.h"
#include "gameDataStorage.h"

IMPLEMENT_RTTI_ENUM( ESaveGameType ) 
IMPLEMENT_ENGINE_CLASS( SSavegameInfo );

// Size of the skip-block cache reservation for CGameDataStorage - keep this small!
const Uint32 c_gameDataStorageSkipblockCacheSize = 1;

// Size of the skip-block cache reservation for save games. Adjust if the cache is being resized too often
const Uint32 c_gameSaveSkipblockCacheSize = 1024 * 32;

////////////////////////////////////////////////////////////////////////////////////////////////////////////
CGameStorageSaver::CGameStorageSaver( CGameStorageWriter* theFile )
	: m_file( theFile )
	, m_numBlocks( 0 )
{		
}

CGameStorageSaver::~CGameStorageSaver()
{
	delete m_file;
}


void CGameStorageSaver::BeginBlock( CName name )
{
	ASSERT( name );

	if ( m_numBlocks >= MAX_SAVE_BLOCKS )
	{
		HALT( "Too many blocks in the hierarchy. Consider increasing MAX_SAVE_BLOCKS" );
		RED_LOG( Save, TXT("************************ CRITICAL ERROR ************************ DEBUG NOW ************************") );
		return;
	}

	// Alloc block
	SaveBlock& block = m_blocks[ m_numBlocks++ ];
	block.m_name = name;

	// Save block code
	Uint32 magic = SAVE_BLOCK_MAGIC;
	*m_file << magic;

	// Save block name in the stream
	*m_file << block.m_name;

	// Remember the skip offset
	block.m_offset = static_cast< Uint32 >( m_file->GetOffset() );

	// Save (dummy) block skip offset
	Uint32 skipOffset = 0;
	*m_file << skipOffset;
}

void CGameStorageSaver::EndBlock( CName name )
{
	// Underflow
	if ( m_numBlocks == 0 )
	{
		HALT( "Out of blocks in save game EndBlock. Please debug." );
	}

	// Make sure we are closing the right block
	SaveBlock& block = m_blocks[ --m_numBlocks ];
	if ( block.m_name != name )
	{
		HALT( "Closing invalid block '%ls' when block '%ls' is still opened.", name.AsString().AsChar(), block.m_name.AsString().AsChar() );
	}

	// Save other block information
	Uint32 currentOffset = static_cast< Uint32 >( m_file->GetOffset() );
	Uint32 skipDataSize = ( currentOffset - block.m_offset ) - sizeof( Uint32 );
	m_file->Seek( block.m_offset );
	*m_file << skipDataSize;
	m_file->Seek( currentOffset );
}

void CGameStorageSaver::WriteValue( CName name, IRTTIType* type, const void* data )
{
	ASSERT( name );
	ASSERT( type );
	ASSERT( data );

	// Write header
	Uint32 magic = SAVE_VALUE_MAGIC;
	*m_file << magic;
	
	// Write value name
	*m_file << name;

	// Write value type name
	CName typeV = type->GetName();
	*m_file << typeV;
	
	// Write skip offset
	Uint32 skipOffset = static_cast< Uint32 >( m_file->GetOffset() );
	*m_file << skipOffset;

	// Write data
	type->Serialize( *m_file, (void*)data );

	// Finalize skip offset
	Uint32 currentOffset = static_cast< Uint32 >( m_file->GetOffset() );
	m_file->Seek( skipOffset );
	skipOffset = (currentOffset - skipOffset) - 4;
	*m_file << skipOffset;
	m_file->Seek( currentOffset );
}

void CGameStorageSaver::WriteRawAnsiValue( CName name, const AnsiChar* data, Uint32 size )
{
	// Write header
	Uint32 magic = SAVE_VALUE_MAGIC;
	*m_file << magic;

	// Write value name
	*m_file << name;

	// Write value type name
	CName typeV = CNAME( String );
	*m_file << typeV;

	// Write skip offset
	Uint32 skipOffset = static_cast< Uint32 >( m_file->GetOffset() );
	*m_file << skipOffset;

	// Write data
	Int32 byteCountNegative = -Int32( size ); 
	*m_file << CCompressedNumSerializer( byteCountNegative );
	m_file->Serialize( ( void* ) data, size * sizeof( AnsiChar ) );

	// Finalize skip offset
	Uint32 currentOffset = static_cast< Uint32 >( m_file->GetOffset() );
	m_file->Seek( skipOffset );
	skipOffset = (currentOffset - skipOffset) - 4;
	*m_file << skipOffset;
	m_file->Seek( currentOffset );
}

void CGameStorageSaver::AddStorageStream( IGameDataStorage* storageStream )
{
	// Write header
	Uint32 magic = SAVE_STORAGE_START_MAGIC;
	*m_file << magic;

	// Write size
	Uint32 size = storageStream ? storageStream->GetSize() : 0;
	*m_file << size;

	// Write data
	if ( size )
	{
		const void* data = storageStream->GetData();
		m_file->Serialize( (void*) data, size );
	}

	// End of stream marker
	Uint32 endMagic = SAVE_STORAGE_END_MAGIC;
	*m_file << endMagic;

	extern Bool GDebugSaves;
	if ( GDebugSaves && storageStream )
	{
		// check version
		ISaveFile *reader = storageStream->CreateReader();
		if ( reader )
		{
			Uint32 saveVersion = 0;
			Uint32 magic = 0;

			*reader << magic;
			if ( magic >= SAVE_GD_STORAGE_MAGIC )
			{
				// Load save version
				*reader << saveVersion;
				reader->m_saveVersion = saveVersion;
			}

			if ( saveVersion != SAVE_VERSION )
			{
				RED_LOG( Save, TXT("Incorrect save version (%ld, while current is %ld) of storage stream in CGameFileSaver::AddStorageStream()"), saveVersion, SAVE_VERSION );
			}

			delete reader;
		}
	}
}

void CGameStorageSaver::Close()
{
	// Close file
	m_file->Close();
}

const void* CGameStorageSaver::GetData() const
{
	return m_file->GetBuffer();
}

Uint32 CGameStorageSaver::GetDataSize() const
{
	return static_cast< Uint32 >( m_file->GetSize() );
}

Uint32 CGameStorageSaver::GetDataCapacity() const
{
	return static_cast< Uint32 >( m_file->GetBufferCapacity() );
}

void CGameStorageSaver::Finalize()
{
	// dump all cnames used in this game session for saving
	SGameSessionManager::GetInstance().GetCNamesRemapper().Save( m_file );

	// Add magic at the end of file
	Uint32 endMagicOffset = Uint32( m_file->GetOffset() );
	Uint32 magic = SAVE_END_FILE_MAGIC;
	*m_file << magic;

	// End magic offset is located as third Uint32
	m_file->Seek( 8 );
	*m_file << endMagicOffset;

	// Report any unclosed blocks
	if ( m_numBlocks > 0 )
	{
		HALT( "There are '%d' unclosed save blocks. Please debug.", m_numBlocks );
	}
}

void CGameStorageSaver::WriteProperty( void* object, CProperty* prop )
{
	ASSERT( object );
	ASSERT( prop );

	CName name = prop->GetName();
	IRTTIType* type = prop->GetType();
	void* data = prop->GetOffsetPtr( object );

	ASSERT( name );
	ASSERT( type );
	ASSERT( data );

	// Write header
	Uint32 magic = SAVE_PROPERTY_MAGIC;
	*m_file << magic;

	// Write property name
	*m_file << name;

	// Write property type name
	CName typeV = type->GetName();
	*m_file << typeV;

	// Write skip offset
	Uint32 skipOffset = static_cast< Uint32 >( m_file->GetOffset() );
	*m_file << skipOffset;

	// Write data
	type->Serialize( *m_file, data );

	// Finalize skip offset
	Uint32 currentOffset = static_cast< Uint32 >( m_file->GetOffset() );
	m_file->Seek( skipOffset );
	skipOffset = ( currentOffset - skipOffset ) - 4;
	*m_file << skipOffset;
	m_file->Seek( currentOffset );
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

CGameStorageLoader::CGameStorageLoader( CGameStorageReader* file, Uint32 saveVersion, Uint32 gameVersion, Uint32 fileVersion, Bool hasRootBlocks )
	: m_file( file )
	, m_saveVersion( saveVersion )
	, m_gameVersion( gameVersion )
	, m_fileVersion( fileVersion )
	, m_hasRootBlocks( hasRootBlocks )
{
	if ( saveVersion >= SAVE_CNAME_HASHES )
	{
		file->SetHashNames( true );
	}

	if ( saveVersion >= SAVE_VERSION_KEEP_CNAMES_AS_LIST )
	{
		file->SetKeepCNameList( true );
	}

	m_blocks.Reserve( MAX_SAVE_BLOCKS );

	Red::MemoryZero( m_rootBlocks, sizeof( m_rootBlocks ) );

	// Load root blocks info
	if ( m_hasRootBlocks )
	{
		if ( saveVersion < SAVE_VERSION_OPTIMIZED_BLOCKS_SAVING )
		{
			struct RootBlock
			{
				AnsiChar		m_name[ 32 ];		//!< Name of the block
				Uint32			m_blockOffset;		//!< Offset to block
			};

			RootBlock rootBlocksOLD[ 32 ];
			m_file->Serialize( rootBlocksOLD, sizeof( rootBlocksOLD ) );

			for ( Uint32 i = 0; i < MAX_ROOT_BLOCKS; ++i )
			{
				if ( rootBlocksOLD[ i ].m_name[ 0 ] == 0 )
				{
					break;
				}

				SaveBlock& newRootBlock = m_rootBlocks[ i ];
				newRootBlock.m_name = CName( ANSI_TO_UNICODE( rootBlocksOLD[ i ].m_name ) );
				newRootBlock.m_offset = rootBlocksOLD[ i ].m_blockOffset;
				newRootBlock.m_isValid = true;
			}
		}
		else
		{
			// Load root blocks
			for ( Uint32 i = 0; i < MAX_ROOT_BLOCKS; ++i )
			{
				*m_file << m_rootBlocks[ i ].m_name;
				*m_file << m_rootBlocks[ i ].m_offset;
			}
		}
	}
};

CGameStorageLoader::~CGameStorageLoader()
{
	// Close file
	delete m_file;
}

Uint32 CGameStorageLoader::GetGameVersion() const
{
	return m_gameVersion;
}

Uint32 CGameStorageLoader::GetSaveVersion() const
{
	return m_saveVersion;
}

void CGameStorageLoader::BeginBlock( CName blockName )
{
	// memorize current block stack's size
	Uint32 blocksStackSize = m_blocks.Size();

	// Initial offset
	Uint32 initialOffset = static_cast< Uint32 >( m_file->GetOffset() );

	// push the new block entry onto the stack
	SaveBlock* block = new ( m_blocks ) SaveBlock;
	block->m_name = blockName;
	block->m_offset = initialOffset;

	// Find the root block
	Bool rootBlockFound = true;
	if ( blocksStackSize == 0 && m_hasRootBlocks )
	{
		const SaveBlock* rootBlock = nullptr;
		for ( Uint32 i = 0; i < MAX_ROOT_BLOCKS; ++i )
		{
			if ( m_rootBlocks[ i ].m_name == blockName )
			{
				rootBlock = &m_rootBlocks[ i ];
				break;
			}
		}

		// Found
		rootBlockFound = rootBlock != nullptr;
		if ( rootBlockFound )
		{
			// Offset to the root block
			m_file->Seek( rootBlock->m_offset );
		}
	}

	if ( rootBlockFound )
	{
		// Read block magic
		Uint32 blockMagic = 0;
		if ( m_file->GetOffset() + sizeof( blockMagic ) <= m_file->GetSize() )
		{
			*m_file << blockMagic;
		}

		Bool blockReadResult = false;
		if ( blockMagic == SAVE_BLOCK_MAGIC )
		{
			// Get block name
			CName savedBlockName;

			if ( m_saveVersion >= SAVE_VERSION_USE_HASH_NAMES_WHERE_SAFE )
			{
				if ( m_saveVersion < SAVE_VERSION_KEEP_CNAMES_AS_LIST )
				{
					m_file->SerializeNameAsHash( savedBlockName );
				}
				else
				{
					*m_file << savedBlockName;
				}
			}
			else
			{
				*m_file << savedBlockName; // name will be saved as string (no mapping)
			}

			// Read skip offset
			Uint32 skipOffset = 0;
			*m_file << skipOffset;

			if ( savedBlockName == blockName )
			{
				block->m_offset = static_cast< Uint32 >( m_file->GetOffset() ) + skipOffset;
				blockReadResult = true;
			}
			else
			{
				WARN_ENGINE( TXT("savedBlockName '%ls' != blockName '%ls'"), savedBlockName.AsString().AsChar(), blockName.AsString().AsChar() );
			}
		}

		// something went wrong while reading the block - go back and fake it
		if ( !blockReadResult )
		{
			WARN_ENGINE( TXT("blockName '%ls' failed to load"), blockName.AsString().AsChar() );
			m_file->Seek( initialOffset );
		}

		block->m_isValid = blockReadResult;
	}
	else
	{
		block->m_isValid = false;
	}
}

void CGameStorageLoader::EndBlock( CName blockName )
{
	// Underflow
	if ( m_blocks.Empty() )
	{
		HALT( "Incorect save game stream. Out of blocks in save game EndBlock. Please debug.");
	}

	// Make sure we are closing the right block
	SaveBlock& block = m_blocks[ m_blocks.Size()-1 ];
	if ( block.m_name != blockName )
	{
		HALT( "Incorrect save game stream. Closing invalid block '%ls' when block '%ls' is still opened.", blockName.AsString().AsChar(), block.m_name.AsString().AsChar() );
	}

	// Move to block end
	m_file->Seek( block.m_offset );
	m_blocks.PopBackFast();
}

void CGameStorageLoader::ReadValue( CName name, IRTTIType* type, const void* data, CObject* defaultParent )
{
	ASSERT( name );
	ASSERT( type );
	ASSERT( data );

	// Create scoped default parent
	CGameStorageReader::ScopedDefault scopedDefaultParent( m_file, defaultParent );

	// Initial offset
	Uint32 initialOffset = static_cast< Uint32 >( m_file->GetOffset() );
	Uint32 magic = 0;

	// Read header
	*m_file << magic;

	// Versioning hack - if we are loading old saves (pre SAVE_SERIALIZED_OBJECTS_NEW_VERSION) 
	// we should use the old serialization code (fileVersion < VER_CLASS_PROPERTIES_DATA_CLEANUP)
	m_file->m_version = m_fileVersion;

	Bool readValueResult = false;
	if ( magic == SAVE_VALUE_MAGIC )
	{
		// Read value name
		CName valName;
		if ( m_saveVersion >= SAVE_VERSION_USE_HASH_NAMES_WHERE_SAFE )
		{
			if ( m_saveVersion < SAVE_VERSION_KEEP_CNAMES_AS_LIST )
			{
				m_file->SerializeNameAsHash( valName );
			}
			else
			{
				*m_file << valName;
			}
		}
		else
		{
			*m_file << valName; // name will be saved as string (no mapping)
		}

		// Read value type name
		CName valTypeName;
		if ( m_saveVersion >= SAVE_VERSION_USE_HASH_NAMES_WHERE_SAFE )
		{
			if ( m_saveVersion < SAVE_VERSION_KEEP_CNAMES_AS_LIST )
			{
				m_file->SerializeNameAsHash( valTypeName );
			}
			else
			{
				*m_file << valTypeName;
			}
		}
		else
		{
			*m_file << valTypeName; // name will be saved as string (no mapping)
		}

		// Read skip offset
		Uint32 skipOffset = 0;
		*m_file << skipOffset;
		Uint32 endOffset = static_cast< Uint32 >( m_file->GetOffset() ) + skipOffset;

		// Check type
		if ( type )
		{
			// old type name -> new type name conversion
			Bool typeValid = true;
			if ( valTypeName != type->GetName() )
			{
				IRTTIType* oldType = SRTTI::GetInstance().FindType( valTypeName );
				if ( !oldType || oldType != type )
				{
					// for arrays, just check the inner type 
					if ( oldType && type->GetType() == RT_Array && oldType->GetType() == RT_Array )
					{
						CRTTIArrayType* newArrayType = static_cast< CRTTIArrayType* > ( type );
						CRTTIArrayType* oldArrayType = static_cast< CRTTIArrayType* > ( oldType );
						if ( newArrayType->GetInnerType() != oldArrayType->GetInnerType() )
						{
							WARN_ENGINE( TXT("ReadValue(): valTypeNameStr '%ls' != typeGetNameStr '%ls' for value '%ls'"), valTypeName.AsChar(), type->GetName().AsChar(), valName.AsChar() );
							typeValid = false;
						}
					}
					else
					{
						WARN_ENGINE( TXT("ReadValue(): valTypeNameStr '%ls' != typeGetNameStr '%ls' for value '%ls'"), valTypeName.AsChar(), type->GetName().AsChar(), valName.AsChar() );
						typeValid = false;
					}
				}
			}

			// Check name
			Bool nameValid = true;
			if ( valName != name )
			{
				WARN_ENGINE( TXT("ReadValue(): valNameStr '%ls' != nameStr '%ls'"), valName.AsChar(), name.AsChar() );
				nameValid = false;
			}

			// Read data
			if ( typeValid && nameValid )
			{
				type->Serialize( *m_file, (void*)data );
				m_file->Seek( endOffset );
				readValueResult = true;
			}
		}
		else
		{
			WARN_ENGINE( TXT("ReadValue(): type == NULL, valTypeNameStr: '%ls' for value '%ls'"), valTypeName.AsChar(), valName.AsChar() );
		}
	}

	if ( !readValueResult )
	{
		// for some reason the value was not read
		m_file->Seek( initialOffset );
	}
}

//! Extract data storage block
IGameDataStorage* CGameStorageLoader::ExtractDataStorage()
{
	// Read header
	Uint32 magic = 0;
	*m_file << magic;
	if ( magic != SAVE_STORAGE_START_MAGIC )
	{
		HALT( "Incorect save game stream. Expected stream magic. Found 0x%08X. Plase debug.", magic );
		return NULL;
	}

	// Get stream size
	Uint32 size = 0;
	*m_file << size;

	// Create stream
	IGameDataStorage* storage = CGameDataStorage< MC_Gameplay, MemoryPool_GameSave >::Create( 0, size );

	// Load data from crap
	void* data = ( void* ) storage->GetData();
	m_file->Serialize( data, size );

	// End of block
	Uint32 endMagic = 0;
	*m_file << endMagic;
	if ( endMagic != SAVE_STORAGE_END_MAGIC )
	{
		HALT( "Incorect save game stream. Expected end of stream magic. Found 0x%08X. Plase debug.", endMagic );
		delete storage;
		return NULL;
	}

	// Return loaded data
	return storage;
}

void CGameStorageLoader::SkipDataStorage()
{
	// Read header
	Uint32 magic = 0;
	*m_file << magic;
	if ( magic != SAVE_STORAGE_START_MAGIC )
	{
		HALT( "Incorect save game stream. Expected stream magic. Found 0x%08X. Plase debug.", magic );
		return;
	}

	// Get stream size
	Uint32 size = 0;
	*m_file << size;

	m_file->Seek( m_file->GetOffset() + size );

	// End of block
	Uint32 endMagic = 0;
	*m_file << endMagic;
	if ( endMagic != SAVE_STORAGE_END_MAGIC )
	{
		HALT( "Incorect save game stream. Expected end of stream magic. Found 0x%08X. Plase debug.", endMagic );
	}
}

void CGameStorageLoader::ReadProperty( void* object, CClass* theClass, CObject* defaultParent )
{
	ASSERT( object );
	ASSERT( theClass );

	// Initial offset
	Uint32 initialOffset = static_cast< Uint32 >( m_file->GetOffset() );
	Uint32 magic = 0;

	// Read header
	*m_file << magic;

	if ( magic == SAVE_PROPERTY_MAGIC )
	{
		// Read property name
		CName name;
		if ( m_saveVersion < SAVE_VERSION_KEEP_CNAMES_AS_LIST )
		{
			m_file->SerializeNameAsHash( name );
		}
		else
		{
			*m_file << name;
		}

		// Read property type name
		CName typeName;
		if ( m_saveVersion < SAVE_VERSION_KEEP_CNAMES_AS_LIST )
		{
			m_file->SerializeNameAsHash( typeName );
		}
		else
		{
			*m_file << typeName;
		}

		// Read skip offset
		Uint32 skipOffset = 0;
		*m_file << skipOffset;
		Uint32 endOffset = static_cast< Uint32 >( m_file->GetOffset() ) + skipOffset;

		// Find property
		CProperty* prop = theClass->FindProperty( name );
		if ( prop )
		{
			// check type name
			if ( prop->GetType()->GetName() == typeName )
			{
				// everything is correct, read the value directly
				CGameStorageReader::ScopedDefault scopedDefaultParent( m_file, defaultParent );
				void* destData = prop->GetOffsetPtr( object );
				prop->GetType()->Serialize( *m_file, destData );
				m_file->Seek( endOffset );
			}
			else
			{
				WARN_ENGINE( TXT("ReadProperty(): wrong type name %s of property %s in class %s (should be: %s)"), typeName.AsChar(), name.AsChar(), theClass->GetName().AsChar(), prop->GetType()->GetName().AsChar() );
				m_file->Seek( endOffset );
			}
		}
		else
		{
			WARN_ENGINE( TXT("ReadProperty(): cannot find property %s of type %s in class %s"), name.AsChar(), typeName.AsChar(), theClass->GetName().AsChar() );
			m_file->Seek( endOffset );
		}
	}
	else
	{
		// !@@# WRONG MAGIC!
		WARN_ENGINE( TXT("ReadProperty(): wrong magic while trying to read property of class %s"), theClass->GetName().AsChar() );
		m_file->Seek( initialOffset );
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

IGameSaveSection::IGameSaveSection()
{
}

IGameSaveSection::~IGameSaveSection()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
CGameSaveManager::CGameSaveManager()
{
}

void CGameSaveManager::DumpContent( IGameDataStorage* data, IGameSaveDumper* dumper )
{
	// Open file
	if ( !data )
		return;

	ISaveFile* file = data->CreateReader();
	if ( !file )
		return;

	// Load magic
	Uint32 magic = 0;
	*file << magic;
	if ( magic != SAVE_GD_STORAGE_MAGIC )
		return;

	// Load save version
	Uint32 saveVersion = 0;
	*file << saveVersion;
	file->m_saveVersion = saveVersion;

	// Load game version
	Uint32 gameVersion = 0;
	*file << gameVersion;
	if ( saveVersion < SAVE_VERSION_CNAMES || gameVersion == 0 )
		return;

	// Load file serialization version, old save games are stored in the pre-serialization cleanup version
	Uint32 serialzationVersion = VER_SERIALIZATION_DATA_CLEANUP - 1;
	if ( saveVersion >= SAVE_SERIALIZED_OBJECTS_NEW_VERSION )
	{
		// serialization version is stored in the file
		*file << serialzationVersion;
	}

	// Restore file serialization version
	file->m_version = serialzationVersion;

	// block stack
	struct BlockInfo
	{
		Uint32		m_startOffset;
		Uint32		m_endOffset;
		CName		m_blockName;
	};

	TDynArray< BlockInfo > blockStack;

	// start parsing data
	Bool isOK = true;
	const Uint64 totalSize = file->GetSize();
	while ( ( (file->GetOffset()+4) <= totalSize) && isOK )
	{
		// current offset
		const Uint32 currentOffset = (Uint32)file->GetOffset();

		// unwind blocks from stack
		while ( !blockStack.Empty() && (currentOffset >= blockStack.Back().m_endOffset) )
		{
			dumper->OnBlockEnd( (Uint32) file->GetOffset() );
			blockStack.PopBack();
		}

		// read section magic
		Uint32 magic = 0;
		*file << magic;

		// parse data
		switch ( magic )
		{
			case SAVE_BLOCK_MAGIC:
			{
				CName blockName;
				*file << blockName;

				Uint32 skipOffset = 0;
				*file << skipOffset;

				const Uint32 initialOffset = (Uint32)file->GetOffset();
				dumper->OnBlockStart( currentOffset, skipOffset, blockName );

				// add block to the block stack
				BlockInfo info;
				info.m_startOffset = initialOffset;
				info.m_endOffset = initialOffset + skipOffset;
				info.m_blockName = blockName;
				blockStack.PushBack(info);

				break;
			}

			case SAVE_PROPERTY_MAGIC:
			case SAVE_VALUE_MAGIC:
			{
				CName valueName;
				*file << valueName;

				// Write value type name
				CName typeName;
				*file << typeName;

				// Write skip offset
				Uint32 skipOffset = 0;
				*file << skipOffset;

				// Load data
				const Uint32 initialOffset = (Uint32) file->GetOffset();
				const IRTTIType* type = SRTTI::GetInstance().FindType( typeName );
				if ( type )
				{
					TDynArray< Uint8 > tempBuffer;
					tempBuffer.Resize( type->GetSize() );

					type->Construct( tempBuffer.Data() );
					type->Serialize( *file, tempBuffer.Data() );

					const Bool isProp = (magic == SAVE_PROPERTY_MAGIC);
					dumper->OnValue( currentOffset, isProp, valueName, type, skipOffset, tempBuffer.Data() );

					type->Destruct( tempBuffer.Data() );
				}

				// skip over data
				file->Seek( initialOffset + skipOffset );
				break;
			}

			case SAVE_STORAGE_START_MAGIC:
			{
				Uint32 size = 0;
				*file << size;

				const Uint32 initialOffset = (Uint32)file->GetOffset();
				dumper->OnStorageStart( initialOffset, size );

				// go to the end of the stream
				file->Seek( initialOffset + size );
				dumper->OnStorageEnd( (Uint32)file->GetOffset() );

				// End of stream marker
				Uint32 endMagic = 0;
				*file << endMagic;

				// Error ?
				if ( endMagic != SAVE_STORAGE_END_MAGIC )
				{
					dumper->OnError( currentOffset, "Unexpected magic at the end of storage stream: 0x%08X", endMagic );
					isOK = false;
				}
				break;
			}

			case SAVE_END_FILE_MAGIC:
			{
				// end of file
				isOK = false;
				break;
			}

			default:
			{
				dumper->OnError( currentOffset, "Unexpected magic: 0x%08X", magic );
				isOK = false;
				break;
			}
		}
	}
}

IGameLoader* CGameSaveManager::CreateLegacyLoader( const SSavegameInfo& info, IFile* sourceFile, Uint32 saveVersion, ELoaderCreationResult& res )
{
	// Create wrapper
	CGameStorageReader* file = new CGameStorageReader( sourceFile, SGameSessionManager::GetInstance().GetCNamesRemapper() );
	file->m_saveVersion = saveVersion;

	Uint32 endMagicOffset = 0;
	if( saveVersion >= SAVE_VERSION_PREALIGNED_CONVERTED )
	{
		Uint32 oldOffset = static_cast< Uint32 >( file->GetOffset() );

		Uint32 carret = static_cast< Uint32 >( file->GetSize() ) - 1;

		// Go to last byte
		file->Seek( carret );

		Uint8 byte;
		*file << byte;

		// Search for first non-zero byte
		while( byte == 0 )
		{
			file->Seek( --carret );
			*file << byte;
		}

		// Go back 3 more bytes to read our end magic
		carret -= 3;

		endMagicOffset = carret;

		file->Seek( oldOffset );

		// While conversion old version is incremented by SAVE_VERSION_PREALIGNED_CONVERTED value,
		// so after decrementing we get the save version before conversion.
		saveVersion -= SAVE_VERSION_PREALIGNED_CONVERTED;
	}
	else if( saveVersion >= SAVE_VERSION_ALIGNED )
	{
		*file << endMagicOffset;
	}
	else
	{
		endMagicOffset = static_cast< Uint32 >( file->GetSize() ) - 4;
	}

	// Load game version
	Uint32 gameVersion = 0;
	*file << gameVersion;

	// Load file serialization version, old save games are stored in the pre-serialization cleanup version
	Uint32 serializationVersion = VER_SERIALIZATION_DATA_CLEANUP - 1;
	if ( saveVersion >= SAVE_SERIALIZED_OBJECTS_NEW_VERSION )
	{
		// serialization version is stored in the file
		*file << serializationVersion;
	}

	// Restore file serialization version
	file->m_version = serializationVersion;

	if ( false == VerifySaveVersion( gameVersion, saveVersion, serializationVersion ) )
	{
		delete file;
		res = LOADER_WrongVersion;
		return nullptr;
	}

	if( saveVersion >= SAVE_VERSION_MAGIC_AT_END )
	{
		// Save offset
		Uint32 fileOffset = static_cast< Uint32 >( file->GetOffset() );

		// Read magic at the end of file
		file->Seek( endMagicOffset );
		Uint32 magic;
		*file << magic;

		if( magic != SAVE_END_FILE_MAGIC )
		{
			RED_LOG( Save, TXT( "Invalid save file '%ls'" ), info.GetFileName().AsChar() );
			delete file;
			return NULL;
		}

		if ( saveVersion >= SAVE_VERSION_KEEP_CNAMES_AS_LIST )
		{
			// Read used cnames
			file->Seek( endMagicOffset - 8 );

			Uint32 offset;
			*file << offset;
			*file << magic;

			if ( magic == CNamesRemapper::NC_MAGIC_END && offset > 0 && offset < endMagicOffset )
			{
				file->Seek( offset );
				SGameSessionManager::GetInstance().GetCNamesRemapper().Load( file );
			}
		}

		// Go back
		file->Seek( fileOffset );
	}

	res = LOADER_Success;
	return new CGameStorageLoader( file, saveVersion, gameVersion, serializationVersion, true );
}

IGameLoader* CGameSaveManager::CreateNewLoader( const SSavegameInfo& info, IFile* sourceFile, Uint32 saveVersion, ELoaderCreationResult& res )
{
	// Wrap it in the new save loader, so the skip-block cache is read properly
	CGameSaveFileReaderNoInlineSkip* wrappedFile = new CGameSaveFileReaderNoInlineSkip( sourceFile, SGameSessionManager::GetInstance().GetCNamesRemapper() );

	Uint32 gameVersion = 0;
	*sourceFile << gameVersion;
	Uint32 serializationVersion = 0;
	*sourceFile << serializationVersion;

	if ( false == VerifySaveVersion( gameVersion, saveVersion, serializationVersion ) )
	{
		delete wrappedFile;
		res = LOADER_WrongVersion;
		return nullptr;
	}

	// Finally, create a FileLoader around the result
	CGameFileLoaderNoSeeks* fileLoader = new CGameFileLoaderNoSeeks( wrappedFile, gameVersion, saveVersion );
	res = LOADER_Success;
	return fileLoader;
}

Bool CGameSaveManager::VerifySaveVersion( Uint32 gameVersion, Uint32 saveVersion, Uint32 serializationVersion )	const
{
	if ( gameVersion > SAVE_GAME_VERSION || saveVersion > SAVE_VERSION || serializationVersion > VER_CURRENT )
	{
		RED_LOG( Save, TXT("Can't create loader, mismatched version.") );
		RED_LOG( Save, TXT("GAME version: current=%d, encoutered=%d"), SAVE_GAME_VERSION, gameVersion );
		RED_LOG( Save, TXT("SAVE version: current=%d, encoutered=%d"), SAVE_VERSION, saveVersion );
		RED_LOG( Save, TXT("SERIALIZATION version: current=%d, encoutered=%d"), VER_CURRENT, serializationVersion );
		return false;
	}

	return true;
}

IGameLoader* CGameSaveManager::CreateLoader( const SSavegameInfo& info, ELoaderCreationResult& res )
{
	SGameSessionManager::GetInstance().GetCNamesRemapper().Reset();

	IFile* saveFile = GUserProfileManager->CreateSaveFileReader();
	if ( !saveFile )
	{
		RED_LOG( GameSave, TXT("Unable to open file '%ls' for reading"), info.GetFileName().AsChar() );
		return NULL;
	}

	Uint32 magic = 0;
	*saveFile << magic;
	Uint32 saveVersion = 0;
	*saveFile << saveVersion;
	if ( magic != SAVE_FILE_MAGIC  && magic != SAVE_FILE_MAGIC_PATCHED )
	{
		RED_LOG( GameSave, TXT("File '%ls' is not save file"), info.GetFileName().AsChar() );
		delete saveFile;
		return NULL;
	}

	if( saveVersion < SAVE_VERSION_STORE_SKIPBLOCKS_AT_END )
	{
		return CreateLegacyLoader( info, saveFile, saveVersion, res );	
	}
	else
	{
		return CreateNewLoader( info, saveFile, saveVersion, res );
	}
}

IGameLoader* CGameSaveManager::CreateDebugLoader( IFile* saveFile )
{
	SGameSessionManager::GetInstance().GetCNamesRemapper().Reset();

	if ( !saveFile )
	{
		RED_LOG( GameSave, TXT("Unable to open debug data for reading") );
		return NULL;
	}

	Uint32 magic = 0;
	*saveFile << magic;
	Uint32 saveVersion = 0;
	*saveFile << saveVersion;
	if ( magic != SAVE_FILE_MAGIC && magic != SAVE_FILE_MAGIC_PATCHED )
	{
		RED_LOG( GameSave, TXT("Debug data is not save file") );
		delete saveFile;
		return NULL;
	}

	// Wrap it in the new save loader, so the skip-block cache is read properly
	CGameSaveFileReaderNoInlineSkip* wrappedFile = new CGameSaveFileReaderNoInlineSkip( saveFile, SGameSessionManager::GetInstance().GetCNamesRemapper() );

	Uint32 gameVersion = 0;
	*saveFile << gameVersion;
	Uint32 serializationVersion = 0;
	*saveFile << serializationVersion;

	// Finally, create a FileLoader around the result
	CGameFileLoaderNoSeeks* fileLoader = new CGameFileLoaderNoSeeks( wrappedFile, gameVersion, saveVersion );
	return fileLoader;
}

IGameLoader* CGameSaveManager::CreateLoader( IGameDataStorage* gameDataStorage, ISaveFile** directStreamAccess, Uint32* version )
{
	// We need source stream
	if ( gameDataStorage )
	{
		// Open file
		ISaveFile* file = gameDataStorage->CreateReader();
		if ( file )
		{
			Uint32 offset = static_cast< Uint32 >( file->GetOffset() );
			Bool useVersion2( false );
			Uint32 saveVersion = 0;
			Uint32 gameVersion = 0;

			// Load magic
			Uint32 magic = 0;
			*file << magic;

			if ( magic == SAVE_GD_STORAGE_MAGIC )
			{
				// Load save version
				*file << saveVersion;
				file->m_saveVersion = saveVersion;

				// Load game version
				*file << gameVersion;

				if ( saveVersion < SAVE_VERSION_CNAMES || gameVersion == 0 )
				{
					useVersion2 = true;
				}
			}
			else
			{
				useVersion2 = true;
			}

			if ( useVersion2 )
			{
				file->Seek( offset );
				saveVersion = 2;
				gameVersion = 1;
			}

			// Load file serialization version, old save games are stored in the pre-serialization cleanup version
			Uint32 serialzationVersion = VER_SERIALIZATION_DATA_CLEANUP - 1;
			if ( saveVersion >= SAVE_SERIALIZED_OBJECTS_NEW_VERSION )
			{
				// serialization version is stored in the file
				*file << serialzationVersion;
			}

			// Restore file serialization version
			file->m_version = serialzationVersion;

			if ( directStreamAccess )
			{
				*directStreamAccess = file;
			}

			if ( version )
			{
				*version = saveVersion;
			}

			// Create wrapper
			return new CGameStorageLoader( static_cast< CGameStorageReader* >( file ), saveVersion, gameVersion, serialzationVersion, false );
		}
	}

	// Not opened
	return NULL;
}

IGameSaver* CGameSaveManager::CreateNewSaver( const SSavegameInfo& info )
{
	IFileEx* saveFile = GUserProfileManager->CreateSaveFileWriter();
	if ( nullptr == saveFile )
	{
		RED_LOG( Save, TXT("Unable to open file '%ls' for writing"), info.GetFileName().AsChar() );
		return nullptr;
	}

	// Wrap the system file in a save-game specific file that caches skip blocks
	CGameSaveFileWriterNoInlineSkip* wrappedFile = new CGameSaveFileWriterNoInlineSkip( saveFile, SGameSessionManager::GetInstance().GetCNamesRemapper(), c_gameSaveSkipblockCacheSize );

	// Header
	Uint32 magic = SAVE_FILE_MAGIC_PATCHED;
	*wrappedFile << magic;
	Uint32 version = SAVE_VERSION;
	*wrappedFile << version;
	Uint32 gameVersion = SAVE_GAME_VERSION;
	*wrappedFile << gameVersion;
	Uint32 serializationVersion = VER_CURRENT;
	*wrappedFile << serializationVersion;

	// Wrap it with the new saver
	return new CGameFileSaverNoSeeks( wrappedFile );
}

IGameSaver* CGameSaveManager::CreateSaver( const SSavegameInfo& info )
{
	return CreateNewSaver( info );
}

IGameSaver* CGameSaveManager::CreateSaver( IGameDataStorage* gameDataStorage, ISaveFile** directStreamAccess )
{
	// We need source stream
	if ( gameDataStorage )
	{
		// Open file
		ISaveFile* file = gameDataStorage->CreateWriter();
		if ( file )
		{
			// save magic
			Uint32 saveMagic = SAVE_GD_STORAGE_MAGIC;
			*file << saveMagic;

			// save version
			Uint32 saveVersion = SAVE_VERSION;
			*file << saveVersion;

			// save game version
			Uint32 gameVersion = SAVE_GAME_VERSION;
			*file << gameVersion;

			// File data version (we need to store this because we are serializing object data using CClass::Serialize)
			Uint32 serializationVersion = VER_CURRENT;
			*file << serializationVersion;

			if ( directStreamAccess )
			{
				*directStreamAccess = file;
			}

			return new CGameStorageSaver( static_cast< CGameStorageWriter* >( file ) );
		}
	}

	// Not opened
	return NULL;
}

CGameStorageWriter* CGameDataExternalStorage::CreateWriter()
{
	m_data->ClearFast();
	return new CGameStorageWriter( new CMemoryFileWriter( *m_data ), SGameSessionManager::GetInstance().GetCNamesRemapper() );
}

//! Create memory reader
ISaveFile* CGameDataExternalStorage::CreateReader() const
{
	return new CGameStorageReader( new CMemoryFileReader( *m_data, 0 ), SGameSessionManager::GetInstance().GetCNamesRemapper() );
}	

String SSavegameInfo::GetDisplayName() const 
{ 
	if ( !m_customFilename && m_displayNameIndex > 0 )
	{
		String str = SLocalizationManager::GetInstance().GetString( m_displayNameIndex );
		if ( str.Empty() )
		{
			#ifdef RED_PLATFORM_ORBIS
				str = SLocalizationManager::GetInstance().GetStringByStringKey( TXT("error_message_damaged_save_unavailable_ps4") );
			#else
				str = SLocalizationManager::GetInstance().GetStringByStringKey( TXT("error_message_damaged_save_unavailable") );
			#endif
		}
		return str;
	}
	
	return m_filename;
}

Bool SSavegameInfo::IsDisplayNameAvailable() const
{
	if ( m_customFilename )
	{
		return true;
	}

	if ( SLocalizationManager::GetInstance().GetString( m_displayNameIndex ).Empty() )
	{
		return false;
	}

	return true;
}
