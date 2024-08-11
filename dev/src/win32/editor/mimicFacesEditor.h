/*
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

/*#include "animBrowserPreview.h"

class CEdMimicFacesEditor	: public wxSmartLayoutFrame
							, public ISkeletonPreviewControl
							, IAnimBrowserPreviewListener
{
	DECLARE_EVENT_TABLE();

public:

	CEdMimicFacesEditor( wxWindow* parent, CResource* mimicFaces );
	virtual ~CEdMimicFacesEditor();

	CMimicFaces*				m_mimicFaces;

	Uint32						m_currPose;
	TDynArray< Matrix >			m_visualPose;

	CHeadComponent*				m_headComponent;
	CEntity*					m_previewEntity;

	CEdAnimBrowserPreview*		m_preview;

public:
	wxToolBar* GetSkeletonToolbar();

protected:
	void UpdatePose();
	void ApplyPose();
	Float GetWeight() const;

	void LoadEntity( const String &entName );
	void UnloadEntity();

	virtual void SaveOptionsToConfig();
	virtual void LoadOptionsFromConfig();

	virtual void SaveSession( CConfigurationManager &config );
	virtual void RestoreSession( CConfigurationManager &config );

	EHeadState GetDesiredHeadState() const;

public: // IAnimBrowserPreviewListener
	virtual void OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame );

protected:
	void OnLoadEntity( wxCommandEvent& event );
	void OnStartPose( wxCommandEvent& event );
	void OnEndPose( wxCommandEvent& event );
	void OnNextPose( wxCommandEvent& event );
	void OnPrevPose( wxCommandEvent& event );
	void OnSliderUpdate( wxCommandEvent& event );
	void OnShowSkeleton( wxCommandEvent& event );
};

class CEdSkeletonDialog : public wxDialog
{
	DECLARE_EVENT_TABLE();

public:
	CEdSkeletonDialog( wxWindow* parent, wxString caption, CSkeleton* skeleton );

	Int32 DoModal();

protected:
	void OnOk( wxCommandEvent& event );
};
*/
