/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#ifndef EDITOR_H
#define EDITOR_H

#define EDMSG_RUN_PARALLEL_FINISHED	(WM_USER + 1)

class CEdRunnable
{
protected:
	Double m_triggerTime;

public:
	CEdRunnable() : m_triggerTime( 0 ) {}
	virtual ~CEdRunnable(){}

	RED_INLINE Double GetTriggerTime() const { return m_triggerTime; }
	RED_INLINE void TriggerAfter( Double seconds ) { m_triggerTime = Red::System::Clock::GetInstance().GetTimer().GetSeconds() + seconds; }

	virtual void Run()=0;

	virtual bool IsDuplicate( CEdRunnable* runnable )
	{
		return typeid(*this) == typeid(*runnable);
	}
};

class CEdApp : public wxApp
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Editor );
	DECLARE_EVENT_TABLE();

	struct SRunParallelInfo
	{
		CEdRunnable*	taskRunnable;
		CEdRunnable*	afterFinishRunnable;
		Bool			finished;
	};

protected:
	ULONG_PTR						m_gdiToken;
	TDynArray<CEdRunnable*>			m_runLaters;
	static Red::Threads::CMutex		m_runLatersMutex;
	TDynArray<SRunParallelInfo*>	m_parallelTasks;
	static Red::Threads::CMutex		m_parallelMutex;
	wxSingleInstanceChecker*		m_processMutex;

    void OnActivateApp( wxActivateEvent& event );

	virtual Bool ProcessIdle();

	static DWORD WINAPI RunParallelThreadProc( LPVOID lpParameter );

public:
	CEdApp() : wxApp() {}
	virtual ~CEdApp();

	void PreInit();
	virtual bool OnInit();
	virtual int OnRun();
	virtual int OnExit();
	void SetAlwaysActive( Bool active = true );
    
#ifndef NO_ASSERTS
	virtual void OnAssertFailure( const wxChar* file, int line, const wxChar* func, const wxChar* cond, const wxChar* msg );
#endif

public:
	bool ProcessWxMessages();

	void RunLater( CEdRunnable* runnable );
	void RunLaterOnce( CEdRunnable* runnable );
	void RunParallel( CEdRunnable* taskRunnable, CEdRunnable* afterFinishRunnable );
};

#endif // EDITOR_H

#define wxEdApp ( static_cast< CEdApp* >( wxTheApp ) )
