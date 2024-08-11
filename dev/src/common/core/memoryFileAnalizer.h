
#pragma once

#include "file.h"
#include "dynarray.h"

// This class can be used only for debugging
class CMemoryFileSizeAnalizer : public IFile
{
protected:
	size_t			m_size;

private:
	CMemoryFileSizeAnalizer& operator=( const CMemoryFileSizeAnalizer& ) { return *this; };

public:
	CMemoryFileSizeAnalizer()
		: IFile( FF_Writer | FF_Mapper )
		, m_size( 0 )
	{
	}

	virtual void Serialize( void*, size_t size )
	{
		m_size += size;
	}

	virtual Uint64 GetOffset() const
	{
		return 0;
	}

	virtual Uint64 GetSize() const
	{
		return m_size;
	}

	virtual void Seek( Int64 )
	{
		
	}
};

// This class can be used only for debugging - extremely slow
class CFileHashGenerator : public IFile
{
protected:
	size_t			m_size;
	Uint64			m_hash;

private:
	CFileHashGenerator& operator=( const CFileHashGenerator& ) { return *this; };

public:
	CFileHashGenerator()
		: IFile( FF_Writer | FF_Mapper )
		, m_size( 0 )
		, m_hash( HASH64_BASE )
	{
		
	}

	virtual void Serialize( void* buffer, size_t size )
	{
		m_hash = ACalcBufferHash64Merge( buffer, size, m_hash );
		m_size += size;
	}

	virtual Uint64 GetOffset() const
	{
		return 0;
	}

	virtual Uint64 GetSize() const
	{
		return m_size;
	}

	virtual void Seek( Int64 )
	{

	}

	Uint64 GetHash() const
	{
		return m_hash;
	}
};

class CClass;

// This class can be used only for debugging
class CObjectMemoryAnalizer
{
public:
	static Uint32 CalcObjectSize( CObject* object );

	static Uint32 CalcObjectSize( CClass* objClass, void* data );

	static Uint32 CalcObjectsSize( const TDynArray< CObject* >& objects );
};

// This class can be used only for debugging - extremely slow
class CObjectHashGenerator
{
public:
	static Uint64 CalcObjectHash( CObject* object );

	static Uint64 CalcObjectsHash( const TDynArray< CObject* >& objects );
};

