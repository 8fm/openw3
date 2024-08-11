/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/
#pragma once

//////////////////////////////////////////////////////////////////////////

struct SStaticMapPin
{
	DECLARE_RTTI_STRUCT( SStaticMapPin )

	CName									m_tag;
	CName									m_iconType;
	Int32									m_posX;
	Int32									m_posY;
#ifndef NO_RESOURCE_IMPORT
	String									m_comment;
#endif

	TSoftHandle< class CJournalResource >	m_journalEntry;

	RED_INLINE Bool operator ==( const SStaticMapPin& rhs ) const { return m_tag == rhs.m_tag; }
	RED_INLINE Bool operator ==( const CName& rhsTag ) const { return m_tag == rhsTag; }
};

struct WorldMapImporterParams : public IImporter::ImportOptions::ImportParams
{
};

struct SWorldMapImageInfo
{
	DECLARE_RTTI_STRUCT( SWorldMapImageInfo )
	Rect				m_cropRect;
	String				m_baseFileName;
	Int32				m_height;
	Int32				m_width;
};

class CWorldMap : public CResource
{
	DECLARE_ENGINE_RESOURCE_CLASS( CWorldMap, CResource, "worldmap", "World Map" );

private:
	CGUID						m_guid;
	SWorldMapImageInfo			m_imageInfo;
	LocalizedString				m_displayTitle;
	TDynArray< SStaticMapPin >	m_staticMapPins;

private:
	#ifndef NO_RESOURCE_IMPORT	
		THashMap< String, CGUID > m_importNameMap;
	#endif

public:

#ifndef NO_RESOURCE_IMPORT

	struct FactoryInfo : public CResource::FactoryInfo< CWorldMap >
	{				
		struct StaticMapPinImportData
		{
			Int32		m_posX;
			Int32		m_posY;
			CName	m_tag;
			CName	m_type;
			String	m_comment;
		};

		TDynArray< StaticMapPinImportData >	m_importData;
		WorldMapImporterParams*				m_params;
	};

	// Create new worldmap template
	static CWorldMap* Create( const FactoryInfo& data );

#endif

	virtual void OnSave();

public:
	CWorldMap();

	RED_INLINE const CGUID& GetGuid() const { return m_guid; }
	RED_INLINE const SWorldMapImageInfo& GetImageInfo() const { return m_imageInfo; }
	RED_INLINE const TDynArray< SStaticMapPin >& GetStaticMapPins() const { return m_staticMapPins; }

public:
	String GetDisplayTitle() const;
};