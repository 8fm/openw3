/**
* Copyright c 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "storySceneDialogset.h"

#include "../core/factory.h"
#include "../core/depot.h"

#include "storySceneUtils.h"
#include "storySceneSystem.h"
#include "storySceneAnimationList.h"
#include "storySceneItems.h"
#include "storySceneEvent.h"

IMPLEMENT_ENGINE_CLASS( CStorySceneDialogsetSlot )

IMPLEMENT_RTTI_ENUM( ESceneDialogsetCameraSet );
IMPLEMENT_ENGINE_CLASS( SSceneCameraShotDescription );
IMPLEMENT_ENGINE_CLASS( SScenePersonalCameraDescription );
IMPLEMENT_ENGINE_CLASS( SSceneMasterCameraDescription );
IMPLEMENT_ENGINE_CLASS( SSceneCustomCameraDescription );
IMPLEMENT_ENGINE_CLASS( CStorySceneDialogset );

IMPLEMENT_ENGINE_CLASS( CStorySceneDialogsetInstance );

RED_DEFINE_NAMED_NAME( CloseUp, "Close Up" );
RED_DEFINE_NAMED_NAME( MediumCloseUp, "Medium Close Up" );
RED_DEFINE_NAMED_NAME( ExtremeCloseUp, "Extreme Close Up" );

CStorySceneDialogset::CStorySceneDialogset()
	: CSkeletalAnimationSet()
	, m_isDynamic( false )
{
#ifndef NO_EDITOR
	m_bitwiseCompressionPreset = ABBCP_VeryHighQuality;
	m_bitwiseCompressionSettings.UsePreset( m_bitwiseCompressionPreset );
#endif
}

void CStorySceneDialogset::GetCameraEyePositionsForSet( ESceneDialogsetCameraSet cameraSet, TDynArray< Vector >& eyePositions ) const
{
	ASSERT( eyePositions.Empty() );

	if ( cameraSet == SDSC_Personal || cameraSet == SDSC_All )
	{
		for ( Uint32 i = 0; i < m_personalCameras.Size(); ++i )
		{
			/*Vector eyePosition = m_cameraEyePositions[ m_personalCameras[ i ].m_cameraNumber - 1 ];
			eyePosition.Z = 0.0f;
			eyePositions.PushBack( eyePosition );*/
		}
	}
	if ( cameraSet == SDSC_Master || cameraSet == SDSC_All )
	{
		for ( Uint32 j = 0; j < m_masterCameras.Size(); ++j )
		{
			/*Vector eyePosition = m_cameraEyePositions[ m_masterCameras[ j ].m_cameraNumber - 1 ];
			eyePosition.Z = 0.0f;
			eyePositions.PushBack( eyePosition );*/
		}
	}
}

void CStorySceneDialogset::CalculateCameraEyePositions()
{
	m_cameraEyePositions.Clear();
	for ( Uint32 i = 0; i < m_cameraTrajectories.Size(); ++i )
	{
		// Calculate eye positions using default camera shot animation
		String defaultCameraShotName = String::Printf( TXT( "camera%d" ), i+1 );

		CSkeletalAnimationSetEntry* defaultCameraShot = FindAnimation( CName( defaultCameraShotName ) );
		if ( defaultCameraShot == NULL )
		{
			m_cameraEyePositions.PushBack( m_cameraTrajectories[ i ].GetPosition() );
		}
		else
		{
			TDynArray< AnimQsTransform > bones;
			TDynArray< Float > tracks;
			defaultCameraShot->GetAnimation()->Sample( 0.f, bones, tracks );

			// For cameras it is safe to assume that camera eye is last bone in skeleton made of simple chained bones
#ifdef USE_HAVOK_ANIMATION
			hkQsTransform lastBoneTransform;
			lastBoneTransform.setIdentity();
			for ( Uint32 j = 0; j < bones.Size(); ++j )
			{
				lastBoneTransform.setMul( lastBoneTransform, bones[ j ] );
			}
			
			Matrix lastBoneMatrix;
			HavokTransformToMatrix( lastBoneTransform, &lastBoneMatrix );

#else
			RedQsTransform lastBoneTransform;
			lastBoneTransform.SetIdentity();
			for ( Uint32 j = 0; j < bones.Size(); ++j )
			{
				lastBoneTransform.SetMul( lastBoneTransform, bones[ j ] );
			}
			
			RedMatrix4x4 conversionMatrix;
			Matrix lastBoneMatrix;
			conversionMatrix = lastBoneTransform.ConvertToMatrix();
			lastBoneMatrix = reinterpret_cast< const Matrix& >( conversionMatrix );
#endif

			Matrix cameraTrajectoryMatrix;
			m_cameraTrajectories[ i ].CalcLocalToWorld( cameraTrajectoryMatrix );

			m_cameraEyePositions.PushBack( ( lastBoneMatrix * cameraTrajectoryMatrix ).GetTranslation() );
			//m_cameraEyePositions.PushBack( ( cameraTrajectoryMatrix * lastBoneMatrix ).GetTranslation() );
		}
	}
}


