/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "animationReporter.h"
#include "lazyWin32feedback.h"
#include "animationReporterPosesSaverLoader.h"
#include "checkListDlg.h"
#include "commonDlgs.h"

#include "../../common/game/jobTree.h"
#include "../../common/game/aiProfile.h"
#include "../../common/game/quest.h"
#include "../../common/game/questPhase.h"
#include "../../common/game/behTree.h"
#include "../../common/game/behTreeNode.h"
#include "../../common/game/attitude.h"
#include "../../common/game/reactionAction.h"
#include "../../common/game/questGraph.h"
#include "../../common/game/questGraphBlock.h"
#include "../../common/game/questScriptBlock.h"
#include "../../common/game/actorLatentAction.h"

#include "../../common/game/actionPointComponent.h"

#include "../../common/core/depot.h"
#include "../../common/core/variantArray.h"
#include "../../common/core/garbageCollector.h"
#include "../../common/core/scriptingSystem.h"
#include "../../common/core/feedback.h"
#include "../../common/engine/behaviorGraph.h"
#include "../../common/core/gatheredResource.h"
#include "../../common/engine/skeleton.h"

BEGIN_EVENT_TABLE( CEdAnimationReporterWindow, wxSmartLayoutPanel )
	EVT_BUTTON( XRCID("generateReport"), CEdAnimationReporterWindow::OnReloadReportFilePC )
	EVT_BUTTON( XRCID("generateReportX"), CEdAnimationReporterWindow::OnReloadReportFileXBox )
	EVT_BUTTON( XRCID("saveReport"), CEdAnimationReporterWindow::OnSaveReport )
	EVT_BUTTON( XRCID("loadReport"), CEdAnimationReporterWindow::OnLoadReport )
	EVT_BUTTON( XRCID("btnTemp"), CEdAnimationReporterWindow::OnTmp )
	EVT_MENU( XRCID("menuComprPosesExport"), CEdAnimationReporterWindow::OnExportCompressedPoses )
	EVT_MENU( XRCID("menuComprPosesImport"), CEdAnimationReporterWindow::OnImportCompressedPoses )
	EVT_MENU( XRCID("menuComprPosesDelete"), CEdAnimationReporterWindow::OnDeleteCompressedPoseFiles )
	EVT_MENU( XRCID("menuComprPosesRecreate"), CEdAnimationReporterWindow::OnRecreateCompressedPoseFiles )
	EVT_MENU( XRCID("menuSkeletonCompressionRecreate"), CEdAnimationReporterWindow::OnRecreateSkeletonCompressionFiles )
	EVT_MENU( XRCID("menuRemoveUnusedAnimation"), CEdAnimationReporterWindow::OnRemoveUnusedAnimation )
	EVT_MENU( XRCID("menuStreamingReport"), CEdAnimationReporterWindow::OnStreamingReport )
	EVT_HTML_LINK_CLICKED( XRCID("viewAnimsetHtml"), CEdAnimationReporterWindow::OnAnimsetViewLinkClicked )
	EVT_HTML_LINK_CLICKED( XRCID("viewBehaviorHtml"), CEdAnimationReporterWindow::OnBehaviorViewLinkClicked )
	EVT_HTML_LINK_CLICKED( XRCID("viewAnimationHtml"), CEdAnimationReporterWindow::OnAnimationViewLinkClicked )
	EVT_HTML_LINK_CLICKED( XRCID("viewApHtml"), CEdAnimationReporterWindow::OnApViewLinkClicked )
	EVT_HTML_LINK_CLICKED( XRCID("viewEntitiesHtml"), CEdAnimationReporterWindow::OnEntityViewLinkClicked )
	EVT_HTML_LINK_CLICKED( XRCID("viewExternalAnimsHtml"), CEdAnimationReporterWindow::OnExternalAnimsViewLinkClicked )
	EVT_HTML_LINK_CLICKED( XRCID("todoHtml"), CEdAnimationReporterWindow::OnTodoLinkClicked )
	EVT_BUTTON( XRCID("btnSoundGenerate"), CEdAnimationReporterWindow::OnGenerateSoundReport )
	EVT_HTML_LINK_CLICKED( XRCID("htmlSoundDepot"), CEdAnimationReporterWindow::OnSoundDepotLinkClicked )
	EVT_HTML_LINK_CLICKED( XRCID("htmlSoundUsed"), CEdAnimationReporterWindow::OnSoundDepotLinkClicked )
	EVT_HTML_LINK_CLICKED( XRCID("htmlSoundUnused"), CEdAnimationReporterWindow::OnSoundDepotLinkClicked )
	EVT_HTML_LINK_CLICKED( XRCID("htmlSoundTocheck"), CEdAnimationReporterWindow::OnSoundDepotLinkClicked )
	EVT_HTML_LINK_CLICKED( XRCID("reportHtmlWin"), CEdAnimationReporterWindow::OnReportWinLinkClicked )
	EVT_BUTTON( XRCID("btnDiff"), CEdAnimationReporterWindow::OnSoundDiffClicked )
END_EVENT_TABLE()

CGatheredResource resEntTemplate_1( TXT("characters\\templates\\witcher\\witcher.w2ent"), 0 );
CGatheredResource resEntTemplate_2( TXT("characters\\templates\\man\\man_base.w2ent"), 0 );
CGatheredResource resEntTemplate_3( TXT("characters\\templates\\woman\\woman_base.w2ent"), 0 );
CGatheredResource resEntTemplate_4( TXT("characters\\templates\\dwarf\\dwarf_base.w2ent"), 0 );
CGatheredResource resEntTemplate_5( TXT("characters\\templates\\child\\child_base.w2ent"), 0 );

CEdAnimationReporterWindow::CEdAnimationReporterWindow( wxWindow* parent )
	: wxSmartLayoutPanel( parent, TXT("AnimationsReport"), true )
	, m_animsetSort( AVS_AnimSize )
	, m_behaviorSort( BVS_Size )
	, m_animationSort( ANVS_Name )
	, m_entitiesSort( EVS_Name )
	, m_externalAnimSort( EAVS_Name )
	, m_todoSort( TDVS_Prio )
	, m_selectedAnimset( NULL )
	, m_apShowAllOwners( false )
	, m_acShowAllOwners( false )
	, m_soundShowDep( false )
{
	// Set icon
	wxIcon iconSmall;
	iconSmall.CopyFromBitmap( SEdResources::GetInstance().LoadBitmap( TXT( "IMG_SKULL" ) ) );
	SetIcon( iconSmall );

	CacheClasses();

	FillSoundDiffTab();

	RefreshReporterWindows();
}

CEdAnimationReporterWindow::~CEdAnimationReporterWindow()
{
	ClearAllRecords();
	UnloadPreloadedTemplates();
}

