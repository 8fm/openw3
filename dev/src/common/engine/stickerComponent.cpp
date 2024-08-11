/**
 * Copyright © 2009 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "stickersSystem.h"
#include "stickerComponent.h"
#include "game.h"
#include "renderFrame.h"
#include "world.h"
#include "baseEngine.h"
#include "entity.h"

IMPLEMENT_ENGINE_CLASS( CStickerComponent );

CStickerComponent::CStickerComponent()
	: m_text( String::EMPTY )
	, m_textColor( 255, 255, 255, 255 )
	, m_isVisible( true )
{
	/* intentionally empty */
}

void CStickerComponent::OnAttached( CWorld* world )
{
	// Pass to base class
	TBaseClass::OnAttached( world );

	// Register in editor fragment list
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_Stickers );
}

void CStickerComponent::OnDetached( CWorld* world )
{
	// Pass to base class
	TBaseClass::OnDetached( world );

	// Unregister from editor fragment list
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_Stickers );
}

void CStickerComponent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag )
{
	// Base fragments
	TBaseClass::OnGenerateEditorFragments( frame, flag );

	// Draw only if visible
	if ( m_isVisible && flag == SHOW_Stickers )
	{
		if( m_text.Empty() == false )
		{
			frame->AddDebugText( GetLocalToWorld().GetTranslation(), m_text, true, m_textColor, Color(0,0,0,255), NULL );
		}
	}
}

void CStickerComponent::SetText( const String& text )
{
	m_text = text;
}

void CStickerComponent::SetVisible( Bool visible )
{
	m_isVisible = visible;
}
