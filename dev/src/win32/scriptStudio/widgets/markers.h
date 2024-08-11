/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#ifndef _SS_MARKERS_CTRL_H_
#define _SS_MARKERS_CTRL_H_

#include "checkedList.h"

#include "../solution/slnDeclarations.h"
#include "../solution/file.h"

enum EMarkerState;
class CMarkerToggledEvent;
class Solution;

class CSSMarkers : public CSSCheckListCtrl
{
	wxDECLARE_CLASS( CSSMarkers );
	wxDECLARE_EVENT_TABLE();

private:
	enum EColumn
	{
		Col_Enabled = 0,
		Col_File,
		Col_Line,

		Col_Max
	};

	struct SortData
	{
		EColumn column;
		bool* sortAsc;
	};

protected:
	struct Entry
	{
		SolutionFilePtr	m_file;
		unsigned int	m_lineNo;
		EMarkerState	m_state;

		inline bool operator==( const Entry& other )
		{
			int fileComp = m_file->m_solutionPath.compare( other.m_file->m_solutionPath );

			return fileComp == 0 && m_lineNo == other.m_lineNo;
		}
	};

protected:
	bool m_sortAsc[ Col_Max ];

	vector< Entry* > m_entries;
	Solution* m_solution;

public:
	CSSMarkers( wxWindow* parent, Solution* solution );
	virtual ~CSSMarkers();

	void Set( const SolutionFilePtr& file, unsigned int line, EMarkerState state );
	void Move( const SolutionFilePtr& file, unsigned int afterLine, int moveBy );

	void RestoreAll( const SolutionFilePtr& file );

	void Remove( const SolutionFilePtr& file, unsigned int line );
	void RemoveAll( const SolutionFilePtr& file );
	void RemoveAll();

	void LoadConfig( const wxConfigBase& config );
	void SaveConfig( wxConfigBase& config ) const;

protected:
	unsigned int Find( const SolutionFilePtr& file, unsigned int line );
	void Remove( unsigned int index );

	void OnItemActivated( wxListEvent& event );
	void OnColumnClick( wxListEvent& event );
	void OnKeyDown( wxListEvent& event );

	virtual void OnStateChange( int itemIndex, EChecked state ) override final;

	void SortByColumn( EColumn column );
	static int wxCALLBACK SortPredicate( wxIntPtr item1, wxIntPtr item2, wxIntPtr data );

private:
	virtual CMarkerToggledEvent* CreateToggleEvent( const SolutionFilePtr& file, Red::System::Int32 line, EMarkerState state ) const = 0;
};

#endif // _SS_MARKERS_CTRL_H_