void CEdAnimationReporterWindow::CacheClasses()
{
	m_cachedClasses.Resize( CC_Last );
	
	m_cachedClasses[ CC_CReactionPlaySlotAnimation ] = SRTTI::GetInstance().FindClass( CNAME( CReactionPlaySlotAnimation ) );
	m_cachedClasses[ CC_CReactionMoveToWaypointPlayAnimations ] = SRTTI::GetInstance().FindClass( CNAME( CReactionMoveToWaypointPlayAnimations ) );
	m_cachedClasses[ CC_CPushedReaction ] = SRTTI::GetInstance().FindClass( CNAME( CPushedReaction ) );
	m_cachedClasses[ CC_CBTTaskPlaySlotAnimation ] = SRTTI::GetInstance().FindClass( CNAME( CBTTaskPlaySlotAnimation ) );
	m_cachedClasses[ CC_CActorLatentActionPlayAnimation ] = SRTTI::GetInstance().FindClass( CNAME( CActorLatentActionPlayAnimation ) );

	for ( Uint32 i=0; i<m_cachedClasses.Size(); ++i )
	{
		ASSERT( m_cachedClasses[ i ] );
	}
}

void CEdAnimationReporterWindow::LoadPreloadedTemplates()
{
	GFeedback->UpdateTaskInfo( TXT("Loading character base tamplates...") );

	{
		GFeedback->UpdateTaskProgress( 0, 5 );
		CEntityTemplate* templ = resEntTemplate_1.LoadAndGet< CEntityTemplate >();
		if ( templ ) m_preloadedTemplates.PushBack( templ );
		ASSERT( templ );
	}
	{
		GFeedback->UpdateTaskProgress( 1, 5 );
		CEntityTemplate* templ = resEntTemplate_2.LoadAndGet< CEntityTemplate >();
		if ( templ ) m_preloadedTemplates.PushBack( templ );
		ASSERT( templ );
	}
	{
		GFeedback->UpdateTaskProgress( 2, 5 );
		CEntityTemplate* templ = resEntTemplate_3.LoadAndGet< CEntityTemplate >();
		if ( templ ) m_preloadedTemplates.PushBack( templ );
		ASSERT( templ );
	}
	{
		GFeedback->UpdateTaskProgress( 3, 5 );
		CEntityTemplate* templ = resEntTemplate_4.LoadAndGet< CEntityTemplate >();
		if ( templ ) m_preloadedTemplates.PushBack( templ );
		ASSERT( templ );
	}
	{
		GFeedback->UpdateTaskProgress( 4, 5 );
		CEntityTemplate* templ = resEntTemplate_5.LoadAndGet< CEntityTemplate >();
		if ( templ ) m_preloadedTemplates.PushBack( templ );
		ASSERT( templ );
	}

	for ( Uint32 i=0; i<m_preloadedTemplates.Size(); ++i )
	{
		m_preloadedTemplates[i]->AddToRootSet();
	}
}

void CEdAnimationReporterWindow::UnloadPreloadedTemplates()
{
	for ( Uint32 i=0; i<m_preloadedTemplates.Size(); ++i )
	{
		m_preloadedTemplates[i]->RemoveFromRootSet();
	}

	m_preloadedTemplates.Clear();
}

//////////////////////////////////////////////////////////////////////////

void CEdAnimationReporterWindow::ReloadReportFile( const String& cookListFile )
{
	// Clear cook list
	m_cookList.ClearFast();

	// Clear 'to do' list
	m_todoList.Clear();

	// Load existing cook file list
	FILE* f = _wfopen( cookListFile.AsChar(), TXT("r") );
	if ( f )
	{
		GFeedback->BeginTask( TXT("Generate report..."), true );

		// Scan file
		while ( !feof(f) )
		{
			// Get line
			TCHAR line[ 1024 ];
			fgetws( line, ARRAYSIZE(line), f );

			// Strip line end
			size_t curLen = Red::System::StringLength( line );
			if ( curLen && line[curLen-1] == '\n' )
			{
				line[ curLen-1 ] = 0;
			}

			// Skip empty lines
			curLen = Red::System::StringLength( line );
			if ( curLen < 1 )
			{
				continue;
			}

			const String fileName = line;

			//ASSERT( !m_cookList.Exist( fileName ) ); // This is not true, but why?
			m_cookList.PushBackUnique( fileName );
		}

		// Close the file
		fclose( f );

		if ( m_cookList.Empty() )
		{
			String message = String::Printf( TXT("File '%s' is empty. You have to cook game first or copy a cook from network drive."), cookListFile.AsChar() );
			wxMessageBox( message.AsChar(), wxT("Animations reporter") );
		}

		ParseAllUsedResourceList();

		GFeedback->EndTask();
	}
	else
	{
		String message = String::Printf( TXT("File '%s' is not found. You have to cook game first or copy a cook from network drive."), cookListFile.AsChar() );
		wxMessageBox( message.AsChar(), wxT("Animations reporter") );
	}
}

void CEdAnimationReporterWindow::ClearAllRecords()
{
	m_todoList.Clear();

	m_acNodes.Clear();
	m_apNodes.Clear();

	m_animsetRecords.ClearPtr();
	m_animEventsRecords.ClearPtr();
	m_behaviorRecords.ClearPtr();
	m_jobRecords.ClearPtr();

	m_externalAnims.Clear();

	m_usedSoundEvents.ClearPtr();
	m_unusedSoundEvents.ClearPtr();
	m_depotSoundEvents.ClearPtr();
	m_tocheckSoundEvents.ClearPtr();
	m_allSoundEvents.ClearPtr();

	m_selectedAnimset = NULL;
}

void CEdAnimationReporterWindow::ParseAllUsedResourceList()
{
	// Clear old data
	ClearAllRecords();

	if ( m_cookList.Empty() )
	{
		return;
	}

	if ( m_preloadedTemplates.Size() == 0 )
	{
		LoadPreloadedTemplates();
	}

	// Parse data for animsets
	CollectRecords< CSkeletalAnimationSet, EdAnimReportAnimset >( TXT("Animsets"), m_animsetRecords );

	// Parse data for animsets
	//CollectRecords< CExtAnimEventsFile, EdAnimReportAnimEvents >( TXT("Animation events"), m_animEventsRecords );

	// Parse data for behaviors
	CollectRecords< CBehaviorGraph, EdAnimReportBehavior >( TXT("Behaviors"), m_behaviorRecords );

	// Parse data for job trees
	CollectRecords< CJobTree, EdAnimReportJobTree >( TXT("JobTrees"), m_jobRecords );	

	// Parse behavior trees
	ParseBehaviorTrees();

	// Parse quests
	ParseQuests();

	// Parse scripts
	ParseScripts();

	// Parse data for templates
	ParseEntityTemplates();
	
	// Add external anims to list
	AddExternalAnimsToList();

	// Find dependences
	FindDependences();

	GFeedback->UpdateTaskInfo( TXT("Calc diff with depot...") );

	// Refresh editor
	RefreshReporterWindows();
}

