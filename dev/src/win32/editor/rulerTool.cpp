#include "build.h"
#include "../../common/game/actionAreaVertex.h"
#include "rulerTool.h"
#include "../../common/core/gatheredResource.h"
#include "../../common/engine/hitProxyObject.h"
#include "../../common/engine/hitProxyMap.h"
#include "../../common/engine/dynamicLayer.h"
#include "../../common/engine/renderFrameInfo.h"
#include "../../common/engine/renderFrame.h"
#include "../../common/engine/fonts.h"

CGatheredResource resRulerTextFont( TXT("gameplay\\gui\\fonts\\arial.w2fnt"), RGF_Startup );

IMPLEMENT_ENGINE_CLASS( CEdRulerTool );

CEdRulerTool::CEdRulerTool()
: m_world( NULL )
, m_viewport( NULL )
, m_rulerStartPoint( NULL )
, m_rulerEndPoint( NULL )
, m_hoveredPoint( NULL )
, m_font( NULL )
, m_mode( RM_Normal )
{
    m_font = resRulerTextFont.LoadAndGet< CFont >();
}

CEdRulerTool::~CEdRulerTool()
{
}

Bool CEdRulerTool::Start( CEdRenderingPanel* viewport, CWorld* world, wxSizer* sizer, wxPanel* panel, const TDynArray< CComponent* >& selection )
{
	// Remember world
	m_world = world;

	// Remember viewport
	m_viewport = viewport;
    m_world->GetSelectionManager()->DeselectAll();

    return true;
}

void CEdRulerTool::End()
{
    DestroyRulerPointEntity( m_rulerStartPoint );
    DestroyRulerPointEntity( m_rulerEndPoint );
}

Bool CEdRulerTool::HandleSelection( const TDynArray< CHitProxyObject* >& objects )
{
    CSelectionManager::CSelectionTransaction transaction(*m_world->GetSelectionManager());

    // Deselect all selected object
    if ( !RIM_IS_KEY_DOWN( IK_Shift ) )
    {
        m_world->GetSelectionManager()->DeselectAll();
    }

	// Select only vertices
	for ( Uint32 i=0; i<objects.Size(); i++ )
	{
		CVertexEditorEntity* tc = Cast< CVertexEditorEntity >( objects[i]->GetHitObject()->GetParent() );
		if ( tc && !tc->IsSelected() )
		{
			m_world->GetSelectionManager()->Select( tc );
		}
	}

	// Handled
	return true;
}

Bool CEdRulerTool::OnViewportClick( IViewport* view, Int32 button, Bool state, Int32 x, Int32 y )
{
	if ( button == 0 )
	{
		//return HandleActionClick( x, y );

        if ( state )
        {
            return OnLeftButtonDown( view, x, y );
        }
        else
        {
            return OnLeftButtonUp( view, x, y );
        }
	}

	return false;
}

Bool CEdRulerTool::OnLeftButtonDown( IViewport* view, Int32 x, Int32 y )
{
    m_stopWatch.Start();

    // Do not filter
    return false;
}

Bool CEdRulerTool::OnLeftButtonUp( IViewport* view, Int32 x, Int32 y )
{
    const Bool leftClick = ( m_stopWatch.Time() < 200 );
    m_stopWatch.Pause();

    if ( m_mode == RM_PickingPoint )
    {
        m_mode = RM_Normal;
    }
    else if ( leftClick )
    {
        if ( m_rulerStartPoint == NULL || m_hoveredPoint == NULL )
        {
			Vector intersectionPoint;
            if ( m_world->ConvertScreenToWorldCoordinates( view, x, y, intersectionPoint ) )
            {
                if ( m_rulerStartPoint == NULL )
                {
                    m_rulerStartPoint = CreateRulerPointEntity( intersectionPoint );
                    m_rulerEndPoint = CreateRulerPointEntity( intersectionPoint );
                    m_mode = RM_PickingPoint;

                    if ( !m_rulerEndPoint || !m_rulerEndPoint )
                    {
                        WARN_EDITOR( TXT("Cannot create ruler point entities - no active layer found.") );
                        return false;
                    }
                }
				else
                {
                    m_rulerEndPoint->SetPosition( intersectionPoint );
                }
            }
        }
    }

    // Do not filter
    return false;
}

