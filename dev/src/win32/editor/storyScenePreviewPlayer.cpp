
#include "build.h"
#include "storyScenePreviewPlayer.h"
#include "../../common/game/storySceneLine.h"
#include "../../common/game/storySceneCutscene.h"
#include "../../common/game/storySceneSectionPlayingPlan.h"
#include "dialogEditor.h"
#include "dialogEditorUtils.h"
#include "../../common/core/gatheredResource.h"
#include "../../common/engine/camera.h"
#include "../../common/engine/viewport.h"
#include "dialogEditorFlowCtrl.h"

#include "../../common/game/storySceneFlowCondition.h"
#include "../../common/engine/renderFrame.h"
#include "../../common/engine/fonts.h"
#include "../../common/game/storySceneInput.h"

#ifdef DEBUG_SCENES_2
#pragma optimize("",off)
#endif

//////////////////////////////////////////////////////////////////////////

CGatheredResource resDialogPreviewFont2( TXT("gameplay\\gui\\fonts\\arial.w2fnt"), RGF_Startup );
CGatheredResource resScenePreviewDefFont2( TXT("gameplay\\gui\\fonts\\arial.w2fnt"), RGF_Startup );
CGatheredResource resScenePreviewDefSmallFont2( TXT("gameplay\\gui\\fonts\\aldo_pc_15.w2fnt"), RGF_Startup );

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CStoryScenePreviewPlayer );

CStoryScenePreviewPlayer::CStoryScenePreviewPlayer()
	: m_camera( nullptr )
	, m_autoGoToNextSection( false )
	, m_playSpeechSound( true )
	, m_drawCompGrid( SSPPCG_None )
	, m_hackFlowCtrl( nullptr )
{
}

void CStoryScenePreviewPlayer::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	m_camera = CStorySceneSystem::CreateSceneCamera( world );
}

void CStoryScenePreviewPlayer::OnDetached( CWorld* world )
{
	if ( m_camera )
	{
		m_camera->Destroy();
		m_camera = nullptr;
	}

	TBaseClass::OnDetached( world );
}

void CStoryScenePreviewPlayer::OnInit( const ScenePlayerPlayingContext& context )
{
	m_display = this;
	m_hackFlowCtrl = static_cast< CEdSceneFlowCtrl* >( context.m_hackFlowCtrl );
	SCENE_ASSERT( m_hackFlowCtrl );
}

const CStorySceneDialogsetInstance* CStoryScenePreviewPlayer::GetDialogsetForEvent( const CStorySceneEvent* e ) const
{
	SCENE_ASSERT( m_hackFlowCtrl );

	const CStorySceneElement* elem = e->GetSceneElement();
	const CStorySceneSection* section = elem->GetSection();

	CName dialgsetName = m_hackFlowCtrl->GetDialogsetNameFromFlowFor( section );
	SCENE_ASSERT( dialgsetName != CName::NONE );

	return dialgsetName ? section->GetScene()->GetDialogsetByName( dialgsetName ) : nullptr;
}

void CStoryScenePreviewPlayer::OnPreResetAllSceneEntitiesState( Bool forceDialogset, const CStorySceneSection* currSection, const CStorySceneDialogsetInstance* currDialogset )
{
	SCENE_ASSERT( m_hackFlowCtrl );

	if( forceDialogset )
	{
		if ( m_hackFlowCtrl )
		{
			if ( !m_hackFlowCtrl->HasFlowFor( currSection ) )
			{
				m_hackFlowCtrl->SetSection( currSection, this );
			}

			m_eventsExecutor.Execute( this, m_hackFlowCtrl->GetEventsFlowCollector(), m_debugger );
		}
	}	
}

CStorySceneSectionPlayingPlan* CStoryScenePreviewPlayer::HACK_GetOrCreateSectionPlayingPlan( const CStorySceneSection* requestedSection, const TDynArray< const CStorySceneLinkElement* >& flow )
{
	CStorySceneSectionPlayingPlan* pp = GetSectionPlayingPlan( requestedSection );
	if ( !pp )
	{
		CName dialogSet;
		RequestSectionPlayingPlan_StartFlow( requestedSection, dialogSet );
		const Int32 MAX_TICKS = 5;
		for ( Int32 i = 0; i < MAX_TICKS && m_sectionLoader.HasAnythingInQueue(); i++ )
		{
			m_sectionLoader.OnTick( 0.f );
		}
		pp = GetSectionPlayingPlan( requestedSection );
	}

	SCENE_ASSERT( pp );
	return pp;
}

