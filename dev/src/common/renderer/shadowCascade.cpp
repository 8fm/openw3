/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "shadowCascade.h"
#include "renderMeshBatcher.h"
#include "../engine/renderFragment.h"
#include "../engine/renderSettings.h"

#ifdef USE_ANSEL
extern Float anselLastFov;
extern Bool isAnselCaptureActive;
#endif // USE_ANSEL

namespace Config
{
	TConfigVar<Float> cvCascadeShadowsDegreesLimit		( "Rendering", "CascadeShadowsDegreesLimit",		15 );
}

/////////////////////////////////////////////////////////////////////////////
// Cascades
/////////////////////////////////////////////////////////////////////////////

static Float ProjectZ( const CRenderCamera& camera, Float zDist )
{
	Vector projZ = camera.GetViewToScreen().TransformVectorWithW( Vector( 0,0,zDist,1) );
	return projZ.Z / projZ.W;
}

static void CalculateBestCascadeParams( const CCascadeShadowResources &cascadeShadowResources, const Vector* corners, Vector &outCenter, Float &outRadius )
{	
	// find minimum enclosing sphere for current frustum slice
	// slice is symmetric, we can find circumcircle for diagonal triangle
	Vector sliceCenter;
	Float sliceRadius = 0.0f;

	{
		const Vector &v1 = corners[ 1 ];
		const Vector &v2 = corners[ 3 ];
		const Vector &v3 = corners[ 5 ];

		Float denom = 1.0f / ( 2.0f * Vector::Cross( v1 - v2, v2 - v3 ).SquareMag3() );
		float bary0 = denom * ( v2 - v3 ).SquareMag3() * Vector::Dot3( v1 - v2, v1 - v3 );
		float bary1 = denom * ( v1 - v3 ).SquareMag3() * Vector::Dot3( v2 - v1, v2 - v3 );
		float bary2 = denom * ( v1 - v2 ).SquareMag3() * Vector::Dot3( v3 - v1, v3 - v2 );

		sliceCenter = v1 * bary0 + v2 * bary1 + v3 * bary2;
		sliceRadius = ( v1 - sliceCenter ).Mag3();
	}

	// extend it a bit to take snapping and filtering into account
	{
		Float worldTexelSize = ( 2.0f * sliceRadius ) / (Float)Max( (Uint32)1, cascadeShadowResources.GetResolution() );
		sliceRadius = ceilf( sliceRadius + 4 * worldTexelSize );
	}

	// finish
	outCenter = sliceCenter;
	outRadius = sliceRadius;
}

#define FIT_TO_CASCADES

//dex++: frustum locking (debug)
static bool IsShadowFrustumLocked = false;
static CRenderCamera ShadowLockedCamera;
static CRenderCamera ShadowCascadeCameras[ MAX_CASCADES ];
static Uint32 ShadowCascadeNumCameras = 0;

void GetFrustumCornersForBox( const CRenderCamera& camera, const Float plane, Vector* corners )
{
	// Calc corners for given plane
	corners[0] = camera.GetScreenToWorld().TransformVectorWithW( Vector(-1,-1,plane) );
	corners[1] = camera.GetScreenToWorld().TransformVectorWithW( Vector(+1,-1,plane) );
	corners[2] = camera.GetScreenToWorld().TransformVectorWithW( Vector(-1,+1,plane) );
	corners[3] = camera.GetScreenToWorld().TransformVectorWithW( Vector(+1,+1,plane) );
	corners[0].Div4( corners[0].W );
	corners[1].Div4( corners[1].W );
	corners[2].Div4( corners[2].W );
	corners[3].Div4( corners[3].W );
}

void DrawCascadeFrustums( CRenderFrame* frame )
{
	if ( IsShadowFrustumLocked )
	{
		for ( Uint32 i=0; i<ShadowCascadeNumCameras; ++i )
		{
			const CRenderCamera& camera = ShadowCascadeCameras[i];

			Vector coords[8];
			GetFrustumCornersForBox( camera, 0.0f, coords + 0 );
			GetFrustumCornersForBox( camera, 1.0f, coords + 4 );

			DebugVertex vertices[8];
			Color color( 255, 0, 0, 32 );
			if ( i==1 ) color = Color( 0, 255, 0, 32 );
			if ( i==2 ) color = Color( 0, 0, 255, 32 );
			if ( i==3 ) color = Color( 255, 255, 255, 32 );
			vertices[0].Set( coords[0], color );
			vertices[1].Set( coords[1], color );
			vertices[2].Set( coords[2], color );
			vertices[3].Set( coords[3], color );
			vertices[4].Set( coords[4], color );
			vertices[5].Set( coords[5], color );
			vertices[6].Set( coords[6], color );
			vertices[7].Set( coords[7], color );

			Uint16 indices[72];
			Uint16 *write = &indices[0];
#define FACE( a, b, c, d ) { *write++ = a; *write++ = b; *write++ = c; *write++ = a; *write++ = c; *write++ = d; *write++ = c; *write++ = b; *write++ = a; *write++ = d; *write++ = c; *write++ = a; } 
			FACE( 0, 1, 3, 2 );
			FACE( 4, 5, 7, 6 );
			FACE( 0, 1, 5, 4 );
			FACE( 2, 3, 7, 8 );
			FACE( 1, 3, 7, 5 );
			FACE( 0, 2, 6, 4 );
#undef FACE

			new ( frame ) CRenderFragmentDebugPolyList( 
				frame, 
				Matrix::IDENTITY, 
				vertices, 8,
				indices, 72, RSG_DebugTransparent );
		}
	}
}

