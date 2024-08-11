#include "build.h"
#include "debugWindowMemoryBudgets.h"
#include "memoryBudgetStorage.h"
#include "redGuiScrollPanel.h"
#include "redGuiPanel.h"
#include "redGuiLabel.h"
#include "redGuiButton.h"
#include "redGuiComboBox.h"
#include "redGuiGroupBox.h"

#if !defined( NO_RED_GUI ) && defined( ENABLE_EXTENDED_MEMORY_METRICS )

class CDebugWindowBudgetsBarGraph : public RedGui::CRedGuiPanel
{
public:
	CDebugWindowBudgetsBarGraph()
		: CRedGuiPanel( 0, 0, 50, 20 )
		, m_budgetUsedPercent( 0.0f )
		, m_peakBarUsedPercent( 0.0f )
	{
		SetBorderVisible( false );
	}
	virtual ~CDebugWindowBudgetsBarGraph() { }

	void SetUsedPercent( Float pc )	{ m_budgetUsedPercent = pc; }
	void SetPeakPercent( Float pc ) { m_peakBarUsedPercent = pc; }

	void Draw()
	{
		CRedGuiPanel::Draw();

		GetTheme()->SetCroppedParent( this );

		Vector2 origin = GetAbsolutePosition();
		Vector2 dimensions( (Float)GetWidth(), (Float)GetHeight() );
		GetTheme()->DrawRawFilledRectangle( origin, dimensions, Color::BLACK );

		Float graphScale = m_budgetUsedPercent / 100.0f;
		Color colour = Color( (Uint8)(graphScale * 255.0f), (Uint8)(( 1.0f - graphScale ) * 255.0f), 0 );
		GetTheme()->DrawRawFilledRectangle( origin, Vector2( dimensions.X * graphScale, dimensions.Y ), colour );

		// peak bar
		Float peakScale = m_peakBarUsedPercent / 100.0f;
		GetTheme()->DrawRawLine( origin + Vector2( dimensions.X * peakScale, 0.0f ), origin + Vector2( dimensions.X * peakScale, dimensions.Y ), Color::WHITE );

		GetTheme()->ResetCroppedParent();
	}
private:
	Float m_budgetUsedPercent;
	Float m_peakBarUsedPercent;
};

class CDebugWindowBudgetsBar : public RedGui::CRedGuiPanel
{
public:
	CDebugWindowBudgetsBar( const String& title )
		 : CRedGuiPanel( 0, 0, 100, 20 )
		 , m_title( title )
	{
		SetBorderVisible( false );

		RedGui::CRedGuiPanel* pnl = new RedGui::CRedGuiPanel( 0, 0, 150, 20 );
		pnl->SetDock( RedGui::DOCK_Top );

		RedGui::CRedGuiPanel* leftpnl = new RedGui::CRedGuiPanel( 0, 0, 150, 20 );
		leftpnl->SetBorderVisible( false );
		leftpnl->SetDock( RedGui::DOCK_Left );

		RedGui::CRedGuiLabel* lbl = new RedGui::CRedGuiLabel( 0, 0, 150, 20 );
		lbl->SetText( m_title );
		lbl->SetDock( RedGui::DOCK_Left );
		leftpnl->AddChild( lbl );

		pnl->AddChild( leftpnl );

		m_graph = new CDebugWindowBudgetsBarGraph();
		m_graph->SetDock( RedGui::DOCK_Fill );
		pnl->AddChild( m_graph );

		AddChild( pnl );
	}
	virtual ~CDebugWindowBudgetsBar() { }
private:
	String m_title;
protected:
	CDebugWindowBudgetsBarGraph* m_graph;
};

class CDebugWindowBudgetsBarPool : public CDebugWindowBudgetsBar
{
public:
	CDebugWindowBudgetsBarPool( const String& title, Red::MemoryFramework::MemoryManager* manager, Red::MemoryFramework::PoolLabel pool, MemSize budget )
		: CDebugWindowBudgetsBar( title )
		, m_manager( manager )
		, m_pool( pool )
		, m_budget( budget )
	{
	}
	virtual ~CDebugWindowBudgetsBarPool() { }

