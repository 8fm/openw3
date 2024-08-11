/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "stripeComponent.h"

#include "../core/mathUtils.h"

#include "../renderer/renderProxyStripe.h"

#include "bitmapTexture.h"
#include "easyMeshBuilder.h"
#include "layer.h"
#include "multiCurve.h"
#include "pathlibAgent.h"
#include "renderCommands.h"
#include "renderFragment.h"
#include "umbraScene.h"
#include "world.h"


#define ONLY_RENDER_PROXY_STRIPE_PROPERTIES

#ifndef NO_EDITOR
Uint32 GStripeComponentEditorFlags = SCEF_NOFLAGS;
#endif

IMPLEMENT_ENGINE_CLASS( SStripeControlPoint );
IMPLEMENT_ENGINE_CLASS( CStripeComponent );

#ifndef NO_EDITOR
RED_DECLARE_NAME( StripeDeleted );
RED_DEFINE_NAME( StripeDeleted );
#endif

RED_DEFINE_STATIC_NAME( density );

//////////////////////////////////////////////////////////////////////////

Bool operator==( const SRenderProxyStripeProperties::Vertex& a, const SRenderProxyStripeProperties::Vertex& b )
{
	return a.color == b.color &&
		a.x == b.x &&
		a.y == b.y &&
		a.z == b.z &&
		a.u == b.u &&
		a.v == b.v &&
		a.bu == b.bu &&
		a.bv == b.bv &&
		a.offset == b.offset &&
		a.tx == b.tx &&
		a.ty == b.ty &&
		a.tz == b.tz;
}

//////////////////////////////////////////////////////////////////////////

CStripeComponent::CStripeComponent()
	: m_width( 4.0f )
	, m_diffuseTexture( nullptr )
	, m_diffuseTexture2( nullptr )
	, m_normalTexture( nullptr )
	, m_normalTexture2( nullptr )
	, m_blendTexture( nullptr )
	, m_depthTexture( nullptr )
	, m_textureLength( 1.0f )
	, m_blendTextureLength( 4.0f )
	, m_alphaScale( 1.0f )
	, m_endpointAlpha( 1.0f )
	, m_stripeColor( Color::WHITE )
	, m_density( 1.0f )
	, m_rotateTexture( false )
	, m_projectToTerrain( false )
	, m_exposedToNavigation( true )
	, m_curve( nullptr )
	, m_autoHideDistance( -1.0f ) // -1.0 means it will take value from Config
	, m_cookedVertices( TDataBufferAllocator< MC_RenderData >::GetInstance() )
	, m_cookedIndices( TDataBufferAllocator< MC_RenderData >::GetInstance() )
{
}

CStripeComponent::~CStripeComponent()
{
	if ( m_curve )
	{
		delete m_curve;
	}
}

static SRenderProxyStripeProperties::Vertex StripeVertex(
	const Vector& pos,
	const Color& rgb,
	Float u, Float v,
	Float bu, Float bv,
	Float ofs,
	const Vector& tang,
	const Vector& cent,
	Float wid )
{
	SRenderProxyStripeProperties::Vertex vtx = {
		pos.X, pos.Y, pos.Z,
		static_cast< Uint32 >( rgb.R | (rgb.G << 8) | (rgb.B << 16) | (rgb.A << 24) ),
		u, v,
		bu, bv,
		ofs,
		tang.X, tang.Y, tang.Z,
		cent.X, cent.Y, cent.Z,
		wid
	};
	return vtx;
}

