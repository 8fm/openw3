/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderCube.h"
#include "renderShaderPair.h"
#include "renderTexture.h"
#include "../engine/renderFragment.h"

CRenderDrawer::CRenderDrawer()
	: m_numDefaultSphereVertices ( 0 )
	, m_numDefaultSphereIndices ( 0 )
	, m_numDefaultConeVertices ( 0 )
	, m_numDefaultConeIndices ( 0 )
{		
	VERIFY( InitDefaultSphere( 12 ) );
	VERIFY( InitDefaultCone( 24 ) );
}

CRenderDrawer::~CRenderDrawer()
{
	GpuApi::SafeRelease(m_defaultSphereVertices);
	GpuApi::SafeRelease(m_defaultSphereIndices);
	GpuApi::SafeRelease(m_defaultConeVertices);
	GpuApi::SafeRelease(m_defaultConeIndices);
}

bool CRenderDrawer::InitDefaultSphere( Uint32 verticalTesselation )
{
	// Generate vertex data
	TDynArray<DebugVertexBase>  sphereVertices;
	TDynArray<Uint16>			sphereIndices;
	if ( !GenerateSphereTriangleList( sphereVertices, sphereIndices, Vector::ZEROS, 1.f, verticalTesselation ) )
	{
		ASSERT( !"Failed to generate default light sphere triangles" );
		return false;
	}

	// Create vertex buffer
	GpuApi::BufferInitData bufInitData;
	bufInitData.m_buffer = sphereVertices.TypedData();
	GpuApi::BufferRef bufferVertices = GpuApi::CreateBuffer( sphereVertices.Size() * sizeof( DebugVertexBase ), GpuApi::BCC_Vertex, GpuApi::BUT_Immutable, 0, &bufInitData );
	if ( !bufferVertices )
	{
		ASSERT( !"Failed to generate default light sphere triangles" );
		return false;
	}
	GpuApi::SetBufferDebugPath( bufferVertices, "defaultSphereVB" );

	// Create index buffer
	bufInitData.m_buffer = sphereIndices.TypedData();
	GpuApi::BufferRef bufferIndices = GpuApi::CreateBuffer( sphereIndices.Size() * 2, GpuApi::BCC_Index16Bit, GpuApi::BUT_Immutable, 0, &bufInitData );
	if ( !bufferIndices )
	{
		ASSERT( !"Failed to generate default light sphere triangles" );
		GpuApi::SafeRelease( bufferVertices );
		return false;
	}
	GpuApi::SetBufferDebugPath( bufferIndices, "defaultSphereIB" );
	
	GpuApi::SafeRelease( m_defaultSphereVertices );
	GpuApi::SafeRelease( m_defaultSphereIndices );
	m_defaultSphereVertices = bufferVertices;
	m_defaultSphereIndices = bufferIndices;
	m_numDefaultSphereVertices = sphereVertices.Size();
	m_numDefaultSphereIndices = sphereIndices.Size();

	return true;
}

Bool CRenderDrawer::InitDefaultCone( Uint32 sideTesselation )
{
	// Generate vertex data
	TDynArray<DebugVertexBase>  coneVertices;
	TDynArray<Uint16>			coneIndices;
	if ( !GenerateConeTriangleList( coneVertices, coneIndices, sideTesselation ) )
	{
		ASSERT( !"Failed to generate default light cone triangles" );
		return false;
	}

	// Create vertex buffer
	GpuApi::BufferInitData bufInitData;
	bufInitData.m_buffer = coneVertices.TypedData();
	GpuApi::BufferRef bufferVertices = GpuApi::CreateBuffer( coneVertices.Size() * sizeof( DebugVertexBase ), GpuApi::BCC_Vertex, GpuApi::BUT_Immutable, 0, &bufInitData );
	if ( !bufferVertices )
	{
		ASSERT( !"Failed to generate default light cone triangles" );
		return false;
	}
	GpuApi::SetBufferDebugPath( bufferVertices, "defaultConeVB" );

	// Create index buffer
	bufInitData.m_buffer = coneIndices.TypedData();
	GpuApi::BufferRef bufferIndices = GpuApi::CreateBuffer( coneIndices.Size() * 2, GpuApi::BCC_Index16Bit, GpuApi::BUT_Immutable, 0, &bufInitData );
	if ( !bufferIndices )
	{
		ASSERT( !"Failed to generate default light cone triangles" );
		GpuApi::SafeRelease( bufferVertices );
		return false;
	}
	GpuApi::SetBufferDebugPath( bufferIndices, "defaultConeIB" );

	GpuApi::SafeRelease( m_defaultConeVertices );
	GpuApi::SafeRelease( m_defaultConeIndices );
	m_defaultConeVertices = bufferVertices;
	m_defaultConeIndices = bufferIndices;
	m_numDefaultConeVertices = coneVertices.Size();
	m_numDefaultConeIndices = coneIndices.Size();

	return true;
}

