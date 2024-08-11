#pragma once

#ifndef NO_EDITOR

#include "localizationExporter.h"
#include "storySceneExporter.h"
#include "../../../common/game/storySceneControlPart.h"
#include "../../../common/core/diskFile.h"
#include "../sceneValidator.h"

class CStorySceneValidator
{
public:
	CStorySceneValidator();

	void Initialize();
	void FillRootExportGroup( BatchExportGroup& exportGroup );

	void AddDirectoryToBatchGroup( CDirectory* directory, BatchExportGroup& batchGroup );
	virtual String	GetResourceExtension() const { return TXT( ".w2scene" ); }

protected:
	Bool CanExportFile( CDiskFile* file ) const;

	void FillSceneFiles( const BatchExportGroup& group, TDynArray< String >& files );
protected:

	TDynArray< String >					m_sceneFiles;
	TDynArray< SBrokenSceneGraphInfo >	m_brokenGraphsInfo;
};

#endif //NO_EDITOR