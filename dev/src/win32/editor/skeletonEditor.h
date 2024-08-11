/*
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "animBrowserPreview.h"
#include "propertiesPageHavok.h"
#include "animationPreview.h"
#include "shapesPreviewItem.h"
#include "../../common/game/storySceneDisplayInterface.h"

class ShapesPreviewContainer;

class CEdSkeletonPreview : public CEdAnimationPreview
{
protected:
	const CSkeleton*			m_skeleton;
	ISkeletonPreviewControl*	m_skeletonPreviewControl;
	TDynArray< Matrix >			m_skeletonPoseWS;
	ShapesPreviewContainer		m_shapesContainer;

public:
	CEdSkeletonPreview( wxWindow* parent, ISkeletonPreviewControl* skeletonPreviewControl, const CSkeleton* skeleton, Bool allowRenderOptionsChange = false );
	~CEdSkeletonPreview();

	void HandleSelection( const TDynArray< CHitProxyObject* >& objects );

protected:
	virtual void OnViewportCalculateCamera( IViewport* view, CRenderCamera &camera );
	virtual void OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame );
	virtual Bool OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data );
	virtual void HandleContextMenu( Int32 x, Int32 y );
	virtual void OnViewportTick( IViewport* view, Float timeDelta );
};

//////////////////////////////////////////////////////////////////////////

class CEdSkeletonEditor	: public wxSmartLayoutFrame
						, public ISkeletonPreviewControl
{
	DECLARE_EVENT_TABLE();

public:

	CEdSkeletonEditor( wxWindow* parent, CSkeleton* skeleton );
	virtual ~CEdSkeletonEditor();

	CSkeleton*					m_skeleton;

	CAnimatedComponent*			m_animComponent;
	CEntity*					m_previewEntity;

	CEdPropertiesPage*			m_propertiesPage;
	CEdHavokPropertiesPage*		m_hkProp;
	CEdSkeletonPreview*			m_preview;

public:
	wxToolBar* GetSkeletonToolbar();

protected:
	void LoadEntity( const String &entName );
	void UnloadEntity();

	virtual void SaveOptionsToConfig();
	virtual void LoadOptionsFromConfig();

	void FillTrees();

public:
	static void FillSkeletonTrees( CSkeleton* skeleton, wxTreeCtrl* boneTree, wxTreeCtrl* trackTree, Bool showIndex = false );

	//dex++: changed to use generalized CSkeleton interface
	static void FillBoneTree( CSkeleton* skeleton, Int32 parentIndex, wxTreeItemId& parent, wxTreeCtrl* tree, Bool showIndex = false );
	static void FillTracksTree( CSkeleton* skeleton, wxTreeCtrl* tree, Bool showIndex = false );
	//dex--

protected:
	void OnLoadEntity( wxCommandEvent& event );
};
