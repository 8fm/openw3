/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "fbxCommon.h"
#include "../../common/core/exporter.h"
#include "../../common/engine/mesh.h"
#include "../../common/engine/meshComponent.h"
#include "../../common/engine/appearanceComponent.h"

class CFBXEntityExporter : public IExporter
{
	DECLARE_ENGINE_CLASS( CFBXEntityExporter, IExporter, 0 );

public:
	CFBXEntityExporter();

	// Export resource
	virtual Bool DoExport( const ExportOptions& options );
};

BEGIN_CLASS_RTTI( CFBXEntityExporter );
	PARENT_CLASS( IExporter );
END_CLASS_RTTI();

IMPLEMENT_ENGINE_CLASS( CFBXEntityExporter );

//-----------------------------------------------------------------------------

CFBXEntityExporter::CFBXEntityExporter()
{
	m_resourceClass = ClassID<CEntityTemplate>();
	FBXExportScene::FillExportFormat( m_formats );
}

Bool CFBXEntityExporter::DoExport( const ExportOptions& options )
{
	FBXExportScene exportScene(options.m_saveFilePath);

	const CEntityTemplate* srcTemplate = static_cast<const CEntityTemplate*>( options.m_resource );
	CEntity* srcEntity = Cast<CEntity>( srcTemplate->CreateTemplateInstance( nullptr ) );

	if (!srcEntity)
	{
		return false;
	}

	SEntityStreamingState state;
	srcEntity->PrepareStreamingComponentsEnumeration( state, false, SWN_DoNotNotifyWorld );
	srcEntity->ForceFinishAsyncResourceLoads();

	CAppearanceComponent* appCmp = CAppearanceComponent::GetAppearanceComponent( srcEntity );
	const TDynArray< CEntityAppearance >& appearance = srcTemplate->GetAppearances();
	Uint32 appearancesSize = appearance.Size();

	if( appCmp )
	{
		for ( Uint32 i=0; i<appearancesSize; ++i )
		{
			const CEntityAppearance& currentApp = appearance[i];
			appCmp->ApplyAppearance(currentApp);
		}
	}

	// Find animated component
	const CAnimatedComponent* ac = nullptr;
	const TDynArray< CComponent* >& entComponents = srcEntity->GetComponents();
	const Uint32 numComponents = entComponents.Size();
	for ( Uint32 i=0; i<numComponents; ++i)
	{
		const CComponent* c = entComponents[i];
		if ( const CAnimatedComponent* animCmp = Cast< CAnimatedComponent >( c ) )
		{
			ac = animCmp;
			break;
		}
	}

	// Export skeleton
	Bool hasSkeleton = false;
	if ( ac && ac->GetSkeleton() )
	{
		exportScene.ExportSkeleton( ac->GetSkeleton() );
		hasSkeleton = true;
	}

	// Export meshes
	for ( Uint32 i=0; i<numComponents; ++i)
	{
		const CComponent* c = entComponents[i];

		if ( const CMeshComponent* mc = Cast< CMeshComponent >( c ) )
		{
			if ( !mc->IsVisible() )
			{
				continue;
			}

			// filter skinned components only
			if ( hasSkeleton )
			{
				if ( mc->GetTransformParent() && !mc->GetTransformParent()->ToSkinningAttachment() )
				{
					continue;
				}

				if ( mc->GetTransformParent()->GetParent() != ac )
				{
					continue;
				}
			}

			// Export mesh
			if ( const CMesh* mesh = mc->GetMeshNow() )
			{
				if ( hasSkeleton )
				{
					exportScene.ExportMesh(Matrix::IDENTITY, true, mesh, mc->GetName() );
				}
				else
				{
					const Matrix& nodeToWorld = mc->GetLocalToWorld();
					exportScene.ExportMesh( nodeToWorld, false, mesh, mc->GetName() );
				}
			}
		}
	}

	srcEntity->FinishStreamingComponentsEnumeration( state );

	EFBXFileVersion ver = FBXExportScene::GetFBXExportVersion(options);

	// Save the scene
	return exportScene.Save( options.m_saveFilePath, ver );
}

//-----------------------------------------------------------------------------
