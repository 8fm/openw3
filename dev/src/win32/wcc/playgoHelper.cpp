/**
* Copyright (c) 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "playgoHelper.h"

namespace PlayGoHelper
{

CChunkResolver::CChunkResolver()
{
	// create the initial layout (hardcoded)
	AddChunk( eChunkType_Install, TXT("content0"), nullptr );
	AddChunk( eChunkType_Install, TXT("content1"), TXT("content0") );
	AddChunk( eChunkType_Install, TXT("content2"), TXT("content1") );
	AddChunk( eChunkType_Install, TXT("content3"), TXT("content2") );
	AddChunk( eChunkType_Install, TXT("content4"), TXT("content3") );
	AddChunk( eChunkType_Install, TXT("content5"), TXT("content4") );
	AddChunk( eChunkType_Install, TXT("content6"), TXT("content5") );
	AddChunk( eChunkType_Install, TXT("content7"), TXT("content6") );
	AddChunk( eChunkType_Install, TXT("content8"), TXT("content7") );
	AddChunk( eChunkType_Install, TXT("content9"), TXT("content8") );
	AddChunk( eChunkType_Install, TXT("content10"), TXT("content9") );
	AddChunk( eChunkType_Install, TXT("content11"), TXT("content10") );
	AddChunk( eChunkType_Install, TXT("content12"), TXT("content11") );
	AddChunk( eChunkType_Install, TXT("content13"), TXT("content12") );
	AddChunk( eChunkType_Install, TXT("content14"), TXT("content13") );
	AddChunk( eChunkType_Install, TXT("content15"), TXT("content14") );
	AddChunk( eChunkType_Install, TXT("content16"), TXT("content15") );
	AddChunk( eChunkType_Install, TXT("content17"), TXT("content16") );
	AddChunk( eChunkType_Install, TXT("content18"), TXT("content17") );
	AddChunk( eChunkType_Install, TXT("content19"), TXT("content18") );
	AddChunk( eChunkType_Install, TXT("content20"), TXT("content19") );

	// use the content0 as fallback chunk
	m_fallbackChunk = FindChunk( CName( TXT("content0") ) );
}

CChunkResolver::~CChunkResolver()
{
	m_chunks.ClearPtr();
	m_chunkMap.Clear();
}

CName CChunkResolver::GetFallBackChunkName() const
{
	return  m_fallbackChunk->m_id;
}

CName CChunkResolver::ResolveContentChunk( const SResolveContext& context ) const
{
	// basic resolve - select the most generic content chunk we would be included in
	const SChunkInfo* finalChunk = nullptr;
	if ( context.m_chunks )
	{
		for ( Uint32 i=0; i<context.m_chunks->Size(); ++i )
		{
			const SChunkInfo* chunk = FindChunk( (*context.m_chunks)[i] );
			if ( !chunk )
				return CName::NONE;

			// always include in the earliest possible content
			if ( !finalChunk || finalChunk->IsBasedOn( chunk ) )
			{
				finalChunk = chunk;
			}
		}
	}

	// not resolved - use fallback
	if ( !finalChunk )
		return CName::NONE;

	// use the calculated chunk
	return finalChunk->m_id;
}

const CChunkResolver::SChunkInfo* CChunkResolver::FindChunk( const CName name ) const
{
	SChunkInfo* ret = nullptr;
	m_chunkMap.Find( name, ret );
	return ret;
}

void CChunkResolver::AddChunk( const EChunkType type, const Char* name, const Char* baseName )
{
	const CName chunkName( name );
	const CName chunkBaseName( baseName );

	// prevent data duplication
	if ( m_chunkMap.KeyExist( chunkName ) )
	{
		ERR_WCC( TXT("Content chunk '%ls' already registered"), name );
		return;
	}

	// do not add chunk if base chunk is not there
	const SChunkInfo* baseChunk = nullptr;
	if ( chunkBaseName )
	{
		baseChunk = FindChunk( chunkBaseName );
		if ( !baseChunk )
		{
			ERR_WCC( TXT("Content chunk '%ls' cannot be registered because base chunk '%ls' cannot be found"), name, baseName );
			return;
		}
	}

	// create info
	SChunkInfo* info = new SChunkInfo();
	info->m_id = chunkName;
	info->m_base = baseChunk;
	info->m_type = type;

	// add to lists
	m_chunks.PushBack( info );
	m_chunkMap.Insert( chunkName, info );
}

} // PlayGoHelper