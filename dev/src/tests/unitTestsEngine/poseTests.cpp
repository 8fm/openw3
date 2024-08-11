/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "../../common/engine/behaviorGraphOutput.h"
#include "../../common/core/uniqueBuffer.h"

TEST( Pose, Pose_Init_assert_if_provided_buffer_is_not_aligned_to_16 )
{
	Red::UniqueBuffer buffer = Red::CreateUniqueBuffer( sizeof( AnimQsTransform ) * 2, 16 );

	Uint8 * unalignedBuffer = static_cast< Uint8* >( buffer.Get() ) + 1;

	SBehaviorGraphOutputParameter param = 
	{
		1,
		0,
		reinterpret_cast< AnimQsTransform* >( unalignedBuffer ),
		nullptr,
		nullptr,
		false
	};

	SBehaviorGraphOutput pose;
	EXPECT_DEATH_IF_SUPPORTED( pose.Init( param ), "" );
}