	virtual void UpdateControl()
	{
		CDebugWindowBudgetsBar::UpdateControl();

		const Red::MemoryFramework::RuntimePoolMetrics& poolMetrics = m_manager->GetMetricsCollector().GetMetricsForPool( m_pool );
		Float budgetUsedPercent = 100.0f * ( (Float)poolMetrics.m_totalBytesAllocated / (Float)m_budget );
		budgetUsedPercent = Red::Math::NumericalUtils::Clamp( budgetUsedPercent, 0.0f, 100.0f );

		Float peakUsedPercent = 100.0f * ( (Float)poolMetrics.m_totalBytesAllocatedPeak / (Float)m_budget );
		peakUsedPercent = Red::Math::NumericalUtils::Clamp( peakUsedPercent, 0.0f, 100.0f );

		m_graph->SetUsedPercent( budgetUsedPercent );
		m_graph->SetPeakPercent( peakUsedPercent );
	}

private:
	Red::MemoryFramework::MemoryManager* m_manager;
	Red::MemoryFramework::PoolLabel m_pool;
	MemSize m_budget;
};

class CDebugWindowBudgetsBarMemclass : public CDebugWindowBudgetsBar
{
public:
	CDebugWindowBudgetsBarMemclass( const String& title, Red::MemoryFramework::MemoryManager* manager, Red::MemoryFramework::MemoryClass memClass, MemSize budget )
		: CDebugWindowBudgetsBar( title )
		, m_manager( manager )
		, m_memoryClass( memClass )
		, m_budget( budget )
	{
	}
	virtual ~CDebugWindowBudgetsBarMemclass() { }

	virtual void UpdateControl()
	{
		CDebugWindowBudgetsBar::UpdateControl();
		
		Red::MemoryFramework::RuntimePoolMetrics allMetrics;
		m_manager->GetMetricsCollector().PopulateAllMetrics( allMetrics );

		Float budgetUsedPercent = 100.0f * ( (Float)m_manager->GetMetricsCollector().GetTotalBytesAllocatedForClass( m_memoryClass ) / (Float)m_budget );
		budgetUsedPercent = Red::Math::NumericalUtils::Clamp( budgetUsedPercent, 0.0f, 100.0f );

		Float peakUsedPercent = 100.0f * ( (Float)m_manager->GetMetricsCollector().GetTotalBytesAllocatedForClassPeak( m_memoryClass ) / (Float)m_budget );
		peakUsedPercent = Red::Math::NumericalUtils::Clamp( peakUsedPercent, 0.0f, 100.0f );

		m_graph->SetPeakPercent( peakUsedPercent );
		m_graph->SetUsedPercent( budgetUsedPercent );
	}

private:
	Red::MemoryFramework::MemoryManager* m_manager;
	Red::MemoryFramework::MemoryClass m_memoryClass;
	MemSize m_budget;
};

class CDebugWindowBudgetsBarMemclassGroup : public CDebugWindowBudgetsBar
{
public:
	CDebugWindowBudgetsBarMemclassGroup( const String& title, Red::MemoryFramework::MemoryManager* manager, Uint32 groupIndex, MemSize budget )
		: CDebugWindowBudgetsBar( title )
		, m_manager( manager )
		, m_groupIndex( groupIndex )
		, m_budget( budget )
		, m_peakSizeBytes( 0 )
	{
	}
	virtual ~CDebugWindowBudgetsBarMemclassGroup() { }

