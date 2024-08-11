/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class ISkeletonPreviewControl;

class ISkeletonPreview
{
public:
	virtual Bool IsSkeleton() = 0;
	virtual Bool IsBoneNames()= 0;
	virtual Bool IsBoneAxis() = 0;

	// Have to be called in render frame function
	void DisplaySkeleton( CRenderFrame *frame, CAnimatedComponent* component, Color color = Color::WHITE );
};

class ISkeletonPreviewControl
{
public:
	Bool IsSkeleton();
	Bool IsBoneNames();
	Bool IsBoneAxis();

	// Have to be called in widget setup
	void CreateSkeletonIcons();

	static const Int32 TOOL_SKELETON =	3001;
	static const Int32 TOOL_AXIS =		3002;
	static const Int32 TOOL_NAMES =		3003;

	virtual wxToolBar* GetSkeletonToolbar() = 0;
};
