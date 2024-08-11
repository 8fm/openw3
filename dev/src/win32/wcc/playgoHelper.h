/**
* Copyright (c) 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef __PLAYGO_HELPER_FILE_H__
#define __PLAYGO_HELPER_FILE_H__

#include "../../common/core/hashmap.h"

namespace PlayGoHelper
{
	/// Chunk resolver
	/// TODO: this should be read from a file
	class CChunkResolver
	{
	public:
		CChunkResolver();
		~CChunkResolver();

		// type of the content chunk
		enum EChunkType
		{
			eChunkType_Install,		//!< Installation chunk
			eChunkType_Patch,		//!< Patching data
			eChunkType_DLC,			//!< DLC data
		};

		// resolve context
		struct SResolveContext
		{
			StringAnsi					m_path;			//!< Path to the resource
			const TDynArray< CName >*	m_chunks;		//!< Currently assigned chunks
			Uint64						m_sourceCRC;	//!< CRC of the source data
		};

		// resolve in which chunk we should put the data, can return CName::NONE if data should be skipped
		CName ResolveContentChunk( const SResolveContext& context ) const;

		// get fallback chunk name
		CName GetFallBackChunkName() const;

	private:
		struct SChunkInfo
		{
			CName				m_id;		//!< ID of the content chunk
			const SChunkInfo*	m_base;		//!< Requirement
			EChunkType			m_type;		//!< Type of the chunk

			RED_INLINE const Bool IsBasedOn( const SChunkInfo* other ) const
			{
				if ( other == this )
					return true;

				if ( m_base )
					return m_base->IsBasedOn( other );

				return false;
			}
		};

		typedef TDynArray< SChunkInfo* >		TChunks;
		typedef THashMap< CName, SChunkInfo* >	TChunkMap;

		TChunks				m_chunks;
		TChunkMap			m_chunkMap;
		const SChunkInfo*	m_fallbackChunk;

		// find chunk by name
		const SChunkInfo* FindChunk( const CName name ) const;

		// add internal chunk definition
		void AddChunk( const EChunkType type, const Char* name, const Char* baseName );
	};

}


#endif
