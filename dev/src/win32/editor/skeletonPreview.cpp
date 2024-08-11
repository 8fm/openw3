/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "skeletonPreview.h"

void ISkeletonPreviewControl::CreateSkeletonIcons()
{
	wxBitmap& bitmapSkeleton =	SEdResources::GetInstance().LoadBitmap( TEXT("IMG_SKELETON") );
	wxBitmap& bitmapNames =		SEdResources::GetInstance().LoadBitmap( TEXT("IMG_TEXT") );
	wxBitmap& bitmapAxis =		SEdResources::GetInstance().LoadBitmap( TEXT("IMG_AXIS") );

	GetSkeletonToolbar()->AddSeparator();

	GetSkeletonToolbar()->AddCheckTool( TOOL_SKELETON, wxT(""), bitmapSkeleton, wxNullBitmap, wxT("Show skeleton"));
	GetSkeletonToolbar()->AddCheckTool( TOOL_NAMES, wxT(""), bitmapNames, wxNullBitmap, wxT("Show bone names"));
	GetSkeletonToolbar()->AddCheckTool( TOOL_AXIS, wxT(""), bitmapAxis, wxNullBitmap, wxT("Show bone axis"));

	GetSkeletonToolbar()->Realize();
}

Bool ISkeletonPreviewControl::IsSkeleton()
{
	return GetSkeletonToolbar()->GetToolState( TOOL_SKELETON );
}

Bool ISkeletonPreviewControl::IsBoneNames()
{
	return GetSkeletonToolbar()->GetToolState( TOOL_NAMES );
}

Bool ISkeletonPreviewControl::IsBoneAxis()
{
	return GetSkeletonToolbar()->GetToolState( TOOL_AXIS );
}

void ISkeletonPreview::DisplaySkeleton( CRenderFrame *frame, CAnimatedComponent* component, Color color /* = Color::WHITE  */)
{
	Bool skeletonVisible = IsSkeleton();
	Bool skeletonNames = IsBoneNames();
	Bool skeletonAxis = IsBoneAxis();

	SkeletonRenderingUtils::DrawSkeleton( component, color, frame, skeletonVisible, skeletonNames, skeletonAxis );
}