void CStoryScenePreviewPlayer::HACK_CollectAllEvents( CStorySceneSectionPlayingPlan* plan, CStorySceneEventsCollector& collector, const CStorySceneEventsCollectorFilter* filter )
{
	CollectAllEvents( plan, collector, filter, nullptr );
}

Bool CStoryScenePreviewPlayer::ShouldPlaySpeechSound() const
{
	return m_playSpeechSound;
}

Bool CStoryScenePreviewPlayer::ShouldPlaySounds() const 
{
	return !IsPaused();// && !m_internalState.m_forcingTimeThisFrame;
}

Bool CStoryScenePreviewPlayer::ShouldPlayEffects() const 
{
	return !IsPaused();// && !m_internalState.m_forcingTimeThisFrame;
}

Bool CStoryScenePreviewPlayer::CanUseCutscene() const 
{
	return true; 
}

CCamera* CStoryScenePreviewPlayer::GetSceneCamera() const
{
	return m_camera;
}

void CStoryScenePreviewPlayer::SetAutoGoToNextSection( Bool flag )
{ 
	m_autoGoToNextSection = flag; 
}

Bool CStoryScenePreviewPlayer::GetAutoGoToNextSection() const
{ 
	return m_autoGoToNextSection; 
}

void CStoryScenePreviewPlayer::PlaySpeechSound( Bool flag )
{
	m_playSpeechSound = flag;
}

void CStoryScenePreviewPlayer::SetPreviewGrid( SSPreviewPlayerCompGrid grid )
{
	m_drawCompGrid = grid;
}

SSPreviewPlayerCompGrid CStoryScenePreviewPlayer::GetPreviewGrid() const
{
	return m_drawCompGrid;
}

Bool CStoryScenePreviewPlayer::WantsToGoToNextSection()
{
	if ( m_autoGoToNextSection )
	{
		return true;
	}

	return false;
}

Bool CStoryScenePreviewPlayer::CanFinishScene()
{
	return false;
}

Bool CStoryScenePreviewPlayer::CalculateCamera( IViewport* view, CRenderCamera &camera ) const
{
	const CStorySceneSectionPlayingPlan* plan =  m_internalState.m_planId != -1 ? m_sectionLoader.FindPlanById( m_internalState.m_planId ) : nullptr;
	if ( plan && plan->GetCurrElement() && plan->GetCurrElement()->GetElement() )
	{
		const CCameraComponent* cam( nullptr );
		if ( plan->GetCurrElement()->GetElement()->IsA< CStorySceneCutscenePlayer >() )
		{
			const StorySceneCutscenePlayerInstanceData* elemInstance = static_cast< const StorySceneCutscenePlayerInstanceData* >( plan->GetCurrElement() );
			cam = elemInstance->GetCamera();
		}
		else if ( plan->GetCurrElement()->GetElement()->IsA< CStorySceneChoice >() && plan->GetCurrElement()->GetElement()->GetSection()->IsA< CStorySceneCutsceneSection >() )
		{
			const CStorySceneCutsceneChoiceInstanceData* elemInstance = static_cast< const CStorySceneCutsceneChoiceInstanceData* >( plan->GetCurrElement() );
			cam = elemInstance->GetCamera();
		}
		
		if ( cam )
		{
			CCameraComponent* c = const_cast< CCameraComponent* >( cam );
			c->OnViewportCalculateCamera( view, camera );
			return true;
		}
	}
	
	if ( m_camera && m_camera->GetSelectedCameraComponent() )
	{
		m_camera->GetSelectedCameraComponent()->OnViewportCalculateCamera( view, camera );
		if ( camera.GetPosition() == Vector::ZERO_3D_POINT )
		{
			EngineTransform placement =	m_sceneDirector.GetCurrentScenePlacement();
			camera.Set( placement.GetPosition(), placement.GetRotation(), camera.GetFOV(), camera.GetAspect(), camera.GetNearPlane(), camera.GetFarPlane() );
		}
		return true;
	}

	return false;
}

