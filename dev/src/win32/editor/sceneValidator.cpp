#include "build.h"
#include "sceneValidator.h"
#include "..\..\common\game\storySceneSection.h"
#include "..\..\common\game\storyScene.h"
#include "..\..\common\game\storySceneElement.h"
#include "..\..\common\game\storySceneEvent.h"
#include "..\..\common\game\storySceneItems.h"
#include "..\..\common\engine\animatedComponent.h"
#include "..\..\common\engine\behaviorGraphStack.h"
#include "..\..\common\engine\mimicComponent.h"
#include "..\..\common\game\itemIterator.h"
#include "..\..\common\game\storySceneCutsceneSection.h"
#include "..\..\common\game\storySceneControlPartsUtil.h"
#include "..\..\common\game\storySceneInput.h"

#ifdef DEBUG_SCENES_2
#pragma optimize("",off)
#endif

#define VALIDATOR_ERROR( s ) context.result.m_messages.PushBack( SValidationOutputMessage( Error, s ) ); 
#define VALIDATOR_WARNING( s ) context.result.m_messages.PushBack( SValidationOutputMessage( Warning, s ) );
#define VALIDATOR_INFO( s ) context.result.m_messages.PushBack( SValidationOutputMessage( Info, s ) );

#define NULL_SAFE_CALL( val, fun, msg, type )									\
if ( val )																		\
{																				\
	fun;																		\
}																				\
else																			\
{																				\
	context.result.m_messages.PushBack( SValidationOutputMessage( type, msg ) );	\
}																				\

CEdSceneValidator::CEdSceneValidator()
{
}

