/**
 * Copyright © 2012 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#include "worldMap.h"
#include "../../common/game/journalResource.h"

BEGIN_CLASS_RTTI( SStaticMapPin )
	PROPERTY_EDIT( m_tag, TXT("Tag") )
	PROPERTY_EDIT( m_iconType, TXT("Pin icon type") )
#ifndef NO_RESOURCE_IMPORT
	PROPERTY_EDIT_NOT_COOKED( m_comment, TXT("Comment") )
#endif
	PROPERTY_EDIT( m_posX, TXT("X map coordinate") )
	PROPERTY_EDIT( m_posY, TXT("Y map coordinate") )
	PROPERTY_EDIT( m_journalEntry, TXT("Journal entry. Should be a location glossary entry..." ) )
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////

// Pulled into the .cpp because the importer project includes the header and PROPERTY needs a defined TTypeName< ... >
BEGIN_CLASS_RTTI( CWorldMap )
PARENT_CLASS( CResource )
	PROPERTY_RO( m_guid, TXT("Resource GUID") )
	PROPERTY_EDIT( m_displayTitle,  TXT("Localized title to display for the map") )
	PROPERTY_EDIT( m_imageInfo, TXT("Image info") )
	PROPERTY_EDIT( m_staticMapPins, TXT("Static map pins") )	
END_CLASS_RTTI()

// Pulled into the .cpp because the importer project includes the header and PROPERTY needs a defined TTypeName< ... >
// FIXME: Editable properties for now since we don't have a complete importer
BEGIN_CLASS_RTTI( SWorldMapImageInfo )
	PROPERTY_EDIT( m_cropRect, TXT("Image crop") )
	PROPERTY_EDIT( m_baseFileName, TXT("Base file name") )
	PROPERTY_EDIT( m_height, TXT("Image height") )
	PROPERTY_EDIT( m_width, TXT("Image width") )
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CWorldMap )
IMPLEMENT_ENGINE_CLASS( SWorldMapImageInfo )
IMPLEMENT_ENGINE_CLASS( SStaticMapPin )

//////////////////////////////////////////////////////////////////////////

CWorldMap::CWorldMap()
	: m_guid(CGUID::Create() )
{
}

//////////////////////////////////////////////////////////////////////////

String CWorldMap::GetDisplayTitle() const
{
	m_displayTitle.Load();
	String displayTitle = m_displayTitle.GetString();

	if ( displayTitle.Empty() )
	{
#ifndef RED_FINAL_BUILD
		displayTitle = TXT( "guid [[%s]]" ) + ToString( m_guid );
#endif
	}
	return displayTitle;
}

//////////////////////////////////////////////////////////////////////////

void CWorldMap::OnSave()
{
#ifndef NO_RESOURCE_IMPORT
	TBaseClass::OnSave();

	ASSERT( GetFile() );
#endif
}

//////////////////////////////////////////////////////////////////////////

#ifndef NO_RESOURCE_IMPORT

CWorldMap* CWorldMap::Create( const CWorldMap::FactoryInfo& data )
{
	// Check import data

	//	String errorMsg;
	Uint32 dataSize = data.m_importData.Size();

	CWorldMap* obj = NULL;

// 	for ( Uint32 i = 0; i < dataSize; ++i )
// 	{
// 
// 	}
	
	// Markers def
	TDynArray< SStaticMapPin > prevDef;

	if ( data.m_reuse )
	{
		// Reuse
		obj = data.m_reuse;
		prevDef = obj->m_staticMapPins;
		obj->m_staticMapPins.Clear();
	}
	else
	{
		// Create new resource
		obj = data.CreateResource();
	}

	// FIXME: For now just clobber. Need an editor to enforce unique tags
	for ( Uint32 i = 0; i < dataSize; ++i )
	{
		const FactoryInfo::StaticMapPinImportData& importData = data.m_importData[i];
		SStaticMapPin staticMapPin;
		staticMapPin.m_tag = importData.m_tag;
		staticMapPin.m_comment = importData.m_comment;
		staticMapPin.m_posX = importData.m_posX;
		staticMapPin.m_posY = importData.m_posY;
		staticMapPin.m_iconType = importData.m_type;
		obj->m_staticMapPins.PushBack( staticMapPin );
	}

	// Done
	//GFeedback->EndTask();

	return obj;
}

#endif