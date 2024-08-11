/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderElementMap.h"
#include "renderOcclusion.h"
#include "renderScene.h"

#include "../engine/umbraScene.h"
#include "../engine/bitmapTexture.h"

const	MemSize	CRenderOcclusionData::s_extraOcclusionQueryAdditionalMemorySize = 3 * 1024 * 1024; // 3 MB
Uint8*			CRenderOcclusionData::s_extraOcclusionQueryAdditionalMemory		= nullptr;

Bool CRenderOcclusionData::InitExtraMemory()
{
	RED_ASSERT( !s_extraOcclusionQueryAdditionalMemory, TXT( "RenderOcclusion ExtraMemory already allocated! Debug this!!" ) );
	if ( !s_extraOcclusionQueryAdditionalMemory )
	{
		s_extraOcclusionQueryAdditionalMemory = reinterpret_cast< Uint8* >( RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_UmbraQueryAdditionalMemory, s_extraOcclusionQueryAdditionalMemorySize ) );
	}
	return s_extraOcclusionQueryAdditionalMemory != nullptr;
}

Bool CRenderOcclusionData::ShutdownExtraMemory()
{
	RED_FATAL_ASSERT( s_extraOcclusionQueryAdditionalMemory, "RenderOcclusion ExtraMemory was not allocated! Debug this!!" );
	RED_MEMORY_FREE( MemoryPool_Default, MC_UmbraQueryAdditionalMemory, s_extraOcclusionQueryAdditionalMemory );
	s_extraOcclusionQueryAdditionalMemory = nullptr;
	return true;
}

#ifdef USE_UMBRA
ExtraOcclusionQueryResults::ExtraOcclusionQueryResults( TVisibleChunksIndices&& visibleObjects, UmbraOcclusionBuffer* occlusionBuffer )
#else
ExtraOcclusionQueryResults::ExtraOcclusionQueryResults( TVisibleChunksIndices&& visibleObjects )
#endif // USE_UMBRA
	: m_visibleObjectsIndices( Move( visibleObjects ) )
#ifdef USE_UMBRA
	, m_occlusionBuffer( occlusionBuffer )
#endif // USE_UMBRA
{
}

ExtraOcclusionQueryResults::~ExtraOcclusionQueryResults()
{
#ifdef USE_UMBRA
	SAFE_RELEASE( m_occlusionBuffer );
#endif // USE_UMBRA
}

#ifdef USE_UMBRA

#ifndef RED_FINAL_BUILD
const Char* GetUmbraErrorMessage( Umbra::Query::ErrorCode code )
{
	switch ( code )
	{
	case Umbra::Query::ERROR_GENERIC_ERROR:			return TXT("GENERIC_ERROR");
	case Umbra::Query::ERROR_OUT_OF_MEMORY:			return TXT("OUT_OF_MEMORY");
	case Umbra::Query::ERROR_INVALID_ARGUMENT:		return TXT("INVALID_ARGUMENT");
	case Umbra::Query::ERROR_SLOTDATA_UNAVAILABLE:	return TXT("SLOTDATA_UNAVAILABLE");
	case Umbra::Query::ERROR_OUTSIDE_SCENE:			return TXT("OUTSIDE_SCENE");
	case Umbra::Query::ERROR_NO_TOME:				return TXT("NO_TOME");
	case Umbra::Query::ERROR_UNSUPPORTED_OPERATION: return TXT("UNSUPPORTED_OPERATION");
	case Umbra::Query::ERROR_NO_PATH:				return TXT("NO_PATH");
	default:										return TXT("<<Unknown Error>>");
	}
}
#endif //RED_FINAL_BUILD

class UmbraRenderer : public Umbra::DebugRenderer, public IRenderObject
{
private:
	static const Uint32	DefaultStatisticsY = 150;
	static const Bool overlay = false;
	static const Bool alwaysVisible = true;

	CRenderFrame*		m_frame;
	Uint32				m_statisticsX;
	Uint32				m_statisticsY;

public:
	UmbraRenderer() 
		: m_frame( nullptr )
	{}

	RED_INLINE void SetFrame( CRenderFrame* frame )
	{ 
		m_frame = frame;
		m_statisticsX = frame->GetFrameOverlayInfo().m_width - 300;
		m_statisticsY = DefaultStatisticsY;
	}

