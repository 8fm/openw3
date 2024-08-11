/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../engine/globalWaterUpdateParams.h"
#include "renderSimWaterFFT.h"
#include "renderSimDynamicWater.h"
#include "../engine/renderProxy.h"

class CRenderTextureArray;
class CClipMapShaderData;
struct SCollisionsDataConstants;


class quad_projector
{
	class linear
	{
	public:
		inline linear( const Vector & a, const Vector & b )
		{
			Vector del = b-a;
			norm = Vector(-del.Y,del.X,0.0f);
			d = -1.0f*Vector::Dot2(norm,a);
		}
		inline float potential( const Vector & p )
		{
			return Vector::Dot2(p,norm)+d;
		}
		Vector norm;
		float d;
	};
public:
	quad_projector();
	~quad_projector();

	bool Project( const Vector & pos, const Vector & r1, const Vector & r2, const Vector & r3, Float farPlane, Float Fov, Float Ratio, Float MinAmp, Float MaxAmp, Float waterMaxLevel );
	bool ProjectFull( const Vector & pos, const Vector & r1, const Vector & r2, const Vector & r3, Float farPlane, Float Fov, Float Ratio, Float MinAmp, Float MaxAmp );
	bool ProjectScreenSpace( const Vector & pos, const Vector & r1, const Vector & r2, const Vector & r3, Float nearPlane, Float Fov, Float Ratio, Vector* outResult );
	bool ProjectWorldSpace( const Vector & pos, const Vector & r1, const Vector & r2, const Vector & r3, Float nearPlane, Float Fov, Float Ratio, Vector* outResult );

	RED_INLINE Vector* GetVectors(){ return (Vector*)&m_projected; }
private:
	Float LinePlaneIntersection( const Vector & p1, const Vector & p2, Float z );
	void Optimize3();
	void Optimize5();
	Vector LinesIntersection( const Vector & p0, const Vector & p1, const Vector & r0, const Vector & r1 );

	TDynArray<Vector>					m_arr; // this are frustum projection points
	Vector								m_optimized[4]; // this is quad optimized (4 vertices)

	Vector								m_minimum[4]; // this is quad on minAmp surface
	Vector								m_maximum[4]; // this is quad on maxAmp surface (or camera surface)

	Vector								m_projected[4];

	Float								m_rad_half_deg;
};

////////////////////////////

