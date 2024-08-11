/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../../common/core/object.h"

#ifndef NO_SECOND_SCREEN 

#include "../../common/platformCommon/secondScreenManager.h"

class CR4SecondScreenManagerScriptProxy : public CObject, public ISecondScreenManagerDelegate
#else
class CR4SecondScreenManagerScriptProxy : public CObject
#endif
{
	DECLARE_ENGINE_CLASS( CR4SecondScreenManagerScriptProxy, CObject, 0 );

	//! Update list of map pins for global map
	void funcSendGlobalMapPins( CScriptStackFrame& stack, void* result );
	//! Update list of map pins for area map
	void funcSendAreaMapPins( CScriptStackFrame& stack, void* result );

	void funcSendGameMenuOpen( CScriptStackFrame& stack, void* result );
	void funcSendGameMenuClose( CScriptStackFrame& stack, void* result );

	void funcSendFastTravelEnable( CScriptStackFrame& stack, void* result );
	void funcSendFastTravelDisable( CScriptStackFrame& stack, void* result );

	void funcPrintJsonObjectsMemoryUsage( CScriptStackFrame& stack, void* result );

#ifndef NO_SECOND_SCREEN 
	//! ISecondScreenManagerDelegate
private:
	virtual void OnHandleFastTravel( const CName mapPinTag, Uint32 areaType, Bool currentWorld);
	virtual void OnHandleTrackQuest( const CGUID questQuid );
	virtual Bool IsGUIActive();
#endif //! NO_SECOND_SCREEN

};

BEGIN_CLASS_RTTI( CR4SecondScreenManagerScriptProxy )
	PARENT_CLASS( CObject )
	NATIVE_FUNCTION( "SendGlobalMapPins", funcSendGlobalMapPins );
	NATIVE_FUNCTION( "SendAreaMapPins", funcSendAreaMapPins);
	NATIVE_FUNCTION( "SendGameMenuOpen", funcSendGameMenuOpen );
	NATIVE_FUNCTION( "SendGameMenuClose", funcSendGameMenuClose);
	NATIVE_FUNCTION( "SendFastTravelEnable", funcSendFastTravelEnable );
	NATIVE_FUNCTION( "SendFastTravelDisable", funcSendFastTravelDisable);
	NATIVE_FUNCTION( "PrintJsonObjectsMemoryUsage", funcPrintJsonObjectsMemoryUsage);
END_CLASS_RTTI();