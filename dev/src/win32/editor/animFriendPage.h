
#pragma once

class CEdAnimationFriend;
class CPlayedSkeletalAnimation;

class CEdAnimationFriendPage
{
private:
	CEdAnimationFriend*		m_owner;

public:
	virtual wxWindow*		GetPanelWindow() = 0;
	virtual wxAuiPaneInfo	GetPageInfo() const = 0;

public:
	CEdAnimationFriendPage( CEdAnimationFriend* owner );

	virtual void OnTick( Float dt ) {}
	virtual void OnGenerateFragments( CRenderFrame *frame ) {}
	virtual void OnLoadPreviewEntity( CAnimatedComponent* component ) {}
	virtual void OnUnloadPreviewEntity() {}
	virtual void OnSelectPreviewItem( IPreviewItem* item ) {}
	virtual void OnDeselectAllPreviewItems() {}
	virtual void OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data ) {}

	virtual void OnAnimationStarted( const CPlayedSkeletalAnimation* animation ) {}
	virtual void OnAnimationBlendInFinished( const CPlayedSkeletalAnimation* animation ) {}
	virtual void OnAnimationBlendOutStarted( const CPlayedSkeletalAnimation* animation ) {}
	virtual void OnAnimationFinished( const CPlayedSkeletalAnimation* animation ) {}
	virtual void OnAnimationStopped( const CPlayedSkeletalAnimation* animation ) {}

protected:
	void SetCustomPerspective( const wxString& code );

	IPreviewItemContainer* GetPreviewItemContainer();
	void CollectAllAnimations( TDynArray< const CSkeletalAnimationSetEntry* >& anims ) const;
	const CAnimatedComponent* GetAnimatedComponent() const;

	CPlayedSkeletalAnimation* PlayAnimation( const CSkeletalAnimationSetEntry* animation );
	CPlayedSkeletalAnimation* PlaySingleAnimation( const CSkeletalAnimationSetEntry* animation );
	CPlayedSkeletalAnimation* GetPlayedAnimation();
	void StopAnimation();
	void PauseAnimation();
};

class CEdAnimationFriendSimplePage  : public wxPanel, public CEdAnimationFriendPage
{
	DECLARE_EVENT_TABLE()

public:
	CEdAnimationFriendSimplePage( CEdAnimationFriend* owner );

	virtual wxWindow* GetPanelWindow() { return this; }
};