void CEdAnimationReporterWindow::FindDependences()
{
	GFeedback->UpdateTaskInfo( TXT("Parsing file... Finding dependences") );

	Uint32 size = m_acNodes.Size() + m_apNodes.Size() + m_animsetRecords.Size();
	Uint32 curr = 0;

	GFeedback->UpdateTaskProgress( curr, size );

	for ( Uint32 i=0; i<m_acNodes.Size(); ++i )
	{
		ACNode& node = m_acNodes[ i ];

		FillDependences( node, m_todoList );

		GFeedback->UpdateTaskProgress( ++curr, size );
	}

	for ( Uint32 i=0; i<m_apNodes.Size(); ++i )
	{
		APNode& node = m_apNodes[ i ];

		FillDependences( node, m_todoList );

		GFeedback->UpdateTaskProgress( ++curr, size );
	}

	for ( Uint32 i=0; i<m_animsetRecords.Size(); ++i )
	{
		EdAnimReportAnimset* as = m_animsetRecords[ i ];
		if ( as )
		{
			for ( Uint32 k=0; k<as->m_animations.Size(); ++k )
			{
				EdAnimReportAnimation* anim = as->m_animations[ k ];
				if ( anim->m_used > 0 )
				{
					as->m_animUsedNum++;
				}
				else
				{
					as->m_animUnusedNum++;
				}
			}

			ASSERT( as->m_animNum == as->m_animUsedNum + as->m_animUnusedNum );
		}

		GFeedback->UpdateTaskProgress( ++curr, size );
	}
}

EdAnimReportAnimset* CEdAnimationReporterWindow::AddAnimset( const String& path )
{
	CSkeletalAnimationSet* animset = LoadResource< CSkeletalAnimationSet >( path );
	if ( animset )
	{
		EdAnimReportAnimset* as = new EdAnimReportAnimset( animset, m_todoList );
		m_animsetRecords.PushBack( as );
		return as;
	}
	return NULL;
}

void CEdAnimationReporterWindow::AddExternalAnimFromProperty( CObject* obj, const CName& propName, const String& owner, const String& desc )
{
	IProperty* animProp = obj->GetClass()->FindProperty( propName );
	if ( animProp )
	{
		CVariant value;
		value.Init( animProp->GetType()->GetName(), NULL );
		animProp->Get( obj, value.GetData() );

		String temp;
		value.ToString( temp );

		SExternalAnim anim;
		anim.m_animation = temp;
		anim.m_owner = owner;
		anim.m_desc = desc;

		m_externalAnims.PushBack( anim );
	}
	else
	{
		ASSERT( animProp );
	}
}

void CEdAnimationReporterWindow::AddExternalAnimFromArrayProperty( CObject* obj, const CName& arrayPropName, const String& owner, const String& desc )
{
	IProperty* animProp = obj->GetClass()->FindProperty( arrayPropName );
	if ( animProp )
	{
		CVariant variant;
		variant.Init( animProp->GetType()->GetName(), NULL );
		animProp->Get( obj, variant.GetData() );

		if ( variant.IsArray() )
		{
			CVariantArray theArray( variant );

			const Uint32 size = theArray.Size();
			for ( Uint32 i=0; i<size; ++i )
			{
				CVariant elem;
				theArray.Get( i, elem );

				String temp;
				elem.ToString( temp );

				SExternalAnim anim;
				anim.m_animation = temp;
				anim.m_owner = owner;
				anim.m_desc = desc;

				m_externalAnims.PushBack( anim );
			}
		}
		else
		{
			ASSERT( variant.IsArray() );
		}
	}
	else
	{
		ASSERT( animProp );
	}
}

void CEdAnimationReporterWindow::AddExternalAnimsToList()
{
	GFeedback->UpdateTaskInfo( TXT("Parsing file... External animations") );

	TDynArray< String > anims;

	for ( Uint32 i=0; i<m_externalAnims.Size(); ++i )
	{
		if ( !m_externalAnims[ i ].m_animation.Empty() && m_externalAnims[ i ].m_animation != TXT("None") )
		{
			anims.PushBackUnique( m_externalAnims[ i ].m_animation );
		}
	}

	// Try to find anims
	for ( Uint32 j=0; j<anims.Size(); ++j )
	{
		GFeedback->UpdateTaskProgress( j, anims.Size() );

		const CName animName( anims[ j ] );

		for ( Uint32 i=0; i<m_animsetRecords.Size(); ++i )
		{
			EdAnimReportAnimset* as = m_animsetRecords[ i ];
			if ( as )
			{
				for ( Uint32 k=0; k<as->m_animations.Size(); ++k )
				{
					EdAnimReportAnimation* anim = as->m_animations[ k ];
					if ( anim->m_name == animName )
					{
						anim->m_used++;
					}
				}
			}
		}
	}
}

void CEdAnimationReporterWindow::ParseQuests()
{
	GFeedback->UpdateTaskInfo( TXT("Parsing file... Quests") );

	static const CName funcName1( TXT("QPlaySlotAnimation") );
	static const CName funcName2( TXT("QPlayMimicAnimation") );

	String questFileExtension = CQuest::GetFileExtension();
	String phaseFileExtension = CQuestPhase::GetFileExtension();

	CClass* questSAClass = m_cachedClasses[ CC_CActorLatentActionPlayAnimation ];

	Uint32 size = m_cookList.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		GFeedback->UpdateTaskProgress( i, size );

		const String& cookedFile = m_cookList[ i ];

		CQuestPhase* phase = NULL;

		if ( cookedFile.EndsWith( questFileExtension ) )
		{
			phase = LoadResource< CQuest >( cookedFile );
		}
		else if ( cookedFile.EndsWith( phaseFileExtension ) )
		{
			phase = LoadResource< CQuestPhase >( cookedFile );
		}

		if ( phase )
		{
			CQuestGraph* graph = phase->GetGraph();
			if ( graph )
			{
				const TDynArray< CGraphBlock* >& blocks = graph->GraphGetBlocks();

				const Uint32 size = blocks.Size();
				for ( Uint32 i=0; i<size; ++i )
				{
					CGraphBlock* b = blocks[ i ];
					if ( b && b->IsA< CQuestScriptBlock >() )
					{
						CQuestScriptBlock* sb = static_cast< CQuestScriptBlock* >( b );
						const CName& func = sb->GetFunctionName();

						if ( funcName1 == func || funcName2 == func )
						{
							CVariant val;

							VERIFY( sb->ReadDynamicPropForEditor( CNAME( animation ), val ) );
							String temp;
							val.ToString( temp );

							SExternalAnim anim;
							anim.m_animation = temp;
							anim.m_desc = String::Printf( TXT("Resource '%s'"), phase->GetDepotPath().AsChar() );

							if ( funcName1 == func  )
							{
								anim.m_owner = TXT("CQuestGraph - QPlaySlotAnimation");
							}
							else
							{
								anim.m_owner = TXT("CQuestGraph - QPlayMimicAnimation");
							}

							m_externalAnims.PushBack( anim );
						}
					}
				}
			}
		}

		if ( GFeedback->IsTaskCanceled() )
		{
			return;
		}
	}

	SGarbageCollector::GetInstance().CollectNow();
}

