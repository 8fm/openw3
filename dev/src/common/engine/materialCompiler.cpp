#include "build.h"
#include "materialCompiler.h"
#include "renderFragment.h"
#include "baseEngine.h"

MaterialRenderingContext::MaterialRenderingContext( const RenderingContext& renderingContext )
	: m_renderingContext( &renderingContext )
	, m_vertexFactory( MVF_Invalid )
	, m_selected( false )
	, m_uvDissolveSeparateUV( false )
	, m_useInstancing( false )
	, m_useParticleInstancing( false )
	, m_materialDebugMode( MDM_None )
	, m_hasVertexCollapse( false )
	, m_hasExtraStreams( false )
	, m_discardingPass( false )
	, m_pregeneratedMaps( false )
	, m_proxyMaterialCast( false )
	, m_lowQuality( false )
	, m_cachedID( (Uint32)-1 )
{};

Uint32 MaterialRenderingContext::CalcID() const
{
	if ( m_cachedID != (Uint32)-1 )
	{
		return m_cachedID;
	}

	const Uint32 rp_bits = 4;
	const Uint32 vf_bits = 5;
	const Uint32 md_bits = 3;
	static_assert(  RP_Max<=(1<<rp_bits)	&&	 RP_Max>(1<<(rp_bits-1)), "change rp_bits if error is being generated." );
	static_assert( MVF_Max<=(1<<vf_bits)	&&	MVF_Max>(1<<(vf_bits-1)), "change vf_bits if error is being generated." );
	static_assert( MDM_Max<=(1<<md_bits)	&&	MDM_Max>(1<<(md_bits-1)), "change md_bits if error is being generated." );	

	Uint32 bitCount = 0;
	Uint32 val = 0;
#define ADD_BITS( x, num ) { val |= (x) << bitCount; bitCount += num; }

	// Assemble context ID
	ADD_BITS( m_renderingContext->m_pass, rp_bits );
	ADD_BITS( m_vertexFactory, vf_bits );
	ADD_BITS( m_selected ? 1 : 0, 1 );
	ADD_BITS( m_materialDebugMode, md_bits );
	ADD_BITS( m_useInstancing ? 1 : 0, 1 ); // not terrain specific anymore
	ADD_BITS( m_discardingPass ? 1 : 0, 1 );

	// Mesh specific
	if ( m_vertexFactory == MVF_MeshSkinned )
	{
		ADD_BITS( m_hasExtraStreams ? 1 : 0, 1 );
		ADD_BITS( m_hasVertexCollapse ? 1 : 0, 1 );
		ADD_BITS( ( m_discardingPass && m_uvDissolveSeparateUV ) ? 1 : 0, 1 );
	}

	// Static mesh specific
	if ( m_vertexFactory == MVF_MeshStatic )
	{
		ADD_BITS( m_hasExtraStreams ? 1 : 0, 1 );
		ADD_BITS( m_hasVertexCollapse ? 1 : 0, 1 );
		ADD_BITS( m_useParticleInstancing ? 1 : 0, 1);
		ADD_BITS( ( m_discardingPass && m_uvDissolveSeparateUV ) ? 1 : 0, 1 );
	}

	if ( m_vertexFactory == MVF_ApexWithBones )
	{
		ADD_BITS( m_hasExtraStreams ? 1 : 0, 1 );
		ADD_BITS( m_hasVertexCollapse ? 1 : 0, 1 );
	}

	if ( m_vertexFactory == MVF_ApexWithoutBones )
	{
		ADD_BITS( m_hasExtraStreams ? 1 : 0, 1 );
		ADD_BITS( m_hasVertexCollapse ? 1 : 0, 1 );
	}

	if ( m_vertexFactory == MVF_MeshDestruction )
	{
		ADD_BITS( m_hasExtraStreams ? 1 : 0, 1 );
	}

	if ( m_vertexFactory == MVF_TesselatedTerrain )
	{
		ADD_BITS( m_renderingContext->m_terrainToolStampVisible ? 1 : 0, 1 );
		ADD_BITS( m_pregeneratedMaps ? 1 : 0, 1 );
		ADD_BITS( m_lowQuality ? 1 : 0, 1 );
	}

	if( m_vertexFactory == MVF_TerrainSkirt )
	{
		ADD_BITS( m_pregeneratedMaps ? 1 : 0, 1 );
		ADD_BITS( m_lowQuality ? 1 : 0, 1 );
	}

	RED_ASSERT( bitCount <= 8 * sizeof(Uint32) );
#undef ADD_BITS

	m_cachedID = val;
	return val;
}

#ifndef NO_RUNTIME_MATERIAL_COMPILATION
IMPLEMENT_RTTI_ENUM( ETessellationDomain );
#endif // NO_RUNTIME_MATERIAL_COMPILATION
