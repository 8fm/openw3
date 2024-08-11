/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "globalWater.h"
#include "globalWaterUpdateParams.h"
#include "renderCommands.h"
#include "weatherManager.h"
#include "dynamicColliderComponent.h"
#include "dynamicCollisionCollector.h"
#include "entity.h"
#include "clipMap.h"
#include "textureArray.h"
#include "waterComponent.h"
#include "renderProxy.h"
#include "environmentDefinition.h"
#include "../core/gatheredResource.h"
#include "../core/dataError.h"
#include "world.h"
#include "worldIterators.h"
#include "utils.h"

IMPLEMENT_ENGINE_CLASS( CGlobalWater );  

RED_DEFINE_STATIC_NAME( Sailing );

CGatheredResource globalWaterControlArray( TXT("environment\\water\\global_ocean\\ocean_control.texarray"), RGF_Startup );

static inline void CalcBezier5v(Float t, Float v0, Float v1, Float v2, Float v3, Float v4, Float &out)
{
	out =	powf(t,4) * (v0 - 4*v1 + 6*v2 - 4*v3 + v4) +
			4*powf(t,3) * (-v0 + 3*v1 - 3*v2 + v3) + 
			6*powf(t,2) * (v0 - 2*v1 + v2) +
			4*t*(-v0 + v1) + 
			v0;
}

#define GLOBAL_WATER_BURST_APPROXIMATION_LOD1 400.0f
#define GLOBAL_WATER_BURST_APPROXIMATION_LOD2 1225.0f
#define GLOBAL_WATER_BURST_APPROXIMATION_LOD3 14400.0f

CGlobalWater::CGlobalWater()
{
}

CGlobalWater::CGlobalWater( CWorld* world )
{	
	m_debugFrame = 0;
	m_debugAccess = 0;
	m_surpassWaterRender = false;

	m_waterMaxLevel = WATER_DEFAULT_LEVEL;

	m_numOfLocalShapesThatNeedsUpdate = 0;

	ASSERT( world );
	m_world = world;
	m_dynamicCollector = world->GetDynamicCollisionsCollector();
	ASSERT( m_dynamicCollector );

	// load default water resources here
	m_waterShaderControlTexture = globalWaterControlArray.LoadAndGet< CTextureArray >();
	ASSERT( m_waterShaderControlTexture, TXT("No default water control array found! Water shaders will be turned off") );

	m_localShapesParams = nullptr;

	m_clipMap = m_world->GetTerrain();

	m_waterHeightmap = new CGlobalWaterHeightmap();

	m_waterProxy = GRender->CreateWaterProxy();
}

void CGlobalWater::OnLocalShapeAttached( CWaterComponent* comp, Bool propertyChanged )
{
	// just force recreate // attach / detach recreate only
	if( propertyChanged || m_computedShapes.Find( comp->GetEntity()->GetGUID() ) != m_computedShapes.End() )
	{
		m_numOfLocalShapesThatNeedsUpdate++;
	}	
}