void CStripeComponent::GenerateStripeGeometry( struct SRenderProxyStripeProperties* properties, Float extraWidth/* =0.0f */ )
{
	// cooked data
	if ( IsCooked() )
	{
		if ( !m_cookedVertices.GetSize() || !m_cookedIndices.GetSize() )
			return;

		const Uint32 vertexCount = m_cookedVertices.GetSize() / sizeof( SRenderProxyStripeProperties::Vertex );
		properties->m_vertices.Resize( vertexCount );
		Red::MemoryCopy( properties->m_vertices.Data(), m_cookedVertices.GetData(), m_cookedVertices.GetSize() );

		const Uint32 indexCount = m_cookedIndices.GetSize() / sizeof( Uint16 );
		properties->m_indices.Resize( indexCount );		
		Red::MemoryCopy( properties->m_indices.Data(), m_cookedIndices.GetData(), m_cookedIndices.GetSize() );

		properties->m_boundingBox = m_cookedBoundingBox;
		return;
	}

	// Clear any previous data
	properties->m_vertices.Clear();
	properties->m_indices.Clear();
	properties->m_boundingBox.Clear();

	// We need at least two points
	if ( m_points.Size() < 2 )
	{
		return;
	}

	// Calculate total distance
	Float totalDistance = 0.0f;
	for ( Uint32 i=0; i < m_points.Size() - 1; ++i )
	{
		totalDistance += m_points[i + 1].m_position.DistanceTo( m_points[i].m_position );
	}

	// Setup the curves
	SMultiCurve curve;						// position curve
	SMultiCurve ccurve;						// color curve (no alpha)
	SMultiCurve pncurve;					// plane normal curve (plane pos from position)
	SSimpleCurve scurve( SCT_Float );		// scale curve
	SSimpleCurve acurve( SCT_Float );		// alpha curve
	SSimpleCurve bcurve( SCT_Float );		// blend offset curve
	curve.SetCurveType( ECurveType_Vector, 0, false );
	curve.SetLooping( false );
	ccurve.SetCurveType( ECurveType_Vector, 0, false );
	ccurve.SetLooping( false );
	pncurve.SetCurveType( ECurveType_Vector, 0, false );
	pncurve.SetLooping( false );
	scurve.SetLoop( false );
	acurve.SetLoop( false );
	bcurve.SetLoop( false );

	// Fill the curve points
	Float distance = 0.0f;
	for ( Uint32 i=0; i < m_points.Size(); ++i )
	{
		const SStripeControlPoint& point = m_points[i];
		Vector colorRGB = point.m_color.ToVector(), up;
		colorRGB.W = 1.0f;
		point.m_rotation.ToAngleVectors( nullptr, nullptr, &up );
		curve.AddControlPoint( distance/totalDistance, point.m_position );
		ccurve.AddControlPoint( distance/totalDistance, colorRGB );
		pncurve.AddControlPoint( distance/totalDistance, up );
		acurve.AddPoint( distance/totalDistance, point.m_color.A );
		scurve.AddPoint( distance/totalDistance, point.m_scale );
		bcurve.AddPoint( distance/totalDistance, point.m_blendOffset );
		distance += i + 1 < m_points.Size() ? m_points[i + 1].m_position.DistanceTo( point.m_position ) : 0.0f;
	}

	// Clear editor center points
#ifndef NO_EDITOR
	m_centerPoints.Clear();
#endif

	// Calculate center point values in the stripe
	TDynArray< Vector, MC_Temporary > centers;
	TDynArray< Plane, MC_Temporary > planes;
	TDynArray< Float, MC_Temporary > scales, alphas, offsets;
	TDynArray< Color, MC_Temporary > colors;
	Float time = 0.0f;
	while ( time < 1.0f )
	{
		Vector startPos, endPos, prevPos, tmp;
		Float slice = 0.0f, startTime = time, endTime = time, startScale;
		Float scaledWidth2;
		curve.GetAbsolutePosition( time, startPos );
		startScale = scurve.GetFloatValue( startTime );
		scaledWidth2 = (m_width/m_density)*startScale*(m_width/m_density)*startScale;
		prevPos = startPos;
		do
		{
			prevPos = endPos;
			if ( time + slice >= 1.0f )
			{
				curve.GetAbsolutePosition( endTime, endPos );
				endTime = time = 1.0f;
				break;
			}
			endTime = time + slice;
			curve.GetAbsolutePosition( endTime, endPos );
			if ( startPos.DistanceSquaredTo( endPos ) > scaledWidth2 )
			{
				time += slice;
				break;
			}
			slice += 0.1f/totalDistance;
		}
		while ( true );
		centers.PushBack( startPos );
#ifndef NO_EDITOR
		m_centerPoints.PushBack( startPos );
#endif
		Vector color;
		ccurve.GetAbsolutePosition( startTime, color );
		colors.PushBack( Color( color ) );
		Vector normal;
		pncurve.GetAbsolutePosition( startTime, normal );
		planes.PushBack( Plane( normal.Normalized3(), 0.0f ) );
		alphas.PushBack( acurve.GetFloatValue( startTime ) );
		scales.PushBack( startScale );
		offsets.PushBack( bcurve.GetFloatValue( startTime ) );
		if ( time >= 1.0f )
		{
			centers.PushBack( endPos );
#ifndef NO_EDITOR
			m_centerPoints.PushBack( endPos );
#endif
			Vector color;
			ccurve.GetAbsolutePosition( 1.0f, color );
			colors.PushBack( Color( color ) );
			Vector normal;
			pncurve.GetAbsolutePosition( 1.0f, normal );
			planes.PushBack( Plane( normal, 0.0f ) );
			alphas.PushBack( acurve.GetFloatValue( 1.0f ) );
			scales.PushBack( scurve.GetFloatValue( 1.0f ) );
			offsets.PushBack( bcurve.GetFloatValue( 1.0f ) );
		}
	}
	curve.RecalculateTimeByDistance();

	// Build mesh
	TEasyMeshBuilder< SRenderProxyStripeProperties::Vertex, Uint16 > builder;
	builder.Begin();

	Float baseU = 0.0f;
	Float baseAlphaU = 0.0f;

	for ( Uint32 i=0; i < centers.Size() - 1; ++i )
	{
		Vector startPos = centers[i];
		Vector endPos = centers[i + 1];
		Vector nextPos = i + 2 < centers.Size() ? centers[i + 2] : ( endPos + ( endPos - startPos ) );
		Vector endDir = ( endPos - startPos ).Normalized3();
		Vector nextDir = ( nextPos - endPos ).Normalized3();
		Vector endRight = planes[i].Project( Vector::Cross( endDir, Vector::EZ, 1.0f ) ).Normalized3();
		Vector nextRight = planes[i + 1].Project( Vector::Cross( nextDir, Vector::EZ, 1.0f ) ).Normalized3();
		Vector startTangent, endTangent;
		Vector a, b, c, d;
		Color startColor( colors[i] );
		Color endColor( colors[i + 1] );
		Float startBlendOffset = offsets[i];
		Float endBlendOffset = offsets[i + 1];

		// Apply alpha scale
		startColor.A = (Uint8)::Clamp( alphas[i]*m_alphaScale, 0.0f, 255.0f );
		endColor.A = (Uint8)::Clamp( alphas[i + 1]*m_alphaScale, 0.0f, 255.0f );

		// Mix with the global stripe color
		startColor = Color( startColor.ToVector()*m_stripeColor.ToVector() );
		endColor = Color( endColor.ToVector()*m_stripeColor.ToVector() );

		// First endpoint, apply endpoint alpha scale
		if ( i == 0 )
		{
			startColor.A = (Uint8)::Clamp( alphas[i]*m_alphaScale*m_endpointAlpha, 0.0f, 255.0f );
		}
		else if ( i == centers.Size() - 2 ) // Second endpoint, apply alpha scale again
		{
			endColor.A = (Uint8)::Clamp( alphas[i + 1]*m_alphaScale*m_endpointAlpha, 0.0f, 255.0f );
			endPos = startPos + endDir * m_width * scales[i + 1];
		}

		// Segment corner points
		a = endPos - nextRight*( extraWidth + m_width )*0.5*scales[i + 1];
		b = endPos + nextRight*( extraWidth + m_width )*0.5*scales[i + 1];
		c = startPos + endRight*( extraWidth + m_width )*0.5*scales[i];
		d = startPos - endRight*( extraWidth + m_width )*0.5*scales[i];

		// Tangents
		startTangent = endDir;
		endTangent = nextDir;

#define OFFSET 0.0f

		// Add segment quad
		if ( m_rotateTexture )
		{
			builder.Quad( StripeVertex( a + Vector( 0, 0, OFFSET ), endColor, 0.0f, 1.0f, 1.0f, baseAlphaU + (m_blendTextureLength*m_density), endBlendOffset, endTangent, endPos, m_width*0.5f*scales[i + 1] ),
						  StripeVertex( b + Vector( 0, 0, OFFSET ), endColor, 1.0f, 1.0f, 1.0f, baseAlphaU + (m_blendTextureLength*m_density), endBlendOffset, endTangent, endPos, m_width*0.5f*scales[i + 1] ),
						  StripeVertex( c + Vector( 0, 0, OFFSET ), startColor, 1.0f, 0.0f, 0.0f, baseAlphaU, startBlendOffset, startTangent, startPos, m_width*0.5f*scales[i] ),
						  StripeVertex( d + Vector( 0, 0, OFFSET ), startColor, 0.0f, 0.0f, 0.0f, baseAlphaU, startBlendOffset, startTangent, startPos, m_width*0.5f*scales[i] ) );
		}
		else
		{
			builder.Quad( StripeVertex( a + Vector( 0, 0, OFFSET ), endColor, baseU + 1.0f/(m_textureLength*m_density), 0.0f, 0.0f, baseAlphaU + 1.0f/m_blendTextureLength, endBlendOffset, endTangent, endPos, m_width*0.5f*scales[i + 1] ),
						  StripeVertex( b + Vector( 0, 0, OFFSET ), endColor, baseU + 1.0f/(m_textureLength*m_density), 1.0f, 1.0f, baseAlphaU + 1.0f/m_blendTextureLength, endBlendOffset, endTangent, endPos, m_width*0.5f*scales[i + 1] ),
						  StripeVertex( c + Vector( 0, 0, OFFSET ), startColor, baseU, 1.0f, 1.0f, baseAlphaU, startBlendOffset, startTangent, startPos, m_width*0.5f*scales[i] ),
						  StripeVertex( d + Vector( 0, 0, OFFSET ), startColor, baseU, 0.0f, 0.0f, baseAlphaU, startBlendOffset, startTangent, startPos, m_width*0.5f*scales[i] ) );
		}

		baseU += 1.0f/(m_textureLength*m_density);
		baseAlphaU += 1.0f/m_blendTextureLength;
	}
	builder.End();

	// Calculate bounding box
	properties->m_boundingBox.Clear();
	for ( Uint32 i=0; i < builder.GetVertexCount(); ++i )
	{
		properties->m_boundingBox.AddPoint(
			m_localToWorld.TransformPoint( Vector(
				builder.GetVertexArray()[i].x,
				builder.GetVertexArray()[i].y,
				builder.GetVertexArray()[i].z
			) )
		);
	}

	// Copy the vertex data from the builder to the stripe properties
	properties->m_indices.Grow( builder.GetIndexCount() );
	properties->m_vertices.Grow( builder.GetVertexCount() );
	Red::System::MemoryCopy( properties->m_indices.TypedData(), builder.GetIndexArray(), properties->m_indices.DataSize() );
	Red::System::MemoryCopy( properties->m_vertices.TypedData(), builder.GetVertexArray(), properties->m_vertices.DataSize() );
}

