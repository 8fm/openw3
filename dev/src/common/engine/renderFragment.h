/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "renderVertices.h"
#include "hitProxyId.h"
#include "renderCamera.h"
#include "renderResource.h"
#include "renderFrame.h"

class IMaterial;
class IMaterialParameterSupplier;
class BrushRenderVertex;
class CRenderFrame;
class IRenderProxy;
enum ETransparencySortGroup : CEnum::TValueType;
enum ERenderingSortGroup : CEnum::TValueType;
enum EMaterialDebugMode : Int32;

struct CMaterialParamUpdate
{
	Uint8				*m_offsets;				//!< offsets to write to in parameter table
	Uint8				*m_sizes;				//!< sizes of fix-ups
	Uint8				*m_data;				//!< update data
	Uint8				m_currOffset;			//!< offset to first free entry in data table
	Uint8				m_numEntries;			//!< number of updates

	CMaterialParamUpdate()
		: m_offsets( NULL )
		, m_sizes( NULL )
		, m_data( NULL )
		, m_currOffset( 0 )
		, m_numEntries( 0 )
	{
	}

	void Init( Uint8* offsetsMemory, Uint8* sizesMemory, Uint8* dataMemory )
	{
		m_offsets = offsetsMemory;
		m_sizes = sizesMemory;
		m_data = dataMemory;
		m_currOffset = 0;
		m_numEntries = 0;
	}

	void Apply( Uint8* data )
	{
		Uint32 currSourceOffset = 0;
		for( Uint32 i=0; i<m_numEntries; ++i )
		{
			Red::System::MemoryCopy( data + m_offsets[ i ], m_data + currSourceOffset, m_sizes[ i ] );
			currSourceOffset += m_sizes[ i ];
		}
	}
};

/// WARNING WARNING WARNING WARNING WARNING WARNING
/// WARNING WARNING WARNING WARNING WARNING WARNING
/// WARNING WARNING WARNING WARNING WARNING WARNING
/// WARNING WARNING WARNING WARNING WARNING WARNING

// WHENEVER THE ERenderingPass IS CHANGED YOU _MUST_ RECREATE COOKER CACHE AND SHADER CACHE !!!!!!
// WHENEVER THE ERenderingPass IS CHANGED YOU _MUST_ RECREATE COOKER CACHE AND SHADER CACHE !!!!!!
// WHENEVER THE ERenderingPass IS CHANGED YOU _MUST_ RECREATE COOKER CACHE AND SHADER CACHE !!!!!!
// WHENEVER THE ERenderingPass IS CHANGED YOU _MUST_ RECREATE COOKER CACHE AND SHADER CACHE !!!!!!

/// Rendering pass
enum ERenderingPass : Int32
{
	RP_HitProxies,
	RP_NoLighting,
	RP_ShadowDepthSolid,
	RP_ShadowDepthMasked,
	RP_Emissive,
	RP_GBuffer,
	RP_RefractionDelta,
	RP_ReflectionMask,
	RP_ForwardLightingSolid,
	RP_ForwardLightingTransparent,
	RP_HiResShadowMask,

	RP_Max
};

enum ELightChannelFilterType
{
	LCF_NoFilter,
	LCF_AllBitsSet
};

/// Is this a shadow group ?
extern Bool IsShadowRenderPass( ERenderingPass pass );

/// Rendering context
class RenderingContext
{
protected:
	CRenderCamera			m_camera;					// Current camera, taken from CRenderFrame

public:
	ERenderingPass			m_pass;						// Rendering pass
	Bool					m_terrainToolStampVisible;	// The flag is true, when TerrainTool is active. In this case, special shader is used.
	Bool					m_grassMaskPaintMode;		// The flag is true, when vegetation tool is active and Painting grass mask is enabled. In this case, special shader is used.
	Bool					m_forceNoDissolves;			// Is this refractive background group?
	Bool					m_forceNoParticles;
	Bool					m_forceNoSwarms;
	EMaterialDebugMode		m_materialDebugMode;		// Material debug mode
	
