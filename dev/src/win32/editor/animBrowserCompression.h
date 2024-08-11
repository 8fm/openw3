/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "animBrowser.h"

class CAnimBrowserCompressionPage	: public wxPanel
									, public IAnimBrowserPageInterface
{
	DECLARE_EVENT_TABLE()

	CEdAnimBrowser*					m_browser;
	Bool							m_active;

	wxChoice*						m_comprAnimationChoice;
	wxNotebook*						m_compressionNotebook;
	wxSlider*						m_compressionSlider;
	wxHtmlWindow*					m_infoWindow;

	CSkeletalAnimationSetEntry*		m_compressedAnimation;		//<! Compressed animation
	CSkeletalAnimationSetEntry*		m_uncompressedAnimation;	//<! Uncompressed animation

	IAnimationCompression*			m_compression;

	struct PreviewElem
	{
		CEntity*					m_entity;
		CAnimatedComponent*			m_component;
		CPlayedSkeletalAnimation*	m_playedAnimation;

		void Clear() { m_entity = NULL; m_component = NULL; m_playedAnimation = NULL; }
	};

	PreviewElem						m_uncomprElem;
	PreviewElem						m_comprElem;

	CEdUndoManager*					m_undoManager;

public:
	CAnimBrowserCompressionPage( wxWindow* parent, CEdAnimBrowser* browser, CEdUndoManager* undoManager );

public: // IAnimBrowserPageInterface
	void DestroyPanel();
	void EnablePanel( Bool flag );
	void OnSelectedAnimation();
	void OnRefreshAnimation();

protected:
	void OnAnimChanged( wxCommandEvent& event );
	void OnLoadAnimation( wxCommandEvent& event );
	void OnAnimUp( wxCommandEvent& event );
	void OnAnimDown( wxCommandEvent& event );
	void OnPosOffsetChanged( wxScrollEvent& event );
	void OnCompressSliderScrolled( wxScrollEvent& event );
	void OnShowMeshes( wxCommandEvent& event );
	void OnApply( wxCommandEvent& event );
	void OnFpsChanged( wxCommandEvent& event );

protected:
	Bool LoadUncompressedAnimation();
	Bool CloneEntitiesFromComponent( const CAnimatedComponent* originalComponent );
	Bool SetEntityForElem( TDynArray< Uint8 >& buffer, const String& originalComponentName, PreviewElem& elem, const Vector& offset );
	void DestroyEntities();
	void DestroyUncompressedAnimations();
	Float GetPreviewEntitiesOffset() const;
	void FillAnimationList( const CSkeletalAnimationSet* set );
	void RefreshAnimation( const CPlayedSkeletalAnimation* browserAnim, PreviewElem& elem, CSkeletalAnimationSetEntry* animation );
	void RecreateAnimation( const CPlayedSkeletalAnimation* browserAnim, PreviewElem& elem, CSkeletalAnimationSetEntry* animation );
	Bool CanLoadUncompressedAnimation() const;
	void RefreshLoadButton();
	void CreateCompression();
	void DestroyCompression();
	void CreateCompressedAnimation();
	void DestroyCompressedAnimation();
	void SetupCompressionFromAnim( const CSkeletalAnimationSetEntry* animation );
	void RefreshCompressionPage();
	void ChangedFpsTo( EAnimationFps newFps );
};