//dex++: ESM filtering
void CRenderDrawer::DrawDepthBufferPatchWithGauss( GpuApi::TextureRef depthSurface, int margin , float farPlane )
{
	// Set texture
	GpuApi::BindTextures( 0, 1, &depthSurface, GpuApi::PixelShader );
	GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_ClampPointNoMip, GpuApi::PixelShader );
	
	// Bind shader
	GetRenderer()->m_shaderESMGauss3->Bind();

	// Calculate positions
	const Float x0 = 0.0f;
	const Float y0 = 0.0f;
	const Float x1 = 1.0f;
	const Float y1 = 1.0f;

	// 4 and 0 are encoded values to calculate tanged in shader, means 1.0f 
	Color color00(   0,   0,   4,   0 );
	Color color01(   0, 255,   4,   0 );
	Color color10( 255,   0,   4,   0 );
	Color color11( 255, 255,   4,   0 );

	const Float zf = farPlane;
	const Float zn = 0.05f;
	const Float nf = zn * zf;

	// Assemble vertices
	DebugVertexUV points[6];
	const Color color(1,1,1,1);
	// In shader there is lovely hardcoded +2 offset for all sampling pixels. Sub 2 to get true shader pixel offset.
	const Float pixelOffset = (Float)( margin - 2 ); 
	points[0].Set( Vector( x0, y0, 0.50f ), color00, pixelOffset, nf );
	points[1].Set( Vector( x1, y0, 0.50f ), color10, pixelOffset, nf );
	points[2].Set( Vector( x1, y1, 0.50f ), color11, pixelOffset, nf );
	points[3].Set( Vector( x0, y0, 0.50f ), color00, pixelOffset, nf );
	points[4].Set( Vector( x1, y1, 0.50f ), color11, pixelOffset, nf );
	points[5].Set( Vector( x0, y1, 0.50f ), color01, pixelOffset, nf );

	// Draw
	GpuApi::DrawSystemPrimitive( GpuApi::PRIMTYPE_TriangleList, 2, points );
}
//dex--

//dex++: ESM filtering
void CRenderDrawer::DrawDepthBufferPatchWithGaussN( GpuApi::TextureRef depthSurface, const DebugVertexUV* vertices, Uint32 numVertices )
{
	// Set texture
	GpuApi::BindTextures( 0, 1, &depthSurface, GpuApi::PixelShader );
	GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_ClampPointNoMip, GpuApi::PixelShader );
	
	// Bind shader
	GetRenderer()->m_shaderESMGauss3->Bind();

	// Draw
	GpuApi::DrawSystemPrimitive( GpuApi::PRIMTYPE_TriangleList, numVertices/3, vertices );
}
//dex--