	Uint32					m_lightChannelFilterMask;	// Mask used when m_lightChannelFilter is not LCF_NoFilter.
	ELightChannelFilterType	m_lightChannelFilter;		// How drawables should be filtered.
	Uint32					m_lightChannelForcedMask;	// Light channels which should be additionally enabled

	CHitProxyID				m_constantHitProxyID;		// Hit proxy ID to use when m_useConstantHitProxyID is true.
	Bool					m_useConstantHitProxyID;	// Whether a RP_HitProxies pass should use a constant hit proxy ID, or per-renderproxy ID.

public:

	RenderingContext();
	RenderingContext( const CRenderCamera& camera );
	RenderingContext( const RenderingContext& context );

	// Test lightChannels value against light channel filter. Return true if drawing should be done, false to skip.
	Bool CheckLightChannels( Uint32 lightChannels ) const
	{
		switch ( m_lightChannelFilter )
		{
		case LCF_NoFilter:		return true;
		case LCF_AllBitsSet:	return ( lightChannels & m_lightChannelFilterMask ) == m_lightChannelFilterMask;

		default:
			HALT( "Invalid light channel filter: %d", m_lightChannelFilter );
			return true;
		}
	}

	RED_INLINE const CRenderCamera& GetCamera() const
	{
		return m_camera;
	}
};

/// Type of rendering fragment
enum ERenderFragmentType
{
	RFT_MeshChunkStatic,					// Static mesh chunk, batched
	RFT_MeshChunkSkinned,					// Skinned mesh chunk, not batched
	RFT_MeshChunkLeafCard,					// Leaf card mesh chunk, batched
	RFT_MeshChunkFoliage,					// Foliage mesh chunk, batched
	RFT_TerrainChunk,						// Terrain chunk, batched
	RFT_Particle,							// Particles (all types), batches
	RFT_DebugLineList,						// DEBUG, list of lines
	RFT_DebugIndexedLineList,				// DEBUG, indexed list of lines
	RFT_DebugRectangle,						// DEBUG, rectangle
	RFT_DebugSprite,						// DEBUG, sprite
	RFT_DebugEnvProbe,						// DEBUG, env probe
	RFT_DebugPolyList,						// DEBUG, list of polygons
	RFT_DebugMesh,							// DEBUG, mesh
	RFT_DebugApex,							// DEBUG, apex geometry
	RFT_Text,								// DEBUG, on screen text
	RFT_BrushFace,							// Face of a brush ( DEPRECATED )
};

/// Parameter mask for rendering fragments
enum ERenderFragmentMaterialParameter
{
	RFMP_ColorShiftMatrices				= FLAG( 0 ),
	RFMP_CustomMaterialParameter0		= FLAG( 1 ),
	RFMP_CustomMaterialParameter1		= FLAG( 2 ),
	RFMP_GameplayParameters				= FLAG( 3 ),
	RFMP_FoliageColor					= FLAG( 4 ),
};

/// Fragment for rendering
class IRenderFragment
{
protected:
	Uint8					m_flags;			//!< Fragment flags
	ERenderingSortGroup		m_sortGroup;		//!< Assigned sort group
	CRenderFrame*			m_frame;			//!< Owner
	ERenderFragmentType		m_type;				//!< Type of fragment
	Uint32					m_numPrimitives;	//!< Number of primitives to draw in this fragment
	Matrix					m_localToWorld;		//!< Local space matrix for this fragment
	IRenderFragment*		m_baseLinkNext;		//!< Link to next fragment in the frame fragment list ( the base list )

public:
	//! Get the frame this fragment belongs to
	RED_INLINE CRenderFrame* GetFrame() const { return m_frame; }

	//! Get type of fragment
	RED_INLINE ERenderFragmentType GetType() const { return m_type; }

	//! Get sort group assigned to this fragment
	RED_INLINE ERenderingSortGroup GetSortGroup() const { return m_sortGroup; }

