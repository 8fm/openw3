/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "../game/behTreeMachine.h"
#include "../game/behTree.h"
#include "../game/behTreeNode.h"
#include "../game/behTreeNodeComposite.h"

#include "../../common/engine/debugWindowsManager.h"
#include "../engine/debugPage.h"
#include "../engine/debugPageManagerBase.h"
#include "../engine/meshTypeComponent.h"
#include "../engine/renderFrame.h"

#ifndef NO_DEBUG_PAGES

#define X_START 55
#define LINE_INCREMENT 15
#define NODE_INDENT 10

/// Interactive list of NPC
class CDebugPageBehaviorTree : public IDebugPage
{
protected:
	TDynArray< THandle< CNewNPC > >		m_allNpcs;				//!< All NPCs in the world
	TDynArray< String >					m_npcNames;				//!< NPC names
	Vector								m_cameraTarget;			//!< Current camera target
	Vector								m_cameraPosition;		//!< Current camera position
	String								m_activeNpcName;		//!< Name of the selected NPC
	Int32									m_active;				//!< Active NPC
	Float								m_cameraRotation;		//!< NPC camera rotation
	Int32									m_verticalOffset;		//!< Verical offset of text

public:
	CDebugPageBehaviorTree()
		: IDebugPage( TXT("BehTree") )
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