//dex++
void CRenderDrawer::DrawTexturePreviewTile( float x, float y, float w, float h, const GpuApi::TextureRef &texture, Uint32 mipIndex, Uint32 sliceIndex, Float colorMin, Float colorMax, Vector channelSelector )
{
	CGpuApiScopedDrawContext scopedDrawContext( GpuApi::DRAWCONTEXT_2D );

	// Set texture
	GpuApi::BindTextures( 0, 1, &texture, GpuApi::PixelShader );
	GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_WrapLinearMip );
	
	// Bind shader
	GpuApi::TextureDesc desc = GpuApi::GetTextureDesc( texture );

	Uint16 cubeSide = 0;

	if( desc.type == GpuApi::TEXTYPE_CUBE )
	{
		cubeSide	= GpuApi::CalculateCubemapFaceIndexFromSlice( desc , sliceIndex );
		mipIndex	= GpuApi::CalculateCubemapMipIndexFromSlice( desc, sliceIndex );
		sliceIndex	= GpuApi::CalculateCubemapArrayIndexFromSlice( desc , sliceIndex );	

		// CubeArray
		if( desc.sliceNum > 1 )
		{
			GetRenderer()->m_shaderTexturePreviewCubeArray->Bind();

			sliceIndex = Clamp< Uint32 >( sliceIndex, 0, desc.sliceNum-1 );
		}
		// Regular Cube
		else
		{
			GetRenderer()->m_shaderTexturePreviewCube->Bind();
		}
	}
	else if ( desc.type == GpuApi::TEXTYPE_ARRAY )
	{
		GetRenderer()->m_shaderTexturePreviewArray->Bind();
		sliceIndex = Clamp< Uint32 >( sliceIndex, 0, desc.sliceNum-1 );
	}
	else if ( desc.type == GpuApi::TEXTYPE_2D )
	{
		GetRenderer()->m_shaderTexturePreview2D->Bind();
		sliceIndex = 0;
	}

	mipIndex = Clamp< Uint32 >( mipIndex, 0, desc.initLevels-1 );

	// Set result exponent
	GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_0, Vector( colorMin, colorMin, colorMin, 0.0f ) );
	GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_1, Vector( colorMax, colorMax, colorMax, 1.0f) );
	GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_2, Vector( (Float)mipIndex, (Float)sliceIndex, (Float)cubeSide, 0.0f ) );
	GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_3, channelSelector );

	// Set matrices	
	GetRenderer()->GetStateManager().SetLocalToWorld( &Matrix::IDENTITY );

	// Mapping
	float u0 = 0.0f;
	float v0 = 0.0f;
	float u1 = 1.0f;
	float v1 = 1.0f;

	// Assemble vertices
	DebugVertexUV points[6];
	Color color(1,1,1,1);
	points[0].Set( Vector( x, y, 0.50f ), color, u0, v0 );
	points[1].Set( Vector( x+w, y, 0.50f ), color, u1, v0 );
	points[2].Set( Vector( x+w, y+h, 0.50f ), color, u1, v1 );
	points[3].Set( Vector( x, y, 0.50f ), color, u0, v0 );
	points[4].Set( Vector( x+w, y+h, 0.50f ), color, u1, v1 );
	points[5].Set( Vector( x, y+h, 0.50f ), color, u0, v1 );

	// Draw
	GpuApi::DrawSystemPrimitive( GpuApi::PRIMTYPE_TriangleList, 2, points );
}
//dex--

void CRenderDrawer::DrawTile( const Matrix* localToWorld, float x, float y, float w, float h, const Color& color, float u0, float v0, float u1, float v1, const GpuApi::TextureRef &texture, Bool textureAlphaChannel, bool textureSRGB, Float resultColorExponent )
{
	const Float srgb_exponent = (textureSRGB ? 2.2f : 1.f);

	// Set texture
	GpuApi::BindTextures( 0, 1, &texture, GpuApi::PixelShader );
	GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_WrapLinearMip );
	
	// Bind shader
	if ( textureAlphaChannel )
	{
		GetRenderer()->m_shaderAlpha->Bind();
	}
	else
	{
		GetRenderer()->m_shaderTextured->Bind();		
	}

	// Set result exponent
	GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_0, Vector::ONES * (srgb_exponent * resultColorExponent) );

	// Set matrices	
	GetRenderer()->GetStateManager().SetLocalToWorld( localToWorld );

	// Assemble vertices
	DebugVertexUV points[6];
	points[0].Set( Vector( x, y, 0.50f ), color, u0, v0 );
	points[1].Set( Vector( x+w, y, 0.50f ), color, u1, v0 );
	points[2].Set( Vector( x+w, y+h, 0.50f ), color, u1, v1 );
	points[3].Set( Vector( x, y, 0.50f ), color, u0, v0 );
	points[4].Set( Vector( x+w, y+h, 0.50f ), color, u1, v1 );
	points[5].Set( Vector( x, y+h, 0.50f ), color, u0, v1 );

	// Draw
	GpuApi::DrawSystemPrimitive( GpuApi::PRIMTYPE_TriangleList, 2, points );
}

