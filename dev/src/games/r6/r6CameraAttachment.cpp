#include "build.h"
#include "r6CameraAttachment.h"
#include "r6CameraComponent.h"


IMPLEMENT_ENGINE_CLASS( CR6CameraAttachment );


Bool CR6CameraAttachment::Init( CNode* parent, CNode* child, const AttachmentSpawnInfo* info )
{
	if( !parent || !child )
	{
		return false;
	}

	if( child->IsA< CR6CameraComponent >() )
	{
		if( parent->IsA< CR6CameraComponent >() )
		{
			return false;
		}
		// else everything is correct - child is a camera, parent is something else
	}
	else
	{
		if( !parent->IsA< CR6CameraComponent >() )
		{
			return false;
		}
		// else
		Swap( parent, child );
	}

	return TBaseClass::Init(parent, child, info);
}

