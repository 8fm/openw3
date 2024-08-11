/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "../../common/engine/appearanceComponent.h"
#include "actorActionWork.h"
#include "communityAgentStub.h"
#include "communitySystem.h"
#include "communityUtility.h"
#include "../../common/engine/debugCommandBox.h"
#include "movingPhysicalAgentComponent.h"
#include "../../common/game/encounter.h"
#include "../../common/engine/debugWindowsManager.h"
#include "../core/fileSkipableBlock.h"
#include "../core/memoryFileAnalizer.h"
#include "../engine/debugPage.h"
#include "../engine/debugPageManagerBase.h"
#include "../engine/meshTypeComponent.h"
#include "../engine/renderFrame.h"
#include "../engine/meshTypeResource.h"


#ifndef NO_DEBUG_PAGES

class CDebugPageNPCList;

class CDebugCommandNPC : public IDebugCommandBox
{
public:
	CDebugCommandNPC( CDebugPageNPCList* parent, String flagSummary, Uint32 npcIndex )
		: IDebugCommandBox( NULL, flagSummary ), m_parentSystem(parent), m_npcIndex(npcIndex) { /*intentionally empty */ };

	virtual void Process();

private:
	CDebugPageNPCList* m_parentSystem;
	Uint32 m_npcIndex;
};

//////////////////////////////////////////////////////////////////////////
/// Interactive list of NPC
class CDebugPageNPCList : public IDebugPage
{
protected:
	TDynArray< THandle< CNewNPC > >		m_allNpcs;				//!< All NPCs in the world
	TDynArray< String >					m_npcNames;				//!< NPC names
	Vector								m_cameraTarget;			//!< Current camera target
	Vector								m_cameraPosition;		//!< Current camera position
	String								m_activeNpcName;		//!< Name of the selected NPC
	Int32								m_active;				//!< Active NPC
	Float								m_cameraRotation;		//!< NPC camera rotation
	Int32								m_verticalOffset;		//!< Verical offset of text

	CDebugOptionsTree*					m_npcTree;

public:
	CDebugPageNPCList()
		: IDebugPage( TXT("NPC Viewer") )
		, m_cameraRotation( 0.0f )
		, m_cameraTarget( Vector::ZEROS )
		, m_cameraPosition( Vector::ZEROS )
		, m_active( -1 )
		, m_verticalOffset( 0 )
	{};

	//! This debug page was shown
	virtual void OnPageShown()
	{
		IDebugPage::OnPageShown();

		// Clear list
		m_active = -1;
		m_allNpcs.Clear();
		m_npcNames.Clear();

		// Reset camera
		if( GGame != nullptr && GGame->GetActiveWorld() != nullptr && GGame->GetActiveWorld()->GetCameraDirector() != nullptr )
		{
			m_cameraTarget = GGame->GetActiveWorld()->GetCameraDirector()->GetCameraPosition();
		}

		// Create npcs tree
		m_npcTree = new CDebugOptionsTree( 50, 50, 200, 700, this );

		// Collect list of NPC
		const CCommonGame::TNPCCharacters& npcs = GCommonGame->GetNPCCharacters();
		for ( CCommonGame::TNPCCharacters::const_iterator it=npcs.Begin(); it!=npcs.End(); ++it )
		{
			CNewNPC* npc = *it;

			// Add to list
			m_allNpcs.PushBack( npc );

			// Remember name in separate list since handle can be lost
			String name = npc->GetFriendlyName();
			m_npcNames.PushBack( npc->GetFriendlyName() );
		}

		// Fill npcs tree
		for(Uint32 i=0; i<m_allNpcs.Size(); ++i)
		{
			String name = m_npcNames[i];
			//name.ReplaceAll(TXT("Unnamed CDynamicLayer::"), TXT(""));
			name = name.StringAfter(TXT("::"), true);
			CDebugCommandNPC* npcRow = new CDebugCommandNPC(this, name, i);
			m_npcTree->AddRoot(npcRow);
		}

		// Use current camera target
		m_cameraPosition = m_cameraTarget;

		// Pause game
		GGame->Pause( TXT( "CDebugPageNPCList" ) );
	}

