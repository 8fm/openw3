/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "renderProxyWater.h"
#include "renderTextureArray.h"
#include "renderProxyTerrain.h"

#include "renderCollector.h"

#include "renderTerrainShadows.h"
#include "renderShaderPair.h"

#include "renderPostProcess.h"
#include "renderPostFx.h"
#include "renderRenderSurfaces.h"

#include "../engine/globalWater.h"
#include "../engine/dynamicCollisionCollector.h"
#include "../engine/dynamicColliderComponent.h"
#include "../engine/renderFragment.h"
#include "../engine/renderProxy.h"
#include "../engine/renderSettings.h"

/////////////////////////////////////////////////////////////////

#define WATER_INSTANCE_BUFFER_SIZE 64*2*sizeof(Float)

// sync with globalOcean.fx
#define WATER_GRID_ROW_SIZE 8
#define WATER_GRID_SIZE 100.0f
#define WATER_GRID_SIZE_HALF 50.0f
#define WATER_GRID_EXPANSION 1500.0f;
#define GRID_TESSLATION 16

/////////////////////////////////////////////////////////////////


quad_projector::quad_projector()
{
	//this is maximum intersection points of frustum (without nearplane) and plane
	m_arr.Reserve( 5 );

	// lazy cache
	m_rad_half_deg = ( M_PI/180.0f )*0.5f;
}

quad_projector::~quad_projector()
{

}

bool quad_projector::Project( const Vector & pos, const Vector & r1, const Vector & r2, const Vector & r3, Float farPlane, Float Fov, Float Ratio, Float MinAmp, Float MaxAmp, Float waterMaxLevel )
{
	Float tFov = tan(Fov*m_rad_half_deg);
	
	// scale the projection based on the shape offsets
	// hardcoded by now	
	const Float scaleProjX = 1.3f;
	
	Float scaleProjY = 1.3f;
	// Hacky way of dealing with holes between hires and lowres water. They were appearing when water was raised by local water shapes above 100 m, hence parameter override for that case
	if( waterMaxLevel > 100.0f )
	{
		scaleProjY = 6.0f;
	}

	Float y = scaleProjY * tFov;
	Float x = scaleProjX * (tFov*Ratio);
	
	Vector v1 = pos + (r3 + (r1*-x) + (r2*-y) )*farPlane;
	Vector v2 = pos + (r3 + (r1*x) + (r2*-y) )*farPlane;
	Vector v3 = pos + (r3 + (r1*x) + (r2*y))*farPlane;
	Vector v4 = pos + (r3 + (r1*-x) + (r2*y))*farPlane;

	// projecting frustum on z0 plane
	m_arr.ClearFast();
	Float e1 = LinePlaneIntersection(pos,v1, 0.0f);	if(e1>=0.0f && e1<=1.0f){m_arr.PushBack( Vector(-x,-y,0.0f,1.0f) );}
	Float e2 = LinePlaneIntersection(v1,v2, 0.0f);	if(e2>=0.0f && e2<=1.0f){m_arr.PushBack( Vector(-x,-y,0.0f,1.0f)*(1.0f-e2)+Vector(x,-y,0.0f,1.0f)*e2 );}
	Float e3 = LinePlaneIntersection(pos,v2, 0.0f);	if(e3>=0.0f && e3<=1.0f){m_arr.PushBack( Vector(x,-y,0.0f,1.0f) );}
	Float e4 = LinePlaneIntersection(v2,v3, 0.0f);	if(e4>=0.0f && e4<=1.0f){m_arr.PushBack( Vector(x,-y,0.0f,1.0f)*(1.0f-e4)+Vector(x,y,0.0f,1.0f)*e4 );}
	Float e5 = LinePlaneIntersection(pos,v3, 0.0f);	if(e5>=0.0f && e5<=1.0f){m_arr.PushBack( Vector(x,y,0.0f,1.0f) );}
	Float e6 = LinePlaneIntersection(v3,v4, 0.0f);	if(e6>=0.0f && e6<=1.0f){m_arr.PushBack( Vector(x,y,0.0f,1.0f)*(1.0f-e6)+Vector(-x,y,0.0f,1.0f)*e6 );}
	Float e7 = LinePlaneIntersection(pos,v4, 0.0f);	if(e7>=0.0f && e7<=1.0f){m_arr.PushBack( Vector(-x,y,0.0f,1.0f) );}
	Float e8 = LinePlaneIntersection(v4,v1, 0.0f);	if(e8>=0.0f && e8<=1.0f){m_arr.PushBack( Vector(-x,y,0.0f,1.0f)*(1.0f-e8)+Vector(-x,-y,0.0f,1.0f)*e8 );}

	const Int32 numpoints = m_arr.Size();

	// m_arr should contain 0, 3, 4 or 5 points
	// other cases and 0 should be skipped ( water is not visible )
	// the quad is projected at z0 plane
	// now it can be optimized ( reduced to 4 points )

	//optimization

	if( numpoints==0 ){ return false; } // not visible

	// only 3, 4, 5 means visible
	bool was_optimized = false;

	if( numpoints==4 ) // trivial case, just copy
	{
		m_optimized[0] = m_arr[0];
		m_optimized[1] = m_arr[1];
		m_optimized[2] = m_arr[2];
		m_optimized[3] = m_arr[3];
		was_optimized = true;
	}
	// more complex cases, when camera rolls this might happen
	if( numpoints==3 )
	{
		Optimize3();
		was_optimized = true;
	}
	if( numpoints==5 )
	{
		Optimize5();
		was_optimized = true;
	}

	if( !was_optimized ){ return false; } // numpoints != 3, 4 or 5 so quad is not visible
	// optimizations are done, there should be exactly 4 points in m_optimized

	//first step is to project m_optimized on minAmp surface (m_minimum) and maxAmp surface (m_maximum)

	// m_optimized are in camera space
	/*
	Float camZ = pos.Z;

	for(Int32 i=0; i<4; i++ )
	{
		Vector pdir = pos + r1*m_optimized[i].X + r2*m_optimized[i].Y + r3;
		Float minT = LinePlaneIntersection( pos, pdir, 0.0f );
		Vector minrz = pos + ( pdir - pos )*minT;
		m_optimized[i] = minrz;
	}
	*/
	//  m_minimum and m_maximum should have projected quads on both levels
	// it is time to merge them
	/*
	
	linear lin1( m_minimum[0], m_minimum[1] );
	linear lin2( m_minimum[1], m_minimum[2] );
	linear lin3( m_minimum[2], m_minimum[3] );
	linear lin4( m_minimum[3], m_minimum[0] );

	Vector lines[8];

	if( lin1.potential( m_maximum[0] )>0.0f ){ lines[0] = m_minimum[0]; lines[1] = m_minimum[1]; } else { lines[0] = m_maximum[0]; lines[1] = m_maximum[1]; }
	if( lin2.potential( m_maximum[1] )>0.0f ){ lines[2] = m_minimum[1]; lines[3] = m_minimum[2]; } else { lines[2] = m_maximum[1]; lines[3] = m_maximum[2]; }
	if( lin3.potential( m_maximum[2] )>0.0f ){ lines[4] = m_minimum[2]; lines[5] = m_minimum[3]; } else { lines[4] = m_maximum[2]; lines[5] = m_maximum[3]; }
	if( lin4.potential( m_maximum[3] )>0.0f ){ lines[6] = m_minimum[3]; lines[7] = m_minimum[0]; } else { lines[6] = m_maximum[3]; lines[7] = m_maximum[0]; }

	m_projected[0] = LinesIntersection( lines[0], lines[1], lines[2], lines[3] );
	m_projected[1] = LinesIntersection( lines[2], lines[3], lines[4], lines[5] );
	m_projected[2] = LinesIntersection( lines[4], lines[5], lines[6], lines[7] );
	m_projected[3] = LinesIntersection( lines[6], lines[7], lines[0], lines[1] );	

	///temp
	*/

	m_projected[0] = pos + r1*m_optimized[0].X + r2*m_optimized[0].Y + r3;
	m_projected[1] = pos + r1*m_optimized[1].X + r2*m_optimized[1].Y + r3;
	m_projected[2] = pos + r1*m_optimized[2].X + r2*m_optimized[2].Y + r3;
	m_projected[3] = pos + r1*m_optimized[3].X + r2*m_optimized[3].Y + r3;
	
	return true;
}

