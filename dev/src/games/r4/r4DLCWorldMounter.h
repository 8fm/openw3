/**
* Copyright c 2015 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/game/dlcMounter.h"

class CR4WorldDescriptionDLC : public CObject
{
	DECLARE_ENGINE_CLASS( CR4WorldDescriptionDLC, CObject, 0 );

public:
	CR4WorldDescriptionDLC();

public:
	//! CObject
	virtual void OnFinalize() override;

public:
	CName			  m_worldEnumAreaName;
	String			  m_worldName;
	String			  m_worldMapPath;
	Int32			  m_worldMapPinX;
	Int32			  m_worldMapPinY;
	CName			  m_worldMapLoactionNameStringKey;
	CName			  m_worldMapLoactionDescriptionStringKey;
	CName			  m_requiredChunk;

	Float			  m_worldMiniMapSize;
	Int32			  m_worldMiniMapTileCount;
	Int32			  m_worldMiniMapExteriorTextureSize;
	Int32			  m_worldMiniMapInteriorTextureSize;
	Int32			  m_worldMiniMapTextureSize;
	Int32			  m_worldMiniMapMinLod;
	Int32			  m_worldMiniMapMaxLod;
	String			  m_worldMiniMapExteriorTextureExtension;
	String			  m_worldMiniMapInteriorTextureExtension;
	Int32			  m_worldMiniMapVminX;
	Int32			  m_worldMiniMapVmaxX;
	Int32			  m_worldMiniMapVminY;
	Int32			  m_worldMiniMapVmaxY;
	Int32			  m_worldMiniMapSminX;
	Int32			  m_worldMiniMapSmaxX;
	Int32			  m_worldMiniMapSminY;
	Int32			  m_worldMiniMapSmaxY;
	Float			  m_worldMiniMapMinZoom;
	Float			  m_worldMiniMapMaxZoom;
	Float			  m_worldMiniMapZoom12;
	Float			  m_worldMiniMapZoom23;
	Float			  m_worldMiniMapZoom34;
	Float             m_worldGradientScale;
	Float             m_worldPreviewHeight;
};

BEGIN_CLASS_RTTI( CR4WorldDescriptionDLC );
PARENT_CLASS( CObject );
PROPERTY_EDIT( m_worldEnumAreaName, TXT( "Name of new enum entry for EAreaName" ) );
PROPERTY_EDIT( m_worldName, TXT( "Name of world visable in scripts" ) );
PROPERTY_EDIT( m_worldMapPath, TXT( "World map file path" ) );
PROPERTY_EDIT( m_worldMapPinX, TXT( "World map pin position X" ) );
PROPERTY_EDIT( m_worldMapPinY, TXT( "World map pin position Y" ) );
PROPERTY_EDIT( m_worldMapLoactionNameStringKey, TXT( "World map name" ) );
PROPERTY_EDIT( m_worldMapLoactionDescriptionStringKey, TXT( "World map description" ) );
PROPERTY_EDIT( m_requiredChunk, TXT( "Required PlayGo chunk name" ) );

PROPERTY_EDIT( m_worldMiniMapSize, TXT( "World mini map size" ) );
PROPERTY_EDIT( m_worldMiniMapTileCount, TXT( "World mini map tile count" ) );
PROPERTY_EDIT( m_worldMiniMapExteriorTextureSize, TXT( "World mini map exterior texture size" ) );
PROPERTY_EDIT( m_worldMiniMapInteriorTextureSize, TXT( "World mini map interior texture size" ) );
PROPERTY_EDIT( m_worldMiniMapTextureSize, TXT( "World mini map texture size" ) );
PROPERTY_EDIT( m_worldMiniMapMinLod, TXT( "World mini map min LOD" ) );
PROPERTY_EDIT( m_worldMiniMapMaxLod, TXT( "World mini map max LOD" ) );
PROPERTY_EDIT( m_worldMiniMapExteriorTextureExtension, TXT( "World mini map exterior texture extension" ) );
PROPERTY_EDIT( m_worldMiniMapInteriorTextureExtension, TXT( "World mini map interior texture extension" ) );
PROPERTY_EDIT( m_worldMiniMapVminX, TXT( "World mini map vminX" ) );
PROPERTY_EDIT( m_worldMiniMapVmaxX, TXT( "World mini map vmaxX" ) );
PROPERTY_EDIT( m_worldMiniMapVminY, TXT( "World mini map vminY" ) );
PROPERTY_EDIT( m_worldMiniMapVmaxY, TXT( "World mini map vmaxY" ) );
PROPERTY_EDIT( m_worldMiniMapSminX, TXT( "World mini map sminX" ) );
PROPERTY_EDIT( m_worldMiniMapSmaxX, TXT( "World mini map smaxX" ) );
PROPERTY_EDIT( m_worldMiniMapSminY, TXT( "World mini map sminY" ) );
PROPERTY_EDIT( m_worldMiniMapSmaxY, TXT( "World mini map smaxY" ) );
PROPERTY_EDIT( m_worldMiniMapMinZoom, TXT( "World mini map min zoom" ) );
PROPERTY_EDIT( m_worldMiniMapMaxZoom, TXT( "World mini map max zoom" ) );
PROPERTY_EDIT( m_worldMiniMapZoom12, TXT( "World mini map zoom12" ) );
PROPERTY_EDIT( m_worldMiniMapZoom23, TXT( "World mini map zoom23" ) );
PROPERTY_EDIT( m_worldMiniMapZoom34, TXT( "World mini map zoom34" ) );
PROPERTY_EDIT( m_worldGradientScale, TXT( "World gradient scale" ) );
PROPERTY_EDIT( m_worldPreviewHeight, TXT( "World preview height" ) );

END_CLASS_RTTI();

class CR4WorldDLCExtender : public CObject
{
	DECLARE_ENGINE_CLASS( CR4WorldDLCExtender, CObject, 0 );

public:
	CR4WorldDLCExtender();

	void RegisterDlcWorld( const Int32 areaType, const String& areaName, THandle< CR4WorldDescriptionDLC >& worldDescriptionHandle );
	void UnregisterDlcWorld( const Int32 areaType );

private:
	void funcAreaTypeToName( CScriptStackFrame& stack, void* result );
	void funcAreaNameToType( CScriptStackFrame& stack, void* result );

	void funcGetMiniMapSize( CScriptStackFrame& stack, void* result );
	void funcGetMiniMapTileCount( CScriptStackFrame& stack, void* result );
	void funcGetMiniMapExteriorTextureSize( CScriptStackFrame& stack, void* result );
	void funcGetMiniMapInteriorTextureSize( CScriptStackFrame& stack, void* result );
	void funcGetMiniMapTextureSize( CScriptStackFrame& stack, void* result );
	void funcGetMiniMapMinLod( CScriptStackFrame& stack, void* result );
	void funcGetMiniMapMaxLod( CScriptStackFrame& stack, void* result );
	void funcGetMiniMapExteriorTextureExtension( CScriptStackFrame& stack, void* result );
	void funcGetMiniMapInteriorTextureExtension( CScriptStackFrame& stack, void* result );
	void funcGetMiniMapVminX( CScriptStackFrame& stack, void* result );
	void funcGetMiniMapVmaxX( CScriptStackFrame& stack, void* result );
	void funcGetMiniMapVminY( CScriptStackFrame& stack, void* result );
	void funcGetMiniMapVmaxY( CScriptStackFrame& stack, void* result );
	void funcGetMiniMapSminX( CScriptStackFrame& stack, void* result );
	void funcGetMiniMapSmaxX( CScriptStackFrame& stack, void* result );
	void funcGetMiniMapSminY( CScriptStackFrame& stack, void* result );
	void funcGetMiniMapSmaxY( CScriptStackFrame& stack, void* result );
	void funcGetMiniMapMinZoom( CScriptStackFrame& stack, void* result );
	void funcGetMiniMapMaxZoom( CScriptStackFrame& stack, void* result );
	void funcGetMiniMapZoom12( CScriptStackFrame& stack, void* result );
	void funcGetMiniMapZoom23( CScriptStackFrame& stack, void* result );
	void funcGetMiniMapZoom34( CScriptStackFrame& stack, void* result );
	void funcGetGradientScale( CScriptStackFrame& stack, void* result );
	void funcGetPreviewHeight( CScriptStackFrame& stack, void* result );
private:
	THashMap<Int32, String> m_areaTypeToName;
	THashMap<String, Int32> m_areaNameToType;
	THashMap<Int32, THandle< CR4WorldDescriptionDLC > > m_dlcWorlds;
};

BEGIN_CLASS_RTTI( CR4WorldDLCExtender );
PARENT_CLASS( CObject );
NATIVE_FUNCTION( "AreaTypeToName", funcAreaTypeToName );
NATIVE_FUNCTION( "AreaNameToType", funcAreaNameToType );
NATIVE_FUNCTION( "GetMiniMapSize", funcGetMiniMapSize );
NATIVE_FUNCTION( "GetMiniMapTileCount", funcGetMiniMapTileCount );
NATIVE_FUNCTION( "GetMiniMapExteriorTextureSize", funcGetMiniMapExteriorTextureSize );
NATIVE_FUNCTION( "GetMiniMapInteriorTextureSize", funcGetMiniMapInteriorTextureSize );
NATIVE_FUNCTION( "GetMiniMapTextureSize", funcGetMiniMapTextureSize );
NATIVE_FUNCTION( "GetMiniMapMinLod", funcGetMiniMapMinLod );
NATIVE_FUNCTION( "GetMiniMapMaxLod", funcGetMiniMapMaxLod  );
NATIVE_FUNCTION( "GetMiniMapExteriorTextureExtension", funcGetMiniMapExteriorTextureExtension );
NATIVE_FUNCTION( "GetMiniMapInteriorTextureExtension", funcGetMiniMapInteriorTextureExtension );
NATIVE_FUNCTION( "GetMiniMapVminX", funcGetMiniMapVminX );
NATIVE_FUNCTION( "GetMiniMapVmaxX", funcGetMiniMapVmaxX );
NATIVE_FUNCTION( "GetMiniMapVminY", funcGetMiniMapVminY );
NATIVE_FUNCTION( "GetMiniMapVmaxY", funcGetMiniMapVmaxY );
NATIVE_FUNCTION( "GetMiniMapSminX", funcGetMiniMapSminX );
NATIVE_FUNCTION( "GetMiniMapSmaxX", funcGetMiniMapSmaxX );
NATIVE_FUNCTION( "GetMiniMapSminY", funcGetMiniMapSminY );
NATIVE_FUNCTION( "GetMiniMapSmaxY", funcGetMiniMapSmaxY );
NATIVE_FUNCTION( "GetMiniMapMinZoom", funcGetMiniMapMinZoom );
NATIVE_FUNCTION( "GetMiniMapMaxZoom", funcGetMiniMapMaxZoom );
NATIVE_FUNCTION( "GetMiniMapZoom12", funcGetMiniMapZoom12 );
NATIVE_FUNCTION( "GetMiniMapZoom23", funcGetMiniMapZoom23 );
NATIVE_FUNCTION( "GetMiniMapZoom34", funcGetMiniMapZoom34 );
NATIVE_FUNCTION( "GetGradientScale", funcGetGradientScale );
NATIVE_FUNCTION( "GetPreviewHeight", funcGetPreviewHeight );
END_CLASS_RTTI();


class CR4WorldDLCMounter : public IGameplayDLCMounter
{
	DECLARE_ENGINE_CLASS( CR4WorldDLCMounter, IGameplayDLCMounter, 0 );

public:
	CR4WorldDLCMounter();

	//! ISerializable
	virtual void OnPostLoad();

	//! CObject
	virtual void OnFinalize();

	//! IGameplayDLCMounter
public:
	virtual bool OnCheckContentUsage() override;
	virtual void OnGameStarting() override;
	virtual void OnGameEnding() override;

	virtual void OnScriptReloaded() override;

#ifndef NO_EDITOR

	virtual void OnEditorStarted() override;
	virtual void OnEditorStopped() override;

	virtual Bool DoAnalyze( CAnalyzerOutputList& outputList ) override;

#endif // !NO_EDITOR

	void RemAreaMapPinsEntry( const CR4WorldDescriptionDLC* worldDescriptionDLC );

private:
	void Activate();
	void Deactivate();

	void AddAreaNameEnumEntries();
	void RemAreaNameEnumEntries();

	void AddAreaMapPinsEntries();
	void RemAreaMapPinsEntries();

private:
	TDynArray< THandle< CR4WorldDescriptionDLC > > m_worlds;
};

BEGIN_CLASS_RTTI( CR4WorldDLCMounter );
PARENT_CLASS( IGameplayDLCMounter );
PROPERTY_INLINED( m_worlds, TXT( "Worlds used by the dlc" ) );
END_CLASS_RTTI();
