/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#pragma once

class CSSFindSymbolParsingThread;
class CSSFrame;
class CSSDocument;
struct SSClassStub;
struct SSFunctionStub;
struct SSEnumStub;
class Solution;

/// Find Symbol dialog
class CSSFindSymbolDialog : public wxDialog 
{
	DECLARE_EVENT_TABLE();

protected:
	enum ESymbolEntryType { SET_Class, SET_Function, SET_Enum, SET_ClassField, SET_ClassMethod };
	struct SSymbolEntry
	{
		// GUI data
		wxString m_symbolName;
		wxString m_symbolTypeName;
		wxString m_signatureName;
		wxString m_fileName;

		ESymbolEntryType m_symbolType;
		wstring m_index;
		unsigned int m_subIndex;

		bool operator < ( const SSymbolEntry& entry );
	};

	// GUI elements
	wxListCtrl*						m_listCtrlSymbols;
	wxTextCtrl*						m_txtFilter;
	wxButton*						m_btnOK;
	wxButton*						m_btnCancel;

	CSSFindSymbolParsingThread*		m_parsingThread;
	bool							m_wasEntriesDataUpdated;

	CSSFrame*						m_frame;			// parent frame
	int								m_selectedItemIndex;
	CSSDocument*					m_selectedDoc;
	bool							m_forceReparse;
	wxString						m_lastFilterText;	// for optimization
	wxTimer							m_findTimer;

	vector< SSymbolEntry >*			m_symbolEntriesFiltered;
	vector< SSymbolEntry >*			m_symbolEntriesFilteredBuffer;
	vector< SSymbolEntry >			m_symbolEntriesFilteredBuffer0;
	vector< SSymbolEntry >			m_symbolEntriesFilteredBuffer1;
	vector< SSymbolEntry >			m_symbolEntriesAll;

	int								m_numClasses;
	int								m_numFunctions;
	int								m_numEnums;

	Solution*						m_solution;

public:
	CSSFindSymbolDialog( wxWindow* parent, Solution* solution );
	~CSSFindSymbolDialog();
	void Init();
	void SelectCurrentInput();
	CSSDocument* GetSelectedDoc();
	void ForceReparse() { m_forceReparse = true; }

	// public for parsing threat
	bool GetAllSymbolEntries( bool isSortingEnabled = true ); // returns true if something was changed

protected:
	void OpenSolutionFile();
	void UpdateSymbolsListCtrl();
	void FilterTextUpdate( bool hasDataChanged );
	bool GotoSymbol();

	wxString GetFunctionSignature( const SSFunctionStub* functionStub );

	void OnOK( wxCommandEvent& event );
	void OnCancel( wxCommandEvent& event );
	void OnKeyDown( wxKeyEvent& event );
	void OnClose( wxCloseEvent& event );
	void OnItemSelected( wxListEvent& event );
	void OnItemActivated( wxListEvent& event );
	void OnFilterTextUpdate( wxCommandEvent& event );
	void OnFindTimer( wxTimerEvent& event );
};

//////////////////////////////////////////////////////////////////////////

class CSSFindSymbolParsingThread : public wxThread
{
public:
	CSSFindSymbolParsingThread( CSSFindSymbolDialog* findSymbolDialog );
	~CSSFindSymbolParsingThread();

	void RequestExit() { m_requestExit = true; }

protected:
	bool ShouldParse();
	void ParseSymbols();

	virtual void OnExit();
	virtual ExitCode Entry();

private:
	bool			m_requestExit;

	CSSFindSymbolDialog* m_findSymbolDialog;
};
