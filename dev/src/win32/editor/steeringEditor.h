/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "steeringGraphEditor.h"
#include "../../common/game/moveSteeringBehavior.h"
#include "../../common/game/movementGoal.h"


class CMoveSteeringBehavior;
class CEdSteeringGraphEditor;
class CMovingAgentComponent;

///Steering graph visual editor
class CEdSteeringEditor :	public wxFrame, 
							public IEdEventListener, 
							public CEdSteeringGraphEditor::IHook,
							public ISteeringBehaviorListener
{
	DECLARE_EVENT_TABLE()

protected:
	CMoveSteeringBehavior*								m_template;	
	CEdPropertiesBrowserWithStatusbar*					m_properties;
	wxToolBar*											m_toolBar;

	wxBitmap											m_btIcon;
	wxBitmap											m_enableRuntimeTrackingBmp;
	wxBitmap											m_disableRuntimeTrackingBmp;

	CEdSteeringGraphEditor*								m_graphEditor;

	// debug data
	THandle< CMovingAgentComponent >					m_mac;
	THashMap< const IMoveSteeringNode*, Int32 >			m_activationTime;
	SMoveLocomotionGoal									m_activeGoal;
	TDynArray< TDynArray< String > >					m_frames;

	static TDynArray< CEdSteeringEditor * >				s_editors;

public:
	CEdSteeringEditor( wxWindow* parent, const THandle< CMovingAgentComponent >& mac );
	CEdSteeringEditor( wxWindow* parent, CMoveSteeringBehavior* steering );
	~CEdSteeringEditor();

	//! Show editor
	void ShowEditor();

	//! Get title
	virtual wxString GetShortTitle();

	//! Selection has changed
	virtual void OnGraphSelectionChanged();

	// ------------------------------------------------------------------------
	// ISteeringBehaviorListener implementation
	// ------------------------------------------------------------------------
	virtual void OnFrameStart( const SMoveLocomotionGoal& goal );
	virtual void OnNodeActivation( const IMoveSteeringNode& node );
	virtual Uint32 AddFrame( const Char* txt );
	virtual void AddText( Uint32 frameIdx, const Char* format, ... );

	// Returns an activation color for the specified node
	wxColour GetActivationColour( const IMoveSteeringNode& node ) const;

	//! Returns the currently set locomotion goal description
	RED_INLINE const SMoveLocomotionGoal& GetActiveGoal() const { return m_activeGoal; }

	//! Returns an active agent instance
	RED_INLINE const CMovingAgentComponent* GetActiveAgent() const { return m_mac.Get(); }

	//! Returns the debug frames
	RED_INLINE const TDynArray< TDynArray< String > >& GetDebugFrames() const { return m_frames; }

	//! Getter for edited resource
	CMoveSteeringBehavior* GetEditedSteeringBehavior() const { return m_template; }

protected:
	void OnPropertiesChanged( wxCommandEvent& event );
	void OnPropertiesRefresh( wxCommandEvent& event );
	void OnEditDelete( wxCommandEvent& event );
	void OnSave( wxCommandEvent& event );
	void OnZoomAll( wxCommandEvent& event );
	void OnRuntimeData( wxCommandEvent& event );

	void DispatchEditorEvent( const CName& name, IEdEventData* data );

private:
	void SaveOptionsToConfig();
	void LoadOptionsFromConfig();
	void Initialize();
};