bool quad_projector::ProjectFull( const Vector & pos, const Vector & r1, const Vector & r2, const Vector & r3, Float farPlane, Float Fov, Float Ratio, Float MinAmp, Float MaxAmp )
{
	Float tFov = tan(Fov*m_rad_half_deg);
	Float y = tFov;
	Float x = (tFov*Ratio);
	m_projected[0] = pos + (r3 + (r1*-x) + (r2*-y))*farPlane*1.001f;
	m_projected[1] = pos + (r3 + (r1*x) + (r2*-y))*farPlane*1.001f;
	m_projected[2] = pos + (r3 + (r1*x) + (r2*y))*farPlane*1.001f;
	m_projected[3] = pos + (r3 + (r1*-x) + (r2*y))*farPlane*1.001f;

	return true;
}

bool quad_projector::ProjectScreenSpace( const Vector & pos, const Vector & r1, const Vector & r2, const Vector & r3, Float nearPlane, Float Fov, Float Ratio, Vector* outResult )
{
	Float tFov = tan(Fov*m_rad_half_deg);
	Float y = tFov;
	Float x = (tFov*Ratio);	

	outResult[2] = pos + (r3 + (r1*-x) + (r2*-y));
	outResult[0] = pos + (r3 + (r1*x) + (r2*-y));
	outResult[1] = pos + (r3 + (r1*x) + (r2*y));
	outResult[3] = pos + (r3 + (r1*-x) + (r2*y));

	return true;
}

bool quad_projector::ProjectWorldSpace( const Vector & pos, const Vector & r1, const Vector & r2, const Vector & r3, Float nearPlane, Float Fov, Float Ratio, Vector* outResult )
{
	Float px = pos.X;
	Float py = pos.Y;
	
	// step size (snap)
	Float siz = 10.0f;
	
	// square size
	Float esiz = 50.0f;

	Float spx = floorf( px/siz )*siz;
	Float spy = floorf( py/siz )*siz;

	m_projected[0] = Vector(spx-esiz,spy-esiz,0.0f);
	m_projected[1] = Vector(spx-esiz,spy+esiz,0.0f);
	m_projected[2] = Vector(spx+esiz,spy+esiz,0.0f);
	m_projected[3] = Vector(spx+esiz,spy-esiz,0.0f);

	return true;
}

void quad_projector::Optimize3()
{
	Int32 win = 0;
	Float maxv = -1.0f;
	Vector dir;
	for(int k=0;k<3;k++)
	{
		int prind = k==0 ? 2 : k-1;
		int nexind = k==2 ? 0 : k+1;
		Vector v1 = ( m_arr[prind] - m_arr[k] ); v1.Normalize2();
		Vector v2 = ( m_arr[nexind] - m_arr[k] ); v2.Normalize2();
		float d = Vector::Dot2( v1, v2 );
		if( d>maxv ){ win = k; maxv = d; }
	}
	int prind = win==0 ? 2 : win-1;
	int nexind = win==2 ? 0 : win+1;
	Vector temp1 = (m_arr[win]-m_arr[prind]);
	Vector temp2 = (m_arr[nexind]-m_arr[win]);
	dir = ( temp1+temp2 ); dir.Normalize2();

	if( win==0 )
	{
		Vector p0 = m_arr[0] + dir*-3.0f;
		Vector p1 = m_arr[0] + dir*3.0f;
		m_optimized[0] = p0;
		m_optimized[1] = p1;
		m_optimized[2] = m_arr[1];
		m_optimized[3] = m_arr[2];
	}
	if( win==1 )
	{
		Vector p0 = m_arr[1] + dir*-3.0f;
		Vector p1 = m_arr[1] + dir*3.0f;
		m_optimized[0] = m_arr[0];
		m_optimized[1] = p0;
		m_optimized[2] = p1;
		m_optimized[3] = m_arr[2];
	}
	if( win==2 )
	{
		Vector p0 = m_arr[2] + dir*-3.0f;
		Vector p1 = m_arr[2] + dir*3.0f;
		m_optimized[0] = m_arr[0];
		m_optimized[1] = m_arr[1];
		m_optimized[2] = p0;
		m_optimized[3] = p1;
	}
}