// ---

SShadowCascade::SShadowCascade( const CRenderCamera& camera )
	: m_camera( camera )
	, m_edgeFade( 0.2f )
{}

SShadowCascade::SShadowCascade( )
{}

SShadowCascade::~SShadowCascade( )
{}

void SShadowCascade::Reset()
{
	m_mergedSolidElements.ClearFast();
	m_solidElements.ClearFast();
	m_discardElements.ClearFast();
	m_apexElements.ClearFast();
}

// ---

const Uint16 SMergedShadowCascades::MAX_NORMAL_CASCADES = 2;
const Float SMergedShadowCascades::MIN_FILTER_SIZE = 1.0f;

SMergedShadowCascades::SMergedShadowCascades()
	: m_numCascades( 0 )
	, m_collector( nullptr )
	, m_frustumJitter( 0 )
{}

void SMergedShadowCascades::Reset()
{
	m_collector = nullptr;
	m_numCascades = 0;
	for ( Uint32 i=0; i<MAX_CASCADES; ++i )
	{
		m_cascades[i].Reset();
	}
}

void SMergedShadowCascades::Init( CRenderCollector* collector, const CRenderFrameInfo &info, const CCascadeShadowResources &cascadeShadowResources )
{
	Reset();
	CalculateData( info, cascadeShadowResources );
	m_collector = collector;
}

Vector CalculateLimitedShadowDirection( const Vector &shadowDir, Float degreesLimit )
{
	Vector limitedShadowDir = shadowDir;

	if ( degreesLimit >= 0 && shadowDir.SquareMag3() > 0.1f )
	{
		limitedShadowDir.Normalize3();

		const Float xyLen = sqrtf(shadowDir.X*shadowDir.X + shadowDir.Y*shadowDir.Y);
		const Float angle = atan2( xyLen, shadowDir.Z );
		const Float limitAngle = DEG2RAD( 90 + degreesLimit );
		if ( angle < limitAngle && xyLen > 0.001f )
		{
			Float s = sinf( limitAngle );
			Float c = cosf( limitAngle );
			limitedShadowDir.X = shadowDir.X / xyLen * s;
			limitedShadowDir.Y = shadowDir.Y / xyLen * s;
			limitedShadowDir.Z = c;
			RED_ASSERT( shadowDir.IsNormalized3() );
		}
	}

	return limitedShadowDir;
}