	virtual void UpdateControl()
	{
		CDebugWindowBudgetsBar::UpdateControl();

		Red::MemoryFramework::RuntimePoolMetrics allMetrics;
		m_manager->GetMetricsCollector().PopulateAllMetrics( allMetrics );

		Int64 bytesUsed = 0;
		Uint32 classesInGroup = m_manager->GetMetricsCollector().GetMemoryClassCountInGroup( m_groupIndex );
		for( Uint32 classIndex = 0; classIndex < classesInGroup; ++classIndex )
		{
			Red::MemoryFramework::MemoryClass memClass = m_manager->GetMetricsCollector().GetMemoryClassInGroup( m_groupIndex, classIndex );
			bytesUsed += allMetrics.m_allocatedBytesPerMemoryClass[ memClass ];
		}
		m_peakSizeBytes = Red::Math::NumericalUtils::Max( m_peakSizeBytes, bytesUsed );

		Float budgetUsedPercent = 100.0f * ( (Float)bytesUsed / (Float)m_budget );
		budgetUsedPercent = Red::Math::NumericalUtils::Clamp( budgetUsedPercent, 0.0f, 100.0f );

		Float peakUsedPercent = 100.0f * ( (Float)m_peakSizeBytes / (Float)m_budget );
		peakUsedPercent = Red::Math::NumericalUtils::Clamp( peakUsedPercent, 0.0f, 100.0f );

		m_graph->SetPeakPercent( peakUsedPercent );
		m_graph->SetUsedPercent( budgetUsedPercent );
	}

private:
	Red::MemoryFramework::MemoryManager* m_manager;
	Uint32 m_groupIndex;
	MemSize m_budget;
	Int64 m_peakSizeBytes;
};

class CDebugWindowBudgetsGroup : public RedGui::CRedGuiGroupBox
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_RedGui, MC_RedGuiControls );
public:
	CDebugWindowBudgetsGroup( CMemoryBudgetGroup* memoryBudgetGroup )
		: CRedGuiGroupBox( 0, 0, 100, 20 + ( memoryBudgetGroup->GetTotalBudgets() * 20 ) )
		, m_memoryBudgetGroup( memoryBudgetGroup )
	{
		SetBorderVisible( false );

		if( memoryBudgetGroup )
		{
			SetText( memoryBudgetGroup->GetLabel() );
		}
		else
		{
			SetText( TXT( "<unknown group>" ) );
		}
	}
	virtual ~CDebugWindowBudgetsGroup() { }

	void AddBudgetBar( CDebugWindowBudgetsBar* bar )
	{
		AddChild( bar );
	}

private:
	CMemoryBudgetGroup* m_memoryBudgetGroup;
};

CDebugWindowMemoryBudgets::CDebugWindowMemoryBudgets( Uint32 left, Uint32 top, Uint32 width, Uint32 height )
	: CRedGuiScrollPanel( left, top, width, height )
	, m_mainPanel( nullptr )
	, m_selectedPlatformId( 0 )
	, m_shouldRebuildWindow( true )
{
	SetBorderVisible( false );

	RedGui::CRedGuiPanel* topPanel = new RedGui::CRedGuiPanel(0, 0, 200, 20);
	topPanel->SetBorderVisible( false );
	topPanel->SetDock( RedGui::DOCK_Top );
	{
		RedGui::CRedGuiLabel* lbl = new RedGui::CRedGuiLabel(0, 0, 100, 20);
		lbl->SetDock( RedGui::DOCK_Left );
		lbl->SetText( TXT( "Show budgets for: " ) );
		topPanel->AddChild( lbl );

		m_platformCombobox = new RedGui::CRedGuiComboBox( 0, 0, 100, 20 );
		m_platformCombobox->SetDock( RedGui::DOCK_Left );
		m_platformCombobox->EventSelectedIndexChanged.Bind( this, &CDebugWindowMemoryBudgets::NotifySelectedPlatform );
		topPanel->AddChild( m_platformCombobox );		
	}
	AddChild( topPanel );
	RebuildPanels();
}

CDebugWindowMemoryBudgets::~CDebugWindowMemoryBudgets()
{
}

void CDebugWindowMemoryBudgets::OnPendingDestruction()
{
	m_platformCombobox->EventSelectedIndexChanged.Unbind( this, &CDebugWindowMemoryBudgets::NotifySelectedPlatform );
}

void CDebugWindowMemoryBudgets::UpdateControl()
{
	if( m_shouldRebuildWindow )
	{
		RebuildPanels();
		m_shouldRebuildWindow = false;
	}

	CRedGuiScrollPanel::UpdateControl();
}

