/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "editorToolContext.h"
#include "../../common/engine/clipMap.h"

#ifndef NO_RED_GUI
#include "../../common/engine/redGuiManager.h"
#endif	// NO_RED_GUI
#include "../../common/engine/hitProxyMap.h"
#include "../../common/engine/viewport.h"
#include "../../common/engine/renderFragment.h"
#include "../../common/engine/renderer.h"
#include "../../common/engine/drawableComponent.h"

enum
{
	DRAG_TIMER_ID = 10,
};

BEGIN_EVENT_TABLE( CEdRenderingPanel, wxPanel )
	EVT_ERASE_BACKGROUND( CEdRenderingPanel::OnEraseBackground )
	EVT_PAINT( CEdRenderingPanel::OnPaint )
END_EVENT_TABLE()


#define RENDERINGPANEL_CAMERA_NEAR		0.2f
#define RENDERINGPANEL_CAMERA_FAR		1900.f
#define RENDERINGPANEL_CAMERA_ZOOM		1.0f

CEdRenderingPanel::CEdRenderingPanel( wxWindow* parent )
	: wxPanel( parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxCLIP_CHILDREN | wxWANTS_CHARS )
	, CDropTarget( this )
	, m_cameraPosition( 0.f,-3.f,0.f )
	, m_cameraSpeedMultiplier( 1.f )
	, m_cameraInputRotMul( 0.32f )
	, m_cameraInputPosMul( 0.064f )
	, m_cameraInputPosMulLevel( 0 )
	, m_cameraRotation( 0.f,0.f,0.f )
	, m_cameraZoom( 3.0f )
	, m_cameraFov( 60.0f )
	, m_cameraMode( RPCM_DefaultFlyby )
	, m_mouseButtonFlags( 0 )
	, m_selectionBoxDrag( 0 )
    , m_tool( NULL )
	, m_moveTillCapture( 0 )
	, m_isAfterDragDelay( false )
	, m_dragDelay( 150 * 60 )		// Multiply by normal tick rate (when applying to timer - delay is divided by last tick rate)
	, m_dragTimer( this, DRAG_TIMER_ID )
	, m_viewportCachetAspectRatio( IViewport::EAspectRatio::FR_16_9 )
{
	// Create sizer
	m_sizer = new wxBoxSizer( wxVERTICAL );

	// Create rendering window
	m_renderingWindow = new CEdRenderingWindow( this );

	// If rendering window desn't have a viewport, exit the application
	if ( m_renderingWindow->GetViewport() == NULL )
	{
		exit( 0 );	
	}

	m_sizer->Add( m_renderingWindow, 1, wxEXPAND | wxALL, 0 );

	// Enable grid rendering
	m_renderingWindow->GetViewport()->SetRenderingMask( SHOW_Grid );

	// Register viewport hook
	m_renderingWindow->GetViewport()->SetViewportHook( this );

	// Create widget manager
	m_widgetManager = new CViewportWidgetManager( this );

    m_editContext = new CEdEditorToolContext();
	
	Connect( wxEVT_KEY_DOWN, wxKeyEventHandler( CEdRenderingPanel::OnKeyDown ), NULL, this );

	// Create dragging start delay timer handler connection
	Connect( wxEVT_TIMER, wxTimerEventHandler( CEdRenderingPanel::OnTimerTimeout ), nullptr, this );

	// Update layout
	SetSizer( m_sizer );
	Layout();

	// Clear key
	ResetCameraMoveKeys();
	OnCameraMoved();

	InitPersistentTools();
}

void CEdRenderingPanel::InitPersistentTools()
{
	// Enumerate tools
	TDynArray< CClass* > editorToolsClasses;
	SRTTI::GetInstance().EnumClasses( ClassID< IEditorTool >(), editorToolsClasses );
	for ( Uint32 i=0; i<editorToolsClasses.Size(); i++ )
	{
		CClass* toolClass = editorToolsClasses[i];
		IEditorTool * tool = toolClass->GetDefaultObject<IEditorTool>();
#if 0
		// Skip tools improper outside the main world editor
		if ( tool->UsableInActiveWorldOnly() && wxTheFrame->GetSolutionBar() != this )
		{
			continue;
		}
#endif
		if ( !tool->IsPersistent() )
		{
			continue;
		}

		// Create tool
		tool = CreateObject< IEditorTool >( toolClass );
		if ( !tool )
		{
			continue;
		}

		// Start tool
		if ( tool->StartPersistent( this ) )
		{
			tool->AddToRootSet();
			m_persistentTools.PushBack( tool );
		}
	}
}