void CEdAnimationReporterWindow::ParseBehaviorTrees()
{
	GFeedback->UpdateTaskInfo( TXT("Parsing file... Behavior Trees") );

	if ( m_cachedClasses[ CC_CBTTaskPlaySlotAnimation ] == NULL )
	{
		return;
	}

	String fileExtension = CBehTree::GetFileExtension();

	Uint32 size = m_cookList.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		GFeedback->UpdateTaskProgress( i, size );

		const String& cookedFile = m_cookList[ i ];

		if ( cookedFile.EndsWith( fileExtension ) )
		{
			CBehTree* tree = LoadResource< CBehTree >( cookedFile );
			if ( tree )
			{
				TDynArray< IBehTreeNodeDefinition* > nodes;

				const IBehTreeNodeDefinition*	root = tree->GetRootNode();
				if ( root )
				{
					root->CollectNodes( nodes );
				}

				CClass* c = m_cachedClasses[ CC_CBTTaskPlaySlotAnimation ];

				for ( Uint32 i=0; i<nodes.Size(); ++i )
				{
					IBehTreeNodeDefinition* node = nodes[ i ];
					if ( node && node->GetClass() == c )
					{
						String path = String::Printf( TXT("Resource '%s'"), tree->GetDepotPath().AsChar() );
						AddExternalAnimFromProperty( node, CNAME( animation ), TXT("Behavior Trees - CBTTaskPlaySlotAnimation"), path );
					}
				}
			}
		}

		if ( GFeedback->IsTaskCanceled() )
		{
			return;
		}
	}

	SGarbageCollector::GetInstance().CollectNow();
}

void CEdAnimationReporterWindow::ParseScripts()
{
	GFeedback->UpdateTaskInfo( TXT("Parsing file... Scripts") );

	const String& path = GScriptingSystem->GetRootPath();

	TDynArray< String > scripts;
	GFileManager->FindFiles( path, TXT("*.ws"), scripts, true );

	const Uint32 size = scripts.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		GFeedback->UpdateTaskProgress( i, size );

		const String& scriptPath = scripts[ i ];

		CollectAnimationsFromScript( scriptPath );
	}
}

void CEdAnimationReporterWindow::ParseEntityTemplates()
{
	ASSERT( !m_cookList.Empty() );

	GFeedback->UpdateTaskInfo( TXT("Parsing file... Entities") );

	Uint32 loadedTempls = 0;
	static const Uint32 maxLoadedTempls = 75;

	String fileExtension = CEntityTemplate::GetFileExtension();

	Uint32 size = m_cookList.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		GFeedback->UpdateTaskProgress( i, size );

		const String& cookedFile = m_cookList[ i ];

		if ( cookedFile.EndsWith( fileExtension ) )
		{
			CEntityTemplate* templ = LoadResource< CEntityTemplate >( cookedFile );
			if ( templ )
			{
				ParseTemplParams( templ );

				const CEntity* ent = templ->GetEntityObject();
				if ( ent )
				{
					const TDynArray< CComponent* >& comps = ent->GetComponents();
					for ( Uint32 i=0; i<comps.Size(); ++i )
					{
						CComponent* c = comps[ i ];

						if ( c->IsA< CAnimatedComponent >() )
						{
							CAnimatedComponent* ac = static_cast< CAnimatedComponent* >( c );	
							
							ACNode& node = GetOrCreateACNodeByComp( ac, m_todoList );
							
							node.m_owners.PushBackUnique( cookedFile );
						}
						else if ( c->IsA< CActionPointComponent >() )
						{
							CActionPointComponent* ap = static_cast< CActionPointComponent* >( c );	

							APNode& node = GetOrCreateAPNodeByComp( ap, m_todoList );

							node.m_owners.PushBackUnique( cookedFile );
						}
					}
				}
				
				loadedTempls++;
			}

			if ( loadedTempls > maxLoadedTempls )
			{
				loadedTempls = 0;
				SGarbageCollector::GetInstance().CollectNow();
			}
		}

		if ( GFeedback->IsTaskCanceled() )
		{
			return;
		}
	}
}

void CEdAnimationReporterWindow::ParseTemplParams( const CEntityTemplate* templ )
{
	static CName animationsAtStart( TXT("animationsAtStart") );
	static CName animationsAtEnd( TXT("animationsAtEnd") );

	CAIProfile* profile = templ->FindParameter< CAIProfile >();
	if ( profile )
	{
		const TDynArray< THandle< CAIReaction > >& reaction = profile->GetReactions();
		for ( Uint32 i=0; i<reaction.Size(); ++i )
		{
			CAIReaction* r = reaction[ i ].Get();
			if ( r )
			{
				if ( r->m_action )
				{
					if ( m_cachedClasses[ CC_CReactionPlaySlotAnimation ] && r->m_action->GetClass() == m_cachedClasses[ CC_CReactionPlaySlotAnimation ] )
					{
						String path = String::Printf( TXT("Resource '%s'"), templ->GetDepotPath().AsChar() );
						AddExternalAnimFromProperty( r->m_action, CNAME( animation ), TXT("AI Reaction - CReactionPlaySlotAnimation"), path );
					}
					else if ( m_cachedClasses[ CC_CReactionMoveToWaypointPlayAnimations ] && r->m_action->GetClass() == m_cachedClasses[ CC_CReactionMoveToWaypointPlayAnimations ] )
					{
						String path = String::Printf( TXT("Resource '%s'"), templ->GetDepotPath().AsChar() );
						AddExternalAnimFromArrayProperty( r->m_action, animationsAtStart, TXT("AI Reaction - CReactionPlaySlotAnimation"), path );
						AddExternalAnimFromArrayProperty( r->m_action, animationsAtEnd, TXT("AI Reaction - CReactionPlaySlotAnimation"), path );
					}
					if ( m_cachedClasses[ CC_CPushedReaction ] && r->m_action->GetClass() == m_cachedClasses[ CC_CPushedReaction ] )
					{
						String path = String::Printf( TXT("Resource '%s'"), templ->GetDepotPath().AsChar() );
						AddExternalAnimFromProperty( r->m_action, CNAME( animation ), TXT("AI Reaction - CPushedReaction"), path );
					}
				}
			}
		}
	}
}

wxString CEdAnimationReporterWindow::MemSizeToText( Uint32 memSize )
{
	if ( memSize == 0 )
	{
		return wxT("None");
	}
	else if ( memSize < 1024 )
	{
		return wxT("&lt 1KB");
	}
	else if ( memSize < 1024*1024 )
	{
		return wxString::Format( wxT("%1.1f KB"), memSize / 1024.0f );
	}
	else
	{
		return wxString::Format( wxT("%1.2f MB"), memSize / (1024.0f*1024.0f) );
	}
}