class disc
{
public:
	disc( Float radius, Int32 numsegsU, Int32 numsegsV )
	{
		Int32 u;
		Int32 v;
		m_vertices.Reserve( numsegsU*(numsegsV+1) );
		m_quads.Reserve( numsegsU*(numsegsV));
		for( u=0; u<numsegsU; u++ )
		{
			Float ang = DEG2RAD(360.0f/Float(numsegsU));
			
			for( v=0; v<numsegsV+1;v++ )
			{
				Float r = (Float(v)/Float(numsegsV))*2.0f;
				Vector vert( cosf(ang*u)*radius*r*r, sinf(ang*u)*radius*r*r, 0.0f );
				m_vertices.PushBack(vert);
			}
		}
		for( u=0; u<numsegsU; u++ )
		{
			for( v=0; v<numsegsV;v++ )
			{
				Vector quad( Float(GetInd(u,v,numsegsU,numsegsV)),Float(GetInd(u,v+1,numsegsU,numsegsV)),Float(GetInd(u+1,v,numsegsU,numsegsV)),Float(GetInd(u+1,v+1,numsegsU,numsegsV)) );
				m_quads.PushBack( quad );
			}
		}
		m_numverts = m_vertices.Size();
		m_numquads = m_quads.Size();

		m_bufferSize = m_numverts * sizeof( GpuApi::SystemVertex_Pos );
		m_indicesBufferSize = m_numquads * 4 * sizeof( Uint16 );

		m_vertexBufferRef = GpuApi::CreateBuffer( m_bufferSize, GpuApi::BCC_Vertex, GpuApi::BUT_Dynamic, GpuApi::BAF_CPUWrite );
		m_indicesBufferRef = GpuApi::CreateBuffer( m_indicesBufferSize, GpuApi::BCC_Index16Bit, GpuApi::BUT_Dynamic, GpuApi::BAF_CPUWrite );

		void* verts_ptr = GpuApi::LockBuffer( m_vertexBufferRef, GpuApi::BLF_Discard, 0, m_bufferSize );		
		GpuApi::SystemVertex_Pos* verts = static_cast<GpuApi::SystemVertex_Pos*>(verts_ptr);

		for( Int32 v=0;v<m_numverts;v++)
		{
			verts[v].x = m_vertices[v].X;
			verts[v].y = m_vertices[v].Y;
			verts[v].z = m_vertices[v].Z;
		}

		GpuApi::UnlockBuffer( m_vertexBufferRef );	

		void* temp_ptr_ind = GpuApi::LockBuffer( m_indicesBufferRef, GpuApi::BLF_Discard, 0, m_indicesBufferSize );
		Uint16* indices = static_cast<Uint16*>(temp_ptr_ind);

		for( Int32 v=0;v<m_numquads;v++)
		{
			indices[(v*4)+0] = Uint16(m_quads[v].X);
			indices[(v*4)+1] = Uint16(m_quads[v].Y);
			indices[(v*4)+2] = Uint16(m_quads[v].Z);
			indices[(v*4)+3] = Uint16(m_quads[v].W);
		}

		GpuApi::UnlockBuffer( m_indicesBufferRef );

		GpuApi::SetBufferDebugPath( m_vertexBufferRef, "WATER_DISC_VB" );
		GpuApi::SetBufferDebugPath( m_indicesBufferRef, "WATER_DISC_IB" );
	}
	~disc()
	{
		GpuApi::SafeRelease(m_vertexBufferRef);
		GpuApi::SafeRelease(m_indicesBufferRef);
	}
	Int32 GetInd( Int32 u, Int32 v, Int32 nu, Int32 nv )
	{
		Int32 uu = u>=nu ? 0 : u; 
		Int32 vv = v>=nv+1 ? 0 : v; 
		return (nv+1)*uu+vv;
	}
	Int32 m_numverts;
	Int32 m_numquads;
	TDynArray<Vector> m_vertices;
	TDynArray<Vector> m_quads; //x,y,z,w

	GpuApi::BufferRef					m_vertexBufferRef;
	GpuApi::BufferRef					m_indicesBufferRef;

	Int32								m_bufferSize;
	Int32								m_indicesBufferSize;
};

class plane
{
public:
	plane() { }

