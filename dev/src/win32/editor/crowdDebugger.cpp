#include "build.h"

#if 0
#include "crowdDebugger.h"

#include "../../games/r6/crowdAgent.h"
#include "../../games/r6/crowdManager.h"
#include "../../games/r6/gridCrawler.inl"

// Event table
BEGIN_EVENT_TABLE( CCrowdDebuggerCanvas, CMaraudersMapCanvasDrawing )
END_EVENT_TABLE()

const Vector CCrowdDebuggerCanvas::DEFAULT_HALF_EXTENTS( 25.f, 25.f, 0.f);

CCrowdDebuggerCanvas::CCrowdDebuggerCanvas( wxWindow* parent )
	: CMaraudersMapCanvasDrawing( parent )
	, m_currentRegionPos( Vector( FLT_MAX, FLT_MAX, FLT_MAX )  )
	, m_lastRegionUpdateTime( 0.f )
{

}

/////////////////////////////////////////////////////////////////////////////////////////////////
// Warning - messy code here, used to debug and profile the prototype
// It will be cleaned at some point...
/////////////////////////////////////////////////////////////////////////////////////////////////

void CCrowdDebuggerCanvas::PaintCanvas( Int32 width, Int32 height )
{
	static wxColour backgroundColor( 0, 0, 0 );
	static wxColour playerColor( 255, 0, 0 );
	static wxColour textColor( 255, 255, 255 );
	static wxColour borderColor( 128, 128, 128 );
	static wxColour agentColor( 200, 200, 200 );
	static wxColour closeAgentColor( 255, 127, 127 );
	static wxColour obstacleColor( 200, 80, 40 );
	static wxColour nearObstacleColor( 255, 192, 0 );
	static wxColour errorColor( 255, 0, 0 );
	static wxColour inactiveColor( 80, 80, 80 );

	Clear( backgroundColor );

	// Get the crowd system
	const CCrowdManager* mgr = GCommonGame->GetSystem< const CCrowdManager > (); 
	if ( nullptr == mgr )
	{
		return;
	}

	// Follow the player
	UpdateRegion( mgr->GetCenterPoint2() );

	// Draw The Grid
	Box2 aabb = mgr->m_crowdSpace.m_aabb; 
	for ( Float x = aabb.Min.X; x <= aabb.Max.X; x += GRID_CELL_SIZE )
	{
		DrawLineCanvas( x, aabb.Min.Y, x, aabb.Max.Y, borderColor );	
	}

	for ( Float y = aabb.Min.Y; y <= aabb.Max.Y; y += GRID_CELL_SIZE )
	{
		DrawLineCanvas( aabb.Min.X, y, aabb.Max.X, y, borderColor );
	}

	const CEntity* playerEntity = GCommonGame->GetPlayerEntity();
	if ( playerEntity )
	{
		// DrawBoxCanvas( Box( m_currentRegionPos - DEFAULT_HALF_EXTENTS, m_currentRegionPos + DEFAULT_HALF_EXTENTS ), borderColor );

		// Draw the player as a red circle
		const Float playerRadius = Cast< const CActor > ( playerEntity )->GetRadius();
		DrawCircleCanvas( playerEntity->GetWorldPositionRef(), playerRadius, playerColor );		



		// Get 10 agents nearest to the player
// 		TDynArray< TAgentIndex > nearest( 10 );
// 		TAgentIndex n;
// 		{
// 			CROWD_PROFILE_SCOPE( d_crowdGrid_NAgents )
// 			//for ( Uint32 i = 0; i < 1000; ++i )
// 			{
// 				n = mgr->GetCrowdSpace().GetNearestAgentsWithinRadius( playerEntity->GetWorldPositionRef().AsVector2(), 2.f, 10, nearest.TypedData() );
// 			}
// 		}
// 		nearest.ResizeFast( n );
// 
// #ifdef DEBUG_CROWD
// 		// Check the results with cs naive
// 		TDynArray< TAgentIndex > nearestNaive( 10 );
// 		TAgentIndex nNaive;
// 		{
// 			CROWD_PROFILE_SCOPE( d_crowdNaive_NAgents )
// 			//for ( Uint32 i = 0; i < 1000; ++i )
// 			{
// 				nNaive = mgr->GetCrowdSpaceNaive_Debug().GetNearestAgentsWithinRadius( playerEntity->GetWorldPositionRef().AsVector2(), 2.f, 10, nearestNaive.TypedData() );
// 			}
// 		}
// 		nearestNaive.ResizeFast( nNaive );
// 
// 		ASSERT( n == nNaive );
// 		for ( TAgentIndex i = 0; i < min( n, nNaive ); ++i )
// 		{
// 			const Float gridDistSq = ( playerEntity->GetWorldPositionRef().AsVector2() - mgr->GetAgentPos2( nearest[ i ] ) ).SquareMag();
// 			const Float naiveDistSq = ( playerEntity->GetWorldPositionRef().AsVector2() - mgr->GetAgentPos2( nearestNaive[ i ] ) ).SquareMag();
// 			ASSERT( gridDistSq == naiveDistSq );
// 		}
// #endif

		// Raycast!
 		SCrowdRay2 rayAny, rayClosest;
 		TDynArray< TAgentIndex > nearest( 3 );
 
 		rayAny.m_start = playerEntity->GetWorldPosition().AsVector2() + Vector2( 2.f, 4.f );
 		rayClosest.m_start = playerEntity->GetWorldPosition().AsVector2() - Vector2( 2.f, 4.f );
 		rayAny.m_end = rayAny.m_start + Vector2( 3.f, 1.5f );
 		rayClosest.m_end = rayClosest.m_start - Vector2( 13.f, 15.f );
		rayClosest.m_radius = 1.f;
 
 		Double time;
 		TAgentIndex n;
 		Bool hitAny;
 		{
 			Red::System::ScopedStopClock clk( time );
 
 			hitAny = mgr->GetCrowdSpace().Ray2Test_Any( rayAny );
 			n = mgr->GetCrowdSpace().Ray2Test_NClosest( rayClosest, 3, nearest.TypedData() );
 		}
 		nearest.ResizeFast( n );

		for ( CCrowdSpace_Grid::SGridCrawlerRay crawler( rayAny, aabb.Min ); crawler; ++crawler )
		{
			const Vector2 cellMin( Float( crawler.X ) * GRID_CELL_SIZE + 0.1f, Float( crawler.Y ) * GRID_CELL_SIZE + 0.1f );	
			const Vector2 cellMax( cellMin.X + GRID_CELL_SIZE - 0.2f, cellMin.Y + GRID_CELL_SIZE - 0.2f );
			DrawBoxCanvas( Box( cellMin + aabb.Min, cellMax + aabb.Min ), closeAgentColor );
		}

		for ( CCrowdSpace_Grid::SGridCrawlerRay crawler( rayClosest, aabb.Min ); crawler; ++crawler )
		{
			const Vector2 cellMin( Float( crawler.X ) * GRID_CELL_SIZE + 0.1f, Float( crawler.Y ) * GRID_CELL_SIZE + 0.1f );	
			const Vector2 cellMax( cellMin.X + GRID_CELL_SIZE - 0.2f, cellMin.Y + GRID_CELL_SIZE - 0.2f );
			DrawBoxCanvas( Box( cellMin + aabb.Min, cellMax + aabb.Min ), closeAgentColor );
		}

		// Draw rays
		DrawLineCanvas( rayAny.m_start, rayAny.m_end, hitAny ? closeAgentColor : agentColor );
		DrawLineCanvas( rayClosest.m_start, rayClosest.m_end, agentColor );


// 		Double time;
// 		CFrustum frustum;
// 		GGame->GetActiveWorld()->GetCameraDirector()->CalcCameraFrustum( frustum );
// 
// 		TDynArray< TAgentIndex > agentsInFrustum( 1000 );
// 		TAgentIndex n;
// 		{
// 			Red::System::ScopedStopClock clk( time );
// 			n = mgr->GetCrowdSpace().GetAgentsInFrustum( frustum, 1000, agentsInFrustum.TypedData() );
// 		}
// 		agentsInFrustum.ResizeFast( n );

		// obstacles near player
		TDynArray< TObstacleIndex > result( 100 );
		n = mgr->GetCrowdSpace().GetObstaclesWithinRadius( playerEntity->GetWorldPositionRef().AsVector2(), 3.f, 100, result.TypedData() );
		result.ResizeFast( n );

		// Draw obstacles
		const TObstacleIndex numObstacles = mgr->GetNumObstacles();
		for ( TObstacleIndex i = 0; i < numObstacles; ++i )
		{
			DrawLineCanvas( mgr->GetObstacle( i ).m_begin, mgr->GetObstacle( i ).m_end, result.Exist( i ) ? nearObstacleColor : ( mgr->GetObstacle( i ).IsValid() ? obstacleColor : errorColor ) );
		}

		// Draw all agents
		const TAgentIndex num = mgr->GetNumAgents();
		for ( TAgentIndex i = 0; i < num; ++i )
		{
			DrawCircleCanvas( mgr->GetAgentPos2( i ), CCrowdManager::AGENT_RADIUS, mgr->IsAgentActive( i ) ? ( nearest.Exist( i ) ? closeAgentColor : agentColor ) : inactiveColor );
		}
	}
	else
	{
		DrawBoxCanvas( Box( -DEFAULT_HALF_EXTENTS, DEFAULT_HALF_EXTENTS ), borderColor );
		DrawTextCanvas( 0.f, 0.f, GetGdiDrawFont(), TXT("No player in game.\nThis tool is usable only when the game is running."), textColor, CVA_Center, CHA_Center );
	}
}

