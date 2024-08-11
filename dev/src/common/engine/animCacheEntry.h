
#pragma once

struct AnimCacheEntry
{
	Uint64		m_hash;
	Uint32		m_offset;
	Uint32		m_size;

	static const Uint32 INVALID = 0xFFFFFFFF;

	AnimCacheEntry()
		: m_offset( INVALID )
		, m_size( INVALID )
		, m_hash( 0 )
	{

	}

	void OnSerialize( IFile& file )
	{
		file << m_hash;
		file << m_offset;
		file << m_size;
	}

	Bool IsValid() const
	{
		return m_offset != INVALID && m_size != INVALID && m_hash != 0;
	}
};
