/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "../../common/game/popup.h"

//////////////////////////////////////////////////////////////////////////
// CR4Popup
//////////////////////////////////////////////////////////////////////////
class CR4Popup : public CPopup
{
	DECLARE_ENGINE_CLASS( CR4Popup, CPopup, 0 );

public:
			Bool					MakeModal( Bool make );
			void					EnableInput( Bool enable );
			void					ApplyParams( Bool onStart );

public:
			void					OnClosing();

private:
	void							funcMakeModal( CScriptStackFrame& stack, void* result );

private:
	static const Int32				m_popupTimeScalePriority;
};

//////////////////////////////////////////////////////////////////////////
// RTTI Boilerplate
//////////////////////////////////////////////////////////////////////////
BEGIN_CLASS_RTTI( CR4Popup );
	PARENT_CLASS( CPopup );
	NATIVE_FUNCTION( "MakeModal", funcMakeModal );
END_CLASS_RTTI();