	//! Get the number of primitives in this fragment
	RED_INLINE Uint32 GetNumPrimitives() const { return m_numPrimitives; }

	//! Get the local to world matrix for rendering this fragment
	RED_INLINE const Matrix& GetLocalToWorld() const { return m_localToWorld; }

	//! Get next base fragment
	RED_INLINE IRenderFragment* GetNextBaseFragment() const { return m_baseLinkNext; }

public:
	IRenderFragment( CRenderFrame* frame, ERenderFragmentType type, Uint32 numPrimitives, const Matrix& localToWorld, ERenderingSortGroup sortGroup );
	virtual ~IRenderFragment() {}

	//! Add fragment to frame
	void AddFragmentToFrame( CRenderFrame* frame, ERenderingSortGroup sortGroup );
	
	//! Draw the fragment
	virtual void Draw( const RenderingContext& context )=0;
};

/// Render fragment for drawing debug lines
class CRenderFragmentDebugLineList : public IRenderFragment
{
protected:
	DebugVertex*	m_points;		//!< Points

public:
	CRenderFragmentDebugLineList( CRenderFrame* frame, const Matrix& localToWorld, const DebugVertex* points, Uint32 numPoints, ERenderingSortGroup sortGroup );

	//! Draw the fragment
	virtual void Draw( const RenderingContext& context );
};

/// Render fragment for drawing debug lines
class CRenderFragmentDebugIndexedLineList : public IRenderFragment
{
protected:
	DebugVertex*			m_points;		//!< Points
	Uint16*					m_indices;		//!< Indices
	Uint32					m_numPoints;	//!< Number of points
	Uint32					m_numIndices;	//!< Number of indices

public:
	CRenderFragmentDebugIndexedLineList( CRenderFrame* frame, const Matrix& localToWorld, const DebugVertex* points, const Uint32 numPoints, const Uint16* indices, const Uint32 numIndices, ERenderingSortGroup sortGroup );

	//! Draw the fragment
	virtual void Draw( const RenderingContext& context );
};

/// Render fragment for drawing debug lines
class CRenderFragmentDebugPolyList : public IRenderFragment
{
protected:
	DebugVertex*		m_points;		//!< Points
	Uint16*				m_indices;		//!< Indices
	Uint32				m_numPoints;	//!< Number of points
	Uint32				m_numIndices;	//!< Number of indices

public:
	CRenderFragmentDebugPolyList( CRenderFrame* frame, const Matrix& localToWorld, const DebugVertex* points, Uint32 numPoints, const Uint16* indices, Uint32 numIndices, ERenderingSortGroup sortGroup, Bool copyData=true );

	//! Draw the fragment
	virtual void Draw( const RenderingContext& context );
};

/// Render fragment for drawing debug rectangles
class CRenderFragmentDebugRectangle : public IRenderFragment
{
protected:
	Matrix			m_translationMatrix;
	Float			m_width;
	Float			m_height;
	Float			m_shiftY;
	Color			m_color;

public:
	CRenderFragmentDebugRectangle( CRenderFrame* frame, const Matrix& translationMatrix, Float width, Float height, Float shiftY, const Color &color );

	//! Draw the fragment
	virtual void Draw( const RenderingContext& context );
};

/// Render fragment for drawing debug rectangles
class CRenderFragmentDebugRectangle2D : public IRenderFragment
{
protected:
	Float	m_x;
	Float	m_y;
	Float	m_width;
	Float	m_height;
	Color	m_color;

public:
	CRenderFragmentDebugRectangle2D( CRenderFrame* frame, Int32 x, Int32 y, Int32 width, Int32 height, const Color &color );
	CRenderFragmentDebugRectangle2D( CRenderFrame* frame, Float x, Float y, Float width, Float height, const Color &color );

	//! Draw the fragment
	virtual void Draw( const RenderingContext& context );
};