void CGlobalWater::GenerateLocalShape( CWaterComponent* component, Uint32 shapeIndex )
{
	// first obtain texture pointer
	Float* localShapeBuffer = m_localShapesParams->m_shapesBuffer + (WATER_LOCAL_RESOLUTION*WATER_LOCAL_RESOLUTION*shapeIndex);
	Float* localMatrix = m_localShapesParams->m_matrices + (shapeIndex*4);

	// generate texture for this area (not using bbox because it can be rotated, and we need to have straight XY orientation)
	const CAreaComponent::TAreaPoints& points = component->GetWorldPoints();

	Box& bounds = m_localShapesBounds[ shapeIndex ];
	bounds = Box( Box::RESET_STATE );

	// find min / max for XY
	if( !points.Empty() )
	{
		for( const Vector& p : points )
		{
			bounds.AddPoint( p );
		}

		RED_ASSERT( bounds.Max.X > bounds.Min.X && bounds.Max.Y > bounds.Min.Y, TXT("LocalWaterArea shape SIZE = 0.0, FIX the area: %s"), component->GetFriendlyName().AsChar() );

		Float bbSizeX = Max( bounds.Max.X - bounds.Min.X, 0.001f );
		Float bbSizeY = Max( bounds.Max.Y - bounds.Min.Y, 0.001f );

		localMatrix[0] = bounds.Min.X;
		localMatrix[1] = bounds.Min.Y;
		localMatrix[2] = bbSizeX;
		localMatrix[3] = bbSizeY;

		// do not update pixels at the borders - force them to be zeros
		for ( Uint32 x=1; x<WATER_LOCAL_RESOLUTION-1; x++ )
		{
			for ( Uint32 y=1; y<WATER_LOCAL_RESOLUTION-1; y++ )
			{
				Vector currentWorldPosition;
				currentWorldPosition.X = bounds.Min.X + ( bbSizeX * (Float(x)/Float(WATER_LOCAL_RESOLUTION)) );
				currentWorldPosition.Y = bounds.Min.Y + ( bbSizeY * (Float(y)/Float(WATER_LOCAL_RESOLUTION)) );
				currentWorldPosition.Z = 0.0f; // does not matter for GetFloorLevel

				// get the floor level for water area
				Float waterLevelZ = WATER_DEFAULT_LEVEL;
				if ( component->GetFloorLevel( currentWorldPosition, waterLevelZ ) )
				{
					if( waterLevelZ > m_waterMaxLevel )
					{
						m_waterMaxLevel = waterLevelZ;
					}
					if( waterLevelZ<m_localShapesParams->m_shapeWaterLevel_min )
					{
						m_localShapesParams->m_shapeWaterLevel_min = waterLevelZ;
					}
					if( waterLevelZ>m_localShapesParams->m_shapeWaterLevel_max )
					{
						m_localShapesParams->m_shapeWaterLevel_max = waterLevelZ;
					}
					// we have a valid height at this position, save it as the water height
					localShapeBuffer[ x + y*WATER_LOCAL_RESOLUTION ] = waterLevelZ;
				}
			}
		}
	}
}

void CGlobalWater::GenerateLocalShapes()
{
	SAFE_RELEASE( m_localShapesParams );
	m_localShapesBounds.ClearFast();

	m_numOfLocalShapesThatNeedsUpdate = 0;


	// all water component shapes should be loaded at this point
	if ( m_world )
	{
		m_computedShapes.ClearFast();

		TDynArray< CWaterComponent* > localShapes;
		m_world->GetAttachedComponentsOfClass< CWaterComponent >( localShapes );
		
		if ( !localShapes.Empty() )
		{
			Uint32 localShapesNum = localShapes.Size();

			// Make sure we don't overflow
			if ( localShapesNum > NUM_LAKES_MAX )
			{
				localShapesNum = NUM_LAKES_MAX;

				if( localShapes[ localShapesNum - 1 ] != nullptr )
				{
					RED_HALT( "You have exceeded limit of Local Water Areas, some of them WILL NOT be loaded. Last loaded shape is: %s", localShapes[ localShapesNum - 1 ]->GetFriendlyName().AsChar() );
				}
				else
				{
					RED_HALT( "You have exceeded limit of Local Water Areas, some of them WILL NOT be loaded." );
				}

				DATA_HALT( DES_Uber, CResourceObtainer::GetResource( localShapes[ localShapesNum - 1 ] ), TXT("Local Water Areas"), TXT("You have exceeded limit of Local Water Areas, some of them WILL NOT be loaded.") );
			}


			m_localShapesParams = new CLocalWaterShapesParams( localShapesNum );
			m_localShapesBounds.Resize( localShapesNum );
					
			// create texture array that holds all the shapes
			for( Uint32 i=0; i < localShapesNum; i++ )
			{
				m_computedShapes.PushBack( localShapes[ i ]->GetEntity()->GetGUID() );

				GenerateLocalShape( localShapes[ i ], i );
			}			

			m_localShapesParams->m_waterMaxLevel = m_waterMaxLevel;

			m_computedShapes.Sort();


			if ( m_waterProxy )
			{
				( new CRenderCommand_AddWaterProxyLocalShape( m_waterProxy, m_localShapesParams ) )->Commit();
			}
		}

		NotifyTerrainOfLocalWaterChange();
	}
}

