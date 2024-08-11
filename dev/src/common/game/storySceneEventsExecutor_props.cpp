
#include "build.h"
#include "storySceneEventsExecutor.h"
#include "storySceneEventsCollector.h"
#include "storySceneEventsCollector_events.h"
#include "storySceneDebugger.h"
#include "storyScenePlayer.h"
#include "storySceneUtils.h"
#include "storySceneVoicetagMapping.h"
#include "storySceneSystem.h"
#include "../engine/behaviorGraphStack.h"
#include "itemIterator.h"

#ifdef DEBUG_SCENES
#pragma optimize("",off)
#endif

void CStorySceneEventsExecutor::ProcessPropsTransforms( CStoryScenePlayer* player, const CStorySceneEventsCollector& collector, IStorySceneDebugger* debugger )
{
	const Uint32 size = collector.m_propPlacements.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		const StorySceneEventsCollector::PropPlacement& evt = collector.m_propPlacements[ i ];
		if( CEntity* ent = FindActorByType( evt.m_actorId, AT_PROP | AT_EFFECT | AT_LIGHT ).Get() )
		{
			if( !ent->GetTransformParent() )
			{
				ent->Teleport( evt.m_placementWS.GetPosition(), evt.m_placementWS.GetRotation() );		
			}
		}
	}
}

void CStorySceneEventsExecutor::ProcessPropsAttachEvents( CStoryScenePlayer* player, const CStorySceneEventsCollector& collector, IStorySceneDebugger* debugger )
{
	const Uint32 size = collector.m_attachPropToBoneEvents.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		const StorySceneEventsCollector::AttachPropToBone& evt = collector.m_attachPropToBoneEvents[i];	
		CEntity* sceneProp = FindActorByType( evt.m_actorId, AT_PROP ).Get();
		if ( evt.m_isAttached )
		{	
			CEntity* attachParent = FindActorByType( evt.m_targetEntity, AT_ACTOR | AT_PROP ).Get();
			if( attachParent && sceneProp )
			{
				if( sceneProp->GetTransformParent() )
				{
					sceneProp->GetTransformParent()->Break();
				}
				const EntitySlot* entitySlot = attachParent->GetEntityTemplate()->FindSlotByName( evt.m_targetSlot, true );
				if ( !entitySlot )
				{
					continue;
				}
				CComponent* parentComponent = attachParent->FindComponent( entitySlot->GetComponentName() );				
				if ( !parentComponent )
				{
					continue;
				}
				const ISkeletonDataProvider* skelData = parentComponent->QuerySkeletonDataProvider();
				if ( !skelData )
				{
					continue;
				}

				HardAttachmentSpawnInfo ainfo;
				ainfo.m_parentSlotName = entitySlot->GetBoneName();
				ainfo.m_relativePosition = entitySlot->GetTransform().GetPosition();
				ainfo.m_relativeRotation = entitySlot->GetTransform().GetRotation();
				if ( !evt.m_snapAtStart )
				{
					Int32 boneId = skelData->FindBoneByName( ainfo.m_parentSlotName );
					if ( boneId >= 0 )
					{
						Matrix attachmentMatWS = sceneProp->GetRotation().ToMatrix();
						attachmentMatWS.SetTranslation( sceneProp->GetPosition() );
						Matrix boneMatWS = skelData->GetBoneMatrixWorldSpace( boneId );
						Matrix attachmentMatBS = attachmentMatWS * boneMatWS.FullInverted();
						Vector relativeLoc = attachmentMatBS.GetTranslation();
						EulerAngles relativeRotation = attachmentMatBS.ToEulerAnglesFull();
						ainfo.m_relativePosition = relativeLoc;
						ainfo.m_relativeRotation = relativeRotation;
					}					
				}
				sceneProp->Teleport(evt.m_offset.GetPosition(), evt.m_offset.GetRotation() );	
				parentComponent->Attach( sceneProp, ainfo );		
			}
		}
		else
		{
			if( sceneProp && sceneProp->GetTransformParent() )
			{
				sceneProp->GetTransformParent()->Break();
				CName propId = evt.m_actorId;

				auto pred = [propId]( const StorySceneEventsCollector::PropPlacement& evtent ) { return evtent.m_actorId == propId; };
				TDynArray< StorySceneEventsCollector::PropPlacement >::const_iterator evtIter = FindIf( collector.m_propPlacements.Begin(), collector.m_propPlacements.End(), pred );
				if( evtIter != collector.m_propPlacements.End() )
				{
					sceneProp->Teleport( evtIter->m_placementWS.GetPosition(), evtIter->m_placementWS.GetRotation() );
				}
				else
				{
					sceneProp->Teleport( sceneProp->GetWorldPosition(), sceneProp->GetWorldRotation() );	
				}					
			}	
		}		
	}
}

#ifdef DEBUG_SCENES
#pragma optimize("",off)
#endif