TDynArray< CName > CStoryScenePreviewPlayer::GetActorIds( Int32 actorTypes ) const
{
	TDynArray< CName > tags;

#define COLLECT_IDS( _type, _hashmap )										\
	if ( actorTypes & _type  )												\
	{																		\
		for ( auto i = _hashmap.Begin() ;i != _hashmap.End(); ++i )			\
		{																	\
			tags.PushBack( i->m_first );									\
		}																	\
	}		

	COLLECT_IDS( AT_ACTOR, m_sceneActorEntities ) 
	COLLECT_IDS( AT_PROP, m_scenePropEntities ) 
	COLLECT_IDS( AT_EFFECT, m_sceneEffectEntities ) 
	COLLECT_IDS( AT_LIGHT, m_sceneLightEntities ) 
#undef COLLECT_IDS

		return tags;
}

EngineTransform	CStoryScenePreviewPlayer::GetCurrentScenePlacement() const
{
	return m_sceneDirector.GetCurrentScenePlacement();
}

Bool CStoryScenePreviewPlayer::ReloadCamera( const StorySceneCameraDefinition* cameraDefinition )
{
	if ( cameraDefinition && m_internalState.m_cameraState.m_cameraUniqueID == cameraDefinition->m_cameraUniqueID )
	{
		m_sceneDirector.ActivateCamera( *cameraDefinition );

		return true;
	}

	return false;
}

void CStoryScenePreviewPlayer::GetActorAnimationNames( const CName& actor, TDynArray< CName >& out ) const
{
	m_eventsExecutor.GetActorAnimationNames( actor, out );
}

Bool CStoryScenePreviewPlayer::GetActorCurrIdleAnimationNameAndTime( CName actorId, CName& animName, Float& animTime ) const
{
	return m_eventsExecutor.GetActorCurrIdleAnimationNameAndTime( actorId, animName, animTime );
}

Bool CStoryScenePreviewPlayer::GetActorCurrAnimationTime( CName actorId, CName animName, Float& animTime ) const
{
	return m_eventsExecutor.GetActorCurrAnimationTime( actorId, animName, animTime );
}

const IStorySceneElementInstanceData* CStoryScenePreviewPlayer::FindElementInstance( const CStorySceneElement* element ) const
{
	SCENE_ASSERT( m_internalState.m_planId != -1 );

	if ( m_internalState.m_planId != -1 )
	{
		const CStorySceneSectionPlayingPlan* plan = m_sectionLoader.FindPlanById( m_internalState.m_planId );
		if ( plan )
		{
			for ( Uint32 i = 0, numElements = plan->GetNumElements(); i < numElements; ++i )
			{
				if ( plan->m_sectionInstanceData.m_elements[ i ] == element )
				{
					return plan->m_sectionInstanceData.m_elemData[ i ];
				}
			}
		}
	}

	return nullptr;
}

Bool CStoryScenePreviewPlayer::LocalVoMatchApprovedVoInCurrentSectionVariant() const
{
	if( m_internalState.m_planId != -1 )
	{
		const CStorySceneSectionPlayingPlan* plan = m_sectionLoader.FindPlanById( m_internalState.m_planId );
		if( plan )
		{
			return plan->LocalVoMatchApprovedVo();
		}
	}

	return false;
}

Bool CStoryScenePreviewPlayer::FindEventsByTime( const CClass* c, Float time, TDynArray< CStorySceneEvent* >& out, Float eps )
{
	SCENE_ASSERT( m_internalState.m_planId != -1 );

	if ( m_internalState.m_planId != -1 )
	{
		const CStorySceneSectionPlayingPlan* plan = m_sectionLoader.FindPlanById( m_internalState.m_planId );
		if ( plan )
		{
			const TDynArray< const CStorySceneEvent* >& events = plan->m_sectionInstanceData.m_evts;
			for ( Uint32 i=0; i<events.Size(); ++i )
			{
				const CStorySceneEvent* e = events[ i ];
				if ( e->GetClass()->IsA( c ) )
				{
					Float sT = 0.f;
					Float eT = 0.f;
					Float dT = 0.f;

					e->GetEventInstanceData( *plan->m_sectionInstanceData.m_data, sT, eT, dT );

					if ( time + eps >= sT && time - eps <= eT )
					{
						out.PushBack( const_cast< CStorySceneEvent* >( e ) );
					}
				}
			}
		}
	}

	return out.Size() > 0;
}

