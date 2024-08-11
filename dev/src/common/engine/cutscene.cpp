/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behaviorGraphNotifier.h"
#include "extAnimCutsceneSoundEvent.h"
#include "cutsceneInstance.h"
#include "extAnimCutsceneEffectEvent.h"
#include "../../common/core/gatheredResource.h"
#include "../../common/core/objectIterator.h"
#include "../../common/core/cooker.h"
#include "cutsceneDebug.h"
#include "../core/feedback.h"
#include "skeletalAnimation.h"
#include "animatedComponent.h"
#include "game.h"
#include "layer.h"
#include "tagManager.h"
#include "world.h"
#include "../core/diskFile.h"
#include "fxDefinition.h"
#include "camera.h"
#include "entity.h"
#include "animationBufferMultipart.h"


#ifdef DEBUG_CUTSCENES
#pragma optimize("",off)
#endif

CGatheredResource resWitcherTemplate( TXT("characters\\models\\geralt\\geralt.w2ent"), 0 );
CGatheredResource resCutsceneCameraTemplate( TXT("gameplay\\camera\\camera.w2ent"), RGF_Startup );
CGatheredResource resCutsceneBehaviorTemplate( TXT("characters\\templates\\behaviors\\cutscene_graph.w2beh"), RGF_Startup );
CGatheredResource resCutsceneBehaviorTemplate2( TXT("characters\\templates\\camera\\camera_cutscene_graph.w2beh"), RGF_Startup );
CGatheredResource resCutsceneBehaviorTemplate3( TXT("characters\\templates\\behaviors\\cutscene_mimic_graph.w2beh"), RGF_Startup );
CGatheredResource resCutsceneBehaviorTemplate4( TXT("characters\\templates\\behaviors\\cutscene_graph_witcher.w2beh"), RGF_Startup );
CGatheredResource resCutsceneBehaviorTemplate5( TXT("characters\\templates\\trajectories\\trajectories_24.w2ent"), RGF_Startup );
CGatheredResource resCutsceneArray( TXT("gameplay\\globals\\cutscene.csv"), RGF_Startup );

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( SCutsceneActorDef );
IMPLEMENT_ENGINE_CLASS( CCutsceneTemplate )
IMPLEMENT_ENGINE_CLASS( CCutsceneInstance );
IMPLEMENT_RTTI_ENUM( ECutsceneActorType );

//////////////////////////////////////////////////////////////////////////

const CName CCutsceneTemplate::CUTSCENE_ANIMATION_NAME( TXT( "__cutsceneAnimation" ) );

CCutsceneTemplate::CCutsceneTemplate()
	: m_isValid( true )
	, m_fadeBefore( 0.f)
	, m_fadeAfter( 0.f )
	, m_blackscreenWhenLoading( true )
	, m_checkActorsPosition( false )
	, m_streamable( false )
	, m_lastLevelLoaded( nullptr )
{
#ifndef NO_EDITOR
	m_bitwiseCompressionPreset = ABBCP_VeryHighQuality;
	m_bitwiseCompressionSettings.UsePreset( m_bitwiseCompressionPreset );
#endif
}

CCutsceneTemplate::~CCutsceneTemplate()
{
	// cleanup
	for ( auto eventIter = m_animEvents.Begin(); eventIter != m_animEvents.End(); ++eventIter )
		delete *eventIter;
}

Bool CCutsceneTemplate::IsValid() const
{
	return m_isValid;
}

void CCutsceneTemplate::SetValid( Bool flag )
{
	if ( flag != m_isValid )
	{
		if ( MarkModified() )
		{
			m_isValid = flag;
		}
	}
}

Bool CCutsceneTemplate::Validate( String& errors, IFeedbackSystem* fsys /* = NULL  */)
{
	return false;
}

ECutsceneActorType CCutsceneTemplate::ExtractActorTypeFromTemplate( const CEntityTemplate* templ )
{
	CClass* entClass = SRTTI::GetInstance().FindClass( templ->GetEntityClassName() );
	ASSERT( entClass );

	if ( entClass->IsA< CCamera >() )
	{
		return CAT_Camera;
	}
	else if ( entClass->IsA< CEntity >() && templ->GetEntityClassName() != CNAME( CEntity ) )
	{
		return CAT_Actor;
	}
	else if ( templ->GetEntityClassName() == CNAME( CEntity ) )
	{
		return CAT_Prop;
	}
	else
	{
		return CAT_None;
	}
}

void CCutsceneTemplate::OnSave()
{
#ifndef NO_RESOURCE_IMPORT
	TBaseClass::OnSave();

	// Save cutscene name in cutsene array definition file
	/*C2dArray* csArray = resCutsceneArray.LoadAndGet< C2dArray >();
	ASSERT( csArray );

	if ( csArray && csArray->MarkModified() && m_file )
	{
		CFilePath filePath( m_file->GetDepotPath() );

		String csName = filePath.m_fileName;
		String csFile = m_file->GetDepotPath();

		String prevPath = csArray->GetValue( TXT("Name"), csName, TXT("Resource") );

		if ( prevPath.Empty() || prevPath != csFile )
		{
			if ( !csArray->SetValue( csFile, TXT("Name"), csName, TXT("Resource") ) )
			{
				// Add new row
				TDynArray< String > row;
				row.PushBack( csName );
				row.PushBack( csFile );
				csArray->AddRow( row );
			}

			csArray->Save();
		}
	}*/
#endif
}

SCutsceneActorDef CCutsceneTemplate::CreateActorDef( const String& actorName, const TDynArray< SCutsceneActorDef >& prevDef ) const
{
	for ( Uint32 i=0; i<prevDef.Size(); ++i )
	{
		// Find actor in prev def list
		if ( prevDef[i].m_name == actorName )
		{
			// Copy template from prev to new def
			SCutsceneActorDef def;
			def = prevDef[i];
			def.m_template = prevDef[i].m_template;
			return def;
		}
	}

	// Create new def
	SCutsceneActorDef def;
	def.m_name = actorName;

	// Small hard code because of lazy designers
	if ( actorName == TXT("camera") || actorName == TXT("Camera") )
	{
		def.m_template = resCutsceneCameraTemplate.LoadAndGet< CEntityTemplate >();
	}
	else
	{
		def.m_template = resWitcherTemplate.LoadAndGet< CEntityTemplate >();
	}

	return def;
}