	// Shouldn't do this in the constructor, because the lock/unlock need to happen on render thread (plane is created in render
	// proxy constructor, on main thread)
	void init( Int32 numsegsU, Int32 numsegsV )
	{
		Int32 u;
		Int32 v;
		m_vertices.Reserve( (numsegsU+1)*(numsegsV+1) );
		m_quads.Reserve( numsegsU*numsegsV);

		for( u=0; u<numsegsU+1; u++ )
		{
			for( v=0; v<numsegsV+1;v++ )
			{
				Vector vert( Float(u)/Float(numsegsU), Float(v)/Float(numsegsV), 0.0f );
				m_vertices.PushBack(vert);
			}
		}
		for( u=0; u<numsegsU; u++ )
		{
			for( v=0; v<numsegsV;v++ )
			{
				Vector quad( Float(GetInd(u,v,numsegsU,numsegsV)),Float(GetInd(u+1,v,numsegsU,numsegsV)),Float(GetInd(u,v+1,numsegsU,numsegsV)),Float(GetInd(u+1,v+1,numsegsU,numsegsV)) );
				m_quads.PushBack( quad );
			}
		}
		m_numverts = m_vertices.Size();
		m_numquads = m_quads.Size();

		m_bufferSize = m_numverts * sizeof( GpuApi::SystemVertex_Water );
		m_indicesBufferSize = m_numquads * 4 * sizeof( Uint16 );

		m_vertexBufferRef = GpuApi::CreateBuffer( m_bufferSize, GpuApi::BCC_Vertex, GpuApi::BUT_Dynamic, GpuApi::BAF_CPUWrite );
		m_indicesBufferRef = GpuApi::CreateBuffer( m_indicesBufferSize, GpuApi::BCC_Index16Bit, GpuApi::BUT_Dynamic, GpuApi::BAF_CPUWrite );

		if( !m_vertexBufferRef.isNull() && !m_indicesBufferRef.isNull() )
		{
			void* verts_ptr = GpuApi::LockBuffer( m_vertexBufferRef, GpuApi::BLF_Discard, 0, m_bufferSize );
			GpuApi::SystemVertex_Water* verts = static_cast<GpuApi::SystemVertex_Water*>(verts_ptr);

			for( Int32 v=0;v<m_numverts;v++)
			{
				verts->m_position[0] = m_vertices[v].X;
				verts->m_position[1] = m_vertices[v].Y;
				verts->m_position[2] = m_vertices[v].Z;

				++verts;
			}

			GpuApi::UnlockBuffer( m_vertexBufferRef );	

			void* temp_ptr_ind = GpuApi::LockBuffer( m_indicesBufferRef, GpuApi::BLF_Discard, 0, m_indicesBufferSize );
			Uint16* indices = static_cast<Uint16*>(temp_ptr_ind);

			for( Int32 v=0;v<m_numquads;v++)
			{
				indices[(v*4)+0] = Uint16(m_quads[v].X);
				indices[(v*4)+1] = Uint16(m_quads[v].Y);
				indices[(v*4)+2] = Uint16(m_quads[v].Z);
				indices[(v*4)+3] = Uint16(m_quads[v].W);
			}

			GpuApi::UnlockBuffer( m_indicesBufferRef );

			GpuApi::SetBufferDebugPath( m_vertexBufferRef, "WATER_PLANE_VB" );
			GpuApi::SetBufferDebugPath( m_indicesBufferRef, "WATER_PLANE_IB" );
		}
		else
		{
			RED_ASSERT( TXT("Locking vertex buffer for water[plane] failed!") );
		}
		
	}
	~plane()
	{
		GpuApi::SafeRelease(m_vertexBufferRef);
		GpuApi::SafeRelease(m_indicesBufferRef);
	}
	void Expand( const Vector & a, const Vector & b, const Vector & c, const Vector & d )
	{
		if( !m_vertexBufferRef.isNull() && !m_indicesBufferRef.isNull() )
		{
			void* verts_ptr = GpuApi::LockBuffer( m_vertexBufferRef, GpuApi::BLF_Discard, 0, m_bufferSize );

			GpuApi::SystemVertex_Water* verts = static_cast<GpuApi::SystemVertex_Water*>(verts_ptr);

			Vector uax1 = b-a;
			Vector uax2 = c-d;
			for( Int32 v=0;v<m_numverts;v++)
			{
				Float uu = m_vertices[v].X;
				Float vv = m_vertices[v].Y;

				Vector p0 = a + uax1*uu;
				Vector p1 = d + uax2*uu;
				Vector r0 = p0 + (p1-p0)*vv;

				verts->m_position[0] = r0.X;
				verts->m_position[1] = r0.Y;
				verts->m_position[2] = r0.Z;

				++verts;
			}
			GpuApi::UnlockBuffer( m_vertexBufferRef );	
		}
		else
		{
			RED_ASSERT( TXT("Locking vertex buffer for water[plane] failed!") );
		}
	}
	void Expand_sq( const Vector & a, const Vector & b, const Vector & c, const Vector & d )
	{
		if( !m_vertexBufferRef.isNull() && !m_indicesBufferRef.isNull() )
		{
			void* verts_ptr = GpuApi::LockBuffer( m_vertexBufferRef, GpuApi::BLF_Discard, 0, m_bufferSize );

			GpuApi::SystemVertex_Water* verts = static_cast<GpuApi::SystemVertex_Water*>(verts_ptr);

			Vector uax1 = b-a;
			Vector uax2 = c-d;
			for( Int32 v=0;v<m_numverts;v++)
			{
				Float uu = m_vertices[v].X;
				Float vv = m_vertices[v].Y;

				Vector p0 = a + uax1*uu;
				Vector p1 = d + uax2*uu;
				Vector r0 = p0 + (p1-p0)*vv;

				verts->m_position[0] = r0.X;
				verts->m_position[1] = r0.Y;
				verts->m_position[2] = r0.Z;

				++verts;

				/*
				
				Float uuu = (uu-0.5f)*2.0f;
				Float vvv = (vv-0.5f)*2.0f;
								
				Float absU = MAbs(uuu);
				Float absV = MAbs(vvv);

				if(  absU >= 1.0f ) verts[v].y += MSign(uuu) * 3500.0f;
				else if(  absU > 0.97f ) verts[v].y += MSign(uuu) * 800.0f;
				else if(  absU > 0.95f ) verts[v].y += MSign(uuu) * 400.0f;

				if(  absV >= 1.0f ) verts[v].x += MSign(vvv) * 3500.0f;
				else if(  absV > 0.97f ) verts[v].x += MSign(vvv) * 800.0f;
				else if(  absV > 0.95f ) verts[v].x += MSign(vvv) * 400.0f;

				*/
			}

			GpuApi::UnlockBuffer( m_vertexBufferRef );	
		}
		else
		{
			RED_ASSERT( TXT("Locking vertex buffer for water[plane] failed!") );
		}
	}
	Int32 GetInd( Int32 u, Int32 v, Int32 nu, Int32 nv )
	{
		Int32 uu = u>=nu+1 ? 0 : u; 
		Int32 vv = v>=nv+1 ? 0 : v; 
		return (nv+1)*uu+vv;
	}
	Int32								m_numverts;
	Int32								m_numquads;
	TDynArray<Vector>					m_vertices;
	TDynArray<Vector>					m_quads; //x,y,z,w