Bool CStoryScenePreviewPlayer::FindEventsByType( const CClass* c, TDynArray< CStorySceneEvent* >& out )
{
	SCENE_ASSERT( m_internalState.m_planId != -1 );

	if ( m_internalState.m_planId != -1 )
	{
		const CStorySceneSectionPlayingPlan* plan = m_sectionLoader.FindPlanById( m_internalState.m_planId );
		if ( plan )
		{
			const TDynArray< const CStorySceneEvent* >& events = plan->m_sectionInstanceData.m_evts;
			for ( Uint32 i=0; i<events.Size(); ++i )
			{
				const CStorySceneEvent* e = events[ i ];
				if ( e->GetClass()->IsA( c ) )
				{
					out.PushBack( const_cast< CStorySceneEvent* >( e ) );
				}
			}
		}
	}

	return out.Size() > 0;
}

CName CStoryScenePreviewPlayer::GetPrevSpeakerName( const CStorySceneElement* currElement ) const
{
	const CStorySceneLine* line = GetPrevLine( currElement );
	return line ? line->GetVoiceTag() : CName::NONE;
}

const CStorySceneLine* CStoryScenePreviewPlayer::GetPrevLine( const CStorySceneElement* currElement ) const
{
	const Uint32 sectionSize = m_storyScene->GetNumberOfSections();
	for ( Uint32 i=0; i<sectionSize; ++i )
	{
		const CStorySceneSection* section = m_storyScene->GetSection( i );
		const TDynArray< CStorySceneElement* >& elements = section->GetElements(); 
		
		Int32 elementIndex = -1;

		if( section->GetChoice() == currElement )
		{
			elementIndex = Int32(section->GetNumberOfElements())-1;
		}
		else
		{
			elementIndex = elements.GetIndex( Find( elements.Begin(), elements.End(), currElement ) );
		}

		if ( elementIndex >= 0 )
		{
			for ( Int32 k=(Int32)(elementIndex-1); k>=0; --k )
			{
				const CStorySceneElement* prevElem = section->GetElement( k );
				if ( prevElem && prevElem->IsA< CStorySceneLine >() )
				{
					const CStorySceneLine* line = static_cast< const CStorySceneLine* >( prevElem );
					return line;
				}
			}

			{
				// DIALOG_TOMSIN_TODO
				TDynArray< CStorySceneSection* > prevSections;
				CEdSceneEditor::GetPrecedingSections( section, prevSections, m_storyScene.Get() );

				if ( prevSections.Size() > 0 )
				{
					CStorySceneSection* prevSection = prevSections[ 0 ];

					const Int32 elementSize = (Int32)prevSection->GetNumberOfElements();
					for ( Int32 k=elementSize-1; k>=0; --k )
					{
						const CStorySceneElement* prevElem = prevSection->GetElement( k );
						if ( prevElem && prevElem->IsA< CStorySceneLine >() )
						{
							const CStorySceneLine* line = static_cast< const CStorySceneLine* >( prevElem );
							return line;
						}
					}
				}
			}
		}
	}

	return NULL;
}

void CStoryScenePreviewPlayer::ShowDialogText( const String& text, ISceneActorInterface* actor, EStorySceneLineType lineType, Bool alternativeUI )
{
	HideDialogText( actor, SSLT_Normal );

	SDialogLine line;
	line.m_actor = actor;
	if( actor )
	{
		line.m_text = actor->GetSceneActorVoiceTag().AsString() + TXT(": ") + text;
	}	
	line.m_type = lineType;
	line.m_isTextWrapped = false;	

	m_lines.PushBack( line );
}

void CStoryScenePreviewPlayer::HideDialogText( ISceneActorInterface* actor, EStorySceneLineType lineType )
{
	const Uint32 size = m_lines.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		if ( m_lines[ i ].m_actor == actor )
		{
			m_lines.RemoveAt( i );
			return;
		}
	}
}

void CStoryScenePreviewPlayer::ShowDebugComment( CGUID commentId, const String& comment )
{
	SDebugComment dbgComment;
	dbgComment.m_id = commentId;
	dbgComment.m_comment = comment;
	m_debugComments.PushBack( dbgComment );
}

void CStoryScenePreviewPlayer::HideDebugComment( CGUID commentId )
{
	for( Uint32 iComment = 0, numComments = m_debugComments.Size(); iComment < numComments; ++iComment )
	{
		if( m_debugComments[ iComment ].m_id == commentId )
		{
			m_debugComments.RemoveAt( iComment );
			break;
		}
	}
}

