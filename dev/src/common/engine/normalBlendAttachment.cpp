/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "normalBlendAttachment.h"
#include "normalBlendComponent.h"
#include "renderCommands.h"
#include "mesh.h"
#include "../core/feedback.h"
#include "meshTypeResource.h"
#include "renderProxyIterator.h"
#include "renderProxyInterface.h"
#include "materialInstance.h"
#include "meshComponent.h"

IMPLEMENT_ENGINE_CLASS( CNormalBlendAttachment );

NormalBlendAttachmentSpawnInfo::NormalBlendAttachmentSpawnInfo()
	: AttachmentSpawnInfo( ClassID< CNormalBlendAttachment >() )
{
}


CNormalBlendAttachment::CNormalBlendAttachment()
{
}


CNormalBlendAttachment::~CNormalBlendAttachment()
{
}

Bool CNormalBlendAttachment::Init( CNode* parent, CNode* child, const AttachmentSpawnInfo* info )
{
	// Parent must be CNormalBlendComponent
	if ( !parent->IsA< CNormalBlendComponent >() )
	{
		WARN_ENGINE( TXT("Unable to create normal-blend attachment because parent component '%ls' is not a CNormalBlendComponent"), parent->GetName().AsChar() );
		return false;
	}

	// Child must be IRenderProxyInterface
	if ( !child->IsA< CComponent >() || !Cast< CComponent >( child )->QueryRenderProxyInterface() )
	{
		WARN_ENGINE( TXT("Unable to create normal-blend attachment because child component '%ls' is not a IRenderProxyInterface"), child->GetName().AsChar() );
		return false;
	}

	// Parent cannot already have a child (NBComponent only supports a single NBAttachment)
	if ( !parent->GetChildAttachments().Empty() )
	{
		WARN_ENGINE( TXT("Unable to create normal-blend attachment because parent component '%ls' already has a child"), parent->GetName().AsChar() );
		return false;
	}

	// Initialize base attachment
	if ( !TBaseClass::Init( parent, child, info ) )
	{
		return false;
	}

	return true;
}

void CNormalBlendAttachment::SetNormalBlendWeights( Uint32 firstWeight, Uint32 numWeights, const Float* weights )
{
	IRenderProxyInterface* proxyInterface = Cast< CComponent >( GetChild() )->QueryRenderProxyInterface();
	for ( RenderProxyIterator it( proxyInterface ); it; ++it )
	{
		( new CRenderCommand_UpdateNormalBlendWeights( *it, firstWeight, numWeights, weights ) )->Commit();
	}
}

void CNormalBlendAttachment::SetNormalBlendAreas( Uint32 firstArea, Uint32 numAreas, const Vector* areas )
{
	IRenderProxyInterface* proxyInterface = Cast< CComponent >( GetChild() )->QueryRenderProxyInterface();
	for ( RenderProxyIterator it( proxyInterface ); it; ++it )
	{
		( new CRenderCommand_DefineNormalBlendAreas( *it, firstArea, numAreas, areas ) )->Commit();
	}
}

void CNormalBlendAttachment::SetNormalBlendMaterial( CMaterialInstance* material, IMaterial* sourceBaseMaterial, ITexture* sourceNormalTexture )
{
	IRenderProxyInterface* proxyInterface = Cast< CComponent >( GetChild() )->QueryRenderProxyInterface();
	for ( RenderProxyIterator it( proxyInterface ); it; ++it )
	{
		( new CRenderCommand_SetNormalBlendMaterial( *it, material, sourceBaseMaterial, sourceNormalTexture ) )->Commit();
	}
}


void CNormalBlendAttachment::GetMaterials( CMeshTypeResource::TMaterials& materials )
{
	// We can't use Render Proxies, since they don't allow direct queries of materials... so instead we have to do this on a per-component-type basis.
	if ( GetChild()->IsA< CMeshComponent >() )
	{
		CMeshComponent *mc = Cast< CMeshComponent >( GetChild() );
		CMesh* mesh = mc->GetMeshNow();
		if ( mesh != nullptr )
		{
			materials.PushBack( mesh->GetMaterials() );
		}
	}
}
