/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "helpTextComponent.h"
#include "game.h"
#include "renderFrame.h"
#include "world.h"
#include "entity.h"

IMPLEMENT_ENGINE_CLASS( CHelpTextComponent );

CHelpTextComponent::CHelpTextComponent()
	: m_text( String::EMPTY )
	, m_textColor( 255, 255, 255, 255 )
	, m_backgroundColor( 0, 0, 0, 255 )
	, m_drawBackground( true )
{
	/* intentionally empty */
}

void CHelpTextComponent::OnAttached( CWorld* world )
{
	// Pass to base class
	TBaseClass::OnAttached( world );

	// Register in editor fragment list
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_VisualDebug );
}

void CHelpTextComponent::OnDetached( CWorld* world )
{
	// Unregister from editor fragment list
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_VisualDebug );

	// Pass to base class
	TBaseClass::OnDetached( world );
}

void CHelpTextComponent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag )
{
	// Base fragments
	TBaseClass::OnGenerateEditorFragments( frame, flag );

	// Draw only if visible
	if( flag == SHOW_VisualDebug )
	{
		frame->AddDebugText( GetLocalToWorld().GetTranslation(), m_text, m_drawBackground, m_textColor, m_backgroundColor, nullptr );
	}
}
