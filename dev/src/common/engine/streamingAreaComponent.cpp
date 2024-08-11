/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "streamingAreaComponent.h"
#include "renderFrame.h"
#include "game.h"

IMPLEMENT_ENGINE_CLASS( CStreamingAreaComponent );

CStreamingAreaComponent::CStreamingAreaComponent()
	: m_testInEditor( false )
{
}

Color CStreamingAreaComponent::CalcLineColor() const
{
	return Color( 200, 64, 128, 128 );
}

void CStreamingAreaComponent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag )
{
	TBaseClass::OnGenerateEditorFragments( frame, flag );

	if ( flag & SHOW_Areas )
	{
		frame->AddDebugBox( GetBoundingBox(), Matrix::IDENTITY, CalcLineColor(), false, true );
	}
}

void CStreamingAreaComponent::OnPropertyPostChange( CProperty* prop )
{
	TBaseClass::OnPropertyPostChange( prop );

#ifndef NO_EDITOR
	if ( prop->GetName() == TXT("testInEditor") )
	{
		const Bool hasTags = !GetEntity()->GetTags().Empty();
		const CName areaTag = hasTags ? GetEntity()->GetTags().GetTag(0) : CName::NONE;
		if ( m_testInEditor && areaTag )
		{
			GGame->EnableStreamingLockdown( GetFriendlyName(), areaTag );
		}
		else if ( !m_testInEditor && areaTag )
		{
			GGame->DisableStreamingLockdown( areaTag );
		}
	}
#endif
}

void CStreamingAreaComponent::OnDetached( CWorld* world )
{
	TBaseClass::OnDetached( world );

#ifndef NO_EDITOR
 	if ( m_testInEditor )
 	{
		const Bool hasTags = !GetEntity()->GetTags().Empty();
		const CName areaTag = hasTags ? GetEntity()->GetTags().GetTag(0) : CName::NONE;
		if ( areaTag )
		{
 			GGame->DisableStreamingLockdown( areaTag );
		}
 	}
#endif
}