void CRenderDrawer::DrawLineList( const Matrix* localToWorld, const DebugVertex* points, Uint32 numLines )
{
	// Bind shader
	GetRenderer()->m_shaderPlain->Bind();

	// Set matrices	
	GetRenderer()->GetStateManager().SetLocalToWorld( localToWorld );

	// Draw lines
	GpuApi::DrawSystemPrimitive( GpuApi::PRIMTYPE_LineList, numLines, points );
}

void CRenderDrawer::DrawIndexedLineList( const Matrix* localToWorld,  const DebugVertex* points, const Uint32 numPoints, const Uint16* indices, const Uint32 numLines )
{
	// Bind shader
	GetRenderer()->m_shaderPlain->Bind();

	// Set matrices	
	GetRenderer()->GetStateManager().SetLocalToWorld( localToWorld );

	// Draw polygons
	GpuApi::DrawSystemIndexedPrimitive( GpuApi::PRIMTYPE_LineList, 0, numPoints, numLines, indices, points );
}

void CRenderDrawer::DrawTile( const DebugVertexUV* points, CRenderTexture* texture, ERenderingPass pass, bool textureSRGB, bool withAlpha )
{
	const Float srgb_exponent = (textureSRGB ? 2.2f : 1.f);
	
	// If we have texture then use it
	if ( texture )
	{
		// Bind to default sampler
		texture->Bind( 0 );

		// Set sampler states
		GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_WrapLinearMip );
		
		// Use shader that render texture
		if ( pass == RP_HitProxies )
		{
			// Render as textured hit proxy
			// [ignore srgb setting here]
			GetRenderer()->m_shaderHitProxy->Bind();
		}
		else
		{
			// Render with texture
			if ( withAlpha )
			{
				GetRenderer()->m_shaderTextured->Bind();
			}
			else
			{
				GetRenderer()->m_shaderTexturedNoAlpha->Bind();
			}

			// Set result exponent
			GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_0, Vector::ONES * srgb_exponent );
		}
	}
	else
	{
		// Use plain color shader
		GetRenderer()->m_shaderPlain->Bind();
	}

	// Draw !
	GpuApi::DrawSystemPrimitive( GpuApi::PRIMTYPE_TriangleStrip, 2, points );
}

void CRenderDrawer::DrawTile( const DebugVertexUV* points, CRenderCubeTexture* texture, ERenderingPass pass, bool textureSRGB )
{
	const Float srgb_exponent = (textureSRGB ? 2.2f : 1.f);

	// If we have texture then use it
	if ( texture )
	{
		// Bind to default sampler
		texture->Bind( 0, RST_PixelShader );

		// Set sampler states
		GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_WrapLinearMip );

		// Use shader that render texture
		if ( pass == RP_HitProxies )
		{
			// Render as textured hit proxy
			// [ignore srgb setting here]
			GetRenderer()->m_shaderHitProxy->Bind();
		}
		else
		{
			// Render with texture
			GetRenderer()->m_shaderTextured->Bind();

			// Set result exponent
			GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_0, Vector::ONES * srgb_exponent );
		}
	}
	else
	{
		// Use plain color shader
		GetRenderer()->m_shaderPlain->Bind();
	}

	// Draw !
	GpuApi::DrawSystemPrimitive( GpuApi::PRIMTYPE_TriangleStrip, 2, points );
}