void quad_projector::Optimize5()
{
	Int32 win = 0;
	Float maxv = 1.0f;
	for(int k=0;k<5;k++)
	{
		int prind = k==0 ? 4 : k-1;
		int nexind = k==4 ? 0 : k+1;
		Vector v1 = ( m_arr[prind] - m_arr[k] ); v1.Normalize2();
		Vector v2 = ( m_arr[nexind] - m_arr[k] ); v2.Normalize2();
		float d = Vector::Dot2( v1, v2 );
		if( d<maxv ){ win = k; maxv = d; }
	}
	int prind = win==0 ? 4 : win-1;
	int nexind = win==4 ? 0 : win+1;
	Vector dir = ( (m_arr[win]-m_arr[prind])+(m_arr[nexind]-m_arr[win]) ); dir.Normalize2();

	int prind2 = prind==0 ? 4 : prind-1;
	int nexind2 = nexind==4 ? 0 : nexind+1;

	Vector p1= LinesIntersection( m_arr[win], m_arr[win]+dir, m_arr[prind],  m_arr[prind2] );
	Vector p2= LinesIntersection( m_arr[win], m_arr[win]+dir, m_arr[nexind], m_arr[nexind2] );

	if( win==0 )
	{
		m_optimized[3] = p1;
		m_optimized[0] = p2;
		m_optimized[1] = m_arr[2];
		m_optimized[2] = m_arr[3];
	}
	if( win==1 )
	{
		m_optimized[3] = m_arr[4];
		m_optimized[0] = p1;
		m_optimized[1] = p2;
		m_optimized[2] = m_arr[3];
	}
	if( win==2 )
	{
		m_optimized[3] = m_arr[0];
		m_optimized[0] = p1;
		m_optimized[1] = p2;
		m_optimized[2] = m_arr[4];
	}
	if( win==3 )
	{
		m_optimized[3] = p2;
		m_optimized[0] = m_arr[0];
		m_optimized[1] = m_arr[1];
		m_optimized[2] = p1;
	}
	if( win==4 )
	{
		m_optimized[3] = m_arr[2];
		m_optimized[0] = p1;
		m_optimized[1] = p2;
		m_optimized[2] = m_arr[1];
	}
}

Vector quad_projector::LinesIntersection( const Vector & p0, const Vector & p1, const Vector & r0, const Vector & r1 )
{
	Vector a = p1-p0;
	Vector u = r1-r0;
	Float uu = Vector::Dot2(u,u);
	Float aa = Vector::Dot2(a,a);
	Float au = Vector::Dot2(a,u);
	Float p0u = Vector::Dot2(p0,u);
	Float ur0 = Vector::Dot2(u,r0);
	float t = -(  (-4.0f*uu)   *  Vector::Dot2(a,(p0-r0)) + (4*au*(p0u-ur0))   )/    ( (4*au*au) - 4*aa*uu );
	return a*t+p0;
}

Float quad_projector::LinePlaneIntersection( const Vector & p1, const Vector & p2, Float z )
{
	Vector dir = p2-p1;
	if( abs(dir.Z) < 0.00001f ){return -1.0f;}
	return -(p1.Z - z)/dir.Z;
}


CRenderProxy_Water::CRenderProxy_Water()
	: IRenderProxy( RPT_Water )
	, m_heightmap( nullptr )
	, m_controlTextureArray( nullptr )	
	, m_localShapesTextureArray( NULL )	
	, m_simulator()
	, m_dynamicsimulator()
	, m_amplitudeScale(1.0f, 1.0f, 1.0f, 1.0f)
	, m_uvScale(1.0f, 1.0f, 1.0f, 1.0f)
	, m_philipsParameters(0.0f, 0.0f, 0.0f, 0.0f)
	, m_numLakes( 0 )
	, m_shouldRenderUnderwater( false )
	, m_visible( true )
	, m_shouldRenderWater( true )
	, m_shouldClearUnderwater( false )
	, m_localWaterShapesMaxHeight( 0.0f )
	, m_shouldSimulateFFT( false )
	, m_shouldReadBackCPUData( false )
	, m_numSkippedFFTFrames( 0 )
	, m_currentClosestTileDistanceSqr( NumericLimits< Float >::Infinity() )
{		
	m_arr.Reserve(8);

	m_instanceData = GpuApi::CreateBuffer( WATER_INSTANCE_BUFFER_SIZE, GpuApi::BCC_Vertex, GpuApi::BUT_Dynamic, GpuApi::BAF_CPUWrite );
	GpuApi::SetBufferDebugPath( m_instanceData, "waterinstance" );
	m_numinstances = 0;

	m_qp.ProjectWorldSpace( Vector::ZEROS, Vector::ZEROS, Vector::ZEROS, Vector::ZEROS, 0.0f, 0.0f, 0.0f, NULL);
	m_plane.Expand_sq( m_qp.GetVectors()[0],m_qp.GetVectors()[1],m_qp.GetVectors()[2],m_qp.GetVectors()[3] );

	m_shapeWaterLevel_min = 0.0f;
	m_shapeWaterLevel_max = 0.0f;
}

CRenderProxy_Water::~CRenderProxy_Water()
{
	SAFE_RELEASE( m_controlTextureArray );
	SAFE_RELEASE( m_heightmap );
	
	GpuApi::SafeRelease( m_localShapesTextureArray );
	GpuApi::SafeRelease( m_furierRef );
	GpuApi::SafeRelease( m_dynamicWater );
	GpuApi::SafeRelease( m_intersectionTexture );
	GpuApi::SafeRelease( m_constantBuffer );
	GpuApi::SafeRelease( m_instanceData );
}

void CRenderProxy_Water::AttachToScene()
{
	//m_fullplane.init( 28, 20 );
	m_plane.init( GRID_TESSLATION, GRID_TESSLATION );
	m_simulator.Initialize( WATER_RESOLUTION );
	m_dynamicsimulator.Initialize( );

	GpuApi::TextureDesc texDescTex2;
	texDescTex2.type = GpuApi::TEXTYPE_ARRAY;
	texDescTex2.format = GpuApi::TEXFMT_Float_R32;
	texDescTex2.initLevels = 1;
	texDescTex2.usage = GpuApi::TEXUSAGE_Samplable;
	texDescTex2.width = WATER_LOCAL_RESOLUTION;
	texDescTex2.height = WATER_LOCAL_RESOLUTION;
	texDescTex2.sliceNum = NUM_LAKES_MAX;
	m_localShapesTextureArray = GpuApi::CreateTexture( texDescTex2, GpuApi::TEXG_System );
	GpuApi::SetTextureDebugPath( m_localShapesTextureArray, "WaterLocalShapesArray" );
		
	GpuApi::TextureDesc texDescTex3;
	texDescTex3.type = GpuApi::TEXTYPE_2D;
	texDescTex3.format = GpuApi::TEXFMT_Float_R16G16;
	texDescTex3.initLevels = 1;
	texDescTex3.usage = GpuApi::TEXUSAGE_Samplable | GpuApi::TEXUSAGE_RenderTarget;

	// halving the res for intersection texture
	texDescTex3.width = GetRenderer()->GetSurfaces()->GetWidth(true)/2;
	texDescTex3.height = GetRenderer()->GetSurfaces()->GetHeight(true)/2;	
	
	m_intersectionTexture = GpuApi::CreateTexture( texDescTex3, GpuApi::TEXG_System );
	GpuApi::SetTextureDebugPath( m_intersectionTexture, "WaterIntersection" );

	m_constantBuffer = GpuApi::CreateBuffer( sizeof( SWaterConstants ), GpuApi::BCC_Constant, GpuApi::BUT_Dynamic, 0 );
}