	virtual void addLine( const Umbra::Vector3 &start, const Umbra::Vector3 &end, const Umbra::Vector4 &color )
	{
		Vector3 s( start.v[0], start.v[1], start.v[2] );
		Vector3 e( end.v[0], end.v[1], end.v[2] );
		Color c( (Uint8)(color.v[0] * 255), (Uint8)(color.v[1] * 255), (Uint8)(color.v[2] * 255), (Uint8)(color.v[3] * 255) );
		Float lineWidth = 0.05f;
		m_frame->AddDebugLine( s, e, c, overlay, alwaysVisible );
		//m_frame->AddDebugFatLine( s, e, c, lineWidth, overlay, alwaysVisible );
	}

	virtual void addPoint( const Umbra::Vector3 &pt, const Umbra::Vector4 &color )
	{
		Vector3 p( pt.v[0], pt.v[1], pt.v[2] );
		Color c( (Uint8)(color.v[0] * 255), (Uint8)(color.v[1] * 255), (Uint8)(color.v[2] * 255), (Uint8)(color.v[3] * 255) );
		m_frame->AddDebugSphere( p, 0.2f, Matrix::IDENTITY, c, overlay, alwaysVisible );
	}

	virtual void addAABB( const Umbra::Vector3 &mn, const Umbra::Vector3 &mx, const Umbra::Vector4 &color )
	{
		Color c( (Uint8)(color.v[0] * 255), (Uint8)(color.v[1] * 255), (Uint8)(color.v[2] * 255), (Uint8)(color.v[3] * 255) );
		Vector vMin( mn.v[0], mn.v[1], mn.v[2] );
		Vector vMax( mx.v[0], mx.v[1], mx.v[2] );
		Box b( vMin, vMax );
		m_frame->AddDebugBox( b, Matrix::IDENTITY, c, overlay, alwaysVisible );
	}

	virtual void addQuad( const Umbra::Vector3 &x0y0, const Umbra::Vector3 &x0y1, const Umbra::Vector3 &x1y1, const Umbra::Vector3 &x1y0, const Umbra::Vector4 &color )
	{
		Color c( (Uint8)(color.v[0] * 255), (Uint8)(color.v[1] * 255), (Uint8)(color.v[2] * 255), (Uint8)(color.v[3] * 255) );

		Vector v00( x0y0.v[0], x0y0.v[1], x0y0.v[2] );
		Vector v01( x0y1.v[0], x0y1.v[1], x0y1.v[2] );
		Vector v11( x1y1.v[0], x1y1.v[1], x1y1.v[2] );
		Vector v10( x1y0.v[0], x1y0.v[1], x1y0.v[2] );

		TDynArray< DebugVertex > vertices;
		vertices.PushBack( DebugVertex( v00, c.ToUint32() ) );
		vertices.PushBack( DebugVertex( v01, c.ToUint32() ) );
		vertices.PushBack( DebugVertex( v10, c.ToUint32() ) );
		vertices.PushBack( DebugVertex( v11, c.ToUint32() ) );

		// create 4 triangles, both sides
		TDynArray< Uint16 > indices;
		indices.PushBack(1);
		indices.PushBack(2);
		indices.PushBack(0);

		indices.PushBack(1);
		indices.PushBack(3);
		indices.PushBack(2);

		indices.PushBack(1);
		indices.PushBack(0);
		indices.PushBack(2);

		indices.PushBack(1);
		indices.PushBack(2);
		indices.PushBack(3);

		m_frame->AddDebugTriangles( Matrix::IDENTITY, vertices.TypedData(), vertices.Size(), indices.TypedData(), indices.Size(), c, overlay, alwaysVisible );
	}

	virtual void addStat( const char *stat, int val )
	{
		if ( m_statisticsY == DefaultStatisticsY )
		{
			m_frame->AddDebugScreenText( m_statisticsX, m_statisticsY, TXT( "Umbra Statistics ('Key' = Value)" ) );
			m_statisticsY += 15;
		}
		m_frame->AddDebugScreenFormatedText( m_statisticsX, m_statisticsY, TXT( "'%ls' = %d" ), ANSI_TO_UNICODE( stat ), val );
		m_statisticsY += 15;
	}
};