	GpuApi::BufferRef					m_vertexBufferRef;
	GpuApi::BufferRef					m_indicesBufferRef;

	Int32								m_bufferSize;
	Int32								m_indicesBufferSize;
};



/// Water proxy
class CRenderProxy_Water : public IRenderProxy
{
	struct SWaterConstants
	{
		struct SGlobalWater
		{
			Vector	uvScale;
			Vector	amplitudeScale;
			Float	tessFactor;
			Float	dynamicWaterResolution;
			Float	dynamicWaterResolutionInv;
			Float	padding_unused[1];
			Vector	choppyFactors;			
			
		} GlobalWater;

		struct SSimulationCamera
		{
			Vector	position;
			Float	nearPlane;
			Float	farPlane;
			Float	tanFov;
			Float	tanFov_x_ratio;
		} SimulationCamera;

		struct SLakes
		{
			Uint32	numLakes;
			Float	padding_unused[3];
			Vector	offsets[NUM_LAKES_MAX];
		} Lakes;

		struct SDisplacements	//Boats
		{
			Uint32	numDispl;
			Float	padding_unused[3];
			Vector	displacementData[ NUM_DISPLACEMENTS_MAX * 4 ]; // 1 boat = 1 matrix, but somebody decided it's a good idea to hide that fact and store it in rows. Hence the *4.
		} Displacements;
	};

private:
	TDynArray<Vector>					m_arr;
	GpuApi::BufferRef					m_instanceData;	//!< 
	Int32								m_numinstances; //!< sum of all the grid instances

	CGlobalWaterHeightmap*				m_heightmap;

	CRenderTextureArray*				m_controlTextureArray;		
	GpuApi::TextureRef					m_localShapesTextureArray;	
	GpuApi::TextureRef					m_intersectionTexture;

	GpuApi::BufferRef					m_constantBuffer;

	Int32								m_numLakes;

	GpuApi::TextureRef					m_furierRef;	
	GpuApi::TextureRef					m_dynamicWater;