Bool CEdRulerTool::OnViewportTrack( const CMousePacket& packet )
{
	const Vector testPoint( packet.m_x, packet.m_y, 0.0f );

	if ( !m_rulerStartPoint || !m_rulerEndPoint )
	{
		return false;
	}

    if ( m_mode == RM_PickingPoint )
    {
        ASSERT( m_rulerEndPoint );
        Vector worldPoint;
		if ( m_world->ConvertScreenToWorldCoordinates( m_viewport->GetViewport(), packet.m_x, packet.m_y, worldPoint ) )
        {
            m_rulerEndPoint->SetPosition( worldPoint );
        }
    }
    else
    {
		// Follow the cursor if shift or ctrl is down
		Bool shiftDown = RIM_IS_KEY_DOWN( IK_LShift );
		Bool ctrlDown  = RIM_IS_KEY_DOWN( IK_LControl );
		if ( shiftDown || ctrlDown )
		{
			Vector intersectionPoint;
			if ( m_world->ConvertScreenToWorldCoordinates( m_viewport->GetViewport(), packet.m_x, packet.m_y, intersectionPoint ) )
			{
				if ( ctrlDown )
					m_rulerStartPoint->SetPosition( intersectionPoint );
				else
					m_rulerEndPoint->SetPosition( intersectionPoint );
			}
		}

        // Invalidate fragment renderer data
        if ( m_hoveredPoint )
        {
            m_hoveredPoint->m_hovered = false;
        }

        // Do hit proxy check on vertices
        CHitProxyMap map;
        CHitProxyObject* object = m_viewport->GetHitProxyAtPoint( map, packet.m_x, packet.m_y );
        m_hoveredPoint = object ? Cast< CVertexEditorEntity >( object->GetHitObject()->GetParent() ) : NULL;
        if ( m_hoveredPoint )
        {
            m_hoveredPoint->m_hovered = true;
        }
    }

    // Not filtered
	return false;
}

Bool CEdRulerTool::OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
{
    if ( m_rulerStartPoint && m_rulerEndPoint )
    {
        const CRenderFrameInfo &frameInfo = frame->GetFrameInfo();
        const Vector startPoint = m_rulerStartPoint->GetWorldPosition();
        const Vector endPoint = m_rulerEndPoint->GetWorldPosition();
        const Vector middlePoint = ( startPoint + endPoint ) * 0.5f;
        const Vector cameraPosition = frameInfo.m_camera.GetPosition();
        Vector &rulerUp = middlePoint - cameraPosition;
        rulerUp.Normalize3();
        Vector rulerForward = endPoint - startPoint;
        rulerForward.Normalize3();
        Vector rulerRight = Vector::Cross( rulerUp, rulerForward );
        rulerRight.Normalize3();
        rulerRight *= 0.5f;

        // Draw ruler line
        frame->AddDebugLine( startPoint, endPoint, Color::YELLOW, true );
        frame->AddDebugLine( startPoint + rulerRight, startPoint - rulerRight, Color::YELLOW, true );
        frame->AddDebugLine( endPoint + rulerRight, endPoint - rulerRight, Color::YELLOW, true );

        // Display ruler's length in the middle of the ruler
        if ( m_font )
        {
            Int32 x, y;
            Uint32 width, height;
            Vector screenPointStart;
            Vector screenPointEnd;
            Vector screenPointMiddle;
            const Float rulerLength = endPoint.DistanceTo( startPoint );
            const String text = String::Printf( TXT("%.2f"), rulerLength );
            m_font->GetTextRectangle( text, x, y, width, height );
            frameInfo.ProjectPoints( &startPoint, &screenPointStart, 1 );
            frameInfo.ProjectPoints( &endPoint, &screenPointEnd, 1 );
            screenPointMiddle = (screenPointStart + screenPointEnd) * 0.5f;
            frame->AddDebugScreenText( (Int32)screenPointMiddle.X - width / 2, (Int32)screenPointMiddle.Y - height - 5, text, Color::GREEN, m_font );
            frame->AddDebugScreenText( 10, 10 + height / 2, text, Color::GREEN, m_font );
        }
    }

	// Not filtered
	return false;
}

CVertexEditorEntity *CEdRulerTool::CreateRulerPointEntity( const Vector &position ) const
{
    // Create the ruler entity on the dynamic layer
    if ( CLayer *dynLayer = m_world->GetDynamicLayer() )
    {
        EntitySpawnInfo sinfo;
        sinfo.m_spawnPosition = position;
        sinfo.m_entityClass   = CVertexEditorEntity::GetStaticClass();

		// Create entity
		CVertexEditorEntity * vertexEntity = Cast< CVertexEditorEntity >( dynLayer->CreateEntitySync( sinfo ) );
		if ( ! vertexEntity )
		{
			ASSERT( vertexEntity );
			return NULL;
		}

		SComponentSpawnInfo spawnInfo;
		spawnInfo.m_name = TXT("Vertex");
		vertexEntity->CreateComponent( CVertexComponent::GetStaticClass(), spawnInfo );
        return vertexEntity;
    }

    return NULL;
}

void CEdRulerTool::DestroyRulerPointEntity( CVertexEditorEntity *rulerPoint )
{
    ASSERT( m_world );
    m_world->GetSelectionManager()->DeselectAll();

    if ( rulerPoint )
    {
		rulerPoint->Destroy();
    }
}

void CEdRulerTool::GetClosestPointToSegment( const Vector &srcPoint, const Vector &a, const Vector &b, Vector &closestPoint ) const
{
    Vector edge = (b - a).Normalized3();
    Float ta = Vector::Dot3( edge, a );
    Float tb = Vector::Dot3( edge, b );
    Float p = Vector::Dot3( edge, srcPoint );
    if ( p >= ta && p <= tb )
    {
        closestPoint = a + edge * (p - ta);
    }
    else if ( p < ta )
    {
        closestPoint = a;
    }
    else
    {
        closestPoint = b;
    }
}