void CRenderProxy_Water::UpdateControlTexture( CRenderTextureArray* newControlTextureArray )
{
	if ( newControlTextureArray )
	{
		SAFE_COPY( m_controlTextureArray, newControlTextureArray );
	}
}

void CRenderProxy_Water::BindWaterConstantsBuffers( GpuApi::eShaderType shaderTarget )
{
	GpuApi::BindConstantBuffer( 4, m_constantBuffer, shaderTarget );
}

void CRenderProxy_Water::UnbindWaterConstantsBuffers( GpuApi::eShaderType shaderTarget )
{
	GpuApi::BindConstantBuffer( 4, GpuApi::BufferRef::Null(), shaderTarget );
}

void CRenderProxy_Water::Render( const CRenderSceneEx* renderScene, const class RenderingContext& context, const CRenderFrameInfo& frameInfo )
{
	// Water doesn't have light channels, so it's as though they were 0
	if ( !context.CheckLightChannels( 0 ) )
	{
		return;
	}

	if( !m_shouldRenderWater || !IsVisible() ){ return; }

	UpdateCPUData();

	UpdateConstantBuffer( frameInfo );

	BindWaterConstantsBuffers( GpuApi::PixelShader );
	BindWaterConstantsBuffers( GpuApi::DomainShader );
	BindWaterConstantsBuffers( GpuApi::VertexShader );
	BindWaterConstantsBuffers( GpuApi::HullShader );

	BindShadersAndDraw( frameInfo, GpuApi::TextureRef::Null() );
}

void CRenderProxy_Water::Render( const CRenderSceneEx* renderScene, const class RenderingContext& context, const CRenderFrameInfo& frameInfo, GpuApi::TextureRef terrainHeightMapTextureArray, const CClipMapShaderData& clipMapData )
{	
	// Water doesn't have light channels, so it's as though they were 0
	if ( !context.CheckLightChannels( 0 ) )
	{
		return;
	}
	if( !m_shouldRenderWater || !IsVisible() ){ return; }

	{
		UpdateCPUData();

		UpdateConstantBuffer( frameInfo );

		BindWaterConstantsBuffers( GpuApi::PixelShader );
		BindWaterConstantsBuffers( GpuApi::DomainShader );
		BindWaterConstantsBuffers( GpuApi::VertexShader );
		BindWaterConstantsBuffers( GpuApi::HullShader );

		GpuApi::RenderTargetSetup rtSetupOld;
		rtSetupOld = GpuApi::GetRenderTargetSetup();

		CRenderSurfaces* surfaces = GetRenderer()->GetSurfaces();
						
		GpuApi::RenderTargetSetup rtSetupMain;
		rtSetupMain.SetColorTarget( 0, surfaces->GetRenderTargetTex( GetRenderer()->IsMSAAEnabled( frameInfo ) ? RTN_MSAAColor : RTN_Color) );
		rtSetupMain.SetColorTarget( 1, surfaces->GetLocalReflectionsMaskTex() );
		rtSetupMain.SetDepthStencilTarget( GetRenderer()->IsMSAAEnabled( frameInfo ) ? surfaces->GetDepthBufferTexMSAA() : surfaces->GetDepthBufferTex() );
		rtSetupMain.SetViewport( frameInfo.m_width, frameInfo.m_height, 0, 0 );

		GpuApi::SetupRenderTargets( rtSetupMain );		
		CGpuApiScopedDrawContext drawContext ( GpuApi::DRAWCONTEXT_GlobalWater, LC_DynamicObject );
		
		GpuApi::TextureRef sceneColor = GetRenderer()->GetSurfaces()->GetRenderTargetTex( RTN_Color2 );		
		GpuApi::BindTextures( PSSMP_SceneColor, 1, &sceneColor, GpuApi::PixelShader );						
		GpuApi::SetSamplerStatePreset( PSSMP_SceneColor, GpuApi::SAMPSTATEPRESET_MirrorLinearNoMip, GpuApi::PixelShader );
		
		GpuApi::BindTextures( 4, 1, &terrainHeightMapTextureArray, GpuApi::VertexShader );
		GpuApi::BindTextures( 4, 1, &terrainHeightMapTextureArray, GpuApi::DomainShader );
		GpuApi::BindTextures( 4, 1, &terrainHeightMapTextureArray, GpuApi::PixelShader );		
		GpuApi::SetSamplerStatePreset( 4, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::DomainShader );
		GpuApi::SetSamplerStatePreset( 4, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::VertexShader );
		GpuApi::SetSamplerStatePreset( 4, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::PixelShader );			
	

		if ( m_localShapesTextureArray )
		{	
			GpuApi::BindTextures( 2, 1, &m_localShapesTextureArray, GpuApi::PixelShader );
			GpuApi::SetSamplerStatePreset( 2, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::PixelShader );
		}

		// Bind kosher unified terrain params.
		clipMapData.BindTerrainParams( 5 );
		clipMapData.BindClipWindows( 17, GpuApi::DomainShader );
		clipMapData.BindClipWindows( 17, GpuApi::VertexShader );
		clipMapData.BindClipWindows( 17, GpuApi::PixelShader );				
		COMPILE_ASSERT( 17 == PSSMP_GlobalShadowAndSSAO ); //< this texture slot is occupied by some global texture, so we'll restore it right after draw

		BindShadersAndDraw( frameInfo, terrainHeightMapTextureArray );

		GpuApi::TextureRef shadowBuffer = surfaces ? surfaces->GetRenderTargetTex( RTN_GlobalShadowAndSSAO ) : GpuApi::TextureRef::Null();
		GpuApi::BindTextures( PSSMP_GlobalShadowAndSSAO, 1, &shadowBuffer, GpuApi::PixelShader );
				
		GpuApi::BindTextures( PSSMP_SceneColor, 1, nullptr, GpuApi::PixelShader );	
		//GpuApi::BindTextures( 15, 1, nullptr, GpuApi::PixelShader );
		GpuApi::BindTextures( 4, 1, nullptr, GpuApi::DomainShader );
		GpuApi::BindTextures( 4, 1, nullptr, GpuApi::VertexShader );	
		GpuApi::BindTextures( 4, 1, nullptr, GpuApi::PixelShader );
		GpuApi::BindTextures( 6, 1, nullptr, GpuApi::DomainShader );
		GpuApi::BindTextures( 6, 1, nullptr, GpuApi::VertexShader );
		GpuApi::BindTextures( 6, 1, nullptr, GpuApi::PixelShader );
		GpuApi::BindTextures( 17, 1, nullptr, GpuApi::PixelShader );
		GpuApi::BindTextures( 17, 1, nullptr, GpuApi::VertexShader );
		GpuApi::BindTextures( 17, 1, nullptr, GpuApi::DomainShader );
		GpuApi::SetupRenderTargets( rtSetupOld );
	}
}