#ifndef NO_EDITOR
void CStripeComponent::Highlight( CRenderFrame* frame, Color stripeColor, Color frameColor ) const
{
	if ( frameColor.A > 0 )
	{
		frame->AddDebugBox( m_boundingBox, Matrix::IDENTITY, frameColor );
	}
	
	if ( stripeColor.A > 0 )
	{
		TDynArray< DebugVertex > vertices;
		vertices.Grow( m_centerPoints.Size() * 2 );
		for ( Uint32 i = 1; i < m_centerPoints.Size(); ++i )
		{
			vertices[(i - 1) * 2] = DebugVertex( m_localToWorld.TransformPoint( m_centerPoints[i - 1] ), stripeColor );
			vertices[(i - 1) * 2 + 1] = DebugVertex( m_localToWorld.TransformPoint( m_centerPoints[i] ), stripeColor );
		}
		frame->AddDebugLines( vertices.TypedData(), vertices.Size(), true );
	}
}
#endif

void CStripeComponent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag )
{
	TBaseClass::OnGenerateEditorFragments( frame, flag );

#ifndef NO_COMPONENT_GRAPH
	if ( frame->GetFrameInfo().m_renderingMode == RM_HitProxies )
	{
		// Generate geometry
		SRenderProxyStripeProperties fakeProperties;
		GenerateStripeGeometry( &fakeProperties );
		
		// Convert stripe vertices to debug vertices
		TDynArray< DebugVertex > vertices;
		vertices.Grow( fakeProperties.m_vertices.Size() );
		for ( Uint32 i=0; i < fakeProperties.m_vertices.Size(); ++i )
		{
			Vector worldPosition = m_localToWorld.TransformPoint( Vector( fakeProperties.m_vertices[i].x, fakeProperties.m_vertices[i].y, fakeProperties.m_vertices[i].z + 0.07f ) );
			vertices[i] = DebugVertex( worldPosition, GetHitProxyID().GetColor() );
		}

		// Render vertices
		new ( frame ) CRenderFragmentDebugPolyList( frame, Matrix::IDENTITY, vertices.TypedData(), vertices.Size(), fakeProperties.m_indices.TypedData(), fakeProperties.m_indices.Size(),  RSG_DebugUnlit );
		return;
	}
#endif

#ifndef NO_EDITOR
	// Render bounding box
	if ( flag == SHOW_NavRoads && IsVisible() && IsExposedToAI() )
	{
		Highlight( frame, Color( 48, 48, 48 ), Color::CLEAR );
	}

	// Do not render anything special if the hide flag is set
	if ( GStripeComponentEditorFlags & SCEF_HIDE_CONTROLS )
	{
		return;
	}

	// Draw overlay geometry if the stripe is selected and the stripe tool isn't active
	if ( IsSelected() && !( GStripeComponentEditorFlags & SCEF_STRIPE_TOOL_ACTIVE ) )
	{
		// Generate geometry
		SRenderProxyStripeProperties fakeProperties;
		GenerateStripeGeometry( &fakeProperties );

		// Convert stripe vertices to debug vertices
		TDynArray< DebugVertex > vertices;
		vertices.Grow( fakeProperties.m_vertices.Size() );
		for ( Uint32 i=0; i < fakeProperties.m_vertices.Size(); ++i )
		{
			Vector worldPosition = m_localToWorld.TransformPoint( Vector( fakeProperties.m_vertices[i].x, fakeProperties.m_vertices[i].y, fakeProperties.m_vertices[i].z + 0.07f ) );
			vertices[i] = DebugVertex( worldPosition, Color( 0, 200, 0, 64 ) );
		}

		// Render vertices
		frame->AddDebugTriangles( Matrix::IDENTITY, vertices.TypedData(), vertices.Size(), fakeProperties.m_indices.TypedData(), fakeProperties.m_indices.Size(), Color::BLACK );
	}

	// Draw AI Road line if the flag is set
	if ( IsExposedToAI() && ( GStripeComponentEditorFlags & SCEF_SHOW_AIROAD_LINES ) == SCEF_SHOW_AIROAD_LINES && !m_centerPoints.Empty() )
	{
		// Convert stripe centers to debug points
		TDynArray< DebugVertex > vertices;
		vertices.Grow( m_centerPoints.Size() );
		for ( Uint32 i=0; i < m_centerPoints.Size(); ++i )
		{
			Vector worldPosition = m_localToWorld.TransformPoint( m_centerPoints[i] );
			vertices[i] = DebugVertex( worldPosition, Color( 63, 72, 204, 220 ) );
		}
		frame->AddDebugFatLines( vertices.TypedData(), vertices.Size(), 0.1f, true );
	}
	// Draw regular stripe line if the flag is set, the stripe is not selected and no AI road flag is set 
	else if ( !IsSelected() && ( GStripeComponentEditorFlags & SCEF_SHOW_CENTER_LINES ) == SCEF_SHOW_CENTER_LINES && !m_centerPoints.Empty() )
	{
		// Convert stripe centers to debug points
		TDynArray< DebugVertex > vertices;
		vertices.Grow( m_centerPoints.Size()*2 );
		for ( Uint32 i=1; i < m_centerPoints.Size(); ++i )
		{
			vertices[(i - 1) * 2] = DebugVertex( m_localToWorld.TransformPoint( m_centerPoints[i - 1] ), Color::WHITE );
			vertices[(i - 1) * 2 + 1] = DebugVertex( m_localToWorld.TransformPoint( m_centerPoints[i] ), Color::WHITE );
		}
		frame->AddDebugLines( vertices.TypedData(), vertices.Size(), true );
	}
#endif
}