#ifndef NO_RESOURCE_IMPORT

CCutsceneTemplate* CCutsceneTemplate::Create( const CCutsceneTemplate::FactoryInfo& data )
{
	// Check import data
	String errorMsg;
	Uint32 dataSize = data.m_importData.Size();
	for ( Uint32 i=0; i<dataSize; ++i )
	{
		const FactoryInfo::ActorImportData& actorData = data.m_importData[i];
		
		if ( actorData.m_animation.Empty() )
		{
			errorMsg += String::Printf( TXT("Actor %d, no animation path\n"), i );
		}
		else
		{
			if ( GFileManager->GetFileSize( actorData.m_animation ) == 0 )
			{
				errorMsg += String::Printf( TXT("Actor %d, no animation hkx resource\n"), i );
			}
		}
	}

	if ( !errorMsg.Empty() )
	{
		// Show errors
		errorMsg = TXT("Cutscene import error(s):\n") + errorMsg;
		GFeedback->ShowError( errorMsg.AsChar() );
		return NULL;
	}

	CCutsceneTemplate* obj = NULL;

	// Actors def
	TDynArray< SCutsceneActorDef > prevDef;

	if ( data.m_reuse )
	{
		// Reuse
		obj = data.m_reuse;
		prevDef = obj->m_actorsDef;
		obj->m_actorsDef.Clear();
	}
	else
	{
		// Create new resource
		obj = data.CreateResource();
		obj->m_checkActorsPosition = false;
	}

	obj->m_isValid = false;

	GFeedback->BeginTask( TXT("Import cutscene animations"), false );

	// Animation names list
	TDynArray< CName > importAnimations;

	Float duration = 0.f;

	// Fill animset
	for ( Uint32 i=0; i<dataSize; ++i )
	{
		const FactoryInfo::ActorImportData& actorData = data.m_importData[i];

		// Create animation name
		CFilePath filePath( actorData.m_animation );
		CName animName;

		ASSERT( filePath.GetDirectories().Size() > 0 );

		if ( actorData.m_component.Empty() )
		{
			animName = CName( filePath.GetDirectories().Back() + TXT(":Root:") + filePath.GetFileName() );

			// Create actor def for each root component
			obj->m_actorsDef.PushBack( obj->CreateActorDef( filePath.GetDirectories().Back(), prevDef ) );
		}
		else
		{
			animName = CName( filePath.GetDirectories().Back() + TXT(":") + actorData.m_component + TXT(":") + filePath.GetFileName() );
		}

		GFeedback->UpdateTaskInfo( TXT("Animation '%ls'"), animName.AsString().AsChar() );
		GFeedback->UpdateTaskProgress( i, dataSize-1 );

		// Import animation
		ImportAnimationOptions options;
		options.m_animationFile = actorData.m_animation;
		options.m_animationName = animName;
		options.m_maxPartFrames = data.m_params ? data.m_params->m_maxPartFrames : -1;
		options.m_extractMotion = false;
		options.m_resetRoot = true;
		options.m_preferBetterQuality = true;
		Bool ret = obj->ImportAnimation( options );
		if ( !ret )
		{
			GFeedback->EndTask();
			return NULL;
		}

		obj->m_streamable = options.m_maxPartFrames > 0;

		CSkeletalAnimationSetEntry* animEntry = obj->FindAnimation( animName );
		ASSERT( animEntry );

		// Remember imported animation
		importAnimations.PushBack( animName );

		// Check duration
		Float animDur = animEntry->GetDuration();
		if ( animDur > duration )
		{
			duration = animDur;
		}
	}

	// Check animations
	for ( Int32 i=(Int32)obj->m_animations.Size()-1; i>=0; --i )
	{
		const CName& anim = obj->m_animations[i]->GetName();
		Bool found = false;

		for ( Uint32 j=0; j<importAnimations.Size(); j++ )
		{
			if ( importAnimations[j] == anim )
			{
				found = true;
				break;
			}
		}

		if ( !found )
		{
			obj->m_animations.Erase( obj->m_animations.Begin() + i );
		}
	}

	// Done
	GFeedback->EndTask();

	return obj;
}
#endif

SCutsceneActorDef* CCutsceneTemplate::GetActorDefinition( const String& actorName )
{
	for ( Uint32 i=0; i<m_actorsDef.Size(); ++i )
	{
		if ( m_actorsDef[i].m_name == actorName )
		{
			return &m_actorsDef[i];
		}
	}

	CS_ERROR( TXT("Cutscene template: Unable to find actor definition for actor '%ls' in cutscene '%ls'"), actorName.AsChar(), GetFriendlyName().AsChar() );

	return NULL;
}

const SCutsceneActorDef* CCutsceneTemplate::GetActorDefinition( const String& actorName ) const
{
	for ( Uint32 i=0; i<m_actorsDef.Size(); ++i )
	{
		if ( m_actorsDef[i].m_name == actorName )
		{
			return &m_actorsDef[i];
		}
	}

	CS_ERROR( TXT("Cutscene template: Unable to find actor definition for actor '%ls' in cutscene '%ls'"), actorName.AsChar(), GetFriendlyName().AsChar() );

	return NULL;
}

const SCutsceneActorDef* CCutsceneTemplate::GetActorDefinition( const CName& voicetag ) const 
{
	for ( Uint32 i=0; i<m_actorsDef.Size(); ++i )
	{
		if ( m_actorsDef[i].m_voiceTag == voicetag )
		{
			return &m_actorsDef[i];
		}
	}

	CS_ERROR( TXT("Cutscene template: Unable to find actor definition for voicetag '%ls' in cutscene '%ls'"), voicetag.AsChar(), GetFriendlyName().AsChar() );

	return NULL;
}