void CRenderDrawer::DrawPolyList( const Matrix* localToWorld, const DebugVertex* points, Uint32 numPoints, const Uint16* indices, Uint32 numIndices )
{
	// Bind shader
	GetRenderer()->m_shaderPlain->Bind();

	// Set matrices	
	GetRenderer()->GetStateManager().SetLocalToWorld( localToWorld );

	// Draw polygons
	if ( indices != nullptr )
	{
		GpuApi::DrawSystemIndexedPrimitive( GpuApi::PRIMTYPE_TriangleList, 0, numPoints, numIndices / 3, indices, points );
	}
	else
	{
		GpuApi::DrawSystemPrimitive( GpuApi::PRIMTYPE_TriangleList, numPoints / 3, points );
	}
}

void CRenderDrawer::DrawText( const Matrix* localToWorld, const DebugVertexUV* points, Uint32 numTriangles, CRenderTexture* texture, bool textureSRGB )
{
	if ( NULL == points || 0 == numTriangles )
	{
		return;
	}

	const Float srgb_exponent = (textureSRGB ? 1.f/*2.2f*/ : 1.f);

	if(!texture)
	{
		RED_HALT( "Font texture missing" );
	}
	else
	{
		// Bind texture
		texture->Bind( 0 );
	}

	GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_WrapPointNoMip );

	// ace_fix!!!!!
	// ace_hack!!!!
	// this is lame, but I leave this here to keep previous convention
	ASSERT( GpuApi::DRAWCONTEXT_2D == GpuApi::GetDrawContext() );
	CGpuApiScopedDrawContext scopedDrawContext( GpuApi::DRAWCONTEXT_Text2D );
	
	// Set matrices	
	GetRenderer()->GetStateManager().SetLocalToWorld( localToWorld );

	// Set exponent
	GetRenderer()->GetStateManager().SetPixelConst( PSC_Custom_0, Vector::ONES * srgb_exponent );

	// Draw text
	GetRenderer()->m_shaderTextNormal->Bind();
	GpuApi::DrawSystemPrimitive( GpuApi::PRIMTYPE_TriangleList, numTriangles, points );
}

bool CRenderDrawer::GenerateSphereTriangleList( TDynArray<DebugVertexBase> &outVertices, TDynArray<Uint16> &outIndices, const Vector &center, Float radius, Int32 tesselation )
{
	// ACE_TODO: currently used vertical vertex distribution is suboptimal. geosphere would be better here.

	if ( tesselation < 2 )
		return false;

	// Calculate primitives number
	const Int32 verts_h = 2 * tesselation + 1;
	const Int32 verts_v = tesselation + 1;
	const Int32 verts_count = verts_h * verts_v;
	const Int32 indices_count = (verts_h - 1) * (verts_v - 1) * 6;

	// If there are too many vertices for given index type, then leave
	if ( verts_count > (1<<16) )
	{
		return false;
	}

	// Allocate buffers space
	outVertices.Resize( verts_count );	
	outIndices.Resize( indices_count );
	
	// Assemble vertices
	for ( Int32 j=0; j<verts_v; ++j )
	{	
		Float x = sinf( M_PI * j / (verts_v - 1.f) );
		Float y = cosf( M_PI * j / (verts_v - 1.f) );
		for ( Int32 i=0; i<verts_h; ++i )
		{
			Float s = sinf( 2 * M_PI * i / (verts_h - 1.f) );
			Float c = cosf( 2 * M_PI * i / (verts_h - 1.f) );
			outVertices[ i + j * verts_h ].Set( center + Vector ( c * x + s * 0/*y*/,  y,  -s * x + c * 0/*y*/ ) * radius );
		}
	}

	// Assemble indices
	for ( Int32 j=0, index_offset=0; j<verts_v-1; ++j )
	for ( Int32 i=0;                 i<verts_h-1; ++i )
	{
#define ADD_NEW_INDEX( value )							\
		do {											\
		ASSERT( (value) < (1<<16) );					\
		outIndices[index_offset++] = (Uint16)(value);	\
		} while (0)

		ADD_NEW_INDEX( i   + j     * verts_h );
		ADD_NEW_INDEX( i+1 + j     * verts_h );
		ADD_NEW_INDEX( i+1 + (j+1) * verts_h );
		ADD_NEW_INDEX( i   + j     * verts_h );
		ADD_NEW_INDEX( i+1 + (j+1) * verts_h );
		ADD_NEW_INDEX( i   + (j+1) * verts_h );

#undef ADD_NEW_INDEX
	}

	return true;
}

