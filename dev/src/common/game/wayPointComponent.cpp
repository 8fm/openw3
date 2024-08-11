/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "wayPointComponent.h"

#include "../core/dataError.h"
#include "../core/gatheredResource.h"

#include "../engine/bitmapTexture.h"
#include "../engine/pathlibWorld.h"
#include "../engine/renderer.h"
#include "../engine/renderFragment.h"
#include "../engine/umbraScene.h"
#include "../engine/utils.h"
#include "../engine/viewport.h"
#include "../engine/worldIterators.h"

#include "gameWorld.h"
#include "wayPointCookingContext.h"


IMPLEMENT_ENGINE_CLASS( CWayPointComponent );

CGatheredResource resWayPointIcon( TXT("engine\\textures\\icons\\waypointicon.xbm"), RGF_NotCooked );

IRenderResource*	CWayPointComponent::m_markerValid;
IRenderResource*	CWayPointComponent::m_markerInvalid;
IRenderResource*	CWayPointComponent::m_markerWarning;
IRenderResource*	CWayPointComponent::m_communityMarkerValid;
IRenderResource*	CWayPointComponent::m_communityMarkerInvalid;
IRenderResource*	CWayPointComponent::m_communityMarkerWarning;
IRenderResource*	CWayPointComponent::m_markerNoMesh; 
IRenderResource*	CWayPointComponent::m_markerSelection;

CWayPointComponent::CWayPointComponent()
	: m_onCommunityLayer( false )
#ifndef WAYPOINT_COMPONENT_NO_VALIDITY_DATA
    , m_usedByPathLib( true )
    , m_isValid( VALIDITY_UNCALCULATED )
#endif
{
    // Set icon
    m_icon = resWayPointIcon.LoadAndGet< CBitmapTexture >();
	InitializeMarkers();
}

void CWayPointComponent::OnAttached( CWorld* world )
{
    // Pass to base class
    TBaseClass::OnAttached( world );

    // Register in the editor drawing system
    world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_Waypoints );

#ifndef RED_FINAL_BUILD
    // Report error if waypoint component is used inside CActor
    if ( GetEntity()->QueryActorInterface() )
    {
        String path = GetEntity()->GetEntityTemplate() ? GetEntity()->GetEntityTemplate()->GetDepotPath() : TXT("<empty template>");
        ERR_GAME( TXT("Actor has got CWayPointComponent! Actor: '%ls', resource: '%ls'"), GetEntity()->GetFriendlyName().AsChar(), path.AsChar() );
    }
#endif

	CEntity* parentEntity = GetEntity();
	if ( parentEntity )
	{
		parentEntity->ScheduleUpdateTransformNode();

#ifndef NO_EDITOR
		if ( CLayer* layer = parentEntity->GetLayer() )
		{
			m_onCommunityLayer = layer->IsCommunityLayer();
		}
#endif
	}

#ifndef NO_RUNTIME_WAYPOINT_COOKING
	if ( GGame->IsActive() )
	{
		CGameWorld* gameWorld = static_cast< CGameWorld* >( world );
		if ( CWayPointCookingContext* cookingContext = gameWorld->GetWaypointCookingContext() )
		{
			cookingContext->RegisterWaypoint( this );
		}
	}
#endif
}

void CWayPointComponent::OnDetached( CWorld* world )
{
    // Pass to base class
    TBaseClass::OnDetached( world );

    // Unregister from the editor drawing system
    world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_Waypoints );
}

#ifndef NO_EDITOR
void CWayPointComponent::OnNavigationCookerInitialization( CWorld* world, CNavigationCookingContext* context )
{
	context->Get< CWayPointCookingContext >()->RegisterWaypoint( this );
}
#endif		// NO_EDITOR

#ifdef USE_UMBRA
Bool CWayPointComponent::OnAttachToUmbraScene( CUmbraScene* umbraScene, const VectorI& bounds )
{
#ifndef NO_UMBRA_DATA_GENERATION
#ifdef USE_UMBRA_COOKING
	if ( umbraScene && umbraScene->IsDuringSyncRecreation() && GetEntity() )
	{
		return umbraScene->AddWaypoint( GetEntity()->GetPosition(), bounds );
	}
#endif // USE_UMBRA_COOKING
#endif // NO_UMBRA_DATA_GENERATION

	return false;
}
#endif // USE_UMBRA

#ifndef WAYPOINT_COMPONENT_NO_VALIDITY_DATA
Bool CWayPointComponent::IsPositionValid() const
{
	switch ( m_isValid )
	{
	case VALIDITY_INVALID:
		return false;
	default:
	case VALIDITY_VALID:
	case VALIDITY_WARRING:
		return true;
	case VALIDITY_UNCALCULATED:
		// TODO: integration hack
		const_cast< CWayPointComponent* >( this )->CheckLocation( GGame->GetActiveWorld(), false );
		return m_isValid != VALIDITY_INVALID;
	}
	return true;
}
#endif