void CGlobalWater::Cooker_IncrementalShapeAddition( CWaterComponent* comp )
{
	if ( !m_localShapesParams )
	{
		m_localShapesParams = new CLocalWaterShapesParams( 1 );
	}
	else
	{
		m_localShapesParams->PushNewShape();
	}

	m_localShapesBounds.Grow( 1 );
	GenerateLocalShape( comp, m_localShapesParams->m_numShapes-1 );

	NotifyTerrainOfLocalWaterChange();
}


void CGlobalWater::NotifyTerrainOfLocalWaterChange()
{
	if ( m_world && m_world->GetTerrain() )
	{
		m_world->GetTerrain()->UpdateWaterLevels( WATER_DEFAULT_LEVEL, m_localShapesBounds );
		m_needTerrainNotify = false;
	}
	else
	{
		m_needTerrainNotify = true;
	}
}


void CGlobalWater::Setup( IRenderScene* irs )
{
	if( m_waterProxy && m_waterShaderControlTexture )
	{
		( new CRenderCommand_SetWaterProxyToScene( irs, m_waterProxy ) )->Commit();
		( new CRenderCommand_UpdateWaterProxy( m_waterProxy, m_waterShaderControlTexture->GetRenderResource() ) )->Commit();

		// force lakse update on next frame, needed since user can enable / diable water shaders anytime from filters
		m_numOfLocalShapesThatNeedsUpdate++;
	}
}

void CGlobalWater::DestroyProxy( IRenderScene* irs )
{
	if( m_waterProxy )
	{
		( new CRenderCommand_RemoveWaterProxyFromScene( irs, m_waterProxy ) )->Commit();	
		m_waterProxy->Release();
		m_waterProxy = nullptr;
	}
}

CGlobalWater::~CGlobalWater()
{
	SAFE_RELEASE( m_waterHeightmap );
	SAFE_RELEASE( m_localShapesParams );
}

#ifdef USE_ANSEL
extern Bool isAnselSessionActive;
#endif // USE_ANSEL

void CGlobalWater::Update( Float deltaTime )
{
	if ( m_waterProxy == nullptr || ( deltaTime == 0.0f
#ifdef USE_ANSEL
		 && !isAnselSessionActive 
#endif // USE_ANSEL
		) )
	{
		return;
	}

	CGlobalWaterUpdateParams* updateParams = new CGlobalWaterUpdateParams( m_waterHeightmap );
	// needs opt
	Simulate( deltaTime, updateParams );
	( new CRenderCommand_SimulateWaterProxy( m_waterProxy, updateParams ) )->Commit();
	SAFE_RELEASE( updateParams );

	if( m_numOfLocalShapesThatNeedsUpdate > 0 )
	{
		GenerateLocalShapes();
	}

	if ( m_needTerrainNotify )
	{
		NotifyTerrainOfLocalWaterChange();
	}
}