CRenderOcclusionData::CRenderOcclusionData()
	: m_tomeCollection( nullptr )
	, m_maxObjectsCount( 0 )
	, m_occlusionBuffer( nullptr )
	, m_queryThreshold( -1.0f )
#ifndef RED_FINAL_BUILD
	, m_debugRenderer( nullptr )
	, m_queryTime( 0.0 )
#endif
	, m_lastQuerySucceeded( false )
	, m_isCutscene( false )
{
	m_dataDistanceThresholdSquared = CUmbraScene::DEFAULT_UMBRA_DISTANCE_MULTIPLIER * 256.0f; /* default umbra tile size: 256.0f */
	m_dataDistanceThresholdSquared *= m_dataDistanceThresholdSquared;
}

CRenderOcclusionData::CRenderOcclusionData( const CUmbraScene* scene )
	: m_tomeCollection( nullptr )
	, m_maxObjectsCount( 0 )
	, m_queryThreshold( -1.0f )
	, m_lastQuerySucceeded( false )
	, m_isCutscene( false )
{
	CUmbraTomeCollection* tomeCollectionWrapper = scene->GetTomeCollectionWrapper();
	RED_FATAL_ASSERT( tomeCollectionWrapper, "" );
	m_tomeCollection = tomeCollectionWrapper->GetTomeCollection();
	RED_ASSERT( m_tomeCollection );

	m_dataDistanceThresholdSquared = scene->GetDistanceMultiplier() * scene->GetTileSize();
	m_dataDistanceThresholdSquared *= m_dataDistanceThresholdSquared;

	size_t gateSize = m_tomeCollection->getGateStateSize();
	m_gatesStorage = RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_UmbraGates, gateSize );
	m_gates = new Umbra::GateStateVector( m_gatesStorage, gateSize );
	m_cutsceneGatesStorage = RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_UmbraGates, gateSize );
	m_cutsceneGates = new Umbra::GateStateVector( m_cutsceneGatesStorage, gateSize );
	Int32 numberOfGates = m_tomeCollection->getGateCount();
	for ( Int32 gateIndex = 0; gateIndex < numberOfGates; ++gateIndex )
	{
		m_cutsceneGates->setState( gateIndex, true );
	}
	m_maxObjectsCount = m_tomeCollection->getObjectCount();
	m_visibleObjectsIndices.Resize( m_maxObjectsCount );
	m_occlusionBuffer = new UmbraOcclusionBuffer();
	m_visibility = new Umbra::Visibility();
	m_query = new Umbra::QueryExt( m_tomeCollection );

#ifndef RED_FINAL_BUILD
	m_debugRenderer = new UmbraRenderer();
	m_query->setDebugRenderer( m_debugRenderer );
#endif
}

CRenderOcclusionData::~CRenderOcclusionData()
{
	SAFE_RELEASE( m_occlusionBuffer );

	if ( m_gates )
	{
		delete m_gates;
		m_gates = nullptr;
	}
	if ( m_gatesStorage )
	{
		RED_MEMORY_FREE( MemoryPool_Default, MC_UmbraGates, m_gatesStorage );
		m_gatesStorage = nullptr;
	}

	if ( m_cutsceneGates )
	{
		delete m_cutsceneGates;
		m_cutsceneGates = nullptr;
	}
	if ( m_cutsceneGatesStorage )
	{
		RED_MEMORY_FREE( MemoryPool_Default, MC_UmbraGates, m_cutsceneGatesStorage );
		m_cutsceneGatesStorage = nullptr;
	}

	if ( m_visibility )
	{
		delete m_visibility;
		m_visibility = nullptr;
	}

#ifndef RED_FINAL_BUILD
	SAFE_RELEASE( m_debugRenderer );
#endif

	if ( m_query )
	{
		delete m_query;
		m_query = nullptr;
	}
}