#if !defined( WAYPOINT_COMPONENT_NO_VALIDITY_DATA ) && !defined( NO_EDITOR )
void CWayPointComponent::EditorOnTransformChanged()
{
    TBaseClass::EditorOnTransformChanged();

	m_isValid = VALIDITY_UNCALCULATED;
}
#endif

EAttachmentGroup CWayPointComponent::GetAttachGroup() const
{
    return ATTACH_GROUP_WAYPOINT;
}

#ifndef NO_DATA_VALIDATION
void CWayPointComponent::OnCheckDataErrors( Bool isInTemplate ) const
{
    // Pass to base class
    TBaseClass::OnCheckDataErrors( isInTemplate );

#ifndef WAYPOINT_COMPONENT_NO_VALIDITY_DATA
    // Invalid waypoint
    if ( m_usedByPathLib && !IsPositionValid() && !isInTemplate )
    {
		DATA_HALT( DES_Major, CResourceObtainer::GetResource( this ), TXT("Pathfinding"), TXT("Waypoint entity is invalid") );
    }
#endif
}
#endif // NO_DATA_VALIDATION

void CWayPointComponent::CheckLocation( CWorld* world, Bool isInitialAttach )
{
#ifndef WAYPOINT_COMPONENT_NO_VALIDITY_DATA
    PC_SCOPE( CheckWaypointLocation );
    ASSERT( world );
    
	// Update validity flag if used by path engine
    if ( m_usedByPathLib )
    {
        if ( world )
        {
            CPathLibWorld* pathlib = world->GetPathLibWorld();
			if ( pathlib )
			{
				Vector pos = GetWorldPosition();
				PathLib::AreaId areaId = pathlib->GetReadyAreaAtPosition( pos.AsVector3() );
				if ( areaId != PathLib::INVALID_AREA_ID )
				{
					if ( pathlib->TestLocation( areaId, pos.AsVector3(), pathlib->GetGlobalSettings().GetCategoryPersonalSpace( 0 ), PathLib::CT_DEFAULT ) )
					{
						m_isValid = VALIDITY_VALID;
					}
					else if ( pathlib->TestLocation( areaId, pos.AsVector3(), PathLib::CT_DEFAULT ) )
					{
						m_isValid = VALIDITY_WARRING;
					}
					else
					{
						m_isValid = VALIDITY_INVALID;
					}
				}
			}
        }
    }
#endif
}

Color CWayPointComponent::CalcSpriteColor() const
{
#ifndef WAYPOINT_COMPONENT_NO_VALIDITY_DATA
    // Show invalid waypoints
    if ( m_usedByPathLib && !IsPositionValid() )
    {
        return IsSelected() ? Color::LIGHT_RED : Color::RED;
    }
#endif

    // Valid waypoint
    return IsSelected() ? Color::LIGHT_GREEN : Color::WHITE;
}

void CWayPointComponent::RefreshWaypoints()
{
#ifndef WAYPOINT_COMPONENT_NO_VALIDITY_DATA
    CWorld * world = GGame->GetActiveWorld();
    if ( world )
    {
        // Get all components from game
		for ( WorldAttachedComponentsIterator it( world ); it; ++it )
		{
			CWayPointComponent * waypoint = Cast< CWayPointComponent >( *it );
			if ( waypoint )
			{
				waypoint->CheckLocation( world, false );
			}
		}
    }
#endif
}

template< class T, Uint32 sides >
struct AgentMesh
{
    TDynArray< DebugVertex > vertices;
    TDynArray< T > wireIndices;
    TDynArray< T > indices;

