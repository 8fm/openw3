/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "renderFragment.h"
#include "materialDefinition.h"
#include "fonts.h"
#include "renderObject.h"
#include "renderProxy.h"
#include "baseEngine.h"
#include "renderFrame.h"

RenderingContext::RenderingContext()
	: m_pass( RP_NoLighting )
	, m_terrainToolStampVisible(false)
	, m_grassMaskPaintMode( false )
	, m_forceNoDissolves( false )
	, m_forceNoParticles( false )
	, m_forceNoSwarms( false )
	, m_materialDebugMode( MDM_None )
	, m_lightChannelFilterMask( 0 )
	, m_lightChannelFilter( LCF_NoFilter )
	, m_lightChannelForcedMask( 0 )
	, m_constantHitProxyID( 0 )
	, m_useConstantHitProxyID( false )
{};

RenderingContext::RenderingContext( const CRenderCamera& camera )
	: m_pass( RP_NoLighting )
	, m_terrainToolStampVisible(false)
	, m_grassMaskPaintMode( false )
	, m_forceNoDissolves( false )
	, m_forceNoParticles( false )
	, m_forceNoSwarms( false )
	, m_materialDebugMode( MDM_None )
	, m_lightChannelFilterMask( 0 )
	, m_lightChannelFilter( LCF_NoFilter )
	, m_lightChannelForcedMask( 0 )
	, m_constantHitProxyID( 0 )
	, m_useConstantHitProxyID( false )
	, m_camera( camera )
{};

RenderingContext::RenderingContext( const RenderingContext& context )
	: m_camera( context.m_camera )
	, m_pass( context.m_pass )
	, m_terrainToolStampVisible( context.m_terrainToolStampVisible )
	, m_grassMaskPaintMode( context.m_grassMaskPaintMode )
	, m_forceNoDissolves( context.m_forceNoDissolves )
	, m_forceNoParticles( context.m_forceNoParticles )
	, m_forceNoSwarms( context.m_forceNoSwarms )
	, m_materialDebugMode( context.m_materialDebugMode )
	, m_lightChannelFilterMask( context.m_lightChannelFilterMask )
	, m_lightChannelFilter( context.m_lightChannelFilter )
	, m_lightChannelForcedMask( context.m_lightChannelForcedMask )
	, m_constantHitProxyID( context.m_constantHitProxyID )
	, m_useConstantHitProxyID( context.m_useConstantHitProxyID )
{};

Bool IsShadowRenderPass( ERenderingPass pass )
{
	return ( pass == RP_ShadowDepthSolid ) || ( pass == RP_ShadowDepthMasked );
}

IRenderFragment::IRenderFragment( CRenderFrame* frame, ERenderFragmentType type, Uint32 numPrimitives, const Matrix& localToWorld, ERenderingSortGroup sortGroup )
	: m_frame( frame )
	, m_numPrimitives( numPrimitives )
	, m_localToWorld( localToWorld )
	, m_flags( 0 )
	, m_type( type )
	, m_sortGroup( sortGroup )
	, m_baseLinkNext( NULL )
{
	AddFragmentToFrame( frame, sortGroup );
}

void IRenderFragment::AddFragmentToFrame( CRenderFrame* frame, ERenderingSortGroup sortGroup )
{
	ASSERT( !m_baseLinkNext );

	if ( frame->m_fragments[ sortGroup ] )
	{
		// Add fragment at the end of list of rendering fragments for this frame
		frame->m_fragmentsLast[ sortGroup ]->m_baseLinkNext = this;
		frame->m_fragmentsLast[ sortGroup ] = this;
	}
	else
	{
		// Add the first fragment to list
		frame->m_fragments[ sortGroup ] = this;
		frame->m_fragmentsLast[ sortGroup ] = this;
	}
}

CRenderFragmentDebugLineList::CRenderFragmentDebugLineList( CRenderFrame* frame, const Matrix& localToWorld, const DebugVertex* points, Uint32 numPoints, ERenderingSortGroup sortGroup )
	: IRenderFragment( frame, RFT_DebugLineList, numPoints / 2, localToWorld, sortGroup )
	, m_points( NULL )
{
	// Copy points
	if ( numPoints )
	{
		m_points = new ( frame ) DebugVertex[ numPoints ];
		Red::System::MemoryCopy( m_points, points, sizeof( DebugVertex ) * numPoints );
	}
}

