#pragma once 

/// World edit mode
class IEditorTool : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IEditorTool, CObject );

public:
	// Edit mode control
	virtual String GetCaption() const=0;
    virtual void GetDefaultAccelerator( Int32 &flags, Int32 &key ) const;
	virtual Bool Start( CEdRenderingPanel* viewport, CWorld* world, wxSizer* m_panelSizer, wxPanel* panel, const TDynArray< CComponent* >& selection ) { return false; }
	virtual Bool StartPersistent( CEdRenderingPanel* viewport ) { return false; }
	virtual void End()=0;

	virtual String GetFriendlyName() const { return GetCaption(); }

	// High level events
	virtual Bool HandleSelection( const TDynArray< CHitProxyObject* >& objects ) { return false; }
	virtual Bool HandleContextMenu( Int32 x, Int32 y, const Vector& collision );
	virtual Bool HandleActionClick( Int32 x, Int32 y ) { return false; }

	// Editor events, disallowed by default
	virtual Bool OnCopy() { return true; }
	virtual Bool OnCut() { return true; }
	virtual Bool OnPaste() { return true; }
	virtual Bool OnDelete() { return true; }

	virtual Bool UsableInActiveWorldOnly() const { return true; }
	virtual Bool IsPersistent() const { return false; }

	// Low level viewport events
	virtual Bool OnViewportTrack( const CMousePacket& packet ) { return false; }
	virtual Bool OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data ) { return false; }
	virtual Bool OnViewportClick( IViewport* view, Int32 button, Bool state, Int32 x, Int32 y ) { return false; }
	virtual Bool OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame ) { return false; }
	virtual Bool OnViewportCalculateCamera( IViewport* view, CRenderCamera &camera ) { return false; }
	virtual Bool OnViewportTick( IViewport* view, Float timeDelta ) { return false; }
	virtual Bool OnViewportMouseMove( const CMousePacket& packet ) { return false; }
	virtual Bool OnViewportKillFocus( IViewport* view ) { return false; }
	virtual Bool OnViewportSetFocus( IViewport* view ) { return false; }

	virtual TEdShortcutArray * GetAccelerators()        { return NULL; }
	virtual Bool OnAccelerator( wxCommandEvent& event ) { return false; }
};

DEFINE_SIMPLE_ABSTRACT_RTTI_CLASS( IEditorTool, CObject );