void CStoryScenePreviewPlayer::HideAllDebugComments()
{
	m_debugComments.Clear();
}

void CStoryScenePreviewPlayer::HideAllDialogTexts()
{
	m_lines.Clear();
}

const String& CStoryScenePreviewPlayer::GetLastDialogText()
{
	return String::EMPTY;
}

void CStoryScenePreviewPlayer::SetChoices( const TDynArray< SSceneChoice >& choices, Bool alternativeUI )
{
	m_choices = choices;
}

const CActor* CStoryScenePreviewPlayer::AsSceneActor( const CEntity* e ) const
{
	for ( auto it = m_sceneActorEntities.Begin(), end = m_sceneActorEntities.End(); it != end; ++it )
	{
		if ( (*it).m_second == e )
		{
			return Cast< const CActor >( e );
		}
	}
	return nullptr;
}

const CEntity* CStoryScenePreviewPlayer::AsSceneProp( const CEntity* e ) const
{
	for ( auto it = m_scenePropEntities.Begin(), end = m_scenePropEntities.End(); it != end; ++it )
	{
		if ( (*it).m_second == e )
		{
			return e;
		}
	}
	return nullptr;
}

const CEntity* CStoryScenePreviewPlayer::AsSceneLight( const CEntity* e ) const
{
	for ( auto it = m_sceneLightEntities.Begin(), end = m_sceneLightEntities.End(); it != end; ++it )
	{
		if ( (*it).m_second == e )
		{
			return e;
		}
	}
	return nullptr;
}

void CStoryScenePreviewPlayer::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags )
{
	TBaseClass::OnGenerateEditorFragments( frame, flags );

	if ( frame->GetFrameInfo().IsShowFlagOn( SHOW_VisualDebug ) || frame->GetFrameInfo().IsShowFlagOn( SHOW_Scenes ) )
	{
		if ( m_drawCompGrid != SSPPCG_None )
		{
			DrawCompGrid( frame );
		}

		CFont* font = resDialogPreviewFont2.LoadAndGet< CFont >();
		ASSERT( font );

		{
			Bool textMaxWidthChanged = false;
			Uint32 newTextMaxWdith = (frame->GetFrameOverlayInfo().m_width / 10) * 9;
			if (newTextMaxWdith != m_textMaxWidth)
			{
				m_textMaxWidth = newTextMaxWdith;
				textMaxWidthChanged = true;
			}

			Int32 lineOffset = 0;
			for ( Uint32 i = 0; i < m_lines.Size(); ++i )
			{
				SDialogLine& dialogLine = m_lines[i];

				// wrap text if it's not yet wrapped or if max width of text changed
				if(!dialogLine.m_isTextWrapped || textMaxWidthChanged)
				{
					StorySceneEditorUtils::WrapString(dialogLine.m_textWrapped, dialogLine.m_text, m_textMaxWidth, *font);
					m_lines[i].m_isTextWrapped = true;
				}

				// get text rectangle
				Int32 unused = 0;
				Uint32 textWidth = 0;
				Uint32 textHeight = 0;
				font->GetTextRectangle( dialogLine.m_textWrapped, unused, unused, textWidth, textHeight );

				// display text
				Uint32 textX = frame->GetFrameOverlayInfo().m_width - m_textMaxWidth;
				Uint32 textY = frame->GetFrameOverlayInfo().m_height - textHeight - lineOffset;
				frame->AddDebugScreenText( textX, textY, dialogLine.m_textWrapped, Color::WHITE, font, false, Color( 0, 0, 0 ), true );

				// next line will be displayed above current one
				lineOffset += textHeight;
			}

			// display debug comments
			for( Uint32 i = 0, numComments = m_debugComments.Size(); i < numComments; ++i )
			{
				// get text rectangle
				Int32 unused = 0;
				Uint32 textWidth = 0;
				Uint32 textHeight = 0;
				font->GetTextRectangle( m_debugComments[ i ].m_comment, unused, unused, textWidth, textHeight );

				// display text
				Uint32 textX = frame->GetFrameOverlayInfo().m_width - m_textMaxWidth;
				Uint32 textY = frame->GetFrameOverlayInfo().m_height - textHeight - lineOffset;
				frame->AddDebugScreenText( textX, textY, m_debugComments[ i ].m_comment, Color::RED, font, false, Color( 0, 0, 0 ), true  );

				// next line will be displayed above current one
				lineOffset += textHeight;
			}
		}

		{
			Int32 offset = 150;
			for ( Int32 i=m_choices.SizeInt()-1; i>=0; --i )
			{
				const String text = String::Printf( TXT("%d.%s"), i+1, m_choices[i].m_description.AsChar() );

				frame->AddDebugScreenText( frame->GetFrameOverlayInfo().m_width * 0.2f, frame->GetFrameOverlayInfo().m_height - offset, text, Color::WHITE, font, false, Color( 0, 0, 0 ), true  );
				offset += 30;
			}

			const CStorySceneDialogsetInstance * dialogset = GetCurrentDialogsetInstance();
			if (  dialogset )
			{
				for ( Uint32 i = 0; i < dialogset->GetSlots().Size(); ++i )
				{
					CStorySceneDialogsetSlot* slot = dialogset->GetSlots()[ i ];
					if( !slot )
					{
						continue;
					}
					EngineTransform slotPlacement;
					CStorySceneDirectorPlacementHelper	placementHelper;
					placementHelper.Init( this );
					EngineTransform slotTransform;
					placementHelper.GetSlotPlacement( slot, dialogset, slotPlacement );
					Matrix slotLocalToWorld;
					slotPlacement.CalcLocalToWorld( slotLocalToWorld );

					frame->AddDebugSphere( slotPlacement.GetPosition(), 0.25f, Matrix::IDENTITY, Color::BLUE );
					frame->AddDebugArrow( slotLocalToWorld, Vector::EY, 1.0f, Color::GREEN );
					frame->AddDebugText( slotPlacement.GetPosition(), slot->GetSlotName().AsString(), 0, 0, true, Color::LIGHT_BLUE );
				}
			}
		}
	}
}

