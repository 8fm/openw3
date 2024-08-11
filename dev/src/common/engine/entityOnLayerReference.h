/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

struct CEntityOnLayerReference
{
	DECLARE_RTTI_STRUCT( CEntityOnLayerReference );

	CGUID m_layerGuid;
	CGUID m_entityGuid;

	static RED_INLINE Bool Equal( const CEntityOnLayerReference& key1, const CEntityOnLayerReference& key2 )
	{
		return key1.m_layerGuid == key2.m_layerGuid && key1.m_entityGuid == key2.m_entityGuid;
	}

	static RED_INLINE Uint32 GetHash( const CEntityOnLayerReference& key )
	{
		return ::GetHash( key.m_layerGuid ) ^ ::GetHash( key.m_entityGuid );
	}
};

BEGIN_CLASS_RTTI( CEntityOnLayerReference );
	PROPERTY_RO( m_layerGuid,  TXT( "Guid of the layer at which the referenced entity is placed" ) );
	PROPERTY_RO( m_entityGuid, TXT( "Guid of the referenced entity" ) );
END_CLASS_RTTI();
