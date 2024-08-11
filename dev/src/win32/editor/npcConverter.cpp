
#include "build.h"
#include "npcConverter.h"
#include "checkListDlg.h"
#include "../../common/game/storySceneComponent.h"
#include "../../common/game/storySceneParam.h"
#include "../../common/game/bgNpcInteraction.h"
#include "../../common/game/bgNpcTrigger.h"
#include "../../common/game/bgNpcItem.h"
#include "../../common/game/bgNpcRoot.h"
#include "../../common/game/bgNpcMesh.h"
#include "../../common/game/bgNpc.h"

#include "../../common/core/depot.h"
#include "../../common/engine/behaviorGraph.h"
#include "../../common/engine/meshSkinningAttachment.h"
#include "../../common/engine/meshComponent.h"
#include "../../common/engine/mesh.h"

IMPLEMENT_ENGINE_CLASS( CEntityConverter );
IMPLEMENT_ENGINE_CLASS( CNewNpcToBgNpcConverter );

Bool CNewNpcToBgNpcConverter::CanConvert( const CClass* entityObjClass ) const
{
	return entityObjClass->IsA< CNewNPC >();
}

CEntityTemplate* CNewNpcToBgNpcConverter::Convert( const CEntityTemplate* temp, const CEntity* templEntity )
{
	CEntity* entity = CreateObject< CEntity >();
	const CNewNPC* oldNpc = Cast< CNewNPC >( templEntity );

	//////////////////////////////////////////////////////////////////////////
	// 1. Create root
	CBgRootComponent* root = CreateRootComponent( entity, oldNpc->GetRootAnimatedComponent() );

	//////////////////////////////////////////////////////////////////////////
	// 2. Meshes
	for ( ComponentIterator< CMeshComponent > it( oldNpc ); it; ++it )
	{
		CMeshComponent* meshComp = *it;
		AddMeshComponent( entity, root, meshComp, oldNpc );
	}

	//////////////////////////////////////////////////////////////////////////
	// 3. Head
	/*const CHeadComponent* head = oldNpc->GetHeadComponent();
	if ( head )
	{
		TDynArray< CMesh* > meshes;
		head->GetMimicLowMeshes( meshes );

		if ( meshes.Size() > 1 )
		{
			for ( Uint32 i=0; i<meshes.Size(); ++i )
			{
				CMesh* meshComp = meshes[ i ];
				AddMesh( entity, root, meshComp, head->GetName(), (Int32)i );
			}
		}
		else if ( meshes.Size() == 1 )
		{
			CMesh* meshComp = meshes[ 0 ];
			AddMesh( entity, root, meshComp, head->GetName(), -1 );
		}
	}*/

	//////////////////////////////////////////////////////////////////////////
	// 4. Collect scenes
	TDynArray< CStoryScene* > scenes;
	TDynArray< String > scenesName;
	TDynArray< Bool > scenesNameStates;
	for ( ComponentIterator< CStorySceneComponent > it( oldNpc ); it; ++it )
	{
		CStorySceneComponent* sceneComp = *it;
		
		TSoftHandle< CStoryScene > sceneHandle = sceneComp->GetStoryScene();

		if ( sceneHandle.Get() )
		{
			scenes.PushBack( sceneHandle.Get() );
			scenesName.PushBack( sceneComp->GetName().AsChar() );
			scenesNameStates.PushBack( false );

			sceneHandle.Release();
		}
	}
	if ( scenesNameStates.Size() > 0 )
	{
		scenesNameStates[ 0 ] = true;
	}

	//////////////////////////////////////////////////////////////////////////
	// Fill template
	CEntityTemplate* newTempl = ::CreateObject< CEntityTemplate >();
	newTempl->CaptureData( entity );
	newTempl->SetEntityClass( GetDestClass() );

	// Reload entity
	EntityTemplateInstancingInfo instanceInfo;
	instanceInfo.m_previewOnly = true;
	entity = newTempl->CreateInstance( templEntity->GetLayer(), instanceInfo );

	// Setup new entity object
	CBgNpc* newNpc = SafeCast< CBgNpc >( entity );
	newNpc->CopyFrom( Cast< const CNewNPC >( templEntity ) );

	// Add scenes to entity template
	{
		CEdCheckListDialog* dlg = new CEdCheckListDialog( NULL, TXT("Choose scenes"), scenesName, scenesNameStates );
		dlg->ShowModal();
	}

	if ( scenesNameStates.Size() > 0 )
	{
		CVoicesetParam* voicesetParams = newTempl->FindParameter< CVoicesetParam >();
		if ( !voicesetParams )
		{
			voicesetParams = new CVoicesetParam();
			VERIFY( newTempl->AddParameterUnique( voicesetParams ) );
		}

		for ( Uint32 i=0; i<scenesNameStates.Size(); ++i )
		{
			if ( scenesNameStates[ i ] )
			{
				CStoryScene* scene = scenes[ i ];
				const String& sceneName = scenesName[ i ];
				CName voiceTag = CName( scenesName[ i ].StringAfter( TXT("voiceset_") ).ToUpper() );

				voicesetParams->AddVoiceset( sceneName, scene, voiceTag );
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// OMG Items
	const CInventoryComponent* inv = oldNpc->GetInventoryComponent();
	if ( inv )
	{
		const auto& items = inv->GetItems();
		TDynArray< String > itemsToImport;
		TDynArray< Bool > itemsToImportStates;

		// 4. Collect items and ask user
		const Uint32 size = items.Size();

		itemsToImport.Reserve( size );
		itemsToImportStates.Reserve( size );

		for ( Uint32 i=0; i<size; ++i )
		{
			const SInventoryItem& item = items[ i ];

			itemsToImport.PushBack( item.GetName().AsString() );

			if ( ShouldAddItem( inv, item ) )
			{
				itemsToImportStates.PushBack( true );
			}
			else
			{
				itemsToImportStates.PushBack( false );
			}
		}

		if ( itemsToImport.Size() > 0 )
		{
			TSet< String > chosen;
			CEdCheckListDialog* dlg = new CEdCheckListDialog( NULL, TXT("Choose items"), itemsToImport, itemsToImportStates, chosen );
			dlg->ShowModal();

			// 5. Copy slots
			for ( Uint32 i=0; i<size; ++i )
			{
				const SItemDefinition* itemDef = GCommonGame->GetDefinitionsManager()->GetItemDefinition( items[ i ].GetName() );
				if ( itemDef )
				{
					AddSlotToTempl( itemDef->GetEquipSlot( entity->IsA< CPlayer >() ), newTempl, temp );
					AddSlotToTempl( itemDef->GetHoldSlot( entity->IsA< CPlayer >() ), newTempl, temp );
				}
			}

			// 6. Suck items
			for ( Uint32 i=0; i<size; ++i )
			{
				const SInventoryItem& item = items[ i ];

				const String itemStr = item.GetName().AsString();

				if ( chosen.Find( itemStr ) != chosen.End() )
				{
					SComponentSpawnInfo spawnInfo;
					spawnInfo.m_name = item.GetName().AsString();
					CBgNpcItemComponent* newComp = SafeCast< CBgNpcItemComponent >( entity->CreateComponent( CBgNpcItemComponent::GetStaticClass(), spawnInfo ) );
					newComp->SetLightChannels( LC_Characters );

					EItemState state;
					if ( item.IsMounted() )
					{
						state = IS_MOUNT;
					}
					else if ( item.IsHeld() )
					{
						state = IS_HOLD;
					}
					else
					{
						state = IS_HIDDEN;
					}

					newComp->SetItemDefaultState( state );
					newComp->SetItemName( item.GetName() );
				}
			}
		}
	}

	// Bg Trigger
	CBgNpcTriggerComponent* trigger = NULL;

	if ( scenesNameStates.Size() )
	{
		if ( !trigger )
		{
			trigger = CreateTrigger( entity );
		}

		SComponentSpawnInfo spawnInfo;
		spawnInfo.m_name = TXT("talk");
		CBgInteractionComponent* inteaction = SafeCast< CBgInteractionComponent >( newNpc->CreateComponent( CBgInteractionComponent::GetStaticClass(), spawnInfo ) );
		inteaction->InitializeComponent();

		trigger->AddVoicesetOption();
	}

	// Ask for look at
	if ( YesNo( TXT("Do you want to add look at reaction?") ) )
	{
		if ( !trigger )
		{
			trigger = CreateTrigger( entity );
		}

		trigger->AddLookAtOption();
	}

	// Capture final entity ( detach template )
	entity->DetachTemplate();
	newTempl->CaptureData( entity );

	// Done!
	return newTempl;
};

CBgNpcTriggerComponent* CNewNpcToBgNpcConverter::CreateTrigger( CEntity* newNpc ) const
{
	SComponentSpawnInfo spawnInfo;
	spawnInfo.m_name = TXT("trigger");
	CBgNpcTriggerComponent* trigger = SafeCast< CBgNpcTriggerComponent >( newNpc->CreateComponent( CBgNpcTriggerComponent::GetStaticClass(), spawnInfo ) );
	trigger->InitializeComponent();

	return trigger;
}

CClass* CNewNpcToBgNpcConverter::GetDestClass() const
{
	return ClassID< CBgNpc >();
}

void CNewNpcToBgNpcConverter::AddSlotToTempl( const CName& slotName, CEntityTemplate* newTempl, const CEntityTemplate* oldTempl ) const
{
	if ( slotName == CName::NONE )
	{
		return;
	}

	if ( newTempl->FindSlotByName( slotName, true ) )
	{
		return;
	}

	const EntitySlot* slot = oldTempl->FindSlotByName( slotName, true );
	if ( slot )
	{
		EntitySlotInitInfo initInfo;
		initInfo.m_boneName = slot->GetBoneName();
		initInfo.m_componentName = slot->GetComponentName();
		initInfo.m_transform = slot->GetTransform();

		VERIFY( newTempl->AddSlot( slotName, &initInfo ) );
	}
}

void CNewNpcToBgNpcConverter::AddMeshComponent( CEntity* entity, CComponent* entityRoot, CMeshComponent* meshComp, const CNewNPC* oldNpc ) const
{
	if ( meshComp->GetNumberOfRenderProxies() == 0 )
	{
		return;
	}

	// Clone
	SComponentSpawnInfo spawnInfo;
	spawnInfo.m_name = meshComp->GetName();
	CMeshComponent* newComp = Cast< CMeshComponent >( entity->CreateComponent( CMeshComponent::GetStaticClass(), spawnInfo ) );
	newComp->SetAsCloneOf( meshComp );
	newComp->SetLightChannels( LC_Characters );

	// Attach
	if ( meshComp->GetTransformParent() && 
		meshComp->GetTransformParent()->GetParent() == oldNpc->GetRootAnimatedComponent() )
	{
		entityRoot->Attach( newComp, meshComp->GetTransformParent()->GetClass() );
	}
}

void CNewNpcToBgNpcConverter::AddMesh( CEntity* entity, CComponent* entityRoot, CMesh* mesh, const String& name, Int32 i ) const
{
	if ( !mesh )
	{
		return;
	}

	// Clone
	String nameStr = i != -1 ? String::Printf( TXT("%s_%d"), name.AsChar(), i ) : name;
	SComponentSpawnInfo spawnInfo;
	spawnInfo.m_name = nameStr;

	if( CMeshComponent* newComp = Cast< CMeshComponent >( entity->CreateComponent( CMeshComponent::GetStaticClass(), spawnInfo ) ) )
	{
		newComp->SetResource( mesh );
		newComp->SetLightChannels( LC_Characters );

		// Attach
		entityRoot->Attach( newComp, CMeshSkinningAttachment::GetStaticClass() );
	}
}

Bool CNewNpcToBgNpcConverter::ShouldAddItem( const CInventoryComponent* inv, const SInventoryItem& item ) const
{
	const SInventoryItem* invItem = inv->GetItem( item.GetUniqueId() );
	if ( !invItem )
	{
		return false;
	}

	return invItem->IsMounted();
}

CBgRootComponent* CNewNpcToBgNpcConverter::CreateRootComponent( CEntity* npc, const CAnimatedComponent* oldRoot )
{
	CAnimatedComponent::SetupData data;
	ResourceLoadingContext contextSet;
	data.m_set = LoadResource< CSkeletalAnimationSet >( TXT("characters\\templates\\background\\idle.w2anims"), contextSet );

	data.m_skeleton = oldRoot ? oldRoot->GetSkeleton() : NULL;

	SBehaviorGraphInstanceSlot slot;
	slot.m_instanceName = CNAME( idle );
	ResourceLoadingContext contextGraph;
	slot.m_graph = LoadResource< CBehaviorGraph >( TXT("characters\\templates\\background\\idle.w2beh"), contextGraph );

	data.m_slot = &slot;

	SComponentSpawnInfo spawnInfo;
	spawnInfo.m_name = TXT("Root");
	CBgRootComponent* ac = SafeCast< CBgRootComponent >( npc->CreateComponent( CBgRootComponent::GetStaticClass(), spawnInfo ) );
	ac->Setup( data );

	return ac;
}
