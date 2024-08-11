
#pragma once

class CEdSceneEditor;

class CEdSceneWorldCtrl : public IViewportHook
{
	enum EMode
	{
		M_Preview,
		M_Game,
		M_Gameplay,
	};

	EMode				m_mode;
	CEdSceneEditor*		m_editor;
	CWorld*				m_previewWorld;
	IViewportHook*		m_prevHook;
	IViewportHook*		m_orgHook;

public:
	CEdSceneWorldCtrl();

	void Init( CEdSceneEditor* editor, CWorld* previewWorld );

	CWorld* GetWorld() const;

public:
	void ToggleMode();

	void SetPreviewMode();
	void SetGameMode();
	void SetGameplayMode();

	Bool IsPreviewMode() const;
	Bool IsGameMode() const;
	Bool IsGameplayMode() const;

	Bool CanUseGameMode() const;

public: // IViewportHook
	virtual void OnViewportCalculateCamera( IViewport* view, CRenderCamera &camera );
	virtual void OnViewportTick( IViewport* view, Float timeDelta );
	virtual void OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame );

	virtual Bool OnViewportTrack( const CMousePacket& packet );
	virtual Bool OnViewportMouseMove( const CMousePacket& packet );
	virtual Bool OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data );
	virtual Bool OnViewportClick( IViewport* view, Int32 button, Bool state, Int32 x, Int32 y );
	virtual CRenderFrame *OnViewportCreateFrame( IViewport *view );
	virtual void OnViewportRenderFrame( IViewport *view, CRenderFrame *frame );
	virtual void OnViewportSetDimensions ( IViewport* view );
	
	virtual void OnViewportKillFocus( IViewport* view )							{}
	virtual void OnViewportSetFocus( IViewport* view )							{}

private:
	CWorld* GetGameWorld() const;
};

//////////////////////////////////////////////////////////////////////////

class CEdSceneWorldProxyRelinker
{
	Bool							m_isLinked;

	IRenderProxy*					m_terrainProxy;
	TDynArray< IRenderProxy* >		m_renderProxies;

public:
	CEdSceneWorldProxyRelinker();
	~CEdSceneWorldProxyRelinker();

	void Destroy( CWorld* world );

	void Link( CWorld* world, const CStoryScene* scene );
	void Unlink( CWorld* world );

	Bool IsLinked() const;

	Bool ToggleLinkProcess( CWorld* world, const CStoryScene* scene );
};