void CEdRenderingPanel::OnKeyDown( wxKeyEvent& event ) {
	event.StopPropagation();
}
CEdRenderingPanel::~CEdRenderingPanel()
{
	for ( auto it = m_persistentTools.Begin(); it != m_persistentTools.End(); ++it )
	{
		(*it)->End();
		(*it)->RemoveFromRootSet();
	}

    delete m_editContext;

	if ( m_widgetManager )
	{
		delete m_widgetManager;
		m_widgetManager = NULL;
	}
}

Color CEdRenderingPanel::GetClearColor() const
{
	return Color ( 16, 16, 16 );
}

Bool CEdRenderingPanel::ShouldDrawGrid() const
{
	return true;
}

Bool rotate = false;

Bool CEdRenderingPanel::OnViewportMouseMove( const CMousePacket& packet )
{
    if ( m_tool && m_tool->OnViewportMouseMove( packet ) )
    {
        return true;
    }

	for ( auto it = m_persistentTools.Begin(); it != m_persistentTools.End(); ++it )
	{
		if ( (*it)->OnViewportMouseMove( packet ) )
		{
			return true;
		}
	}

    if ( m_widgetManager->OnViewportMouseMove( packet ) )
	{
		return true;
	}

	return false;
}

Bool CEdRenderingPanel::OnViewportTrack( const CMousePacket& packet )
{
    if ( m_widgetManager->OnViewportTrack( packet ) )
	{
		return true;
	}

    if ( m_tool && m_tool->OnViewportTrack( packet ) )
    {
        return true;
    }

	for ( auto it = m_persistentTools.Begin(); it != m_persistentTools.End(); ++it )
	{
		if ( (*it)->OnViewportTrack( packet ) )
		{
			return true;
		}
	}

	return false;
}


void CEdRenderingPanel::OnViewportKillFocus(IViewport* view)
{
    if ( m_tool && m_tool->OnViewportKillFocus(view) )
    {
            return;
    }

	for ( auto it = m_persistentTools.Begin(); it != m_persistentTools.End(); ++it )
	{
		if ( (*it)->OnViewportKillFocus( view ) )
		{
			return;
		}
	}
}

void CEdRenderingPanel::OnViewportSetFocus(IViewport* view)
{
    if ( m_tool && m_tool->OnViewportSetFocus(view) )
    {
        return;
    }

	for ( auto it = m_persistentTools.Begin(); it != m_persistentTools.End(); ++it )
	{
		if ( (*it)->OnViewportSetFocus( view ) )
		{
			return;
		}
	}
}

