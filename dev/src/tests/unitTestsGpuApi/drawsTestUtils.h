/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#ifndef _DRAWS_TEST_UTILS_H_
#define _DRAWS_TEST_UTILS_H_

#include "meshGenerator.h"
#include "effectInterface.h"

enum DrawableObjectType
{
	DOT_Quad,
	DOT_Sphere,
	DOT_Box,
	DOT_Cylinder,
	DOT_Disc,

	DOT_Max
};

enum EffectType
{
	ET_FlickeringIllusion,
	ET_SimpleTriangle,
	ET_SimplePhong,
	ET_SimplePhongRaw,
	ET_SimplePhongInstanced,
	ET_SimpleTriangleInstanced,
	ET_SimpleTexture,
	ET_SimpleDraw,
	ET_SimpleCull,
	ET_SimpleDrawCsVs,
	ET_ColoredTriangleGrid,

	ET_Max
};

//////////////////////////////////////////////////////////////////////////
// Constant buffers for each effect type

struct CBFlickeringIllusion0
{
	RedMatrix4x4	m_worldToScreen;
	Float			m_time;
	Float			PADDING[3];
};
struct CBFlickeringIllusion1
{
	RedVector4		m_objectPosition;
};

struct CBSimpleTriangle
{
	RedVector4		m_colors[3];
};

struct CBSimplePhongVS0
{
	RedMatrix4x4	m_worldToView;
	RedMatrix4x4	m_viewToScreen;
	RedVector4		m_lightPos;
};
struct CBSimplePhongVS1
{
	RedMatrix4x4	m_localToWorld;
};
struct CBSimplePhongPS0
{
	RedVector4		m_surfaceColor;
};

struct CBSimplePhongRaw
{
	RedMatrix4x4	m_worldToView;
	RedMatrix4x4	m_viewToScreen;
	RedVector4		m_lightPos;
	RedMatrix4x4	m_localToWorld;
	RedVector4		m_surfaceColor;
};

struct CBSimplePhongInstanced
{
	RedMatrix4x4	m_worldToView;
	RedMatrix4x4	m_viewToScreen;
	RedVector4		m_lightPos;
};

typedef CBSimpleTriangle CBSimpleTriangleInstanced;

struct CBSimpleTexture
{
	RedMatrix4x4	m_worldToView;
	RedMatrix4x4	m_viewToScreen;
	RedMatrix4x4	m_localToWorld;
	Float			m_uvScale;
	Float			PADDING[3];
};

struct CBSimpleDraw
{
	RedMatrix4x4	m_localToWorld;
	RedMatrix4x4	m_worldToView;
	RedMatrix4x4	m_viewToScreen;
};


struct CBSimpleCull
{
	RedMatrix4x4	m_localToWorld;
	RedMatrix4x4	m_worldToView;
	RedMatrix4x4	m_viewToScreen;
};
struct CBSimpleDrawCsVs
{
	RedMatrix4x4	m_localToWorld;
	RedMatrix4x4	m_worldToView;
	RedMatrix4x4	m_viewToScreen;
	RedVector4		m_color;
};

struct CBColoredTriangleGrid
{
	RedVector4		m_colors[ 64 ];
};


//////////////////////////////////////////////////////////////////////////

struct SDrawableObject
{
	Uint16* Indices;
	SCENE_VERTEX* Vertices;
	SCENE_VERTEX* VerticesNonIndexed;
	GpuApi::BufferRef VertexBufferNonIndexed;
	GpuApi::BufferRef VertexBufferIndexed;
	GpuApi::BufferRef IndexBuffer;
	Uint32 NumVertices;
	Uint32 NumIndices;
	RedMatrix4x4 LocalMatrix;
	RedVector4 Color;
};

class CTestUtils
{
public:
	CTestUtils( );
	~CTestUtils();

	Bool Initialize( );

	RED_INLINE const TDynArray< SDrawableObject >& GetObjects() { return m_objects; }
	RED_INLINE IEffect* GetEffect( EffectType type ) { return m_effects[ type ]; }

protected:
	TDynArray< SDrawableObject > m_objects;
	TDynArray< IEffect* > m_effects;

private:
	Bool InitializeDrawableObjects();
	Bool InitializeEffects();

};

#endif