CEdSceneValidator::SValidationOutput CEdSceneValidator::Process( const CStoryScene* scene ) const
{
	SValidationContext context;
	context.scene = scene;

	ProcessActorDefDuplication( context );

	const TDynArray< CStorySceneActor* >&	actorDef = scene->GetSceneActorsDefinitions();
	for ( const CStorySceneActor* actor : actorDef )
	{		
		NULL_SAFE_CALL( actor, ProcessActorDef( context, actor ), TXT("NULL actor definition"), Error )
	}
	const TDynArray< CStorySceneProp* >&	propsDef = scene->GetScenePropDefinitions();
	for ( const CStorySceneProp* prop : propsDef )
	{
		NULL_SAFE_CALL( prop, ProcessPropDef( context, prop ), TXT("NULL prop definition"), Error )
	}
	const TDynArray< CStorySceneLight* >&	lightDef = scene->GetSceneLightDefinitions();
	for ( const CStorySceneLight* light : lightDef )
	{
		NULL_SAFE_CALL( light, ProcessLightDef( context, light ), TXT("NULL light definition"), Error )	
	}

	const TDynArray< CStorySceneDialogsetInstance* >& dialogsets = scene->GetDialogsetInstances();
	for ( const CStorySceneDialogsetInstance* dialogset : dialogsets )
	{
		NULL_SAFE_CALL( dialogset, ProcessDialogset( context, dialogset ), TXT("NULL dialogset instance"), Error )			
		if( dialogset )
		{
			const TDynArray< CStorySceneDialogsetSlot* >&slots = dialogset->GetSlots();
			for ( const CStorySceneDialogsetSlot* slot : slots )
			{
				NULL_SAFE_CALL( slot, ProcessDialogsetSlot( context, slot ), TXT("NULL dialogset slot"), Error )				
			}
		}
	}

	Uint32 num = scene->GetNumberOfSections();
	for ( Uint32 i = 0; i < num; i++ )
	{
		const CStorySceneSection* section = scene->GetSection( i );
		NULL_SAFE_CALL( section, ProcessSection( context, section ), TXT("NULL scene section"), Warning )

		Uint32 num = section->GetNumberOfElements();
		for ( Uint32 i = 0; i < num; ++i )
		{
			const CStorySceneElement* el = section->GetElement( i );
			NULL_SAFE_CALL( el, ProcessElement( context, el ), TXT("NULL scene element"), Warning )
		}

		const TDynArray< CStorySceneEvent* >& evts = section->GetEventsFromAllVariants();
		for ( Uint32 i = 0; i < evts.Size(); ++i )
		{
			const CStorySceneEvent* evt = evts[ i ];
			NULL_SAFE_CALL( evt, ProcessEvent( context, evt ), TXT("NULL scene event"), Warning )			
		}
	}

	return context.result;
}


 /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CEdSceneValidator::ProcessSection( SValidationContext& context, const CStorySceneSection* section ) const
{
	const CName& dialogsetName = section->GetDialogsetChange();
	if ( dialogsetName )
	{
		if( !context.dialogsetNames.Exist( dialogsetName ) )
		{
			VALIDATOR_ERROR( String::Printf( TXT("Invalid dialogset name in section %s"), section->GetName().AsChar() ) )
		}
	}



//// CStorySceneVideoSection
//	String	m_videoFileName;
//	String	m_eventDescription;
//	Bool	m_suppressRendering;
//
//
//// CStorySceneCutsceneSection
//	THandle< CCutsceneTemplate >	m_cutscene;
//	TagList							m_point;
//	Bool							m_looped;
//	Bool							m_clearActorsHands;

	if ( const CStorySceneCutsceneSection* csSection = Cast< const CStorySceneCutsceneSection >( section ) )
	{
		if( CCutsceneTemplate* temp = csSection->GetCsTemplate() )
		{
			// 1. Check animatons
			const TDynArray< CSkeletalAnimationSetEntry* > animations = temp->GetAnimations();
	
			Uint32 size = animations.Size();
			for ( Uint32 i=0; i< size; ++i )
			{
				CSkeletalAnimationSetEntry* anim = animations[i];
				if ( anim && anim->GetAnimation() )
				{
					TDynArray< AnimQsTransform > bonesOut;
					TDynArray< AnimFloat > tracksOut;
					anim->GetAnimation()->Sample( temp->GetDuration(), bonesOut, tracksOut );

					Int32 trajBone = 1;
					Int32 rootBone = 0;

					if ( bonesOut.Size() > 1 )
					{
						RedQsTransform trajTransform = bonesOut[ trajBone ];
						RedQsTransform rootTransform = bonesOut[ rootBone ];
						RedQsTransform movement; 
						movement.SetMul( rootTransform, trajTransform );

						Matrix csFinalMat;
						RedMatrix4x4 conversionMatrix = movement.ConvertToMatrixNormalized(); 
						csFinalMat = reinterpret_cast< const Matrix& >( conversionMatrix );
						
						String actorName = temp->GetActorName( anim->GetName() );
						SCutsceneActorDef* actorDef = temp->GetActorDefinition( actorName );
						Bool mimicAnim = anim->GetName().AsString().ContainsSubstring( TXT("face") ) || anim->GetName().AsString().ContainsSubstring( TXT("mimic") );
						if( !mimicAnim && Vector::Near3( csFinalMat.GetTranslation(), Vector::ZERO_3D_POINT ) && actorDef && actorDef->m_type == CAT_Actor && !actorDef->UseFinalPosition() )
						{
							VALIDATOR_WARNING( String::Printf( TXT("Cutscene ( %s ) final trajectory for actor ( %s ) animation ( %s ) is at point (0, 0, 0) "), csSection->GetName().AsChar(), anim->GetName().AsChar(), actorName.AsChar() ) )
						}
					}
				}
			}

			// 2. Check actors
			TDynArray< CName > csActors;
			temp->GetActorsVoicetags( csActors );
			for ( Uint32 i=0; i<csActors.Size(); ++i )
			{
				SCENE_ASSERT( csActors[ i ] );
				const CStorySceneActor* ssActor = context.scene->GetActorDescriptionForVoicetag( csActors[ i ] );
				if ( !ssActor )
				{
					VALIDATOR_ERROR( String::Printf( TXT("Cutscene ( %ls ) actor '%ls' is not in story scene actors description list"), csSection->GetName().AsChar(), csActors[ i ].AsChar() ) )
				}
			}
		}
	}

//
//	TDynArray< SCutsceneActorOverrideMapping >	m_actorOverrides;
//
////CStorySceneSection 
//	String									m_sectionName;			//!< Name of the section
//	TagList									m_tags;					//!< DEPRECATED Tags used by section
//
//	CStorySceneChoice*						m_choice;				//!< Section choice. It doesn't exist in m_sceneElements list but it's
//	//!< also a scene element and it's always treated as a last scene element
//	//!< of a section. May be nullptr - this means section has no choice.
//
//	Uint32									m_sectionId;
//	Bool	m_isGameplay;
//	Bool	m_isImportant;
//	Bool	m_allowCameraMovement;
//	Bool	m_hasCinematicOneliners;
//	Bool	m_fadeInAtBeginning;
//	Bool	m_fadeOutAtEnd;
//	Float	m_interceptRadius;
//	Float	m_interceptTimeout;
//	Bool	m_pauseInCombat;
//	Bool	m_canBeSkipped;
//	Bool	m_canHaveLookats;
//	Uint32	m_numberOfInputPaths;
//	CName	m_dialogsetChangeTo;
//	Bool	m_forceDialogset;
//	Int32	m_contexID;
//	TDynArray< CStorySceneLinkElement* >	m_inputPathsElements;
//	TDynArray< CStorySceneSection* >		m_interceptSections;
//	TDynArray< CStorySceneElement* >		m_sceneElements;		//!< List of elements contained by section, in order of their appearance.
//	TDynArray< CStorySceneEvent* >			m_events;				//!< List of events associated with elements, in no special order.
//	THashMap< CGUID, CStorySceneEvent* >	m_guidToEvent;			//!< Maps event GUID to event ptr.

}