	//! This debug page was hidden
	virtual void OnPageHidden()
	{
		IDebugPage::OnPageHidden();

		// Unpause game
		GGame->Unpause( TXT( "CDebugPageNPCList" ) );
	}

	//! External viewport tick
	virtual void OnTick( Float timeDelta )
	{
		// Update virtual camera rotation
		m_cameraRotation += timeDelta * 30.0f;
		m_npcTree->OnTick(timeDelta);
	}

	//! Sync shit
	void SyncToNewNPC(Int32 npcIndex)
	{
		// Reset vertical offset
		m_verticalOffset = 0;

		// Reset
		m_activeNpcName = TXT("");
		//m_cameraRotation = 0.0f;

		// Reset camera
		m_cameraTarget = GGame->GetActiveWorld()->GetCameraDirector()->GetCameraPosition();

		// Is new shit set ?
		m_active = npcIndex;
		if ( m_active != -1 )
		{
			// Cache name
			m_activeNpcName = m_npcNames[ npcIndex ];

			// Reset rotation
			CNewNPC* npc = m_allNpcs[ npcIndex ].Get();
			if ( npc )
			{
				//m_cameraRotation = npc->GetWorldYaw() + 180.0f;
				m_cameraTarget = CalculateCameraOrbit( npc );
				m_cameraPosition = m_cameraTarget;
			}
		}
	}

	//! Generalized input event
	virtual Bool OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
	{
#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS
		if( action == IACT_Press && key == IK_Enter)
		{
			GDebugWin::GetInstance().SetVisible(true);
			GDebugWin::GetInstance().ShowDebugWindow( DebugWindows::DW_NPCViewer );
		}
		return true;
#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI

		// hide informations about current npc
		if( key == IK_Backspace && action == IACT_Press )
		{
			SyncToNewNPC(-1);
			return true;
		}

		// Change vertical offset
		if( key == IK_PageDown && action == IACT_Press )
		{
			m_verticalOffset -= 10;
			return true;
		}

		if( key == IK_PageUp && action == IACT_Press )
		{
			m_verticalOffset += 10;
			if( m_verticalOffset > 0 )
			{
				m_verticalOffset = 0;
			}
			return true;
		}

		// Reset camera
		if ( ( key == IK_R || key == IK_Pad_A_CROSS ) && action == IACT_Press )
		{
			m_cameraPosition = m_cameraTarget;
			return true;
		}

		m_npcTree->OnInput(key, action, data);

		// Not handled
		return false;
	}

	void AddEncounterInfo( CNewNPC* npc, CRenderFrame *frame, Uint32& y, Uint32 x )
	{
		TDynArray< IActorTerminationListener* >& terminationListenrs = npc->GetTerminationListeners();

		CEncounter* encounter = nullptr;
		for( Uint32 i=0; i < terminationListenrs.Size(); ++i  )
		{
			encounter = terminationListenrs[ i ]->AsEncounter();
			if( encounter )
			{
				break;
			}
		}

		frame->AddDebugScreenText( x, y, String::Printf( TXT("NPC spawned from Encounter: %s"), encounter ? TXT("yes") : TXT("no") ) );
		y += 15;
		if( encounter )
		{
			frame->AddDebugScreenText( x, y, String::Printf( TXT("Encounter layer: %s"), encounter->GetLayer()->GetFriendlyName().AsChar() ) );
			y += 15;
		}
	}

