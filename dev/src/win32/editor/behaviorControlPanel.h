/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "behaviorEditorPanel.h"
#include "skeletonPreview.h"
#include "animationPreviewGhost.h"

class CEdDisplayItemDialog;

class CEdBehaviorGraphControlPanel :  public CEdBehaviorEditorSimplePanel
									, public ISkeletonPreviewControl
{
	DECLARE_EVENT_TABLE()

	wxToolBar*	m_toolbar;
	wxSlider*	m_timeSlider;

	wxChoice*	m_lodLevel;
	wxChoice*	m_meshLodLevel;

	wxBitmap	m_playIcon;
	wxBitmap	m_pauseIcon;

	wxBitmap	m_mimicOnIcon;
	wxBitmap	m_mimicOffIcon;
	wxBitmap	m_mimicDisIcon;

	// item display dialog
	CEdDisplayItemDialog			*m_itemDialog;

public:
	CEdBehaviorGraphControlPanel( CEdBehaviorEditor* editor );

	static wxString		GetPanelNameStatic()	{ return wxT("Control"); }
	virtual wxString	GetPanelName() const	{ return wxT("Control"); }
	virtual wxString	GetPanelCaption() const { return wxT("Control panel"); }
	wxAuiPaneInfo		GetPaneInfo() const;

	virtual void OnReset();
	virtual void OnDebug( Bool flag );
	virtual void OnLoadEntity();

public:
	wxToolBar* GetSkeletonToolbar();

public:
	void OnPlayOneFrame( wxCommandEvent& event );
	void OnPlayPause( wxCommandEvent& event );
	void OnResetPlayback( wxCommandEvent& event );
	void OnToggleCameraPlayer( wxCommandEvent& event );
	void OnToggleCameraMoving( wxCommandEvent& event );
	void OnToggleExMotion( wxCommandEvent& event );
	void OnToggleExTrajectory( wxCommandEvent& event );
	void OnToggleMimic( wxCommandEvent& event );
	void OnToggleShowAlpha( wxCommandEvent& event );
	void OnAutoTracking( wxCommandEvent& event );
	void OnTimeScroll( wxScrollEvent& event );
	void OnToggleSkeleton( wxCommandEvent& event );
	void OnToggleSkeletonAxis( wxCommandEvent& event );
	void OnToggleSkeletonName( wxCommandEvent& event );
	void OnToggleFloor( wxCommandEvent& event );
	void OnToggleDynamicTarget( wxCommandEvent& event );
	void OnConnectInputs( wxCommandEvent& event );
	void OnDisplayItem( wxCommandEvent& event );
	void OnDisplayItemUndo( wxCommandEvent& event );
	void OnLodLevelChanged( wxCommandEvent& event );
	void OnMeshLodLevelChanged( wxCommandEvent& event );

	Float GetTimeFactor() const;
	void RefreshAllButtons();

protected:
	void UpdateCameraButtons();
	void UpdatePlayButtons();
	void UpdateMimicButtons();
};

//////////////////////////////////////////////////////////////////////////

class CEdBehaviorGraphStackPanel : public CEdBehaviorEditorSimplePanel
{
	DECLARE_EVENT_TABLE()

	wxCheckListBox*		m_list;
	wxListBox*			m_stack;

public:
	CEdBehaviorGraphStackPanel( CEdBehaviorEditor* editor );

	virtual wxString	GetPanelName() const	{ return wxT("Stack"); }
	virtual wxString	GetPanelCaption() const { return wxT("Instance stack"); }
	wxAuiPaneInfo		GetPaneInfo() const;

	virtual void OnReset();
	virtual void OnLoadEntity();
	virtual void OnUnloadEntity();
	virtual void OnDebug( Bool flag );
	virtual void OnInstanceReload();

protected:
	void OnInstUp( wxCommandEvent& event );
	void OnInstDown( wxCommandEvent& event );
	void OnCheckChanged( wxCommandEvent& event );

protected:
	void FillStackList();
	void FillStack();
	void UpdateStack();
};

//////////////////////////////////////////////////////////////////////////

class CEdBehaviorGraphLookAtPanel : public CEdBehaviorEditorSimplePanel
{
	DECLARE_EVENT_TABLE()

	Bool			m_autoTracking;
	Vector			m_prevTarget;