void CCrowdDebuggerCanvas::UpdateRegion( const Vector& pos )
{
	EngineTime t = GEngine->GetRawEngineTime();
	
	// Update each 5 sec
	if ( ( t - m_lastRegionUpdateTime ) > 5.f && !Vector::Equal3( pos, m_currentRegionPos ) )
	{
		SetZoomedRegion( pos - DEFAULT_HALF_EXTENTS, pos + DEFAULT_HALF_EXTENTS );
		m_lastRegionUpdateTime = t;
		m_currentRegionPos = pos;
	}
}

void CCrowdDebuggerCanvas::DrawBoxCanvas( const Box& worldBox, const wxColour& color, Float width /*= 1.0f */ )
{
	const Vector points[] =
	{
		worldBox.Min,
		Vector( worldBox.Max.X, worldBox.Min.Y, 0.f ),
		worldBox.Max,
		Vector( worldBox.Min.X, worldBox.Max.Y, 0.f )
	};

	DrawPolyCanvas( points, 4, color, width );
}

// Event table
BEGIN_EVENT_TABLE( CEdCrowdDebugger, wxSmartLayoutPanel )
	EVT_MENU( XRCID( "m_menuExit" ), CEdCrowdDebugger::OnExit )
	EVT_CLOSE( CEdCrowdDebugger::OnClose )
