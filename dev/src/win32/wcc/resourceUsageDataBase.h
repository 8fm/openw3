/**
* Copyright (c) 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef __RESOURCE_USAGE_DATA_BASE__H___
#define __RESOURCE_USAGE_DATA_BASE__H___

#include "../../common/core/resourceUsage.h"
#include "jsonFileHelper.h"

// generalized resource usage (placed resource)
struct CResourceUsageEntry
{
	typedef TStaticArray< CName, 6 >	TFlags;

	Vector		m_position;					// always valid
	TFlags		m_layerFlags;				// always saved
	TFlags		m_componentFlags;			// always saved
	TFlags		m_entityFlags;				// always saved
	Box			m_box;						// saved in valid
	CName		m_layerName;				// always saved
	CName		m_entityName;				// saved if valid
	CName		m_entityClass;				// saved if valid
	CName		m_componentClass;			// saved if valid
	CName		m_componentName;			// saved if valid
	Float		m_visibilityRangeMin;		// saved if valid (!= 0.0f)
	Float		m_visibilityRangeMax;		// saved if valid (!= FLT_MAX)
	StringAnsi	m_template;					// source template

	CResourceUsageEntry();

	RED_INLINE const Uint32 CalcRuntimeHash() const
	{		
		Uint32 hash = RED_FNV_OFFSET_BASIS32;
		hash = Red::System::CalculateHash32( &m_position, sizeof(m_position), hash );
		return hash;
	}

	static Bool Parse( const JSONFileHelper::JSONValue& val, CResourceUsageEntry& ret );
	void Write(JSONFileHelper::JSONWriter& writer) const;
};

// generalized resource usage cost (for graphical resources)
struct CResourceUsageCost
{
	CName		m_name;  // not saved if empty
	Uint32		m_memorySizeCPU; // not saved if 0
	Uint32		m_memorySizeGPU; // not saved if 0
	Uint32		m_triangleCount; // not saved if 0
	Uint32		m_vertexCount;  // not saved if 0
	Float		m_visibilityRangeMin;  // not saved if 0.0f
	Float		m_visibilityRangeMax; // not saved if FLT_MAX

	CResourceUsageCost();

	static Bool Parse( const JSONFileHelper::JSONValue& val, CResourceUsageCost& ret );
	void Write(JSONFileHelper::JSONWriter& writer) const;
};

struct CResourceUsageFileInfo
{
	StringAnsi		m_depotPath;
	Float			m_autoHideDistance; // not saved if FLT_MAX

	typedef TStaticArray< CResourceUsageCost, 6 >	TCostData;
	typedef TDynArray< CResourceUsageEntry >		TEntries;

	TEntries	m_entries;	
	TCostData	m_cost;

	CResourceUsageFileInfo();

	static CResourceUsageFileInfo* Parse( const JSONFileHelper::JSONValue& val );
	void Write(JSONFileHelper::JSONWriter& writer) const;
};

/// container for resource usage data
class CResourceUsageDataBase : public JSONFileHelper
{
public:
	CResourceUsageDataBase();
	~CResourceUsageDataBase();

	void Reset();

	// add cost entry to DB
	void AddCost( const String& resourceDepotPath, const Float autoHideDistance, const CResourceUsageCost& costInfo );

	// add usage entry to DB
	void AddUsage( const String& resourceDepotPath, const CResourceUsageEntry& usageInfo );

	// load/save data base
	Bool LoadFromFile( const String& absolutePath );
	Bool SaveToFile( const String& absolutePath ) const;

private:
	typedef TDynArray< CResourceUsageFileInfo* >			TResources;
	typedef THashMap< StringAnsi, CResourceUsageFileInfo* >	TResourcesMap;

	TResources		m_resources; // directly saved (for performance)
	TResourcesMap	m_resourceMap; // not saved, regenerated

	CResourceUsageFileInfo* GetFileEntry( const String& depotPath );
	static void ConformPath( const String& depotPath, StringAnsi& outDepotPath );
};

#endif