/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "fbxCommon.h"
#include "../../common/core/exporter.h"
#include "../../common/engine/mesh.h"

class CFBXMeshExporter : public IExporter
{
	DECLARE_ENGINE_CLASS( CFBXMeshExporter, IExporter, 0 );

public:
	CFBXMeshExporter();

	// Export resource
	virtual Bool DoExport( const ExportOptions& options );
};

BEGIN_CLASS_RTTI( CFBXMeshExporter );
PARENT_CLASS( IExporter );
END_CLASS_RTTI();

IMPLEMENT_ENGINE_CLASS( CFBXMeshExporter );

//-----------------------------------------------------------------------------

CFBXMeshExporter::CFBXMeshExporter()
{
	m_resourceClass = ClassID<CMesh>();
	FBXExportScene::FillExportFormat( m_formats );
}

Bool CFBXMeshExporter::DoExport( const ExportOptions& options )
{
	FBXExportScene exportScene(options.m_saveFilePath);

	// Export bones
	const CMesh* srcModel = Cast< CMesh >( options.m_resource );
	exportScene.ExportMesh(Matrix::IDENTITY, true, srcModel, TEXT("Mesh"));

	EFBXFileVersion ver = FBXExportScene::GetFBXExportVersion(options);

	// Save the scene
	return exportScene.Save(options.m_saveFilePath, ver );
}

//-----------------------------------------------------------------------------