void CStorySceneDialogset::ImportFromOldDialogset( const CName& dialogsetName, Uint32 charactersInDialogset, Uint32 camerasInDialogset )
{

}

//////////////////////////////////////////////////////////////////////////

String CStorySceneDialogset::GetDefaultDialogsetPath()
{
	return TXT( "globals\\scenes\\dialogsets\\4vs4.w2dset" );
}

String CStorySceneDialogset::GetDialogsetPathByName( const CName& dialogsetName )
{
	C2dArray* oldDialogsetTable = Cast< C2dArray >( GDepot->LoadResource( TXT("gameplay\\globals\\scenes\\scene_dialogsets.csv") ) );
	if ( oldDialogsetTable != NULL )
	{
		String newDialogsetPath
			= oldDialogsetTable->GetValue< String >( TXT( "Name" ), dialogsetName.AsString(), TXT( "NewDialogset" ) );
		if ( newDialogsetPath.Empty() == false )
		{
			return newDialogsetPath;
		}
	}
	return GetDefaultDialogsetPath();
}

String CStorySceneDialogset::GetDialogsetPathById( const String& dialogsetId )
{
	C2dArray* oldDialogsetTable = Cast< C2dArray >( GDepot->LoadResource( TXT("gameplay\\globals\\scenes\\scene_dialogsets.csv") ) );
	if ( oldDialogsetTable != NULL )
	{
		String newDialogsetPath
			= oldDialogsetTable->GetValue< String >( TXT( "Id" ), dialogsetId, TXT( "NewDialogset" ) );
		if ( newDialogsetPath.Empty() == false )
		{
			return newDialogsetPath;
		}
	}
	return GetDefaultDialogsetPath();
}

Bool CStorySceneDialogset::IsCameraNumberValid( Int32 cameraNumber ) const
{
	return ( cameraNumber > 0 && (Uint32) cameraNumber <= m_cameraTrajectories.Size() ) ? true : false;
}

Bool CStorySceneDialogset::IsSlotNumberValid( Int32 slotNumber ) const
{
	return ( slotNumber > 0 && (Uint32) slotNumber <= m_slots.Size() ) ? true : false;
}

Bool CStorySceneDialogset::ImportCharacterTrajectories( CSkeletalAnimation* trajectoryAnimation, Uint32 maxTrajectories )
{
	/*ASSERT( trajectoryAnimation != NULL );
	if ( trajectoryAnimation == NULL )
	{
		return false;
	}

	m_characterTrajectories.Clear();

	TDynArray< hkQsTransform > bones;
	TDynArray< Float > tracks;
	trajectoryAnimation->Sample( 0.f, bones, tracks );

	for ( Uint32 i = 0; i < bones.Size() - 1 && i < maxTrajectories; ++i )
	{
		Matrix trajectoryTransform;
		HavokTransformToMatrix( bones[ i + 1 ], &trajectoryTransform );

		m_characterTrajectories.PushBack( EngineTransform( trajectoryTransform ) );
	}*/
	return true;
}

