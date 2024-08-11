#include "build.h"
#include "debugWindowArrayMemory.h"

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS
#ifdef USE_ARRAY_METRICS

#include "redGuiButton.h"
#include "redGuiGridLayout.h"
#include "redGuiSaveFileDialog.h"
#include "../core/depot.h"

namespace DebugWindows
{

	CDebugWindowArrayMemoryMetrics::CDebugWindowArrayMemoryMetrics()
		: RedGui::CRedGuiWindow( 200, 200, 200, 250 )
	{
		SetCaption( TXT("Array metrics") );

		CreateControls();
	}

	CDebugWindowArrayMemoryMetrics::~CDebugWindowArrayMemoryMetrics()
	{

	}

	void CDebugWindowArrayMemoryMetrics::CreateControls()
	{
		m_dumpSummaryDialog = new RedGui::CRedGuiSaveFileDialog();
		m_dumpSummaryDialog->SetDefaultFileName( TXT("ArraysSummary") );
		m_dumpSummaryDialog->AddFilter( TXT("Text file"), TXT("txt") );
		m_dumpSummaryDialog->EventFileOK.Bind( this, &CDebugWindowArrayMemoryMetrics::NotifyDumpSummaryFileOK );

		m_dumpDetailsDialog = new RedGui::CRedGuiSaveFileDialog();
		m_dumpDetailsDialog->SetDefaultFileName( TXT("ArraysDetails") );
		m_dumpDetailsDialog->AddFilter( TXT("Text file"), TXT("txt") );
		m_dumpDetailsDialog->EventFileOK.Bind( this, &CDebugWindowArrayMemoryMetrics::NotifyDumpDetailsFileOK );

		RedGui::CRedGuiGridLayout* menuPanel = new RedGui::CRedGuiGridLayout( 0, 0, 200, 250 );
		menuPanel->SetDock( RedGui::DOCK_Fill );
		menuPanel->SetMargin( Box2( 10, 10, 10, 10 ) );
		menuPanel->SetDimensions( 1, 4 );
		AddChild( menuPanel );

		RedGui::CRedGuiButton* summaryLog = new RedGui::CRedGuiButton( 0, 0, 100, 20 );
		summaryLog->SetText( TXT("Dump Summary To Log") );
		summaryLog->SetMargin( Box2( 5, 5, 10, 10 ) );
		summaryLog->EventButtonClicked.Bind( this, &CDebugWindowArrayMemoryMetrics::NotifyOnClickedSummaryToLog );
		menuPanel->AddChild( summaryLog );

		RedGui::CRedGuiButton* summaryFile = new RedGui::CRedGuiButton( 0, 0, 100, 20 );
		summaryFile->SetText( TXT("Dump Summary To File") );
		summaryFile->SetMargin( Box2( 5, 5, 10, 10 ) );
		summaryFile->EventButtonClicked.Bind( this, &CDebugWindowArrayMemoryMetrics::NotifyOnClickedSummaryToFile );
		menuPanel->AddChild( summaryFile );

		RedGui::CRedGuiButton* detailedLog = new RedGui::CRedGuiButton( 0, 0, 100, 20 );
		detailedLog->SetText( TXT("Dump Detailed To Log") );
		detailedLog->SetMargin( Box2( 5, 5, 10, 10 ) );
		detailedLog->EventButtonClicked.Bind( this, &CDebugWindowArrayMemoryMetrics::NotifyOnClickedDetailsToLog );
		menuPanel->AddChild( detailedLog );

		RedGui::CRedGuiButton* detailedFile = new RedGui::CRedGuiButton( 0, 0, 100, 20 );
		detailedFile->SetText( TXT("Dump Detailed To File") );
		detailedFile->SetMargin( Box2( 5, 5, 10, 10 ) );
		detailedFile->EventButtonClicked.Bind( this, &CDebugWindowArrayMemoryMetrics::NotifyOnClickedDetailsToFile );
		menuPanel->AddChild( detailedFile );
	}

	void CDebugWindowArrayMemoryMetrics::NotifyOnClickedSummaryToLog( RedGui::CRedGuiEventPackage& eventPackage )
	{
		RED_UNUSED( eventPackage );
		CBaseArray::TArrayDumpToLog dumpToLog;
		CBaseArray::DumpArrayMetricsSummary( dumpToLog );
	}

	void CDebugWindowArrayMemoryMetrics::NotifyOnClickedSummaryToFile( RedGui::CRedGuiEventPackage& eventPackage )
	{
		RED_UNUSED( eventPackage );
		m_dumpSummaryDialog->SetVisible( true );
	}

	void CDebugWindowArrayMemoryMetrics::NotifyOnClickedDetailsToLog( RedGui::CRedGuiEventPackage& eventPackage )
	{
		RED_UNUSED( eventPackage );
		CBaseArray::TArrayDumpToLog dumpToLog;
		CBaseArray::DumpArrayMetricsDetailed( dumpToLog );
	}

	void CDebugWindowArrayMemoryMetrics::NotifyOnClickedDetailsToFile( RedGui::CRedGuiEventPackage& eventPackage )
	{
		RED_UNUSED( eventPackage );
		m_dumpDetailsDialog->SetVisible( true );
	}

	void CDebugWindowArrayMemoryMetrics::NotifyDumpSummaryFileOK( RedGui::CRedGuiEventPackage& eventPackage )
	{
		String path = String::EMPTY;
		GDepot->GetAbsolutePath( path );
		path += m_dumpSummaryDialog->GetFileName();

		CBaseArray::TArrayDumpToFile dumpToFile( path.AsChar() );
		CBaseArray::DumpArrayMetricsSummary( dumpToFile );
	}

	void CDebugWindowArrayMemoryMetrics::NotifyDumpDetailsFileOK( RedGui::CRedGuiEventPackage& eventPackage )
	{
		String path = String::EMPTY;
		GDepot->GetAbsolutePath( path );
		path += m_dumpDetailsDialog->GetFileName();

		CBaseArray::TArrayDumpToFile dumpToFile( path.AsChar() );
		CBaseArray::DumpArrayMetricsDetailed( dumpToFile );
	}
}

#endif	//NO_RED_GUI
#endif	//NO_DEBUG_WINDOWS
#endif  //USE_ARRAY_METRICS