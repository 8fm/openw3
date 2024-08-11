/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "pathlibBundler.h"

#if 0
#include "pathlibConfiguration.h"
#include "pathlibNavmeshArea.h"
#include "pathlibTerrain.h"
#include "pathlibObstaclesMap.h"
#include "pathlibStreamingManager.h"


namespace PathLib
{

////////////////////////////////////////////////////////////////////////////
// CBundler::SGridCel
////////////////////////////////////////////////////////////////////////////
void CBundler::SGridCel::FeedBundler( Red::Core::BundleDefinition::CBundleDefinitionWriter* writer, const StringAnsi& bundleName )
{
	writer->AddBundle( bundleName );

	for( Uint32 i = 0; i < m_files.Size(); ++i )
	{
		writer->AddBundleFileDesc( bundleName, m_files[i] );
	}
}

////////////////////////////////////////////////////////////////////////////
// CBundler::SGrid
////////////////////////////////////////////////////////////////////////////
void CBundler::SGrid::FeedBundler( Red::Core::BundleDefinition::CBundleDefinitionWriter* writer, CDirectory* dir )
{
	Uint32 i = 0;
	StringAnsi bundleName;
	//StringAnsi path = TXT_TO_ANSI( dir->GetDepotPath().AsChar() );
	for ( Uint32 y = 0; y < m_rows; ++y )
	{
		for ( Uint32 x = 0; x < m_cols; ++x )
		{
			auto& cel = m_gridCels[ i++ ];
			bundleName = CStreamingManager::ComputeBundleNameAnsi( x, y );
			cel.FeedBundler( writer, bundleName );
		}
	}
}


////////////////////////////////////////////////////////////////////////////
// CBundler
////////////////////////////////////////////////////////////////////////////
Bool CBundler::InitConfigurationFile()
{
	auto& saver = m_configurationSaver;
	if ( !saver.Init( m_sourceDir ) )
	{
		return false;
	}
	PathLib::CPathLibConfiguration* configuration = saver.Get();

	configuration->SetTerrainInfo( m_pathlib.GetTerrainInfo() );

	const auto& instanceAreasList = m_pathlib.m_instanceAreas;
	const auto& terrainAreasList = m_pathlib.m_terrainAreas;

	configuration->Reserve( instanceAreasList.Size() + terrainAreasList.Size() );
	for ( auto it = instanceAreasList.Begin(), end = instanceAreasList.End(); it != end; ++it )
	{
		configuration->AddArea( it->m_area, m_resourceManager );
	}
	for ( auto it = terrainAreasList.Begin(), end = terrainAreasList.End(); it != end; ++it )
	{
		configuration->AddArea( *it, m_resourceManager );
	}

	return true;
}

Bool CBundler::SaveConfigurationFile()
{
	return m_configurationSaver.Save();
}
Bool CBundler::CreateBundleDefinitionWriter()
{
	StringAnsi filePath = UNICODE_TO_ANSI( m_sourceDir->GetAbsolutePath().AsChar() );
	filePath += "bundleDef.json";
	m_bundleDefinitionWriter = new Red::Core::BundleDefinition::CBundleDefinitionWriter( filePath.AsChar() );
	return true;
}

template < class ResType >
Bool CBundler::SetupBundleFileDescription( Red::Core::BundleDefinition::SBundleFileDesc& outFileDesc, CDirectory* dir, AreaId areaId, const Red::Core::ResourceManagement::ECompressionType compressionType ) const
{
	String fileName;
	m_pathlib.GetGenericFileName( areaId, fileName, ResType::GetFileExtension() );
	String depotPath = dir->GetDepotPath() + fileName;

	Red::Core::ResourceManagement::CResourceLoadRequest request( depotPath, ResType::GetFourCC() );

	Red::Core::ResourceManagement::CResourceHandle handle = m_resourceManager.ImmediateLoadResource( request );

	ResType* res = handle.Get< ResType >();
	if ( !res )
	{
		return false;
	}

	// Create a reader for this file, so we can tell how large it is.
	IFile* reader = GFileManager->CreateFileReader( dir->GetAbsolutePath() + fileName, FOF_AbsolutePath );

	// Fill out the bundle file description object
	outFileDesc.m_resourceId = handle.GetResourceID();
	outFileDesc.m_fourCC = ResType::GetFourCC();
	outFileDesc.m_cookedResourcePath = StringAnsi::EMPTY;
	outFileDesc.m_fileSize = Uint32( reader->GetSize() );
	outFileDesc.m_rawResourcePath = StringAnsi( UNICODE_TO_ANSI( depotPath.AsChar() ) );
	outFileDesc.m_compressionType = compressionType;

	// Clean up memory allocation.
	delete reader;

	return true;
}

void CBundler::CollectAreaFilesForGridCel( CAreaDescription* area, SGridCel& cel )
{
	String fileName;

	AreaId areaId = area->GetId();
	Bool isNavmeshArea = (areaId & PathLib::CAreaDescription::ID_MASK_TERRAIN) == 0;
	{
		Red::Core::BundleDefinition::SBundleFileDesc fileDesc;
		if ( SetupBundleFileDescription< CAreaNavgraphsRes >( fileDesc, m_sourceDir, areaId, Red::Core::ResourceManagement::CT_Uncompressed ) )
		{
			cel.m_files.PushBack( fileDesc );
		}
	}
	{
		Red::Core::BundleDefinition::SBundleFileDesc fileDesc;

		if ( SetupBundleFileDescription< CObstaclesMap >( fileDesc, m_sourceDir, areaId, Red::Core::ResourceManagement::CT_Uncompressed ) )
		{
			cel.m_files.PushBack( fileDesc );
		}
	}
	if ( isNavmeshArea )
	{
		Red::Core::BundleDefinition::SBundleFileDesc fileDesc;

		if ( SetupBundleFileDescription< CNavmeshRes >( fileDesc, m_sourceDir, areaId, Red::Core::ResourceManagement::CT_Uncompressed ) )
		{
			cel.m_files.PushBack( fileDesc );
		}
	}
	if ( !isNavmeshArea )
	{
		Red::Core::BundleDefinition::SBundleFileDesc fileDesc;

		if ( SetupBundleFileDescription< CTerrainMap >( fileDesc, m_sourceDir, areaId, Red::Core::ResourceManagement::CT_Uncompressed ) )
		{
			cel.m_files.PushBack( fileDesc );
		}
	}
}
Bool CBundler::ConfigureBundleGrid()
{
	const auto& terrainInfo = m_pathlib.GetTerrainInfo();
	const auto& instanceMap = *m_pathlib.GetInstanceMap();

	// process tile-by-tile bundles
	Uint32 tilesCount = terrainInfo.GetTilesCount();

	m_grid.m_cols = instanceMap.GetCelsInRow();
	m_grid.m_rows = instanceMap.GetCelsInRow();
	m_grid.m_gridCels.Resize( m_grid.m_cols * m_grid.m_rows );

	for ( AreaId tile = 0; tile < tilesCount; ++tile )
	{
		Int32 x, y;
		terrainInfo.GetTileCoordsFromId( tile, x, y );
		PATHLIB_ASSERT( x < Int32( instanceMap.GetCelsInRow() ) && y < Int32( instanceMap.GetCelsInRow() ) );

		auto& bundlerCel = m_grid.m_gridCels[ m_grid.GridCelIndex( x, y ) ];

		PathLib::CTerrainAreaDescription* terrainArea = m_pathlib.GetTerrainAreaDescription( tile | CAreaDescription::ID_MASK_TERRAIN );
		if ( terrainArea )
		{
			// bundle terrain area
			CollectAreaFilesForGridCel( terrainArea, bundlerCel );
		}

		const PathLib::CInstanceMapCel& mapCel = instanceMap.GetMapCelAt( x, y );
		const auto& instanceList = mapCel.GetInstancesList();
		for ( auto it = instanceList.Begin(), end = instanceList.End(); it != end; ++it )
		{
			PathLib::AreaId id = *it;
			PathLib::CNavmeshAreaDescription* naviArea = m_pathlib.GetInstanceAreaDescription( id );
			if ( naviArea )
			{
				// bundle navi area
				CollectAreaFilesForGridCel( naviArea, bundlerCel );
			}
		}
	}

	return true;
}

Bool CBundler::FeedConfigurationWithGridData()
{
	CPathLibConfiguration* configuration = m_configurationSaver.Get();
	if ( !configuration )
	{
		return false;
	}

	SStreamingGridConfiguration& gridConfiguration = configuration->GetStreamingGridConfiguration();
	gridConfiguration.m_cels.Resize( m_grid.m_gridCels.Size() );

	for ( Uint32 i = 0, celsCount = m_grid.m_gridCels.Size(); i != celsCount; ++i )
	{
		gridConfiguration.m_cels[ i ].m_itemsCount = m_grid.m_gridCels[ i ].m_files.Size();
	}

	return true;
}

CBundler::CBundler( CPathLibWorld& pathlib, Red::Core::ResourceManagement::CResourceManager& resourceManager )
	: m_pathlib( pathlib )
	, m_resourceManager( resourceManager )
	, m_configurationSaver( resourceManager )
	, m_bundleDefinitionWriter( NULL )
{
	m_sourceDir = m_pathlib.GetSourceDataDirectory( false );
	m_cookedDir = NULL;
	//m_cookedDir = m_pathlib.GetCookedDataDirectory( true );
}
CBundler::~CBundler()
{
	if ( m_bundleDefinitionWriter )
	{
		delete m_bundleDefinitionWriter;
	}
}

Bool CBundler::Cook()
{
	if ( !m_sourceDir )
	{
		return false;
	}

	if ( !InitConfigurationFile() )
	{
		return false;
	}

	if ( !ConfigureBundleGrid() )
	{
		return false;
	}

	if ( !FeedConfigurationWithGridData() )
	{
		return false;
	}

	// initialize bundle definition writer
	if ( !CreateBundleDefinitionWriter() )
	{
		return false;
	}

	// feed definition with bundles
	m_grid.FeedBundler( m_bundleDefinitionWriter, m_sourceDir );

	// save bundle definition
	if ( !m_bundleDefinitionWriter->Write() )
	{
		return false;
	}

	if ( !SaveConfigurationFile() )
	{
		return false;
	}

	return true;
}

};			// namespace PathLib


#endif