void CRenderProxy_Water::Simulate( GpuApi::TextureRef terrainHeightMapTextureArray, const CRenderFrameInfo& frameInfo )
{
	if( !m_shouldRenderWater ){ return; }
	SimulateDynamics( terrainHeightMapTextureArray, frameInfo );
}

void CRenderProxy_Water::BindShadersAndDraw( const CRenderFrameInfo& frameInfo, GpuApi::TextureRef terrainHeightMapTextureArray )
{
	PC_SCOPE_RENDER_LVL1(BindShadersAndDraw);	


	m_dynamicsimulator.FinishAsyncSimulate();


	// screen plane - water top
	Project( frameInfo, false );

	CRenderSurfaces* surfaces = GetRenderer()->GetSurfaces();
	
	const CGpuApiScopedTwoSidedRender scopedForcedTwoSided ( true );

	if( m_furierRef )
	{		
		const GpuApi::SamplerStateRef &stateRefMip = GpuApi::GetSamplerStatePreset( GpuApi::SAMPSTATEPRESET_WrapLinearNoMip );
		GpuApi::BindTextures( 5, 1, &m_furierRef, GpuApi::PixelShader );
		GpuApi::SetSamplerState( 5, stateRefMip, GpuApi::PixelShader );	
				
		const GpuApi::SamplerStateRef &stateRef = GpuApi::GetSamplerStatePreset( GpuApi::SAMPSTATEPRESET_WrapLinearNoMip );
		GpuApi::BindTextures( 5, 1, &m_furierRef, GpuApi::DomainShader );
		GpuApi::SetSamplerState( 5, stateRef, GpuApi::DomainShader );		
	}
	else
	{
		UnbindWaterConstantsBuffers( GpuApi::DomainShader );
		UnbindWaterConstantsBuffers( GpuApi::HullShader );
		UnbindWaterConstantsBuffers( GpuApi::PixelShader );
		UnbindWaterConstantsBuffers( GpuApi::VertexShader );
		return;
	}

	// decals for currents and shore waves
	GpuApi::TextureRef internalCrap = GpuApi::GetInternalTexture( GpuApi::INTERTEX_Blank2D);

	{		
		GpuApi::TextureRef decals = GetRenderer()->GetSurfaces()->GetRenderTargetTex( RTN_Color3 );		
		RED_ASSERT( decals, TXT("Seems like the RTN_Color3 is gone? Water decals might be wrong at this poing") );
		
		const GpuApi::SamplerStateRef &stateRefMip = GpuApi::GetSamplerStatePreset( GpuApi::SAMPSTATEPRESET_WrapLinearNoMip );
		
		if( decals ) GpuApi::BindTextures( 3, 1, &decals, GpuApi::PixelShader );
		else 
			GpuApi::BindTextures( 3, 1, &internalCrap, GpuApi::PixelShader );		

		GpuApi::SetSamplerState( 3, stateRefMip, GpuApi::PixelShader );
	}

	const GpuApi::SamplerStateRef &stateRefNoMip = GpuApi::GetSamplerStatePreset( GpuApi::SAMPSTATEPRESET_BorderLinearNoMip );
	GpuApi::SetSamplerState( 1, stateRefNoMip, GpuApi::PixelShader );	
	
	if( m_dynamicWater ) GpuApi::BindTextures( 1, 1, &m_dynamicWater, GpuApi::PixelShader );
	else
		GpuApi::BindTextures( 1, 1, &internalCrap, GpuApi::PixelShader );
	

	UpdateConstantBuffer( frameInfo );

	BindWaterConstantsBuffers( GpuApi::PixelShader );
	BindWaterConstantsBuffers( GpuApi::DomainShader );
	BindWaterConstantsBuffers( GpuApi::VertexShader );
	BindWaterConstantsBuffers( GpuApi::HullShader );

	if ( m_controlTextureArray )
	{
		m_controlTextureArray->Bind( 0, RST_PixelShader );				
		GpuApi::SetSamplerStatePreset( 0, GpuApi::SAMPSTATEPRESET_WrapAnisoMip, GpuApi::PixelShader );
	}
	
	if ( m_localShapesTextureArray )
	{	
		GpuApi::BindTextures( 2, 1, &m_localShapesTextureArray, GpuApi::DomainShader );
		GpuApi::BindTextures( 2, 1, &m_localShapesTextureArray, GpuApi::VertexShader );
		GpuApi::SetSamplerStatePreset( 2, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::DomainShader );
		GpuApi::SetSamplerStatePreset( 2, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::VertexShader );
	}
					
	GetRenderer()->m_shaderGlobalOcean->Bind();

	// prepare to draw - water top
	Uint32 vbSstride = GpuApi::GetChunkTypeStride( GpuApi::BCT_VertexWater );
	Uint32 vbOffset = 0;
	GpuApi::SetVertexFormatRaw( GpuApi::BCT_VertexWater );
	
	{
		GpuApi::BufferRef buffers[2]	= { m_plane.m_vertexBufferRef,	m_instanceData	};
		Uint32 strides[2]				= { vbSstride,					sizeof(Float)*2 };
		Uint32 offsets[2]				= { 0,							0	};
		GpuApi::BindVertexBuffers( 0, 2, buffers, strides, offsets );
		GpuApi::BindIndexBuffer( m_plane.m_indicesBufferRef );
		GpuApi::DrawInstancedIndexedPrimitive( GpuApi::PRIMTYPE_4CP_PATCHLIST, 0, 0, m_plane.m_numverts, 0, m_plane.m_numquads, m_numinstances );
	}
	
	GetRenderer()->GetStateManager().SetShader( GpuApi::ShaderRef::Null(), RST_HullShader );
	GetRenderer()->GetStateManager().SetShader( GpuApi::ShaderRef::Null(), RST_DomainShader );

	if( m_shouldRenderUnderwater ) BindShadersAndDrawUnderwater( frameInfo );
	else
		if( m_shouldClearUnderwater )
		{
			GetRenderer()->ClearColorTarget( m_intersectionTexture, Vector::ZEROS );
			m_shouldClearUnderwater = false;
		}

	GpuApi::BindTextures( 0, 4, nullptr, GpuApi::PixelShader );
	GpuApi::BindTextures( 5, 2, nullptr, GpuApi::PixelShader );

	if ( m_localShapesTextureArray )
	{
		GpuApi::BindTextures( 2, 1, nullptr, GpuApi::DomainShader );
	}

	if( m_furierRef )
	{
		GpuApi::BindTextures( 5, 1, nullptr, GpuApi::DomainShader );
	}

	UnbindWaterConstantsBuffers( GpuApi::DomainShader );
	UnbindWaterConstantsBuffers( GpuApi::HullShader );
	UnbindWaterConstantsBuffers( GpuApi::PixelShader );
	UnbindWaterConstantsBuffers( GpuApi::VertexShader );
}