void CGlobalWater::Simulate( Float deltaTime, CGlobalWaterUpdateParams* updateParams )
{
	if ( m_world )
	{		
		// rendering actually can be disabled by scene / cutscene or any other game system
		updateParams->m_shouldRenderWater = !m_surpassWaterRender;

		const CRenderCamera& camera = GGame->GetCachedFrameInfo().m_camera;
		const Vector camPos = camera.GetPosition();

		if( updateParams->m_shouldRenderWater )
		{
			Float wdir = 0.0f;
			Float wspeed = 10.0f;
			Float a = 1.0f;
			Float lambda = 0.00023f;
			Float currWindScale = 0.01f;

			// set wind influence based on weather conditions	
			CEnvironmentManager* cEnvMgr = m_world->GetEnvironmentManager();
			if( cEnvMgr ) 
			{
				CWeatherManager* cWMgr = cEnvMgr->GetWeatherManager();
				if( cWMgr != nullptr )
				{
					updateParams->m_rainIntensity = cWMgr->GetEffectStrength( EWeatherEffectType::WET_RAIN );
					currWindScale = cWMgr->GetWindScale();
					wdir = cWMgr->GetWindRotationZ();
				}
			}		

			CalcBezier5v( currWindScale, 4.05f, 5.6f, 8.2f, 13.2f, 17.5f, wspeed );

			updateParams->m_phillipsData.X = wdir;
			updateParams->m_phillipsData.Y = wspeed;
			updateParams->m_phillipsData.Z = a;
			updateParams->m_phillipsData.W = lambda;

			// this is needed for normal map generation from height map
			updateParams->m_amplitudeScale.W = currWindScale;			

			if( m_world->GetEnvironmentParameters().m_localWaterVisibility )
			{
				if( m_world->GetEnvironmentParameters().m_localWaterVisibility->Get().FindIDAtPoint( camPos.X, camPos.Y ) == 0 ) updateParams->m_shouldRenderWater = false;
			}

			if( GetWaterLevelAccurate( camPos.X, camPos.Y ) > camPos.Z - 0.5f ) updateParams->m_shouldRenderUnderwater = true;
			else
				updateParams->m_shouldRenderUnderwater = false;

			// still rendering - after simplex areas check
			if( updateParams->m_shouldRenderWater )
			{
				m_activeNormalCollisions = m_dynamicCollector->GetActiveColliders( DYNAMIC_COLLIDERS_TYPE_WATER_NORMAL );
				// water waves, splashes
				updateParams->m_impulses.Reserve( m_activeNormalCollisions.Size() );
				for( Uint32 i=0; i<m_activeNormalCollisions.Size(); ++i )
				{			
					if( m_activeNormalCollisions[i].m_intensity > 0.001f )
					{
						Vector translation = m_activeNormalCollisions[i].m_transformMatrix.GetRow(3);
						Float z = GetWaterLevelAccurate( translation.X, translation.Y );

						Float dist = Abs( m_activeNormalCollisions[i].m_transformMatrix.GetRow(3).Z - z );

						if( dist < 0.6f ) // 0.05f + 0.5f*m_activeNormalCollisions[i].m_scale.Z ) // this makes no sense
						{
							Float velocity = m_activeNormalCollisions[i].m_transformMatrix.GetRow(3).W;

							if( velocity*0.005f > 0.00001f )
							{
								updateParams->m_impulses.PushBack( m_activeNormalCollisions[i] );
							}
						}
					}
				}
				// update displ vectors
				updateParams->m_displCollisions = m_dynamicCollector->GetActiveColliders( DYNAMIC_COLLIDERS_TYPE_WATER_DISPLACEMENT );
			}			
		}	
		
		// update these regardless if we are rendering
		updateParams->m_gameTime = GGame->GetCleanEngineTime();
		updateParams->m_camera = camPos;
	}
}
// keep in sync with globalOceanConstants.fx
Vector CGlobalWater::GetWaterShoreProximityDampValue( Float hmapHeight, Float referenceWaterLevel ) const
{	
	const Vector2 highWavesRange = Vector2 (0.1f, 5.0f);
	const Vector2 mediumWavesRange = Vector2 (0.1f, 2.0f);	

	Float waterDepth = Abs(referenceWaterLevel - hmapHeight);
	Float highWaterRange = Clamp<Float>( (waterDepth - highWavesRange.X)/(highWavesRange.Y - highWavesRange.X), 0.0f, 1.0f );
	Float mediumWaterRange = Clamp<Float>( (waterDepth - mediumWavesRange.X)/(mediumWavesRange.Y - mediumWavesRange.X), 0.0f, 1.0f);	

	return Vector( Lerp( highWaterRange, 0.3f, 1.0f ), Lerp( mediumWaterRange, 0.6f, 1.0f ), 1.0f, 0.0f );
}