ExtraOcclusionQueryResults* CRenderOcclusionData::PerformExtraOcclusionQuery( CRenderSceneEx* scene, CRenderFrame* frame, Bool doOcclusion /*= true*/ ) const
{
	if ( !m_tomeCollection || m_maxObjectsCount <= 0 )
	{
		return nullptr;
	}

	UmbraOcclusionBuffer* occlusionBuffer = new UmbraOcclusionBuffer();
	TVisibleChunksIndices visibleObjectsIndices( m_maxObjectsCount );


	CRenderCamera camera( frame->GetFrameInfo().m_occlusionCamera );
	Float desiredNearPlane = Max<Float>( camera.GetNearPlane(), MIN_OCCLUSION_CAMERA_NEAR_PLANE );
	camera.SetNearPlane( desiredNearPlane );
	camera.CalculateMatrices();
	Umbra::CameraTransform cam = UmbraHelpers::CalculateCamera( camera );

	Uint32 flags = Umbra::Query::QUERYFLAG_IGNORE_CAMERA_POSITION;

	{
		PC_SCOPE_PIX( QueryPortalVisibilityExtra );

		Umbra::IndexList objects( visibleObjectsIndices.TypedData(), m_maxObjectsCount );
		Umbra::Visibility visibility( &objects, occlusionBuffer );
		Umbra::Query::ErrorCode code;
		Umbra::QueryExt query( m_tomeCollection );
		query.setGateStates( m_isCutscene ? m_cutsceneGates : m_gates );

		if ( s_extraOcclusionQueryAdditionalMemory )
		{
			code = query.setWorkMem( s_extraOcclusionQueryAdditionalMemory, s_extraOcclusionQueryAdditionalMemorySize );
			RED_ASSERT( code == Umbra::Query::ERROR_OK );
		}

		if ( doOcclusion )
		{
			code = query.queryPortalVisibility( flags, visibility, cam, 0.0f, m_queryThreshold );
		}
		else
		{
			code = query.queryFrustumVisibility( flags, visibility, cam );
		}

		if ( code != Umbra::Query::ERROR_OK )
		{
#ifndef RED_FINAL_BUILD
			const Char* message = GetUmbraErrorMessage( code );
			WARN_RENDERER( TXT("Extra Umbra query failed: %ls" ), message );
#endif //RED_FINAL_BUILD

			occlusionBuffer->Release();

			return nullptr;
		}

		visibleObjectsIndices.ResizeFast( objects.getSize() );
	}


	ExtraOcclusionQueryResults* results = new ExtraOcclusionQueryResults( Move( visibleObjectsIndices ), occlusionBuffer );

	// results now owns occlusionBuffer, so no need to release here.

	return results;
}