Bool CStorySceneDialogset::ImportCameraTrajectories( CSkeletalAnimation* trajectoryAnimation, Uint32 maxTrajectories )
{
	ASSERT( trajectoryAnimation != NULL );
	if ( trajectoryAnimation == NULL )
	{
		return false;
	}
	m_cameraTrajectories.Clear();
	TDynArray< AnimQsTransform > bones;
	TDynArray< Float > tracks;

	trajectoryAnimation->Sample( 0.f, bones, tracks );

	for ( Uint32 j = 0; j < bones.Size() - 1 && j < maxTrajectories; ++j )
	{
#ifdef USE_HAVOK_ANIMATION
		Matrix trajectoryTransform;
		HavokTransformToMatrix( bones[ j + 1 ], &trajectoryTransform );
#else
		RedMatrix4x4 conversionMatrix;
		conversionMatrix = bones[ j + 1 ].ConvertToMatrix();
		Matrix trajectoryTransform  = reinterpret_cast< const Matrix& >( conversionMatrix );
#endif
		m_cameraTrajectories.PushBack( EngineTransform( trajectoryTransform ) );
	}

	return true;
}

void CStorySceneDialogset::OnPropertyPostChange( IProperty* property )
{
}

void CStorySceneDialogset::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );
	//if ( file.IsReader() && file.GetVersion() < VER_APPERTURE_SCENE_DOF )
	if ( file.IsReader()  )
	{
		for( TDynArray< StorySceneCameraDefinition >::iterator cameraIter = m_cameras.Begin();
			cameraIter != m_cameras.End(); ++cameraIter )
		{
			if ( cameraIter->m_dof.IsDefault() == true )
			{
				cameraIter->m_dof.FromEngineDofParams( cameraIter->m_dofFocusDistNear, cameraIter->m_dofFocusDistFar );
			}
		}
	}
}

void CStorySceneDialogset::GetSlotPlacements( TDynArray< EngineTransform >& placements ) const
{
	for ( Uint32 i = 0; i < m_slots.Size(); ++i )
	{
		if ( m_slots[ i ] == NULL )
		{
			continue;
		}
		
		placements.PushBack( m_slots[ i ]->GetSlotPlacement() );
	}
}

#ifndef NO_EDITOR

void CStorySceneDialogset::OnCloned()
{
	for ( auto slot : m_slots )
	{
		slot->OnCloned();
	}
}

void CStorySceneDialogset::ConvertSlots()
{
	m_slots.Clear();
	for ( Uint32 i = 0; i < m_characterTrajectories.Size(); ++i )
	{
		EngineTransform& characterTrajectory = m_characterTrajectories[ i ];

		CStorySceneDialogsetSlot* slot = ::CreateObject< CStorySceneDialogsetSlot >( this );
		slot->SetSlotNumber( i + 1 );
		String slotName = String::Printf( TXT( "slot%d" ), i + 1 );
		slot->SetSlotName( CName( slotName ) );
		slot->SetSlotPlacement( characterTrajectory );
		slot->OnCreatedInEditor();

		m_slots.PushBack( slot );
	}
}

