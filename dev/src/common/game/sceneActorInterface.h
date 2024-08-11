
#pragma once

class ISceneActorInterface
{
public:
	virtual Bool HasMimicAnimation( const CName& slot ) const = 0;
	virtual Bool PlayMimicAnimation( const CName& anim, const CName& slot, Float blentTime = 0.0f, Float offset = 0.0f ) = 0;
	virtual Bool StopMimicAnimation( const CName& slot ) = 0;

	virtual Bool PlayLipsyncAnimation( CSkeletalAnimationSetEntry* anim, Float offset = 0.0f ) = 0;
	virtual Bool StopLipsyncAnimation() = 0;

public:
	virtual Bool HasSceneMimic() const = 0;
	virtual Bool SceneMimicOn() = 0;
	virtual void SceneMimicOff() = 0;

public:
	virtual CEntity* GetSceneParentEntity() = 0;

	virtual Vector GetSceneHeadPosition() const = 0;

	virtual Int32 GetSceneHeadBone() const = 0;

	virtual Bool WasSceneActorVisibleLastFrame() const = 0;

	virtual Vector GetBarPosition() const = 0;
	virtual Vector GetAimPosition() const = 0;

	virtual CName GetSceneActorVoiceTag() const = 0;
};