void CEdAnimationReporterWindow::CalcAnimMems( Uint32& total, Uint32& used, Uint32& unused ) const
{
	total = 0;
	used = 0;
	unused = 0;

	for ( Uint32 i=0; i<m_animsetRecords.Size(); ++i )
	{
		const EdAnimReportAnimset* animset = m_animsetRecords[ i ];

		for ( Uint32 j=0; j<animset->m_animations.Size(); ++j )
		{
			const EdAnimReportAnimation* anim = animset->m_animations[ j ];

			total += anim->m_animSize;

			if ( anim->m_used > 0 )
			{
				used += anim->m_animSize;
			}
			else
			{
				unused += anim->m_animSize;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////

void CEdAnimationReporterWindow::RefreshReporterWindows()
{
	RefreshViewAnimsets();
	RefreshViewBehaviors();
	RefreshViewAnimations();
	RefreshViewEntities();
	RefreshViewAp();
	RefreshTodoList();
	RefreshReportLog();
	RefreshDiffPages();
	RefreshExternalAnimPage();
}

void CEdAnimationReporterWindow::ShowAndRefreshView( EReporterView view )
{
	ShowView( view );

	switch ( view )
	{
	case RV_AnimsetData:
		RefreshViewAnimations();
		break;
	case RV_AnimsetList:
		RefreshViewAnimsets();
		break;
	case RV_BehaviorList:
		RefreshViewBehaviors();
		break;
	}
}

void CEdAnimationReporterWindow::ShowView( EReporterView view )
{
	const unsigned short page = view;

	wxNotebook* editorsNotebook = XRCCTRL( *this, "viewNotebook", wxNotebook );
	editorsNotebook->ChangeSelection( page );
}

void CEdAnimationReporterWindow::SelectAnimset( Int32 num )
{
	m_selectedAnimset = num >= 0 && m_animsetRecords.Size() > (Uint32)num ? m_animsetRecords[ num ] : NULL;
}

void CEdAnimationReporterWindow::SelectEntity( Int32 num )
{
	//m_selectedEntity = num >= 0 && m_entityTemplateRecords.Size() > (Uint32)num ? m_entityTemplateRecords[ num ] : NULL;
}

void CEdAnimationReporterWindow::RefreshReportLog()
{
	wxString html;

	html += wxString::Format( wxT("All files: %d<br>"), m_cookList.Size() );
	html += wxString::Format( wxT("Animsets: %d<br>"), m_animsetRecords.Size() );
	html += wxString::Format( wxT("Behaviors: %d<br>"), m_behaviorRecords.Size() );
	html += wxString::Format( wxT("JobTrees: %d<br>"), m_jobRecords.Size() );

	html += wxT("<br>");

	html += wxString::Format( wxT("Animated component(s): %d<br>"), m_acNodes.Size() );
	html += wxString::Format( wxT("Action point component(s): %d<br>"), m_apNodes.Size() );

	html += wxT("<br>");

	html += wxString::Format( wxT("Error(s): %d<br>"), m_todoList.GetErrorsNum() );
	html += wxString::Format( wxT("Warn(s): %d<br>"), m_todoList.GetWarnsNum() );
	//html += wxString::Format( wxT("Check(s): %d<br>"), m_todoList.GetChecksNum() );
	html += wxString::Format( wxT("Info(s): %d<br>"), m_todoList.GetInfosNum() );

	html += wxT("<br>");

	Uint32 totalAnimMem = 0;
	Uint32 usedAnimMem = 0;
	Uint32 unusedAnimMem = 0;

	CalcAnimMems( totalAnimMem, usedAnimMem, unusedAnimMem );

	html += wxString::Format( wxT("Anim total mem: %s<br>"), MemSizeToText( totalAnimMem ) );
	html += wxString::Format( wxT("Used: %s<br>"), MemSizeToText( usedAnimMem ) );
	html += wxString::Format( wxT("Wasted: %s<br>"), MemSizeToText( unusedAnimMem ) );	

	html += wxT("<br>");
	html += wxT("Do not remove animation(s):   (<i><a href=\"doNotRemoveAnimList\">edit</a></i>)<br>");

	if ( m_doNotRemoveAnimationList.Size() > 0 )
	{
		for ( Uint32 i=0; i<m_doNotRemoveAnimationList.Size(); ++i )
		{
			html += wxString::Format( wxT("   %d.%s<br>"), i, m_doNotRemoveAnimationList[ i ].AsString().AsChar() );
		}
	}
	else
	{
		html += wxT("empty list<br>");
	}

	html += wxT("");

	html += wxT("<br>");
	html += wxT("<br>");

	wxHtmlWindow* htmlWindow = XRCCTRL( *this, "reportHtmlWin", wxHtmlWindow );
	htmlWindow->SetPage( html );
}

void CEdAnimationReporterWindow::RefreshDiffPages()
{
	TDynArray< String > depotFilesAnimsets;
	TDynArray< String > depotFilesBehaviors;
	TDynArray< String > depotFilesJobs;

	String depotPath;
	GDepot->GetAbsolutePath( depotPath );

	String animsetEx = TXT("*."); 
	animsetEx += CSkeletalAnimationSet::GetFileExtension();

	String behaviorsEx = TXT("*."); 
	behaviorsEx += CBehaviorGraph::GetFileExtension();

	String jobsEx = TXT("*."); 
	jobsEx += CJobTree::GetFileExtension();

	if ( m_animsetRecords.Size() != 0 || m_behaviorRecords.Size() != 0 || m_jobRecords.Size() != 0 )
	{
		GFileManager->FindFiles( depotPath, animsetEx, depotFilesAnimsets, true );
		GFileManager->FindFiles( depotPath, behaviorsEx, depotFilesBehaviors, true );
		GFileManager->FindFiles( depotPath, jobsEx, depotFilesJobs, true );
	}	

	{
		wxString html;
		CreateDiffHtmlCode< CSkeletalAnimationSet, EdAnimReportAnimset >( depotFilesAnimsets, m_animsetRecords, wxT("Animsets"), html );
		wxHtmlWindow* htmlWindow = XRCCTRL( *this, "htmlDiffAnimset", wxHtmlWindow );
		htmlWindow->SetPage( html );
	}

	{
		wxString html;
		CreateDiffHtmlCode< CBehaviorGraph, EdAnimReportBehavior >( depotFilesBehaviors, m_behaviorRecords, wxT("Behaviors"), html );
		wxHtmlWindow* htmlWindow = XRCCTRL( *this, "htmlDiffBehavior", wxHtmlWindow );
		htmlWindow->SetPage( html );
	}

	{
		wxString html;
		CreateDiffHtmlCode< CJobTree, EdAnimReportJobTree >( depotFilesJobs, m_jobRecords, wxT("Job Trees"), html );
		wxHtmlWindow* htmlWindow = XRCCTRL( *this, "htmlDiffJobTree", wxHtmlWindow );
		htmlWindow->SetPage( html );
	}
}

//////////////////////////////////////////////////////////////////////////

void CEdAnimationReporterWindow::OnReloadReportFilePC( wxCommandEvent& event )
{
	String cookListFile;
	GDepot->GetAbsolutePath( cookListFile );

	cookListFile = cookListFile.StringBefore( TXT("data\\"), true );
	cookListFile += TXT( "CookedPC\\cookAll.list" );

	ReloadReportFile( cookListFile );
}

void CEdAnimationReporterWindow::OnReloadReportFileXBox( wxCommandEvent& event )
{
	String cookListFile;
	GDepot->GetAbsolutePath( cookListFile );

	cookListFile = cookListFile.StringBefore( TXT("data\\"), true );
	cookListFile += TXT( "CookedXBox\\cookAll.list" );

	ReloadReportFile( cookListFile );
}

void CEdAnimationReporterWindow::OnSaveReport( wxCommandEvent& event )
{
	Save();
}

void CEdAnimationReporterWindow::OnLoadReport( wxCommandEvent& event )
{
	Load();
}

void CEdAnimationReporterWindow::OnTmp( wxCommandEvent& event )
{
	
}

void CEdAnimationReporterWindow::OnExportCompressedPoses( wxCommandEvent& event )
{
	ExportCompressedPoses();
}

void CEdAnimationReporterWindow::OnImportCompressedPoses( wxCommandEvent& event )
{
	ImportCompressedPoses();
}

void CEdAnimationReporterWindow::OnGenerateSoundReport( wxCommandEvent& event )
{
	GenerateSoundReport();
}

void CEdAnimationReporterWindow::OnDeleteCompressedPoseFiles( wxCommandEvent& event )
{
	DeleteCompressedPoseFiles();
}

void CEdAnimationReporterWindow::OnRemoveUnusedAnimation( wxCommandEvent& event )
{
	RemoveUnusedAnimations();
}

void CEdAnimationReporterWindow::OnRecreateCompressedPoseFiles( wxCommandEvent& event )
{
	RecreateCompressedPoses();
}

void CEdAnimationReporterWindow::OnRecreateSkeletonCompressionFiles( wxCommandEvent& event )
{
	RecreateSkeletonCompression();
}

void CEdAnimationReporterWindow::OnReportWinLinkClicked( wxHtmlLinkEvent& event )
{
	const wxHtmlLinkInfo& linkInfo = event.GetLinkInfo();
	wxString href = linkInfo.GetHref();

	if ( href == wxT("doNotRemoveAnimList") )
	{
		if ( YesNo( TXT("Do you want to clear list?") ) )
		{
			if ( YesNo( TXT("Are you sure?") ) )
			{
				m_doNotRemoveAnimationList.Clear();
			}
		}

		if ( YesNo( TXT("Do you want to add new records to list?") ) )
		{
			while ( 1 )
			{
				String val;
				if ( InputBox( NULL, TXT(""), TXT("Animation name"), val ) && !val.Empty() )
				{
					CName nameVal( val );
					m_doNotRemoveAnimationList.PushBackUnique( nameVal );
				}

				if ( !YesNo( TXT("Continue?") ) )
				{
					break;
				}
			}
		}
	}

	RefreshReportLog();
}

void CEdAnimationReporterWindow::ExportCompressedPoses()
{
	if ( !YesNo( TXT("Do you want to export compressed poses?") ) )
	{
		return;
	}

	GFeedback->BeginTask( TXT("Export compressed poses"), true );
	GFeedback->UpdateTaskInfo( TXT("Collecting animsets...") );

	TDynArray< String > depotFilesAnimsets;

	String depotPath;
	GDepot->GetAbsolutePath( depotPath );

	String animsetEx = TXT("*."); 
	animsetEx += CSkeletalAnimationSet::GetFileExtension();

	GFileManager->FindFiles( depotPath, animsetEx, depotFilesAnimsets, true );

	CLazyWin32Feedback lazyFeedback;

	const Uint32 size = depotFilesAnimsets.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		const String& filePath = depotFilesAnimsets[ i ];

		if ( GFeedback->IsTaskCanceled() )
		{
			break;
		}

		GFeedback->UpdateTaskInfo( TXT("Animset %s"), filePath.AsChar() );
		GFeedback->UpdateTaskProgress( i, size );
		
		String localPath;
		GDepot->ConvertToLocalPath( filePath, localPath );

		CSkeletalAnimationSet* set = LoadResource< CSkeletalAnimationSet >( localPath );
		if ( set )
		{
			CSkeletalAnimationSet::SCompressedPosesData data;
			set->ExportCompressedPoses( data );
			
			if ( !data.IsEmpty() )
			{
				if ( !CEdCompressedPosesSaver::Save( filePath, data ) )
				{
					lazyFeedback.ShowWarn( TXT("Couldn't save compressed poses for animset '%s'"), filePath.AsChar() );
				}
			}
		}
		else
		{
			lazyFeedback.ShowWarn( TXT("Couldn't load resource '%s'"), filePath.AsChar() );
		}
	}

	GFeedback->EndTask();

	lazyFeedback.ShowAll();
}

void CEdAnimationReporterWindow::ImportCompressedPoses()
{
	if ( !YesNo( TXT("Do you want to import compressed poses?") ) )
	{
		return;
	}

	GFeedback->BeginTask( TXT("Import compressed poses"), true );
	GFeedback->UpdateTaskInfo( TXT("Collecting animsets...") );

	TDynArray< String > depotFilesAnimsets;

	String depotPath;
	GDepot->GetAbsolutePath( depotPath );

	String animsetEx = TXT("*."); 
	animsetEx += CSkeletalAnimationSet::GetFileExtension();

	GFileManager->FindFiles( depotPath, animsetEx, depotFilesAnimsets, true );

	CLazyWin32Feedback lazyFeedback;

	const Uint32 size = depotFilesAnimsets.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		const String& filePath = depotFilesAnimsets[ i ];

		if ( GFeedback->IsTaskCanceled() )
		{
			break;
		}

		GFeedback->UpdateTaskInfo( TXT("Animset %s"), filePath.AsChar() );
		GFeedback->UpdateTaskProgress( i, size );

		String localPath;
		GDepot->ConvertToLocalPath( filePath, localPath );

		CSkeletalAnimationSet::SCompressedPosesData data;

		if ( !CEdCompressedPosesLoader::Load( filePath, data ) )
		{
			lazyFeedback.ShowWarn( TXT("Couldn't load compressed poses for animset '%s'"), filePath.AsChar() );
		}
		else
		{
			if ( data.IsEmpty() )
			{
				continue;
			}

			CSkeletalAnimationSet* set = LoadResource< CSkeletalAnimationSet >( localPath );
			if ( set )
			{
				CDiskFile* diskFile = set->GetFile();
				if ( diskFile )
				{
					if ( diskFile->SilentCheckOut() )
					{
						VERIFY( set->MarkModified() );

						set->ImportCompressedPoses( data, &lazyFeedback );

						if ( !set->Save() )
						{
							lazyFeedback.ShowWarn( TXT("Couldn't save file '%s'"), filePath.AsChar() );
						}
					}
					else
					{
						lazyFeedback.ShowWarn( TXT("Couldn't checkout file '%s'"), filePath.AsChar() );
					}
				}
				else
				{
					lazyFeedback.ShowWarn( TXT("Couldn't find file '%s'"), filePath.AsChar() );
				}
			}
			else
			{
				lazyFeedback.ShowWarn( TXT("Couldn't load resource '%s'"), filePath.AsChar() );
			}
		}
	}

	GFeedback->EndTask();

	lazyFeedback.ShowAll();
}

void CEdAnimationReporterWindow::DeleteCompressedPoseFiles()
{
	TDynArray< String > files;

	String depotPath;
	GDepot->GetAbsolutePath( depotPath );

	String fileEx = TXT("*.cposes"); 

	GFileManager->FindFiles( depotPath, fileEx, files, true );

	CLazyWin32Feedback feedback;

	for ( Uint32 i=0; i<files.Size(); ++i )
	{
		const String& filePath = files[ i ];

		if ( !GFileManager->DeleteFile( filePath ) )
		{
			feedback.ShowError( TXT("Couldn't delete file '%s'"), filePath.AsChar() );
		}
	}

	feedback.ShowAll();
}

void CEdAnimationReporterWindow::RecreateCompressedPoses()
{
	if ( !YesNo( TXT("Do you want to recreate compressed poses?") ) )
	{
		return;
	}

	GFeedback->BeginTask( TXT("Recreate compressed poses"), true );
	GFeedback->UpdateTaskInfo( TXT("Collecting animsets...") );

	TDynArray< String > depotFilesAnimsets;

	String depotPath;
	GDepot->GetAbsolutePath( depotPath );

	String animsetEx = TXT("*."); 
	animsetEx += CSkeletalAnimationSet::GetFileExtension();

	GFileManager->FindFiles( depotPath, animsetEx, depotFilesAnimsets, true );

	CLazyWin32Feedback lazyFeedback;

	const Uint32 size = depotFilesAnimsets.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		const String& filePath = depotFilesAnimsets[ i ];

		if ( GFeedback->IsTaskCanceled() )
		{
			break;
		}

		GFeedback->UpdateTaskInfo( TXT("Animset %s"), filePath.AsChar() );
		GFeedback->UpdateTaskProgress( i, size );

		String localPath;
		GDepot->ConvertToLocalPath( filePath, localPath );

		CSkeletalAnimationSet* set = LoadResource< CSkeletalAnimationSet >( localPath );
		if ( set )
		{
			CDiskFile* diskFile = set->GetFile();
			if ( diskFile )
			{
				if ( diskFile->SilentCheckOut() )
				{
					VERIFY( set->MarkModified() );

					if ( !set->RecreateAllCompressedPoses( &lazyFeedback ) )
					{
						lazyFeedback.ShowWarn( TXT("Couldn't recreate poses for file '%s'"), filePath.AsChar() );
					}
					else
					{
						if ( !set->Save() )
						{
							lazyFeedback.ShowWarn( TXT("Couldn't save file '%s'"), filePath.AsChar() );
						}
					}
				}
				else
				{
					lazyFeedback.ShowWarn( TXT("Couldn't checkout file '%s'"), filePath.AsChar() );
				}
			}
			else
			{
				lazyFeedback.ShowWarn( TXT("Couldn't find file '%s'"), filePath.AsChar() );
			}
		}
		else
		{
			lazyFeedback.ShowWarn( TXT("Couldn't load resource '%s'"), filePath.AsChar() );
		}
	}

	GFeedback->EndTask();

	lazyFeedback.ShowAll();
}

void CEdAnimationReporterWindow::RecreateSkeletonCompression()
{
	if ( !YesNo( TXT("Do you want to recreate skeleton compressions?") ) )
	{
		return;
	}

	GFeedback->BeginTask( TXT("Recreate skeleton compressions"), true );
	GFeedback->UpdateTaskInfo( TXT("Collecting skeletons...") );

	TDynArray< String > depotFilesSkeletons;

	String depotPath;
	GDepot->GetAbsolutePath( depotPath );

	String skeletonEx = TXT("*."); 
	skeletonEx += CSkeleton::GetFileExtension();

	GFileManager->FindFiles( depotPath, skeletonEx, depotFilesSkeletons, true );

	CLazyWin32Feedback lazyFeedback;

	const Uint32 size = depotFilesSkeletons.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		const String& filePath = depotFilesSkeletons[ i ];

		if ( GFeedback->IsTaskCanceled() )
		{
			break;
		}

		GFeedback->UpdateTaskInfo( TXT("Skeleton %s"), filePath.AsChar() );
		GFeedback->UpdateTaskProgress( i, size );

		String localPath;
		GDepot->ConvertToLocalPath( filePath, localPath );

		CSkeleton* skeleton = LoadResource< CSkeleton >( localPath );
		if ( skeleton )
		{
			CDiskFile* diskFile = skeleton->GetFile();
			if ( diskFile )
			{
				if ( diskFile->SilentCheckOut() )
				{
					VERIFY( skeleton->MarkModified() );
					skeleton->RecreatePoseCompression();

					if ( !skeleton->Save() )
					{
						lazyFeedback.ShowWarn( TXT("Couldn't save file '%s'"), filePath.AsChar() );
					}
				}
				else
				{
					lazyFeedback.ShowWarn( TXT("Couldn't checkout file '%s'"), filePath.AsChar() );
				}
			}
			else
			{
				lazyFeedback.ShowWarn( TXT("Couldn't find file '%s'"), filePath.AsChar() );
			}
		}
		else
		{
			lazyFeedback.ShowWarn( TXT("Couldn't load resource '%s'"), filePath.AsChar() );
		}
	}

	GFeedback->EndTask();

	lazyFeedback.ShowAll();
}