bool CRenderDrawer::GenerateConeTriangleList( TDynArray<DebugVertexBase> &outVertices, TDynArray<Uint16> &outIndices, Int32 tesselation )
{
	if ( tesselation < 2 )
		return false;

	// Calculate primitives number
	const Int32 verts_h = 2 * tesselation + 1;
	const Int32 verts_v = tesselation + 1;
	const Int32 verts_count = verts_h * verts_v + 1;
	const Int32 indices_count = (verts_h - 1) * (verts_v - 1) * 6 + (verts_h - 1) * 3;
	const Vector center = Vector::ZERO_3D_POINT;
	const Float radius = 1.f;

	// If there are too many vertices for given index type, then leave
	if ( verts_count > (1<<16) )
	{
		return false;
	}

	// Allocate buffers space
	outVertices.Resize( verts_count );	
	outIndices.Resize( indices_count );

	// Assemble vertices
	for ( Int32 j=0; j<verts_v; ++j )
	{	
		Float x = sinf( 0.5f * M_PI * j / (verts_v - 1.f) );
		Float y = cosf( 0.5f * M_PI * j / (verts_v - 1.f) );
		for ( Int32 i=0; i<verts_h; ++i )
		{
			Float s = sinf( 2 * M_PI * i / (verts_h - 1.f) );
			Float c = cosf( 2 * M_PI * i / (verts_h - 1.f) );
			outVertices[ i + j * verts_h ].Set( center + Vector ( c * x + s * 0/*y*/,  y,  -s * x + c * 0/*y*/ ) * radius );
		}
	}
	outVertices.Back().Set( center );

	// Assemble indices
	{
#define ADD_NEW_INDEX( value )							\
		do {											\
		ASSERT( (value) < (1<<16) );					\
		ASSERT( (value) < verts_count );				\
		outIndices[index_offset++] = (Uint16)(value);	\
		} while (0)

		Int32 index_offset = 0;
		for ( Int32 j=0; j<verts_v-1; ++j )
		for ( Int32 i=0; i<verts_h-1; ++i )
		{
			ADD_NEW_INDEX( i   + j     * verts_h );
			ADD_NEW_INDEX( i+1 + j     * verts_h );
			ADD_NEW_INDEX( i+1 + (j+1) * verts_h );
			ADD_NEW_INDEX( i   + j     * verts_h );
			ADD_NEW_INDEX( i+1 + (j+1) * verts_h );
			ADD_NEW_INDEX( i   + (j+1) * verts_h );
		}

		for ( Int32 i=0; i<verts_h-1; ++i )
		{
			ADD_NEW_INDEX( i   + (verts_v - 1) * verts_h );
			ADD_NEW_INDEX( i+1 + (verts_v - 1) * verts_h );
			ADD_NEW_INDEX( verts_count - 1 );
		}

#undef ADD_NEW_INDEX
	}

	return true;
}

void CRenderDrawer::DrawQuad( const Vector &min, const Vector &max, Float z )
{
	// Setup vertices
	DebugVertexUV vertices[ 4 ];	
	vertices[0].Set( Vector( max.X, max.Y, z ), Color::WHITE, 1.0f, 1.0f );
	vertices[1].Set( Vector( max.X, min.Y, z ), Color::WHITE, 1.0f, 0.0f );
	vertices[2].Set( Vector( min.X, max.Y, z ), Color::WHITE, 0.0f, 1.0f );
	vertices[3].Set( Vector( min.X, min.Y, z ), Color::WHITE, 0.0f, 0.0f );

	// Set matrices
	GetRenderer()->GetStateManager().SetLocalToWorld( NULL );

	// Render
	GpuApi::DrawSystemPrimitive( GpuApi::PRIMTYPE_TriangleStrip, 2, &vertices[0] );
}

