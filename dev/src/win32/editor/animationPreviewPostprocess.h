
#pragma once

class CRenderFrame;
class CAnimatedComponent;
class CPlayedSkeletalAnimation;

class CEdAnimationPreviewPostProcess
{
public:
	virtual void OnPreviewPostProcessGenerateFragments( CRenderFrame *frame, const CAnimatedComponent* componenet, const CPlayedSkeletalAnimation* animation ) = 0;
};

//////////////////////////////////////////////////////////////////////////

class CEdAnimationPreviewPostProcessTrajectory : public CEdAnimationPreviewPostProcess
{
	// bone etc.

public:
	CEdAnimationPreviewPostProcessTrajectory();

	virtual void OnPreviewPostProcessGenerateFragments( CRenderFrame *frame, const CAnimatedComponent* componenet, const CPlayedSkeletalAnimation* animation );
};

//////////////////////////////////////////////////////////////////////////

class CEdAnimationPreviewPostSkeleton: public CEdAnimationPreviewPostProcess
{
	Color m_color;

public:
	CEdAnimationPreviewPostSkeleton( const Color& color );

	virtual void OnPreviewPostProcessGenerateFragments( CRenderFrame *frame, const CAnimatedComponent* componenet, const CPlayedSkeletalAnimation* animation );
};

//////////////////////////////////////////////////////////////////////////

class CEdAnimationPreviewPostProcessHit : public CEdAnimationPreviewPostProcess
{
public:
	virtual void OnPreviewPostProcessGenerateFragments( CRenderFrame *frame, const CAnimatedComponent* componenet, const CPlayedSkeletalAnimation* animation );
};
