/**
* Copyright (c) 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "resourceUsageDataBase.h"
#include "../../common/core/fileSys.h"
#include "../../common/core/dependencyLoader.h"
#include "../../common/core/dependencySaver.h"

//---

CResourceUsageEntry::CResourceUsageEntry()
	: m_position(0,0,0)
	, m_box( Box::EMPTY )
	, m_visibilityRangeMin( 0.0f )
	, m_visibilityRangeMax( FLT_MAX )
{
}

Bool CResourceUsageEntry::Parse( const JSONFileHelper::JSONValue& val, CResourceUsageEntry& ret )
{
	if ( !val.IsObject() )
		return false;

	ret.m_position.X = JSONFileHelper::GetAttrFloat( val, "x" );
	ret.m_position.Y = JSONFileHelper::GetAttrFloat( val, "y" );
	ret.m_position.Z = JSONFileHelper::GetAttrFloat( val, "z" );
	ret.m_layerFlags = JSONFileHelper::GetAttrNameArray( val, "layerFlags" );
	ret.m_layerName = JSONFileHelper::GetAttrName( val, "layerName" );
	ret.m_entityFlags = JSONFileHelper::GetAttrNameArray( val, "entityFlags" );
	ret.m_entityClass = JSONFileHelper::GetAttrName( val, "entityClass" );
	ret.m_entityName = JSONFileHelper::GetAttrName( val, "entityName" );
	ret.m_componentFlags = JSONFileHelper::GetAttrNameArray( val, "componentFlags" );
	ret.m_componentClass = JSONFileHelper::GetAttrName( val, "componentClass" );
	ret.m_componentName = JSONFileHelper::GetAttrName( val, "componentName" );
	ret.m_box.Min.X = JSONFileHelper::GetAttrFloat( val, "minX" );
	ret.m_box.Min.Y = JSONFileHelper::GetAttrFloat( val, "minY" );
	ret.m_box.Min.Z = JSONFileHelper::GetAttrFloat( val, "minZ" );
	ret.m_box.Max.X = JSONFileHelper::GetAttrFloat( val, "maxX" );
	ret.m_box.Max.Y = JSONFileHelper::GetAttrFloat( val, "maxY" );
	ret.m_box.Max.Z = JSONFileHelper::GetAttrFloat( val, "maxZ" );
	ret.m_visibilityRangeMin = JSONFileHelper::GetAttrFloat( val, "visibilityRangeMin" );
	ret.m_visibilityRangeMax = JSONFileHelper::GetAttrFloat( val, "visibilityRangeMax" );
	ret.m_template = JSONFileHelper::GetAttrStr( val, "template" );


	return true;
}

void CResourceUsageEntry::Write(JSONFileHelper::JSONWriter& writer) const
{
	writer.StartObject();

	if ( m_position != Vector::ZEROS )
	{
		writer.String( "x" );
		writer.Double( m_position.X );
		writer.String( "y" );
		writer.Double( m_position.Y );
		writer.String( "z" );
		writer.Double( m_position.Z );
	}

	if ( !m_box.IsEmpty() )
	{
		writer.String( "minX" );
		writer.Double( m_box.Min.X );
		writer.String( "minY" );
		writer.Double( m_box.Min.Y );
		writer.String( "minZ" );
		writer.Double( m_box.Min.Z );
		writer.String( "maxX" );
		writer.Double( m_box.Max.X );
		writer.String( "maxY" );
		writer.Double( m_box.Max.Y );
		writer.String( "maxZ" );
		writer.Double( m_box.Max.Z );
	}

	if ( m_visibilityRangeMin > 0.0f )
	{
		writer.String( "visibilityRangeMin" );
		writer.Double( m_visibilityRangeMin );
	}

	if ( m_visibilityRangeMax != FLT_MAX )
	{
		writer.String( "visibilityRangeMax" );
		writer.Double( m_visibilityRangeMax );
	}

	if ( m_layerName )
	{
		writer.String( "layerName" );
		writer.String( UNICODE_TO_ANSI( m_layerName.AsChar() ) );
	}

	if ( !m_layerFlags.Empty() )
	{
		writer.String( "layerFlags" );

		writer.StartArray();
		for ( Uint32 i=0; i<m_layerFlags.Size(); ++i )
		{
			writer.String( UNICODE_TO_ANSI( m_layerFlags[i].AsChar() ) );
		}
		writer.EndArray();
	}

	if ( m_entityClass )
	{
		writer.String( "entityClass" );
		writer.String( UNICODE_TO_ANSI( m_entityClass.AsChar() ) );
	}

	if ( m_entityName )
	{
		writer.String( "entityName" );
		writer.String( UNICODE_TO_ANSI( m_entityName.AsChar() ) );
	}

	if ( !m_entityFlags.Empty() )
	{
		writer.String( "entityFlags" );

		writer.StartArray();
		for ( Uint32 i=0; i<m_entityFlags.Size(); ++i )
		{
			writer.String( UNICODE_TO_ANSI( m_entityFlags[i].AsChar() ) );
		}
		writer.EndArray();
	}

	if ( m_componentClass )
	{
		writer.String( "componentClass" );
		writer.String( UNICODE_TO_ANSI( m_componentClass.AsChar() ) );
	}

	if ( m_componentName )
	{
		writer.String( "componentName" );
		writer.String( UNICODE_TO_ANSI( m_componentName.AsChar() ) );
	}

	if ( !m_componentFlags.Empty() )
	{
		writer.String( "componentFlags" );

		writer.StartArray();
		for ( Uint32 i=0; i<m_componentFlags.Size(); ++i )
		{
			writer.String( UNICODE_TO_ANSI( m_componentFlags[i].AsChar() ) );
		}
		writer.EndArray();
	}

	if ( !m_template.Empty() )
	{
		writer.String( "template" );
		writer.String( m_template.AsChar() );
	}

	writer.EndObject();
}

//---

CResourceUsageCost::CResourceUsageCost()
	: m_memorySizeCPU( 0 )
	, m_memorySizeGPU( 0 )
	, m_triangleCount( 0 )
	, m_vertexCount( 0 )
	, m_visibilityRangeMin(0.0f)
	, m_visibilityRangeMax(FLT_MAX)
{}

Bool CResourceUsageCost::Parse( const JSONFileHelper::JSONValue& val, CResourceUsageCost& ret )
{
	if ( !val.IsObject() )
		return false;

	ret.m_name = JSONFileHelper::GetAttrName( val, "name" );
	ret.m_memorySizeCPU = JSONFileHelper::GetAttrInt( val, "memorySizeCPU" );
	ret.m_memorySizeGPU = JSONFileHelper::GetAttrInt( val, "memorySizeGPU" );
	ret.m_triangleCount = JSONFileHelper::GetAttrInt( val, "triangleCount" );
	ret.m_vertexCount = JSONFileHelper::GetAttrInt( val, "vertexCount" );
	ret.m_visibilityRangeMin = JSONFileHelper::GetAttrFloat( val, "visibilityRangeMin", 0.0f );
	ret.m_visibilityRangeMax = JSONFileHelper::GetAttrFloat( val, "visibilityRangeMax", FLT_MAX );

	return true;
}

void CResourceUsageCost::Write(JSONFileHelper::JSONWriter& writer) const
{
	writer.StartObject();

	if ( m_name )
	{
		writer.String( "name" );
		writer.String( m_name.AsAnsiChar() );
	}

	if ( m_memorySizeCPU )
	{
		writer.String( "memorySizeCPU" );
		writer.Int( m_memorySizeCPU );
	}

	if ( m_memorySizeGPU )
	{
		writer.String( "memorySizeGPU" );
		writer.Int( m_memorySizeGPU );
	}

	if ( m_triangleCount )
	{
		writer.String( "triangleCount" );
		writer.Int( m_triangleCount );
	}

	if ( m_vertexCount )
	{
		writer.String( "vertexCount" );
		writer.Int( m_vertexCount );
	}

	if ( m_visibilityRangeMin >= 0.0f )
	{
		writer.String( "visibilityRangeMin" );
		writer.Double( m_visibilityRangeMin );
	}

	if ( m_visibilityRangeMax != FLT_MAX )
	{
		writer.String( "visibilityRangeMax" );
		writer.Double( m_visibilityRangeMax );
	}

	writer.EndObject();
}

//---

CResourceUsageFileInfo::CResourceUsageFileInfo()
	: m_autoHideDistance( FLT_MAX )
{
}

CResourceUsageFileInfo* CResourceUsageFileInfo::Parse( const JSONFileHelper::JSONValue& val )
{
	if ( !val.IsObject() )
		return nullptr;

	const StringAnsi path = JSONFileHelper::GetAttrStr( val, "path" );
	if ( path.Empty() )
		return nullptr;

	Red::TUniquePtr< CResourceUsageFileInfo > entry( new CResourceUsageFileInfo() );
	entry->m_depotPath = path;
	entry->m_autoHideDistance = JSONFileHelper::GetAttrFloat( val, "autoHideDistance" );

	// parse cost array
	const JSONFileHelper::JSONValue& costArray = val["cost"];
	if ( costArray.IsArray() )
	{
		//entry->m_cost.Reserve( costArray.Size() );

		for ( auto it = costArray.Begin(); it != costArray.End(); ++it )
		{
			CResourceUsageCost constInfo;
			if ( CResourceUsageCost::Parse( *it, constInfo ) )
			{
				entry->m_cost.PushBack( constInfo );
			}
		}
	}

	// parse usage list
	const JSONFileHelper::JSONValue& usageArray = val["usage"];
	if ( usageArray.IsArray() )
	{
		entry->m_entries.Reserve( usageArray.Size() );

		for ( auto it = usageArray.Begin(); it != usageArray.End(); ++it )
		{
			CResourceUsageEntry usageInfo;
			if ( CResourceUsageEntry::Parse( *it, usageInfo ) )
			{
				entry->m_entries.PushBack( usageInfo );
			}
		}
	}

	// return resource info
	return entry.Release();
}

void CResourceUsageFileInfo::Write(JSONFileHelper::JSONWriter& writer) const
{
	writer.StartObject();

	{
		writer.String( "path" );
		writer.String( m_depotPath.AsChar() );
	}

	if ( m_autoHideDistance != FLT_MAX )
	{
		writer.String( "autoHideDistance" );
		writer.Double( m_autoHideDistance );
	}

	// cost of using the resource
	if ( !m_cost.Empty() )
	{
		writer.String( "cost" );

		writer.StartArray();
		for ( Uint32 i=0; i<m_cost.Size(); ++i )
		{
			m_cost[i].Write( writer );
		}
		writer.EndArray();
	}

	// places where resource is used
	if ( !m_entries.Empty() )
	{
		writer.String( "usage" );

		writer.StartArray();
		for ( Uint32 i=0; i<m_entries.Size(); ++i )
		{
			m_entries[i].Write( writer );
		}
		writer.EndArray();
	}

	writer.EndObject();
}

//---

CResourceUsageDataBase::CResourceUsageDataBase()
{
}

CResourceUsageDataBase::~CResourceUsageDataBase()
{
	Reset();
}

void CResourceUsageDataBase::Reset()
{
	m_resources.ClearPtr();
}

void CResourceUsageDataBase::ConformPath( const String& depotPath, StringAnsi& outDepotPath )
{
	outDepotPath = UNICODE_TO_ANSI( depotPath.ToLower().AsChar() );
}

CResourceUsageFileInfo* CResourceUsageDataBase::GetFileEntry( const String& depotPath )
{
	StringAnsi safePath;
	ConformPath( depotPath, safePath );

	// find existing entry or create new one
	CResourceUsageFileInfo* info = nullptr;
	if ( !m_resourceMap.Find( safePath, info ) )
	{
		info = new CResourceUsageFileInfo();
		info->m_depotPath = safePath;

		m_resources.PushBack( info );
		m_resourceMap.Set( safePath, info );
	}

	return info;
}

void CResourceUsageDataBase::AddCost( const String& resourceDepotPath, const Float autoHideDistance, const CResourceUsageCost& costInfo )
{
	CResourceUsageFileInfo* entry = GetFileEntry( resourceDepotPath );

	for ( Uint32 i=0; i<entry->m_cost.Size(); ++i )
	{
		if ( entry->m_cost[i].m_name == costInfo.m_name )
		{
			entry->m_autoHideDistance = Red::Math::NumericalUtils::Min( autoHideDistance, entry->m_autoHideDistance );
			return;
		}
	}

	entry->m_autoHideDistance = autoHideDistance;
	entry->m_cost.PushBack( costInfo );
}

void CResourceUsageDataBase::AddUsage( const String& resourceDepotPath, const CResourceUsageEntry& usageInfo )
{
	CResourceUsageFileInfo* entry = GetFileEntry( resourceDepotPath );
	entry->m_entries.PushBack( usageInfo );
}

Bool CResourceUsageDataBase::LoadFromFile( const String& absoluteFilePath )
{
	CTimeCounter timer;

	Reset();

	// load the file data
	LOG_WCC( TXT("Loading resource usage data base...") );
	TDynArray< Uint8 > fileData;
	if ( !GFileManager->LoadFileToBuffer( absoluteFilePath, fileData, true ) ) // JSON parser needs the NULL at the end
		return false;

	// parse JSON file
	LOG_WCC( TXT("Parsing resource usage data base...") );
	JSONDocument d;
	d.Parse<0>((AnsiChar*) fileData.Data());

	// errors, cooker data base is invalid, manual deletion is required
	if ( d.HasParseError() )
	{
		ERR_WCC( TXT("JSON parsing error: '%s'"), d.GetParseError() );
		ERR_WCC( TXT("Resource usage data base is corrupted!") );
		return false;
	}

	// parse the resource entries
	const JSONValue& db = d["files"];
	const rapidjson::SizeType numFiles = db.Size();
	Uint32 numUsageEntries = 0;
	for( rapidjson::SizeType i = 0; i<numFiles; ++i )
	{
		const JSONValue& fileEntry = db[ i ];

		CResourceUsageFileInfo* entry = CResourceUsageFileInfo::Parse( fileEntry );
		if ( entry )
		{
			m_resourceMap[ entry->m_depotPath ] = entry;
			m_resources.PushBack( entry );

			numUsageEntries += entry->m_entries.Size();
		}
	}

	// info
	LOG_WCC( TXT("Loaded %d resource entries and %d usage entries from db '%ls'"),
		m_resources.Size(), numUsageEntries, absoluteFilePath.AsChar() );

	return true;
}

Bool CResourceUsageDataBase::SaveToFile( const String& absoluteFilePath ) const
{
	CTimeCounter timer;

	// open the output file
	IFile* outputFile = GFileManager->CreateFileWriter( absoluteFilePath, FOF_AbsolutePath );
	if ( !outputFile )
		return false;

	// build the output JSON file
	{
		typedef JSONStreamWriter< 4096 > TFileStream;

		TFileStream fileStream( outputFile );
		rapidjson::PrettyWriter< TFileStream > writer(fileStream);

		writer.StartObject();
		{
			writer.String("files");
			writer.StartArray();

			for ( Uint32 i=0; i<m_resources.Size(); ++i )
			{
				const CResourceUsageFileInfo* entry = m_resources[i];
				entry->Write( writer );
			}

			writer.EndArray();
		}
		writer.EndObject();
	}

	// stats
	LOG_WCC( TXT("Resource usage data base saved in %1.2fs (%d entries, %1.2f MB)"),
		timer.GetTimePeriod(), m_resources.Size(), outputFile->GetSize() / (1024.0f*1024.0f) );

	delete outputFile;
	return true;
}

//---
