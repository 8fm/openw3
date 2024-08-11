/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class CPathLibWorld;

#include "pathlibConfiguration.h"

namespace PathLib
{
/*

class CBundler : public Red::System::NonCopyable
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_PathLib );

protected:
	// bundle grid representation
	struct SGridCel
	{
		TDynArray< Red::Core::BundleDefinition::SBundleFileDesc >		m_files;

		void FeedBundler( Red::Core::BundleDefinition::CBundleDefinitionWriter* writer, const StringAnsi& bundleName );
	};

	struct SGrid
	{
		TDynArray< SGridCel >									m_gridCels;
		Uint32													m_rows;
		Uint32													m_cols;

		void FeedBundler( Red::Core::BundleDefinition::CBundleDefinitionWriter* writer, CDirectory* dir );

		Uint32 GridCelIndex( Uint32 x, Uint32 y )								{ ASSERT( x < m_cols && y < m_rows ); return y * m_cols + x; }
	};

	typedef TDynArray< Red::Core::BundleDefinition::SBundleFileDesc* > BundleFileDescriptions;
	////////////////////////////////////////////////////////////////////////////

	CPathLibWorld&											m_pathlib;
	PathLib::CPathLibConfiguration*							m_configurationSaver;
	CDirectory*												m_sourceDir;
	CDirectory*												m_cookedDir;
	Red::Core::BundleDefinition::CBundleDefinitionWriter*	m_bundleDefinitionWriter;
	SGrid													m_grid;

	template < class ResType >
	Bool SetupBundleFileDescription( Red::Core::BundleDefinition::SBundleFileDesc& outFileDesc, CDirectory* dir, AreaId areaId, const Red::Core::ResourceManagement::ECompressionType compressionType ) const;

	Bool InitConfigurationFile();
	Bool SaveConfigurationFile();
	void CollectAreaFilesForGridCel( CAreaDescription* area, SGridCel& cel );
	Bool ConfigureBundleGrid();
	Bool FeedConfigurationWithGridData();
	Bool CreateBundleDefinitionWriter();

public:
	CBundler( CPathLibWorld& pathlib, Red::Core::ResourceManagement::CResourceManager& resourceManager );
	~CBundler();

	Bool Cook();

};
*/

};			// namespace PathLib