void CStorySceneDialogset::ConvertCameraDefinitions()
{
	m_cameras.Clear();
	TDynArray< CSkeletalAnimationSetEntry* > cameraShotAnimations;
	GetAnimations( cameraShotAnimations );

	TDynArray< TPair< StorySceneCameraDefinition, SSceneCameraShotDescription > > tempCameraDefinitions;

	for ( Uint32 i = 0; i < m_personalCameras.Size(); ++i )
	{
		for ( Uint32 j = 0; j < m_personalCameras[ i ].m_cameraShots.Size(); ++j  )
		{
RED_MESSAGE( "Need to come up with a solution for generating new cnames from old cnames" )
			String shotName = String::Printf( TXT( "slot%d %s" ), i + 1, m_personalCameras[ i ].m_cameraShots[ j ].m_shotName.AsString().AsChar() );
			
			StorySceneCameraDefinition cameraDefinition;
			cameraDefinition.m_cameraName = CName( shotName );

			CStorySceneDialogsetSlot* sourceSlot = NULL;
			//CStorySceneDialogsetSlot* targetSlot = NULL;

			if ( m_personalCameras[ i ].m_sourceSlot >= 1 && m_personalCameras[ i ].m_sourceSlot <= m_slots.Size() )
			{
				CStorySceneDialogsetSlot* sourceSlot = m_slots[ m_personalCameras[ i ].m_sourceSlot - 1 ];
				if ( sourceSlot != NULL )
				{
					cameraDefinition.m_sourceSlotName = sourceSlot->GetSlotName();
				}
			}

			if ( m_personalCameras[ i ].m_targetSlot >= 1 && m_personalCameras[ i ].m_targetSlot <= m_slots.Size() )
			{
				CStorySceneDialogsetSlot* targetSlot = m_slots[ m_personalCameras[ i ].m_targetSlot - 1 ];
				if ( sourceSlot != NULL )
				{
					cameraDefinition.m_targetSlotName = targetSlot->GetSlotName();
				}
			}

			cameraDefinition.m_cameraTransform = m_cameraTrajectories[ i ];

			tempCameraDefinitions.PushBack( TPair< StorySceneCameraDefinition, SSceneCameraShotDescription >( cameraDefinition, m_personalCameras[ i ].m_cameraShots[ j ] ) );
		}
	}
	for ( Uint32 k = 0; k < m_masterCameras.Size(); ++k )
	{
		for ( Uint32 l = 0; l < m_masterCameras[ k ].m_cameraShots.Size(); ++l  )
		{
RED_MESSAGE( "Need to come up with a solution for generating new cnames from old cnames" )
			String shotName = String::Printf( TXT( "master%d %s" ), k + 1, m_masterCameras[ k ].m_cameraShots[ l ].m_shotName.AsString().AsChar() );

			StorySceneCameraDefinition cameraDefinition;
			cameraDefinition.m_cameraName = CName( shotName );

			cameraDefinition.m_cameraTransform = m_cameraTrajectories[ k + m_personalCameras.Size() ];

			tempCameraDefinitions.PushBack( TPair< StorySceneCameraDefinition, SSceneCameraShotDescription >( cameraDefinition, m_masterCameras[ k ].m_cameraShots[ l ] ) );
		}
	}

	for ( Uint32 m = 0; m < tempCameraDefinitions.Size(); ++m )
	{
		CSkeletalAnimationSetEntry* cameraShot = FindAnimation( tempCameraDefinitions[ m ].m_second.m_animationName );
		if ( cameraShot == NULL )
		{
			continue;
		}

		StorySceneCameraDefinition& cameraDefinition = tempCameraDefinitions[ m ].m_first;
		TDynArray< AnimQsTransform > bones;
		TDynArray< Float > tracks;
		cameraShot->GetAnimation()->Sample( 0.f, bones, tracks );

		// For cameras it is safe to assume that camera eye is last bone in skeleton made of simple chained bones
		AnimQsTransform lastBoneTransform( AnimQsTransform::IDENTITY );
#ifdef USE_HAVOK_ANIMATION
		for ( Uint32 n = 0; n < bones.Size(); ++n )
		{
			lastBoneTransform.setMul( lastBoneTransform, bones[ n ] );
		}

		Matrix lastBoneMatrix;
		HavokTransformToMatrix( lastBoneTransform, &lastBoneMatrix );
#else
		for ( Uint32 n = 0; n < bones.Size(); ++n )
		{
			lastBoneTransform.SetMul( lastBoneTransform, bones[ n ] );
		}
		RedMatrix4x4 conversionMatrix = lastBoneTransform.ConvertToMatrix();
		Matrix lastBoneMatrix = reinterpret_cast< const Matrix& >( conversionMatrix );
#endif
		Matrix cameraTrajectoryMatrix;
		cameraDefinition.m_cameraTransform.CalcLocalToWorld( cameraTrajectoryMatrix );

		Matrix cameraWorldSpaceMatrix = lastBoneMatrix * cameraTrajectoryMatrix;

		
		EngineTransform cameraEyePlacement( cameraWorldSpaceMatrix );


		
		cameraDefinition.m_cameraTransform = cameraEyePlacement;
		
		if ( tracks.Size() > SBehaviorGraphOutput::FTT_FOV )
		{
			cameraDefinition.m_cameraFov = tracks[ SBehaviorGraphOutput::FTT_FOV ];
		}
		else
		{
			cameraDefinition.m_cameraFov = 45.0f;
		}

		// DOF
		if ( tracks.Size() > SBehaviorGraphOutput::FTT_DOF_BlurDistNear && tempCameraDefinitions[ m ].m_second.m_overrideDof == false ) 
		{
			cameraDefinition.m_dofFocusDistFar = tracks[ SBehaviorGraphOutput::FTT_DOF_FocusDistFar ];
			cameraDefinition.m_dofBlurDistFar = tracks[ SBehaviorGraphOutput::FTT_DOF_BlurDistFar ];
			cameraDefinition.m_dofIntensity = tracks[ SBehaviorGraphOutput::FTT_DOF_Intensity ];
			cameraDefinition.m_dofFocusDistNear = tracks[ SBehaviorGraphOutput::FTT_DOF_FocusDistNear ];
			cameraDefinition.m_dofBlurDistNear = tracks[ SBehaviorGraphOutput::FTT_DOF_BlurDistNear ];
		}
		else if ( tempCameraDefinitions[ m ].m_second.m_overrideDof == true )
		{
			cameraDefinition.m_dofFocusDistFar = tempCameraDefinitions[ m ].m_second.m_dofFocusDistFar;
			cameraDefinition.m_dofBlurDistFar = tempCameraDefinitions[ m ].m_second.m_dofBlurDistFar;
			cameraDefinition.m_dofIntensity = tempCameraDefinitions[ m ].m_second.m_dofIntensity;
			cameraDefinition.m_dofFocusDistNear = tempCameraDefinitions[ m ].m_second.m_dofFocusDistNear;
			cameraDefinition.m_dofBlurDistNear = tempCameraDefinitions[ m ].m_second.m_dofBlurDistNear;
		}

		cameraDefinition.m_enableCameraNoise = true;

		m_cameras.PushBack( cameraDefinition );
	}
}

