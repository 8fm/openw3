
#include "build.h"
#include "bgNpcRoot.h"
#include "bgNpc.h"

#include "../../common/engine/behaviorGraphOutput.h"
#include "../../common/core/gatheredResource.h"
#include "../engine/behaviorGraphContext.h"
#include "extAnimItemEvents.h"
#include "extAnimSoundEvent.h"
#include "../engine/skinningAttachment.h"
#include "../engine/meshSkinningAttachment.h"

CGatheredResource resIdleBehavior( TXT("characters\\templates\\background\\idle.w2beh"), RGF_Startup );

IMPLEMENT_ENGINE_CLASS( CBgRootComponent );

CBgRootComponent::CBgRootComponent()
{
}

void CBgRootComponent::OnPostLoad()
{
	TBaseClass::OnPostLoad();

	m_useExtractedMotion = false;
}

Bool CBgRootComponent::ShouldAddToTickGroups() const
{
	return false;
}

void CBgRootComponent::AsyncUpdate( Float dt )
{
	{
		//PC_SCOPE_PIX( BgRootAnimUpdate );

		InternalUpdateAndSampleMultiAsyncPart( dt );
	}

	if ( m_behaviorGraphSampleContext && m_behaviorGraphSampleContext->IsValid() )
	{
		FilterEvents();

		//PC_SCOPE_PIX( BgRootAnimCalcMS );

		// Update model space matrices
		SBehaviorGraphOutput& pose = m_behaviorGraphSampleContext->GetSampledPose();
		pose.GetBonesModelSpace( m_skeleton.Get(), m_skeletonModelSpace );
	}

	{
		//PC_SCOPE_PIX( BgRootAnimCalcWS );

		// Update world space matrices
		ASSERT( m_skeletonModelSpace.Size() == m_skeletonWorldSpace.Size() );
		SkeletonBonesUtils::MulMatrices( m_skeletonModelSpace.TypedData(), m_skeletonWorldSpace.TypedData(), m_skeletonModelSpace.Size(), &m_localToWorld );
	}

	{
		//PC_SCOPE_PIX( BgRootAnimSendSkinning );

		SMeshSkinningUpdateContext skinningContext;

		const TList< IAttachment* >& attachments = GetChildAttachments();
		for ( TList< IAttachment* >::const_iterator it = attachments.Begin(); it != attachments.End(); ++it )
		{
			IAttachment* att = *it;
			CMeshSkinningAttachment* skinAtt = att ? att->ToSkinningAttachment() : nullptr;
			if ( skinAtt )
			{
				Box box( Box::RESET_STATE );
				skinAtt->UpdateTransformAndSkinningData( box, skinningContext );
			}
		}

		skinningContext.CommitCommands();
	}
}

void CBgRootComponent::FilterEvents()
{
	SBehaviorGraphOutput& output = m_behaviorGraphSampleContext->GetSampledPose();
	if ( output.m_numEventsFired > 0 )
	{
		for ( Uint32 i=0; i<output.m_numEventsFired; ++i )
		{
			CAnimationEventFired &currEvent = output.m_eventsFired[i];
			
			if ( IsType< CExtAnimItemEvent >( currEvent.m_extEvent ) || IsType< CExtAnimSoundEvent >( currEvent.m_extEvent ) || IsType< CExtAnimLookAtEvent >( currEvent.m_extEvent ) )
			{
				continue;
			}
			else if ( IsEventForExJob( currEvent.m_extEvent->GetEventName() ) )
			{
				continue;
			}
			else
			{
				currEvent.m_extEvent = NULL;
			}
		}
	}
}

void CBgRootComponent::FireEvents()
{
	if ( m_behaviorGraphSampleContext )
	{
		//PC_SCOPE_PIX( BgRootAnimCalcMS );

		SBehaviorGraphOutput& output = m_behaviorGraphSampleContext->GetSampledPose();
		if ( output.m_numEventsFired > 0 )
		{
			for ( Uint32 i=0; i<output.m_numEventsFired; ++i )
			{
				const CAnimationEventFired &currEvent = output.m_eventsFired[i];
				if ( !currEvent.m_extEvent )
				{
					continue;
				}

				if ( IsEventForExJob( currEvent.m_extEvent->GetEventName() ) )
				{
					// Special case for job system - is is not perfect solution...
					CBgNpc* parent = SafeCast< CBgNpc >( GetEntity() );
					parent->FireJobEvent( currEvent.m_extEvent->GetEventName() );
				}

				{
					// Special case for duration events
					if( currEvent.m_type == AET_DurationStart )
					{
						// Start event
						static_cast< const CExtAnimDurationEvent* >( currEvent.m_extEvent )->Start( currEvent, this );
					}
					else if ( currEvent.m_type == AET_DurationEnd )
					{
						// End event
						static_cast< const CExtAnimDurationEvent* >( currEvent.m_extEvent )->Stop( currEvent, this );
					}

					currEvent.m_extEvent->Process( currEvent, this );
				}
			}
		}
	}
}