	CRenderSimWaterFFT					m_simulator;
	CRenderSimDynamicWater				m_dynamicsimulator;
	Int32								m_numBoats;

	Vector								m_philipsParameters;
	Vector								m_amplitudeScale;
	Vector								m_uvScale;
	Vector								m_lakes[ NUM_LAKES_MAX ];
	Matrix								m_displacements[ NUM_DISPLACEMENTS_MAX];

	//plane								m_fullplane;
	plane								m_plane;	
	quad_projector						m_qp;
	Float								m_gameTime;
	Vector								m_cameraForward;

	Bool								m_shouldRenderUnderwater;
	Bool								m_shouldClearUnderwater;
	Bool								m_shouldRenderWater;
	Bool								m_visible;
	
	Bool								m_shouldSimulateFFT;
	Bool								m_shouldReadBackCPUData;
	Uint32								m_numSkippedFFTFrames;
	Float								m_currentClosestTileDistanceSqr;
	void								PerformFFTVisibilityTest();			//!< Skip FFT update when not needed
	Float								m_localWaterShapesMaxHeight;
	Float								m_shapeWaterLevel_min;  //!< minimum Z of bbox frustun test in this world
	Float								m_shapeWaterLevel_max;  //!< maximum Z of bbox frustun test in this world

public:
	CRenderProxy_Water();
	virtual ~CRenderProxy_Water();

	RED_INLINE GpuApi::TextureRef		GetUnderwaterIntersectionTexture() const	{ return m_intersectionTexture; };
	RED_INLINE GpuApi::TextureRef		GetFurierTexture() const					{ return m_furierRef; };
	RED_INLINE CRenderTextureArray*		GetControlTexture() const					{ return m_controlTextureArray; };
	RED_INLINE CRenderSimWaterFFT*		GetWaterSimulator()							{ return &m_simulator; }
	RED_INLINE Bool						ShouldRenderUnderwater() const				{ return m_shouldRenderUnderwater; };

	RED_INLINE virtual void				SetVisible( Bool visible )					{ m_visible = visible; }
	RED_INLINE Bool						IsVisible()									{ return m_visible; }

	// Could inherit IRenderProxyBase, and use the existing attach/detach interface, but don't need anything else
	// that comes with IRenderProxyBase. Don't currently have any need for a Detach, so it is left out.
	void								AttachToScene();
	
	// calls from render commands
	void								UpdateControlTexture( CRenderTextureArray* newControlTextureArray );	
	void								UpdatePhilipsParameters( CGlobalWaterUpdateParams* waterParams );
	void								AddLocalShapes( const CLocalWaterShapesParams* shapesParams );

	// Simulate the water surface
	void								Simulate( GpuApi::TextureRef terrainHeightMapTextureArray, const CRenderFrameInfo& frameInfo );

	// Render the water surface using the simulated heightmap
	void								Render( const CRenderSceneEx* renderScene, const class RenderingContext& context, const CRenderFrameInfo& frameInfo );
	void								Render( const CRenderSceneEx* renderScene, const class RenderingContext& context, const CRenderFrameInfo& frameInfo, GpuApi::TextureRef terrainHeightMapTextureArray, const CClipMapShaderData& clipMapData );

private:
	void								BindWaterConstantsBuffers( GpuApi::eShaderType shaderTarget );
	void								UnbindWaterConstantsBuffers( GpuApi::eShaderType shaderTarget );
	void								UpdateConstantBuffer( const CRenderFrameInfo& frameInfo );

	void								BindShadersAndDraw( const CRenderFrameInfo& frameInfo, GpuApi::TextureRef terrainHeightMapTextureArray );
	void								BindShadersAndDrawUnderwater( const CRenderFrameInfo& frameInfo );

	// terrain height map is a null texture when there is no terrain
	void								SimulateDynamics( GpuApi::TextureRef terrainHeightMapTextureArray, const CRenderFrameInfo& frameInfo );

	void								UpdateCPUData();

	void								Project( const CRenderFrameInfo& frameInfo, bool full );
};
