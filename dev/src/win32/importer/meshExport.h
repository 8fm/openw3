#pragma once

#include "../../common/engine/mesh.h"
#include "re_archive/include/reFileArchive.h"

namespace MeshReExport
{
	void ExportLODToRE( ReFileArchive& archive, Int32 lodIdx, const CMesh::LODLevel& lod, const CMesh& mesh );
}