void CCutsceneTemplate::UnloadActorDefinitiones()
{
	for ( Uint32 i=0; i<m_actorsDef.Size(); ++i )
	{
		m_actorsDef[ i ].m_template.Release();
	}
}

const TagList& CCutsceneTemplate::GetPointTag() const
{
	return m_point;
}

const String CCutsceneTemplate::GetLastLevelLoaded() const
{
	return m_lastLevelLoaded;
}

void CCutsceneTemplate::SetLastLevelLoaded( const String lastLevelLoaded )
{
	m_lastLevelLoaded = lastLevelLoaded;
}

void CCutsceneTemplate::GetActorsName( TDynArray< String >& names ) const
{
	for ( Uint32 i=0; i<m_actorsDef.Size(); ++i )
	{
		names.PushBack( m_actorsDef[i].m_name );
	}
}

void CCutsceneTemplate::GetTypedActorNames( TDynArray< String >& names, ECutsceneActorType actorType ) const
{
	for ( Uint32 i=0; i<m_actorsDef.Size(); ++i )
	{
		if ( m_actorsDef[ i ].m_type == actorType )
		{
			names.PushBack( m_actorsDef[i].m_name );
		}		
	}
}

CName CCutsceneTemplate::GetAnimationName( Uint32 num ) const
{
	return m_animations.Size() > num ? m_animations[num]->GetName() : CName::NONE;
}

void CCutsceneTemplate::UnregisterInstance( CCutsceneInstance* instance )
{
	RED_UNUSED( instance );
}

CCutsceneInstance* CCutsceneTemplate::CreateInstance( CLayer* layer, const Matrix& point, String& errors, const THashMap< String, CEntity*>& actors, Bool doInit /* = true */, const Char* forceSpawnActor /* nullptr */ )
{
	CSimpleCutsceneProvider provider( actors );
	if ( forceSpawnActor )
	{
		provider.AddForceSpawnActor( forceSpawnActor );
	}	
	return CreateInstance( layer, point, errors, &provider, new CCutsceneInGameWorldInterface, doInit );
}

CCutsceneInstance* CCutsceneTemplate::CreateInstance( CLayer* layer, const Matrix& point, String& errors, ICutsceneProvider* provider, ICutsceneWorldInterface* worldInterface, Bool doInit, ICutsceneSceneInterface* sceneInterface )
{
#ifdef NO_CUTSCENES
	return NULL;
#else
	ASSERT( layer );

	if ( !IsValid() )
	{
		//errors = String::Printf( TXT("Cutscene: Cutscene template is invalid, '%ls'"), GetFriendlyName().AsChar() );
		//return NULL;
	}

	// Create new cutscene instance
	EntitySpawnInfo einfo;
	einfo.m_entityClass = ClassID< CCutsceneInstance >();

	CCutsceneInstance* cutscene  = Cast< CCutsceneInstance >( layer->CreateEntitySync( einfo ) );

	if ( !cutscene )
	{
		errors = String::Printf( TXT("Cutscene: Unable to create cutscene instance from template %s"), GetFriendlyName().AsChar() );
		return NULL;
	}

	// Set template
	cutscene->m_csTemplate = this;

	// Increment reference count
	m_instanceReferenceCount.Increment();

	// Set OF_Transient flag
	cutscene->SetFlag( OF_Transient );

	// Create modifiers
	for ( Uint32 i=0; i<m_modifiers.Size(); ++i )
	{
		ICutsceneModifier* m = m_modifiers[ i ];
		if ( m )
		{
			ICutsceneModifierInstance* mInst = m->CreateInstance( cutscene );
			cutscene->AddModifier( mInst );
		}
	}

	// Initialize
	if ( doInit == true )
	{
		if ( !cutscene->Init( point, provider, worldInterface, errors, sceneInterface ) )
		{
			// Delete and return nothing
			cutscene->Discard();
			return NULL;
		}
	}

	// Return new cutscene instance
	return cutscene;
#endif
}

Bool CCutsceneTemplate::HasCameraSwitches() const
{
	Int32 camNum = GetCameraActorNum();
	if ( camNum != -1 )
	{
		ASSERT( (Int32)m_animations.Size() > camNum );

		if ( (Int32)m_animations.Size() > camNum && m_animations[camNum] && m_animations[camNum]->GetAnimation() )
		{

			return m_animations[camNum]->GetAnimation()->HasSwitches();
		}
	}

	return false;
}

Bool CCutsceneTemplate::HasBoundingBoxes() const
{
	for ( Uint32 i=0; i<m_animations.Size(); ++i )
	{
		if ( m_animations[i]->GetAnimation() && m_animations[i]->GetAnimation()->HasBoundingBox() )
		{
			return true;
		}
	}

	return false;
}

void CCutsceneTemplate::GetBoundingBoxes( TDynArray< Box >& boxes ) const
{
	for ( Uint32 i=0; i<m_animations.Size(); ++i )
	{
		if ( m_animations[i]->GetAnimation() && m_animations[i]->GetAnimation()->HasBoundingBox() )
		{
			boxes.PushBack( m_animations[i]->GetAnimation()->GetBoundingBox() );
		}
	}
}

Bool CCutsceneTemplate::HasFadeAfter() const
{
	return m_fadeAfter > 0.f ? true : false;
}

Bool CCutsceneTemplate::HasFadeBefore() const
{
	return m_fadeBefore > 0.f ? true : false;
}

Float CCutsceneTemplate::GetFadeBeforeDuration() const
{
	return m_fadeBefore;
}

Float CCutsceneTemplate::GetFadeAfterDuration() const
{
	return m_fadeAfter;
}

