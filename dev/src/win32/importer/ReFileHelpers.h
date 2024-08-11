#pragma once

#include "..\..\common\core\importer.h"
#include "re_archive/include/reFileNodes.h"
#include "re_archive/include/reFileArchive.h"
#include "re_archive/include/reFileBaseNode.h"

namespace CReFileHelpers
{
	Bool		ShouldImportFile( IImporter::ImportOptions& options );
};