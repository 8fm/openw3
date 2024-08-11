class CVertexEditorEntity;

enum RulerMode
{
    RM_Normal,
    RM_PickingPoint,
};

class CEdRulerTool : public IEditorTool
{
	DECLARE_ENGINE_CLASS( CEdRulerTool, IEditorTool, 0 );

private:

	CWorld *m_world;
	CEdRenderingPanel *m_viewport;
    CVertexEditorEntity *m_rulerStartPoint;
    CVertexEditorEntity *m_rulerEndPoint;
    CVertexEditorEntity *m_hoveredPoint;
    CFont *m_font;
    RulerMode m_mode;
    wxStopWatch m_stopWatch;

public:

	CEdRulerTool();
    ~CEdRulerTool();

	virtual String GetCaption() const { return TXT("Ruler"); }
	virtual Bool Start( CEdRenderingPanel* viewport, CWorld* world, wxSizer* m_panelSizer, wxPanel* panel, const TDynArray< CComponent* >& selection );
	virtual void End();	
	virtual Bool HandleSelection( const TDynArray< CHitProxyObject* >& objects );
	virtual Bool OnViewportClick( IViewport* view, Int32 button, Bool state, Int32 x, Int32 y );
	virtual Bool OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame );
	virtual Bool OnViewportTrack( const CMousePacket& packet );

private:

    Bool OnLeftButtonDown( IViewport* view, Int32 x, Int32 y );
    Bool OnLeftButtonUp( IViewport* view, Int32 x, Int32 y );
    CVertexEditorEntity *CreateRulerPointEntity( const Vector &position ) const;
    void DestroyRulerPointEntity( CVertexEditorEntity *rulerPoint );
    void GetClosestPointToSegment( const Vector &srcPoint, const Vector &a, const Vector &b, Vector &closestPoint ) const;
    String ConvertLengthToUnits( Float length ) const;
};

BEGIN_CLASS_RTTI( CEdRulerTool );
	PARENT_CLASS( IEditorTool );
END_CLASS_RTTI();