CRenderFragmentDebugIndexedLineList::CRenderFragmentDebugIndexedLineList( CRenderFrame* frame, const Matrix& localToWorld, const DebugVertex* points, const Uint32 numPoints, const Uint16* indices, const Uint32 numIndices, ERenderingSortGroup sortGroup/*=RSG_DebugUnlit*/ )
	: IRenderFragment( frame, RFT_DebugIndexedLineList, numIndices / 2, localToWorld, sortGroup )
	, m_numIndices( numIndices )
	, m_numPoints( numPoints )
	, m_points( NULL )
	, m_indices( NULL )
{
	// Copy points
	if ( numPoints )
	{
		m_points = new ( frame ) DebugVertex[ numPoints ];
		Red::System::MemoryCopy( m_points, points, sizeof( DebugVertex ) * numPoints );
	}

	// Copy indices
	if ( numIndices )
	{
		m_indices = new ( frame ) Uint16[ numIndices ];
		Red::System::MemoryCopy( m_indices, indices, sizeof( Uint16) * numIndices );
	}
}

CRenderFragmentDebugPolyList::CRenderFragmentDebugPolyList( CRenderFrame* frame, const Matrix& localToWorld, const DebugVertex* points, Uint32 numPoints, const Uint16* indices, Uint32 numIndices, ERenderingSortGroup sortGroup/*=RSG_DebugUnlit*/, Bool copyData/*=true*/ )
	: IRenderFragment( frame, RFT_DebugPolyList, numIndices / 3, localToWorld, sortGroup )
	, m_numIndices( numIndices )
	, m_numPoints( numPoints )
	, m_points( NULL )
	, m_indices( NULL )
{
	// Copy points
	if ( numPoints )
	{
		if ( copyData )
		{
			m_points = new ( frame ) DebugVertex[ numPoints ];
			Red::System::MemoryCopy( m_points, points, sizeof( DebugVertex ) * numPoints );
		}
		else
		{
			m_points = const_cast< DebugVertex* >( points );
		}
	}

	// Copy indices
	if ( numIndices )
	{
		if ( copyData )
		{
			m_indices = new ( frame ) Uint16[ numIndices ];
			Red::System::MemoryCopy( m_indices, indices, sizeof( Uint16) * numIndices );
		}
		else
		{
			m_indices = const_cast< Uint16* >( indices );
		}
	}
}

CRenderFragmentDebugRectangle::CRenderFragmentDebugRectangle( CRenderFrame* frame, const Matrix& translationMatrix, Float width, Float height, Float shiftY, const Color &color )
	: IRenderFragment( frame, RFT_DebugRectangle, 2, Matrix::IDENTITY, RSG_2D )
{
	m_translationMatrix = translationMatrix;
	m_width = width;
	m_height = height;
	m_shiftY = shiftY;
	m_color = color;
}

CRenderFragmentDebugRectangle2D::CRenderFragmentDebugRectangle2D( CRenderFrame* frame, Int32 x, Int32 y, Int32 width, Int32 height, const Color &color )
	: IRenderFragment( frame, RFT_DebugRectangle, 2, Matrix::IDENTITY, RSG_2D )
{
	m_x = (Float)x;
	m_y = (Float)y;
	m_width = (Float)width;
	m_height = (Float)height;
	m_color = color;
}

CRenderFragmentDebugRectangle2D::CRenderFragmentDebugRectangle2D( CRenderFrame* frame, Float x, Float y, Float width, Float height, const Color &color )
	: IRenderFragment( frame, RFT_DebugRectangle, 2, Matrix::IDENTITY, RSG_2D )
{
	m_x = x;
	m_y = y;
	m_width = width;
	m_height = height;
	m_color = color;
}

CRenderFragmentTexturedDebugRectangle::CRenderFragmentTexturedDebugRectangle( CRenderFrame* frame, const Float x, const Float y, Float width, Float height, CBitmapTexture *texture, const Color &color, Bool withAlpha )
	: IRenderFragment( frame, RFT_DebugRectangle, 2, Matrix::IDENTITY, RSG_2D )
	, m_texture( texture )
	, m_withAlpha( withAlpha )
{
	// Calculate vertices
	m_vertices[0].Set( Vector( x + width,	y,			0.0f ), color, 1.0f, 0.0f );
	m_vertices[1].Set( Vector( x + width,	y + height,	0.0f ), color, 1.0f, 1.0f );
	m_vertices[2].Set( Vector( x,			y,			0.0f ), color, 0.0f, 0.0f );
	m_vertices[3].Set( Vector( x,			y + height,	0.0f ), color, 0.0f, 1.0f );
}