Float CCutsceneTemplate::GetDuration() const
{
	Float duration = 0.f;

	for ( Uint32 i=0; i<m_animations.Size(); ++i )
	{
		Float temp = m_animations[i]->GetDuration();

		if ( temp > duration )
		{
			duration = temp;
		}
	}

	return duration;
}

Int32 CCutsceneTemplate::GetCameraActorNum() const
{
	for ( Uint32 i=0; i<m_actorsDef.Size(); ++i )
	{
		if ( m_actorsDef[i].m_type == CAT_Camera )
		{
			return i;
		}
	}

	return -1;
}

Bool CCutsceneTemplate::GetCameraAnimation( const CSkeletalAnimation*& outAnimation, const CAnimatedComponent*& outCameraComponent, Int32& outBoneIndex ) const
{
	const CCutsceneTemplate* csTemplate = this;


	Int32 camNum = csTemplate->GetCameraActorNum();
	if ( camNum == -1 )
	{
		return false;
	}

	TDynArray< String > actorNames;
	csTemplate->GetActorsName( actorNames );
	const SCutsceneActorDef* cameraActorDef = csTemplate->GetActorDefinition( actorNames[ camNum ] );
	if ( cameraActorDef == nullptr )
	{
		return false;
	}

	const CSkeletalAnimationSetEntry* camAnim = nullptr;
	for ( auto anim : csTemplate->GetAnimations() )
	{
		if ( csTemplate->GetActorName( anim->GetName() ) == cameraActorDef->m_name )
		{
			camAnim = anim;
			break;
		}
	}

	const CAnimatedComponent* ac = nullptr;
	if ( const CEntityTemplate* cameraEntityTemplate = cameraActorDef->m_template.Get() )
	{
		for ( ComponentIterator< const CAnimatedComponent > iter( cameraEntityTemplate->GetEntityObject() ); iter; ++iter )
		{
			ac = *iter;
			break;
		}
	}

	if ( ac == nullptr || camAnim == nullptr )
	{
		return false;
	}

	const CSkeletalAnimation* anim = camAnim->GetAnimation();
	if ( anim == nullptr || ac->GetBonesNum() <= 0 || anim->GetTracksNum() <= SBehaviorGraphOutput::FTT_FOV )
	{
		return false;
	}

	outAnimation = camAnim->GetAnimation();
	outCameraComponent = ac;
	outBoneIndex = ac->GetBonesNum() - 1;

	return true;
}


Bool CCutsceneTemplate::IsStreamable() const
{
	return m_streamable;
}
#ifdef USE_HAVOK_ANIMATION
Bool CCutsceneTemplate::GetCameraPoseAtTime( Float time, TDynArray< hkQsTransform >& pose )
#else
Bool CCutsceneTemplate::GetCameraPoseAtTime( Float time, TDynArray< RedQsTransform >& pose )
#endif
{
	Int32 camNum = GetCameraActorNum();

	if ( camNum != -1 )
	{
		ASSERT( (Int32)m_animations.Size() > camNum );

		if ( (Int32)m_animations.Size() > camNum && m_animations[camNum] && m_animations[camNum]->GetAnimation() )
		{
			TDynArray< Float > tracks;
			return m_animations[camNum]->GetAnimation()->Sample( time, pose, tracks );
		}
	}

	return false;
}
#ifdef USE_HAVOK_ANIMATION
Bool CCutsceneTemplate::GetCameraInitialPose( TDynArray< hkQsTransform >& pose )
#else
Bool CCutsceneTemplate::GetCameraInitialPose( TDynArray< RedQsTransform >& pose )
#endif
{
	return GetCameraPoseAtTime( 0.f, pose );
}

#ifdef USE_HAVOK_ANIMATION
Bool CCutsceneTemplate::GetCameraFinalPose( TDynArray< hkQsTransform >& pose )
#else
Bool CCutsceneTemplate::GetCameraFinalPose( TDynArray< RedQsTransform >& pose )
#endif
{
	return GetCameraPoseAtTime( GetDuration(), pose );
}

Matrix CCutsceneTemplate::CalcActorCurrentPelvisPositionInTime( const String& inputActorName, Float time, const Matrix* offset ) const
{
	Bool finalPos = time == GetDuration();

	for ( Uint32 i=0; i<m_animations.Size(); ++i )
	{
		if ( !m_animations[i] )
		{
			continue;
		}

		String actorName = GetActorName( m_animations[i]->GetName() );

		if ( actorName != inputActorName )
		{
			continue;
		}

		const SCutsceneActorDef* def = GetActorDefinition( actorName );
		if ( !def )
		{
			ASSERT( def );
			continue;
		}

		if ( !IsRootComponent( m_animations[i]->GetName() ) )
		{
			continue;
		}

		if ( finalPos && def->UseFinalPosition() )
		{
			Matrix finalPos;
			if ( GetTagPointMatrix( def->m_finalPosition, finalPos ) )
			{
				return finalPos;
			}
			else
			{
				String tagStr = def->m_finalPosition.ToString();
				CS_WARN( TXT("Cutscene: Couldn't find actor's '%ls' final position - check tag '%ls'. Cutscene will use trajectory position for this actor."), actorName.AsChar(), tagStr.AsChar() );
			}
		}

		if ( m_animations[i]->GetAnimation() )
		{
			TDynArray< RedQsTransform > bones;
			TDynArray< Float > tracks;

			m_animations[i]->GetAnimation()->Sample( time, bones, tracks );

			//Int32 rootBone = 0;
			//Int32 trajBone = 1;
			Int32 pelvisBone = 9;

			if ( bones.Size() <= 9 )
			{
				return offset ? *offset : Matrix::IDENTITY;
			}

			//RedQsTransform rootTransform = bones[ rootBone ];
			//RedQsTransform trajTransform = bones[ trajBone ];
			RedQsTransform pelvisTransform = bones[ pelvisBone ];

			RedQsTransform out = pelvisTransform;

			Matrix csFinalMat;
			RedMatrix4x4 conversionMatrix = out.ConvertToMatrixNormalized(); 
			csFinalMat = reinterpret_cast< const Matrix& >( conversionMatrix );

			return Matrix::Mul( *offset, csFinalMat );
		}
	}

	return offset ? *offset : Matrix::IDENTITY; 
}