void CRenderDrawer::DrawCubeMap( GpuApi::TextureRef cubeTexture )
{
	GetShaderCubeTextured()->Bind();

	GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_WrapLinearMip );
	GpuApi::BindTextures( 0, 1, &cubeTexture, GpuApi::PixelShader );

	DrawUnitSphere();
}

void CRenderDrawer::DrawUnitSphere()
{
	ASSERT( m_defaultSphereVertices && m_defaultSphereIndices );

	// Bind buffers

	Uint32 vbOffset = 0;
	Uint32 vbSstride = GpuApi::GetChunkTypeStride( GpuApi::BCT_VertexSystemPos );
	GpuApi::SetVertexFormatRaw( GpuApi::BCT_VertexSystemPos );
	GpuApi::BindVertexBuffers(0, 1, &m_defaultSphereVertices, &vbSstride, &vbOffset);

	GpuApi::BindIndexBuffer( m_defaultSphereIndices );

	// Draw chunk
	GpuApi::DrawIndexedPrimitive( GpuApi::PRIMTYPE_TriangleList, 0, 0, m_numDefaultSphereVertices, 0, m_numDefaultSphereIndices / 3 );
}

void CRenderDrawer::DrawUnitCone()
{
	ASSERT( m_defaultConeVertices && m_defaultConeIndices );

	// Bind buffers

	Uint32 vbOffset = 0;
	Uint32 vbSstride = GpuApi::GetChunkTypeStride( GpuApi::BCT_VertexSystemPos );
	GpuApi::SetVertexFormatRaw( GpuApi::BCT_VertexSystemPos );
	GpuApi::BindVertexBuffers(0, 1, &m_defaultConeVertices, &vbSstride, &vbOffset);

	GpuApi::BindIndexBuffer( m_defaultConeIndices );

	// Draw chunk
	GpuApi::DrawIndexedPrimitive( GpuApi::PRIMTYPE_TriangleList, 0, 0, m_numDefaultConeVertices, 0, m_numDefaultConeIndices / 3 );
}

void CRenderDrawer::DrawUnitCube()
{
	// Setup vertices
	DebugVertexBase vertices[ 8 ];	
	vertices[0].Set( Vector( -1.0f, -1.0f, -1.0f ) );
	vertices[1].Set( Vector( -1.0f, 1.0f, -1.0f ) );
	vertices[2].Set( Vector( 1.0f, 1.0f, -1.0f ) );		
	vertices[3].Set( Vector( 1.0f, -1.0f, -1.0f ) );
	vertices[4].Set( Vector( -1.0f, -1.0f, 1.0f ) );
	vertices[5].Set( Vector( -1.0f, 1.0f, 1.0f ) );
	vertices[6].Set( Vector( 1.0f, 1.0f, 1.0f ) );
	vertices[7].Set( Vector( 1.0f, -1.0f, 1.0f ) );

	Uint16 indices[36];

	/*Front side*/
	indices[0]  = 1; indices[1]  = 0; indices[2]  = 2;
	indices[3]  = 2; indices[4]  = 0; indices[5]  = 3;
	/*Back side*/
	indices[6]  = 6; indices[7]  = 4; indices[8]  = 5;
	indices[9]  = 7; indices[10] = 4; indices[11] = 6;
	/*Left side*/
	indices[12] = 5; indices[13] = 4; indices[14] = 1;
	indices[15] = 1; indices[16] = 4; indices[17] = 0;
	/*Right side*/
	indices[18] = 2; indices[19] = 3; indices[20] = 6;
	indices[21] = 6; indices[22] = 3; indices[23] = 7;
	/*Top*/
	indices[24] = 5; indices[25] = 1; indices[26] = 6;
	indices[27] = 6; indices[28] = 1; indices[29] = 2;
	/*Bottom*/
	indices[30] = 0; indices[31] = 4; indices[32] = 3;
	indices[33] = 3; indices[34] = 4; indices[35] = 7;

	// Render
	GpuApi::DrawSystemIndexedPrimitive( GpuApi::PRIMTYPE_TriangleList, 0, 8, 12, &indices[0], &vertices[0] );
}