class CRenderFragmentTexturedDebugRectangle : public IRenderFragment
{
protected:
	CRenderResourceSmartPtr		m_texture;				//!< Texture
	DebugVertexUV				m_vertices[4];			//!< Rect vertices
	bool						m_withAlpha;			//!< Draw alpha

public:
	CRenderFragmentTexturedDebugRectangle( CRenderFrame* frame, Float x, Float y, Float width, Float height, CBitmapTexture *texture, const Color &color, Bool withAlpha=true );

	//! Draw the fragment
	virtual void Draw( const RenderingContext& context );
};

class CRenderFragmentDynamicTexture : public IRenderFragment
{
protected:
	GpuApi::TextureRef	m_textureRef;	//!< Texture ref
	Float				m_x;			//!< Drawing position x
	Float				m_y;			//!< Drawing position y
	Float				m_width;		//!< Drawing width
	Float				m_height;		//!< Drawing height
	Float				m_colorMin;		//!<
	Float				m_colorMax;		//!<
	Uint32				m_mipIndex;		//!<
	Uint32				m_sliceIndex;	//!< Index in the texture array
	Vector				m_channelSelector;

public:
	CRenderFragmentDynamicTexture( CRenderFrame* frame, Float x, Float y, Float width, Float height, GpuApi::TextureRef textureRef, Uint32 mipIndex, Uint32 sliceIndex, Float colorMin, Float colorMax, Vector channelSelector );

	//! Draw the fragment
	virtual void Draw( const RenderingContext& context );
};

/// Render fragment for drawing sprite
class CRenderFragmentDebugSprite : public IRenderFragment
{
private:
	CRenderResourceSmartPtr		m_texture;		//!< Texture
	DebugVertexUV				m_vertices[4];	//!< Sprite vertices

public:
	CRenderFragmentDebugSprite( CRenderFrame* frame, const Vector& position, Float size, CBitmapTexture* texture, const Color& color, ERenderingSortGroup sortGroup  );

	//! Draw the fragment
	virtual void Draw( const RenderingContext& context );
};

/// Render fragment for drawing env probes
class CRenderFragmentDebugEnvProbe : public IRenderFragment
{
	enum EDrawMode
	{
		DRAWMODE_CubeAmbient,
		DRAWMODE_CubeReflection,
	};

private:
	CRenderResourceSmartPtr		m_renderResource;	//!< EnvProbe resource
	CHitProxyID					m_hitProxy;			//!< Hit proxy ID
	Float						m_gameTime;			//!< Game time

private:
	void DrawSingleCube( const RenderingContext& context, const Vector &posOffset, Float radius, EDrawMode drawMode, Int32 forcedMipIndex );

public:
	CRenderFragmentDebugEnvProbe( CRenderFrame* frame, Float gameTime, IRenderResource *renderResource, const Vector& position, Float radius, const CHitProxyID& hitProxy, ERenderingSortGroup sortGroup );

	//! Draw the fragment
	virtual void Draw( const RenderingContext& context );
};

/// Render fragment for drawing debug mesh
class CRenderFragmentDebugMesh : public IRenderFragment
{
private:
	CRenderResourceSmartPtr		m_mesh;			//!< Mesh
	CHitProxyID					m_hitProxy;		//!< Hit proxy ID

	Bool m_transparent;
	Bool m_wireframe;
	Bool m_useColor;

public:
	CRenderFragmentDebugMesh( CRenderFrame* frame, const Matrix& localToWorld, IRenderResource* debugMesh, Bool transparent = false, Bool wireframe = false );
	CRenderFragmentDebugMesh( CRenderFrame* frame, const Matrix& localToWorld, IRenderResource* debugMesh, ERenderingSortGroup sortGroup, Bool transparent = false, Bool wireframe = false );
	CRenderFragmentDebugMesh( CRenderFrame* frame, const Matrix& localToWorld, IRenderResource* debugMesh, const CHitProxyID& hitProxyID, Bool transparent = false, Bool wireframe = false );

	//! Draw the fragment
	virtual void Draw( const RenderingContext& context );
};