Bool CCutsceneTemplate::SampleActorPoseInTime( const String& inputActorName, Float time, SBehaviorGraphOutput& pose ) const
{
	for ( Uint32 i=0; i<m_animations.Size(); ++i )
	{
		if ( !m_animations[i] )
		{
			continue;
		}

		String actorName = GetActorName( m_animations[i]->GetName() );

		if ( actorName != inputActorName )
		{
			continue;
		}

		const SCutsceneActorDef* def = GetActorDefinition( actorName );
		if ( !def )
		{
			ASSERT( def );
			continue;
		}

		if ( !IsRootComponent( m_animations[i]->GetName() ) )
		{
			continue;
		}

		if ( m_animations[i]->GetAnimation() )
		{
			m_animations[i]->GetAnimation()->Sample( time, pose.m_numBones, pose.m_numFloatTracks, pose.m_outputPose, pose.m_floatTracks );

			return true;
		}
	}

	return false;
}

Matrix CCutsceneTemplate::GetActorPositionInTime( const String& inputActorName, Float time ,const Matrix* offset, Matrix* pelvisPos ) const
{
	Bool finalPos = time == GetDuration();

	for ( Uint32 i=0; i<m_animations.Size(); ++i )
	{
		if ( !m_animations[i] )
		{
			continue;
		}

		String actorName = GetActorName( m_animations[i]->GetName() );

		if ( actorName != inputActorName )
		{
			continue;
		}

		const SCutsceneActorDef* def = GetActorDefinition( actorName );
		if ( !def )
		{
			ASSERT( def );
			continue;
		}

		if ( !IsRootComponent( m_animations[i]->GetName() ) )
		{
			continue;
		}

		if ( finalPos && def->UseFinalPosition() )
		{
			Matrix finalPos;
			if ( GetTagPointMatrix( def->m_finalPosition, finalPos ) )
			{
				return finalPos;
			}
			else
			{
				String tagStr = def->m_finalPosition.ToString();
				CS_WARN( TXT("Cutscene: Couldn't find actor's '%ls' final position - check tag '%ls'. Cutscene will use trajectory position for this actor."), actorName.AsChar(), tagStr.AsChar() );
			}
		}

		if ( m_animations[i]->GetAnimation() )
		{
			TDynArray< RedQsTransform > bones;
			TDynArray< Float > tracks;
			m_animations[i]->GetAnimation()->Sample( time, bones, tracks );

			Int32 trajBone = 1;
			Int32 rootBone = 0;
			Int32 pelvisBone = 9;

			if ( bones.Size() <= 1 )
			{
				return offset ? *offset : Matrix::IDENTITY;
			}

			RedQsTransform trajTransform = bones[ trajBone ];
			RedQsTransform rootTransform = bones[ rootBone ];
			RedQsTransform movement; 
			movement.SetMul( rootTransform, trajTransform );

			if( pelvisPos && bones.Size() > 9 )
			{
				RedQsTransform pelvisTransform = bones[ pelvisBone ];
				RedQsTransform pelvisMov;
				pelvisMov.SetMul( rootTransform, pelvisTransform );
				RedMatrix4x4 conversionMatrix = pelvisMov.ConvertToMatrixNormalized(); 
				*pelvisPos = reinterpret_cast< const Matrix& >( conversionMatrix );
			}

			Matrix csFinalMat;
			RedMatrix4x4 conversionMatrix = movement.ConvertToMatrixNormalized(); 
			csFinalMat = reinterpret_cast< const Matrix& >( conversionMatrix );
			return Matrix::Mul( *offset, csFinalMat );
		}
	}

	return offset ? *offset : Matrix::IDENTITY; 
}

void CCutsceneTemplate::GetActorsPositionsAtTime( Float time, TDynArray< Matrix >& pos, const Matrix* offset, Bool onlyActors, TDynArray< String >* names ) const
{
	Bool finalPos = time == GetDuration();

	for ( Uint32 i=0; i<m_animations.Size(); ++i )
	{
		if ( !m_animations[i] )
		{
			continue;
		}

		String actorName = GetActorName( m_animations[i]->GetName() );
		const SCutsceneActorDef* def = GetActorDefinition( actorName );
		if ( !def )
		{
			ASSERT( def );
			continue;
		}

		if ( onlyActors && ( def->m_type != CAT_Actor || !IsRootComponent( m_animations[i]->GetName() ) ) )
		{
			continue;
		}

		if ( finalPos && def->UseFinalPosition() )
		{
			Matrix finalPos;
			if ( GetTagPointMatrix( def->m_finalPosition, finalPos ) )
			{
				pos.PushBack( finalPos );

				if ( names )
				{
					names->PushBack( m_animations[i]->GetName().AsString() );
				}
			}
			else
			{
				String tagStr = def->m_finalPosition.ToString();
				CS_WARN( TXT("Cutscene: Couldn't find actor's '%ls' final position - check tag '%ls'. Cutscene will use trajectory position for this actor."), actorName.AsChar(), tagStr.AsChar() );
			}
		}

		if ( m_animations[i]->GetAnimation() )
		{
#ifdef USE_HAVOK_ANIMATION
			TDynArray< hkQsTransform > bones;
#else
			TDynArray< RedQsTransform > bones;
#endif
			TDynArray< Float > tracks;
			m_animations[i]->GetAnimation()->Sample( time, bones, tracks );

			Int32 trajBone = 1;
			Int32 rootBone = 0;

			if ( bones.Size() <= 1 )
			{
				if ( names )
				{
					names->PushBack( m_animations[i]->GetName().AsString() );
				}

				offset ? pos.PushBack( *offset ) : pos.PushBack( Matrix::IDENTITY );
				continue;
			}
#ifdef USE_HAVOK_ANIMATION
			hkQsTransform trajTransform = bones[ trajBone ];
			hkQsTransform rootTransform = bones[ rootBone ];
			hkQsTransform movement; 
			movement.setMul( rootTransform, trajTransform );

			Matrix csFinalMat;
			HavokTransformToMatrix_Renormalize( movement, &csFinalMat );
#else
			RedQsTransform trajTransform = bones[ trajBone ];
			RedQsTransform rootTransform = bones[ rootBone ];
			RedQsTransform movement; 
			movement.SetMul( rootTransform, trajTransform );

			Matrix csFinalMat;
			RedMatrix4x4 conversionMatrix = movement.ConvertToMatrixNormalized();
			csFinalMat = reinterpret_cast< const Matrix& >( conversionMatrix );
#endif
			if ( offset )
			{
				csFinalMat = Matrix::Mul( *offset, csFinalMat );
			}

			pos.PushBack( csFinalMat );

			if ( names )
			{
				names->PushBack( m_animations[i]->GetName().AsString() );
			}
		}
	}
}

