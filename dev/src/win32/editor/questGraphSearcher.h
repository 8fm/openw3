/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once


class CEdQuestGraphEditor;

class CQuestGraphSearcher : public wxDialog
{
	DECLARE_EVENT_TABLE();

private:
	class IResult : public wxClientData
	{
	public:
		virtual ~IResult() {}

		virtual void Execute( CQuestGraphSearcher& parent ) = 0;
		virtual CObject* GetObject() = 0;
	};

	class SQuestBlockData : public IResult 
	{
	private:
		CQuestGraphBlock* block;

	public:
		SQuestBlockData( CQuestGraphBlock* _block ) : block( _block ) {}

		void Execute( CQuestGraphSearcher& parent );
		CObject* GetObject();
	};
	friend class SQuestBlockData;

	class SSpawnsetData : public IResult
	{
	private:
		CCommunity* spawnset;

	public:
		SSpawnsetData( CCommunity* _spawnset ) : spawnset( _spawnset ) {}

		void Execute( CQuestGraphSearcher& parent );
		CObject* GetObject();
	};
	friend class SSpawnsetData;

	typedef void (CQuestGraphSearcher::*SearchMethod)( CQuestGraph& /* inGraph */, TDynArray< CQuestGraphBlock* >& /* outResults */) const;

private:
	CEdQuestGraphEditor&				m_parent;

	THashMap< String, CClass* >			m_blockClasses;
	THashMap< String, CClass* >			m_conditionClasses;

	// ------------------------------------------------------------------------
	// Search category related members
	// ------------------------------------------------------------------------
	// searching by type
	wxChoice*							m_blockTypeChoice;
	wxButton*							m_searchByTypeBtn;

	// searching by name
	wxTextCtrl*							m_blockName;
	wxButton*							m_searchByNameBtn;

	// searching by script function
	wxChoice*							m_functionName;
	wxButton*							m_searchByFunctionBtn;

	// searching by a fact
	wxTextCtrl*							m_factIdText;
	wxButton*							m_searchByFactBtn;

	// searching by an administered reward
	wxChoice*							m_rewardNameChoice;
	wxButton*							m_searchByRewardBtn;

	// searching by condition
	wxChoice*							m_conditionTypeChoice;
	wxButton*							m_searchByConditionBtn;

	// searching by GUID
	wxTextCtrl*							m_guidVal;
	wxButton*							m_searchByGUIDBtn;

	// searching for spawnsets
	wxTextCtrl*							m_storyPhaseName;
	wxButton*							m_searchForSpawnsetBtn;

	// searching for outdated blocks
	wxButton*							m_searchForOutdatedBtn;

	// searching for invalid blocks
	wxButton*							m_searchForInvalidBtn;

	// searching for zero guid blocks
	wxButton*							m_searchForZeroGUIDBtn;

	// importing search results from XML
	mutable TDynArray< CGUID >			m_importedBlocksGUIDs;
	wxButton*							m_importFromXML;

	// ------------------------------------------------------------------------
	// Results display related members
	// ------------------------------------------------------------------------
	// results display
	wxListBox*							m_results;

	// ------------------------------------------------------------------------
	// Results tools related members
	// ------------------------------------------------------------------------
	// exporting search results to XML
	wxButton*							m_exportToXML;

	// updating GUIDS of the spawnsets
	wxButton*							m_findCutscenesBtn;

public:
	CQuestGraphSearcher( CEdQuestGraphEditor& parent );

protected:
	// ------------------------------------------------------------------------
	// window events handlers
	// ------------------------------------------------------------------------
	void OnSearchByType( wxCommandEvent& event );
	void OnSearchByName( wxCommandEvent& event );
	void OnSearchByFunction( wxCommandEvent& event );
	void OnSearchByFact( wxCommandEvent& event );
	void OnSearchByReward( wxCommandEvent& event );
	void OnSearchByCondition( wxCommandEvent& event );
	void OnSearchByGUID( wxCommandEvent& event );
	void OnSearchForSpawnset( wxCommandEvent& event );
	void OnSearchForOutdated( wxCommandEvent& event );
	void OnSearchForInvalid( wxCommandEvent& event );
	void OnSearchForZeroGUID( wxCommandEvent& event );
	void ImportFromXML( wxCommandEvent& event );
	void ExportToXML( wxCommandEvent& event );
	void FindCutscenes( wxCommandEvent& event );
	void OnResultDblClicked( wxCommandEvent& event );

	void OnPageChanged( wxBookCtrlEvent & event );

#if 0 
	void mcinekTEMPSHITreplacer( wxCommandEvent& event );
	void mcinekTEMPSHITsearch( CQuestGraph& graph, TDynArray< CQuestGraphBlock* >& foundBlocks ) const;

#endif

private:
	//! Sets a new blocks list
	void SetBlocksList( TDynArray< CQuestGraphBlock* >& blocks );

	void GraphBFS( CQuestGraph& rootGraph, SearchMethod method );

	// ---------------------------------------------------------------------------
	// Search methods
	// ---------------------------------------------------------------------------
	void ValidateSingleGraph( CQuestGraph& graph, TDynArray< CQuestGraphBlock* >& outdatedBlocks ) const;
	void FindByName( CQuestGraph& graph, TDynArray< CQuestGraphBlock* >& foundBlocks ) const;
	void FindByType( CQuestGraph& graph, TDynArray< CQuestGraphBlock* >& foundBlocks ) const;
	void FindByFunction( CQuestGraph& graph, TDynArray< CQuestGraphBlock* >& foundBlocks ) const;
	void FindByFact( CQuestGraph& graph, TDynArray< CQuestGraphBlock* >& foundBlocks ) const;
	void FindByReward( CQuestGraph& graph, TDynArray< CQuestGraphBlock* >& foundBlocks ) const;
	void FindByGUID( CQuestGraph& graph, TDynArray< CQuestGraphBlock* >& foundBlocks ) const;
	void FindByCondition( CQuestGraph& graph, TDynArray< CQuestGraphBlock* >& foundBlocks ) const;
	void CutscenesFinder( CQuestGraph& graph, TDynArray< CQuestGraphBlock* >& blocksWithCutscenes ) const;

	void SearchForDefinitionsFromXML( CQuestGraph& graph, TDynArray< CQuestGraphBlock* >& foundBlocks ) const;
	void FindByInvalidJournalPath( CQuestGraph& graph, TDynArray< CQuestGraphBlock* >& foundBlocks ) const;
	void FindByZeroGUID( CQuestGraph& graph, TDynArray< CQuestGraphBlock* >& foundBlocks ) const;

	Bool HasCutscenes( CStoryScene& scene ) const;
	Bool MatchBlockName( const String& blockName ) const;
	Bool MatchBlockType( CQuestGraphBlock* questBlock ) const;
	Bool MatchConditionType( IQuestCondition* condition ) const;

	// ---------------------------------------------------------------------------
	// Tools for data types
	// ---------------------------------------------------------------------------
	void OpenQuestBlockInEditor( CQuestGraphBlock* block );
	void EditSpawnset( CCommunity* spawnset );
};