void CEdSceneValidator::ProcessSceneGraph( SValidationContext& context, const CStorySceneGraph* graph ) const
{
	//sockets 
	//dead ends

	//TDynArray< CGraphBlock* >	m_graphBlocks;			//!< Blocks in the graph
	//Bool CheckConsistency( SBrokenSceneGraphInfo* graphInfo = NULL, Bool doNotShowInfoIfItsOK = true, Bool doNotShowInfoIfItsNotOK = true );
}

void CEdSceneValidator::ProcessDialogset( SValidationContext& context, const CStorySceneDialogsetInstance* dialogset ) const
{
	if ( !dialogset->GetName() )
	{	
		VALIDATOR_ERROR( String::Printf( TXT("Empty dialogset name") ) )
	}
	else if( !context.dialogsetNames.PushBackUnique( dialogset->GetName() ) )
	{
		VALIDATOR_ERROR( String::Printf( TXT("Duplicated dialogset name %s"), dialogset->GetName().AsChar() ) )
	}
}


void CEdSceneValidator::ProcessDialogsetSlot( SValidationContext& context, const CStorySceneDialogsetSlot* slot ) const
{
	//slot;
	//Uint32							m_slotNumber;
	//CName							m_slotName;
	//EngineTransform					m_slotPlacement;
	//CName							m_actorName;
	//Bool							m_actorVisibility;
	//TDynArray<CStorySceneAction*>	m_setupAction;

	//CName							m_actorStatus;
	//CName							m_actorEmotionalState;
	//CName							m_actorPoseName;

	//CName							m_actorMimicsEmotionalState;
	//CName							m_actorMimicsLayer_Eyes;
	//CName							m_actorMimicsLayer_Pose;
	//CName							m_actorMimicsLayer_Animation;
	//CName							m_forceBodyIdleAnimation;
	//Float							m_forceBodyIdleAnimationWeight;
	//Float							m_actorMimicsLayer_Pose_Weight;

	////zduploikowane GUIDY do dialogsetow
	//CGUID							m_ID;
}

void CEdSceneValidator::ProcessActorDefDuplication( SValidationContext& context ) const
{
	const TDynArray< CStorySceneActor* >& actorDef = context.scene->GetSceneActorsDefinitions();
	const Uint32 num = actorDef.Size();
	for ( Uint32 i=0; i<num; ++i )
	{
		const CStorySceneActor* actorA = actorDef[ i ];
		if ( !actorA )
		{
			continue;
		}

		if ( actorA->m_dontSearchByVoicetag ) // m_dontSearchByVoicetag case
		{
			for ( Uint32 j=i+1; j<num; ++j )
			{
				const CStorySceneActor* actorB = actorDef[ j ];
				if ( !actorB )
				{
					continue;
				}

				SCENE_ASSERT( actorA != actorB );

				// 1. m_actorTags == m_actorTags case
				if ( actorB->m_dontSearchByVoicetag )
				{
					if ( actorA->m_actorTags == actorB->m_actorTags )
					{
						VALIDATOR_ERROR( String::Printf( TXT("Actors %ls and %ls have the same tags and 'dontSearchByVoicetag' option checked" ), actorA->m_id.AsChar(), actorB->m_id.AsChar() ) );
						continue;
					}
				}

				// 2. m_actorTags => voicetag == other voicetag case
				{
					TSoftHandle< CEntityTemplate > handleB = actorB->m_entityTemplate;
					if ( CEntityTemplate* templB = handleB.Get() )
					{
						if ( const CActor* actorPtrB = Cast< CActor >( templB->GetEntityObject() ) )
						{
							if ( actorA->m_actorTags == actorPtrB->GetTags() )
							{
								VALIDATOR_ERROR( String::Printf( TXT("Actor %ls has 'dontSearchByVoicetag' option checked and his tags match to other scene actor %ls" ), actorA->m_id.AsChar(), actorB->m_id.AsChar() ) );
							}
						}
					}
				}
			}
		}
	}
}

