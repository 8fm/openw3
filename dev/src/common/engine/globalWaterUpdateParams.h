/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "renderObject.h"

#define WATER_RESOLUTION 512 //if changed, change also in globalOcean.fx

#ifdef RED_PLATFORM_CONSOLE
#define DYNAMIC_WATER_RESOLUTION			512.0f
#else
#define DYNAMIC_WATER_RESOLUTION			1024.0f
#endif

#define WATER_DEFAULT_LEVEL ( 0.0f )
#define WATER_DEFAULT_NON_INIT_LEVEL ( -10000.0f )

#define WATER_LOCAL_RESOLUTION	128
#define NUM_LAKES_MAX			44
#define NUM_DISPLACEMENTS_MAX	16 // means 16 transforms (8boats)

class CGlobalWaterHeightmap : public IRenderObject, Red::System::NonCopyable
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_GlobalWater );

private:
	mutable Red::Threads::CMutex	m_mutex;

	Uint16*							m_heightmaps;			//!< Two heightmap buffers, for double-buffering

	Uint32							m_width;				//!< Width of heightmap
	Uint32							m_height;				//!< Height of heightmap

	Bool							m_flipped;				//!< Whether the readback heightmap is the first or second buffer


public:
	CGlobalWaterHeightmap();
	~CGlobalWaterHeightmap();

	// Should only be called once by render proxy.
	void Initialize( Uint32 width, Uint32 height );
	Bool IsInit() const;

	// Safe to call from render.
	void* GetWritePointer();
	void FlipBuffers();

	// Safe to call from main.
	// u,v are wrapped around to the range [0,1]
	Float GetHeight( Float u, Float v ) const;
};


class CGlobalWaterUpdateParams: public IRenderObject, Red::System::NonCopyable
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_GlobalWater );

public:

	static const Vector DefaultAmplitudeScale;
	static const Vector DefaultUVScale;

	CGlobalWaterHeightmap*			m_heightmap;

	Vector							m_phillipsData;

	Float							m_amplitudeRatio;

	Vector							m_uvScale;
	Vector							m_amplitudeScale;

	Float							m_rainIntensity;

	TDynArray< SDynamicCollider >	m_impulses;

	TDynArray< SDynamicCollider >	m_displCollisions;

	Vector							m_camera;

	Bool							m_shouldRenderUnderwater;

	Bool							m_shouldRenderWater;

	Float							m_gameTime;

	CGlobalWaterUpdateParams( CGlobalWaterHeightmap* heightmap );
	~CGlobalWaterUpdateParams();
};



class CLocalWaterShapesParams : public IRenderObject, Red::System::NonCopyable
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_GlobalWater );

private:
	void							AllocateBuffers( Uint32 size );
	void							FreeBuffers();

public:
	Float*							m_shapesBuffer;			//!< each shape is WATER_LOCAL_RESOLUTION x WATER_LOCAL_RESOLUTION
	Float*							m_matrices;/*WHY IS IT EVEN CALLED MATRICES?*/				//!< 4-element transforms
	Uint32							m_numShapes;			//!< Number of shapes held in shapes and matrices buffers
	Uint32							m_allocatedShapes;		//!< Size of allocated buffers. In-game it equals to m_numShapes
	Float							m_waterMaxLevel;		//!< Maximum value of warer level set by local water shapes
	Float							m_shapeWaterLevel_min;
	Float							m_shapeWaterLevel_max;

	CLocalWaterShapesParams( Uint32 numShapes );
	~CLocalWaterShapesParams();

	void							PushNewShape();
};