Bool CCutsceneTemplate::GetTagPointMatrix( const TagList& tag, Matrix& mat ) const
{
	TDynArray< CNode* > nodes;
	if ( !GGame || !GGame->GetActiveWorld() )
	{
		return false;
	}
	GGame->GetActiveWorld()->GetTagManager()->CollectTaggedNodes( tag, nodes );

	if ( nodes.Size() > 0 )
	{
		CNode* node = nodes[0];
		mat = node->GetWorldRotation().ToMatrix();
		mat.SetTranslation( node->GetWorldPositionRef() );
		return true;
	}

	return false;
}

void CCutsceneTemplate::GetActorsInitialPositions( TDynArray< Matrix >& pos, const Matrix* offset, Bool onlyActors, TDynArray< String >* names ) const
{
	GetActorsPositionsAtTime( 0.f, pos, offset, onlyActors, names );
}

void CCutsceneTemplate::GetActorsFinalPositions( TDynArray< Matrix >& pos, const Matrix* offset, Bool onlyActors, TDynArray< String >* names ) const
{
	GetActorsPositionsAtTime( GetDuration(), pos, offset, onlyActors, names );
}

String CCutsceneTemplate::GetComponentName( const CName& animation )
{
	return animation.AsString().StringAfter( TXT(":") ).StringBefore( TXT(":") );
}

String CCutsceneTemplate::GetActorName( const CName& animation )
{
	return animation.AsString().StringBefore( TXT(":") );
}

Bool CCutsceneTemplate::IsRootComponent( const CName& animation )
{
	return GetComponentName( animation ) == TXT("Root") ? true : false;
}

Bool CCutsceneTemplate::IsMimicComponent( const CName& animation )
{
	return GetComponentName( animation ) == TXT("face") ? true : false;
}

Bool CCutsceneTemplate::SaveFileWithThisCutscene( const String& fileName )
{
	if ( IsFileUsedThisCutscene( fileName ) == false )
	{
		if ( MarkModified() )
		{
			m_usedInFiles.PushBack( fileName );
		}
		else
		{
			return false;
		}
	}

	return true;
}

Bool CCutsceneTemplate::ReleaseFileWithThisCutscene( const String& fileName )
{
	if ( IsFileUsedThisCutscene( fileName ) )
	{
		if ( MarkModified() )
		{
			m_usedInFiles.Remove( fileName );
		}
		else
		{
			return false;
		}
	}

	return true;
}

Bool CCutsceneTemplate::IsFileUsedThisCutscene( const String& fileName ) const
{
	return m_usedInFiles.Exist( fileName );
}

const TDynArray< String >& CCutsceneTemplate::GetFilesWithThisCutscene() const
{
	return m_usedInFiles;
}

void CCutsceneTemplate::AddEvent( CExtAnimEvent* event )
{ 
	m_animEvents.Insert( event );
	// Commented until resave!
	//MarkModified();
}

void CCutsceneTemplate::RemoveEvent( CExtAnimEvent* event )
{ 
	m_animEvents.Remove( event );
	MarkModified();
}

void CCutsceneTemplate::GetEventsForAnimation( const CName& animName, TDynArray< CExtAnimEvent* >& events )
{
	if( animName == CUTSCENE_ANIMATION_NAME )
	{
		// Get common events
		events.PushBack( m_animEvents );
	}
	else
	{
		CSkeletalAnimationSetEntry* animation = FindAnimation( animName );
		if( animation != NULL )
		{
			animation->GetAllEvents( events );
		}
	}
}

const CResource* CCutsceneTemplate::GetParentResource() const
{
	return this;
}

