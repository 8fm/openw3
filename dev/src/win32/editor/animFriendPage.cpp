
#include "build.h"
#include "animFriendPage.h"
#include "animFriend.h"

CEdAnimationFriendPage::CEdAnimationFriendPage( CEdAnimationFriend* owner )
	: m_owner( owner )
{

}

void CEdAnimationFriendPage::SetCustomPerspective( const wxString& code )
{
	m_owner->SetCustomPagePerspective( code );
}

IPreviewItemContainer* CEdAnimationFriendPage::GetPreviewItemContainer()
{
	return m_owner->GetPreviewItemContainer();
}

void CEdAnimationFriendPage::CollectAllAnimations( TDynArray< const CSkeletalAnimationSetEntry* >& anims ) const
{
	m_owner->CollectAllAnimations( anims );
}

const CAnimatedComponent* CEdAnimationFriendPage::GetAnimatedComponent() const
{
	return m_owner->GetAnimatedComponent();
}

CPlayedSkeletalAnimation* CEdAnimationFriendPage::PlayAnimation( const CSkeletalAnimationSetEntry* animation )
{
	return m_owner->PlayAnimation( animation );
}

CPlayedSkeletalAnimation* CEdAnimationFriendPage::PlaySingleAnimation( const CSkeletalAnimationSetEntry* animation )
{
	return m_owner->PlaySingleAnimation( animation );
}

CPlayedSkeletalAnimation* CEdAnimationFriendPage::GetPlayedAnimation()
{
	return m_owner->GetPlayedAnimation();
}

void CEdAnimationFriendPage::StopAnimation()
{
	m_owner->StopAnimation();
}

void CEdAnimationFriendPage::PauseAnimation()
{
	m_owner->PauseAnimation();
}

BEGIN_EVENT_TABLE( CEdAnimationFriendSimplePage, wxPanel )
END_EVENT_TABLE()

CEdAnimationFriendSimplePage::CEdAnimationFriendSimplePage( CEdAnimationFriend* owner )
	: wxPanel( owner )
	, CEdAnimationFriendPage( owner )
{

}
