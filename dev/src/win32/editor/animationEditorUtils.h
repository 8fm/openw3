
#pragma once

namespace AnimationEditorUtils
{
	void SetActorsItemsVisible( CEntity* entity );
	void SetActorsMeshRepresentation( CEntity* entity );

	CAnimatedComponent* CloneEntity( const CAnimatedComponent* component );

	Bool SyncComponentsPoses( const CAnimatedComponent* componentSrc, CAnimatedComponent* componentDest );
};