void SMergedShadowCascades::CalculateData( const CRenderFrameInfo &info, const CCascadeShadowResources &cascadeShadowResources )
{
	PC_SCOPE_PIX( CalculateCascadesData );

	// Reset
	Reset();

	// Assume no shadows
	m_numCascades = 0;

	// Calculate shadow dir
	const Vector shadowDir = CalculateLimitedShadowDirection( -info.m_baseLightingParameters.m_lightDirection, Config::cvCascadeShadowsDegreesLimit.Get() );
	
	// Build cascades
	if ( info.IsShowFlagOn( SHOW_Shadows ) )
	{
		CRenderCamera renderCamera = info.m_camera;

		// Split array, take into consideration cascades that are closer that camera far plane
		Uint16 effectiveCascadesNum = 0;
		const Float farPlane = renderCamera.GetFarPlane();
		for ( Uint32 i = 0; i < info.m_requestedNumCascades; ++i )
		{
			++effectiveCascadesNum;
			if( farPlane < info.GetCascadeDistance( i ) ) break;
		}

		const Uint32 numSplits = Clamp<Uint32>( effectiveCascadesNum, 1, Min<Uint32>( MAX_CASCADES, cascadeShadowResources.GetMaxCascadeCount() ) );

		LookMatrix2 lookAtMatrix( shadowDir, Vector::EZ ); // with this we should be able to make a tighter frustum than with the spheres
		EulerAngles cameraRotation = lookAtMatrix.ToEulerAngles();

		// Build cascade parameters - basicly spheres around the cascade parts
		Vector cascadeCenters[ MAX_CASCADES ];
		Float cascadeRadiuses[ MAX_CASCADES ];
		Float prevDistance = 0.1f;
		for ( Uint32 i=0; i<numSplits; i++ )
		{ 
			// Extract slice frustum range
			Vector corners[8];
#ifdef USE_ANSEL
			if ( isAnselCaptureActive )
			{
				CRenderCamera nonTiledCamera = renderCamera;
				nonTiledCamera.SetFOV( anselLastFov );
				nonTiledCamera.SetSubpixelOffset( 0, 0, 2, 2 );

				nonTiledCamera.GetFrustumCorners( ProjectZ( nonTiledCamera, prevDistance ), corners+0, true );
				nonTiledCamera.GetFrustumCorners( ProjectZ( nonTiledCamera, info.GetCascadeDistance( i ) ), corners+4, true );
			}
			else
#endif // USE_ANSEL
			{
				renderCamera.GetFrustumCorners( ProjectZ( renderCamera, prevDistance ), corners+0, true );
				renderCamera.GetFrustumCorners( ProjectZ( renderCamera, info.GetCascadeDistance( i ) ), corners+4, true );
			}

			// Build camera parameters
			CalculateBestCascadeParams( cascadeShadowResources, corners, cascadeCenters[i], cascadeRadiuses[i] );
			cascadeCenters[i] += renderCamera.GetPosition() * Vector ( 1.f, 1.f, 1.f, 0.f );
		}

		// Snap parameters to eliminate shadows flickering
		{
			const Vector currLightDirSide = lookAtMatrix.GetAxisX();
			const Vector currLightDirUp = lookAtMatrix.GetAxisZ();
			const Float sh_size = (Float) Max<Uint32>( 1, cascadeShadowResources.GetResolution() );
			const Float sh_invSize = 1.f / sh_size;

			bool isLightDirConst = !info.IsShowFlagOn( SHOW_CascadesStabilizedRotation );

			// Force const snapping in non main scenes, because we are unable to differentiate global light instances, 
			// and some static vars are used for nonConst snapping.
			if ( !info.m_isWorldScene ) 
			{
				isLightDirConst = true;
			}

			// Snap multiplier is here because of shadows dither. Specifically speedTree shadow dither (we're having 4x4 kernel in there).
			// The downside of this is that cascades movement is a bit less smooth (every 4 texels instead of 1), but it seems good enough.
			// Btw. this could albo be done in the shaders, but I don't have time for this right now.
			const Float snapMultiplier = 4;

			if ( isLightDirConst )
			{
				for ( Uint32 seg_i=0; seg_i<numSplits; ++seg_i )
				{
					Float  &curr_radius = cascadeRadiuses[seg_i];
					Vector &curr_center = cascadeCenters[seg_i];

					Float snap_dist = snapMultiplier * 2.f * curr_radius / sh_size;
					Float dx = Vector::Dot3( currLightDirSide, curr_center ) / snap_dist;
					Float dy = Vector::Dot3( currLightDirUp, curr_center ) / snap_dist;
					dx = (dx - floorf(dx)) * snap_dist;
					dy = (dy - floorf(dy)) * snap_dist;
					curr_center -= currLightDirSide * dx + currLightDirUp * dy;
					curr_center.W = 1;
				}
			}
			else
			{
				struct SSnapData
				{
					SSnapData ()
					{
						Reset();
					}

					void Reset()
					{
						lightDirSide = Vector ( 1, 0, 0, 0 );
						lightDirUp = Vector ( 0, 1, 0, 0 );
						snapAccum[0] = 0;
						snapAccum[1] = 0;
					}

					Vector lightDirSide;
					Vector lightDirUp;
					Float snapAccum[2];
				};

				static SSnapData snap_data[MAX_CASCADES];

				const Vector currLightDirSide = lookAtMatrix.GetAxisX();
				const Vector currLightDirUp = lookAtMatrix.GetAxisZ();
				for ( Uint32 seg_i=0; seg_i<numSplits; ++seg_i )
				{
					const Float curr_radius = cascadeRadiuses[seg_i];
					SSnapData &seg_rdata = snap_data[seg_i];

					const Float unit_size = (snapMultiplier * 2.f * Max( 0.0001f, curr_radius)) * sh_invSize;
					const Float unit_invSize = 1.f / unit_size;

					Vector snap_origin = cascadeCenters[seg_i];

					float dx = Vector::Dot3( currLightDirSide, snap_origin );
					float dy = Vector::Dot3( currLightDirUp, snap_origin );
					float p_dx = Vector::Dot3( seg_rdata.lightDirSide, snap_origin );
					float p_dy = Vector::Dot3( seg_rdata.lightDirUp, snap_origin );
					seg_rdata.snapAccum[0] += dx - p_dx;
					seg_rdata.snapAccum[1] += dy - p_dy;
					for (int i=0; i<2; ++i)
					{
						// snap accumulation if it gets too big to prevent precision issues
						float &acc = seg_rdata.snapAccum[i];
						if (fabsf(acc) > unit_size )
							acc -= unit_size*floorf(acc*unit_invSize);
					}
					seg_rdata.lightDirSide = currLightDirSide;
					seg_rdata.lightDirUp = currLightDirUp;

					dx -= seg_rdata.snapAccum[0];
					dy -= seg_rdata.snapAccum[1];
					dx = dx - unit_size*(floorf(dx*unit_invSize));
					dy = dy - unit_size*(floorf(dy*unit_invSize));

					const Vector centerOffset = -(currLightDirSide * dx + currLightDirUp * dy);

					// Guard for nan's in case incoming data is broken.
					// We don't want such stuff to permanently brake shadows.
					const Float guardValue = centerOffset.Dot3(Vector::ONES);
					if ( Red::Math::NumericalUtils::IsNan( guardValue ) || !Red::Math::NumericalUtils::IsFinite( guardValue ) )
					{
						RED_ASSERT( !"Broken cascades stabilization - please make sure input data is valid" );
						seg_rdata.Reset();
					}
					else
					{
						Vector &curr_center = cascadeCenters[seg_i];
						curr_center += centerOffset;
						curr_center.W = 1;
					}
				}
			}
		}

		// Cascade distances
		Float cascadeDistances[ MAX_CASCADES ];
		for ( Uint32 i=0; i<numSplits; ++i )
		{
			cascadeDistances[i] = Vector::Dot3( shadowDir, cascadeCenters[i] );
		}

		// Calculate min/max distance for Z near/far
		Float cascadeDistancesMin = cascadeDistances[0];
		Float cascadeDistancesMax = cascadeDistances[0];
		for ( Uint32 i=1; i<numSplits; ++i )
		{
			cascadeDistancesMin = Min( cascadeDistancesMin, cascadeDistances[i] );
			cascadeDistancesMax = Max( cascadeDistancesMax, cascadeDistances[i] );
		}
		
		// Generate cascade frames
		if ( numSplits )
		{
			// Collect all cascade info
			for ( Uint32 i=0; i<numSplits; i++ )
			{
				//We need these to give the same range
				//TODO: this range should not be finite...
				const Float  planeNear = -1000 - (cascadeDistances[i] - cascadeDistancesMin);
				const Float  planeFar  = +1000 + (cascadeDistancesMax - cascadeDistances[i]);

				CRenderCamera shadowCamera = CRenderCamera( cascadeCenters[i], 
					cameraRotation, 
					0.0f, 1.0f, 
					planeNear, planeFar, 
					2.0f * cascadeRadiuses[i] ); //this shouldn't be called zoom

				// allocate local cascade
				ASSERT( m_numCascades < MAX_CASCADES );
				SShadowCascade* cascade = &m_cascades[ m_numCascades ];
				m_numCascades += 1;

				// remember cameras ( for debug )
				if ( IsShadowFrustumLocked )
				{
					ShadowCascadeNumCameras = numSplits;
					ShadowCascadeCameras[i] = shadowCamera;
				}

				// write to local cascades
				cascade->m_camera = shadowCamera;

				// calculate shadowmap pixel size
				const Float viewportSize	= 2.0f / shadowCamera.GetViewToScreen().V[0].X;

				cascade->m_pixelSize		= viewportSize / (Float)Max<Uint32>( 1, cascadeShadowResources.GetResolution() );
				cascade->m_invPixelSize		= 1.0f / cascade->m_pixelSize;
				cascade->m_cascadeIndex		= (Uint8)i;
				cascade->m_edgeFade			= info.m_shadowEdgeFade[i] / viewportSize;

				// initialize frustum
				if( m_frustumJitter )
				{
					const Float	 jitterRadius = 2.0f * ( cascadeRadiuses[i] - info.m_shadowEdgeFade[i] );

					cascade->m_jitterCamera = CRenderCamera( cascadeCenters[i], 
						cameraRotation, 
						0.0f, 1.0f, 
						planeNear, planeFar, 
						jitterRadius ); //this shouldn't be called zoom

				}
				else
				{
					cascade->m_jitterCamera = cascade->m_camera;
				}

				m_frustums[i].InitFromCamera( cascade->m_jitterCamera.GetWorldToScreen() );
			}
		}

		// Flip jitter in next frame
		m_frustumJitter = 1 - m_frustumJitter;

	}
}