	//! Generate debug viewport fragments
	virtual void OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
	{
#ifndef NO_DEBUG_WINDOWS
		String message = TXT("This debug page is converted to debug window. If you want use it, click key: Enter.");

		frame->AddDebugRect(49, 80, 502, 20, Color(100,100,100,255));
		frame->AddDebugScreenFormatedText( 60, 95, Color(255, 255, 255, 255), TXT("Debug Message Box"));

		frame->AddDebugRect(50, 100, 500, 50, Color(0,0,0,255));
		frame->AddDebugFrame(50, 100, 500, 50, Color(100,100,100,255));

		frame->AddDebugScreenFormatedText( 60, 120, Color(127, 255, 0, 255), message.AsChar());

		frame->AddDebugScreenFormatedText( 275, 140, Color(255, 255, 255), TXT(">> OK <<"));
		return;
#endif

		Uint32 x = 255;

		const Uint32 width = frame->GetFrameOverlayInfo().m_width;
		const Uint32 height = frame->GetFrameOverlayInfo().m_height;

		// Draw npcs tree
		m_npcTree->m_height = height - 130;
		m_npcTree->OnRender(frame);

		// Draw legend
		frame->AddDebugRect( 250, 50, width-300, 50, Color( 50, 50, 50, 255 ) );
		frame->AddDebugFrame( 250, 50, width-300, 50, Color( 0, 0, 0, 128 ));
		frame->AddDebugScreenText( 260, 65, TXT("LEGEND:"), Color::WHITE );
		frame->AddDebugScreenText( 260, 80, TXT("Up / Down - choose NPC"), Color::WHITE );
		frame->AddDebugScreenText( 400, 80, TXT("Enter - show info"), Color::WHITE );
		frame->AddDebugScreenText( 530, 80, TXT("Backspace - hide info"), Color::WHITE );
		frame->AddDebugScreenText( 700, 80, TXT("Page Up / Page Down - move info"), Color::WHITE );

		// No NPC
		if ( m_active == -1 )
		{
			return;
		}

		// Draw info background
		frame->AddDebugRect( 250, 100, width-300, height-180, Color( 0, 0, 0, 128 ) );
		frame->AddDebugFrame( 250, 100, width-300, height-180, Color::WHITE );

		// NPC name
		Uint32 y = m_verticalOffset + 115;

		String caption = String::Printf( TXT("NPC ( %i of %i )"), m_active+1, m_allNpcs.Size() );
		frame->AddDebugScreenText( x, y, caption, Color::WHITE );

		y += 15;

		// NPC is lost
		CNewNPC* npc = m_allNpcs[ m_active ].Get();
		if ( !npc )
		{
			frame->AddDebugScreenText( x, y, TXT("SIGNAL LOST"), Color::RED );
			return;
		}

		// General properties
		{
			// Header
			frame->AddDebugScreenText( x, y, TXT("General"), Color::YELLOW );
			y += 15;

			// Template name
			String templateName = npc->GetEntityTemplate()
				? npc->GetEntityTemplate()->GetFriendlyName()
				: TXT("Unknown");
			frame->AddDebugScreenText( x, y, String::Printf( TXT("Template: '%ls'"), templateName.AsChar() ), Color::WHITE );
			y += 15;

			// Mem usage
			Uint32 memoryUsageByObject = CObjectMemoryAnalizer::CalcObjectSize( npc ) / 1024;
			Uint32 memoryUsageByTemplate = CObjectMemoryAnalizer::CalcObjectSize( npc->GetEntityTemplate() ) / 1024;
			frame->AddDebugScreenText( x, y, String::Printf( TXT("Memory usage by object: '%dkB'"), memoryUsageByObject ), Color::YELLOW );
			y += 15;
			frame->AddDebugScreenText( x, y, String::Printf( TXT("Memory usage by template: '%dkB'"), memoryUsageByTemplate ), Color::YELLOW );
			y += 15;

			// Tag
			String tags = npc->GetTags().ToString();
			frame->AddDebugScreenText( x, y, String::Printf( TXT("Tags: '%ls'"), tags.AsChar() ), Color::WHITE );
			y += 15;

			// Appearance
			CAppearanceComponent* appearanceComponent = npc->GetAppearanceComponent();
			if ( appearanceComponent )
			{
				String appearance = appearanceComponent->GetAppearance().AsString();
				frame->AddDebugScreenText( x, y, String::Printf( TXT("Appearance: '%ls'"), appearance.AsChar() ), Color::WHITE );
				y += 15;
			}

			// Voice tag
			String voiceTag = npc->GetVoiceTag().AsString();
			frame->AddDebugScreenText( x, y, String::Printf( TXT("VoiceTag: '%ls'"), voiceTag.AsChar() ), Color::WHITE );
			y += 15;

			// Attitude group name
			CAIProfile* profileRecursive = npc->GetEntityTemplate() ? npc->GetEntityTemplate()->FindParameter< CAIProfile >( true ) : NULL;
			String attitudeGroup = String::Printf( TXT("Attitude group: %s (def. %s)"), npc->GetAttitudeGroup().AsString().AsChar(), (profileRecursive ? profileRecursive->GetAttitudeGroup().AsString().AsChar() : TXT("No AI profile defined")) );
			frame->AddDebugScreenText( x, y, attitudeGroup );
			y += 15;
		}

		// Script
		{
			// Header
			y += 5;
			frame->AddDebugScreenText( x, y, TXT("Script"), Color::YELLOW );
			y += 15;

			// Get state name
			String stateName = npc->GetCurrentStateName().AsString();
			frame->AddDebugScreenText( x, y, String::Printf( TXT("State: '%ls'"), stateName.AsChar() ), Color::WHITE );
			y += 15;

			// Get current thread
			const CFunction* topFunction = npc->GetTopLevelFunction();
			frame->AddDebugScreenText( x, y, String::Printf( TXT("Function: '%ls'"), topFunction ? topFunction->GetName().AsString().AsChar() : TXT("None") ), Color::WHITE );
			y += 15;
		}

		// Community and work
		{
			CCommunitySystem *cs = GCommonGame->GetSystem< CCommunitySystem >();
			if ( cs )
			{
				// Header
				y += 5;
				frame->AddDebugScreenText( x, y, TXT("Community/Encounter and working info"), Color::YELLOW );
				y += 15;

				// Encounter info
				AddEncounterInfo( npc, frame, y, x );

				// Is NPC in Community
				const SAgentStub *agentStub = cs->FindStubForNPC( npc );
				frame->AddDebugScreenText( x, y, String::Printf( TXT("Is NPC in Community: %s"), agentStub ? TXT("yes") : TXT("no") ) );
				y += 15;

				// Agent stub info
				if ( agentStub )
				{
					// Story phase
					const CName storyPhase = agentStub->GetActivePhaseName();
					if ( storyPhase == CName::NONE )
					{
						frame->AddDebugScreenText( x, y, String::Printf( TXT("Story phase: Default") ) );
					}
					else
					{
						frame->AddDebugScreenText( x, y, String::Printf( TXT("Story phase: %s"), storyPhase.AsString().AsChar() ) );
					}
					y += 15;

					// Community
					frame->AddDebugScreenText( x, y, agentStub->GetSpawnsetName().AsChar() );
					y += 15;

					// Agent stub state
					frame->AddDebugScreenText( x, y, String::Printf(TXT("Current stub state: %s"), CCommunityUtility::GetFriendlyAgentStateName( agentStub->m_state ).AsChar()) );
					y += 15;
				}

				// NPC Action Schedule
				const NewNPCScheduleProxy &npcSchedule = npc->GetActionSchedule();
				if ( npcSchedule.GetActiveAP() != ActionPointBadID )
				{
					frame->AddDebugScreenText( x, y, String::Printf(TXT("Active AP: %s"), GCommonGame->GetSystem< CCommunitySystem >()->GetActionPointManager()->GetFriendlyAPName( npcSchedule.GetActiveAP() ).AsChar()) );
				}
				else
				{
					frame->AddDebugScreenText( x, y, String::Printf(TXT("Active AP is EMPTY")) );
				}
				y += 15;
				frame->AddDebugScreenText( x, y, String::Printf(TXT("Is using last AP: %s"), npcSchedule.UsesLastAP() ? TXT("true") : TXT("false")) );
				y += 15;

				// Actor action work
				frame->AddDebugScreenText( x, y, String::Printf(TXT("Actor Action Work")), Color::YELLOW );
				y += 15;
			}
		}

		// Inventory
		{
			// Header
			y += 5;
			frame->AddDebugScreenText( x, y, TXT("Inventory"), Color::YELLOW );
			y += 15;

			// Get inventory component
			CInventoryComponent* ic = npc->GetInventoryComponent();
			if ( ic )
			{
				const auto & items = ic->GetItems();
				if ( items.Empty() )
				{
					frame->AddDebugScreenText( x, y, TXT("Empty"), Color::WHITE );
					y += 15;
				}
				else
				{
					// Print items
					Uint32 index = 0;
					for ( auto it=items.Begin(); it!=items.End(); ++it, ++index )
					{
						const SInventoryItem& item = *it;

						// Item info
						String text = String::Printf( TXT("[%i]: %s"), index, item.GetName().AsString().AsChar() );
						if ( item.GetQuantity() > 1 ) text += String::Printf( TXT("  x%i"), item.GetQuantity() );
						if ( item.IsHeld() ) text += TXT("  HELD");
						if ( item.IsMounted() ) text += TXT(" MOUNTED");
						//if ( item.IsCloned() ) text += TXT(" CLONED");
						frame->AddDebugScreenText( x, y, text, Color::WHITE );
						y += 15;

						// Don't display to much..
						if ( index > 8 )
						{
							frame->AddDebugScreenText( x, y, TXT("more..."), Color::GRAY );
							y += 15;
							break;
						}
					}
				}
			}
			else
			{
				frame->AddDebugScreenText( x, y, TXT("No inventory component"), Color::WHITE );
				y += 15;
			}
		}

		// Right column
		y = m_verticalOffset + 120;
		x = 700;

		// Moving agent component
		{
			// Header
			y += 5;
			frame->AddDebugScreenText( x, y, TXT("Movement"), Color::YELLOW );
			y += 15;

			// Print shit
			CMovingAgentComponent* mac = npc->GetMovingAgentComponent();
			if ( mac )
			{
				if ( mac->IsMotionEnabled() )
				{
					frame->AddDebugScreenText( x, y, TXT("ENABLED"), Color::GREEN );
					y += 15;
				}
				else
				{
					frame->AddDebugScreenText( x, y, TXT("DISABLED"), Color::RED );
					y += 15;
				}

				// Teleport info
				const Vector& teleportPos = mac->GetTeleportedPosition();
				const EulerAngles& teleportRot = mac->GetTeleportedRotation();

				frame->AddDebugScreenText( x, y, String::Printf(
					TXT( "Last teleport: pos( %.2f, %.2f, %.2f ), rot( %.2f, %.2f, %.2f )" ),
					teleportPos.X, teleportPos.Y, teleportPos.Z,
					teleportRot.Yaw, teleportRot.Pitch, teleportRot.Roll ), Color::WHITE );
				y += 15;

				// Top representation
				frame->AddDebugScreenText( x, y, String::Printf( TXT( "Active representation: %s" ), mac->GetActiveRepresentationName().AsChar() ), Color::WHITE );
				y += 15;

				CMovingPhysicalAgentComponent* physicalMac = Cast< CMovingPhysicalAgentComponent >( mac );
				if ( physicalMac != NULL )
				{
					Float playerInteractionPriority = 0.0f;
					Uint32 playerInteractionTableIndex = 0;

					CMovingPhysicalAgentComponent* playerPhysicalMac = NULL;
					if ( GCommonGame != NULL && GCommonGame->GetPlayer() != NULL && GCommonGame->GetPlayer()->GetMovingAgentComponent() != NULL )
					{
						playerPhysicalMac = Cast< CMovingPhysicalAgentComponent >( GCommonGame->GetPlayer()->GetMovingAgentComponent() );
					}

// 					if ( playerPhysicalMac != NULL )
// 					{
// 						playerInteractionPriority = playerPhysicalMac->GetActualInteractionPriority();
// 						playerInteractionTableIndex = playerPhysicalMac->GetInteractionTableIndex();
// 					}

					//frame->AddDebugScreenText( x, y, String::Printf( TXT( "Character movement priority: %.2f (Player: %.2f) " ), physicalMac->GetActualInteractionPriority(), playerInteractionPriority ), Color::WHITE );
					y += 15;
					//frame->AddDebugScreenText( x, y, String::Printf( TXT( "Movement queue position: %d (Player: %d)" ), physicalMac->GetInteractionTableIndex(), playerInteractionTableIndex  ), Color::WHITE );
					y += 15;
				}

				// Locomotion data
				TDynArray< String > lines;
				mac->GetLocoDebugInfo( lines );

				if( lines.Size() > 0 )
				{
					// Locomotion
					frame->AddDebugScreenText( x, y, TXT("Locomotion"), Color::YELLOW );
					y += 15;
				}

				for( TDynArray< String >::const_iterator lineIter = lines.Begin();
					lineIter != lines.End(); ++lineIter )
				{
					frame->AddDebugScreenText( x, y, *lineIter, Color::WHITE );
					y += 15;
				}

				const CStaticMovementTargeter* staticTarget = mac->GetStaticTarget();
				if( staticTarget )
				{
					if( staticTarget->IsRotationTargetSet() )
					{
						frame->AddDebugScreenText( x, y, TXT( "Static rotation is set" ), Color::WHITE );
						y += 15;
					}
				}
			}
			else
			{
				frame->AddDebugScreenText( x, y, TXT("NO COMPONENT"), Color::RED );
				y += 15;
			}
		}

		{
			// Header
			y += 5;
			frame->AddDebugScreenText( x, y, TXT("Quest locks"), Color::YELLOW );
			y += 15;

			// Quest locks
			const TDynArray< NPCQuestLock* >& locks = npc->GetQuestLockHistory();
			for ( Uint32 i=0; i<locks.Size(); ++i )
			{
				NPCQuestLock* lock = locks[i];

				if ( lock->GetLockState() )
				{
					frame->AddDebugScreenText( x, y, TXT("LOCK"), Color::RED );
				}
				else
				{
					frame->AddDebugScreenText( x, y, TXT("UNLOCK"), Color::GREEN );
				}

				String txt = lock->GetQuestBlock()->GetCaption().AsChar();
				txt += TXT(" in ");
				txt += lock->GetQuestPhase()->GetFriendlyName().AsChar();
				frame->AddDebugScreenText( x + 60, y, txt.AsChar(), Color::WHITE );
				y += 15;
			}
		}

		{
			// Noticed objects

			// Header
			y += 5;
			frame->AddDebugScreenText( x, y, TXT("Noticed objects"), Color::YELLOW );
			y += 15;

			const TDynArray< NewNPCNoticedObject >& noticedObjects = npc->GetNoticedObjects();
			for ( TDynArray< NewNPCNoticedObject >::const_iterator noticedObj = noticedObjects.Begin(); noticedObj != noticedObjects.End(); ++noticedObj )
			{
				frame->AddDebugScreenText( x, y, noticedObj->ToString(), Color::WHITE );
				y += 15;
			}
		}

		{
			// Attitude

			// Header
			y += 5;
			frame->AddDebugScreenText( x, y, TXT("Attitudes"), Color::YELLOW );
			y += 15;

			TActorAttitudeMap attMap;
			npc->GetAttitudeMap( attMap );
			TActorAttitudeMap::const_iterator iter = attMap.Begin();
			String name, attitude;
			CEnum* attEnum = SRTTI::GetInstance().FindEnum( CNAME( EAIAttitude ) );
			ASSERT( attEnum );
			for( ; iter!= attMap.End(); ++iter )
			{
				if ( iter->m_first != NULL )
					name = iter->m_first->GetName();
				else
					name = TXT("NULL");

				attitude.Clear();
				Bool res = attEnum->ToString( &(iter->m_second), attitude );
				ASSERT( res );
				frame->AddDebugScreenText( x, y, String::Printf( TXT("Actor: %s, attitude: %s"), name.AsChar(), attitude.AsChar() ), Color::WHITE );
				y += 15;
			}
		}

		// Meshes
		{
			// Header
			y += 5;
			frame->AddDebugScreenText( x, y, TXT("Mesh components"), Color::YELLOW );
			y += 15;

			for ( ComponentIterator< CMeshTypeComponent > it( npc ); it; ++it )
			{
				CMeshTypeComponent* meshComponent = *it;

				String debugText;
				if ( meshComponent->GetMeshTypeResource() )
				{
					debugText = meshComponent->GetMeshTypeResource()->GetDepotPath();
				}
				else
				{
					debugText = TXT("Unknown");
				}
				frame->AddDebugScreenText( x, y, debugText, meshComponent->IsVisible() ? Color::GREEN : Color::RED );
				y += 15;
			}
		}

		// Story information
		{
			// Header
			y += 5;
			frame->AddDebugScreenText( x, y, TXT("Story informations - active locks:"), Color::YELLOW );
			y += 15;

			const TDynArray< NPCQuestLock* >& quests = npc->GetQuestLockHistory();
			for( Uint32 i=0; i<quests.Size(); ++i)
			{
				if(quests[i]->GetLockState() == true)
				{
					CQuestPhase* questPhase = quests[i]->GetQuestPhase();
					CQuestGraphBlock* block =  quests[i]->GetQuestBlock();

					//String text = String::Printf( TXT("[%i]: %s"), i, block->GetBlockName().AsChar() );
					String text = String::Printf( TXT("[%i]: %s"), i, questPhase->GetFriendlyName().AsChar() );
					frame->AddDebugScreenText( x, y, text);
					y += 15;
				}
			}
		}
	}

