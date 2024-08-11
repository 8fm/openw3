/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "destructionResetTool.h"
#include "../../common/engine/destructionSystemComponent.h"


IMPLEMENT_ENGINE_CLASS( CEdDestructionResetTool );

CEdDestructionResetTool::CEdDestructionResetTool()
{
}


CEdDestructionResetTool::~CEdDestructionResetTool()
{
}


String CEdDestructionResetTool::GetCaption() const
{
	return TXT("Reset Destructibles");
}

Bool CEdDestructionResetTool::Start( CEdRenderingPanel* viewport, CWorld* world, wxSizer* m_panelSizer, wxPanel* panel, const TDynArray< CComponent* >& selection )
{
	TDynArray< CDestructionSystemComponent* > destructibles;
	world->GetAttachedComponentsOfClass< CDestructionSystemComponent >( destructibles );

	for ( Uint32 i = 0; i < destructibles.Size(); ++i )
	{
		destructibles[i]->Reset();
	}

	// Return false, since this tool doesn't stay active.
	return false;
}

void CEdDestructionResetTool::End()
{
}
