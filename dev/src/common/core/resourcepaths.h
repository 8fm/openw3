/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef _RED_RESOURCE_PATHS_H_
#define _RED_RESOURCE_PATHS_H_

#include "staticarray.h"

class CDirectory;

namespace Red
{
	namespace Core
	{
		namespace ResourceManagement
		{
			class CResourcePaths
			{
			public:

				enum EPath
				{
					Path_StreamingDirectoryDepot = 0,
					Path_WorkingDirectoryDepot,
					Path_BundlesDirectoryAbsolute,

					Path_WorldMetadataDepot,
					Path_DatabaseDepot,
					Path_DatabaseAbsolute,
					Path_BundleDefinitionFilename,
					Path_QuestMetadataDepot,
					Path_FoliageSourceData,
					Path_LayerBundleDefinitionFilename,
					Path_Max
				};

				enum EDirectory
				{
					Directory_Working = 0,
					Directory_WorkingQuest,

					Directory_Max
				};

			public:
				CResourcePaths();
				RED_MOCKABLE ~CResourcePaths();

				void Initialize( CDirectory* worldDirectory, const String& worldFilename );

				RED_INLINE RED_MOCKABLE const String& GetPath( EPath path ) const { return m_paths[ path ]; }
				
				RED_INLINE CDirectory* GetDirectory( EDirectory directory ) { return m_directories[ directory ]; }
				RED_INLINE const CDirectory* GetDirectory( EDirectory directory ) const { return m_directories[ directory ]; }

			private:
				static const Char* STREAMING_DIRECTORY_NAME;
				static const Char* WORKING_DIRECTORY_NAME;
				static const Char* BUNDLES_DIRECTORY_NAME;
				static const Char* QUEST_WORKING_DIRECTORY_NAME;
				static const Char* SOURCE_FOLIAGE_DIRECTORY_NAME;

				static const Char* METADATA_FILE_EXTENSION;
				static const Char* BUNDLEDEF_FILE_EXTENSION;
				static const Char* DATABASE_FILE_EXTENSION;
				static const Char* QUEST_METADATA_FILE_EXTENSION;

				static const Char* LAYER_DEFINITION_FILENAME_ADDITION;
			private:
				TStaticArray< String, Path_Max > m_paths;
				TStaticArray< CDirectory*, Directory_Max > m_directories;
			};
		}
	}
}

#endif // _RED_RESOURCE_PATHS_H_