#endif

//////////////////////////////////////////////////////////////////////////

class CStorySceneDialogsetFactory : public IFactory
{
	DECLARE_ENGINE_CLASS( CStorySceneDialogsetFactory, IFactory, 0 );

public:
	CStorySceneDialogsetFactory()
	{
		m_resourceClass = ClassID< CStorySceneDialogset >();
	}

	virtual CResource* DoCreate( const FactoryOptions& options );
};

BEGIN_CLASS_RTTI( CStorySceneDialogsetFactory )
PARENT_CLASS( IFactory )
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( CStorySceneDialogsetFactory );

CResource* CStorySceneDialogsetFactory::DoCreate( const FactoryOptions& options )
{
	CStorySceneDialogset* dialogset = ::CreateObject< CStorySceneDialogset >( options.m_parentObject );
	return dialogset;
}

//////////////////////////////////////////////////////////////////////////

CStorySceneDialogsetSlot* CStorySceneDialogsetInstance::GetSlotByActorName( const CName& actorName ) const
{
	CStorySceneDialogsetSlot* slot = NULL;
	for ( Uint32 i = 0; i < m_slots.Size(); ++i )
	{
		if ( m_slots[ i ] != NULL && m_slots[ i ]->GetActorName() == actorName )
		{
			slot = m_slots[ i ];
			break;
		}
	}
	return slot;
}

CStorySceneDialogsetSlot* CStorySceneDialogsetInstance::GetSlotBySlotNumber( Uint32 slotNumber ) const
{
	CStorySceneDialogsetSlot* slot = NULL;
	for ( Uint32 i = 0; i < m_slots.Size(); ++i )
	{
		if ( m_slots[ i ] != NULL && m_slots[ i ]->GetSlotNumber() == slotNumber )
		{
			slot = m_slots[ i ];
			break;
		}
	}
	return slot;
}

CStorySceneDialogsetSlot* CStorySceneDialogsetInstance::GetSlotByName( const CName& slotName ) const
{	
	CStorySceneDialogsetSlot* slot = NULL;
	for ( Uint32 i = 0; i < m_slots.Size(); ++i )
	{
		if ( m_slots[ i ] != NULL && m_slots[ i ]->GetSlotName() == slotName )
		{
			slot = m_slots[ i ];
			break;
		}
	}
	return slot;
}