namespace AnimationReporterHelpers
{
	Bool IsAnimationInExternalAnimations( const EdAnimReportAnimation* animation, const TDynArray< CEdAnimationReporterWindow::SExternalAnim >& externalAnims )
	{
		for ( Uint32 i = 0; i < externalAnims.Size(); ++i )
		{
			// compare names
			if ( Red::System::StringCompare( externalAnims[i].m_animation.AsChar(), animation->m_name.AsString().AsChar() ) == 0 )
			{
				return true;
			}
		}
		return false;
	}
}

void CEdAnimationReporterWindow::RemoveUnusedAnimations()
{
	if ( m_animsetRecords.Empty() )
	{
		wxMessageBox( wxT("There are no animsets to analyze, try making or loading the report first...") );
		return;
	}

	if ( m_externalAnims.Empty() )
	{
		if ( !YesNo( TXT("There are no external animsets. Are you sure you want to continue?") ) )
		{
			return;
		}
	}

	m_isUnusedAnimationRemovalTaskCancelled = false;
	GFeedback->BeginTask( TXT("Removing unused animations."), true );
	
	for ( Uint32 i = 0; i < m_animsetRecords.Size(); ++i )
	{
		EdAnimReportAnimset* animset = m_animsetRecords[ i ];

		if ( animset && animset->GetResource() )
		{
			GFeedback->UpdateTaskInfo( TXT( "Processing '%s' [animset %d of %d]" ), animset->m_name.AsChar(), i + 1, m_animsetRecords.Size() );
			GFeedback->UpdateTaskProgress( 0, animset->m_animations.Size() + 1 );
			
			RemoveUnusedAnimationFromAnimset( animset );
		}

		if ( m_isUnusedAnimationRemovalTaskCancelled )
		{
			break;
		}
	}

	GFeedback->EndTask();
}