void CEdSceneValidator::ProcessActorDef( SValidationContext& context, const CStorySceneActor* actor ) const
{
	TSoftHandle< CEntityTemplate > handle =	actor->m_entityTemplate;
	CEntityTemplate* templ = handle.Get();

	if( !actor->m_id )
	{
		VALIDATOR_ERROR( String::Printf( TXT("One of the actors does not have Id ( voicetag )" ) ) )
	}
	else if( !context.usedIds.PushBackUnique( actor->m_id ) )
	{
		VALIDATOR_ERROR( String::Printf( TXT("Duplicated id for actor %s"), actor->m_id.AsChar() ) )
	}

	if ( actor->m_dontSearchByVoicetag && actor->m_actorTags.Empty() )
	{	
		VALIDATOR_INFO( String::Printf( TXT("dontSearchByVoicetag and no Tags to search with for %s. Actor will be spawned"), actor->m_id.AsChar() ) )
	}

	if( templ )
	{
		for( CName appearance : actor->m_appearanceFilter )
		{
			if( !templ->GetAppearance( appearance, true ) )
			{
				VALIDATOR_ERROR( String::Printf( TXT("Entity template does not have specifed appearance ( %s ) for actor ( %s )"), appearance.AsChar(), actor->m_id.AsChar() ) )
			}
			if( !templ->GetEnabledAppearancesNames().Exist( appearance ) )
			{
				VALIDATOR_ERROR( String::Printf( TXT("Appearance ( %s ) for actor ( %s ) is not enabled"), appearance.AsChar(), actor->m_id.AsChar() ) )
			}
			if ( templ->GetApperanceVoicetag( appearance ) != actor->m_id && !actor->m_dontSearchByVoicetag )
			{
				VALIDATOR_ERROR( String::Printf( TXT("Voicetag for appearance ( %s ) doesnt match actor id ( %s ) and dontSearchByVoicetag = false"), appearance.AsChar(), actor->m_id.AsChar() ) )
			}
			//scene->GetActorSpawnDefinition(  );
		}

		if( const CActor* cactor = Cast< CActor >( templ->GetEntityObject() ) )
		{
			if( !cactor->GetRootAnimatedComponent() )
			{
				VALIDATOR_WARNING( String::Printf( TXT("No animated component in actor entity for actor %s"), actor->m_id.AsChar() ) )
			}
			if( !cactor->GetMovingAgentComponent() )
			{
				VALIDATOR_ERROR( String::Printf( TXT("Actor does not have moving agent component. For actor %s"), actor->m_id.AsChar() ) )
			}
			EntityWithItemsComponentIterator<CMimicComponent> iter( cactor );
			if( !iter )
			{
				context.actorsWithNoMimic.PushBack( actor->m_id );
			}
		}
		else
		{
			VALIDATOR_ERROR( String::Printf( TXT("Specifed entity template in not an actor for id %s"), actor->m_id.AsChar() ) )
		}
	}
	else
	{
		VALIDATOR_ERROR( String::Printf( TXT("No entity template given for actor %s"), actor->m_id.AsChar() ) )
	}
}