void CCutsceneTemplate::GetEventsByTime( Float prevTime, Float currTime, TDynArray< CAnimationEventFired >& events )
{
	if( currTime < prevTime )
	{
		// Todo: Need to check if something can cause this condition to be true even if we play cutscene the normal way
		RED_WARNING( false, "Backward cutscene playing is not yet supported. Consult mcinek" );
		return;
	}

	if( m_animEvents.Empty() )
	{
		return;
	}

	// Iterate through all events in file
	for( TSortedArray< CExtAnimEvent*, CExtAnimEvent::Sorter >::const_iterator eventIter = m_animEvents.Begin();
		eventIter != m_animEvents.End(); ++eventIter )
	{
		CExtAnimEvent* event = *eventIter;
		ASSERT( event != NULL );

		// Special case for duration events
		if( IsType< CExtAnimDurationEvent>( event ) )
		{
			CExtAnimDurationEvent* durEvent = static_cast< CExtAnimDurationEvent* >( event );

			Float startTime = durEvent->GetStartTime();
			Float endTime = Min( startTime + durEvent->GetDuration(), GetDuration() );

			if( prevTime <= startTime && currTime > startTime )
			{
				// Event just started
				CAnimationEventFired fired;
				fired.m_alpha = 1.0f;
				fired.m_extEvent = durEvent;
				fired.m_type = AET_DurationStart;
				fired.m_animInfo.m_eventName = event->GetEventName();
				fired.m_animInfo.m_animation = nullptr;
				fired.m_animInfo.m_localTime = currTime;
				fired.m_animInfo.m_eventEndsAtTime = endTime;
				fired.m_animInfo.m_eventDuration = durEvent->GetDuration();

				events.PushBack( fired );
			}
			if ( prevTime > startTime && currTime < endTime  )
			{
				// Event is still fired
				CAnimationEventFired fired;
				fired.m_alpha = 1.0f;
				fired.m_extEvent = durEvent;
				fired.m_type = AET_Duration;
				fired.m_animInfo.m_eventName = event->GetEventName();
				fired.m_animInfo.m_animation = nullptr;
				fired.m_animInfo.m_localTime = currTime;
				fired.m_animInfo.m_eventEndsAtTime = endTime;
				fired.m_animInfo.m_eventDuration = durEvent->GetDuration();

				events.PushBack( fired );
			}
			if( prevTime <= endTime && currTime > endTime )
			{
				// Event just ended
				CAnimationEventFired fired;
				fired.m_alpha = 1.0f;
				fired.m_extEvent = durEvent;
				fired.m_type = AET_DurationEnd;
				fired.m_animInfo.m_eventName = event->GetEventName();
				fired.m_animInfo.m_animation = nullptr;
				fired.m_animInfo.m_localTime = currTime;
				fired.m_animInfo.m_eventEndsAtTime = endTime;
				fired.m_animInfo.m_eventDuration = durEvent->GetDuration();

				events.PushBack( fired );
			}
		}
		// Normal events
		else if( event->GetStartTime() >= prevTime && event->GetStartTime() < currTime )
		{
			// Add event to the list
			CAnimationEventFired fired;
			fired.m_alpha = 1.0f;
			fired.m_extEvent = event;
			fired.m_type = AET_Tick;
			fired.m_animInfo.m_eventName = event->GetEventName();
			fired.m_animInfo.m_animation = nullptr;
			fired.m_animInfo.m_localTime = currTime;
			fired.m_animInfo.m_eventEndsAtTime = currTime;
			fired.m_animInfo.m_eventDuration = 0.f;

			events.PushBack( fired );
		}
	}
}

void CCutsceneTemplate::OnPostLoad()
{
	TBaseClass::OnPostLoad();


#ifndef NO_EDITOR
	for ( Uint32 i=0; i<m_actorsDef.Size(); ++i )
	{
		SCutsceneActorDef& def = m_actorsDef[i];
		if ( def.m_type == CAT_None && def.m_template.Get() )
		{
			def.m_type = ExtractActorTypeFromTemplate( def.m_template.Get() );
		}
	}
#endif

}


void CCutsceneTemplate::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );

	// Serialize events
	CExtAnimEvent::Serialize( m_animEvents, file );
}

TDynArray<Float> CCutsceneTemplate::GetCameraCuts() const 
{
	TDynArray<Float> res;
	if( m_animations.Size() > 0 )
	{
		const CSkeletalAnimationSetEntry* animation = m_animations[0];
		const CSkeletalAnimation* sAnim = animation->GetAnimation();
		const IAnimationBuffer* animBuf = sAnim->GetAnimBuffer();
		if ( const CAnimationBufferMultipart* multiBuf = Cast< const CAnimationBufferMultipart >( animBuf ) )
		{
			const TDynArray< Uint32 >& frames = multiBuf->GetPartsFirstFrames(); 
			for ( const Uint32& frame : frames )
			{
				res.PushBack( 1.f / sAnim->GetFramesPerSecond() * frame );				
			}
		}
	}
	return res;
}

Bool CCutsceneTemplate::HasCameraCut( Float prevTime, Float currTime ) const
{
	if( m_animations.Size() > 0 )
	{
		const CSkeletalAnimationSetEntry* animation = m_animations[0];
		const CSkeletalAnimation* sAnim = animation->GetAnimation();
		const IAnimationBuffer* animBuf = sAnim->GetAnimBuffer();
		if ( const CAnimationBufferMultipart* multiBuf = Cast< const CAnimationBufferMultipart >( animBuf ) )
		{
			const TDynArray< Uint32 >& frames = multiBuf->GetPartsFirstFrames(); 
			for ( const Uint32& frame : frames )
			{
				const Float frameTime = frame / sAnim->GetFramesPerSecond();
				if( ( frameTime > prevTime && frameTime <= currTime ) ||
					( frameTime == 0.0f && currTime == 0.0f ) )
				{
					return true;
				}			
			}
		}
	}
	return false;
}

void CCutsceneTemplate::GetRequiredTemplates( TDynArray< TSoftHandle< CResource > >& requiredTemplates )
{
	for ( TDynArray< SCutsceneActorDef >::iterator actorDefIter = m_actorsDef.Begin();
		actorDefIter != m_actorsDef.End(); ++actorDefIter )
	{
		requiredTemplates.PushBackUnique( TSoftHandle< CResource >( actorDefIter->m_template.GetPath() ) );
	}

	// Preload effect resources (particle systems)
	for( const CFXDefinition* fxDef : m_effects )
	{
		fxDef->PrefetchResources(requiredTemplates);
	}

	// Get all cutscene effects
	TDynArray< const CExtAnimCutsceneEffectEvent* > events;
	GetEventsOfType( events );

	for( TDynArray< const CExtAnimCutsceneEffectEvent* >::const_iterator eventIter = events.Begin(); eventIter != events.End(); ++eventIter )
	{
		const CExtAnimCutsceneEffectEvent* event = *eventIter;
		ASSERT( event != NULL );

		// Get entity template

		const String& path =  event->GetEntityTemplateHandle().GetPath();
		if ( !path.Empty() )
		{
			requiredTemplates.PushBack( TSoftHandle< CResource >( path ) );
		}
	}

	for ( Uint32 i = 0; i < m_resourcesToPreloadManuallyPaths.Size(); ++i )
	{
		if ( !m_resourcesToPreloadManuallyPaths[i].Empty() )
		{
			requiredTemplates.PushBack( TSoftHandle< CResource >( m_resourcesToPreloadManuallyPaths[i] ) );
		}
	}
}