Bool CEdRenderingPanel::OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
{
    // Let the edit mode handle event
    if ( m_tool && m_tool->OnViewportInput( view, key, action, data ) )
    {
        return true;
    }

	for ( auto it = m_persistentTools.Begin(); it != m_persistentTools.End(); ++it )
	{
		if ( (*it)->OnViewportInput( view, key, action, data ) )
		{
			return true;
		}
	}

	// Box drag
	if ( m_selectionBoxDrag )
	{
		GetCursorPos( m_boxDragEnd.x, m_boxDragEnd.y );
		return true;
	}

	// Camera movement
	Bool cameraMoved( false );
	if ( m_cameraMode == RPCM_DefaultFlyby && !m_isContextMenuOpen )
	{
		if ( ( m_mouseButtonFlags == 2) && key == IK_MouseX )
		{
			if( RIM_IS_KEY_DOWN( IK_Alt ) )
			{
				m_cameraRotation.Roll -= data * m_cameraInputRotMul;
			}
			else
			{
				m_cameraRotation.Yaw -= data * m_cameraInputRotMul;
			}
			cameraMoved = true;
		}
		else if ( ( m_mouseButtonFlags == 2) && key == IK_MouseY )
		{
			m_cameraRotation.Pitch -= data * m_cameraInputRotMul;
			cameraMoved = true;
		}
		else if ( ( m_mouseButtonFlags == 1) && key == IK_MouseY )
		{
			Vector forward;
			EulerAngles angles( 0, 0, m_cameraRotation.Yaw );
			angles.ToAngleVectors( &forward, NULL, NULL );
			m_cameraPosition -= forward * data * m_cameraInputPosMul;
			cameraMoved = true;
		}
		else if ( ( m_mouseButtonFlags == 1) && key == IK_MouseX )
		{
			m_cameraRotation.Yaw -= data * m_cameraInputRotMul;
			cameraMoved = true;
		}
		else if ( (m_mouseButtonFlags == 3) && key == IK_MouseY )
		{
			Vector up( 0,0,1 );
			m_cameraPosition -= up * data * m_cameraInputPosMul;
			cameraMoved = true;
		}
		else if ( (m_mouseButtonFlags == 3) && key == IK_MouseX )
		{
			Vector side;
			EulerAngles angles( 0, 0, m_cameraRotation.Yaw );
			angles.ToAngleVectors( NULL, &side, NULL );
			m_cameraPosition += side * data * m_cameraInputPosMul;
			cameraMoved = true;
		}
		else if ( key == IK_MouseZ && ( m_mouseButtonFlags == 2 ) )
		{
			m_cameraSpeedMultiplier = Clamp<Float>( m_cameraSpeedMultiplier + data*0.125f, 0.125f, 20.f );
		}
		else if ( key == IK_MouseZ && ( m_mouseButtonFlags == 0 ) && RIM_IS_KEY_DOWN( IK_Ctrl ) )
		{
			Float multiplier;
			if ( data > 0.f )
			{
				multiplier = 2.0f;
				++m_cameraInputPosMulLevel;
			}
			else if ( data < 0.f )
			{
				multiplier = 0.5f;
				--m_cameraInputPosMulLevel;
			}
			else
			{
				multiplier = 1.0f;
			}

			Int32 cachedLevel = m_cameraInputPosMulLevel;
			m_cameraInputPosMulLevel = Clamp<Int32>( m_cameraInputPosMulLevel, -5, 5 );
			Bool changeAllowed = m_cameraInputPosMulLevel == cachedLevel;

			if ( changeAllowed )
			{
				m_cameraInputPosMul = m_cameraInputPosMul * multiplier;
				m_cameraInputRotMul = m_cameraInputRotMul * multiplier;
			}


		}
	}
	else if ( m_cameraMode == RPCM_DefaultOrbiting )
	{
		if ( ( m_mouseButtonFlags&3) && key == IK_MouseX )
		{
			m_cameraRotation.Yaw -= data * m_cameraInputRotMul;
			cameraMoved = true;
		}
		if ( ( m_mouseButtonFlags&3) && key == IK_MouseY )
		{
			m_cameraRotation.Pitch -= data * m_cameraInputRotMul;
			cameraMoved = true;
		}
		if ( key == IK_MouseZ )
		{
			m_cameraZoom = Clamp( m_cameraZoom * ( 1.0f + data * 0.05f ), 0.1f, 10.0f );
			cameraMoved = true;
		}
	}

	// Restore cursor position only if camera is moved
	if ( cameraMoved )
	{
		RECT rect;
		::GetWindowRect( (HWND)GetHandle(), &rect );
		::SetCursorPos( ( rect.left + rect.right ) / 2, ( rect.top + rect.bottom ) / 2 );
		GetViewport()->SetCursorVisibility( false );
	}

	// Update camera related events
	if ( cameraMoved )
	{
		OnCameraMoved();
	}

	// Count mouse movement
	if ( key == IK_MouseX || key == IK_MouseY )
	{
		m_moveTillCapture += (Int32)Abs( data);
	}
	
	if( RIM_IS_KEY_DOWN( IK_LControl ) == false ) // Special functions
	{
		if (wxPanel::HasFocus() || action == IACT_Release) {
			// Movement keys
			if ( key == IK_W || key == IK_Up ) m_moveKeys[ RPMK_Forward ] = ( action == IACT_Press );
			if ( key == IK_S || key == IK_Down ) m_moveKeys[ RPMK_Back ] = ( action == IACT_Press );
			if ( key == IK_A || key == IK_Left ) m_moveKeys[ RPMK_StrafeLeft ] = ( action == IACT_Press );
			if ( key == IK_D || key == IK_Right ) m_moveKeys[ RPMK_StrafeRight ] = ( action == IACT_Press );
			if ( key == IK_Q ) m_moveKeys[ RPMK_Up ] = ( action == IACT_Press );
			if ( key == IK_E ) m_moveKeys[ RPMK_Down ] = ( action == IACT_Press );
			if ( key == IK_LShift || key == IK_RShift ) m_moveKeys[ RPMK_Sprint ] = ( action == IACT_Press );
			if ( key == IK_Delete ) SEvents::GetInstance().QueueEvent( CNAME( Delete ), nullptr );
		}

		/*// this is added to prevent focus lost after pressing arrows
		if ( key == IK_Up || key == IK_Down || key == IK_Left || key == IK_Right )
			SetFocusIgnoringChildren();*/
	}

	// Handled
	return true;
}

