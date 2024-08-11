/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#ifndef RED_UNIT_TEST_ENGINE_POSE_MOCKS_H_
#define RED_UNIT_TEST_ENGINE_POSE_MOCKS_H_

#include "../../common/engine/skeleton.h"
#include "../../common/engine/poseBlock.h"
#include "../../common/engine/poseProvider.h"
#include "../../common/engine/poseProviderFactory.h"
#include "../../common/engine/posePool.h"


class CSkeletonMock : public CSkeleton
{
public:
	RED_DECLARE_UNIT_TEST_CLASS( CSkeletonMock );
	MOCK_CONST_METHOD0( IsValid, Bool() );
};

class CPoseProviderMock : public CPoseProvider
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

public:
	MOCK_METHOD0( Sanitize, void() );
};

class CPoseProviderFactoryMock : public CPoseProviderFactory
{
public:
	MOCK_CONST_METHOD1( CreatePoseProvider, PoseProviderHandle ( const CSkeleton * skeleton ) );
};

class CPoseBlockMock : public CPoseBlock
{
public:
	MOCK_METHOD3( Initialize, void ( const CSkeleton * skeleton, Red::UniqueBuffer & boneBuffer, Red::UniqueBuffer & trackBuffer ) );
	MOCK_METHOD0( TakePose, CPoseHandle () );
	MOCK_CONST_METHOD0( IsPoseAvailable, Bool () );
	MOCK_CONST_METHOD0( IsAllPoseAvailable, Bool() );
};

class CPosePoolMock : public CPosePool
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

public:
	MOCK_METHOD1( TakeBlock, CPoseBlock * ( const CSkeleton *) );
	MOCK_METHOD1( GiveBlock, void ( CPoseBlock * ) );
};

class CPoseMock : public SBehaviorGraphOutput
{
public:

	MOCK_METHOD3( Init, void ( Uint32 numBones, Uint32 numFloatTracks, Bool createEvents ) );
	MOCK_METHOD1( Reset, void ( const CSkeleton* skeleton ) );
};

#endif