/// Render fragment for drawing wire debug mesh
class CRenderFragmentDebugWireMesh : public IRenderFragment
{
private:
	CRenderResourceSmartPtr		m_mesh;		//!< Texture	

public:
	CRenderFragmentDebugWireMesh( CRenderFrame* frame, const Matrix& localToWorld, IRenderResource* debugMesh );

	//! Draw the fragment
	virtual void Draw( const RenderingContext& context );
};

/// Render fragment for drawing wire debug mesh
class CRenderFragmentDebugWireSingleColorMesh : public IRenderFragment
{
private:
	CRenderResourceSmartPtr		m_mesh;		//!< Texture	

public:
	CRenderFragmentDebugWireSingleColorMesh( CRenderFrame* frame, const Matrix& localToWorld, IRenderResource* debugMesh );

	//! Draw the fragment
	virtual void Draw( const RenderingContext& context );
};


enum
{
	NEIGHBOR_SOUTH				= 0,
	NEIGHBOR_EAST				= 1,
	NEIGHBOR_NORTH				= 2,
	NEIGHBOR_WEST				= 3,
	LOD_LEVELS					= 6,						//!< Number of LOD levels LOD0 to LODn
	CHUNK_RESOLUTION			= 1 << ( LOD_LEVELS - 1 ),	//!< CHUNK_RESOLUTION^2 quads or (CHUNK_RESOLUTION+1)^2 vertices
};

/// Particle system rendering fragment
class CRenderFragmentParticleSystemChunk : public IRenderFragment
{
	friend class CDX9PartricleRenderer;

private:
	CRenderResourceSmartPtr		m_material;				//!< Material
	CRenderResourceSmartPtr		m_parameters;			//!< TODO: Are those parameters for material instance?
	CHitProxyID					m_hitProxyID;			//!< Hit proxy ID	
	const void*					m_particles;			//!< Buffer with particles
	CName						m_drawerType;			//!< Particle drawer for supporting different vertex factories
	ETransparencySortGroup		m_transparencySortGroup;//!< Transparency sort group

public:
	//! Get fragment material
	RED_INLINE IRenderResource* GetMaterial() const { return m_material.m_resource; }

	//! Get fragment material parameters
	RED_INLINE IRenderResource* GetParameters() const { return m_parameters.m_resource; }

	//! Get particles data
	RED_INLINE const void* GetParticles() const { return m_particles; }

	//! Get particle drawer
	RED_INLINE const CName& GetDrawerType() const { return m_drawerType; }

	//! Get transparency sort group
	RED_INLINE ETransparencySortGroup GetTransparencySortGroup() const { return m_transparencySortGroup; }

public:
	CRenderFragmentParticleSystemChunk( CRenderFrame* frame, Uint32 numParticles, const Matrix& localToWorld, const CHitProxyID& id, IMaterial* material, const void* particleRawData, Bool selected, CName drawerType, ETransparencySortGroup transparencySortGroup );

	//! Draw the fragment
	virtual void Draw( const RenderingContext& context );
};

/// Text on screen
class CRenderFragmentText : public IRenderFragment
{
	IRenderResource**	m_textures;			//!< Used textures
	Uint32				m_numTextures;		//!< Number of used textures
	DebugVertexUV**		m_vertices;			//!< Vertices for every texture
	Uint32*				m_numVertices;		//!< Number of vertices for every texture
	Uint32				m_color;			//!< Color of the text

public:
	CRenderFragmentText( CRenderFrame* frame, const Matrix& localToCanvas, class CFont &font, const Char* text, const Color &color = Color( 255, 255, 255 ) );
	~CRenderFragmentText();

	//! Draw the fragment
	virtual void Draw( const RenderingContext& context );
};

/// Lines on screen
class CRenderFragmentOnScreenLineList : public CRenderFragmentDebugLineList
{
public:
	CRenderFragmentOnScreenLineList( CRenderFrame* frame, const DebugVertex* points, Uint32 numPoints );
};