Bool CEdRenderingPanel::OnViewportClick( IViewport* view, Int32 button, Bool state, Int32 x, Int32 y )
{
	// be sure to gain focus
	SetFocusIgnoringChildren();

	Bool gameActive = GGame->IsActive();

	const Uint32 lastMouseButtons = m_mouseButtonFlags;

	if( !gameActive )
	{
		// If no button processed yet, pass to widget manager
		if ( !lastMouseButtons )
		{
			if ( m_widgetManager->OnViewportClick( view, button, state, x, y ) )
			{
				return true;
			}
		}

		// Let the edit mode handle event
		if ( m_tool && m_tool->OnViewportClick( view, button, state, x, y ) )
		{
			return true;
		}

		// Display camera info on status bar
		OnCameraMoved();

	}

	// Update mouse flags
	if ( state )
	{
		m_mouseButtonFlags |= 1 << button;
	}
	else
	{
		m_mouseButtonFlags &= ~(1 << button);
	}

	// Reset move counter
	if ( state && !lastMouseButtons )
	{
		m_moveTillCapture = 0;
	}

	if( !gameActive )
	{
		// Selection box drag
		const Bool ctrl = RIM_IS_KEY_DOWN( IK_LControl );
		if ( ctrl && lastMouseButtons == 0 && !m_selectionBoxDrag && m_mouseButtonFlags == 1 )
		{
			m_boxDragEnd = m_boxDragStart = wxPoint( x,y );
			m_selectionBoxDrag = true;
			view->SetMouseMode( MM_Clip );

			// Start drag timer delay
			const Float avgFPS = GEngine->GetLastTickRate();
			m_dragTimer.Start(m_dragDelay / avgFPS, true);

			// Give a chance to select stuff
			wxPoint cursorPoint;
			GetCursorPos( cursorPoint.x, cursorPoint.y );
			HandleActionClick( cursorPoint.x, cursorPoint.y );
			return true;
		}
		else if ( lastMouseButtons && m_mouseButtonFlags == 0 && m_selectionBoxDrag )
		{
			if ( m_boxDragStart.x != m_boxDragEnd.x && m_boxDragStart.y != m_boxDragEnd.y && m_isAfterDragDelay == true )
			{
				wxRect selectionRect;
				selectionRect.x = Min< Int32 >( m_boxDragStart.x, m_boxDragEnd.x ); 
				selectionRect.y = Min< Int32 >( m_boxDragStart.y, m_boxDragEnd.y ); 
				selectionRect.width = Abs< Int32 >( m_boxDragEnd.x - m_boxDragStart.x );
				selectionRect.height = Abs< Int32 >( m_boxDragEnd.y - m_boxDragStart.y );
				
				HandleSelectionRect( selectionRect );

				m_isAfterDragDelay = false;
			}

			m_selectionBoxDrag = false;
			view->SetMouseMode( MM_Normal, true );
			return true;
		}
	}

	// Capture control
	if ( state && m_mouseButtonFlags && !lastMouseButtons )
	{
		view->SetMouseMode( MM_Capture );
	}
	else if ( !state && !m_mouseButtonFlags && lastMouseButtons )
	{
		view->SetMouseMode( MM_Normal );

#ifndef NO_RED_GUI
		if(GRedGui::GetInstance().GetEnabled() == true)
		{
			GRedGui::GetInstance().GetInputManager()->ResetMouseCaptureControl();
		}
#endif	// NO_RED_GUI
	}

	if( !gameActive )
	{

		// Calculate new cursor position
		Int32 cursorPointX, cursorPointY;
		GetCursorPos( cursorPointX, cursorPointY );

		POINT cursorPoint;
		::GetCursorPos( &cursorPoint );
		::ScreenToClient( (HWND)GetHandle(), &cursorPoint );

		// Click
		if ( button == 0 && !state && lastMouseButtons==1 && m_moveTillCapture < 4 )
		{
			HandleActionClick( cursorPointX, cursorPointY );
		}

		// Context menu
		if ( m_isContextMenuOpen && !state && ( lastMouseButtons==1 || lastMouseButtons==2 ) )
		{
			m_isContextMenuOpen = false;
		}
		else if ( button == 1 && !state && lastMouseButtons==2 && m_moveTillCapture < 4 )
		{
			ResetCameraMoveKeys();
			HandleContextMenu( cursorPointX, cursorPointY );
			m_isContextMenuOpen = true;
		}
	}

	// Handled
	return true;
}