void CRenderProxy_Water::BindShadersAndDrawUnderwater( const CRenderFrameInfo& frameInfo )
{	
	{
		// TODO
		CRenderStateManager &stateManager = GetRenderer()->GetStateManager();	

		GpuApi::RenderTargetSetup rtSetupOld;
		rtSetupOld = GpuApi::GetRenderTargetSetup();

		GpuApi::RenderTargetSetup rtSetup1;
		rtSetup1.SetColorTarget(0, m_intersectionTexture);				
		rtSetup1.SetViewportFromTarget( m_intersectionTexture );			
		GpuApi::SetupRenderTargets(rtSetup1);
		
		// well, dont delete it because it will break the lakes underwater post (and this is bound already, weird)
		if ( m_localShapesTextureArray )
		{	
			
			GpuApi::BindTextures( 2, 1, &m_localShapesTextureArray, GpuApi::PixelShader );
			GpuApi::SetSamplerStatePreset( 2, GpuApi::SAMPSTATEPRESET_ClampLinearNoMip, GpuApi::PixelShader );
		}

		CGpuApiScopedDrawContext drawContext ( GpuApi::DRAWCONTEXT_PostProcSet );				
		GetRenderer()->GetPostProcess()->m_drawer->DrawQuad( GetRenderer()->m_shaderGlobalOceanIntersectionRtt );
		

		// restore
		GpuApi::SetupRenderTargets(rtSetupOld);		
	}	
}

void CRenderProxy_Water::SimulateDynamics( GpuApi::TextureRef terrainHeightMapTextureArray, const CRenderFrameInfo& frameInfo )
{
	PC_SCOPE_RENDER_LVL1(SimulateDynamics);

	GpuApi::TextureRef simResult;
	Float tim = m_gameTime;

	Bool useDynamicWaterSimulation = Config::cvUseDynamicWaterSimulation.Get();


	PerformFFTVisibilityTest();


	const Bool doImpulsesFirst = useDynamicWaterSimulation && m_shouldSimulateFFT && !m_simulator.IsAsyncCompute();

	// If the FFT isn't using async compute, apply impulses first. This way we keep the compute jobs together, instead of
	// switching to graphics in between. Should be more efficient.
	if( doImpulsesFirst )
	{
		m_dynamicsimulator.ApplyImpulses();
	}


	if( m_shouldSimulateFFT )
	{
		simResult = m_simulator.Calculate( m_philipsParameters.X, m_philipsParameters.Y, m_philipsParameters.Z, m_philipsParameters.W, tim, m_amplitudeScale.W, m_shouldReadBackCPUData );
		GpuApi::SafeRefCountAssign( m_furierRef, simResult );
		m_shouldSimulateFFT = false;
		m_numSkippedFFTFrames = 0;
	}

	if( useDynamicWaterSimulation )
	{
		// If we didn't apply the impulses earlier, do them now. When FFT is done with async compute, then this lets that get started
		// while we do the impulses, for hopefully slightly better parallelism.
		if( !doImpulsesFirst )
		{
			m_dynamicsimulator.ApplyImpulses();
		}

		//////////////////////////////////////////////////////////////////////////
		// This is only needed to support the non-compute dynwater shaders, for GlobalWater.dynamicWaterResolutionInv
		UpdateConstantBuffer( frameInfo );
		BindWaterConstantsBuffers( GpuApi::PixelShader );
		//////////////////////////////////////////////////////////////////////////

		simResult = m_dynamicsimulator.Calculate( terrainHeightMapTextureArray, tim );
		GpuApi::SafeRefCountAssign( m_dynamicWater, simResult );
	}
	else
	{
		simResult = GpuApi::GetInternalTexture( GpuApi::INTERTEX_Blank2D );
		GpuApi::SafeRefCountAssign( m_dynamicWater, simResult );
	}
}

void CRenderProxy_Water::PerformFFTVisibilityTest()
{
	// Yes, it is a last minute performance chop
	// skip some frames from the FFT simulation when the tiles are culled (or far away)
	// this will effectively make distant water look like it runs slower
	// but the tests looks like this is acceptable
	//
	// TODO - instead of skipping the frames we should dynamically compute with lower resolution
	//

	Uint32 skipFFTFrames = 0;	
	// 80m
	if( m_currentClosestTileDistanceSqr > 6400.0f ) skipFFTFrames = 2;
	// 30m
	else if( m_currentClosestTileDistanceSqr > 900.0f ) skipFFTFrames = 1;

	if( m_numSkippedFFTFrames >= skipFFTFrames )
	{		
		m_shouldSimulateFFT = true;
		m_numSkippedFFTFrames = 0;
	}
	else
		++m_numSkippedFFTFrames;
}

void CRenderProxy_Water::UpdateCPUData()
{
	if ( m_shouldReadBackCPUData )
	{
		RED_WARNING( m_heightmap != nullptr, "Water render proxy has null heightmap" );
		void* heightmapWritePtr = m_heightmap != nullptr ? m_heightmap->GetWritePointer() : nullptr;

		m_simulator.FillCPUData( heightmapWritePtr );

		// Simulator has filled the heightmap, so we can flip it now.
		if ( m_heightmap != nullptr )
		{
			m_heightmap->FlipBuffers();
		}

		m_shouldReadBackCPUData = false;
	}
}

