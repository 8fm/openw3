/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

/// Shared buffer container
class SharedDataBufferData : public Red::NonCopyable
{
	DECLARE_STRUCT_MEMORY_POOL( MemoryPool_SmallObjects, MC_SharedBuffer );

public:
	SharedDataBufferData( const void* sourceData, const Uint32 sourceSize, const Uint64 sourceDataHash );
	~SharedDataBufferData();

	// Get data buffer - read only
	RED_FORCE_INLINE const void* GetData() const { return m_data; }

	// Get size of the data
	RED_FORCE_INLINE const Uint32 GetSize() const { return m_size; }

	// Get data hash
	RED_FORCE_INLINE const Uint64 GetHash() const { return m_hash; }

private:
	void*				m_data;
	Uint32				m_size;
	Uint64				m_hash;
};

/// Shared data buffer cache
class SharedDataBufferCache
{
public:
	SharedDataBufferCache();
	~SharedDataBufferCache();

	/// Request buffer, thread safe
	SharedDataBufferData* Request( const void* sourceData, const Uint32 sourceDataSize );

	/// Copy handle
	SharedDataBufferData* Copy( SharedDataBufferData* data );

	/// Release reference to data buffer, thread safe
	void Release( SharedDataBufferData* data );

private:
	// Helper function: calculate hash of the data
	static Uint64 CalcDataHash( const void* sourceData, const Uint32 sourceSize );

	struct Token
	{
		SharedDataBufferData*		m_data;
		Uint32						m_numRefs;
	};

	// Data buffer cache
	typedef THashMap< Uint64, Token >		TCacheTokens;
	TCacheTokens				m_cache;

	typedef Red::Threads::CMutex						TCacheLock;
	typedef Red::Threads::CScopedLock< TCacheLock >		TCacheScopeLock;
	TCacheLock					m_lock;
};

typedef TSingleton< SharedDataBufferCache >			SSharedDataBufferCache;