void CEdRenderingPanel::OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
{
    // Let the edit mode handle event
    if ( m_tool && m_tool->OnViewportGenerateFragments( view, frame ) )
    {
        return;
    }

	for ( auto it = m_persistentTools.Begin(); it != m_persistentTools.End(); ++it )
	{
		if ( (*it)->OnViewportGenerateFragments( view, frame ) )
		{
			return;
		}
	}

	// Override clear color
	CRenderFrameInfo& frameInfo = const_cast< CRenderFrameInfo& >( frame->GetFrameInfo() );
	frameInfo.m_clearColor = GetClearColor();

	// Generate fragments from widget manager
	m_widgetManager->OnViewportGenerateFragments( view, frame );

	// Debug pages
#ifndef NO_DEBUG_PAGES
	GGame->GenerateDebugPageFragmets( view, frame );
#endif

	// Selection box
	if ( m_selectionBoxDrag && m_boxDragEnd.x != m_boxDragStart.x && m_boxDragEnd.y != m_boxDragStart.y && m_isAfterDragDelay )
	{
		DebugVertex vertices[8];
		Float minX = Min<Float>( m_boxDragStart.x, m_boxDragEnd.x );
		Float minY = Min<Float>( m_boxDragStart.y, m_boxDragEnd.y );
		Float maxX = Max<Float>( m_boxDragStart.x, m_boxDragEnd.x );
		Float maxY = Max<Float>( m_boxDragStart.y, m_boxDragEnd.y );
		vertices[0] = DebugVertex( Vector( minX, minY, 0.001f ), Color::BLUE );
		vertices[1] = DebugVertex( Vector( maxX, minY, 0.001f ), Color::BLUE );
		vertices[2] = DebugVertex( Vector( maxX, minY, 0.001f ), Color::BLUE );
		vertices[3] = DebugVertex( Vector( maxX, maxY, 0.001f ), Color::BLUE );
		vertices[4] = DebugVertex( Vector( maxX, maxY, 0.001f ), Color::BLUE );
		vertices[5] = DebugVertex( Vector( minX, maxY, 0.001f ), Color::BLUE );
		vertices[6] = DebugVertex( Vector( minX, maxY, 0.001f ), Color::BLUE );
		vertices[7] = DebugVertex( Vector( minX, minY, 0.001f ), Color::BLUE );

		// Draw frame
		new ( frame ) CRenderFragmentOnScreenLineList( frame, vertices, 8 );
	}
}

void CEdRenderingPanel::OnViewportCalculateCamera( IViewport* view, CRenderCamera &camera )
{
    // Let the edit mode handle event
    if ( m_tool && m_tool->OnViewportCalculateCamera( view, camera ) )
    {
        return;
    }

	for ( auto it = m_persistentTools.Begin(); it != m_persistentTools.End(); ++it )
	{
		if ( (*it)->OnViewportCalculateCamera( view, camera ) )
		{
			return;
		}
	}

	// Use panel camera
	if ( m_cameraMode == RPCM_DefaultFlyby )
	{
		camera = CRenderCamera( m_cameraPosition, m_cameraRotation, m_cameraFov, view->GetWidth() / (Float)view->GetHeight(), RENDERINGPANEL_CAMERA_NEAR, RENDERINGPANEL_CAMERA_FAR, RENDERINGPANEL_CAMERA_ZOOM, false, m_lastFrameCamera );
	}
	else if ( m_cameraMode == RPCM_DefaultOrbiting )
	{
		Vector dir, u, r;
		m_cameraRotation.ToAngleVectors( &dir, &r, &u );
		camera = CRenderCamera( m_cameraPosition - dir * m_cameraZoom, m_cameraRotation, m_cameraFov, view->GetWidth() / (Float)view->GetHeight(), RENDERINGPANEL_CAMERA_NEAR, RENDERINGPANEL_CAMERA_FAR, RENDERINGPANEL_CAMERA_ZOOM, false, m_lastFrameCamera );
	}
}

Float CEdRenderingPanel::GetCameraSpeedMultiplier() const
{
	return m_moveKeys[ RPMK_Sprint ] ? m_cameraSpeedMultiplier*5.f : m_cameraSpeedMultiplier;
}

Int32 CEdRenderingPanel::GetCameraPosSensitivity() const
{
	return m_cameraInputPosMulLevel;
}

