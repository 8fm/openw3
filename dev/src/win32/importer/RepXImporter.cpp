/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../../common/physics/physXEngine.h"
#include "../../common/physics/PhysXStreams.h"
#include "../../common/engine/ragdollPhysX.h"

class CRepXImporter : public IImporter
{
	DECLARE_ENGINE_CLASS( CRepXImporter, IImporter, 0 );

public:
	CRepXImporter();
	virtual CResource* DoImport( const ImportOptions& options );
};

BEGIN_CLASS_RTTI( CRepXImporter )
	PARENT_CLASS(IImporter)
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( CRepXImporter );

CRepXImporter::CRepXImporter()
{
	m_resourceClass = ClassID< CRagdoll >();
	m_formats.PushBack( CFileFormat( TXT("repx"), TXT("PhysX RepX resource") ) );
}

CResource* CRepXImporter::DoImport( const ImportOptions& options )
{
#ifndef USE_PHYSX
	return NULL;
#else
	IFile* file = GFileManager->CreateFileReader( options.m_sourceFilePath, FOF_Buffered | FOF_AbsolutePath | FOF_DoNotIOManage );
	if ( !file )
	{
		return NULL;
	}

	RepXImporterParams* params = static_cast< RepXImporterParams* >( options.m_params );

	Uint32 fileSize = static_cast< Uint32 >( file->GetSize() );

	CRagdoll::FactoryInfo buildData;
	buildData.m_parent = options.m_parentObject;
	buildData.m_reuse = Cast< CRagdoll >( options.m_existingResource );
	buildData.m_repxBuffer = DataBuffer( TDataBufferAllocator< MC_PhysxRagdollBuffer >::GetInstance(), fileSize );

	file->Serialize( buildData.m_repxBuffer.GetData(), fileSize );
	delete file;
	return CRagdoll::Create( buildData );
#endif
}
