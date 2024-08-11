
#pragma once

#include "../../common/game/storySceneDebugger.h"

class CEdSceneEditor;

class CEdSceneDebugger : public IStorySceneDebugger
{
	struct ElementContext
	{
		String					m_element;
		TDynArray< String >		m_eventsStarted;
		TDynArray< String >		m_eventsEnded;
		TDynArray< String >		m_eventsProcessed;
	};

	struct TickContext
	{
		Float						m_time;
		TDynArray< ElementContext > m_elems;
	};

	struct LatentItem
	{
		Float			m_time;
		Float			m_localTime;
		String			m_info;
		const void*		m_id;

		LatentItem() : m_time( 0.f ), m_localTime( 0.f ), m_id( NULL ) {}
		LatentItem( const String& i, Float time, Float localTime, const void* id ) : m_time( time ), m_localTime( localTime ), m_info( i ), m_id( id ) {}
	};

private:
	CEdSceneEditor*		m_mediator;

	Bool				m_isInitialized;
	Bool				m_isRunning;

	wxHtmlWindow*		m_mainPanel;
	wxHtmlWindow*		m_eventListPanel;
	wxHtmlWindow*		m_actorStatesPanel;
	wxHtmlWindow*		m_loadingPanel;
	wxHtmlWindow*		m_executorPanel;

	TDynArray< TickContext >	m_context;
	TDynArray< LatentItem >		m_latentInfos;

	Bool						m_sceneMainFlowChanged;
	TDynArray< String >			m_sceneMainFlow;

	Bool									m_loadingsChanged;
	TDynArray< TPair< Double, String > >	m_loadings;

	Bool						m_executorLogChanged;
	TDynArray< String >			m_executorLog;

	CName						m_selectedActor;
	Bool						m_showActorDetails;

public:
	CEdSceneDebugger();

	wxPanel* Setup( CEdSceneEditor* mediator, wxWindow* parent );

	void Init();
	void EnableDebugging( Bool flag );

public:
	virtual void PlayerLogicTickBegin( const CStoryScenePlayer* player ) override;
	virtual void PlayerLogicTickEnd( const CStoryScenePlayer* player ) override;

	virtual void BeginTickElement( const IStorySceneElementInstanceData* element, Float dt ) override;
	virtual void EndTickElement( const IStorySceneElementInstanceData* element, Float dt ) override;

	virtual void FireAllStartEvents( const IStorySceneElementInstanceData* element ) override;
	virtual void FireAllStopEvents( const IStorySceneElementInstanceData* element ) override;

	virtual void EventInit( const CStorySceneEvent* event ) override {}
	virtual void EventDeinit( const CStorySceneEvent* event ) override {}

	virtual void EventStart( const CStorySceneEvent* event ) override;
	virtual void EventProcess( const CStorySceneEvent* event, Float eventTime, Float progress, Float timeDelta ) override;
	virtual void EventEnd( const CStorySceneEvent* event ) override;

	virtual void OnCreatedSceneElementInstanceData( const CStorySceneElement* element, const IStorySceneElementInstanceData* instanceData ) override {}

	virtual void SignalAcceptChoice(  const SSceneChoice& choosenLine ) override {}
	virtual void SignalSkipLine() override {}
	virtual void SignalForceSkipSection() override {}

	virtual void OnChangeSection( const CStorySceneSection* prev, const CStorySceneSection* next ) override;
	virtual void OnFinishChangingSection( const CStorySceneSection* prev, const CStorySceneSection* next ) override;

	virtual void OnStartFromInput( const CStorySceneInput* input ) override;
	virtual void OnFinishedAtOutput( const CStorySceneOutput* output ) override;

	virtual void OnExecutor_ProcessPlacements( const CEntity* entity, const Matrix& placementWS ) override;
	virtual void OnExecutor_ChangeIdle( const CEntity* entity, const SStorySceneActorAnimationState& newState ) override;

protected:
	Bool IsThisPanelVisible() const;

	void RemoveItemsFromLatentInfoListByTime( Float time );
	void RemoveDuplicatesFromLatentInfoList();

	void RefreshMainPage();
	void RefreshLatentListPage();
	void RefreshActorStatesPage();
	void RefreshLoadingPage();
	void RefreshExecutorPage();

	void WriteActorState( const CName& actorId, const CEntity* e, wxString& code );

protected:
	void OnDebuggerLinkClicked( wxHtmlLinkEvent& event );
};

//////////////////////////////////////////////////////////////////////////

class CEdDialogFCurveEditor : public CEdCurveEditorCanvas
{
	DECLARE_EVENT_TABLE();

	TDynArray< SCurveData > m_plotData;

public:
	CEdDialogFCurveEditor( wxWindow *parentWin );

public:
	void SetDataToPlot( const TDynArray< String >& tracks, const TDynArray< TDynArray< TPair< Vector2, Vector > > >& data );
};

//////////////////////////////////////////////////////////////////////////
