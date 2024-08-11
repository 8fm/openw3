/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "clothAttachment.h"
#include "../core/dataError.h"
#include "attachment.h"
#include "entity.h"
#include "entityTemplate.h"
#include "component.h"
#include "layer.h"
#include "utils.h"


IMPLEMENT_ENGINE_CLASS( CClothAttachment );


CClothAttachment::CClothAttachment()
{
}

CClothAttachment::~CClothAttachment()
{
}

Bool CClothAttachment::Init( CNode* parent, CNode* child, const AttachmentSpawnInfo* info )
{
	Bool result = TBaseClass::Init( parent, child, info );

	ReportDeprecated();

	return result;
}

void CClothAttachment::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );

	if ( !file.IsGarbageCollector() )
	{
		ReportDeprecated();
	}
}


void CClothAttachment::ReportDeprecated()
{
	// Try to get some extra information about where the attachment is being used.
	if ( m_parent )
	{
		CEntityTemplate* entTemplate = nullptr;
		CEntity* ent = nullptr;
		ent = m_parent->AsEntity();
		if ( !ent )
		{
			ent = m_parent->AsComponent()->GetEntity();
		}
		if ( ent )
		{
			entTemplate = ent->GetEntityTemplate();
			if ( entTemplate )
			{
				DATA_HALT( DES_Minor, entTemplate, TXT("Deprecated"), TXT("Entity '%ls' in '%ls' using deprecated CClothAttachment. Use CMeshSkinningAttachment instead."), ent->GetName().AsChar(), entTemplate->GetDepotPath().AsChar() );
			}
			else
			{
				DATA_HALT( DES_Minor, ent->GetLayer(), TXT("Deprecated"), TXT("Entity '%s' is using deprecated CClothAttachment. Use CMeshSkinningAttachment instead."), ent->GetName().AsChar() );
			}

			return;
		}
	}

	// We weren't able to get anything, so just let the user know.
	DATA_HALT( DES_Minor, CResourceObtainer::GetResource( this ), TXT("Deprecated"), TXT("Found deprecated CClothAttachment being used. Use CMeshSkinningAttachment instead.") );
}