void CEdAnimationReporterWindow::RemoveUnusedAnimationFromAnimset( EdAnimReportAnimset* animset )
{
	CSkeletalAnimationSet* skeletalAnimationSet = const_cast< CSkeletalAnimationSet* >( animset->GetResource() );

	TDynArray< CSkeletalAnimationSetEntry* > animsToCheck;

	// Collect animations to remove
	for (Uint32 j = 0; j < animset->m_animations.Size(); ++j)
	{
		if ( GFeedback->IsTaskCanceled() )
		{
			m_isUnusedAnimationRemovalTaskCancelled = true;
			return;
		}
				
		EdAnimReportAnimation* animation = animset->m_animations[ j ];

		if ( animation && animation->m_animation )
		{
			if ( !animation->m_used )
			{
				if ( AnimationReporterHelpers::IsAnimationInExternalAnimations( animation, m_externalAnims ) )
				{
					LOG_EDITOR( TXT( "Animation '%s' from animset '%s' was found in External Animations set. It won't be deleted." ), animation->m_name.AsString().AsChar(), animset->m_name.AsChar() );
					continue;
				}

				if ( m_doNotRemoveAnimationList.Exist( animation->m_name ) )
				{
					LOG_EDITOR( TXT( "Skip removing animation '%s' form animset '%s'"), animation->m_name.AsString().AsChar(), animset->m_name.AsChar() );
					continue;;
				}

				LOG_EDITOR( TXT( "Animation: '%s' in animset: '%s' is not used and will be marked for delete." ), animation->m_name.AsString().AsChar(), animset->m_name.AsChar() );

				CSkeletalAnimationSetEntry* animationSetEntry = const_cast< CSkeletalAnimationSetEntry* >( animation->m_animation );
				animsToCheck.PushBack( animationSetEntry );
			}
		}
	}

	// Ask user
	if ( animsToCheck.Size() > 0 )
	{
		TDynArray< CSkeletalAnimationSetEntry* > animsToRemove;
		animsToRemove.Reserve( animsToCheck.Size() );

		TDynArray< String > options;
		TDynArray< Bool > optionsState;

		options.Reserve( 24 );
		optionsState.Reserve( 24 );

		for ( Uint32 i=0; i<animsToCheck.Size(); ++i )
		{
			CSkeletalAnimationSetEntry* animation = animsToCheck[ i ];

			options.PushBack( animation->GetName().AsString() );
			optionsState.PushBack( true );
		}

		{
			CEdCheckListDialog* dlg = new CEdCheckListDialog( NULL, String::Printf( TXT("Animation(s) to remove - animset '%s'"), animset->m_name.AsChar() ), options, optionsState );
			dlg->ShowModal();

			for ( Uint32 j=0; j<optionsState.Size(); ++j )
			{
				if ( optionsState[ j ] )
				{
					ASSERT( animsToCheck[ j ]->GetName().AsString() == options[ j ] );
					animsToRemove.PushBack( animsToCheck[ j ] );
				}
			}
		}

		if ( animsToRemove.Size() == animset->m_animations.Size() )
		{
			// Remove all animations
			if ( YesNo( TXT("All animations are marked to remove. Do you want to delete empty animset?") ) )
			{
				Bool ret = skeletalAnimationSet->GetFile()->Delete( false, false );
				if ( !ret )
				{
					String str = String::Printf( TXT( "Error deleting animset '%s'" ), animset->m_name.AsChar() );
					wxMessageBox( str.AsChar(), wxT("Error") );
					WARN_EDITOR( TXT("%s"), str.AsChar() );
				}
				return;
			}
		}

		if ( animsToRemove.Size() > 0 )
		{
			if ( !skeletalAnimationSet->MarkModified() )
			{
				String str = String::Printf( TXT( "Error checking out animset '%s'" ), animset->m_name.AsChar() );
				wxMessageBox( str.AsChar(), wxT("Error") );
				WARN_EDITOR( TXT("%s"), str.AsChar() );
				return;
			}

			for ( Uint32 i=0; i<animsToRemove.Size(); ++i )
			{
				Bool ret = skeletalAnimationSet->RemoveAnimation( animsToRemove[ i ] );
				if ( !ret )
				{
					String str = String::Printf( TXT( "Something went wrong when removing animation: '%s' from animset: '%s'." ), animsToRemove[ i ]->GetName().AsString().AsChar(), animset->m_name.AsChar() );
					wxMessageBox( str.AsChar(), wxT("Error") );
					WARN_EDITOR( TXT("%s"), str.AsChar() );
				}
			}

			if ( !skeletalAnimationSet->Save() )
			{
				String str = String::Printf( TXT( "Error while saving animset '%s'" ), animset->m_name.AsChar() );
				wxMessageBox( str.AsChar(), wxT("Error") );
				WARN_EDITOR( TXT("%s"), str.AsChar() );
			}
		}
	}
}

