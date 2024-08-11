/*
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS

#include "redGuiManager.h"
#include "redGuiLabel.h"
#include "redGuiPanel.h"
#include "redGuiList.h"
#include "redGuiListItem.h"
#include "redGuiGridLayout.h"
#include "redGuiCheckBox.h"
#include "debugWindowPhysicsArea.h"
#include "renderer.h"
#include "renderSettings.h"
#include "fonts.h"
#include "game.h"
#include "renderProxy.h"
#include "world.h"
#include "../physics/physicsWorld.h"

CDebugWindowPhysicsArea::CDebugWindowPhysicsArea()
	: RedGui::CRedGuiWindow( 512, 512, 640, 512 )
{
	GRedGui::GetInstance().EventTick.Bind( this, &CDebugWindowPhysicsArea::NotifyOnTick );
}

CDebugWindowPhysicsArea::~CDebugWindowPhysicsArea()
{
	GRedGui::GetInstance().EventTick.Unbind( this, &CDebugWindowPhysicsArea::NotifyOnTick );
}

void CDebugWindowPhysicsArea::OnWindowOpened( CRedGuiControl* control )
{
}

void CDebugWindowPhysicsArea::NotifyOnTick( RedGui::CRedGuiEventPackage& eventPackage, Float deltaTime )
{
}
	
void CDebugWindowPhysicsArea::Draw()
{
	Box2 area = GetAbsoluteCoord();
	GetTheme()->DrawRawRectangle( area.Min, area.Max, Color::GRAY, true );

	// Font info
	auto font = GetFont();
	Int32 fx = 0, fy = 0;
	Uint32 fw = 0, fh = 0;
	if ( font != nullptr )
	{
		font->GetTextRectangle( TXT("W"), fx, fy, fw, fh );
		fh += 2;
	}

	Vector2 gmin = area.Min + Vector2( 5.0f, (Float)fh*2.0f + 5.0f );
	Vector2 gmax = (area.Min + area.Max) - Vector2( 5.0f, 5.0f );
	Vector2 csize = Vector2( 8.0f * 2.0f + 1.0f, 8.0f * 2.0f + 1.0f );

	// Get world and streaming info
	CWorld* world = GGame->GetActiveWorld();
	if ( world == nullptr ) return;
	CPhysicsWorld* physicsWorld = nullptr;
	world->GetPhysicsWorld( physicsWorld );
	for ( Uint32 row = 0; row < physicsWorld->m_areaNumTilesPerEdge; ++row )
	{
		for ( Uint32 col = 0; col < physicsWorld->m_areaNumTilesPerEdge; ++col )
		{
			Vector2 cmin = gmin + Vector2( csize.X*col + 1.0f, csize.Y*row + 1.0f );

			Color color;
			Uint64 sector =	physicsWorld->m_area[ row * physicsWorld->m_areaNumTilesPerEdge + col ];
			if( sector == 0 )
			{
				GetTheme()->DrawRawFilledRectangle( cmin, csize - Vector2( 1.0f, 1.0f ), Color::RED, true );
				continue;
			}

			for( Uint8 y = 0; y != 8; ++y )
				for( Uint8 x = 0; x != 8; ++x )
				{
					Uint64 bit = 0x1LL << ( y * 8 + x );
					Color color = ( ( sector & bit ) == 0 ) ? Color::RED : Color::GREEN;

					GetTheme()->DrawRawFilledRectangle( cmin + Vector2( 2.0f * x, 2.0f * y ), Vector2( 2.0f, 2.0f ), color, true );
				}

		}
	}
}


#endif
#endif
