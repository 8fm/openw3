/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "../engine/renderVertices.h"

class CRenderTexture;
class CRenderShaderPair;
class CRenderShaderQuadruple;
class CRenderShaderCompute;
class CRenderShaderTriple;
class CRenderShaderStreamOut;
class CRenderCubeTexture;

/// Stuff drawer
class CRenderDrawer
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_SmallObjects, MC_RenderData );
private:
	GpuApi::BufferRef			m_defaultSphereVertices;
	GpuApi::BufferRef			m_defaultSphereIndices;

	GpuApi::BufferRef			m_defaultConeVertices;
	GpuApi::BufferRef			m_defaultConeIndices;

	Uint32						m_numDefaultSphereVertices;
	Uint32						m_numDefaultSphereIndices;
	Uint32						m_numDefaultConeVertices;
	Uint32						m_numDefaultConeIndices;

private:
	bool GenerateSphereTriangleList( TDynArray<DebugVertexBase> &outVertices, TDynArray<Uint16> &outIndices, const Vector &center, Float radius, Int32 tesselation );
	bool GenerateConeTriangleList( TDynArray<DebugVertexBase> &outVertices, TDynArray<Uint16> &outIndices, Int32 tesselation );
	bool InitDefaultSphere( Uint32 verticalTesselation );
	bool InitDefaultCone( Uint32 sideTesselation );

public:
	CRenderShaderPair* GetShaderPlain();
	CRenderShaderPair* GetShaderSingleColor();
	CRenderShaderPair* GetShaderTextured();
	CRenderShaderPair* GetShaderCubeTextured();

public:
	CRenderDrawer();
	~CRenderDrawer();

	// Normal rendering
	void DrawLineList( const Matrix* localToWorld, const DebugVertex* points, Uint32 numLines );
	void DrawIndexedLineList( const Matrix* localToWorld,  const DebugVertex* points, const Uint32 numPoints, const Uint16* indices, const Uint32 numLines );
	void DrawPolyList( const Matrix* localToWorld, const DebugVertex* points, Uint32 numPoints, const Uint16* indices, Uint32 numIndices );
	void DrawText( const Matrix* localToWorld, const DebugVertexUV* points, Uint32 numTriangles, CRenderTexture* texture, bool textureSRGB );
	void DrawTile( const Matrix* localToWorld, float x, float y, float w, float h, const Color& color, float u0, float v0, float u1, float v1, const GpuApi::TextureRef &texture, Bool textureAlphaChannel, bool textureSRGB, Float resultColorExponent );
	void DrawTile( const DebugVertexUV* points, CRenderTexture* texture, ERenderingPass pass, bool textureSRGB, bool withAlpha=true );
	void DrawTile( const DebugVertexUV* points, CRenderCubeTexture* texture, ERenderingPass pass, bool textureSRGB );
	void DrawCubeMap( GpuApi::TextureRef cubeTexture );
	void DrawQuad2D( const Matrix* translationMatrix, Float width, Float height, Float shiftY, const Color &color ); // draws 2D quad on the screen without lighting
	void DrawQuad2DEx( Int32 x, Int32 y, Int32 width, Int32 height, const Color &color ); // draws 2D quad on the screen without lighting
	void DrawQuad2DExf( Float x, Float y, Float width, Float height, const Color &color ); // draws 2D quad on the screen without lighting
	void DrawUnitSphere();
	void DrawUnitCone();
	void DrawUnitCube();

	void DrawTexturePreviewTile( float x, float y, float w, float h, const GpuApi::TextureRef &texture, Uint32 mipIndex, Uint32 sliceIndex, Float colorMin, Float colorMax, Vector channelSelector );

	//dex++: ESM filtering
	void DrawDepthBufferPatchWithGauss( GpuApi::TextureRef depthSurface, int margin , float farPlane );
	void DrawDepthBufferPatchWithGaussN( GpuApi::TextureRef depthSurface, const DebugVertexUV* vertices, Uint32 numVertices );
	//dex--
	
	// Light shapes and shadows
	void DrawQuad( const Vector &min, const Vector &max, Float z );
};