void CEdSceneValidator::ProcessPropDef( SValidationContext& context, const CStorySceneProp* prop ) const
{
	TSoftHandle< CEntityTemplate > handle =	prop->m_entityTemplate;
	CEntityTemplate* templ = handle.Get();

	if( !prop->m_id )
	{
		VALIDATOR_ERROR( String::Printf( TXT("One of the props does not have Id" ) ) )
	}
	else if( !context.usedIds.PushBackUnique( prop->m_id ) )
	{
		VALIDATOR_ERROR( String::Printf( TXT("Duplicated id for prop %s"), prop->m_id.AsChar() ) )
	}

	if ( !templ )
	{
		VALIDATOR_ERROR( String::Printf( TXT("No entity template for prop %s"), prop->m_id.AsChar() ) )
	}
	else if( const CEntity* ent = templ->GetEntityObject() )
	{
		if( const CActor* cactor = Cast< CActor >( ent ) )
		{
			if( !cactor->GetMovingAgentComponent() )
			{
				VALIDATOR_ERROR( String::Printf( TXT("Prop is an actor and doesnt have moving agent component. For prop %s"), prop->m_id.AsChar() ) )
			}
		}

		if( CAnimatedComponent* ac = ent->GetRootAnimatedComponent() )
		{
			if( prop->m_forceBehaviorGraph )
			{
				if( !ac->GetBehaviorStack()->HasInstance( prop->m_forceBehaviorGraph ) )
				{
					VALIDATOR_ERROR( String::Printf( TXT("Prop does not have %s beh graph instance ( forceBehaviorGraph ). For prop %s"), prop->m_forceBehaviorGraph.AsChar(), prop->m_id.AsChar() ) )
				}
			}			
			if( ac->HasRagdoll() )
			{
				VALIDATOR_ERROR( String::Printf( TXT("Prop has a ragdoll. Ragdols in prop entities are currenly not supported. For prop %s"), prop->m_id.AsChar() ) )
			}
		}
		else if( prop->m_forceBehaviorGraph || prop->m_resetBehaviorGraph )
		{			
			VALIDATOR_WARNING( String::Printf( TXT("No animated component for prop %s and beh flags used"), prop->m_id.AsChar() ) )
		}
		else
		{
			//info no anim component			
		}
		if ( !ent->FindComponent<CMimicComponent>() )
		{
			if( prop->m_useMimics )
			{
				//error no mimic component and mimic flag checked
				VALIDATOR_WARNING( String::Printf( TXT("No mimic component for prop %s and useMimics flag used"), prop->m_id.AsChar() ) )
			}
			context.actorsWithNoMimic.PushBack( prop->m_id );
		}		
	}
}

void CEdSceneValidator::ProcessLightDef( SValidationContext& context, const CStorySceneLight* light ) const
{
	if( !light->m_id )
	{
		VALIDATOR_ERROR( String::Printf( TXT("One of the lights does not have Id" ) ) )
	}
	else if( !context.usedIds.PushBackUnique( light->m_id ) )
	{
		VALIDATOR_ERROR( String::Printf( TXT("Duplicated id for light %s"), light->m_id.AsChar() ) )
	}

	light->m_innerAngle;
	light->m_outerAngle;
	light->m_softness;

	light->m_shadowCastingMode;
	light->m_shadowFadeDistance;
	light->m_shadowFadeRange;
}

namespace
{
	void CollectDialogsetNames( TDynArray< CName >& dialogsetsNames, const CStorySceneSection* section, TDynArray< const CStorySceneSection* >& visited )
	{
		CName dialogsetName = section->GetDialogsetChange();
		if ( dialogsetName )
		{
			dialogsetsNames.PushBackUnique( dialogsetName );
		}
		else
		{
			TDynArray< const CStorySceneSection* > res;
			StorySceneControlPartUtils::GetPrevElementsOfType( section, res );
			if ( res.Size() > 0 )
			{
				for ( const CStorySceneSection* prevSection : res )
				{
					if ( !visited.Exist( prevSection ) )
					{
						visited.PushBack( prevSection );
						CollectDialogsetNames( dialogsetsNames, prevSection, visited );
					}					
				}
			}
			else
			{
				TDynArray< const CStorySceneInput* > res;
				StorySceneControlPartUtils::GetPrevElementsOfType( section, res );
				for ( const CStorySceneInput* input : res )
				{
					dialogsetsNames.PushBackUnique( input->GetDialogsetName() );
				}
			}
		}
	}
}

const TDynArray< const CStorySceneDialogsetInstance*>& CEdSceneValidator::GetDialogsetsForSection( SValidationContext& context, const CStorySceneSection* section ) const 
{
	Int32 ind = context.section2dialogsetKeys.GetIndex( section );
	if( ind != -1 )
	{
		return context.section2dialogsetVals[ind];
	}

	TDynArray< CName >	dialogsetsNames;
	TDynArray< const CStorySceneSection* >	visited;
	CollectDialogsetNames( dialogsetsNames, section, visited );

	TDynArray< const CStorySceneDialogsetInstance* > dialogsets;
	for ( CName dialogsetName : dialogsetsNames )
	{
		if ( const CStorySceneDialogsetInstance* dlgset = context.scene->GetDialogsetByName( dialogsetName ) )
		{
			dialogsets.PushBack( dlgset );
		}		
	}

	context.section2dialogsetKeys.PushBack( section );
	context.section2dialogsetVals.PushBack( dialogsets );

	return context.section2dialogsetVals.Last();
}

