/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#ifndef _RED_STREAMING_FOLIAGE_CONVERSION_PLUGIN_H_
#define _RED_STREAMING_FOLIAGE_CONVERSION_PLUGIN_H_

#if 0

//////////////////////////////////////////////////////////////////////
#include "streamingDataConversionPlugin.h"
#include "foliageLayer.h"
#include "baseTree.h"

class CFoliageConversionGroup : public CObject
{
public:
	DECLARE_ENGINE_CLASS(CFoliageConversionGroup, CObject, 0);

	THandle< CSRTBaseTree > m_baseTree;
	Box m_bounds;
	TDynArray< CFoliageInstance > m_instances;
	THashMap< Uint32, Uint32 > m_tempInstancesMap;		// In order to speed up multi-LOD instance merge, keep a bloom filter of instance indices

	virtual void OnSerialize( IFile& file )
	{
		TBaseClass::OnSerialize( file );
		if( file.IsWriter() )
		{
			file << m_bounds.Min;
			file << m_bounds.Max;

			Uint32 instanceCount = m_instances.Size();
			file << instanceCount;
			if( instanceCount > 0 )
			{
				// #YOLO
				CFoliageInstance* instanceBuffer = &m_instances[0];
				file.SerializeSimpleType( instanceBuffer, sizeof( CFoliageInstance ) * instanceCount );
			}
		}
		else if( file.IsReader() )
		{
			Uint32 instanceCount = 0;
			file << m_bounds.Min;
			file << m_bounds.Max;

			file << instanceCount;
			m_instances.Resize( instanceCount );
			if( instanceCount > 0 )
			{
				// #YOLO
				CFoliageInstance* instanceBuffer = &m_instances[0];
				file.SerializeSimpleType( instanceBuffer, sizeof( CFoliageInstance ) * instanceCount );
			}
		}
	}
};

BEGIN_CLASS_RTTI( CFoliageConversionGroup );
	PARENT_CLASS( CObject );
	PROPERTY( m_baseTree );
END_CLASS_RTTI();

class CFoliageStreamingConversionPlugin : public CStreamingDataConversionPlugin
{
public:
	CFoliageStreamingConversionPlugin();
	virtual ~CFoliageStreamingConversionPlugin();
	virtual void Initialise( CWorld* world, CGuidValidator* guidValidator, SChangelist* changelist );
	virtual void OnWorldStreamingTile( CLayer* worldLayer, CLayer* streamingTileLayer );
	virtual void OnShutdown();
	virtual void OnWorldLayer( CLayer* loadedLayer );
	virtual void OnFinaliseConversion();

private:
	void TakeOwnershipOfLayerFoliage( CLayer* sourceLayer );

	CFoliageConversionGroup* FindOrCreateGroup( CSRTBaseTree* baseTree, TDynArray< CFoliageConversionGroup* >& sourceGroups, const Box& bounds );
	TDynArray< CFoliageConversionGroup* >* ExtractAllInstanceGroups( CLayerFoliage* sourceLayer );
	String GenerateCombinedInstanceDataPath( CLayer* parentLayer, CLayerFoliage* foliageLayer );
	void SaveCombinedInstanceData( TDynArray< CFoliageConversionGroup* >* data, String path );
	Uint32 CalculateVectorHash( const Vector& v );
	TDynArray< CFoliageConversionGroup* >* LoadCombinedInstanceData( String path );
	void RefreshGc();

	CWorld* m_world;
	SChangelist* m_changelist;

	Int32 m_gcCollectionCounter;

	// Since we only need to run through the tile data once; keep track of tiles already processed
	TDynArray< String > m_processedStreamingTileFiles;
	TDynArray< String > m_processedInstanceData;
};

RED_INLINE Uint32 CFoliageStreamingConversionPlugin::CalculateVectorHash( const Vector& v )
{
	// Cheap, nasty, horrible vector hash - but its still better than a linear search! 
	String hashedString = String::Printf( TXT( "%f, %f, %f" ), v.X, v.Y, v.Z );
	return GetHash( hashedString );
}

#endif // if 0

#endif // _RED_STREAMING_FOLIAGE_CONVERSION_PLUGIN_H_
