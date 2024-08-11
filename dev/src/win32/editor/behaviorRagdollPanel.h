/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "behaviorEditorPanel.h"
#include "behaviorDebugVisualizer.h"

class CEdBehaviorGraphPoseMatcher : public CEdBehaviorEditorSimplePanel
{
	DECLARE_EVENT_TABLE()

	wxToolBar*					m_toolbar;
	wxTextCtrlEx*				m_nodeName;
	wxColourPickerCtrl*			m_colorPicker;
	wxTextCtrl*					m_valueEdit;
	wxTextCtrl*					m_timeEdit;
	wxSlider*					m_timeSlider;

	wxChoice*					m_choiceRootBone;
	wxChoice*					m_choiceFirstBone;
	wxChoice*					m_choiceSecondBone;

	wxTreeCtrl*					m_animTree;
	wxListCtrl*					m_poseList;

	Bool						m_connected;
	Bool						m_showSelected;
	Bool						m_showAll;
	Bool						m_showBest;

	static const Uint32			POSE_LIST_ANIM_COL = 0;
	static const Uint32			POSE_LIST_TIME_COL = 1;
	static const Uint32			POSE_LIST_VALUE_COL = 2;
#ifdef USE_HAVOK_ANIMATION
	hkaPoseMatchingUtility*		m_poseMatchUtils;
#endif
	struct SPoseItem
	{
		SPoseItem() : m_visualPose( NULL ), m_time( 0.f ), m_value( 0.f ), m_animEntry( NULL ) {}

		CBehaviorDebugVisualizer*	m_visualPose;
		Float						m_time;
		CName						m_animationName;
		Float						m_value;
		CSkeletalAnimationSetEntry* m_animEntry;
	};

	TDynArray< SPoseItem >		m_poses;

public:
	CEdBehaviorGraphPoseMatcher( CEdBehaviorEditor* editor );

	virtual wxString	GetPanelName() const	{ return wxT("PoseMatcher"); }
	virtual wxString	GetPanelCaption() const { return wxT("Pose Matcher"); }
	virtual wxString	GetInfo() const			{ return wxT("Pose Matcher"); }
	wxAuiPaneInfo		GetPaneInfo() const;

	virtual void OnTick( Float dt );
	virtual void OnReset();
	virtual void OnLoadEntity();
	virtual void OnUnloadEntity();
	virtual void OnPanelClose();

protected:
	void SetupPoseList();
	void FillAnimTree();
	void UpdateFullPoseList();
	void UpdatePoseListValues();

	void CreatePose( const CName& animation );
	void DeletePose( Uint32 index );
	void DeleteAllPoses();
	void UpdatePoses();
	void SetPoseTime( SPoseItem& pose, Float time );

	CBehaviorDebugVisualizer* CreateVisualPose();
	void DestroyVisualPose( CBehaviorDebugVisualizer* visualPose );

	Color RandColor() const;

	void SelectPose( Int32 num );

	void CreatePoseMatchingTool();
	void DeletePoseMatchingTool();

	void FillBoneChoices();
	Bool GetBones( Int32& root, Int32& first, Int32& second );

protected:
	void OnConnect( wxCommandEvent& event );
	void OnShowBest( wxCommandEvent& event );
	void OnShowSelected( wxCommandEvent& event );
	void OnShowAll( wxCommandEvent& event );
	void OnAdd( wxCommandEvent& event );
	void OnRemove( wxCommandEvent& event );
	void OnPoseListSelected( wxListEvent& event );
	void OnPoseColorChanged( wxColourPickerEvent& event );
	void OnPoseTimeEdit( wxCommandEvent& event );
	void OnPoseTimeSlider( wxScrollEvent& event );
};

//////////////////////////////////////////////////////////////////////////