void CRenderProxy_Water::UpdatePhilipsParameters( CGlobalWaterUpdateParams* waterParams )
{
	RED_ASSERT( waterParams != nullptr, TXT("Null update parameters") );
	if ( waterParams == nullptr )
	{
		return;
	}

	SAFE_COPY( m_heightmap, waterParams->m_heightmap );

	if ( m_heightmap != nullptr && !m_heightmap->IsInit() )
	{
		m_heightmap->Initialize( m_simulator.GetReadBackResolution(), m_simulator.GetReadBackResolution() );
	}

	m_philipsParameters = waterParams->m_phillipsData;
	m_amplitudeScale = waterParams->m_amplitudeScale;
	m_uvScale = waterParams->m_uvScale;

	const Vector cameraPosition = waterParams->m_camera + m_cameraForward*10.0f;
	m_dynamicsimulator.UpdateCamera( cameraPosition );

	m_dynamicsimulator.SetRainIntensity( waterParams->m_rainIntensity );
	
	m_gameTime = waterParams->m_gameTime;
	
	if( m_shouldRenderUnderwater && !waterParams->m_shouldRenderUnderwater ) m_shouldClearUnderwater = true;
	m_shouldRenderUnderwater = waterParams->m_shouldRenderUnderwater;

	m_shouldRenderWater = waterParams->m_shouldRenderWater;

	Vector camPos = m_dynamicsimulator.GetSnappedCameraPosition();
	Matrix texMat = Matrix::IDENTITY;
	texMat.SetTranslation( camPos );
	texMat.SetRow(0, Vector(15.0f,0.0f,0.0f,0.0f) );
	texMat.SetRow(1, Vector(0.0f,-15.0f,0.0f,0.0f) );
	texMat.SetRow(2, Vector(0.0f,0.0f,1.0f,0.0f) );
	texMat = texMat.FullInvert();
	texMat.V[2].A[2] = 0.0f;

	// impulses update like rain swimming etc.
	RED_WARNING_ONCE( waterParams->m_impulses.Size() <= NUM_DISPLACEMENTS_MAX, "Water has too many impulses: %u", waterParams->m_impulses.Size() );
	const Uint32 numImpulses = Min< Uint32 >( waterParams->m_impulses.Size(), NUM_DISPLACEMENTS_MAX );
	for( Uint32 i=0; i<numImpulses; ++i )
	{
		Matrix m = Matrix::IDENTITY;
		m = waterParams->m_impulses[i].m_transformMatrix;

		Float v = m.GetRow(3).W;
		m.V[3].A[3] = 1.0f;
		m = Matrix::Mul( texMat, m );

		v = v>1.0f ? 1.0f : v;
		m.V[3].A[2] = 0.0f;
		m.V[3].A[3] = waterParams->m_impulses[i].m_intensity * v;

		m_dynamicsimulator.AddImpulse( m );
	}
	
	// displacement collisions update
	if ( waterParams->m_displCollisions.Size() > 0 )
	{
		RED_WARNING_ONCE( waterParams->m_displCollisions.Size() <= NUM_DISPLACEMENTS_MAX, "Water has too many displacement collisions: %u", waterParams->m_displCollisions.Size() );

		Uint32 numCollisions = Min< Uint32 >( waterParams->m_displCollisions.Size(), NUM_DISPLACEMENTS_MAX );

		Int32 ind = -1;
		for( Uint32 i=0; i<numCollisions; ++i )
		{
			const Matrix& globalMat = waterParams->m_displCollisions[i].m_transformMatrix;
			if( waterParams->m_displCollisions[i].m_intensity > 0.0f )
			{
				ind++;
				m_displacements[ ind ] = globalMat;
			}		
		}
		m_numBoats = ind+1;
	}
}

void CRenderProxy_Water::AddLocalShapes( const CLocalWaterShapesParams* shapesParams )
{
	RED_WARNING_ONCE( shapesParams->m_numShapes <= NUM_LAKES_MAX, "Water has too many local shapes: %u", shapesParams->m_numShapes );
	m_numLakes = Min< Uint32 >( shapesParams->m_numShapes, NUM_LAKES_MAX );
	if( m_numLakes > 0 )
	{
		for(Int32 i=0;i<m_numLakes;i++)
		{
			const Uint32 tsize = WATER_LOCAL_RESOLUTION*WATER_LOCAL_RESOLUTION;
			GpuApi::LoadTextureData2D( m_localShapesTextureArray, 0, i, nullptr, shapesParams->m_shapesBuffer+(tsize*i), WATER_LOCAL_RESOLUTION*sizeof(Float) );
		}

		m_shapeWaterLevel_min = shapesParams->m_shapeWaterLevel_min; 
		m_shapeWaterLevel_max = shapesParams->m_shapeWaterLevel_max;

		// Store the lakes offsets
		Red::System::MemoryCopy( &m_lakes[0], shapesParams->m_matrices, m_numLakes * sizeof( Vector ) );
	}
	else
	{
		if( m_localShapesTextureArray ) GpuApi::SafeRelease( m_localShapesTextureArray );
		Red::System::MemoryZero( &m_lakes[0], NUM_LAKES_MAX * sizeof( Vector ) );
	}

	m_localWaterShapesMaxHeight = shapesParams->m_waterMaxLevel;
}

