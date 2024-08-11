/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "gameSaveFile.h"
#include "persistentEntity.h"
#include "idTag.h"
#include "../core/resource.h"
#include "node.h"
#include "gameSaveManager.h"

//#define DEBUG_DETECT_DUPLICATE_SKIP_BLOCKS
#define DEBUG_INVALID_REFERENCES
#define SAVE_FILE_END_TAG	Uint16( ( 'E' << 8 ) + 'S' )

/////////////////////////////////////////////////////////////////////////////////////////////

void CGameStorageWriter::SerializePointer( const class CClass* pointerClass, void*& pointer )
{
	if ( pointerClass->IsSerializable() )
	{
		ISerializable* obj = static_cast< ISerializable* >( pointer );

		if ( !obj ||
			obj->IsA< CResource >() ||		// Not saving resources
			obj->IsA< CNode >() )			// Not saving CNodes (to save CEntity one shall use EntityHandle)
		{
#ifdef RED_LOGGING_ENABLED
			if ( CNode* node = Cast< CNode >( obj ) )
			{
				RED_LOG_WARNING( RED_LOG_CHANNEL( GameSave ), TXT("Attempted to directly save reference to CNode '%ls'. To save reference to entity use EntityHandle."), node->GetFriendlyName().AsChar() );
			}
#endif

			Bool isNull = true;
			(*this) << isNull;
		}
		else
		{
			Bool isNull = false;
			(*this) << isNull;

			Uint32* index = m_objectsIndexMap.FindPtr( obj );
			if( !index )
			{
				Bool isObject = true;
				(*this) << isObject;

				Uint32 newIndex = m_objectsIndexMap.Size();
				(*this) << newIndex;

				m_objectsIndexMap.Insert( obj, newIndex );

				CName typeName = obj->GetClass()->GetName();
				TBaseClass::operator<<(typeName);
				obj->GetClass()->Serialize( *this, (void *)obj );
			}
			else
			{
				Bool isObject = false;
				(*this) << isObject;

				(*this) << *index;
			}
		}
	}
}

CGameStorageWriter::CGameStorageWriter( IFileEx* writer, CNamesRemapper& reporter, Uint32 extraFileFlags ) 
	: ISaveFile( writer->GetFlags() | FF_HashNames | FF_KeepCNameList | extraFileFlags, SAVE_VERSION, reporter )
	, m_writer( writer )
{
	ASSERT( writer->IsWriter() );
}

/////////////////////////////////////////////////////////////////////////////////////////////

void CGameStorageReader::SerializePointer( const class CClass* pointerClass, void*& pointer )
{
	pointer = NULL;

	Bool isNull = true;
	(*this) << isNull;

	if( !isNull )
	{
		Bool isObject = true;
		Uint32 index = m_indexObjectsMap.Size();

		if( GetVersion() >= VER_POINTER_WITH_MAP_SERIALIZATION )
		{
			(*this) << isObject;
			(*this) << index;
		}

		if( isObject )
		{
			CName typeName = CName::NONE;
			TBaseClass::operator<<(typeName);
			if( typeName != CName::NONE )
			{
				CClass * c = SRTTI::GetInstance().FindClass( typeName );
				if( c )
				{
					if ( GetVersion() < VER_REMOVED_PERSISTENT_ENTITY_REF_SAVING && c->IsA< CPeristentEntity >() )
					{
						IdTag idTag;
						(*this) << idTag;

						CObject* obj = CPeristentEntity::FindByIdTag( idTag );
						pointer = obj;
					}
					else
					{
						if( m_parents.Size() == 0 )
						{
							RED_HALT( "Parent stack underflow" );
						}

						if ( c->IsObject() )
						{
							CObject* parentObject = m_parents.Back();
							ASSERT( parentObject );

							CObject* obj = CreateObject< CObject >( c, parentObject );
							VERIFY( m_indexObjectsMap.Insert( index, (ISerializable*)obj ) );
							m_parents.PushBack( obj );
							obj->GetClass()->Serialize( *this, (void *)obj );

							m_parents.PopBack();

							pointer = obj;
						}
						else if ( c->IsSerializable() )
						{
							ISerializable* obj = c->CreateObject< ISerializable >();
							VERIFY( m_indexObjectsMap.Insert( index, (ISerializable*)obj ) );
							obj->GetClass()->Serialize( *this, (void *)obj );
							pointer = obj;
						}
					}
				}
			}
		}
		else
		{
			ISerializable** obj = m_indexObjectsMap.FindPtr( index );
			RED_FATAL_ASSERT( obj, "Object not found in the map" );

			pointer = *obj;
		}
	}
}

CGameStorageReader::CGameStorageReader( IFile* reader, CNamesRemapper& reporter, Uint32 additionalFlags ) 
	: ISaveFile( reader->GetFlags() | additionalFlags, SAVE_VERSION, reporter ) 
	, m_reader( reader )
{
	ASSERT( reader->IsReader() );
}

///////////////////////////////////////////////////////////////////

