/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include <wx/dnd.h>
#include "../../common/engine/viewport.h"

/// Rendering panel movement keys
enum ERPMovementKeys
{
	RPMK_Forward,
	RPMK_Back,
	RPMK_StrafeLeft,
	RPMK_StrafeRight,
	RPMK_Up,
	RPMK_Down,
	RPMK_Sprint,
};

/// Rendering panel camera mode
enum ERPCameraMode
{
	RPCM_DefaultFlyby,
	RPCM_DefaultOrbiting
};

class wxMenuWithSeparators : public wxMenu 
{
protected:
	Bool	m_needsGroupSeparator;

public:
	wxMenuWithSeparators()
		: m_needsGroupSeparator( false )
	{};

	void BeginGroup()
	{
		if ( m_needsGroupSeparator )
		{
			AppendSeparator();
			m_needsGroupSeparator = false;
		}
	}

	virtual wxMenuItem* DoAppend(wxMenuItem *item)
	{
		m_needsGroupSeparator = true;
		return wxMenu::DoAppend( item );
	}
};

class CViewportWidgetManager;
class IEditorTool;
class CEdEditorToolContext;
class CEdUndoManager;
class CEdSceneExplorer;
class CHitProxyObject;

/// Basic rendering panel for editor
class CEdRenderingPanel : public wxPanel, public IViewportHook, public CDropTarget
{
	DECLARE_EVENT_TABLE();

protected: 
	wxBoxSizer*				m_sizer;					//!< Internal sizer
	CEdRenderingWindow*		m_renderingWindow;			//!< Rendering window
	CViewportWidgetManager*	m_widgetManager;			//!< Viewport widget manager
	Vector					m_cameraPosition;			//!< Camera position
	Vector					m_prevCameraPosition;		//!< Previous Camera position
	Float 					m_cameraSpeedMultiplier;	//!< Camera speed multiplier
	Float					m_cameraInputRotMul;		//!< Camera input multiplier, rotation
	Float					m_cameraInputPosMul;		//!< Camera input multiplier, position
	Int32					m_cameraInputPosMulLevel;	//!< Intuitive number representing input sensitivity
	EulerAngles				m_cameraRotation;			//!< Camera rotation
	Float					m_cameraZoom;				//!< Camera zoom ( used in orbiting mode )
	Float					m_cameraFov;				//!< Camera FOV
	Bool					m_moveKeys[7];				//!< Camera move keys
	Int32					m_moveTillCapture;			//!< Mouse movement from mouse capture
	Uint32					m_mouseButtonFlags;			//!< Flags for mouse buttons
	ERPCameraMode			m_cameraMode;				//!< Camera mode
	wxPoint					m_boxDragStart;				//!< First corner of selection box
	wxPoint					m_boxDragEnd;				//!< Second corner of selection box
	Bool					m_selectionBoxDrag;			//!< Dragging a selection box
    IEditorTool*            m_tool;						//!< Active editor tool
	TDynArray<IEditorTool*>	m_persistentTools;			//!< Persistent tools
	CEdUndoManager*			m_undoManager;				//!< Undo manager
	SRenderCameraLastFrameData	m_lastFrameCamera;
	
	IViewport::EAspectRatio m_viewportCachetAspectRatio; //!< aspect ratio for cachet calculation in viewport

    CEdEditorToolContext*	m_editContext;

	const Int32				m_dragDelay;			//!< Delay after which timer sends timeout event
	wxTimer					m_dragTimer;			//!< Timer for dragging start delay
	Bool					m_isAfterDragDelay;		//!< Indicates whether drag can be executed or not
	Bool					m_isContextMenuOpen;	//!< Flag used to ignore mouse input after context menu close
	
public:
	CEdRenderingPanel( wxWindow* parent );
	~CEdRenderingPanel();

	// Set current tool
	void SetTool( IEditorTool* tool ) { m_tool = tool; }

	//! Get current tool
	RED_INLINE IEditorTool* GetTool() { return m_tool; }