void CRenderProxy_Water::Project( const CRenderFrameInfo& frameInfo, bool full )
{
	Float farr = frameInfo.m_camera.GetFarPlane();
	Float ratio = frameInfo.m_camera.GetAspect();
	Float fov  = frameInfo.m_camera.GetFOV();

	Vector row3 = frameInfo.m_camera.GetCameraForward();
	Vector row1 = frameInfo.m_camera.GetCameraRight();
	Vector row2 = frameInfo.m_camera.GetCameraUp();
	Vector pos  = frameInfo.m_camera.GetPosition();

	Float px = pos.X;
	Float py = pos.Y;
	
	// step size (snap)
	const Float siz = Float(WATER_GRID_SIZE)/Float(GRID_TESSLATION);

	// square size
	Float esiz = WATER_GRID_SIZE_HALF;

	Float spx = floorf( px/siz )*siz - (WATER_GRID_SIZE_HALF*WATER_GRID_ROW_SIZE);
	Float spy = floorf( py/siz )*siz - (WATER_GRID_SIZE_HALF*WATER_GRID_ROW_SIZE);

	m_qp.GetVectors()[0] = Vector(-esiz,-esiz,0.0f);
	m_qp.GetVectors()[1] = Vector(-esiz,+esiz,0.0f);
	m_qp.GetVectors()[2] = Vector(+esiz,+esiz,0.0f);
	m_qp.GetVectors()[3] = Vector(+esiz,-esiz,0.0f);

	m_currentClosestTileDistanceSqr = NumericLimits<Float>::Infinity();

	if( fov > 0.0001f )
	{
		// keeping for testing purposes
		//m_qp.Project( pos, row1, row2, row3, farr, fov, ratio, -10.0f, 10.0f, m_localWaterShapesMaxHeight );
		//m_fullplane.Expand( m_qp.GetVectors()[0],m_qp.GetVectors()[1],m_qp.GetVectors()[2],m_qp.GetVectors()[3] );
	
		//m_qp.ProjectWorldSpace( pos, row1, row2, row3, farr, fov, ratio, NULL);
		//m_plane.Expand_sq( m_qp.GetVectors()[0],m_qp.GetVectors()[1],m_qp.GetVectors()[2],m_qp.GetVectors()[3] );

		//m_qp.ProjectWorldSpace( Vector::ZEROS, Vector::ZEROS, Vector::ZEROS, Vector::ZEROS, 0.0f, 0.0f, 0.0f, NULL);

		// we will test every grid patch with frustum
		Box box;
		CFrustum frustum;
		frustum.InitFromCamera( frameInfo.m_camera.GetWorldToScreen() );			

		m_plane.Expand_sq( m_qp.GetVectors()[0],m_qp.GetVectors()[1],m_qp.GetVectors()[2],m_qp.GetVectors()[3] );
		Float* instancedPtr = (Float*)GpuApi::LockBuffer( m_instanceData, GpuApi::BLF_Discard, 0, WATER_INSTANCE_BUFFER_SIZE );
		m_numinstances = 0;
		
		// we successfully locked
		if( instancedPtr )
		{
			for( Uint32 jj=0;jj<WATER_GRID_ROW_SIZE; ++jj )
			{
				for( Uint32 kk=0;kk<WATER_GRID_ROW_SIZE; ++kk )	
				{
					Float finalX = spx + kk * WATER_GRID_SIZE;
					Float finalY = spy + jj * WATER_GRID_SIZE;

					box.Min = Vector( finalX-WATER_GRID_SIZE_HALF,finalY-WATER_GRID_SIZE_HALF, m_shapeWaterLevel_min );
					box.Max = Vector( finalX+WATER_GRID_SIZE_HALF,finalY+WATER_GRID_SIZE_HALF, m_shapeWaterLevel_max );

					if( kk==0 ){ box.Min.X += -WATER_GRID_EXPANSION; }
					if( kk==WATER_GRID_ROW_SIZE-1 ){ box.Max.X +=  WATER_GRID_EXPANSION; }

					if( jj==0 ){ box.Min.Y += -WATER_GRID_EXPANSION; }
					if( jj==WATER_GRID_ROW_SIZE-1 ){ box.Max.Y +=  WATER_GRID_EXPANSION; }

					Uint32 btest = frustum.TestBox(box);

					if( btest )
					{
						instancedPtr[0] = finalX;
						instancedPtr[1] = finalY;
						instancedPtr += 2;
						m_numinstances++;

						const Float tileSqrDistanceToCamera = box.SquaredDistance( pos );
						if( m_currentClosestTileDistanceSqr > tileSqrDistanceToCamera ) m_currentClosestTileDistanceSqr = tileSqrDistanceToCamera;
					}
				}
			}
		}		

		GpuApi::UnlockBuffer( m_instanceData );
	}
	else
	// map rendering
	{
		Vector* arr = m_qp.GetVectors();
		arr[0] = Vector(  -4000.0f, -4000.0f, 0.0f  );
		arr[1] = Vector(   4000.0f, -4000.0f, 0.0f  );
		arr[2] = Vector(   4000.0f,  4000.0f, 0.0f  );
		arr[3] = Vector(  -4000.0f,  4000.0f, 0.0f  );
		m_plane.Expand( m_qp.GetVectors()[0],m_qp.GetVectors()[1],m_qp.GetVectors()[2],m_qp.GetVectors()[3] );
		m_currentClosestTileDistanceSqr = 0.0f;
	}
}


static inline void CalcBezier4v(Float t, Float v0, Float v1, Float v2, Float v3, Float &out)
{
	out = powf(t,3.0f) * (-v0 + 3.0f*v1 - 3.0f*v2 + v3) + 
		powf(t,2.0f) * (3.0f*v0 - 6.0f*v1 + 3.0f*v2) +
		t*(-3.0f*v0 + 3.0f*v1) + 
		v0;
}

void CRenderProxy_Water::UpdateConstantBuffer( const CRenderFrameInfo& frameInfo )
{
	void* constantData = GpuApi::LockBuffer( m_constantBuffer, GpuApi::BLF_Discard, 0, sizeof( SWaterConstants ) );
	if ( constantData )
	{
		SWaterConstants* waterConstants = static_cast< SWaterConstants* >( constantData );

		// Global
		{
			const Float globalTesselationFactor = Clamp( (Float) Config::cvGlobalOceanTesselationFactor.Get(), 4.0f, 64.0f );
			const Float globalTesselationFactorNormalized = globalTesselationFactor/64.0f;

			waterConstants->GlobalWater.uvScale = m_uvScale;
			waterConstants->GlobalWater.amplitudeScale = m_amplitudeScale;
			waterConstants->GlobalWater.tessFactor = globalTesselationFactor;
			waterConstants->GlobalWater.dynamicWaterResolution = DYNAMIC_WATER_RESOLUTION;
			waterConstants->GlobalWater.dynamicWaterResolutionInv = 1.0f/DYNAMIC_WATER_RESOLUTION;

			Vector choppyFactors = Vector::ZEROS;
			// DS (X = large, Y = medium)
			CalcBezier4v(globalTesselationFactorNormalized, 0.15f, 0.16f, 0.2f, 0.19f, choppyFactors.X);
			CalcBezier4v(globalTesselationFactorNormalized, 0.1f, 0.1f, 0.17f, 0.16f, choppyFactors.Y);
			// PS (Z = large, W = medium)
			CalcBezier4v(globalTesselationFactorNormalized, 12.0f, 8.0f, 4.0f, 2.0f, choppyFactors.Z);
			CalcBezier4v(globalTesselationFactorNormalized, 10.0f, 7.2f, 5.0f, 2.0f, choppyFactors.W);

			waterConstants->GlobalWater.choppyFactors = choppyFactors;
		}
		
		// Camera
		{
			waterConstants->SimulationCamera.position = m_dynamicsimulator.GetSnappedCameraPosition();
			waterConstants->SimulationCamera.farPlane = frameInfo.m_camera.GetFarPlane();
			waterConstants->SimulationCamera.nearPlane = frameInfo.m_camera.GetNearPlane();
			const Float ratio = frameInfo.m_camera.GetAspect();
			const Float fov  = frameInfo.m_camera.GetFOV();
			const Float tanFov = tan(fov*( M_PI/180.0f)*0.5f);
										
			waterConstants->SimulationCamera.tanFov = tanFov;
			waterConstants->SimulationCamera.tanFov_x_ratio = (tanFov*ratio);
		}

		// Lakes
		{
			waterConstants->Lakes.numLakes = m_numLakes;
			Red::System::MemoryCopy( &(waterConstants->Lakes.offsets[0]), &m_lakes[0], NUM_LAKES_MAX * sizeof( Vector ) );
		}

		// Displacements (boats)
		{
			waterConstants->Displacements.numDispl = m_numBoats;
			Red::System::MemoryCopy( &(waterConstants->Displacements.displacementData[0]), &m_displacements[0], NUM_DISPLACEMENTS_MAX * sizeof( Matrix ) );
		}

		GpuApi::UnlockBuffer( m_constantBuffer );
	}
}

IRenderProxy* CRenderInterface::CreateWaterProxy()
{	
	return new CRenderProxy_Water();
}