void CStripeComponent::OnUpdateBounds()
{
	// Do nothing, bounds are updated from geometry generation
}

void CStripeComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	PC_SCOPE_PIX( CStripeComponent_OnAttached );

	// Add stripe to scene
	if ( m_renderProxy )
	{
		( new CRenderCommand_AddStripeToScene( world->GetRenderSceneEx(), m_renderProxy ) )->Commit();

		// Update the stripe data
		UpdateStripeProxy();
	}

	// Register editor fragments
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_Stripes );

	if ( IsExposedToAI() )
	{
		world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_NavRoads );
		IMetalinkComponent::Attach( world );
		GGame->RegisterStripeComponent( this );
	}
}

void CStripeComponent::OnDetached( CWorld* world )
{
	if ( IsExposedToAI() )
	{
		IMetalinkComponent::Detach( world );
		GGame->UnregisterStripeComponent( this );
	}

	// Remove stripe from scene
	if ( m_renderProxy )
	{
		( new CRenderCommand_RemoveStripeFromScene( world->GetRenderSceneEx(), m_renderProxy ) )->Commit();
	}

	// Unregister editor fragments
	if ( IsExposedToAI() )
	{
		world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_NavRoads );
	}
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_Stripes );

	TBaseClass::OnDetached( world );
}