CGameSaveFileWriterNoInlineSkip::CGameSaveFileWriterNoInlineSkip( IFileEx* writer, CNamesRemapper& reporter, Uint32 skipBlockCacheStartSize )
	: CGameStorageWriter( writer, reporter, FF_NoInlineSkipBlocks )
{
	m_skipBlockCache.Reserve( skipBlockCacheStartSize );
}

CGameSaveFileWriterNoInlineSkip::~CGameSaveFileWriterNoInlineSkip()
{
}

void CGameSaveFileWriterNoInlineSkip::Close()
{
	// Skip-block data is written on close to the end of the file
	Uint32 skipBlockOffset = static_cast< Uint32 >( m_writer->GetOffset() );

	Uint32 skipBlockCount = m_skipBlockCache.Size();
	*m_writer << skipBlockCount;
	for( auto& currentBlock : m_skipBlockCache )
	{
		*m_writer << currentBlock.m_blockStartOffset;
		*m_writer << currentBlock.m_blockSkipOffset;
	}
	Uint16 magic = SAVE_FILE_END_TAG;
	*m_writer << skipBlockOffset;
	*m_writer << magic;

	CGameStorageWriter::Close();
}

// Skip-block data is cached internally. Is is the responsibility of the parent to serialise skip-block data
Uint32 CGameSaveFileWriterNoInlineSkip::QuerySkippableBlockOffset( Uint32 blockStartOffset )
{
	RED_FATAL_ASSERT( false, "Cannot query skip-blocks in writer" );
	return 0;
}

void CGameSaveFileWriterNoInlineSkip::RegisterSkippableBlockOffset( Uint32 blockStartOffset, Uint32 blockSkipOffset )
{
#ifdef DEBUG_DETECT_DUPLICATE_SKIP_BLOCKS
	for( const auto& currentBlock : m_skipBlockCache )
	{
		RED_FATAL_ASSERT( currentBlock.m_blockStartOffset != blockStartOffset, "Registering duplicate skip-block data" );
	}
#endif
	SkipBlockData newBlock;
	newBlock.m_blockStartOffset = blockStartOffset;
	newBlock.m_blockSkipOffset = blockSkipOffset;
	m_skipBlockCache.PushBack( newBlock );
}

///////////////////////////////////////////////////////////////////

CGameSaveFileReaderNoInlineSkip::CGameSaveFileReaderNoInlineSkip( IFile* reader, CNamesRemapper& reporter )
	: CGameStorageReader( reader, reporter, FF_NoInlineSkipBlocks )
{
	RED_FATAL_ASSERT( reader, "No reader" );

	// Read footer preamble to calculate where skip-block data is stored
	const Uint32 c_preambleSize = sizeof( Uint16 ) + sizeof( Uint32 );
	Uint32 fileEndOffset = static_cast< Uint32 >( reader->GetSize() );
	if( fileEndOffset <= c_preambleSize )
	{
		return;		// Bad file (too small to contain anything useful)
	}

	Uint32 initialOffset = static_cast< Uint32 >( reader->GetOffset() );
	reader->Seek( fileEndOffset - c_preambleSize );
	Uint32 blockDataOffset = 0;
	*reader << blockDataOffset;
	Uint16 magic = 0;
	*reader << magic;
	if( magic != SAVE_FILE_END_TAG )
	{
		RED_LOG( Save, TXT( "End-of-save tag not found. Save is incomplete or corrupt" ) );
		return;		// This is not a valid save-game!
	}
	if( blockDataOffset > fileEndOffset )
	{
		RED_LOG( Save, TXT( "Skip block offset is bad" ) );
		return;
	}

	reader->Seek( blockDataOffset );
	Uint32 skipBlockCount = 0;
	*reader << skipBlockCount;
	for( Uint32 i=0; i<skipBlockCount; ++i )
	{
		Uint32 startOffset = 0, endOffset = 0;
		*reader << startOffset;
		*reader << endOffset;
		m_skipBlockOffsetLookup.Insert( startOffset, endOffset );
	}
	reader->Seek( initialOffset );

	m_endMetadataOffset = blockDataOffset;
}

CGameSaveFileReaderNoInlineSkip::~CGameSaveFileReaderNoInlineSkip()
{
}

Uint32 CGameSaveFileReaderNoInlineSkip::QuerySkippableBlockOffset( Uint32 blockStartOffset )
{
	Uint32 foundOffset = 0;
	if( !m_skipBlockOffsetLookup.Find( blockStartOffset, foundOffset ) )
	{
		RED_LOG( Save, TXT( "Skip-block offset not found in cached data. Old save-game data" ) );
		return 0;
	}
	return foundOffset;		// Offset is relative to start
}

void CGameSaveFileReaderNoInlineSkip::RegisterSkippableBlockOffset( Uint32 blockStartOffset, Uint32 blockSkipOffset )
{
	RED_FATAL_ASSERT( false, "Skippable block should not be registering offsets on read" );
}