/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "materialOverrideAttachment.h"
#include "materialOverrideProvider.h"
#include "materialOverrideComponent.h"

IMPLEMENT_ENGINE_CLASS( CMaterialOverrideAttachment );
IMPLEMENT_ENGINE_CLASS( CMaterialOverrideAttachmentSelectByChunk );

///////////////////////////////////////////////////////////////////////////////
// CMaterialOverrideAttachment

CMaterialOverrideAttachment::CMaterialOverrideAttachment ()
{
	// empty
}

CMaterialOverrideAttachment::~CMaterialOverrideAttachment ()
{
	// empty
}

Bool CMaterialOverrideAttachment::Init( CNode* parent, CNode* child, const AttachmentSpawnInfo* info )
{
	if ( !parent || !child )
		return false;

	if ( !parent->IsA< CMaterialOverrideComponent >() )
		return false;	

	return TBaseClass::Init( parent, child, info );
}

const CMaterialOverrideComponent* CMaterialOverrideAttachment::GetMaterialOverrideComponent() const
{
	return Cast< CMaterialOverrideComponent >( GetParent() );
}

Bool CMaterialOverrideAttachment::IsConformed( const SMaterialOverrideContext &context )
{
	return true;
}

///////////////////////////////////////////////////////////////////////////////
// CMaterialOverrideAttachmentSelectByChunk

Bool CMaterialOverrideAttachmentSelectByChunk::IsConformed( const SMaterialOverrideContext &context )
{
	return m_chunkIndices.Exist( context.m_chunkIndex );
}