void CStorySceneDialogsetInstance::GetAllActorNames( TDynArray< CName >& names ) const
{
	for ( Uint32 i = 0; i < m_slots.Size(); ++i )
	{
		if ( m_slots[ i ] != NULL && m_slots[ i ]->GetActorName() != CName::NONE )
		{
			names.PushBackUnique( m_slots[ i ]->GetActorName() );
		}
	}
}

void CStorySceneDialogsetInstance::CollectRequiredTemplates( TDynArray< TSoftHandle< CResource > >& requiredTemplates ) const
{
	const CStoryScene* scene = FindParent< CStoryScene >();
	SCENE_ASSERT( scene );
	if ( scene )
	{
		const TDynArray< CStorySceneDialogsetSlot* >& dialogsetSlots = GetSlots();
		for ( TDynArray< CStorySceneDialogsetSlot* >::const_iterator slotIter = dialogsetSlots.Begin(); slotIter != dialogsetSlots.End(); ++slotIter )
		{
			if ( *slotIter == NULL )
			{
				continue;
			}
			const CStorySceneActor* actorDescription = scene->GetActorDescriptionForVoicetag( (*slotIter)->GetActorName() );
			if ( actorDescription != NULL )
			{					
				requiredTemplates.PushBackUnique( TSoftHandle< CResource >( actorDescription->m_entityTemplate.GetPath() ) );
			}
		}
	}
}

void CStorySceneDialogsetInstance::CollectUsedAnimations( CStorySceneAnimationContainer& container ) const
{
	for ( const CStorySceneDialogsetSlot* s : m_slots )
	{
		if ( s )
		{
			s->CollectUsedAnimations( container );
		}
	}
}

CStorySceneDialogsetInstance* CStorySceneDialogsetInstance::CreateFromResource( CStorySceneDialogset* dialogsetResource, CObject* parent )
{
	CStorySceneDialogsetInstance* instance = ::CreateObject< CStorySceneDialogsetInstance >( parent );
	instance->SetName( CName( dialogsetResource->GetFile()->GetFileName() ) );

	instance->m_path = dialogsetResource->GetDepotPath();

	const TDynArray< CStorySceneDialogsetSlot* >& resourceSlots = dialogsetResource->GetSlots();
	for ( Uint32 i = 0; i < resourceSlots.Size(); ++i )
	{
		instance->AddSlot( Cast<CStorySceneDialogsetSlot>( resourceSlots[ i ]->Clone( instance ) ) );
	}

	return instance;
}

#ifndef NO_EDITOR

void CStorySceneDialogsetInstance::OnCloned()
{
	for ( auto slot : m_slots )
	{
		slot->OnCloned();
	}
}

#endif

//////////////////////////////////////////////////////////////////////////

CStorySceneDialogsetSlot::CStorySceneDialogsetSlot()
	: m_actorVisibility( true )
	, m_actorMimicsLayer_Pose_Weight( 1.f )
	, m_forceBodyIdleAnimationWeight( 1.f )
	, m_actorStatus( CStorySceneAnimationList::DEFAULT_STATUS )
	, m_actorEmotionalState( CStorySceneAnimationList::DEFAULT_EMO_STATE )
	, m_actorPoseName( CStorySceneAnimationList::DEFAULT_POSE )
	, m_actorMimicsEmotionalState( CStorySceneAnimationList::DEFAULT_MIMICS_EMO_STATE )
	, m_actorMimicsLayer_Eyes( CStorySceneAnimationList::DEFAULT_MIMICS_LAYER_EYES )
	, m_actorMimicsLayer_Pose( CStorySceneAnimationList::DEFAULT_MIMICS_LAYER_POSE )
	, m_actorMimicsLayer_Animation( CStorySceneAnimationList::DEFAULT_MIMICS_LAYER_ANIMATION )
{
}

void CStorySceneDialogsetSlot::GetMimicsLayers( CName& eyes, CName& pose, CName& animation ) const
{
	eyes = m_actorMimicsLayer_Eyes;
	pose = m_actorMimicsLayer_Pose;
	animation = m_actorMimicsLayer_Animation;
}