    AgentMesh( Float radius, Float height, const Color& color, Bool blendColors )
    {
        // Generate vertices of the lower ring at z=0
        T lowerRing = (T)vertices.Size();
        for ( Int32 i=0; i<sides; i++ )
        {
            const Float s = sinf( 2.0f * M_PI * i / ( Float ) sides );
            const Float c = cosf( 2.0f * M_PI * i / ( Float ) sides );
            Color vertexColor = blendColors ? Color::Mul3( color, 0.55f + 0.45f * c ) : color;
            vertexColor.A = color.A;
            new ( vertices ) DebugVertex( Vector( c * radius, s * radius, 0.0f ), vertexColor );
        }

        // Generate vertices of the higher ring at z=height
        const Uint32 color32 = color.ToUint32();
        Uint32 higherRing = vertices.Size();
        for ( Int32 i=0; i<sides; i++ )
        {
            const Float s = sinf( 2.0f * M_PI * i / ( Float ) sides );
            const Float c = cosf( 2.0f * M_PI * i / ( Float ) sides );
            Color vertexColor = blendColors ? Color::Mul3( color, 0.6f + 0.4f * c ) : color;
            vertexColor.A = color.A;
            new ( vertices ) DebugVertex( Vector( c * radius, s * radius, height ), vertexColor );
        }

        // Generate sides
        for ( Int32 i=0; i<sides; i++ )
        {
            T a = lowerRing + i;
            T b = lowerRing + ((i+1) % sides);
            T c = higherRing + ((i+1) % sides);
            T d = higherRing + i;

            // Generate two triangles
            indices.PushBack( c );
            indices.PushBack( b );
            indices.PushBack( a );
            indices.PushBack( d );
            indices.PushBack( c );
            indices.PushBack( a );

            // Wire indices uses only lines
            wireIndices.PushBack( a );
            wireIndices.PushBack( b );
            wireIndices.PushBack( c );
            wireIndices.PushBack( d );
            wireIndices.PushBack( a );
            wireIndices.PushBack( d );
            wireIndices.PushBack( b );
            wireIndices.PushBack( c );
        }

        // Generate upper cap vertices
        Uint32 upperCap = vertices.Size();
        for ( Int32 i=0; i<sides; i++ )
        {
            const Float s = sinf( 2.0f * M_PI * i / ( Float ) sides );
            const Float c = cosf( 2.0f * M_PI * i / ( Float ) sides );
            Color vertexColor = blendColors ? Color::Mul3( color, 1.0f ) : color;
            vertexColor.A = color.A;
            new ( vertices ) DebugVertex( Vector( c * radius, s * radius, height ), vertexColor );
        }

        // Generate lower cap vertices
        Uint32 lowerCap = vertices.Size();
        for ( Int32 i=0; i<sides; i++ )
        {
            const Float s = sinf( 2.0f * M_PI * i / ( Float ) sides );
            const Float c = cosf( 2.0f * M_PI * i / ( Float ) sides );
            Color vertexColor = blendColors ? Color::Mul3( color, 0.5f ) : color;
            vertexColor.A = color.A;
            new ( vertices ) DebugVertex( Vector( c * radius, s * radius, 0.0f ), vertexColor );
        }

        // Generate upper cap triangles
        for ( Int32 i=2; i<sides; i++ )
        {
            T a = upperCap;
            T b = upperCap + i-1;
            T c = upperCap + i;
            indices.PushBack( c );
            indices.PushBack( b );
            indices.PushBack( a );
        }

        // Generate lower cap triangles
        for ( Int32 i=2; i<sides; i++ )
        {
            T a = lowerCap;
            T b = lowerCap + i-1;
            T c = lowerCap + i;
            indices.PushBack( a );
            indices.PushBack( b );
            indices.PushBack( c );
        }
    }
};

IRenderResource* CWayPointComponent::CreateAgentMesh( Float radius, Float height, const Color& color, Bool wireframe )
{
    // Upload mesh
    AgentMesh< Uint32, 32 > mesh( radius, height, color, !wireframe );
    return GRender->UploadDebugMesh( mesh.vertices, wireframe ? mesh.wireIndices : mesh.indices );
}

void CWayPointComponent::InitializeMarkers()
{
	struct InitOnce
	{
		enum EColors
		{
			EColors_Green,
			EColors_Red,
			EColors_LightGreen,
			EColors_DarkCyan,
			EColors_Magenta,
			EColors_Cyan,
			EColors_Gray,
			EColors_White
		};

		InitOnce( )
		{
			TDynArray< Color > colors;
			colors.PushBack( Color::DARK_GREEN );
			colors.PushBack( Color::RED );
			colors.PushBack( Color::LIGHT_GREEN );
			colors.PushBack( Color::DARK_CYAN );
			colors.PushBack( Color::LIGHT_MAGENTA );
			colors.PushBack( Color::CYAN );
			colors.PushBack( Color::GRAY );
			colors.PushBack( Color::WHITE );

			ForEach( colors.Begin(), colors.End(), []( Color& c ){ c.A = 200; } );

			CWayPointComponent::m_markerValid = CreateAgentMesh( 0.4f, 1.8f, colors[ EColors_Green ] );
			CWayPointComponent::m_markerInvalid = CreateAgentMesh( 0.4f, 1.8f, colors[ EColors_Red ] );
			CWayPointComponent::m_markerWarning = CreateAgentMesh( 0.4f, 1.8f, colors[ EColors_LightGreen ] );
			CWayPointComponent::m_communityMarkerValid = CreateAgentMesh( 0.4f, 1.8f, colors[ EColors_DarkCyan ] );
			CWayPointComponent::m_communityMarkerInvalid = CreateAgentMesh( 0.4f, 1.8f, colors[ EColors_Magenta ] );
			CWayPointComponent::m_communityMarkerWarning = CreateAgentMesh( 0.4f, 1.8f, colors[ EColors_Cyan ] );
			CWayPointComponent::m_markerNoMesh = CreateAgentMesh( 0.4f, 1.8f, colors[ EColors_Gray ] );
			CWayPointComponent::m_markerSelection = CreateAgentMesh( 0.44f, 1.8f, colors[ EColors_White ], true );
		}
	};
	static InitOnce initOnce;
}