void CRenderOcclusionData::PerformOcclusionQuery( CRenderSceneEx* scene, CRenderFrame* frame )
{
	if ( !m_tomeCollection || m_maxObjectsCount <= 0 )
	{
		m_lastQuerySucceeded = false;
		frame->AddDebugScreenFormatedText( 10, frame->GetFrameOverlayInfo().m_height - 20, TXT("No Umbra data loaded, expect problems with objects visibility") );
		return;
	}

	// this needs to be here to make sure we have the proper size, we are swapping the pointers so the array might not be allocated yet or it can be allocated to something smaller
	m_visibleObjectsIndices.Resize( m_maxObjectsCount );

	CRenderCamera camera( frame->GetFrameInfo().m_occlusionCamera );
	m_cameraFrustum.InitFromCamera( camera.GetWorldToScreen() );
	Float desiredNearPlane = Max<Float>( camera.GetNearPlane(), MIN_OCCLUSION_CAMERA_NEAR_PLANE );
	camera.SetNearPlane( desiredNearPlane );
	camera.CalculateMatrices();
	Umbra::CameraTransform cam = UmbraHelpers::CalculateCamera( camera );

	Uint32 flags = Umbra::Query::QUERYFLAG_IGNORE_CAMERA_POSITION;
#ifndef RED_FINAL_BUILD
	if ( frame->GetFrameInfo().IsShowFlagOn( SHOW_UmbraFrustum ) )			flags |= Umbra::Query::DEBUGFLAG_VIEW_FRUSTUM;
	if ( frame->GetFrameInfo().IsShowFlagOn( SHOW_UmbraObjectBounds ) )		flags |= Umbra::Query::DEBUGFLAG_OBJECT_BOUNDS;
	if ( frame->GetFrameInfo().IsShowFlagOn( SHOW_UmbraPortals ) )			flags |= Umbra::Query::DEBUGFLAG_PORTALS;
	if ( frame->GetFrameInfo().IsShowFlagOn( SHOW_UmbraViewCell ) )			flags |= Umbra::Query::DEBUGFLAG_VIEWCELL;
	if ( frame->GetFrameInfo().IsShowFlagOn( SHOW_UmbraVisibilityLines ) )	flags |= Umbra::Query::DEBUGFLAG_VISIBILITY_LINES;
	if ( frame->GetFrameInfo().IsShowFlagOn( SHOW_UmbraVisibleVolume ) )	flags |= Umbra::Query::DEBUGFLAG_VISIBLE_VOLUME;
	if ( frame->GetFrameInfo().IsShowFlagOn( SHOW_UmbraStatistics ) )		flags |= Umbra::Query::DEBUGFLAG_STATISTICS;
	if ( flags != 0 )
	{
		m_debugRenderer->SetFrame( frame );
	}
#endif //RED_FINAL_BUILD

#ifndef RED_FINAL_BUILD
	CTimeCounter timer;
#endif //RED_FINAL_BUILD

	PC_SCOPE_PIX( QueryPortalVisibility );

	Umbra::IndexList objects( m_visibleObjectsIndices.TypedData(), m_maxObjectsCount );
	m_visibility->setOutputObjects( &objects );
	m_visibility->setOutputBuffer( m_occlusionBuffer );
	Umbra::Query::ErrorCode code;
	m_query->setGateStates( m_isCutscene ? m_cutsceneGates : m_gates );
	if ( scene->GetQueryAdditionalMemory() )
	{
		code = m_query->setWorkMem( scene->GetQueryAdditionalMemory(), scene->GetQueryAdditionalMemorySize() );
		RED_ASSERT( code == Umbra::Query::ERROR_OK );
	}

	code = m_query->queryPortalVisibility( flags, *m_visibility, cam, 0.0f, m_queryThreshold );
	m_lastQuerySucceeded = code == Umbra::Query::ERROR_OK;

	if ( code == Umbra::Query::ERROR_OK )
	{
		scene->GetRenderElementMap()->FeedVisibleObjects( m_visibleObjectsIndices, objects.getSize() );
	}
#ifndef RED_FINAL_BUILD
	else
	{
		String message( TXT( "Umbra query failed (" ) );
		message += GetUmbraErrorMessage( code );
		message += TXT( ")" );
		frame->AddDebugScreenText( 10, frame->GetFrameOverlayInfo().m_height - 20, message.AsChar() );
	}
#endif //RED_FINAL_BUILD

#ifndef RED_FINAL_BUILD
	m_queryTime = timer.GetTimePeriodMS();
#endif //RED_FINAL_BUILD

#ifndef RED_FINAL_BUILD
	if ( frame->GetFrameInfo().IsShowFlagOn( SHOW_UmbraOcclusionBuffer ) )
	{
		Umbra::OcclusionBuffer::BufferDesc desc;
		m_occlusionBuffer->getBufferDesc( desc );

		Uint8* srcData = (Uint8*)RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_UmbraGeneric, desc.stride * desc.height );
		m_occlusionBuffer->getBuffer( srcData );

		CBitmapTexture* texture			= CreateObject< CBitmapTexture >();
		CSourceTexture* sourceTexture	= CreateObject< CSourceTexture >( texture );
		sourceTexture->Init( desc.width, desc.height, TRF_Grayscale );
		RED_ASSERT( srcData );

		// copying buffer "upside down", with CopyBufferPitched, the result texture was flipped horizontally
		const Uint8* srcPtr = ( const Uint8* ) srcData;
		Uint8* destPtr = ( Uint8* ) sourceTexture->GetBufferAccessPointer();
		destPtr += ( desc.height - 1 ) * desc.width;
		for ( Uint32 i = 0; i < (Uint32)desc.height; ++i, srcPtr += desc.width, destPtr -= desc.width )
		{
			Red::System::MemoryCopy( destPtr, srcPtr, desc.width );
		}

		// place preview of occlusion buffer in bottom-left corner of the screen, twice the size of the occlusion buffer
		Uint32 x = 0;
		Uint32 y = frame->GetFrameOverlayInfo().m_height - desc.height * 2;
		Uint32 width = desc.width * 2;
		Uint32 height = desc.height * 2;
		texture->InitFromSourceData( sourceTexture, CNAME( Default ), true );
		frame->AddDebugTexturedRect( x, y, width, height, texture, Color::WHITE );
		RED_MEMORY_FREE( MemoryPool_Default, MC_UmbraGeneric, srcData );
		
		sourceTexture->Discard();
		sourceTexture = nullptr;
		texture->Discard();
		texture = nullptr;	
	}
