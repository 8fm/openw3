/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

struct SChunkLayerInfoPS4
{
	Uint32	m_chunkID;
	Uint32	m_layer;

	SChunkLayerInfoPS4()
		: m_chunkID(0xFFFFFFFF)
		, m_layer(0xFFFFFFFF)
	{
	}

	SChunkLayerInfoPS4( Uint32 chunkID, Uint32 layer )
		: m_chunkID( chunkID )
		, m_layer( layer )
	{}

	static const SChunkLayerInfoPS4 INVALID;

	Bool operator==( const SChunkLayerInfoPS4& rhs ) const
	{ return m_chunkID == rhs.m_chunkID && m_layer == rhs.m_layer; }

	RED_FORCE_INLINE Uint32 CalcHash() const { return m_chunkID ^ m_layer; }
};
