/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "collisionCache.h"
#include "collisionCacheDataFormat.h"

/// This is a dynamic version of collision cache for use in editor, cooker, etc
/// It requires write access to the file

/// Cache for compiled collision meshes
class CCollisionCacheBuilder : public ICollisionCache
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_Physics, MC_CompiledCollision );

public:
	CCollisionCacheBuilder();
	virtual ~CCollisionCacheBuilder();

	// ICollisionCache interface
	virtual Bool Initialize( const String& absoluteFilePath ) override;
	virtual void InvalidateCollision( const String& name ) override;
	virtual EResult HasCollision( const String& name, Red::System::DateTime time ) const override;
	virtual EResult FindCompiled( CompiledCollisionPtr& result, const String& name, Red::System::DateTime time, Box2* bounds = nullptr ) override;
	virtual EResult Compile( CompiledCollisionPtr& result, const ICollisionContent* content, const String& name, Red::System::DateTime time, CObject* sourceObject = nullptr ) override;
	virtual void Flush() override;
	virtual class C2dArray* DumpStatistics(  Bool createIfDoesntExist = true ) override;

protected:
	struct RuntimeToken
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_Physics, MC_CompiledCollision );

	public:
		String						m_name;				//!< Index in the string table
		Uint32						m_dataOffset;		//!< Offset to compiled mesh data
		Uint32						m_dataSizeOnDisk;	//!< Size of the compressed data (in cache)
		Uint32						m_dataSizeInMemory;	//!< Size of the decompressed data (in memory)
		Uint64						m_diskCRC;			//!< CRC of the data on disk
		Red::System::DateTime		m_dateTime;			//!< Time stamp of the mesh
		CompiledCollisionWeakPtr	m_compiledMesh;		//!< Compiled collision mesh
		CompiledCollisionPtr		m_additionalRef;	//!< Are we keeping aditional reference to the data? (needed for fresh data before the cache is flushed)
		Box2						m_boundingArea;
		EColllisionTokenType		m_collisionType;

		RED_INLINE RuntimeToken()
			: m_dataOffset( 0 )
			, m_dataSizeOnDisk( 0 )
			, m_dataSizeInMemory( 0 )
			, m_boundingArea( Box2::ZERO )
			, m_collisionType( RTT_Unknown )
		{}

		RED_INLINE RuntimeToken( const CCollisionCacheData& data, const CCollisionCacheData::CacheToken& token )
			: m_dataOffset( token.m_dataOffset )
			, m_dataSizeOnDisk( token.m_dataSizeOnDisk )
			, m_dataSizeInMemory( token.m_dataSizeInMemory )
			, m_diskCRC( token.m_diskCRC )
			, m_dateTime( token.m_dateTime )
			, m_name( ANSI_TO_UNICODE( &data.m_strings[ token.m_name ] ) )
			, m_boundingArea( token.m_boundingArea )
			, m_collisionType( token.m_collisionType )
		{}
	};

	typedef THashMap< String, RuntimeToken, DefaultHashFunc< String >, DefaultEqualFunc< String >, MC_PhysicalCollision >		TCacheMap;
	typedef TDynArray< Uint8, MC_PhysicalCollision, MemoryPool_Physics > TInternalBuffer;

	TCacheMap								m_tokens;				//!< Mapped tokens by the source mesh GUID

	FILE*									m_file;					//!< Cache file (read/write)
	mutable Red::Threads::CMutex			m_lock;					//!< File access lock
	Bool									m_dirty;				//!< Have we added data to the cache ?

	TInternalBuffer							m_readBuffer;			//!< Internal read buffer - used for reading compressed data
	TInternalBuffer							m_loadBuffer;			//!< Internal load buffer - used for decompression and loading uncompressed data

	//! Flush cache with any unsaved meshes
	void FlushCache();

	//! Load data from token, returns pointer to loaded token data or NULL
	void* LoadTokenData( const RuntimeToken& token ); 
};