	struct AutoTracking
	{
		AutoTracking() : 
			m_horSpeed( 0.f ), 
			m_verSpeed( 0.f ), 
			m_radius( 2.f ), 
			m_hMin( 0.f ), 
			m_hMax( 2.5f ),
			m_horValue( 0.f ),
			m_verValue( 0.f ),
			m_verValueDeltaSign( 1.f )
		{}

		Float		m_horSpeed;
		Float		m_verSpeed;
		Float		m_radius;
		Float		m_hMin;
		Float		m_hMax;

		Float		m_horValue;
		Float		m_verValue;
		Float		m_verValueDeltaSign;

		void ResetTarget()
		{
			m_horValue = 0.f;
			m_verValue = 1.7f;
			m_verValueDeltaSign = 1.f;
		}
	};

	AutoTracking	m_autoTrackingParam;

	wxChoice*		m_varList;
	wxRadioButton*	m_lookAtState;

public:
	CEdBehaviorGraphLookAtPanel( CEdBehaviorEditor* editor );

	virtual wxString	GetPanelName() const	{ return wxT("LookAt"); }
	virtual wxString	GetPanelCaption() const { return wxT("Look At Panel"); }
	wxAuiPaneInfo		GetPaneInfo() const;

	virtual void OnTick( Float dt );
	virtual void OnLoadEntity();
	virtual void OnUnloadEntity();
	virtual void OnInstanceReload();

protected:
	void OnLookAtLevelChanged( wxCommandEvent& event );
	void OnLookAtWeightChanged( wxCommandEvent& event );
	void OnVarChanged( wxCommandEvent& event );
	void OnLookAtTypeChanged( wxCommandEvent& event );
	void OnLookAtTypeListChanged( wxCommandEvent& event );
	void OnAuto( wxCommandEvent& event );
	void OnAutoParamChanged( wxCommandEvent& event );
	void OnAutoParamSpeedChanged( wxScrollEvent& event );
	void OnAutoResetTarget( wxCommandEvent& event );

protected:
	void FillVarList();
	void FillLookAtTypes();
	void FillLookAtTypeList();
	CActor* GetActor() const;
	void CheckActorMimic();
	void UpdateActorLookAtLevel();
	void UpdateAutoTracking( Float dt );
	Vector GetTargetFromAutoTracking() const;
	Float GetValueFromEditBox( wxTextCtrl* edit ) const;
	Float GetValueFromSlider( wxSlider* slider ) const;
	void SetActorLookAt( CActor* actor, const Vector& target ) const;
	void ResetPrevTarget();
};

//////////////////////////////////////////////////////////////////////////

class CEdBehaviorSimInputCanvas : public CEdCanvas
{
	DECLARE_EVENT_TABLE();

	class AnimBg
	{
		Float m_time;

	public:
		AnimBg();

		void Update( Float dt );
		void Print( Int32 width, Int32 height, CEdBehaviorSimInputCanvas* canvas ) const;
	};

	Bool			m_running;

	Float			m_inputX;
	Float			m_inputY;

	AnimBg			m_bg;

public:
	CEdBehaviorSimInputCanvas( wxWindow* parent );

	virtual void PaintCanvas( Int32 width, Int32 height );
	void OnCanvasTick( Float dt );

	void SetRunning( Bool state );

	void SetInputX( Float var );
	void SetInputY( Float var );
};

class CEdBehaviorSimInputTool : public CEdBehaviorEditorSimplePanel
{
	DECLARE_EVENT_TABLE()

	Bool						m_connect;

	String						m_varX;
	String						m_varY;

	Float						m_varFloatX;
	Float						m_varFloatY;

	Float						m_varSpeed;
	Float						m_varRotation;

	CEdBehaviorSimInputCanvas*	m_canvas;

public:
	CEdBehaviorSimInputTool( CEdBehaviorEditor* editor );

	virtual wxString	GetPanelName() const	{ return wxT("SimInput"); }
	virtual wxString	GetPanelCaption() const { return wxT("Sim input"); }
	virtual wxString	GetInfo() const			{ return wxT("Sim input"); }
	wxAuiPaneInfo		GetPaneInfo() const;

	virtual void OnReset();
	virtual void OnInstanceReload();
	virtual void OnTick( Float dt );

protected:
	void OnConnect( wxCommandEvent& event );
	void OnVarXChanged( wxCommandEvent& event );
	void OnVarYChanged( wxCommandEvent& event );

protected:
	void Connect( Bool flag );
	void RefreshPanel();
	void FillVariables();
	void FillChoicesVars( wxChoice* choice );
	void UpdateAndSetVariables();
};