void CEdRenderingPanel::OnViewportTick( IViewport* view, Float timeDelta )
{
    // Let the edit mode handle event
    if ( m_tool && m_tool->OnViewportTick( view, timeDelta ) )
    {
        return;
    }

	for ( auto it = m_persistentTools.Begin(); it != m_persistentTools.End(); ++it )
	{
		if ( (*it)->OnViewportTick( view, timeDelta ) )
		{
			return;
		}
	}

	POINT cursorPoint;
	::GetCursorPos( &cursorPoint );
	::ScreenToClient( (HWND)GetHandle(), &cursorPoint );
	
	if (cursorPoint.x >= (LONG)view->GetWidth() || cursorPoint.y >= (LONG)view->GetHeight() || cursorPoint.x < 0 || cursorPoint.y < 0)
	{
		ResetCameraMoveKeys();
	}


	Float speed = wxTheFrame->GetEditorUserConfig().m_renderingPanelCameraSpeed * GetCameraSpeedMultiplier();

	// save 'lastFrame' camera
	const Float engineTime = GGame->GetEngineTime();
	m_lastFrameCamera.Init( engineTime, m_cameraPosition, m_cameraRotation, m_cameraFov, view->GetWidth() / (Float)view->GetHeight(), RENDERINGPANEL_CAMERA_NEAR, RENDERINGPANEL_CAMERA_FAR, RENDERINGPANEL_CAMERA_ZOOM );
	
	// calculate input direction vector
	Vector direction(
		m_moveKeys[ RPMK_StrafeRight ] - m_moveKeys[ RPMK_StrafeLeft ],
		m_moveKeys[ RPMK_Forward ] - m_moveKeys[ RPMK_Back],
		m_moveKeys[ RPMK_Up ] - m_moveKeys[ RPMK_Down ] );

	Vector cameraSpeed = direction * speed;

	// time delta for camera
	const Float cameraTimeDelta = GEngine->GetLastTimeDelta();

	if ( m_cameraMode == RPCM_DefaultFlyby )
	{
		// update current camera position
		Matrix rot = m_cameraRotation.ToMatrix();
		Vector x = rot.GetAxisX();
		Vector y = rot.GetAxisY();
		Vector z = Vector::EZ;
		Vector delta = ( x * cameraSpeed.X ) + ( y * cameraSpeed.Y ) + ( z * cameraSpeed.Z );
		m_cameraPosition += delta * cameraTimeDelta;
	}
	else if ( m_cameraMode == RPCM_DefaultOrbiting )
	{
		m_cameraRotation.Yaw += direction.X * cameraTimeDelta * 90.0f;
		m_cameraRotation.Pitch += direction.Y * cameraTimeDelta * 90.0f;
	}

	if ( !Vector::Near3( direction, Vector::ZEROS ) )
	{
		OnCameraMoved();
	}
}
void CEdRenderingPanel::TakeScreenshot( const String &destinationFile ) const
{
	IViewport* viewPort = GetViewport();

	TDynArray< Color > buffer;

	Uint32 width = viewPort->GetWidth();
	Uint32 height = viewPort->GetHeight();

	wxImage targetImage( width, height );
	Uint8 *charBuffer = targetImage.GetData();

	viewPort->GetPixels( 0, 0, width, height, buffer );
	for( Uint32 y = 0; y < height; y++ )
	{
		Uint8 *line = &charBuffer[ y * width * 3 ];
		Color *src = &buffer[ y * width ];

		for( Uint32 x = 0; x < width; x++, line+=3, src++ )
		{
			line[0] = src->R;
			line[1] = src->G;
			line[2] = src->B;
		}
	}

	wxBitmap bitmap( targetImage );

	bitmap.SaveFile( wxString( destinationFile.AsChar() ), wxBITMAP_TYPE_BMP );

}

void CEdRenderingPanel::HandleSelectionRect( wxRect rect )
{
    CWorld* world = GetWorld();
	if ( world )
	{
		// Generate hit proxies
		CHitProxyMap map;
		world->GenerateEditorHitProxies( map );

		// Render world with special frame but do not show it !
		CRenderFrameInfo info( GetViewport() );
		info.m_renderingMode = RM_HitProxies;
		info.m_present = false;
		CRenderFrame* frame = GRender->CreateFrame( NULL, info );
		if ( frame )
		{
			// Generate editor rendering fragments 
			world->GenerateEditorFragments( frame );
			
			// Generate scene fragments
			world->RenderWorld( frame );
			frame->Release();

			// Get clicked color :)
			TDynArray< Color > colors;
			colors.Reserve( rect.width * rect.height );
			if ( GetViewport()->GetPixels( rect.x, rect.y, rect.width, rect.height, colors ) )
			{
				// Generate hit proxy set
				//THashSet< CHitProxyID > idMap;
				TDynArray< CHitProxyID > idMap;
				for ( Int32 y=0; y<rect.height; y++ )
				{
					for ( Int32 x=0; x<rect.width; x++ )
					{
						Color color = colors[ x + y*rect.width ];
						color.A = 0;
						CHitProxyID id( color );
						idMap.PushBackUnique( id );
					}
				}

				// Remove border objects
				if ( rect.height >= 2 && rect.width >= 2 )
				{
					for ( Int32 y=0; y<rect.height; y++ )
					{
						for ( Int32 x=0; x<rect.width; x++ )
						{
							if ( y == 0 || x == 0 || x == rect.width-1 || y == rect.height-1 )
							{
								CHitProxyID id( colors[ x + y*rect.width ] );
								//idMap.Erase( id );
								idMap.Remove( id );
							}
						}
					}
				}

				// Generate list of selected hit proxy objects
				TDynArray< CHitProxyObject* > objects;
//				for ( THashSet< CHitProxyID >::const_iterator i = idMap.Begin(); i != idMap.End(); ++i )
				for ( TDynArray< CHitProxyID >::const_iterator i = idMap.Begin(); i != idMap.End(); ++i )
				{
					CHitProxyObject* object = map.FindHitProxy( *i );
					if ( object )
					{
						objects.PushBack( object );
					}
				}

				// Handle object selection
				HandleSelection( objects );
			}
		}
	}
}

