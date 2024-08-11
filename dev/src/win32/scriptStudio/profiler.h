/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

struct SProfileData
{
	wxString m_class;
	wxString m_function;

	Red::System::Uint32 m_numCalls;
	Red::System::Uint32 m_flags;

	Red::System::Int32 m_totalTimeIn;
	Red::System::Int32 m_totalTimeEx;

	Red::System::Float m_prcIn;
	Red::System::Float m_prcEx;

	Red::System::Uint64 m_recursion;

	Red::System::Uint32 m_numIndicies;
	Red::System::Int32* m_indicies;

	SProfileData() {}
	~SProfileData()
	{
		delete [] m_indicies;
		m_indicies = nullptr;
	}
};

/// Profiler panel
class CSSProfiler : public wxPanel
{
	wxDECLARE_CLASS( CSSProfiler );
	DECLARE_EVENT_TABLE();

private:
	struct StatFunction
	{
		wxString					m_class;
		wxString					m_name;
		int							m_numTicksIn;
		int							m_numTicksEx;
		int							m_numTicksPerCallIn;
		int							m_numTicksPerCallEx;
		float						m_prcIn;
		float						m_prcEx;
		int							m_recursionLevel;
		int							m_numCalls;
		unsigned int				m_flags;
		vector< StatFunction* >		m_called;

		StatFunction()
			: m_numCalls( 0 )
			, m_numTicksEx( 0 )
			, m_numTicksIn( 0 )
			, m_numTicksPerCallEx( 0 )
			, m_numTicksPerCallIn( 0 )
			, m_prcIn( 0 )
			, m_prcEx( 0 )
            , m_recursionLevel( 0 )
			, m_flags( 0 )
		{};
	};

	enum
	{
		STATS_COLUMN_NAME = 0,
		STATS_COLUMN_CLASS,
		STATS_COLUMN_CALLS,
		STATS_COLUMN_INTICKS,
		STATS_COLUMN_EXTICKS,
		STATS_COLUMN_PRCIN,
		STATS_COLUMN_PRCEX,
		STATS_COLUMN_INPERCALL,
		STATS_COLUMN_EXPERCALL,
		STATS_COLUMN_COMMENT
	};

	static bool NameComparator( StatFunction* func1, StatFunction* func2 );
	static bool ClassComparator( StatFunction* func1, StatFunction* func2 );
	static bool CallsComparator( StatFunction* func1, StatFunction* func2 );
	static bool InTicksComparator( StatFunction* func1, StatFunction* func2 );
	static bool ExTicksComparator( StatFunction* func1, StatFunction* func2 );
	static bool PrcInComparator( StatFunction* func1, StatFunction* func2 );
	static bool PrcExComparator( StatFunction* func1, StatFunction* func2 );
	static bool InPerCallComparator( StatFunction* func1, StatFunction* func2 );
	static bool ExPerCallComparator( StatFunction* func1, StatFunction* func2 );
	static bool CommentComparator( StatFunction* func1, StatFunction* func2 );

protected:
	wxListView*					m_functionList;
	wxListView*					m_callsList;
	wxListView*					m_callersList;
	vector< StatFunction* >		m_functions;

public:
	CSSProfiler( wxWindow* parent );
	~CSSProfiler();

	void SetData( SProfileData* data, Red::System::Uint32 size );

	//! Update function list
	void UpdateFunctionList();

	//! Update callers list
	void UpdateCallersList( StatFunction* func );

	//! Update calls list
	void UpdateCallsList( StatFunction* func );

	//! Sort functions
	void SortFunctions();

protected:
	void OnFunctionSelected( wxListEvent& event );
	void OnFunctionActivated( wxListEvent& event );
	void OnCallingFunctionActivated( wxListEvent& event );
	void OnCalledFunctionActivated( wxListEvent& event );
	void OnSortColumn( wxListEvent& event );

private:
	void SetupFunctionLine( wxListCtrl* ctrl, long index, StatFunction* func );
	StatFunction* GetSelectedFunction( wxListCtrl* ctrl );
	int FindFunctionIndex( StatFunction* func, wxListCtrl* ctrl );
};