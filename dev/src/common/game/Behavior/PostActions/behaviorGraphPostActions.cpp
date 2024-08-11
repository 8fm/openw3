#include "build.h"
#include "..\..\build.h" // hack for using precompiled header on PS4 (only the case when you have cpp file in subfolders, hope that engine team will fix this correctly)
#include "behaviorGraphPostActions.h"
#include "..\..\movingAgentComponent.h"
#include "..\..\..\engine\decalEmitter.h"
#include "..\..\commonGame.h"
#include "..\..\..\redSystem\error.h"
#include "..\..\..\engine\visualDebug.h"
#include "..\..\..\engine\game.h"
#include "..\..\..\engine\bitmapTexture.h"

void CFootStepAction::Process( CAnimatedComponent* ac, const SBehaviorGraphOutput& pose )
{
	RED_FATAL_ASSERT( ::SIsMainThread(), "CFootStepAction can only be called from Main Thread." );

	ASSERT( ac != NULL );

	if ( !ac->GetEntity()->IsPlayer() ) // rethink
	{
		return;
	}

	if ( CMovingAgentComponent* mac = Cast< CMovingAgentComponent >( ac ) )
	{
		CEntity* entity = ac->GetEntity();
		const Matrix& localToWorld = entity->GetLocalToWorld();
		const SPhysicalMaterial* material = mac->GetCurrentStandPhysicalMaterial();
		const CName materialName = material ? material->m_name : CNAME( default );

		Matrix bonePosition( localToWorld );
		if ( m_boneIndex != -1 )
		{
			bonePosition = ac->GetBoneMatrixWorldSpace( m_boneIndex );
		}

		CPlayer* player = Cast< CPlayer >( entity );
		const Bool isPrimaryPlayer = !GCommonGame->IsPrimaryPlayer( player );
		Uint8 atlasTile = 0;
		if ( !isPrimaryPlayer )
		{
			atlasTile = 1;
		}

		CWorld * world = entity->GetLayer()->GetWorld();
		if( world && material )
		{
			if ( material->m_decalFootstepsDiff || material->m_decalFootstepsNormal ) 
			{
				// Add a decal with footstep:

				Vector boneFront = -bonePosition.GetAxisX();

				// We need to add some offset here, cuz 'toe' is not exactly in the middle of the foot, as the decal expects.
				Vector decalPosition = ( bonePosition.GetTranslation() + ( boneFront * 0.07f ) ); 

				Vector decalFront( 0.0f, 0.0f, -1.0f, 0.0f );
				Vector decalUp( boneFront.X, boneFront.Y, 0, 0 );
				decalUp.Normalize3();

				Uint8 atlasScaleS = 2;
				Uint8 atlasScaleT = 1;
				CLightweightDecalEmitter().Spawn( entity->GetLayer()->GetWorld(), decalPosition, decalFront, decalUp, Cast< CBitmapTexture >( material->m_decalFootstepsDiff ), Cast< CBitmapTexture >( material->m_decalFootstepsNormal ), atlasScaleS, atlasScaleT, atlasTile );
			}
		}
	}
}