#ifdef USE_UMBRA
Bool CStripeComponent::OnAttachToUmbraScene( CUmbraScene* umbraScene, const VectorI& bounds )
{
#if !defined(NO_UMBRA_DATA_GENERATION) && defined(USE_UMBRA_COOKING)
	if ( umbraScene && umbraScene->IsDuringSyncRecreation() && CUmbraScene::ShouldAddComponent( this ) )
	{
		const Float VERTEX_OFFSET = 0.3f;

		// Generate geometry
		SRenderProxyStripeProperties fakeProperties;
		GenerateStripeGeometry( &fakeProperties );
		TDynArray< Vector > vertices;
		vertices.Reserve( fakeProperties.m_vertices.Size() );
		for ( auto& pos : fakeProperties.m_vertices )
		{
			Vector vertexPosition( pos.x, pos.y, pos.z );
			vertexPosition.X += VERTEX_OFFSET;
			vertices.PushBack( vertexPosition );
		}

		return umbraScene->AddStripe( this, bounds, vertices, fakeProperties.m_indices, fakeProperties.m_boundingBox );
	}
#endif // NO_UMBRA_DATA_GENERATION

	return false;
}
#endif // USE_UMBRA

void CStripeComponent::OnPropertyPostChange( IProperty* property )
{
	if ( property->GetName() == CNAME( density ) )
	{
		m_density = Clamp( m_density, 0.5f, 32.0f );
	}
	UpdateStripeProxy();
}

