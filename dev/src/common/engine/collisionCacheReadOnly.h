/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../redIO/redIOAsyncIO.h"
#include "../redIO/redIOAsyncFileHandleCache.h"
#include "../redIO/redIOAsyncReadToken.h"

#include "collisionCache.h"
#include "collisionCacheDataFormat.h"
#include "collisionCachePreloadedData.h"

/// This is a read only version of collision cache for use in cooked game
/// It only requires read access to the file
/// It's using AsyncIO to read the data
/// Only accessible from one thread at a time

/// Cache for compiled collision meshes
class CCollisionCacheReadOnly : public ICollisionCache
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_Physics, MC_CompiledCollision );

public:
	CCollisionCacheReadOnly();
	virtual ~CCollisionCacheReadOnly();

	// ICollisionCache interface
	virtual Bool Initialize( const String& absoluteFilePath ) override;
	virtual void InvalidateCollision( const String& ) override { }
	virtual EResult HasCollision( const String& name, Red::System::DateTime time ) const override;
	virtual EResult FindCompiled( CompiledCollisionPtr& result, const String& name, Red::System::DateTime time, Box2* bounds = nullptr ) override;
	virtual EResult Compile( CompiledCollisionPtr& result, const ICollisionContent* content, const String& name, Red::System::DateTime time, CObject* sourceObject = nullptr ) override;
	virtual void Flush() override { }
	virtual class C2dArray* DumpStatistics( Bool createIfDoesntExist = true ) override { return nullptr; }

	// Direct debug interface
	Uint32 GetNumEntries_Debug();
	String GetEntryPath_Debug( const Uint32 index );
	EColllisionTokenType GetEntryType_Debug( const Uint32 index );

	// Process kickstart list (prefetching)
	void KickstartToken( const Uint32 tokenIndex );

private:
	struct RuntimeData
	{
	public:
		CompiledCollisionWeakPtr	m_compiledMesh;			//!< Loaded collision mesh
		volatile Bool				m_hasFailed;			//!< Did loading of this entry failed ?

		RED_INLINE RuntimeData()
			: m_hasFailed( false )
		{}
	};


	typedef Red::IO::CAsyncFileHandleCache::TFileHandle	FileHandle;
	typedef Uint32 FileKey;

	typedef THashMap< FileKey, Uint32, DefaultHashFunc< FileKey >, DefaultEqualFunc< FileKey >, MC_PhysicalCollision >						TCacheMap;
	typedef TDynArray< RuntimeData, MC_PhysicalCollision, MemoryPool_Physics >									TCacheRuntimeData;
	typedef THashSet< Uint32, DefaultHashFunc< Uint32 >, DefaultEqualFunc< Uint32 >, MC_PhysicalCollision >		TKickstartSet;
	typedef THashSet< CompiledCollisionPtr > 																	TCollisionReadyContainer;

	// hash map key
	static FileKey CalcStringHash( const String& str );
	static FileKey CalcStringHash( const StringAnsi& str );
	static FileKey CalcStringHash( const AnsiChar* str );

	// prepare collision cache for access (first time init after successful loading)
	Bool PrepareAccess();

	// request data for given token ID
	EResult FindCompiled( CompiledCollisionPtr* result, const Uint32 tokenInex, Box2* bounds = nullptr );

	// kick start more work for collision cache
	static void OnKickstartMoreWork();

	// add/remove stuff to kick start list
	void AddToKickStartQueue( const Uint32 tokenIndex );
	void RemoveFromKickStartQueue( const Uint32 tokenIndex );

	// collision got ready
	void OnCollisionReady( Uint32 token, CompiledCollisionPtr collision );

	CCollisionCacheData						m_staticData;			//!< Static data (loaded from file)
	TCacheRuntimeData						m_runtimeData;			//!< Runtime data, per object
	CCollisionCachePreloadedData			m_preloadedData;		//!< Preloaded data block
	TCacheMap								m_map;					//!< Mapped tokens by the source mesh file name (hashed)
	TCollisionReadyContainer				m_collisionReadyContainer;

	FileHandle								m_file;					//!< Cache file (read/write)
	volatile Bool							m_isReady;				//!< Are we ready to service requests
	volatile Bool							m_isLoaded;				//!< Have we finished loading
	volatile Bool							m_isFailed;				//!< The collision cache is corrupted
	CTimeCounter							m_loadingTimer;			//!< Internal loading timer

	BitSet64Dynamic							m_kickstartSet;			//!< Set of tokens in the kickstart list (so we are not adding stuff twice)

	CCollisionCacheDataAsyncLoader			m_mounter;				//!< Initial async loader
	class CCollsionCacheAsyncReader*		m_reader;				//!< Shared reading interface
};
