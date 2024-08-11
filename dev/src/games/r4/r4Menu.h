/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "../../common/game/menu.h"

//////////////////////////////////////////////////////////////////////////
// CR4Menu
//////////////////////////////////////////////////////////////////////////
class CR4Menu : public CMenu
{
	DECLARE_ENGINE_CLASS( CR4Menu, CMenu, 0 );

public:
	CR4Menu();

public:
	virtual Bool					CanKeepHud() const override;
	virtual Bool					RequestSubMenu( const CName& menuName, const THandle< IScriptable >& scriptInitData ) override;

public:
			void					CloseSubMenu();
			Bool					MakeModal( Bool make );
			void					EnableInput( Bool enable, Bool recursively = true );
			void					ApplyParams( Bool onStart );
			void					SetRenderOverride( Bool overrideOn );

public:
			void					OnClosing();

private:
			void					DisallowRenderGameWorld( Bool disallow );

private:
			void					funcGetSubMenu( CScriptStackFrame& stack, void* result );
			void					funcMakeModal( CScriptStackFrame& stack, void* result );
			void					funcSetRenderGameWorldOverride( CScriptStackFrame& stack, void* result );

private:
	Bool							m_gameRenderDeactivated;

private:
	static const Int32				m_menuTimeScalePriority;
};

//////////////////////////////////////////////////////////////////////////
// RTTI Boilerplate
//////////////////////////////////////////////////////////////////////////
BEGIN_CLASS_RTTI( CR4Menu );
	PARENT_CLASS( CMenu );
	NATIVE_FUNCTION( "GetSubMenu", funcGetSubMenu );
	NATIVE_FUNCTION( "MakeModal", funcMakeModal );
	NATIVE_FUNCTION( "SetRenderGameWorldOverride", funcSetRenderGameWorldOverride )
END_CLASS_RTTI();