CHitProxyObject* CEdRenderingPanel::GetHitProxyAtPoint( CHitProxyMap& hitProxyMap, Int32 x, Int32 y )
{
	// Validate coordinates
	if ( x < 0 || y < 0 || x >= GetClientSize().GetWidth() || y >= GetClientSize().GetHeight() )
	{
		// No way to click on something outside the window area
		return NULL;
	}

	// Ask the world
	CWorld* world = GetWorld();
	if ( world )
	{
		// Generate hit proxies
		world->GenerateEditorHitProxies( hitProxyMap );

		// Render world with special frame but do not show it !
		CRenderFrameInfo info( GetViewport() );
		info.m_renderingMode = RM_HitProxies;
		info.m_present = false;
		CRenderFrame* frame = world->GenerateFrame( GetViewport(), info );
		if ( frame )
		{
			// Generate scene fragments
			world->RenderWorld( frame );
			frame->Release();

			// Get clicked color :)
			TDynArray< Color > colors;
			if ( GetViewport()->GetPixels( x, y, 1, 1, colors ) )
			{
				LOG_EDITOR( TXT("HitProxy: %i,%i,%i"), colors[0].R, colors[0].G, colors[0].B );
				Color color = colors[0];
				// HACK: Colors written to buffer for HitProxies pass are writing 1.0 to alpha always, but
				// hit proxy IDs are generated so that alpha is 0 (unless overflow, but that's unlikely).
				// So, clear the alpha that we read back, so we can find the object...
				color.A = 0;
				CHitProxyID id( color );
				return hitProxyMap.FindHitProxy( id );
			}
		}
	}

	// Nothing selected
	return NULL;
}

void CEdRenderingPanel::HandleActionClick( Int32 x, Int32 y )
{
    if ( m_tool && m_tool->HandleActionClick( x, y ) )
    {
        return;
    }

	for ( auto it = m_persistentTools.Begin(); it != m_persistentTools.End(); ++it )
	{
		if ( (*it)->HandleActionClick( x, y ) )
		{
			return;
		}
	}

	// Get clicked object
	CHitProxyMap map;
	CHitProxyObject* object = GetHitProxyAtPoint( map, x, y );

	// Select object
	TDynArray< CHitProxyObject* > objects;
	if ( object )
	{
		objects.PushBack( object );
	}

	// Handle object selection
	HandleSelection( objects );
}

void CEdRenderingPanel::HandleSelection( const TDynArray< CHitProxyObject* >& object )
{
    // Ask edit mode to handle event
    if ( m_tool && m_tool->HandleSelection( object ) )
    {
        return;
    }

	for ( auto it = m_persistentTools.Begin(); it != m_persistentTools.End(); ++it )
	{
		if ( (*it)->HandleSelection( object ) )
		{
			return;
		}
	}
}

void CEdRenderingPanel::HandleContextMenu( Int32 x, Int32 y )
{
	if ( !GetWorld() )
		return;

	Vector clickedWorldPos, clickedWorldNormal;
	GetWorld()->ConvertScreenToWorldCoordinates( GetViewport(), x, y, clickedWorldPos, &clickedWorldNormal );

	if ( m_tool && m_tool->HandleContextMenu( x, y, clickedWorldPos ) )
	{
		return;
	}

	for ( auto it = m_persistentTools.Begin(); it != m_persistentTools.End(); ++it )
	{
		if ( (*it)->HandleContextMenu( x, y, clickedWorldPos ) )
		{
			return;
		}
	}
}

void CEdRenderingPanel::OnDelete()
{
	if ( m_tool && m_tool->OnDelete() ) 
	{
		return;
	}

	for ( auto it = m_persistentTools.Begin(); it != m_persistentTools.End(); ++it )
	{
		if ( (*it)->OnDelete() )
		{
			return;
		}
	}
}