namespace
{
	void DrawLine( CRenderFrame* frame, Float x1, Float y1, Float x2, Float y2, Float w, Float h, const Color& c )
	{
		frame->AddDebugLineOnScreen( Vector2( x1*w, y1*h ), Vector2( x2*w, y2*h ), c );
	}

	void DrawArc( CRenderFrame* frame, Float x1, Float y1, Float x2, Float y2, Float px, Float py, Float w, Float h, const Color& c )
	{
		Vector2 pointA( x1*w, y1*h );
		Vector2 pointB( x2*w, y2*h );
		Vector2 pointC( px*w, py*h );

		Vector2 vecA = pointA - pointC;
		Vector2 vecB = pointB - pointC;

		const Float l = vecA.Mag();

		Vector2 vecM = vecA + vecB;
		vecM.Normalize();
		vecM *= l;

		Vector2 pointM_m = pointC + vecM;

		frame->AddDebugLineOnScreen( pointA, pointM_m, c );
		frame->AddDebugLineOnScreen( pointM_m, pointB, c );
	}

	void DrawLineX( CRenderFrame* frame, Float x, Float y1, Float y2, Float w, Float h, const Color& c )
	{
		frame->AddDebugLineOnScreen( Vector2( x*w, y1*h ), Vector2( x*w, y2*h ), c );
	}

	void DrawLineY( CRenderFrame* frame, Float y, Float x1, Float x2, Float w, Float h, const Color& c )
	{
		frame->AddDebugLineOnScreen( Vector2( x1*w, y*h ), Vector2( x2*w, y*h ), c );
	}
}

