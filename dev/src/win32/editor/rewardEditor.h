/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "..\..\common\game\rewards.h"
#include "rewardsSummary.h"

enum ERewardTreeDataType
{
	RTDT_Directory,
	RTDT_Group,
	RTDT_Reward
};

class CRewardTreeData : public wxTreeItemData
{

public:

	CRewardTreeData( CDirectory* directory ) : m_dataType( RTDT_Directory ), m_directory( directory ) {}
	CRewardTreeData( CRewardGroup* group ) : m_dataType( RTDT_Group ), m_group( group ) {}
	CRewardTreeData( CName  reward ) : m_dataType( RTDT_Reward ), m_reward( reward ) {}
	
	const ERewardTreeDataType m_dataType;

	union
	{
		CDirectory*		m_directory;
		CRewardGroup*	m_group;
	};
	CName m_reward;
};

// A mock class that resources are parented to in order to prevent being garbage collected
class CRewardResourceManager : public CObject
{
	DECLARE_ENGINE_CLASS( CRewardResourceManager, CObject, 0 );

private:
	TDynArray< THandle< CRewardGroup > > m_rewards;

public:
	CRewardResourceManager() {};

	RED_INLINE void AddRewardGroup( CRewardGroup* group ) { m_rewards.PushBack( group ); }
	RED_INLINE void RemoveRewardGroup( CRewardGroup* group ) { m_rewards.RemoveFast( group ); }
	RED_INLINE TDynArray< THandle< CRewardGroup > >* GetRewardGroups() { return &m_rewards; }
};

BEGIN_CLASS_RTTI( CRewardResourceManager );
	PARENT_CLASS( CObject );
	PROPERTY( m_rewards );
END_CLASS_RTTI();


BEGIN_DECLARE_EVENT_TYPES()
	DECLARE_EVENT_TYPE( wxEVT_CHOOSE_REWARD_OK, wxEVT_USER_FIRST + 1 )
	DECLARE_EVENT_TYPE( wxEVT_CHOOSE_REWARD_CANCEL, wxEVT_USER_FIRST + 2 )
END_DECLARE_EVENT_TYPES()

class CEdRewardEditor : public wxFrame
{
	DECLARE_EVENT_TABLE();

public:
	CEdRewardEditor( wxWindow* parent, Bool chooseMode = false, CName currentReward = CName::NONE );
	~CEdRewardEditor();

	CName GetSelectedRewardName() const;

	static void			GetRewardNames( TDynArray<CName>& names );
	SReward *	GetRewardByName( CName  name );
	

protected:
	wxTreeCtrl*				m_rewardTree;
	wxStaticText*			m_nameLabel;
	CEdPropertiesPage*		m_propertiesBrowser;
	CRewardsSummary*		m_htmlSummary;
	CRewardResourceManager*	m_resourceManager;
	wxTreeItemId			m_draggedItem;

	void SaveOptionsToConfig();
	void LoadOptionsFromConfig();

	void SaveAll();
	void LoadAll( CName selectReward = CName::NONE );
	void LoadDirectory( CDirectory& dir, const wxTreeItemId& item, CName selectReward = CName::NONE );
	void LoadGroup( CRewardGroup& group, const wxTreeItemId& item, CName selectReward = CName::NONE );
	void LoadRewards( CRewardGroup& group, const wxTreeItemId& item, CName selectReward = CName::NONE );

	CRewardGroup* CreateResource( CDirectory* dir, CRewardGroup* parent );

	void RepopulateGroup( const wxTreeItemId& item, CRewardGroup* group, const CName& selectName );

	// Event handlers
	void OnPropertiesChanged( wxCommandEvent& event );
	void OnExit( wxCloseEvent& event );
	void OnOK( wxCommandEvent& event );
	void OnCancel( wxCommandEvent& event );
	void OnItemExpanding( wxTreeEvent& event );
	void OnItemCollapsed( wxTreeEvent& event );
	void OnItemRightClick( wxTreeEvent& event );
	void OnSelectionChanged( wxTreeEvent& event );
	void OnKeyDown( wxTreeEvent& event );
	
	void OnAddReward( wxCommandEvent& event );
	void OnDeleteReward( wxCommandEvent& event );
	void OnDuplicateReward( wxCommandEvent& event );
	void EditLabel(wxCommandEvent& event);
	void OnLabelEdit( wxTreeEvent& event );
	void OnFinishedLabelEdit( wxTreeEvent& event );

	void OnBeginDrag( wxTreeEvent& event );
	void OnEndDrag( wxTreeEvent& event );
	Bool CanItemBeDragged( const wxTreeItemId& item );

	void OnAddDirectory( wxCommandEvent& event );
	void OnDeleteDirectory( wxCommandEvent& event);

	void OnAddGroup( wxCommandEvent& event );
	void OnDeleteGroup( wxCommandEvent& event );

	void OnRevert( wxCommandEvent& event );
	void OnSubmit( wxCommandEvent& event );
	void OnHistory( wxCommandEvent& event );
	void OnAdd( wxCommandEvent& event );
	void OnRename( wxCommandEvent& event );
	void OnSave( wxCommandEvent& event );
	void OnCheckout( wxCommandEvent& event );
	void OnSync( wxCommandEvent& event );

};