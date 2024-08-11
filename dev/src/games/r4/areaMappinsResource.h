#pragma once

struct SAreaMapPinInfo
{
	DECLARE_RTTI_STRUCT( SAreaMapPinInfo );

	Int32		m_areaType;
	Vector		m_position;
	String		m_worldPath;
	CName		m_requiredChunk;
	CName		m_localisationName;
	CName		m_localisationDescription;
};

BEGIN_CLASS_RTTI( SAreaMapPinInfo );
	PROPERTY_CUSTOM_EDIT( m_areaType, TXT( "Area type" ), TXT("ScriptedEnum_EAreaName") );
	PROPERTY_EDIT( m_position, TXT( "Position" ) );
	PROPERTY_EDIT( m_worldPath, TXT( "World path" ) );
	PROPERTY_CUSTOM_EDIT( m_requiredChunk, TXT("Required chunk"), TXT("PlayGoChunkSelector") );
	PROPERTY_EDIT( m_localisationName, TXT( "Localisation name" ) );
	PROPERTY_EDIT( m_localisationDescription, TXT( "Localisation description" ) );
END_CLASS_RTTI();

class CAreaMapPinsResource : public CResource
{
	DECLARE_ENGINE_RESOURCE_CLASS( CAreaMapPinsResource, CResource, "w2am", "Area Mappins" );

public:
	CAreaMapPinsResource();
	~CAreaMapPinsResource();

	RED_INLINE const TDynArray< SAreaMapPinInfo >& GetMapPinsInfo() const	{ return m_mappinsInfo; } 
	RED_INLINE TDynArray< SAreaMapPinInfo >& GetMapPinsInfo()				{ return m_mappinsInfo; } 

private:
    TDynArray< SAreaMapPinInfo > m_mappinsInfo;
};

BEGIN_CLASS_RTTI( CAreaMapPinsResource )
	PARENT_CLASS( CResource )
	PROPERTY_EDIT( m_mappinsInfo, TXT("Mappins info") )
END_CLASS_RTTI()