void CStoryScenePreviewPlayer::DrawCompGrid( CRenderFrame* frame )
{
	Color color( 128, 128, 128 );

	const Float w = (Float)frame->GetFrameOverlayInfo().m_width;
	const Float h = (Float)frame->GetFrameOverlayInfo().m_height;

	if ( m_drawCompGrid == SSPPCG_33 )
	{
		frame->AddDebugLineOnScreen( Vector2( 1.f/3.f*w, 0.f ), Vector2( 1.f/3.f*w, h ), color );
		frame->AddDebugLineOnScreen( Vector2( 2.f/3.f*w, 0.f ), Vector2( 2.f/3.f*w, h ), color );

		frame->AddDebugLineOnScreen( Vector2( 0.f, 1.f/3.f*h ), Vector2( w, 1.f/3.f*h ), color );
		frame->AddDebugLineOnScreen( Vector2( 0.f, 2.f/3.f*h ), Vector2( w, 2.f/3.f*h ), color );
	}
	else if ( m_drawCompGrid == SSPPCG_FIB_RATIO )
	{
		const Float p = 1.618f;
		const Float pw = 2.f*p+1.f;

		frame->AddDebugLineOnScreen( Vector2( p/pw*w, 0.f ), Vector2( p/pw*w, h ), color );
		frame->AddDebugLineOnScreen( Vector2( ((1.f+p)/pw)*w, 0.f ), Vector2( ((1.f+p)/pw)*w, h ), color );

		frame->AddDebugLineOnScreen( Vector2( 0.f, p/pw*h ), Vector2( w, p/pw*h ), color );
		frame->AddDebugLineOnScreen( Vector2( 0.f, ((1.f+p)/pw)*h ), Vector2( w, ((1.f+p)/pw)*h ), color );
	}
	else if ( m_drawCompGrid >= SSPPCG_FIB_LT && m_drawCompGrid <= SSPPCG_FIB_RB )
	{
		Float x[6];
		Float y[6];
		x[0] = 1.f;
		y[0] = 1.f;
		x[1] = 0.f;
		y[1] = 0.f;
		x[2] = 21.f / 55.f;
		y[2] = 13.f / 34.f;
		x[3] = 13.f / 55.f;
		y[3] = 8.f / 34.f;
		x[4] = 16.f / 55.f;
		y[4] = 10.f / 34.f;
		x[5] = 15.f / 55.f;
		y[5] = 9.f / 34.f;

#define DRAW_ARC( a, b )\
		DrawArc( frame, a+1.f,	  b+0.f,	a+x[0+2], b+1.f,	a+x[0+2], b+0.f,	w, h, color );\
		DrawArc( frame, a+x[0+2], b+1.f,	a+0.f,	  b+y[0+2], a+x[0+2], b+y[0+2], w, h, color );\
		DrawArc( frame, a+0.f,	  b+y[0+2], a+x[1+2], b+0.f,	a+x[1+2], b+y[0+2], w, h, color );\
		DrawArc( frame, a+x[1+2], b+0.f,	a+x[0+2], b+y[1+2], a+x[1+2], b+y[1+2], w, h, color );\
		DrawArc( frame, a+x[0+2], b+y[1+2], a+x[2+2], b+y[0+2], a+x[2+2], b+y[1+2], w, h, color );\
		DrawArc( frame, a+x[2+2], b+y[0+2], a+x[1+2], b+y[2+2], a+x[2+2], b+y[2+2], w, h, color );\
		DrawArc( frame, a+x[1+2], b+y[2+2], a+x[3+2], b+y[1+2], a+x[3+2], b+y[2+2], w, h, color );\
		DrawArc( frame, a+x[3+2], b+y[1+2], a+x[2+2], b+y[3+2], a+x[3+2], b+y[3+2], w, h, color );

		if ( m_drawCompGrid == SSPPCG_FIB_LT )
		{
			for ( Uint32 i=2; i<6; ++i )
			{
				DrawLineX( frame, x[i], y[i-1], y[i-2], w, h, color );
				DrawLineY( frame, y[i], x[i], x[i-1], w, h, color );
			}

			DRAW_ARC( 0.f, 0.f );
		}
		else if ( m_drawCompGrid == SSPPCG_FIB_RT )
		{
			for ( Uint32 i=2; i<6; ++i )
			{
				DrawLineX( frame, 1.f-x[i], y[i-1], y[i-2], w, h, color );
				DrawLineY( frame, y[i], 1.f-x[i], 1.f-x[i-1], w, h, color );

				DRAW_ARC( 1.f-, 0.f );
			}
		}
		else if ( m_drawCompGrid == SSPPCG_FIB_LB )
		{
			for ( Uint32 i=2; i<6; ++i )
			{
				DrawLineX( frame, x[i], 1.f-y[i-1], 1.f-y[i-2], w, h, color );
				DrawLineY( frame, 1.f-y[i], x[i], x[i-1], w, h, color );

				DRAW_ARC( 0.f, 1.f- );
			}
		}
		else if ( m_drawCompGrid == SSPPCG_FIB_RB )
		{
			for ( Uint32 i=2; i<6; ++i )
			{
				DrawLineX( frame, 1.f-x[i], 1.f-y[i-1], 1.f-y[i-2], w, h, color );
				DrawLineY( frame, 1.f-y[i], 1.f-x[i], 1.f-x[i-1], w, h, color );

				DRAW_ARC( 1.f-, 1.f- );
			}
		}
		#undef DRAW_ARC
	}
	else if ( m_drawCompGrid == SSPPCG_DYN_SYMMETRY )
	{
		frame->AddDebugLineOnScreen( Vector2( 0.f, 0.f ), Vector2( w, h ), color );
		Vector2 p1 = Vector2( w - h * h / w, h );
		frame->AddDebugLineOnScreen( Vector2( w, 0.f ), p1, color );
		frame->AddDebugLineOnScreen( p1, Vector2( p1.X, 0 ), color );
		frame->AddDebugLineOnScreen( Vector2( p1.X, 0 ), Vector2( w, h ), color );

		frame->AddDebugLineOnScreen( Vector2( w, 0.f ), Vector2( 0.f, h ), color );
		Vector2 p2 = Vector2( h * h / w, h );
		frame->AddDebugLineOnScreen( Vector2( 0.f, 0.f ), p2, color );
		frame->AddDebugLineOnScreen( p2, Vector2( p2.X, 0 ), color );
		frame->AddDebugLineOnScreen( Vector2( p2.X, 0 ), Vector2( 0.f, h ), color );
	}
}