CRenderFragmentDynamicTexture::CRenderFragmentDynamicTexture( CRenderFrame* frame, Float x, Float y, Float width, Float height, GpuApi::TextureRef textureRef, Uint32 mipIndex, Uint32 sliceIndex, Float colorMin, Float colorMax, Vector channelSelector )
	: IRenderFragment( frame, RFT_DebugRectangle, 2, Matrix::IDENTITY, RSG_2D )
	, m_x( x )
	, m_y( y )
	, m_width( width )
	, m_height( height )
	, m_textureRef( textureRef )
	, m_mipIndex( mipIndex )
	, m_sliceIndex( sliceIndex )
	, m_colorMin( colorMin )
	, m_colorMax( colorMax )
	, m_channelSelector( channelSelector )
{
	/* intentionally empty */
}

CRenderFragmentDebugSprite::CRenderFragmentDebugSprite( CRenderFrame* frame, const Vector& position, Float size, CBitmapTexture* texture, const Color& color, ERenderingSortGroup sortGroup )
	: IRenderFragment( frame, RFT_DebugSprite, 2, Matrix::IDENTITY, sortGroup )
	, m_texture( texture )
{
	// Calculate vertices
	const Vector up = frame->GetFrameInfo().m_camera.GetCameraUp() * size * 0.5f;
	const Vector right = frame->GetFrameInfo().m_camera.GetCameraRight() * size * 0.5f;
	m_vertices[0].Set( position + up + right, color, 1.0f, 0.0f );
	m_vertices[1].Set( position - up + right, color, 1.0f, 1.0f );
	m_vertices[2].Set( position + up - right, color, 0.0f, 0.0f );
	m_vertices[3].Set( position - up - right, color, 0.0f, 1.0f );
}

CRenderFragmentDebugEnvProbe::CRenderFragmentDebugEnvProbe( CRenderFrame* frame, Float gameTime, IRenderResource *renderResource, const Vector& position, Float radius, const CHitProxyID& hitProxy, ERenderingSortGroup sortGroup /*= RSG_Sprites*/ )
	: IRenderFragment( frame, RFT_DebugEnvProbe, 2, Matrix().SetIdentity().SetTranslation( position ).SetScale33( radius ), sortGroup )
	, m_renderResource ( renderResource )
	, m_hitProxy ( hitProxy )
	, m_gameTime ( gameTime )
{}

CRenderFragmentDebugMesh::CRenderFragmentDebugMesh( CRenderFrame* frame, const Matrix& localToWorld, IRenderResource* debugMesh, Bool transparent, Bool wireframe )
	: IRenderFragment( frame, RFT_DebugMesh, 0, localToWorld, transparent ? RSG_DebugTransparent : RSG_DebugUnlit )
	, m_mesh( debugMesh )
	, m_transparent( transparent )
	, m_wireframe( wireframe )
	, m_useColor( false )
{
}

CRenderFragmentDebugMesh::CRenderFragmentDebugMesh( CRenderFrame* frame, const Matrix& localToWorld, IRenderResource* debugMesh, ERenderingSortGroup sortGroup, Bool transparent, Bool wireframe )
	: IRenderFragment( frame, RFT_DebugMesh, 0, localToWorld, sortGroup )
	, m_mesh( debugMesh )
	, m_transparent( transparent )
	, m_wireframe( wireframe )
	, m_useColor( false )
{
}


CRenderFragmentDebugMesh::CRenderFragmentDebugMesh( CRenderFrame* frame, const Matrix& localToWorld, IRenderResource* debugMesh, const CHitProxyID& hitProxyID, Bool transparent, Bool wireframe )
	: IRenderFragment( frame, RFT_DebugMesh, 0, localToWorld, transparent ? RSG_DebugTransparent : RSG_DebugUnlit )
	, m_hitProxy( hitProxyID )
	, m_mesh( debugMesh )
	, m_transparent( transparent )
	, m_wireframe( wireframe )
	, m_useColor( true )
{
}