#endif //RED_FINAL_BUILD
}

void CRenderOcclusionData::SetGateState( TObjectIdType objectId, Bool opened )
{
	Int32 gateIndex = m_tomeCollection->findGateIndex( objectId );
	if ( gateIndex != -1 )
	{
		m_gates->setState( gateIndex, opened );
	}
}

Bool CRenderOcclusionData::PerformLineQuery( const Vector& start, const Vector& end )
{
	if ( !m_query )
	{
		return true;
	}

	Umbra::LineSegmentQuery lineQuery( (Umbra::Vector3&)start.A, (Umbra::Vector3&)end.A );

	m_query->queryLineSegment( &lineQuery, 1 );

	Umbra::LineSegmentQuery::ResultCode result = lineQuery.getResult();
	switch ( result )
	{
	case Umbra::LineSegmentQuery::RESULT_INTERSECTION:
		return false; // not visible
	case Umbra::LineSegmentQuery::RESULT_NO_INTERSECTION:
	case Umbra::LineSegmentQuery::RESULT_OUTSIDE_SCENE:
		return true; // visible
	}
	RED_HALT( "Unknown Umbra::LineQuery result" );
	return false;
}

void CRenderOcclusionData::PerformLineQueries( const Vector& end, CharacterOcclusionInfo* infos, Int32 numberOfEntries ) const
{
	PC_SCOPE_PIX( PerformLineQueries );
	if ( !m_query )
	{
		for ( Int32 i = 0; i < numberOfEntries; ++i )
		{
			infos[ i ].m_positionVisible = true;
		}
		return;
	}
	const Umbra::Vector3& uEnd = (const Umbra::Vector3&)end.A;
	
	TDynArray< Int32 > testedIndices( numberOfEntries );
	TDynArray< Umbra::LineSegmentQuery > queries( numberOfEntries );
	Int32 testedIndicesSize = 0;
	for ( Int32 i = 0; i < numberOfEntries; ++i )
	{
		const Vector& pos = infos[ i ].m_position;
		if ( !m_cameraFrustum.IsPointInside( pos ) )
		{
			infos[ i ].m_positionVisible = false;
			continue;
		}

		testedIndices[ testedIndicesSize ] = i;
		queries[ testedIndicesSize ] = Umbra::LineSegmentQuery( (Umbra::Vector3&)pos.A, uEnd );
		++testedIndicesSize;
	}

	if ( testedIndicesSize == 0 )
	{
		// do not perform line test, everything is outside frustum
		return;
	}
	Umbra::Query::ErrorCode code = m_query->queryLineSegment( queries.TypedData(), testedIndicesSize );
	if ( code == Umbra::Query::ERROR_OK )
	{
		for ( Int32 i = 0; i < testedIndicesSize; ++i )
		{
			const Int32 testedIndex = testedIndices[ i ];
			const Umbra::LineSegmentQuery::ResultCode result = queries[ i ].getResult();
			switch ( result )
			{
			case Umbra::LineSegmentQuery::RESULT_NO_INTERSECTION:	infos[ testedIndex ].m_positionVisible = true;	break;
			case Umbra::LineSegmentQuery::RESULT_INTERSECTION:		infos[ testedIndex ].m_positionVisible = false;	break;
			case Umbra::LineSegmentQuery::RESULT_OUTSIDE_SCENE:		infos[ testedIndex ].m_positionVisible = false;	break;
			}
		}
	}
	else
	{
#ifndef RED_FINAL_BUILD
		RED_LOG_ERROR( UmbraError, TXT( "UmbraLineQueries error: %s" ), GetUmbraErrorMessage( code ) );
#endif // RED_FINAL_BUILD
		for ( Int32 i = 0; i < numberOfEntries; ++i )
		{
			// set all results to false, so they can be tested with regular bounding box test later
			infos[ i ].m_positionVisible = false;
		}
	}
}

IRenderObject* CRenderInterface::UploadOcclusionData( const CUmbraScene* umbraScene )
{
	return new CRenderOcclusionData( umbraScene );
}

#endif // USE_UMBRA
