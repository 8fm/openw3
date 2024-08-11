#pragma once

#include "shaderCacheData.h"

//////////////////////////////////////////////////////////////////////////

#pragma pack(push)
#pragma pack(1)
struct StaticShaderCacheFileInfo
{
	Uint32 m_numEntries;
	Uint64 m_includesCRC;
	Uint64 m_shadersCRC;
	Uint32 m_magic;
	Uint32 m_version;

	void Serialize( IFile& file )
	{
		file << m_numEntries;
		file << m_includesCRC;
		file << m_shadersCRC;
		file << m_magic;
		file << m_version;
	}
};
#pragma pack(pop)

static_assert( sizeof( StaticShaderCacheFileInfo ) == ( 2 * sizeof( Uint64 ) + 3 * sizeof( Uint32 ) ), "Invalid StaticShaderCacheFileInfo struct size" );

//////////////////////////////////////////////////////////////////////////

// static shader cache is always loaded synchronously
class IStaticShaderCache
{
public:
	virtual ~IStaticShaderCache() {}

public:
	RED_INLINE const String& GetPath() const { return m_absoluteFilePath; }

public:
	virtual Bool ShadersMatch() const { return true; }
	virtual void InvalidateStaticShaders() {  }

public:
	virtual Bool GetShader( const Uint64 hash, StaticShaderEntry*& entry ) = 0;
	virtual Bool AddShader( const Uint64 hash, StaticShaderEntry* entry ) = 0;

	//! Mark as dirty when data has been changed externally
	virtual void ForceDirty( ) { }
	
	//! Flush dirty cache content to file, this only works for a cache that is not read only and has a file
	virtual void Flush() = 0;

	virtual void SetAbsolutePath( const String& absoluteFilePath ) { }

	virtual Uint64 GetCRC() const = 0;
	virtual void GetEntries( TDynArray< StaticShaderEntry* >& entries ) const = 0;

public:
	// Create read only collision cache
	static IStaticShaderCache* CreateReadOnly( const String& absolutePath );

	// Create runtime read/write collision cache
	static IStaticShaderCache* CreateReadWrite( const String& absolutePath );

	// Create NULL collision cache - will silently fail all requests
	static IStaticShaderCache* CreateNull();

protected:
	//! Initialize cache at given file, can return false if specific file cannot be opened
	virtual Bool Initialize( const String& absoluteFilePath ) = 0;

protected:
	String m_absoluteFilePath;
};

//////////////////////////////////////////////////////////////////////////

class CStaticShaderCacheReadWrite : public IStaticShaderCache
{
public:
	CStaticShaderCacheReadWrite();
	virtual ~CStaticShaderCacheReadWrite();

public:
	virtual Bool ShadersMatch() const override { return m_shadersMatch; }
	virtual void InvalidateStaticShaders() override;

	virtual Bool GetShader( const Uint64 hash, StaticShaderEntry*& entry ) override;
	virtual Bool AddShader( const Uint64 hash, StaticShaderEntry* entry ) override;
	virtual void Flush() override;
	virtual Bool Initialize( const String& absoluteFilePath ) override;
	RED_FORCE_INLINE virtual void ForceDirty( ) override { m_dirty = true; }
	RED_FORCE_INLINE virtual void SetAbsolutePath( const String& absoluteFilePath ) override { m_absoluteFilePath = absoluteFilePath; }

	virtual Uint64 GetCRC() const override { return m_info.m_shadersCRC; }
	virtual void GetEntries( TDynArray< StaticShaderEntry* >& entries ) const;

private:
	Bool ValidateCache_NoLock();

private:
	THashMap< Uint64, StaticShaderEntry* >	m_entries;
	StaticShaderCacheFileInfo				m_info;
	Bool									m_dirty;

	Bool									m_includesMatch;
	Bool									m_shadersMatch;

private:
	typedef Red::Threads::CMutex CMutex;
	typedef Red::Threads::CScopedLock< CMutex > CScopedLock;

private:
	mutable CMutex							m_mutex;

};

//////////////////////////////////////////////////////////////////////////

class CStaticShaderCacheReadonly : public IStaticShaderCache
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_RenderData );

public:
	CStaticShaderCacheReadonly();
	virtual ~CStaticShaderCacheReadonly();

	//! Methods below can return eResult_Pending if the data is not yet available
	virtual Bool GetShader( const Uint64 hash, StaticShaderEntry*& entry ) override;

	//! We cannot add entries to readonly cache
	virtual Bool AddShader( const Uint64 hash, StaticShaderEntry* entry ) override;

	//! Flush dirty cache content to file, this only works for a cache that is not read only and has a file
	virtual void Flush() override;

	virtual Bool Initialize( const String& absoluteFilePath ) override;

	virtual Uint64 GetCRC() const override { return m_info.m_shadersCRC; }
	virtual void GetEntries( TDynArray< StaticShaderEntry* >& entries ) const;

private:
	THashMap< Uint64, StaticShaderEntry* >	m_entries;
	StaticShaderCacheFileInfo				m_info;
};

//////////////////////////////////////////////////////////////////////////

class CStaticShaderCacheNull : public IStaticShaderCache
{
public:
	virtual ~CStaticShaderCacheNull()
	{}

	virtual Bool GetShader( const Uint64 hash, StaticShaderEntry*& entry )
	{
		return false;
	}

	virtual Bool AddShader( const Uint64 hash, StaticShaderEntry* entry )
	{
		return false;
	}

	virtual void Flush()
	{
	}

	virtual Bool Initialize( const String& absoluteFilePath )
	{
		return true;
	}

	virtual Uint64 GetCRC() const override { return 0; }
	virtual void GetEntries( TDynArray< StaticShaderEntry* >& entries ) const { }
};