IRenderResource* CWayPointComponent::GetMarkerValid(){ return m_onCommunityLayer ? m_communityMarkerValid : m_markerValid; }
IRenderResource* CWayPointComponent::GetMarkerInvalid(){ return m_onCommunityLayer ? m_communityMarkerInvalid : m_markerInvalid; }
IRenderResource* CWayPointComponent::GetMarkerWarning(){ return m_onCommunityLayer ? m_communityMarkerWarning : m_markerWarning; }
IRenderResource* CWayPointComponent::GetMarkerNoMesh(){ return m_markerNoMesh; }
IRenderResource* CWayPointComponent::GetMarkerSelection(){ return m_markerSelection; }	

void CWayPointComponent::WaypointGenerateEditorFragments( CRenderFrame* frame )
{
#ifndef WAYPOINT_COMPONENT_NO_VALIDITY_DATA
	if ( m_usedByPathLib )
	{
		Matrix translationOnly = Matrix::IDENTITY;
		const Vector& position = GetWorldPositionRef();
		translationOnly.SetTranslation( position );
		
		// Hit proxy
		if ( frame->GetFrameInfo().m_renderingMode == RM_HitProxies )
		{
#ifndef NO_COMPONENT_GRAPH
			new ( frame ) CRenderFragmentDebugMesh( frame, translationOnly, GetMarkerNoMesh(), GetHitProxyID() );
#endif
			return;
		}

		// Display special objects
		if ( m_isValid == VALIDITY_UNCALCULATED )
		{
			CWorld *world = GetLayer()->GetWorld();
			CheckLocation( world, true );
		}
		switch( m_isValid )
		{
		case VALIDITY_INVALID:
			{
				static const String S(TXT("Invalid navi placement"));
				frame->AddDebugText( position + Vector( 0,0,1 ), S, true, Color::RED );
				new ( frame ) CRenderFragmentDebugMesh( frame, translationOnly, GetMarkerInvalid(), true );
			}
			break;
		case VALIDITY_VALID:
			new ( frame ) CRenderFragmentDebugMesh( frame, translationOnly, GetMarkerValid(), true );
			break;
		case VALIDITY_WARRING:
			new ( frame ) CRenderFragmentDebugMesh( frame, translationOnly, GetMarkerWarning(), true );
			break;
		}

		// Selected
		if ( IsSelected() )
		{
			new ( frame ) CRenderFragmentDebugWireMesh( frame, translationOnly, GetMarkerSelection() );
		}

		// Display direction
		frame->AddDebug3DArrow( position + Vector(0, 0, 0.5f), GetWorldRotation().ToMatrix().GetAxisY(), 1, 0.035f, 0.045f, 0.12f, Color::BLUE, Color::RED );
	}
#endif
}

void CWayPointComponent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag )
{
    // Pass to base class
    TBaseClass::OnGenerateEditorFragments( frame, flag );

	if ( flag == SHOW_Waypoints && !frame->GetFrameInfo().IsClassRenderingDisabled( GetClass() ) )
	{
		// Waypoints uses only simple matrix
		const Vector& position = GetWorldPositionRef();

		// Disable rendering of debug meshes with distance (filters option)
		CRenderFrameInfo &info = frame->GetFrameInfo();
		Float visualDebugMaxRenderDistance = info.GetRenderingDebugOption( VDCommon_MaxRenderingDistance );
		if( visualDebugMaxRenderDistance > 0.0001f && ( position - info.m_camera.GetPosition()  ).SquareMag3() > visualDebugMaxRenderDistance )
		{
			return;
		}	
		WaypointGenerateEditorFragments( frame );
	}
}

void CWayPointComponent::SetUsedByPathLib( Bool value )
{
#ifndef WAYPOINT_COMPONENT_NO_VALIDITY_DATA
    m_usedByPathLib = value;
    PerformFullRecreation();
#endif
}

void CEntity::funcFindWaypoint( CScriptStackFrame& stack, void* result )
{
    GET_PARAMETER( String, waypointName, String::EMPTY );
    FINISH_PARAMETERS;

    // PAKSAS TODO: ???? HACK HACK HACK ???
    CComponent* comp = Cast< CWayPointComponent >( FindComponent( waypointName ) );
    RETURN_OBJECT( comp );
}