void CStoryScenePreviewPlayer::DeactivateCustomEnv()
{
	m_eventsExecutor.DeactivateCustomEnv( this );
}

void CStoryScenePreviewPlayer::SetCameraAdjustedDebugFrame( Bool adjusted )
{
	m_sceneDirector.SetCameraAdjustedDebugFrame( adjusted );
}

void CStoryScenePreviewPlayer::OnActorSpawned( const EntitySpawnInfo& info, const CEntity* entity )
{
	SCENE_ASSERT( 0 );
}

void CStoryScenePreviewPlayer::OnPropSpawned( const EntitySpawnInfo& info, const CEntity* entity )
{
	//SCENE_ASSERT( 0 );
}

Bool CStoryScenePreviewPlayer::EvaluateFlowCondition( const CStorySceneFlowCondition* condition ) const 
{
	return condition->GetSelectedOutputLinkElement() == 1;
}

Bool CStoryScenePreviewPlayer::HasValidDialogsetFor( const CStorySceneSection* section, const CStorySceneDialogsetInstance* d ) const
{
	const CName dialogsetName = d->GetName();

	if ( section->GetDialogsetChange() )
	{
		return section->GetDialogsetChange() == dialogsetName;
	}

	TDynArray< const CStorySceneLinkElement* > flow;
	if( m_hackFlowCtrl )
	{
		m_hackFlowCtrl->GetFlow( section, flow, true );
	}
	

	const Uint32 num = flow.Size();
	for ( Uint32 i=0; i<num; ++i )
	{
		if ( const CStorySceneSection* s = Cast< const CStorySceneSection >( flow[ i ] ) )
		{
			if ( s->GetDialogsetChange() )
			{
				return s->GetDialogsetChange() == dialogsetName;
			}
		}
		else if ( const CStorySceneInput* input = Cast< const CStorySceneInput >( flow[ i ] ) )
		{
			if ( input->GetDialogsetName() )
			{
				return input->GetDialogsetName() == dialogsetName;
			}
		}
	}

	return false;
}

void CStoryScenePreviewPlayer::OnRequestSectionPlayingPlan_StartFlow( const CStorySceneSection* requestedSection, CName& dialogSet )
{
	if ( !dialogSet )
	{
		// Find proper dialogset based on flow
		TDynArray< const CStorySceneLinkElement* > flow;
		if( m_hackFlowCtrl )
		{
			m_hackFlowCtrl->GetFlow( requestedSection, flow, true );
		}

		CName ret;

		const Uint32 num = flow.Size();
		for ( Uint32 i=0; i<num; ++i )
		{
			if ( const CStorySceneSection* s = Cast< const CStorySceneSection >( flow[ i ] ) )
			{
				if ( s->GetDialogsetChange() )
				{
					ret = s->GetDialogsetChange();
					break;
				}
			}
			else if ( const CStorySceneInput* input = Cast< const CStorySceneInput >( flow[ i ] ) )
			{
				if ( input->GetDialogsetName() )
				{
					ret = input->GetDialogsetName();
					break;
				}
			}
		}

		dialogSet = ret;
	}
}

#ifdef DEBUG_SCENES_2
#pragma optimize("",on)
#endif