void CStorySceneDialogsetSlot::CacheMimicsLayerFromEmoState( Bool force )
{
	if ( m_actorMimicsEmotionalState && ( force || ( !m_actorMimicsLayer_Eyes || !m_actorMimicsLayer_Pose || !m_actorMimicsLayer_Animation ) ) )
	{
		if ( force || !m_actorMimicsLayer_Eyes )
		{
			m_actorMimicsLayer_Eyes = m_actorMimicsEmotionalState;
		}

		if ( force || !m_actorMimicsLayer_Pose )
		{
			m_actorMimicsLayer_Pose = m_actorMimicsEmotionalState;
		}

		if ( force || !m_actorMimicsLayer_Animation )
		{
			m_actorMimicsLayer_Animation = m_actorMimicsEmotionalState;
		}
	}
}

#ifndef NO_EDITOR

void CStorySceneDialogsetSlot::OnPropertyPostChange( IProperty* prop )
{
	TBaseClass::OnPropertyPostChange( prop );

	if ( prop->GetName() == TXT("actorMimicsEmotionalState") )
	{
		CacheMimicsLayerFromEmoState( true );
	}
	else if ( prop->GetName() == TXT("actorMimicsLayer_Eyes") || prop->GetName() == TXT("actorMimicsLayer_Pose") || prop->GetName() == TXT("actorMimicsLayer_Animation") )
	{
		m_actorMimicsEmotionalState = CName::NONE;
	}
}

String CStorySceneDialogsetSlot::GetDescriptionString() const
{
	return String::Printf
	(
		TXT( "%s, %s, %s, %s, %s, %s, %s" ),
		m_slotName.AsString().AsChar(),
		m_actorName.AsString().AsChar(),
		( m_actorVisibility == true ) ? TXT( "visible" ) : TXT( "hidden" ),
		m_actorStatus.AsString().AsChar(), m_actorEmotionalState.AsString().AsChar(), m_actorPoseName.AsString().AsChar(),
		m_slotPlacement.ToString().AsChar()
	);
}

void CStorySceneDialogsetSlot::OnCreatedInEditor()
{
	m_ID = CGUID::Create();
}

void CStorySceneDialogsetSlot::OnCloned()
{
	m_ID = CGUID::Create();
}

#endif

void CStorySceneDialogsetSlot::OnPostLoad()
{
	TBaseClass::OnPostLoad();

	if ( m_actorState )
	{
		StorySceneUtils::ConvertStateToAnimationFilters( m_actorState, m_actorStatus, m_actorEmotionalState, m_actorPoseName );
		m_actorState = CName::NONE;
	}

	if ( m_ID.IsZero() )
	{
		m_ID = CGUID::Create();
	}

	CacheMimicsLayerFromEmoState( false );
}

void CStorySceneDialogsetSlot::CollectUsedAnimations( CStorySceneAnimationContainer& container ) const
{
	container.AddBodyIdle( m_actorName, m_actorStatus, m_actorEmotionalState, m_actorPoseName );
	if ( m_forceBodyIdleAnimation )
	{
		container.AddBodyAnimation( m_actorName, m_forceBodyIdleAnimation );
	}
	container.AddMimicIdle( m_actorName, m_actorMimicsLayer_Eyes, m_actorMimicsLayer_Pose, m_actorMimicsLayer_Animation );

	// We have to add default animations always because of behavior graph default properties
	// TODO - do it smarter...
	container.AddBodyIdle( m_actorName, CStorySceneAnimationList::DEFAULT_STATUS, CStorySceneAnimationList::DEFAULT_EMO_STATE, CStorySceneAnimationList::DEFAULT_POSE );
}

Bool CStorySceneDialogsetSlot::OnPropertyMissing( CName propertyName, const CVariant& readValue )
{
	/*if ( propertyName == TXT( "actorState" ) )
	{
		ASSERT( readValue.GetType() == GetTypeName< CName >() );

		CName actorStateName;
		if ( readValue.AsType< CName >( actorStateName ) )
		{
			StorySceneUtils::ConvertStateToAnimationFilters( actorStateName, m_actorStatus, m_actorEmotionalState, m_actorPoseName );
			
			return true;
		}
	}*/

	return TBaseClass::OnPropertyMissing( propertyName, readValue );	
}
