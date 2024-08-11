/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "animBrowser.h"

class CAnimBrowserTrajectoryPage;
struct SSplineBoneTrajectoryData;

class CSplineKnotItem : public IPreviewItem
{
public:
	CSplineKnotItem( CAnimBrowserTrajectoryPage* page, SSplineBoneTrajectoryData& data );

	virtual Bool IsValid() const;

	virtual void Refresh();

	virtual void SetPositionFromPreview( const Vector& prevPos, Vector& newPos );
	virtual void SetRotationFromPreview( const EulerAngles& prevRot, EulerAngles& newRot ) {}

	virtual IPreviewItemContainer* GetItemContainer() const;

protected:
	Int32 GetIdx() const;

protected:
	CAnimBrowserTrajectoryPage*	m_page;
	SSplineBoneTrajectoryData&	m_data;
};

struct SRawBoneTrajectoryData
{
	TDynArray< AnimQsTransform >	m_trans;
	TDynArray< Matrix >				m_mats;
	TDynArray< AnimVector4 >		m_vels;
	Float						m_minVel;
	Float						m_maxVel;

	void Set( Uint32 size )		{ m_trans.Resize( size ); m_mats.Resize( size ); m_vels.Resize( size ); }
	void Clear()				{ m_trans.Clear(); m_mats.Clear(); m_vels.Clear(); }

	Bool IsValid() const		{ return m_trans.Size() > 0; }
};

struct SSplineBoneTrajectoryData
{
	TDynArray< Vector >			m_pos;
	TDynArray< Vector >			m_vels;

	void Set( Uint32 size )		{ m_pos.Resize( size ); m_vels.Resize( size ); }

	Bool IsValid() const		{ return m_pos.Size() > 0; }
};

class CAnimBrowserTrajectoryPage	: public wxPanel
									, public IAnimBrowserPageInterface
									, public IPreviewItemContainer
{
	DECLARE_EVENT_TABLE()

	CEdAnimBrowser*					m_browser;
	Bool							m_active;
	wxNotebook*						m_notebook;

	Bool							m_showRawTrajectory;
	Bool							m_showRawRot;
	Bool							m_showRawLine;
	Bool							m_showRawCurrent;
	Bool							m_showRawVel;
	Bool							m_extractTrajectory;

	Bool							m_showSplineTrajectory;

	Uint32							m_ikHandsId;

	Int32								m_boneIdx;
	SRawBoneTrajectoryData			m_rawData;
	SSplineBoneTrajectoryData		m_splineData;

	CSkeletalAnimationSetEntry*		m_uncompressedAnimation;	//<! Uncompressed animation
	Red::Threads::CAtomic< Int32 >	m_refershIconsRequest;

public:
	CAnimBrowserTrajectoryPage( wxWindow* parent, CEdAnimBrowser* browser );
	~CAnimBrowserTrajectoryPage();

public: // IAnimBrowserPageInterface
	void DestroyPanel();
	void EnablePanel( Bool flag );
	void OnSelectedAnimation();
	void OnRefreshAnimation();
	void OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame );

	void Tick( Float timeDelta );

public: // IPreviewItemContainer
	virtual void OnSelectItem( IPreviewItem* item );
	virtual CWorld* GetPreviewItemWorld() const { return m_browser->m_previewPanel->GetPreviewWorld(); }
	void HandleSelection( const TDynArray< CHitProxyObject* >& objects );

protected:
	void OnLoadUncompressedAnimation( wxCommandEvent& event );
	void OnCreateRawTraj( wxCommandEvent& event );
	void OnCreateSplineTraj( wxCommandEvent& event );
	void OnEditSplineTraj( wxCommandEvent& event );
	void OnRefreshSettings( wxCommandEvent& event );
	void OnBoneSelected( wxCommandEvent& event );
	void OnSplinePointRemoved( wxCommandEvent& event );
	void OnSplinePointAddedAfter( wxCommandEvent& event );
	void OnSplinePointAddedBefore( wxCommandEvent& event );
	void OnIkHandsPressed( wxCommandEvent& event );

protected:
	Bool LoadUncompressedAnimation();
	void DestroyUncompressedAnimations();
	Bool CanLoadUncompressedAnimation() const;

	void RefreshAll();
	void RefreshLoadButton();
	void RefreshAnimNameLabel();
	void RefreshTrajButton();
	void RefreshSplineButtons();
	void RefreshSettings();

	void CreateRawTrajectory();
	void CreateSplineTrajectory();
	void SaveSplineTrajectoryToSelAnim();

	void EditSplineTrajectory( Bool state );
	void CreateItems( Uint32 size );

	void DrawBoneTrajectory( CRenderFrame *frame, const SRawBoneTrajectoryData& data );
	void DrawBoneTrajectory( CRenderFrame *frame, const SSplineBoneTrajectoryData& data );

	void EnableIkHands( Bool status );
};