Bool CGlobalWater::GetWaterLevelBurst( Uint32 elementsCount, void* inputPos, void* outputPos, void* outputHeightDepth, size_t stride, Vector referencePosition, Bool useApproximation ) const
{

	if ( !m_waterHeightmap->IsInit() )
	{
		return false;
	}

	if(!m_clipMap)
	{
		return false;
	}

	Float hmapHeight = WATER_DEFAULT_LEVEL;

	const Float scaleLarge  = 1.0f / CGlobalWaterUpdateParams::DefaultUVScale.X;
	const Float scaleMedium = 1.0f / CGlobalWaterUpdateParams::DefaultUVScale.Y;

	const Float amplitudeLarge  = CGlobalWaterUpdateParams::DefaultAmplitudeScale.X;
	const Float amplitudeMedium = CGlobalWaterUpdateParams::DefaultAmplitudeScale.Y;

	for( Uint32 i = 0; i != elementsCount; ++i )
	{
		float* X = ( float* ) ( ( char* ) inputPos + stride * i );
		float* Y = X + 1;

		Float refX = referencePosition.X - (*X);
		Float refY = referencePosition.Y - (*Y);
		Float sqrDist = (refX*refX + refY*refY);				

		float* output = ( float* ) ( ( char* )outputPos + stride * i );

		Float referenceWaterLevel = 0.0f;
				
		// accurate check
		if( sqrDist < GLOBAL_WATER_BURST_APPROXIMATION_LOD1 || !useApproximation )
		{
			if( outputHeightDepth )
			{
				Vector* forThatWorldPosition = ( Vector* ) ( ( char* ) inputPos + stride * i );
				m_clipMap->GetHeightForWorldPosition( *forThatWorldPosition, hmapHeight );

				float* heightDepth = ( float* ) ( ( char* ) outputHeightDepth + stride * i );
				*heightDepth = hmapHeight;
			}

			Float heightLarge  = amplitudeLarge  * m_waterHeightmap->GetHeight( *X * scaleLarge, *Y * scaleLarge );
			Float heightMedium = amplitudeMedium * m_waterHeightmap->GetHeight( *X * scaleMedium, *Y * scaleMedium );
						

			referenceWaterLevel = GetReferenceWaterLevel(*X, *Y);			
			Vector shoreProximity = GetWaterShoreProximityDampValue( hmapHeight, referenceWaterLevel );
			
			*output = shoreProximity.X*heightLarge + shoreProximity.Y*heightMedium + referenceWaterLevel;
		}
		// approximate check
		else if( sqrDist < GLOBAL_WATER_BURST_APPROXIMATION_LOD2 )
		{
			Float heightLarge  = amplitudeLarge  * m_waterHeightmap->GetHeight( *X * scaleLarge, *Y * scaleLarge );
			Float h = heightLarge;

			referenceWaterLevel = GetReferenceWaterLevel(*X, *Y);
			*output = h + referenceWaterLevel;
		}
		// simple check
		else if( sqrDist < GLOBAL_WATER_BURST_APPROXIMATION_LOD3 )
		{
			referenceWaterLevel = GetReferenceWaterLevel(*X, *Y);
			*output = referenceWaterLevel;
		}
		else
		{
			*output = WATER_DEFAULT_LEVEL;
		}
	}	

	return true;
}

Float CGlobalWater::GetWaterLevelBasic( const Float X, const Float Y ) const // Vector& forThatWorldPosition ) const
{
	// cheapest version of GetWaterLevelXXX
	// does:
	//			- gives raw 'average' water level at given position
	// does NOT:
	//			- access clip map
	//			- calculates any kind of waves
	//			- calculates shore proximity or whatever
	//
	// basically it return default height, including local shape(s) at given position

	PC_SCOPE_PIX(Water_GetLevelBasic);

	// NOTE: We don't use heightmap in this method

	return GetReferenceWaterLevel( X, Y );
}

Float CGlobalWater::GetWaterLevelApproximate( const Float X, const Float Y, Float* heightDepth ) const
{
	// cheaper version of GetWaterLevelAccurate
	// does NOT:
	//			- access clip map
	//			- calculates smaller waves (currently, less than 0.25f meter)
	//			- calculates shore proximity waves damping
	//
	// this one still access the lake shapes!

	PC_SCOPE_PIX(Water_GetLevelApproximate);

	if ( !m_waterHeightmap->IsInit() )
	{
		return WATER_DEFAULT_NON_INIT_LEVEL;
	}

	const Float scaleLarge = 1.0f / CGlobalWaterUpdateParams::DefaultUVScale.X;
	const Float amplitudeLarge = CGlobalWaterUpdateParams::DefaultAmplitudeScale.X;
	
	Float heightLarge = amplitudeLarge * m_waterHeightmap->GetHeight( (X) * scaleLarge, (Y) * scaleLarge );

	return heightLarge + GetReferenceWaterLevel( X, Y );	
}