CRenderFragmentDebugWireMesh::CRenderFragmentDebugWireMesh( CRenderFrame* frame, const Matrix& localToWorld, IRenderResource* debugMesh )
	: IRenderFragment( frame, RFT_DebugMesh, 0, localToWorld, RSG_DebugUnlit )
	, m_mesh( debugMesh )
{
}

CRenderFragmentDebugWireSingleColorMesh::CRenderFragmentDebugWireSingleColorMesh( CRenderFrame* frame, const Matrix& localToWorld, IRenderResource* debugMesh )
	: IRenderFragment( frame, RFT_DebugMesh, 0, localToWorld, RSG_DebugUnlit )
	, m_mesh( debugMesh )
{
}


RED_INLINE IRenderResource* GetMaterialRenderResource( IMaterial* material )
{
	IMaterialDefinition* graph = material ? material->GetMaterialDefinition() : NULL;
	return graph ? graph->GetRenderResource() : NULL;
}

RED_INLINE IRenderResource* GetMaterialParamRenderResource( IMaterial* material )
{
	return material ? material->GetRenderResource() : NULL;
}

CRenderFragmentText::CRenderFragmentText( CRenderFrame* frame, const Matrix& localToCanvas, CFont &font, const Char* text, const Color &color )
	: IRenderFragment( frame, RFT_Text, 0, Matrix::IDENTITY, RSG_2D )
	, m_textures( NULL )
	, m_numTextures( 0 )
	, m_vertices( NULL )
	, m_color( color.ToUint32() )
{
	m_numTextures = font.GetNumTextures();

	// get textures
	m_textures = (IRenderResource**) frame->GetMemoryPool().Alloc( sizeof( IRenderResource*) * m_numTextures );
	for( Uint32 i=0; i<m_numTextures; ++i )
	{
		m_textures[i] = font.GetTexture(i)->GetRenderResource();
		if ( m_textures[i] )
		{
			m_textures[i]->AddRef();
		}
	}

	// find num vertices required
	m_numVertices = (Uint32*) frame->GetMemoryPool().Alloc( sizeof( Uint32 ) * m_numTextures );
	for( Uint32 i=0; i<m_numTextures; ++i )
	{
		m_numVertices[i] = 0;
	}

	const Uint32 len = static_cast< Uint32 >( Red::System::StringLength( text ) );
	for( Uint32 i=0; i<len; ++i )
	{
		const SFontGlyph* glyph = font.GetGlyph( text[i] );
		if ( glyph )
		{
			m_numVertices[ glyph->m_textureIndex ] += 6;
		}
	}

	// create vertices
	m_vertices = (DebugVertexUV**) frame->GetMemoryPool().Alloc( sizeof( DebugVertexUV* ) * m_numTextures );
	for( Uint32 i=0; i<m_numTextures; ++i )
	{	
		if ( m_numVertices[i] > 0 )
		{
			m_vertices[i] = (DebugVertexUV*) frame->GetMemoryPool().Alloc( sizeof( DebugVertexUV ) * m_numVertices[i] );
		}
		else
		{
			m_vertices[i] = NULL;
		}
	}

	// fill vertex data
	Uint32* currNumVertices = (Uint32*) frame->GetMemoryPool().Alloc( sizeof( Uint32 ) * m_numTextures );
	for( Uint32 i=0; i<m_numTextures; ++i )
	{
		currNumVertices[i] = 0;
	}

	Int32 newLine = 0;
	Uint32 penPos = 0;
	for( Uint32 i=0; i<len; ++i )
	{
		const SFontGlyph* glyph = font.GetGlyph( text[i] );
		if ( !glyph )
		{
			if ( text[i] == '\n' )
			{
				newLine++;
				penPos = 0;
			}
			continue;
		}

		Vector screenPos = localToCanvas.TransformPoint( Vector( Float( penPos ), 0, 0 ) );

 		Int32 xPos = Int32( screenPos.X );
 		Int32 yPos = Int32( screenPos.Y );

		yPos += newLine * font.GetLineDist();
		
		// add bearing
		xPos += glyph->m_bearingX;
		yPos -= glyph->m_bearingY;

		// emit glyph
		const Uint32 textureIndex = glyph->m_textureIndex;
		Uint32 lastVertex = currNumVertices[ textureIndex ];

		

		// shift half pixel to exactly fit pixel centers
		m_vertices[ textureIndex ][ lastVertex + 0 ].x = (Float)xPos;
		m_vertices[ textureIndex ][ lastVertex + 0 ].y = (Float)yPos;
		m_vertices[ textureIndex ][ lastVertex + 0 ].z = 0.0f;
		m_vertices[ textureIndex ][ lastVertex + 0 ].u = glyph->m_UVs[0][0];
		m_vertices[ textureIndex ][ lastVertex + 0 ].v = glyph->m_UVs[0][1];		

		m_vertices[ textureIndex ][ lastVertex + 1 ].x = (Float)xPos + (Float)glyph->m_width;
		m_vertices[ textureIndex ][ lastVertex + 1 ].y = (Float)yPos;
		m_vertices[ textureIndex ][ lastVertex + 1 ].z = 0.0f;
		m_vertices[ textureIndex ][ lastVertex + 1 ].u = glyph->m_UVs[1][0];
		m_vertices[ textureIndex ][ lastVertex + 1 ].v = glyph->m_UVs[0][1];

		m_vertices[ textureIndex ][ lastVertex + 2 ].x = (Float)xPos;
		m_vertices[ textureIndex ][ lastVertex + 2 ].y = (Float)yPos + (Float)glyph->m_height;
		m_vertices[ textureIndex ][ lastVertex + 2 ].z = 0.0f;
		m_vertices[ textureIndex ][ lastVertex + 2 ].u = glyph->m_UVs[0][0];
		m_vertices[ textureIndex ][ lastVertex + 2 ].v = glyph->m_UVs[1][1];

		m_vertices[ textureIndex ][ lastVertex + 3 ].x = (Float)xPos + (Float)glyph->m_width;
		m_vertices[ textureIndex ][ lastVertex + 3 ].y = (Float)yPos;
		m_vertices[ textureIndex ][ lastVertex + 3 ].z = 0.0f;
		m_vertices[ textureIndex ][ lastVertex + 3 ].u = glyph->m_UVs[1][0];
		m_vertices[ textureIndex ][ lastVertex + 3 ].v = glyph->m_UVs[0][1];

		m_vertices[ textureIndex ][ lastVertex + 4 ].x = (Float)xPos + (Float)glyph->m_width;
		m_vertices[ textureIndex ][ lastVertex + 4 ].y = (Float)yPos + (Float)glyph->m_height;
		m_vertices[ textureIndex ][ lastVertex + 4 ].z = 0.0f;
		m_vertices[ textureIndex ][ lastVertex + 4 ].u = glyph->m_UVs[1][0];
		m_vertices[ textureIndex ][ lastVertex + 4 ].v = glyph->m_UVs[1][1];

		m_vertices[ textureIndex ][ lastVertex + 5 ].x = (Float)xPos;
		m_vertices[ textureIndex ][ lastVertex + 5 ].y = (Float)yPos + (Float)glyph->m_height;
		m_vertices[ textureIndex ][ lastVertex + 5 ].z = 0.0f;
		m_vertices[ textureIndex ][ lastVertex + 5 ].u = glyph->m_UVs[0][0];
		m_vertices[ textureIndex ][ lastVertex + 5 ].v = glyph->m_UVs[1][1];

		for( int j=0; j<6; ++j )
		{
			m_vertices[ textureIndex ][ lastVertex + j ].color = m_color;
		}

		currNumVertices[ textureIndex ] += 6;

		// move pen to next position
		Uint32 newPenPos = penPos + glyph->m_advanceX;
		penPos = Max( newPenPos, penPos );
	}
}

CRenderFragmentText::~CRenderFragmentText()
{
	for ( Uint32 i=0; i<m_numTextures; i++ )
	{
		if ( m_textures[i] )
		{
			m_textures[i]->Release();
			m_textures[i] = NULL;
		}
	}
}

CRenderFragmentOnScreenLineList::CRenderFragmentOnScreenLineList( CRenderFrame* frame, const DebugVertex* points, Uint32 numPoints )
	: CRenderFragmentDebugLineList( frame, Matrix::IDENTITY, points, numPoints, RSG_2D )
{
}

