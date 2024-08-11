
#include "build.h"
#include "attachmentUtils.h"
#include "meshSkinningAttachment.h"
#include "meshTypeResource.h"
#include "animatedComponent.h"
#include "meshTypeComponent.h"
#include "entity.h"


void CAttachmentUtils::CreateSkinningAttachment( CEntity* entity, CAnimatedComponent* rootAnimComponent, CMeshTypeComponent*  meshComponent )
{
	// Check root first
	CMeshSkinningAttachment* attToRoot = Cast< CMeshSkinningAttachment >( rootAnimComponent->Attach( meshComponent, ClassID< CMeshSkinningAttachment >() ) );
	if ( attToRoot )
	{
		const Bool isRootAttValid = attToRoot->IsSkinningMappingValid();
		if ( !isRootAttValid )
		{
			// Break invalid attachment
			attToRoot->Break();

			// Find correct animated component
			for ( ComponentIterator< CAnimatedComponent > it( entity ); it; ++it )
			{
				CAnimatedComponent* ac = *it;

				if ( ac != rootAnimComponent )
				{
					CMeshSkinningAttachment* att = SafeCast< CMeshSkinningAttachment >( ac->Attach( meshComponent, ClassID< CMeshSkinningAttachment >() ) );

					if ( att && att->IsSkinningMappingValid() )
					{
						break;
					}
					else if ( att )
					{
						// Break invalid attachment
						att->Break();
					}
				}
			}
		}
	}
}