Float CGlobalWater::GetWaterLevelAccurate( const Float X, const Float Y, Float* heightDepth ) const
{
	// most expensive version of GetWaterLevel
	// basically - don't use it, 
	// there are better functions for you - see above	

	PC_SCOPE(Water_GetLevelAccurate)
	
	/*
	if( m_debugFrame % 1000 == 0 )
	{
		m_debugAccess++;
	}	
	*/	

	if ( !m_waterHeightmap->IsInit() )
	{
		return WATER_DEFAULT_NON_INIT_LEVEL;
	}

	Float hmapHeight = WATER_DEFAULT_LEVEL;
	
	if( m_clipMap )
	{
		m_clipMap->GetHeightForWorldPosition( Vector( X, Y, 0.0f ), hmapHeight );
	}

	if( heightDepth )
	{
		*heightDepth = hmapHeight;
	}

	const Float scaleLarge  = 1.0f / CGlobalWaterUpdateParams::DefaultUVScale.X;
	const Float scaleMedium = 1.0f / CGlobalWaterUpdateParams::DefaultUVScale.Y;

	const Float amplitudeLarge  = CGlobalWaterUpdateParams::DefaultAmplitudeScale.X;
	const Float amplitudeMedium = CGlobalWaterUpdateParams::DefaultAmplitudeScale.Y;

	Float heightLarge  = amplitudeLarge  * m_waterHeightmap->GetHeight( X * scaleLarge, Y * scaleLarge );
	Float heightMedium = amplitudeMedium * m_waterHeightmap->GetHeight( X * scaleMedium, Y * scaleMedium );
	
	Float referenceWaterLevel = GetReferenceWaterLevel( X, Y );		
	Vector shoreProximity = GetWaterShoreProximityDampValue( hmapHeight, referenceWaterLevel );

	return shoreProximity.X*heightLarge + shoreProximity.Y*heightMedium + referenceWaterLevel;
}

Float CGlobalWater::GetWaterLevelReference()
{
	// as you can see its cheapest 
	// possible version of GetWaterLevel

	PC_SCOPE(Water_GetLevelReference)
	return !m_waterHeightmap->IsInit() ? WATER_DEFAULT_NON_INIT_LEVEL : WATER_DEFAULT_LEVEL;
}

Float CGlobalWater::GetReferenceWaterLevel( const Float X, const Float Y ) const
{
	// ARE YOU SURE THAT WE ARE INITIALIZED?

	// Check local shapes - aka "lake offsets"	
	if ( m_localShapesParams && m_localShapesParams->m_matrices && m_localShapesParams->m_shapesBuffer && m_localShapesParams->m_numShapes )
	{
		for( Uint32 i=0; i<m_localShapesParams->m_numShapes; i++ )
		{
			const Float* mat = &m_localShapesParams->m_matrices[i*4];
			const Float* buf = &m_localShapesParams->m_shapesBuffer[WATER_LOCAL_RESOLUTION*WATER_LOCAL_RESOLUTION*i];

			Float u = (mat[2] != 0.0f) ? (X - mat[0])/mat[2] : 0.0f;
			Float v = (mat[3] != 0.0f) ? (Y - mat[1])/mat[3] : 0.0f;

			Int32 ui = Int32( u*Float(WATER_LOCAL_RESOLUTION) );
			Int32 vi = Int32( v*Float(WATER_LOCAL_RESOLUTION) );

			ui = Clamp( ui, 0, WATER_LOCAL_RESOLUTION-1 );
			vi = Clamp( vi, 0, WATER_LOCAL_RESOLUTION-1 );

			Float pix = buf[ (vi*WATER_LOCAL_RESOLUTION)+ui ];

			// no lakes stacking currently
			if( Abs(pix) > 0.001f ) 
			{
				return pix;
			}
		}
	}

	return WATER_DEFAULT_LEVEL;
}