void CStripeComponent::OnVisibilityForced()
{
	if( !IsAttached() ) return;
	if( !GetLayer()->IsAttached() ) return;

	IRenderScene* renderSceneEx = GetLayer()->GetWorld()->GetRenderSceneEx();

	const Bool shouldAdd = CanAttachToRenderScene();

	if ( shouldAdd == false && m_renderProxy != nullptr )
	{
		if ( renderSceneEx )
		{
			( new CRenderCommand_RemoveStripeFromScene( renderSceneEx, m_renderProxy ) )->Commit();
		}
		TBaseClass::OnVisibilityForced();
	}
	else if ( shouldAdd == true && m_renderProxy == nullptr )
	{
		TBaseClass::OnVisibilityForced();
		if ( m_renderProxy )
		{
			( new CRenderCommand_AddStripeToScene( renderSceneEx, m_renderProxy ) )->Commit();
		}
	}
}

void CStripeComponent::RefreshRenderProxies()
{
	CLayer* layer = GetLayer();

	// Remove proxy from scene
	if ( m_renderProxy && IsAttached() && layer->IsAttached() )
	{
		IRenderScene* renderSceneEx = layer->GetWorld()->GetRenderSceneEx();
		( new CRenderCommand_RemoveStripeFromScene( renderSceneEx, m_renderProxy ) )->Commit();
	}

	// Call base - this might recreate the proxy
	TBaseClass::RefreshRenderProxies();

	// Put the proxy back in scene
	if ( m_renderProxy && IsAttached() && layer->IsAttached() )
	{
		IRenderScene* renderSceneEx = layer->GetWorld()->GetRenderSceneEx();
		( new CRenderCommand_AddStripeToScene( renderSceneEx, m_renderProxy ) )->Commit();

		// Update it in the case we have a new proxy
		UpdateStripeProxy();
	}
}
void CStripeComponent::ComputeCurve( SMultiCurve& curve, Float *outTotalDistance )
{
	curve.SetCurveType( ECurveType_Vector, 0, false );
	curve.SetLooping( false );

	Float totalDistance = 0.0f;
	for ( Uint32 i=0; i < m_points.Size() - 1; ++i )
	{
		totalDistance += m_points[i + 1].m_position.DistanceTo( m_points[i].m_position );
	}

	Float distance = 0.0f;
	for ( Uint32 i=0; i < m_points.Size(); ++i )
	{
		const SStripeControlPoint& point = m_points[i];
		curve.AddControlPoint( distance/totalDistance, point.m_position );
		distance +=
			i + 1 < m_points.Size()
			? m_points[i + 1].m_position.DistanceTo( point.m_position )
			: 0.0f;
	}
	curve.RecalculateTimeByDistance();
	if ( outTotalDistance )
	{
		*outTotalDistance = totalDistance;
	}
	curve.SetParent( this );
}

const SMultiCurve& CStripeComponent::RequestCurve()
{
	// lazy initialization
	if ( !m_curve )
	{
		m_curve = new SMultiCurve();

		ComputeCurve( *m_curve );
	}
	return *m_curve;
}


CComponent* CStripeComponent::AsEngineComponent()
{
	return this;
}
PathLib::IComponent* CStripeComponent::AsPathLibComponent()
{
	return this;
}
Bool CStripeComponent::IsNoticableInGame( PathLib::CProcessingEvent::EType eventType ) const
{
	if ( !IsExposedToAI() )
	{
		return false;
	}
	return PathLib::IMetalinkComponent::IsNoticableInGame( eventType );
}
Bool CStripeComponent::IsNoticableInEditor( PathLib::CProcessingEvent::EType eventType ) const
{
	if ( !IsExposedToAI() )
	{
		return false;
	}
	return PathLib::IMetalinkComponent::IsNoticableInEditor( eventType );
}

