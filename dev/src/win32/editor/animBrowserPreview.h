/*
 * Copyright © 2007 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "skeletonPreview.h"
#include "animationPreviewGhost.h"
#include "../../common/engine/soundAdditionalListener.h"

class CEdTimeline;
class CEdAnimBrowserPreview;
class CPlayedSkeletalAnimation;

class IAnimBrowserPreviewListener
{
public:
	virtual void Tick( Float timeDelta ) {}
	virtual void SelectItem( CNode* node ) {}
	virtual void HandleSelection( const TDynArray< CHitProxyObject* >& objects ) {}
	virtual void OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame ) {}
};

class CEdEntityClone
{
public:
	enum ECloneColor
	{
		CC_Original,
		CC_Default,
		CC_Red,
		CC_Green
	};

private:
	Uint32		m_id;
	CEntity*	m_entity;

	Float		m_offset;

	Bool		m_firstUpdate;
	Bool		m_temporary;
	Float		m_lifeTimer;

	THandle< CEntity > m_original;

public:
	CEdEntityClone( Uint32 id );
	~CEdEntityClone();

	Bool Init( const CEntity* entity );

	Bool Update( Float dt );

	Uint32 GetId() const;

	Bool CopyPoseFrom( const CEntity* entity );
	Bool CopyPoseFrom( const CPlayedSkeletalAnimation* anim, Bool compressedPose );

	void SetOffset( Float offset );
	void SetLifeTime( Float time );
	void SetColor( ECloneColor color );
	void SetVisibility( Bool flag );
};

class CEdAnimBrowserPreview : public CEdPreviewPanel 
							, public ISkeletonPreview
							, public IEdEventListener
							, public CSoundAdditionalListener

{
	Bool								m_usePlayerCamera;
	Bool								m_isCameraAlign;
	wxMenu*								m_contextMenu;
	CEntity*							m_cameraEntity;
	CCameraComponent*					m_cameraComponent;
	Vector								m_prevEntityPos;
	Float								m_timeMultiplier;
	CPlayedSkeletalAnimation*			m_playedAnimation;
	float								m_playerAnimationPreviousTime;
	ISkeletonPreviewControl*			m_skeletonPreviewControl;
	Bool								m_showBBox;
	TDynArray< CAnimatedComponent* >	m_animatedComponents;
	TDynArray< CEdEntityClone* >		m_clones;

	static const Vector					CAMERA_POS_OFFSET;
	static const EulerAngles			CAMERA_ROT_OFFSET;

	IAnimBrowserPreviewListener*		m_listener;

	PreviewGhostContainer				m_ghosts;

public:
	CEdAnimBrowserPreview( wxWindow* parent, ISkeletonPreviewControl* skeletonPreviewControl, IAnimBrowserPreviewListener* listener = NULL );

	~CEdAnimBrowserPreview();

	void SetUsePlayerCamera( Bool flag );

	RED_INLINE void SetContextMenu( wxMenu* contextMenu )
	{ m_contextMenu = contextMenu; }

	void AlignCamera( const Vector& pos );
	void ResetCamera();

	void OnLoadEntity( CEntity* entityCamera );
	void OnUnloadEntity();

	virtual void OnViewportTick( IViewport* view, Float timeDelta );
	virtual void OnViewportCalculateCamera( IViewport* view, CRenderCamera &camera );
	virtual void OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame );
	virtual Bool OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data );

	virtual void HandleContextMenu( Int32 x, Int32 y );
	virtual void HandleSelection( const TDynArray< CHitProxyObject* >& objects );

	virtual void DispatchEditorEvent( const CName& name, IEdEventData* data );

	void TakeScreenshotsFromAnim( CPlayedSkeletalAnimation *playedAnimation );

	virtual Bool IsSkeleton();
	virtual Bool IsBoneNames();
	virtual Bool IsBoneAxis();

	RED_INLINE Float GetTimeMultiplier() const
	{ return m_timeMultiplier; }

	RED_INLINE void SetTimeMultiplier( Float timeMultiplier )
	{ m_timeMultiplier = timeMultiplier; }

	RED_INLINE void SetPlayedAnimation( CPlayedSkeletalAnimation*	playedAnimation )
	{ m_playedAnimation = playedAnimation; }

	RED_INLINE void ToggleBBox()
	{  m_showBBox = !m_showBBox; }

	void SetAnimatedComponent( CAnimatedComponent* animatedComponent );
	void SetAnimatedComponents( TDynArray< CAnimatedComponent* >& animatedComponents );

	void RecreateClones();
	void DestroyClones();
	void UpdateClones( Float dt );

	Bool CreateClone( Uint32 id, const CEntity* entity );
	Bool CreateTemporaryClone( Uint32 id, const CEntity* entity, Float lifeTime );
	Bool DestroyClone( Uint32 id );
	Bool HasClone( Uint32 id ) const;
	Bool SyncCloneTo( Uint32 id, const CEntity* entity );
	Bool SyncCloneTo( Uint32 id, const CPlayedSkeletalAnimation* anim, Bool compressedPose = false );
	void SetOffsetForClone( Uint32 id, Float offset );
	void SetCloneColor( Uint32 id, CEdEntityClone::ECloneColor color );
	void SetCloneVisibility( Uint32 id, Bool flag );
	
	void ShowGhosts( Uint32 number, PreviewGhostContainer::EGhostType type );
	void HideGhosts();
	Bool HasGhosts();

private:
	CEdEntityClone* FindClone( Uint32 id ) const;
	void ResetCameraPosition();
};