END_EVENT_TABLE()

CEdCrowdDebugger::CEdCrowdDebugger( wxWindow* parent )
	: wxSmartLayoutPanel( parent, TXT("CrowdDebugger"), true )
{
	wxPanel* panelCanvas = XRCCTRL( *this, "m_panelCanvas", wxPanel );
	if ( panelCanvas )
	{
		m_canvas = new CCrowdDebuggerCanvas( panelCanvas );
		m_canvas->SetSize( panelCanvas->GetSize() );
		panelCanvas->SetSizer( new wxBoxSizer( wxVERTICAL ) );
		panelCanvas->GetSizer()->Add( m_canvas, 1, wxEXPAND );
	}
	LoadOptionsFromConfig();
	Layout();

	// Set timer
	m_timer = new CEdTimer();
	m_timer->Connect( wxEVT_TIMER, wxCommandEventHandler( CEdCrowdDebugger::OnTimer ), NULL, this );
	m_timer->Start( 200 ); // 5 fps?
}

CEdCrowdDebugger::~CEdCrowdDebugger()
{
	delete m_timer;
}

void CEdCrowdDebugger::SaveOptionsToConfig()
{
	SaveLayout( TXT("/Frames/CrowdDebugger") );
}

void CEdCrowdDebugger::LoadOptionsFromConfig()
{
	LoadLayout( TXT("/Frames/CrowdDebugger") );
}

void CEdCrowdDebugger::OnExit( wxCommandEvent &event )
{
	Close();
}

void CEdCrowdDebugger::OnClose( wxCloseEvent &event )
{
	SaveOptionsToConfig();
}

void CEdCrowdDebugger::OnTimer( wxCommandEvent &event )
{
	m_canvas->Repaint();
}
#endif