void CDebugWindowMemoryBudgets::NotifySelectedPlatform( RedGui::CRedGuiEventPackage& eventPackage, Int32 selectedItem )
{
	m_selectedPlatformId = selectedItem;
	m_shouldRebuildWindow = true;
}

void CDebugWindowMemoryBudgets::RebuildPanels()
{
	if( m_mainPanel )
	{
		m_mainPanel->Dispose();
		m_mainPanel = nullptr;
	}

	m_mainPanel = new RedGui::CRedGuiScrollPanel(0, 0, 200, 300);
	m_mainPanel->SetBorderVisible( false );
	m_mainPanel->SetVisible(true);
	m_mainPanel->SetDock(RedGui::DOCK_Fill);
	m_mainPanel->SetVisibleVScroll( true );
	AddChild(m_mainPanel);	

	for( auto platformIt = SMemoryBudgets::GetInstance().BeginPlatforms(); platformIt != SMemoryBudgets::GetInstance().EndPlatforms(); ++platformIt )
	{
		if( platformIt.Key() == m_selectedPlatformId )
		{
			for( auto grpIt = SMemoryBudgets::GetInstance().BeginGroups( platformIt.Key() ); grpIt != SMemoryBudgets::GetInstance().EndGroups( platformIt.Key() ); ++grpIt )
			{
				CDebugWindowBudgetsGroup* grp = new CDebugWindowBudgetsGroup( &(*grpIt) );
				grp->SetDock( RedGui::DOCK_Top );

				// Add pool bars
				for( auto poolIt = grpIt->BeginPools(); poolIt != grpIt->EndPools(); ++poolIt )
				{
					String poolName = ANSI_TO_UNICODE( grpIt->GetMemoryManager()->GetMemoryPoolName( poolIt.Key() ) );
					CDebugWindowBudgetsBarPool* budgetBar = new CDebugWindowBudgetsBarPool( poolName, grpIt->GetMemoryManager(), poolIt.Key(), poolIt.Value() );
					budgetBar->SetDock( RedGui::DOCK_Top );
					grp->AddBudgetBar( budgetBar );
				}

				// Add memclass bars
				for( auto classIt = grpIt->BeginClasses(); classIt != grpIt->EndClasses(); ++classIt )
				{
					String className = ANSI_TO_UNICODE( grpIt->GetMemoryManager()->GetMemoryClassName( classIt.Key() ) );
					CDebugWindowBudgetsBarMemclass* budgetBar = new CDebugWindowBudgetsBarMemclass( className, grpIt->GetMemoryManager(), classIt.Key(), classIt.Value() );
					budgetBar->SetDock( RedGui::DOCK_Top );
					grp->AddBudgetBar( budgetBar );
				}

				// Add memclass group bars
				for( auto classGrpIt = grpIt->BeginClassGroups(); classGrpIt != grpIt->EndClassGroups(); ++classGrpIt )
				{
					String className = ANSI_TO_UNICODE( grpIt->GetMemoryManager()->GetMetricsCollector().GetMemoryClassGroupName( classGrpIt.Key() ) );
					CDebugWindowBudgetsBarMemclassGroup* budgetBar = new CDebugWindowBudgetsBarMemclassGroup( className, grpIt->GetMemoryManager(), classGrpIt.Key(), classGrpIt.Value() );
					budgetBar->SetDock( RedGui::DOCK_Top );
					grp->AddBudgetBar( budgetBar );
				}

				m_mainPanel->AddChild( grp );
			}
		}
	}

	m_platformCombobox->ClearAllItems();
	for( auto platformIt = SMemoryBudgets::GetInstance().BeginPlatforms(); platformIt != SMemoryBudgets::GetInstance().EndPlatforms(); ++platformIt )
	{
		m_platformCombobox->AddItem( platformIt.Value() );
	}
	m_platformCombobox->SetSelectedIndex( m_selectedPlatformId );
}

#endif