Bool CStripeComponent::ConfigureGraph( GraphConfiguration& configuration, CPathLibWorld& pathlib )
{
	auto& nodesList = configuration.m_nodes;

	//TDynArray< Node >			m_nodes;
	//TDynArray< Connection >		m_connections;
	//Box							m_bbox;

	SMultiCurve curve;
	Float totalCurveDist;
	ComputeCurve( curve, &totalCurveDist );

	const Matrix& localToWorld = GetLocalToWorld();

	// create starting point
	{
		GraphConfiguration::Node node;
		node.m_nodeFlags = PathLib::NF_DEFAULT;
		node.m_pos = localToWorld.TransformPoint( m_points[ 0 ].m_position );

		nodesList.PushBack( node );
	}

	// create intermediate points
	Int32 intermediatePoints = Int32(totalCurveDist / 20.f);
	if ( intermediatePoints > 0 )
	{
		Float pointsDist = totalCurveDist / Float(intermediatePoints+1);

		Int32 pointsCreated = 0;
		Float distToNextPoint = pointsDist;
		Vector lastVertex;
		SMultiCurvePosition curvePos;
		curvePos.m_edgeAlpha = 0.f;
		curvePos.m_edgeIdx = 0;
		Uint32 edgeCount = curve.Size();
		do
		{
			Bool isEndOfPath;
			Vector intermediatePos;
			curve.GetPointOnCurveInDistance( curvePos, pointsDist, intermediatePos, isEndOfPath );
			ASSERT( !isEndOfPath );

			GraphConfiguration::Node node;
			node.m_nodeFlags = PathLib::NF_DEFAULT;
			node.m_pos = intermediatePos;
			nodesList.PushBack( node );
		}
		while ( ++pointsCreated < intermediatePoints );
	}

	// create destination point
	{
		GraphConfiguration::Node node;
		node.m_nodeFlags = PathLib::NF_DEFAULT;
		node.m_pos = localToWorld.TransformPoint( m_points.Back().m_position );
		nodesList.PushBack( node );
	}

	// compute bbox
	{
		Box& bbox = configuration.m_bbox;
		bbox.Clear();
		for ( Uint32 i = 0, n = nodesList.Size(); i != n; ++i )
		{
			bbox.AddPoint( nodesList[ i ].m_pos );
		}
	}
	
	// compute connections
	{
		const Float ROAD_NAVCOST_MULTIPLIER = 0.25f;

		auto& connections = configuration.m_connections;
		connections.Resize( nodesList.Size()-1 );
		for ( Uint16 i = 1, n = Uint16(nodesList.Size()); i != n; ++i )
		{
			Float dist = (nodesList[ i-1 ].m_pos - nodesList[ i ].m_pos).Mag();

			GraphConfiguration::Connection connection;
			connection.m_ind[ 0 ] = i-1;
			connection.m_ind[ 1 ] = i;
			connection.m_linkCost = PathLib::CalculateLinkCost( dist * ROAD_NAVCOST_MULTIPLIER );
			connection.m_linkFlags = PathLib::NF_IS_CUSTOM_LINK;

			connections[ i-1 ] = connection;
		}
	}

	return true;
}

PathLib::IMetalinkSetup::Ptr CStripeComponent::CreateMetalinkSetup() const
{
	// return global const metalink instance
	return PathLib::CMetalinkSetupFactory::GetInstance().GetClassFactory( PathLib::MetalinkClassId( EEngineMetalinkType::T_STRIPE ) )->Request();
}


void CStripeComponent::GetRenderProxyStripeProperties( struct SRenderProxyStripeProperties* properties )
{
	// Make sure we have properties
	ASSERT( properties, TXT("No stripe properties given!") );
	if ( !properties )
	{
		return;
	}

	// Copy stripe attributes
	properties->m_diffuseTexture = m_diffuseTexture ? m_diffuseTexture->GetRenderResource() : nullptr;
	properties->m_diffuseTexture2 = m_diffuseTexture2 ? m_diffuseTexture2->GetRenderResource() : nullptr;
	properties->m_normalTexture = m_normalTexture ? m_normalTexture->GetRenderResource() : nullptr;
	properties->m_normalTexture2 = m_normalTexture2 ? m_normalTexture2->GetRenderResource() : nullptr;
	properties->m_blendTexture = m_blendTexture ? m_blendTexture->GetRenderResource() : nullptr;
	properties->m_depthTexture = m_depthTexture ? m_depthTexture->GetRenderResource() : nullptr;
	properties->m_projectToTerrain = m_projectToTerrain;

	// Generate geometry
	GenerateStripeGeometry( properties );

	// Update bounds
	m_boundingBox = properties->m_boundingBox;
}

