/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "spriteComponent.h"
#include "renderFrame.h"
#include "world.h"
#include "bitmapTexture.h"

IMPLEMENT_ENGINE_CLASS( CSpriteComponent );

CSpriteComponent::CSpriteComponent()
	: m_isVisible( true )
{
}

void CSpriteComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_Sprites );
}

void CSpriteComponent::OnDetached( CWorld* world )
{
	TBaseClass::OnDetached( world );

	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_Sprites );
}

void CSpriteComponent::SetVisible( Bool visible )
{
	m_isVisible = visible;
}

void CSpriteComponent::SetSpriteIcon( CBitmapTexture* icon )
{
	m_icon = icon;
}

Color CSpriteComponent::CalcSpriteColor() const
{
	if ( IsSelected() )
	{
		return Color::GREEN;
	}
	else
	{
		return Color::WHITE;
	}
}

Float CSpriteComponent::CalcSpriteSize() const
{
	return 0.25f;
}

void CSpriteComponent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag )
{
	// Base fragments
	TBaseClass::OnGenerateEditorFragments( frame, flag );

	// Draw only if visible
	if ( IsVisible() && flag == SHOW_Sprites )
	{
		// Sprite icon
		CBitmapTexture* icon = GetSpriteIcon();
		if ( icon )
		{
			// Draw editor icons
			Float screenScale = frame->GetFrameInfo().CalcScreenSpaceScale( GetWorldPosition() );
			const Float size = 0.25f*screenScale;

#ifndef NO_COMPONENT_GRAPH
			frame->AddSprite( GetLocalToWorld().GetTranslation(), size, CalcSpriteColor(), GetHitProxyID(), icon, IsOverlay() );
#else
			frame->AddSprite( GetLocalToWorld().GetTranslation(), size, CalcSpriteColor(), CHitProxyID(), icon, IsOverlay() );
#endif
		}
	}
}

CBitmapTexture* CSpriteComponent::GetSpriteIcon() const
{
	return m_icon.Get();
}

Bool CSpriteComponent::IsOverlay() const
{
	return false;
}
