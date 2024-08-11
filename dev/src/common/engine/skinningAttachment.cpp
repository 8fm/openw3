/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "skinningAttachment.h"
#include "externalProxy.h"

IMPLEMENT_ENGINE_CLASS( CSkinningAttachment );

SkinningAttachmentSpawnInfo::SkinningAttachmentSpawnInfo()
	: HardAttachmentSpawnInfo()
{
	m_attachmentClass = ClassID< CSkinningAttachment >();
}

Bool CSkinningAttachment::Init( CNode* parent, CNode* child, const AttachmentSpawnInfo* info )
{
	// Make sure parent implements ISkeletonDataProvider
	const ISkeletonDataProvider* provider = parent->QuerySkeletonDataProvider();
	if ( !provider )
	{
		// External proxy components may temporarily posess CSkinningAttachments - this is valid
		if ( ! parent->IsA< CExternalProxyComponent >() )
		{
			WARN_ENGINE( TXT("Unable to create skinning attachment because '%ls' does not implement skeleton data provider"), parent->GetName().AsChar() );
		}
		return false;
	}

	// Initialize base attachment
	if ( !TBaseClass::Init( parent, child, info ))
	{
		return false;
	}

	// Created
	return true;
}

void CSkinningAttachment::CalcAttachedLocalToWorld( Matrix& out ) const
{
	// Use parent transform
	ASSERT( GetParent()->IsA< CComponent >() );
	CComponent* tparent = static_cast< CComponent* >( GetParent() );
	out = tparent->GetLocalToWorld();
}