void CRenderDrawer::DrawQuad2D( const Matrix* translationMatrix, Float width, Float height, Float shiftY, const Color &color )
{
	const float bezel = 5.0f;
	Vector startPoint = translationMatrix->TransformPoint( Vector( 0, 0, 0 ) );

	const Matrix m = Matrix::IDENTITY;
	GetRenderer()->GetStateManager().SetLocalToWorld( &m );

	// Set render shaders
	GetRenderer()->m_shaderPlain->Bind();

	// Assemble vertices
	float x = startPoint.X - bezel;
	float y = startPoint.Y - shiftY - bezel;
	float z = 0.5f;
	float w = width + 2.0f * bezel;
	float h = height + 2.0f * bezel;
	DebugVertex points[6];
	points[0].Set( Vector( x,   y,   z ), color );
	points[1].Set( Vector( x+w, y,   z ), color );
	points[2].Set( Vector( x+w, y+h, z ), color );
	points[3].Set( Vector( x,   y,   z ), color );
	points[4].Set( Vector( x+w, y+h, z ), color );
	points[5].Set( Vector( x,   y+h, z ), color );

	// Draw quad
	GpuApi::DrawSystemPrimitive( GpuApi::PRIMTYPE_TriangleList, 2, points );
}

void CRenderDrawer::DrawQuad2DEx( Int32 x, Int32 y, Int32 width, Int32 height, const Color &color )
{
	GetRenderer()->GetStateManager().SetLocalToWorld( &Matrix::IDENTITY );

	// Set render shaders
	GetRenderer()->m_shaderPlain->Bind();

	// Assemble vertices
	float fx = (Float)x;
	float fy = (Float)y;
	float fz = 0.5f;
	float fw = (Float)width;
	float fh = (Float)height;
	DebugVertex points[6];
	points[0].Set( Vector( fx,   fy,   fz ), color );
	points[1].Set( Vector( fx+fw, fy,   fz ), color );
	points[2].Set( Vector( fx+fw, fy+fh, fz ), color );
	points[3].Set( Vector( fx,   fy,   fz ), color );
	points[4].Set( Vector( fx+fw, fy+fh, fz ), color );
	points[5].Set( Vector( fx,   fy+fh, fz ), color );

	// Draw quad
	GpuApi::DrawSystemPrimitive( GpuApi::PRIMTYPE_TriangleList, 2, points );
}

void CRenderDrawer::DrawQuad2DExf( Float x, Float y, Float width, Float height, const Color &color )
{
	GetRenderer()->GetStateManager().SetLocalToWorld( &Matrix::IDENTITY );

	// Set render shaders
	GetRenderer()->m_shaderPlain->Bind();

	// Assemble vertices
	float fx = (Float)x;
	float fy = (Float)y;
	float fz = 0.5f;
	float fw = (Float)width;
	float fh = (Float)height;
	DebugVertex points[6];
	points[0].Set( Vector( fx,   fy,   fz ), color );
	points[1].Set( Vector( fx+fw, fy,   fz ), color );
	points[2].Set( Vector( fx+fw, fy+fh, fz ), color );
	points[3].Set( Vector( fx,   fy,   fz ), color );
	points[4].Set( Vector( fx+fw, fy+fh, fz ), color );
	points[5].Set( Vector( fx,   fy+fh, fz ), color );

	// Draw quad
	GpuApi::DrawSystemPrimitive( GpuApi::PRIMTYPE_TriangleList, 2, points );
}

CRenderShaderPair* CRenderDrawer::GetShaderPlain()
{
	return GetRenderer()->m_shaderPlain;
}

CRenderShaderPair* CRenderDrawer::GetShaderSingleColor()
{
	return GetRenderer()->m_shaderSingleColor;
}

CRenderShaderPair* CRenderDrawer::GetShaderTextured()
{
	return GetRenderer()->m_shaderPlain;
}

CRenderShaderPair* CRenderDrawer::GetShaderCubeTextured()
{
	return GetRenderer()->m_shaderCubeTexture;
}