void CStripeComponent::UpdateStripeProxy()
{
	SRenderProxyStripeProperties* properties = new SRenderProxyStripeProperties();
	GetRenderProxyStripeProperties( properties );
	( new CRenderCommand_UpdateStripeProperties( m_renderProxy, properties ) )->Commit();
}

#ifndef NO_EDITOR
void CStripeComponent::SetProjectToTerrain( bool projectToTerrain )
{
	if ( projectToTerrain != m_projectToTerrain )
	{
		m_projectToTerrain = projectToTerrain;
		EDITOR_DISPATCH_EVENT( CNAME( RefreshPropertiesPage ), NULL );
		UpdateStripeProxy();
	}
}

void CStripeComponent::SetExposeToAI( bool exposeToAI )
{
	if ( exposeToAI != m_exposedToNavigation )
	{
		m_exposedToNavigation = exposeToAI;
		EDITOR_DISPATCH_EVENT( CNAME( RefreshPropertiesPage ), NULL );
	}
}

void CStripeComponent::EditorPreDeletion()
{
	EDITOR_DISPATCH_EVENT( CNAME( StripeDeleted ), CreateEventData( this ) );
}

#endif

#ifdef USE_UMBRA
Uint32 CStripeComponent::GetOcclusionId() const
{
	return UmbraHelpers::CalculateBoundingBoxHash( GetBoundingBox() );
}
#endif // USE_UMBRA

#ifndef NO_RESOURCE_COOKING
void CStripeComponent::OnCook( class ICookerFramework& cooker )
{
	TBaseClass::OnCook( cooker );

	SRenderProxyStripeProperties tempData;
	GenerateStripeGeometry( &tempData );

	// cache vertex data
	m_cookedVertexCount = tempData.m_vertices.Size();
	m_cookedVertices.Allocate( (Uint32) tempData.m_vertices.DataSize() );
	Red::MemoryCopy( m_cookedVertices.GetData(), tempData.m_vertices.Data(), tempData.m_vertices.DataSize() );

	// cache index data
	m_cookedIndexCount = tempData.m_indices.Size();
	m_cookedIndices.Allocate( (Uint32) tempData.m_indices.DataSize() );
	Red::MemoryCopy( m_cookedIndices.GetData(), tempData.m_indices.Data(), tempData.m_indices.DataSize() );

	m_cookedBoundingBox = tempData.m_boundingBox;
}
#endif

///////////////////////////////////////////////////////////////////////////////
// CStripeComponentSetup
///////////////////////////////////////////////////////////////////////////////
PathLib::MetalinkClassId CStripeComponentSetup::GetClassId() const
{
	return PathLib::MetalinkClassId( EEngineMetalinkType::T_STRIPE );
}

Uint32 CStripeComponentSetup::GetMetalinkPathfollowFlags() const
{
	return METALINK_PATHFOLLOW_ALLOW_PATHOPTIMIZATION;
}
Bool CStripeComponentSetup::AgentPathfollowUpdate( RuntimeData& r, PathLib::CAgent* pathAgent, const Vector3& interactionPoint, const Vector3& destinationPoint )
{
	if ( pathAgent->GetPathfollowFlags() & PathLib::CAgent::PATHFOLLOW_SUPPORT_ROADS )
	{
		//pathAgent->DynamicEnablePathfollowingFlags( PathLib::CAgent::PATHFOLLOW_PRECISE );
		CComponent* component = r.GetEngineComponent( pathAgent );
		if ( component )
		{
			pathAgent->UseRoad( component );
		}
	}

	return true;
}

Bool CStripeComponentSetup::AgentPathfollowOver( RuntimeData& r, PathLib::CAgent* pathAgent, const Vector3& interactionPoint, const Vector3& destinationPoint )
{
	if ( pathAgent->GetPathfollowFlags() & PathLib::CAgent::PATHFOLLOW_SUPPORT_ROADS )
	{
		//pathAgent->DynamicDisablePathfollowingFlags( PathLib::CAgent::PATHFOLLOW_PRECISE );
		pathAgent->DontUseRoad();
	}

	return true;
}

Bool CStripeComponentSetup::AgentPathfollowIgnore( PathLib::CAgent* pathAgent )
{
	return ( pathAgent->GetPathfollowFlags() & PathLib::CAgent::PATHFOLLOW_SUPPORT_ROADS ) == 0;
}