	// Getters
	RED_INLINE const Vector& GetCameraPosition() const { return m_cameraPosition; }
	RED_INLINE const EulerAngles& GetCameraRotation() const { return m_cameraRotation; }
	RED_INLINE Float GetCameraZoom() const { return m_cameraZoom; }
	RED_INLINE Float GetCameraFov() const { return m_cameraFov; }
	RED_INLINE ERPCameraMode GetCameraMode() const { return m_cameraMode; }
	RED_INLINE CEdRenderingWindow* GetRenderingWindow() const { return m_renderingWindow; }
	RED_INLINE IViewport* GetViewport() const { return m_renderingWindow->GetViewport().Get(); }
	RED_INLINE IViewport::EAspectRatio GetViewportCachetAspectRatio() const { return m_viewportCachetAspectRatio; }
	RED_INLINE void SetCameraPosition( const Vector& position ) { m_cameraPosition = position; }
	RED_INLINE void SetCameraRotation( const EulerAngles& rotation ) { m_cameraRotation = rotation; }
	RED_INLINE void SetCameraZoom( Float cameraZoom ) { m_cameraZoom = cameraZoom; }
	RED_INLINE void SetCameraMode( ERPCameraMode cameraMode ) { m_cameraMode = cameraMode; }
	RED_INLINE void SetCameraFov( Float fov ) { m_cameraFov = fov; }
	RED_INLINE void SetViewportCachetAspectRatio( const IViewport::EAspectRatio viewportCachetAspectRatio ) { m_viewportCachetAspectRatio = viewportCachetAspectRatio; }

	void LookAtNode( CNode* node, const Float minZoom=1.0f, const Vector& offset = Vector::ZEROS );
	void LookAtSelectedNodes();
	void MoveToTerrainLevel();

	// Key Down event callback. It's used to prevent focus lose due to arrow press.
	void CEdRenderingPanel::OnKeyDown( wxKeyEvent& event );

	// Get world we are viewing in this panel
	virtual CWorld* GetWorld() { return nullptr; }

	// Get selection manager
	virtual CSelectionManager* GetSelectionManager() { return nullptr; }

	// Get transform manager
	virtual CNodeTransformManager* GetTransformManager() { return nullptr; }

	// Gets scene explorer
	virtual CEdSceneExplorer* GetSceneExplorer() { return nullptr; }

	// Set Undo Manager for this viewport
	void SetUndoManager( CEdUndoManager* undoManager ) { m_undoManager = undoManager; }
	
	// Get Undo Manager for this viewport
	CEdUndoManager* GetUndoManager() { return m_undoManager; }

	// Get the clear color
	virtual Color GetClearColor() const;

	// Should we draw grid ?
	virtual Bool ShouldDrawGrid() const;

	//! Viewport hook interface
	virtual Bool OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data );
	virtual Bool OnViewportClick( IViewport* view, Int32 button, Bool state, Int32 x, Int32 y );
	virtual void OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame );
	virtual void OnViewportCalculateCamera( IViewport* view, CRenderCamera &camera );
	virtual void OnViewportTick( IViewport* view, Float timeDelta );
	virtual Bool OnViewportMouseMove( const CMousePacket& packet );
	virtual Bool OnViewportTrack( const CMousePacket& packet );
    virtual void OnViewportKillFocus( IViewport* view );
    virtual void OnViewportSetFocus( IViewport* view );
	virtual void OnDelete();

	// Get hit proxy object at point
	CHitProxyObject* GetHitProxyAtPoint( CHitProxyMap& hitProxyMap, Int32 x, Int32 y );

	// Take a screenshot of panel contents
	virtual void TakeScreenshot( const String &destinationFile ) const;

	// Enable widgets
	void EnableWidgets( Bool flag );

    CEdEditorToolContext* context() { return m_editContext; }

	void OnEraseBackground( wxEraseEvent &event );
	void OnPaint(wxPaintEvent& event);

	Bool GetCursorPos( Int32& x, Int32& y );

	// wxPanel can't be focused if it has focusable children. We want rendering panel to accept focus so it can move its camera.
	bool AcceptsFocus() const { return true; }

	// Drag Timer handler
	void OnTimerTimeout( wxTimerEvent& event );

protected:
	virtual void HandleActionClick( Int32 x, Int32 y );
	virtual void HandleSelection( const TDynArray< CHitProxyObject* >& object );
	virtual void HandleContextMenu( Int32 x, Int32 y );
	virtual void HandleSelectionRect( wxRect rect );
	virtual void OnCameraMoved();

	void InitPersistentTools();
	RED_INLINE void ResetCameraMoveKeys() { Red::System::MemorySet( &m_moveKeys[0], 0, sizeof(m_moveKeys) ); }

	Float GetCameraSpeedMultiplier() const;
	Int32 GetCameraPosSensitivity() const;
};
