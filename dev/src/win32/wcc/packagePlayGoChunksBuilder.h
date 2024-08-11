/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "build.h"

#include "../../common/redSystem/utility.h"

namespace PackageHelpers
{
	struct SPackageChunk
	{
		String m_label;
		String m_rawPlayGoLanguages;
		Uint32 m_chunkID;
		Uint32 m_layer;
		
		SPackageChunk()
			: m_chunkID(0xFFFFFFFF)
			, m_layer(0)
		{}
	};

	struct SPackageScenario
	{
		String				m_label;
		Uint32				m_scenarioID;
		Uint32				m_numInitialChunks;
		TDynArray< Uint32 > m_installOrderChunkIDs;

		SPackageScenario()
			: m_scenarioID(0)
			, m_numInitialChunks(0)
		{}
	};

	struct SPackageInfo
	{
		String						m_rawSupportedPlayGoLanguages;
		String						m_rawDefaultPlayGoLanguage;
		SPackageScenario			m_scenario;
		TDynArray< SPackageChunk >	m_chunks;
	};

	class CPackagePlayGoChunksBuilder : private Red::System::NonCopyable
	{
		DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

	private:
		CXMLReader&							m_xmlReader;

	public:
		CPackagePlayGoChunksBuilder( CXMLReader& xmlReader );
		~CPackagePlayGoChunksBuilder() {}

	public:
		Bool ParsePackage( SPackageInfo& outPackage ) const;

	private:
		Bool ParseProject( CXMLReader& xmlReader, SPackageInfo& outPackage ) const;
		Bool ParseVolume( CXMLReader& xmlReader, SPackageInfo& outPackage ) const;
		Bool ParseChunkInfo( CXMLReader& xmlReader, SPackageInfo& outPackage ) const;
		Bool ParseChunks( CXMLReader& xmlReader, SPackageInfo& outPackage ) const;
		Bool ParseChunk( CXMLReader& xmlReader, SPackageChunk& outChunk ) const;
		Bool ParseScenarios( CXMLReader& xmlReader, SPackageInfo& outPackage ) const;
		Bool ParseScenario( CXMLReader& xmlReader, SPackageScenario& outScenario ) const;

	private:
		Bool ParsePlayGoIDRange( const String& src, TDynArray< Uint32 >& outIDs) const;
		Bool ParsePlayGoLanguages( const String& src, Uint64& outLangauges ) const;
	};

}