void CEdAnimationReporterWindow::OnStreamingReport( wxCommandEvent& event )
{
	GFeedback->BeginTask( TXT("Report"), true );

	// 1. Collect data
	GFeedback->UpdateTaskInfo( TXT("Collecting animsets...") );

	TDynArray< String > depotFilesAnimsets;

	String depotPath;
	GDepot->GetAbsolutePath( depotPath );

	String animsetEx = TXT("*."); 
	animsetEx += CSkeletalAnimationSet::GetFileExtension();

	GFileManager->FindFiles( depotPath, animsetEx, depotFilesAnimsets, true );
	const Uint32 size = depotFilesAnimsets.Size();

	TDynArray< TPair< String, TPair< Uint32, Uint32 > > > data;
	data.Resize( size );

	for ( Uint32 i=0; i<size; ++i )
	{
		const String& filePath = depotFilesAnimsets[ i ];

		data[ i ].m_second.m_first = 0;
		data[ i ].m_second.m_second = 0;

		if ( GFeedback->IsTaskCanceled() )
		{
			break;
		}

		GFeedback->UpdateTaskInfo( TXT("Animset %s"), filePath.AsChar() );
		GFeedback->UpdateTaskProgress( i, size );

		String localPath;
		GDepot->ConvertToLocalPath( filePath, localPath );

		CSkeletalAnimationSet* set = LoadResource< CSkeletalAnimationSet >( localPath );
		if ( set )
		{
			Bool added = false;

			const TDynArray< CSkeletalAnimationSetEntry* >& anims = set->GetAnimations();

			const Uint32 animSize = anims.Size();
			for ( Uint32 a=0; a<animSize; ++a )
			{
				const CSkeletalAnimationSetEntry* entry = anims[ a ];
				if ( entry && entry->GetAnimation() )
				{
					const CSkeletalAnimation* animation = entry->GetAnimation();

					if ( animation->GetStreamingType() == SAST_Standard )
					{
						continue;
					}

					if ( !added )
					{
						data[ i ].m_first = localPath;
						added = true;
					}

					if ( animation->GetStreamingType() == SAST_Prestreamed )
					{
						data[ i ].m_second.m_first += animation->GetSizeOfAnimBuffer();
					}
					else if ( animation->GetStreamingType() == SAST_Persistent )
					{
						data[ i ].m_second.m_second += animation->GetSizeOfAnimBuffer();
					}
				}
			}
		}
	}

	GFeedback->EndTask();

	// 2. Show data
	String msg;

	Float allPrestr = 0;
	Float allPer = 0;

	msg += String::Printf( TXT("Prestreamed; Persistent; Path;\n") );

	const Float mg = 1.f / 1024.f / 1024.f;

	for ( Uint32 i=0; i<size; ++i )
	{
		if ( data[i].m_second.m_first > 0 || data[i].m_second.m_second > 0 )
		{
			const String& path = data[i].m_first;
			const Float prestr = (Float)data[i].m_second.m_first * mg;
			const Float per = (Float)data[i].m_second.m_second * mg;

			allPrestr += prestr;
			allPer += per;

			msg += String::Printf( TXT("%1.3f;     %1.3f;     %s;\n"), prestr, per, path.AsChar() );
		}
	}

	msg += String::Printf( TXT("-------------------------------------------------------------\n") );
	msg += String::Printf( TXT("%1.3f;     %1.3f;     all"), allPrestr, allPer );

	CEdMsgDlg dlg( TXT("Report"), msg );
	dlg.ShowModal();
}
