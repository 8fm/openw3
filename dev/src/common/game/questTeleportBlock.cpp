#include "build.h"
#include "questGraphSocket.h"
#include "questTeleportBlock.h"
#include "../engine/graphConnectionRebuilder.h"
#include "../engine/pathlibWorld.h"

IMPLEMENT_ENGINE_CLASS( CQuestTeleportBlock )

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CQuestTeleportBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	// Create sockets
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( In ), LSD_Input, LSP_Left ) );
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Out ), LSD_Output, LSP_Right ) );
}

#endif

void CQuestTeleportBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	TBaseClass::OnActivate( data, inputName, parentThread );

	Vector safeOffset( Vector::ZEROS );
	const Float angleDiff = 60;
	if ( m_distanceToDestination > 0 && !m_locationTag.Empty() )
	{
		CNode* destinationNode = GGame->GetActiveWorld()->GetTagManager()->GetTaggedNode( m_locationTag.GetTag( 0 ) );
		if ( destinationNode )
		{
			const Vector& destinationPosition = destinationNode->GetWorldPositionRef();
			CWorld* world = GGame->GetActiveWorld();
			if ( world )
			{
				CPathLibWorld* pathlibWorld = world->GetPathLibWorld();
				if ( pathlibWorld )
				{
					for ( Int32 step = 0; step < ( 360 / angleDiff ); ++step )
					{
						Vector3 offset = EulerAngles::YawToVector( step * angleDiff );
						offset *= m_distanceToDestination;

						Vector3 findSaveSpotResult;
						if ( pathlibWorld->FindSafeSpot( PathLib::INVALID_AREA_ID, destinationPosition + offset, 10, 1, findSaveSpotResult ) )
						{
							PathLib::AreaId areaId = PathLib::INVALID_AREA_ID;
							Vector3 getClearLineResult;
							if ( pathlibWorld->GetClearLineInDirection( areaId, destinationPosition, findSaveSpotResult, 1, getClearLineResult ) != PathLib::CLEARLINE_INVALID_START_POINT )
							{
								safeOffset = getClearLineResult - destinationPosition;
								break;
							}
						}
					}
				}
			}
		}
	}

	STeleportInfo teleportInfo;
	teleportInfo.SetTarget( STeleportInfo::TargetType_Node, m_locationTag, m_distance, safeOffset, m_actorsTags );

	if ( !CTeleportHelper( &teleportInfo ).TeleportAllActors() )
	{
		ThrowErrorNonBlocking( data, CNAME( Out ), TXT( "Teleport location does not existor there are no actors to teleport." ) );
		return;
	}
	
	ActivateOutput( data, CNAME( Out ) );
}

RED_DEFINE_STATIC_NAME( locationTag );

Bool CQuestTeleportBlock::OnPropertyTypeMismatch( CName propertyName, IProperty* existingProperty, const CVariant& readValue )
{
	if ( propertyName == CNAME( locationTag ) )
	{
		CName nameValue;
		readValue.AsType( nameValue );

		TagList* taglist = ( TagList* ) existingProperty->GetOffsetPtr( this );
		taglist->Clear();
		taglist->AddTag( nameValue );

		return true;
	}

	return TBaseClass::OnPropertyTypeMismatch( propertyName, existingProperty, readValue );
}