static Box GetBoxForNode( const CNode* node )
{
	if ( node->IsA< CBoundedComponent >() )
	{
		const CBoundedComponent* bc = static_cast< const CBoundedComponent* >( node );
		return bc->GetBoundingBox();
	}

	if ( node->IsA< CEntity >() )
	{
		const CEntity* entity = static_cast< const CEntity * >( node );
		return entity->CalcBoundingBox();
	}

	return Box( node->GetWorldPosition(), 0.1f );
}

void CEdRenderingPanel::LookAtNode( CNode* node, const Float minZoom/*=1.0f*/, const Vector& offset/*= Vector::ZEROS*/ )
{
	ASSERT( node );

	if ( GGame->GetActiveWorld() )
	{
		// Select
		{
			CSelectionManager::CSelectionTransaction transaction(*GetSelectionManager());
			GetSelectionManager()->DeselectAll();
			GetSelectionManager()->Select( node );
		}

		// Look at node
		const Vector bbSize = GetBoxForNode( node ).CalcSize();
		const Float zoom = Max< Float >( minZoom, bbSize.Mag3() * 4.0f );
		const Vector zoomOffset = m_cameraRotation.TransformVector( Vector( 0.0f, zoom, 0.0f ) );
		SetCameraPosition( node->GetWorldPosition() + offset - zoomOffset );
	}
}

void CEdRenderingPanel::LookAtSelectedNodes()
{
	if ( GGame->GetActiveWorld() )
	{
		// Get selected nodes
		TDynArray< CNode* > nodes;
		GGame->GetActiveWorld()->GetSelectionManager()->GetSelectedNodes( nodes );

		// Calculate bounding box
		Box bbox;
		bbox.Clear();
		for ( Uint32 i=0; i<nodes.Size(); ++i )
		{
			const Box nodeBox = GetBoxForNode( nodes[i] );
			bbox.AddBox( nodeBox );
		}

		// Look at nodes
		Vector bbSize = bbox.CalcSize();
		Vector offset = m_cameraRotation.TransformVector( Vector( 0.0f, bbSize.Mag3(), 0.0f ) );
		SetCameraPosition( bbox.CalcCenter() - offset );
	}
}

void CEdRenderingPanel::MoveToTerrainLevel()
{
	CClipMap* clipmap = GGame->GetActiveWorld() ? GGame->GetActiveWorld()->GetTerrain() : NULL;
	if ( !clipmap )
	{
		wxMessageBox( wxT("There is no terrain available"), wxT("No terrain"), wxICON_ERROR|wxOK|wxCENTRE, this );
		return;
	}

	Vector newPosition = GetCameraPosition();
	clipmap->GetHeightForWorldPositionSync( newPosition, 0, newPosition.Z );
	newPosition += 2.0f;

	SetCameraPosition( newPosition );
	OnCameraMoved();
}

void CEdRenderingPanel::OnEraseBackground( wxEraseEvent &event )
{
	// Do not erase background
}

void CEdRenderingPanel::OnPaint(wxPaintEvent& event)
{
	// Do not erase background
	ValidateRect( (HWND)GetHWND(), NULL );
}

void CEdRenderingPanel::OnCameraMoved()
{
	m_prevCameraPosition = m_cameraPosition;

	// Refresh mesh coloring if the current coloring scheme requires it
	if ( GEngine->GetMeshColoringRefreshOnCameraMove() )
	{
		CDrawableComponent::RenderingSelectionColorChangedInEditor();
	}
}

void CEdRenderingPanel::EnableWidgets( Bool flag )
{
	m_widgetManager->EnableWidgets( flag );
}

Bool CEdRenderingPanel::GetCursorPos( Int32& x, Int32& y )
{
	POINT pos;
	::GetCursorPos( &pos );
	if ( !::ScreenToClient( (HWND) GetHWND(), &pos ) )
	{
		return false;
	}

	x = pos.x;
	y = pos.y;

	// Handle letterboxed windows

	IViewport* viewport = GetViewport();
	if ( viewport && viewport->IsCachet() )
	{
		RECT winRect;
		::GetClientRect( (HWND) GetHWND(), &winRect );

		const Float windowWidth = winRect.right - winRect.left;
		const Float windowHeight = winRect.bottom - winRect.top;

		if ( windowWidth != viewport->GetWidth() || windowHeight != viewport->GetHeight() )
		{
			x -= viewport->GetX();
			y -= viewport->GetY();
		}

		return 0 <= x && x < (Int32) viewport->GetWidth() && 0 <= y && y < (Int32) viewport->GetHeight();
	}

	return true;
}

void CEdRenderingPanel::OnTimerTimeout(wxTimerEvent& event)
{
	if( m_selectionBoxDrag == true )
	{
		m_isAfterDragDelay = true;
	}
}
