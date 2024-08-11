/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

struct StorySceneCameraSetting
{
	TDynArray< AnimQsTransform >	m_pose;		// Pose
	Matrix							m_offset;	// Offset - for calc pose in world space
};
