/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "skeletalAnimation.h"

//////////////////////////////////////////////////////////////////////////
// To jest totalnie do przerobienia
// Nie ma dziedziczenia tylko
// Anim buffer jest debug itp - nie ma sample virtual bo niepotrzebne na teraz a kosztuje

#ifndef NO_DEFAULT_ANIM

// This is cool debug procedural animation
class CDebugAnimation : public CSkeletalAnimation
{
	DECLARE_RTTI_SIMPLE_CLASS( CDebugAnimation );

private:
	const CSkeleton*	m_skeleton;
	Int32					m_boneLeft;
	Int32					m_boneRight;

public:
	CDebugAnimation();

	void Initialize( const CSkeleton* skeleton );

public:	
	//! Serialize object
	virtual void OnSerialize( IFile &file );

public:
	//! Generate bounding box
	virtual Bool GenerateBoundingBox( const CAnimatedComponent* component );

	//! Does this animation have extracted motion ?
	virtual Bool HasExtractedMotion() const; 

	//! Get movement parameters at given time
	virtual RedQsTransform GetMovementAtTime( Float time ) const;

	//! Get movement delta between times in animation
	virtual RedQsTransform GetMovementBetweenTime( Float startTime, Float endTime, Int32 loops ) const;

	//! Is compressed
	virtual Bool IsCompressed() const;

public:
	//! Preload animation
	virtual void Preload() const;

	//! Force sync load of  animation
	virtual void SyncLoad() const;

	//! Is animation loaded
	virtual EAnimationBufferDataAvailable GetAnimationBufferDataAvailable( Uint32 bonesRequested, Uint32 & outBonesLoaded, Uint32 & outBonesAlwaysLoaded ) const;

	//! Get number of track
	virtual Uint32 GetTracksNum() const;

	//! Get number of bones
	virtual Uint32 GetBonesNum() const;

	//! Sample animation
	virtual Bool Sample( Float time, Uint32 boneNumIn, Uint32 tracksNumIn, AnimQsTransform* bonesOut, AnimFloat* tracksOut ) const;

	//! Sample animation
	virtual Bool Sample( Float time, TDynArray< AnimQsTransform >& bonesOut, TDynArray< AnimFloat >& tracksOut ) const;

	//! Sample animation (fallback)
	virtual Bool SampleFallback( Uint32 boneNumIn, Uint32 tracksNumIn, AnimQsTransform* bonesOut, AnimFloat* tracksOut ) const;

protected:
	// Cache bone index
	void CacheBones();

	// Set T Pose
	void SetTPose( Uint32 bonesNum, AnimQsTransform* bones ) const;

	// Rotate bone
	void RotateBone( Int32 boneIndex, Uint32 bonesNum, AnimQsTransform* bones, Float progress ) const;

	//! Sample procedural animation
	Bool SampleProceduralAnim( Float time, Uint32 boneNumIn, AnimQsTransform* bonesOut ) const;

	// Can add procedural anim
	Bool CanAddProceduralAnim() const;
};

BEGIN_CLASS_RTTI( CDebugAnimation );
	PARENT_CLASS( CSkeletalAnimation );
END_CLASS_RTTI();

#endif