			// Is it the last active NPC ?
			if ( name == m_activeNpcName )
			{
				// Start browsing from last active NPC
				m_active = m_allNpcs.Size() - 1;
				SyncToNewNPC();
			}
		}

		// Use current camera target
		m_cameraPosition = m_cameraTarget;

		// Pause game
		GGame->Pause( TXT( "CDebugPageBehaviorTree" ) );
	}

	//! This debug page was hidden
	virtual void OnPageHidden()
	{
		IDebugPage::OnPageHidden();

		// Unpause game
		GGame->Unpause( TXT( "CDebugPageBehaviorTree" ) );
	}

	//! External viewport tick
	virtual void OnTick( Float timeDelta )
	{
		// Update virtual camera rotation
		m_cameraRotation += timeDelta * 30.0f;

		// Interpolate
		Vector cameraDist = m_cameraTarget - m_cameraPosition;
		const Float dist = cameraDist.Mag3();
		if ( dist > 0.0f )
		{
			// Fly to target
			const Float move = Min< Float >( dist, timeDelta * 20.0f );
			m_cameraPosition += cameraDist.Normalized3() * move;
		}
	}

	//! Sync shit
	void SyncToNewNPC()
	{
		// Reset vertical offset
		m_verticalOffset = 0;

		// Reset
		m_activeNpcName = TXT("");
		//m_cameraRotation = 0.0f;

		// Reset camera
		m_cameraTarget = GGame->GetActiveWorld()->GetCameraDirector()->GetCameraPosition();

		// Is new shit set ?
		if ( m_active != -1 )
		{
			// Cache name
			m_activeNpcName = m_npcNames[ m_active ];

			// Reset rotation
			CNewNPC* npc = m_allNpcs[ m_active ].Get();
			if ( npc )
			{
				//m_cameraRotation = npc->GetWorldYaw() + 180.0f;
				m_cameraTarget = CalculateCameraOrbit( npc );
			}
		}
	}

	//! Generalized input event
	virtual Bool OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
	{
#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS
		if( action == IACT_Press && key == IK_Enter )
		{
			GDebugWin::GetInstance().SetVisible(true);
		}
		return true;
#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI

		// Go to previous NPC
		if ( ( key == IK_Left || key == IK_Pad_DigitLeft ) && action == IACT_Press )
		{
			m_active = m_active - 1;
			if ( m_active < -1 ) m_active = (Int32)m_allNpcs.Size() - 1;
			SyncToNewNPC();
			return true;
		}

		// Go to next NPC
		if ( ( key == IK_Right || key == IK_Pad_DigitRight ) && action == IACT_Press )
		{
			m_active = m_active + 1;
			if ( m_active >= (Int32)m_allNpcs.Size() ) m_active = -1;			
			SyncToNewNPC();
			return true;
		}

		// Change vertical offset
		if( key == IK_Down && action == IACT_Press )
		{
			m_verticalOffset -= 10;
			return true;
		}

		if( key == IK_Up && action == IACT_Press )
		{
			m_verticalOffset += 10;
			if( m_verticalOffset > 0 )
			{
				m_verticalOffset = 0;
			}
			return true;
		}

		// Reset camera
		if ( ( key == IK_Enter || key == IK_Pad_A_CROSS ) && action == IACT_Press )
		{
			m_cameraPosition = m_cameraTarget;
			return true;
		}

		if( key == IK_P )
		{

			CNewNPC* npc = m_allNpcs[ m_active ].Get();
			if ( npc )
			{
				String templateName = npc->GetEntityTemplate()? npc->GetEntityTemplate()->GetFriendlyName()	: TXT("Unknown");
				LOG_GAME( TXT( "Template: '%ls'\n" ), templateName.AsChar() );

				CBehTreeMachine* behTM = npc->GetBehTreeMachine();
				if ( behTM != NULL )
				{
					LOG_GAME( ( String::Printf( TXT("Beh Machine Info: ") ) + behTM->GetInfo() ).AsChar() );
				}
				

				//const CBehTree* tree = behTM->GetTree();

				//if( tree )
				//{
				//	// Nodes
				//	const IBehTreeNodeDefinition* rootNode = tree->GetRootNode();

				//	RecurseTreeNodes( rootNode, 0, X_START, NULL );
				//}
			}
		}

		// Not handled
		return false;
	}

	//! Generate debug viewport fragments
	virtual void OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
	{
#ifndef NO_DEBUG_WINDOWS
		String message = TXT("This debug page is destined to remove. Click key 'Enter' to open new debug framework.");

		frame->AddDebugRect(49, 80, 502, 20, Color(100,100,100,255));
		frame->AddDebugScreenFormatedText( 60, 95, Color(255, 255, 255, 255), TXT("Debug Message Box"));

		frame->AddDebugRect(50, 100, 500, 50, Color(0,0,0,255));
		frame->AddDebugFrame(50, 100, 500, 50, Color(100,100,100,255));

		frame->AddDebugScreenFormatedText( 70, 120, Color(255, 0, 0, 255), message.AsChar());

		frame->AddDebugScreenFormatedText( 275, 140, Color(255, 255, 255), TXT(">> OK <<"));
		return;
#endif

		Uint32 x = X_START;

		// No NPC
		if ( m_active == -1 )
		{
			frame->AddDebugScreenText( x, 65, TXT("No NPC selected. Use arrows to navigate"), Color::GREEN );
			return;			
		}

		// Draw info background
		const Uint32 width = frame->GetFrameOverlayInfo().m_width;
		const Uint32 height = frame->GetFrameOverlayInfo().m_height;
		frame->AddDebugRect( 50, 50, width-100, height-120, Color( 0, 0, 0, 128 ) );
		frame->AddDebugFrame( 50, 50, width-100, height-120, Color::WHITE );

		// NPC name
		Uint32 y = m_verticalOffset + 65;

		String caption = String::Printf( TXT("NPC ( %i of %i )"), m_active+1, m_allNpcs.Size() );
		frame->AddDebugScreenText( x, y, caption, Color::WHITE );
		
		y += LINE_INCREMENT;

		// NPC is lost
		CNewNPC* npc = m_allNpcs[ m_active ].Get();
		if ( !npc )
		{
			frame->AddDebugScreenText( x, y, TXT("SIGNAL LOST"), Color::RED );
			return;
		}

		// Template name
		String templateName = npc->GetEntityTemplate()
			? npc->GetEntityTemplate()->GetFriendlyName()
			: TXT("Unknown");
		frame->AddDebugScreenText( x, y, String::Printf( TXT("Template: '%ls'"), templateName.AsChar() ), Color::WHITE );
		y += LINE_INCREMENT;

		//Behaviours 

		// Tree Machine
		CBehTreeMachine* behTM = npc->GetBehTreeMachine();

		frame->AddDebugScreenText( x, y, TXT("Information"), Color::YELLOW );
		y += LINE_INCREMENT;

		if ( behTM != NULL )
		{
			frame->AddDebugScreenText( x, y, String::Printf( TXT("Beh Machine Info: ") ) + behTM->GetInfo(), Color::WHITE );
			y += LINE_INCREMENT;
		}
		

		// Tree
		//const CBehTree* tree = behTM->GetTree();

		//if( tree )
		//{
		//	frame->AddDebugScreenText( x, y, String::Printf( TXT("Tree: ") ) + tree->GetFriendlyName(), Color::WHITE );
		//	y += LINE_INCREMENT;

		//	//frame->AddDebugScreenText( x, y, String::Printf( TXT("Tree Description: ") ) + tree->GetFriendlyDescription(), Color::WHITE );
		//	y += LINE_INCREMENT;

		//	// Nodes
		//	const IBehTreeNodeDefinition* rootNode = tree->GetRootNode();

		//	y = RecurseTreeNodes( rootNode, y, x, frame );
		//}
	}

	Uint32 RecurseTreeNodes( const IBehTreeNodeDefinition* node, Uint32 y, Uint32 x, CRenderFrame* frame = NULL )
	{
		if( frame )
		{
			frame->AddDebugScreenText( x, y, node->GetNodeName().AsString(), Color::WHITE );
		}
		else
		{
			Char indentBuf[16];
			Uint32 indent = Min( ( ( x - X_START ) / LINE_INCREMENT ), static_cast< Uint32 >( ARRAY_COUNT( indentBuf ) - 1 ) ) ;
			Red::System::MemorySet( indentBuf, '-', indent * sizeof( Char ) );
			indentBuf[indent] = '\0';
			
			LOG_GAME( TXT( "%s %s" ), indentBuf, node->GetNodeName().AsString().AsChar() );
		}

		y += LINE_INCREMENT;

		if( node->IsA< IBehTreeNodeCompositeDefinition >() )
		{
			x += NODE_INDENT;

			const IBehTreeNodeCompositeDefinition* compositeNode = static_cast<const IBehTreeNodeCompositeDefinition*>( node );

			Int32 numChildren = compositeNode->GetNumChildren();

			for(Int32 i = 0; i < numChildren; ++i)
			{
				y = RecurseTreeNodes( compositeNode->GetChild( i ), y, x, frame );
			}
		}

		return y;
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

void CreateDebugPageBehaviorTree()
{
	IDebugPage* page = new CDebugPageBehaviorTree();
	IDebugPageManagerBase::GetInstance()->RegisterDebugPage( page );
}

#endif
