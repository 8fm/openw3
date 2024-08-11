/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../../common/engine/behaviorGraphInstance.h"

#include "behaviorEditorPanel.h"
#include "behaviorDebugVisualizer.h"

class CEdBehaviorGraphPoseTracer : public CEdBehaviorEditorSimplePanel
{
	DECLARE_EVENT_TABLE()

	wxToolBar*					m_toolbar;
	wxTextCtrlEx*				m_nodeName;
	wxListCtrl*					m_trackList;
	wxListCtrl*					m_boneList;
	wxListCtrl*					m_eventList;
	wxTextCtrl*					m_motionExText;

	static const Uint32			LIST_TRACK_COL = 0;
	static const Uint32			LIST_VALUE_COL = 1;
	static const Uint32			LIST_BONE_COL = 0;
	static const Uint32			LIST_BONE_POS_LS_COL = 1;
	static const Uint32			LIST_BONE_ROT_LS_COL = 2;
	static const Uint32			LIST_BONE_POS_MS_COL = 3;
	static const Uint32			LIST_BONE_ROT_MS_COL = 4;
	static const Uint32			LIST_BONE_POS_WS_COL = 5;
	static const Uint32			LIST_BONE_ROT_WS_COL = 6;
	static const Uint32			LIST_EVENT_NAME_COL = 0;

	Bool						m_connected;

	CBehaviorDebugVisualizer*	m_visualPose;

public:
	CEdBehaviorGraphPoseTracer( CEdBehaviorEditor* editor );

	virtual wxString	GetPanelName() const	{ return wxT("PoseTracer"); }
	virtual wxString	GetPanelCaption() const { return wxT("Pose Tracer"); }
	virtual wxString	GetInfo() const			{ return wxT("Pose Tracer"); }
	wxAuiPaneInfo		GetPaneInfo() const;

	virtual void OnReset();
	virtual void OnNodesSelect( const TDynArray< CGraphBlock* >& nodes );
	virtual void OnNodesDeselect();
	virtual void OnTick( Float dt );

protected:
	void OnConnect( wxCommandEvent& event );
	void OnSelectBones( wxCommandEvent& event );
	void OnSelectHelpers( wxCommandEvent& event );
	void OnShowNames( wxCommandEvent& event );
	void OnShowAxis( wxCommandEvent& event );

protected:
	void SelectNode( CBehaviorGraphNode* node );
	void CreateVisualPose();
	void DestroyVisualPose();
	void SetupTrackList();
	void FillTrackList( CBehaviorGraphNode* node );
	void SetupBoneList();
	void FillBoneList( CBehaviorGraphNode* node );
	void UpdateTrackList();
	void UpdateBoneList();
	void SetupEventList();
	void FillEventList( CBehaviorGraphNode* node );
	void UpdateEventList();
	void UpdateMotionExText();
};

//////////////////////////////////////////////////////////////////////////

class CEdBehaviorGraphMotionAnalyzer : public CEdBehaviorEditorSimplePanel
{
	DECLARE_EVENT_TABLE()

	wxToolBar*					m_toolbar;
	
	static const Uint32			CELL_NUM = 6;

	wxTextCtrl*					m_anim[ CELL_NUM ];
	wxTextCtrl*					m_animDelta[ CELL_NUM ];
	wxTextCtrl*					m_entity[ CELL_NUM ];
	wxTextCtrl*					m_diff[ CELL_NUM ];

	wxColor						m_defaultColor;

	Vector						m_animPos;
	EulerAngles					m_animRot;

	Vector						m_lastAnimPos;
	EulerAngles					m_lastAnimRot;

	Vector						m_animDeltaPos;
	EulerAngles					m_animDeltaRot;

	Vector						m_entityPos;
	EulerAngles					m_entityRot;

	Vector						m_lastEntityPos;
	EulerAngles					m_lastEntityRot;

	Vector						m_diffPos;
	EulerAngles					m_diffRot;

	Bool						m_diffFlag;

	Bool						m_connected;

public:
	CEdBehaviorGraphMotionAnalyzer( CEdBehaviorEditor* editor );

	virtual wxString	GetPanelName() const	{ return wxT("MotionAnalyzer"); }
	virtual wxString	GetPanelCaption() const { return wxT("Motion Analyzer"); }
	virtual wxString	GetInfo() const			{ return wxT("Motion Analzyer"); }
	wxAuiPaneInfo		GetPaneInfo() const;

	virtual void OnReset();
	virtual void OnTick( Float dt );
	virtual void OnDebuggerPostSamplePose( const SBehaviorGraphOutput& pose );

protected:
	void OnConnect( wxCommandEvent& event );

protected:
	void UpdateTextEdits();
	void UpdateTextEdit( const Vector& pos, const EulerAngles& rot, wxTextCtrl** edits, const wxColour* color = NULL );
};

//////////////////////////////////////////////////////////////////////////

class CEdBehaviorGraphProfiler : public CEdBehaviorEditorSimplePanel, public IBehaviorGraphInstanceEditorListener
{
	DECLARE_EVENT_TABLE()

	struct HistoryFilter
	{
		enum { E_Size = 30 };

		Float	m_history[ E_Size ];
		Uint32	m_curr;

		void Reset()
		{
			m_curr = 0;
			Red::System::MemoryZero( m_history, sizeof( m_history ) );
		}

		Float Update( Float newVal )
		{
			const Uint32 size = ARRAY_COUNT( m_history );

			m_curr = ( m_curr + 1 ) % size;
			ASSERT( m_curr >= 0 && m_curr < size );

			m_history[ m_curr ] = newVal;

			Float ret = 0.f;
			for ( Uint32 i=0; i<size; ++i )
			{
				ret += m_history[ i ];
			}
			return ret / (Float)size;
		}
	};

	wxToolBar*					m_toolbar;

	wxTextCtrl*					m_updateTimeEdit[2];
	wxTextCtrl*					m_sampleTimeEdit[2];
	wxTextCtrl*					m_allTimeEdit[2];

	Bool						m_connected;

	Float						m_updateTime;
	Float						m_sampleTime;

	HistoryFilter				m_filteredUpdateTime;
	HistoryFilter				m_filteredSampleTime;

	CTimeCounter				m_updateTimer;
	CTimeCounter				m_sampleTimer;

public:
	CEdBehaviorGraphProfiler( CEdBehaviorEditor* editor );

	virtual wxString	GetPanelName() const	{ return wxT("Profiler"); }
	virtual wxString	GetPanelCaption() const { return wxT("Profiler"); }
	virtual wxString	GetInfo() const			{ return wxT("Profiler"); }
	wxAuiPaneInfo		GetPaneInfo() const;

	virtual void OnTick( Float dt );
	virtual void OnReset();

public:
	virtual void OnPreUpdateInstance( Float& dt ) override;
	virtual void OnPostUpdateInstance( Float dt ) override;

	virtual void OnPreSampleInstance();
	virtual void OnPostSampleInstance( const SBehaviorGraphOutput& pose );

protected:
	void OnConnect( wxCommandEvent& event );

	void ConnectToInstance();
	void DisconnectFromInstance();
};
