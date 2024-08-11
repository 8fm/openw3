
#pragma once

#include "animCacheEntry.h"

#include "../core/fileLatentLoadingToken.h"

#define ANIM_CACHE_MAGIC_HEADER		'ACMH'
#define ANIM_CACHE_MAGIC_HEADER_BS	'HMCA'
#define ANIM_CACHE_MAGIC_FOOTER		'ACMF'
#define ANIM_CACHE_MAGIC_SEP		'ACMS'

//////////////////////////////////////////////////////////////////////////

/// Anim cache file wrapper
class CAnimCacheFileWrapper : public IFile
{
protected:
	IFile*		m_file;
	Uint64		m_baseOffset;

public:
	CAnimCacheFileWrapper( IFile* source, Uint64 offset )
		: IFile( FF_Reader | FF_Buffered | FF_FileBased )
		, m_file( source )
		, m_baseOffset( offset )
	{
		// Seek to the beginning
		m_file->Seek( m_baseOffset );
	};

	// Serialize data buffer of given size
	virtual void Serialize( void* buffer, size_t size )
	{
		return m_file->Serialize( buffer, size );
	}

	// Get position in file stream
	virtual Uint64 GetOffset() const
	{
		return m_file->GetOffset() - m_baseOffset;
	}

	// Get size of the file stream
	virtual Uint64 GetSize() const
	{
		return m_file->GetSize();
	}

	// Seek to file position
	virtual void Seek( Int64 offset )
	{
		m_file->Seek( offset + m_baseOffset );
	}
};

//////////////////////////////////////////////////////////////////////////

class CAnimCacheLoadingToken : public IFileLatentLoadingToken
{
protected:
	IFile*			m_file;

public:
	CAnimCacheLoadingToken( IFile* file, Uint64 offset )
		: IFileLatentLoadingToken( offset )
		, m_file( file )
	{}

	//! Resume loading, returns valid IFile that can be used to continue file loading
	virtual IFile* Resume( Uint64 relativeOffset )
	{
		return new CAnimCacheFileWrapper( m_file, m_offset );
	}

	//! Clone token. Required as a token can be passed between threads.
	virtual IFileLatentLoadingToken* Clone() const
	{
		return new CAnimCacheLoadingToken( m_file, m_offset );
	}

	//! Describe loading token
	virtual String GetDebugInfo() const
	{
		return String::Printf( TXT("AnimationCache, offset %") RED_PRIWu64, m_offset );
	}
};

//////////////////////////////////////////////////////////////////////////

class AnimationsCache
{
	TDynArray< AnimCacheEntry >		m_entries;
	IFile*							m_file;
	Uint32							m_animOffset;

public:
	Bool Initialize( const String& absoluteFileName );
	void Destroy();

	IFileLatentLoadingToken* CreateLoadingToken( Int32 animationId );
};

typedef TSingleton< AnimationsCache > SAnimationsCache;