#undef NULL_SAFE_CALL
#undef VALIDATOR_ERROR
#undef VALIDATOR_WARNING
#undef VALIDATOR_INFO



String CEdSceneValidator::GenerateHTMLReport( const TDynArray< CEdSceneValidator::SValidationOutputMessage >  messages, const TDynArray< String >* links, Bool addControlElements )
{
	String report = TXT("<html>\n");

	{
		report +=	TXT( "<head>\n" )
			TXT( "<style TYPE=\"text/css\">\n" )
			TXT( "body {\n" )
			TXT( "	background-color: white;\n" )
			TXT( "	margin: 0px;" )
			TXT( "}\n" )
			TXT( "div.error{\n" )
			TXT( "	background-color: #ffcccc;\n" )
			TXT( "	padding:5px;\n" )
			TXT( "}\n" )
			TXT( "div.warning{\n" )
			TXT( "	background-color: #ffffcc;\n" )
			TXT( "	padding:5px;\n" )
			TXT( "}\n" )
			TXT( "div.info{\n" )
			TXT( "	background-color: #ccccff;\n" )
			TXT( "	padding:5px;\n" )
			TXT( "}\n" )
			TXT( "div.error:hover {\n" )
			TXT( "	background-color: #ffbbbb;\n" )
			TXT( "}" )
			TXT( "div.warning:hover {\n" )
			TXT( "	background-color: #ffff99;\n" )
			TXT( "}\n" )
			TXT( "div.info:hover {\n" )
			TXT( "	background-color: #bbbbff;\n" )
			TXT( "}\n" )
			TXT( "</style>\n" )
			TXT( "<script>\n" )
			TXT( "function updateElements (checkBoxid, className) {\n" )
			TXT( "	var vis = \"none\";\n" )
			TXT( "	if(document.getElementById( checkBoxid ).checked) vis = \"block\";\n" )
			TXT( "	var elements = document.getElementsByClassName( className ), i;\n" )
			TXT( "	for (var i = 0; i < elements.length; i ++) {\n" )
			TXT( "		elements[i].style.display = vis;\n" )
			TXT( "	}\n" )
			TXT( "}\n" )
			TXT( "</script>\n" )
			TXT( "</head>");	
	}
	{
		report += TXT("<body>\n");

		if ( addControlElements )
		{
			report +=	TXT("<div>\n")
				TXT("<input type=\"checkbox\" id=\"c1\" onclick=\"updateElements('c1', 'error')\" checked>Show errors \n")
				TXT("<input type=\"checkbox\" id=\"c2\" onclick=\"updateElements('c2', 'warning')\" >Show warnings \n")
				TXT("<input type=\"checkbox\" id=\"c3\" onclick=\"updateElements('c3', 'info')\" >Show info \n")
				TXT("</div>\n");
		}	

		Uint32 size = messages.Size();
		for ( Uint32 i = 0; i < size; i++ )
		{

			String cls = messages[i].m_msgType == Error ? TXT("error") : messages[i].m_msgType == Warning ? TXT("warning") : TXT("info");
			report += TXT("<div CLASS=\"") + cls + TXT("\">\n");
			if ( links )
			{
				report += TXT("<font size=\"2\"><a href=\"") + (*links)[ i ] + TXT("\">") + messages[i].m_message + TXT("</a></font>");
			}
			else
			{
				report += TXT("<font size=\"3\">") + messages[i].m_message + TXT("</font>");
			}				
			report += TXT("\n</div>\n");
		}
		if ( addControlElements )
		{
			report +=	TXT( "<script>\n" )
						TXT( "updateElements('c1', 'error')\n" )
						TXT( "updateElements('c2', 'warning')\n" )
						TXT( "updateElements('c3', 'info')\n" )
						TXT( "</script>\n" );
		}
		report +=	TXT("</body>\n");
	}
	report += TXT("</html>");

	return report;
}

#ifdef DEBUG_SCENES_2
#pragma optimize("",on)
#endif