#ifndef NO_RESOURCE_COOKING

void CCutsceneTemplate::OnCook( class ICookerFramework& cooker )
{
#ifndef NO_EDITOR
	m_resourcesToPreloadManuallyPaths.Clear();

	for ( Uint32 i = 0; i < m_resourcesToPreloadManually.Size(); ++i )
	{
		if ( m_resourcesToPreloadManually[i] )
		{
			const String depotPath( m_resourcesToPreloadManually[i]->GetDepotPath() );
			if ( !depotPath.Empty() )
			{
				m_resourcesToPreloadManuallyPaths.PushBack( depotPath );

				// capture this dependency
				cooker.ReportSoftDependency( depotPath );
			}
		}
	}
#endif
}

#endif

#ifndef NO_EDITOR
void CCutsceneTemplate::FillSkeletonsInAnimationsBasingOnTemplates()
{
	for ( Uint32 i=0; i<m_animations.Size(); ++i )
	{
		if ( !m_animations[i] )
		{
			continue;
		}

		String actorName = GetActorName( m_animations[i]->GetName() );
		const SCutsceneActorDef* def = GetActorDefinition( actorName );
		if ( !def )
		{
			ASSERT( def );
			continue;
		}

		if ( CSkeletalAnimation* anim = m_animations[i]->GetAnimation() )
		{
			if ( ! anim->GetSkeleton() &&
				 def->m_template.Get() )
			{
				if ( const CEntity * entity = def->m_template.Get()->GetEntityObject() )
				{
					if ( entity->GetRootAnimatedComponent() )
					{
						anim->SetSkeleton( entity->GetRootAnimatedComponent()->GetSkeleton() );
					}
				}
			}
		}
	}
}
#endif


CFXDefinition* CCutsceneTemplate::AddEffect( const String& effectName )
{
	// Make sure such effect does not exist
	CName realEffectName( effectName.AsChar() );
	if ( FindEffect( realEffectName ) )
	{
		WARN_ENGINE( TXT("Effect '%ls' already exists in '%ls'. Cannot create new one."), effectName.AsChar(), GetFriendlyName().AsChar() );
		return NULL;
	}

	// Create the effect definition
	CFXDefinition* fx = new CFXDefinition( this, realEffectName );
	if ( !fx )
	{
		WARN_ENGINE( TXT("Unable to create effect '%ls' in '%ls'."), effectName.AsChar(), GetFriendlyName().AsChar() );
		return NULL;
	}

	// Add to list of effects
	m_effects.PushBack( fx );
	return fx;
}

Bool CCutsceneTemplate::AddEffect( CFXDefinition* effect )
{
	// Make sure such effect does not exist
	if ( FindEffect( effect->GetName() ) )
	{
		WARN_ENGINE( TXT("Effect '%ls' already exists in '%ls'. Cannot paste new one."), effect->GetName().AsString().AsChar(), GetFriendlyName().AsChar() );
		return false;
	}

	effect->SetParent( this );

	// Add to list of effects
	m_effects.PushBack( effect );
	return true;
}

Bool CCutsceneTemplate::RemoveEffect( CFXDefinition* effect )
{
	for ( ObjectIterator< CEntity > it; it; ++it )
	{
		CEntity* entity = (*it);
		if ( entity->IsPlayingEffect( effect ) )
		{
			entity->DestroyAllEffects();
		}
	}

	// Delete effect
	m_effects.Remove( effect );
	return true;
}

CFXDefinition* CCutsceneTemplate::FindEffect( const CName& effectName ) const
{
	// Search by name
	for ( Uint32 i=0; i<m_effects.Size(); i++ )
	{
		CFXDefinition* fx = m_effects[i];
		if ( fx && fx->GetName() == effectName )
		{
			return fx;
		}
	}

	// Not found
	return nullptr;
}

Bool CCutsceneTemplate::HasEffect( const CName& effectName ) const
{
	// Search by name
	for ( Uint32 i=0; i<m_effects.Size(); i++ )
	{
		CFXDefinition* fx = m_effects[i];
		if ( fx && fx->GetName() == effectName )
		{
			return true;
		}
	}

	// Not found
	return false;
}

void CCutsceneTemplate::GetActorsVoicetags( TDynArray< CName >& voiceTags )
{
	for ( Uint32 i=0; i<m_actorsDef.Size(); ++i )
	{
		if ( m_actorsDef[ i ].m_type == CAT_Actor && m_actorsDef[i].m_voiceTag )
		{
			voiceTags.PushBack( m_actorsDef[i].m_voiceTag );
		}
	}
}

Bool CCutsceneTemplate::HasActor( CName actorId, const TagList* tags ) const
{
	for ( const SCutsceneActorDef& actorDef : m_actorsDef )
	{
		if ( actorDef.m_type == CAT_Actor )
		{
			const Bool canMatchByVoicetag = actorId && actorDef.m_voiceTag;
			const Bool canMatchByTags = !actorDef.m_tag.Empty() && tags && !tags->Empty();

			// Can not match by any criteria?

			if ( !canMatchByVoicetag && !canMatchByTags )
			{
				continue;
			}

			// Try to match by voicetag

			if ( canMatchByVoicetag && actorDef.m_voiceTag != actorId )
			{
				continue;
			}

			// Try to match by tags

			if ( canMatchByTags && !TagList::MatchAll( actorDef.m_tag, *tags ) )
			{
				continue;
			}

			// Match successful!

			return true;
		}
	}
	return false;
}

#ifdef DEBUG_CUTSCENES
#pragma optimize("",on)
#endif