	//! Calculate camera orbit point for given NPC
	Vector CalculateCameraOrbit( CNewNPC* npc )
	{
		// Try to use neck bone :)
		CAnimatedComponent* ac = npc->GetRootAnimatedComponent();
		if ( ac )
		{
			Int32 boneIndex = ac->FindBoneByName( TXT("neck") );
			if ( boneIndex != -1 )
			{
				Matrix boneToWorld = ac->GetBoneMatrixWorldSpace( boneIndex );
				return boneToWorld.GetTranslation();
			}
		}

		// Start with center of the bounding box + some offset
		Uint32 numMeshes = 0;
		Vector pos = Vector::ZEROS;
		for ( ComponentIterator< CMeshTypeComponent > it( npc ); it; ++it )
		{
			// Calculate bounding box
			const Vector meshCenter = (*it)->GetBoundingBox().CalcCenter();
			pos += meshCenter;
			numMeshes += 1;
		}

		// Use center of drawable shit
		if ( numMeshes )
		{
			pos /= ( Float ) numMeshes;
			return pos + Vector( 0.0f, 0.0f, 0.7f );
		}

		// Use entity position
		return npc->GetWorldPosition();
	}

	//! Override camera
	virtual Bool OnViewportCalculateCamera( IViewport* view, CRenderCamera &camera )
	{
		// Show the selected actor :)
		if ( m_active >= 0 )
		{
			CNewNPC* npc = m_allNpcs[ m_active ].Get();
			if ( npc )
			{
				// Orbit camera around NPC
				Vector pos = m_cameraPosition;//CalculateCameraOrbit( npc );
				EulerAngles rot( 0.0f, 0.0f, m_cameraRotation );

				// Offset camera
				Vector dir, right;
				rot.ToAngleVectors( &dir, &right, NULL );
				pos -= dir * 1.5f;

				// Construct preview camera
				CRenderCamera previewCamera( pos, rot, camera.GetFOV(), camera.GetAspect(), camera.GetNearPlane(), camera.GetFarPlane(), camera.GetZoom() );
				camera = previewCamera;
				return true;
			}
		}

		// Not
		return false;
	}
};

void CreateDebugPageNPCList()
{
	IDebugPage* page = new CDebugPageNPCList();
	IDebugPageManagerBase::GetInstance()->RegisterDebugPage( page );
}

void CDebugCommandNPC::Process()
{
	m_parentSystem->SyncToNewNPC(m_npcIndex);
}

#endif
