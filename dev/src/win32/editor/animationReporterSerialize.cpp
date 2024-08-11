
#include "build.h"
#include "animationReporter.h"

void CEdAnimationReporterWindow::Save()
{
	CEdFileDialog dlg;
	dlg.SetMultiselection( false );
	dlg.SetIniTag( TXT("CEdAnimationReporterWindow_Save") );
	dlg.AddFormat( TXT("txt"), TXT( "Animation report" ) );

	if ( dlg.DoSave( (HWND)GetHandle() ) )
	{				
		String filePath = dlg.GetFile();

		IFile* file = GFileManager->CreateFileWriter( filePath, FOF_AbsolutePath );
		if ( file )
		{
			Serialize( *file );
		}

		delete file;
	}
}

void CEdAnimationReporterWindow::Load()
{
	CEdFileDialog dlg;
	dlg.SetMultiselection( false );
	dlg.SetIniTag( TXT("CEdAnimationReporterWindow_Load") );
	dlg.AddFormat( TXT("txt"), TXT( "Animation report" ) );

	if ( dlg.DoOpen( (HWND)GetHandle() ) )
	{				
		String filePath = dlg.GetFile();

		IFile* file = GFileManager->CreateFileReader( filePath, FOF_AbsolutePath | FOF_Buffered );
		if ( file )
		{
			ClearAllRecords();

			Serialize( *file );

			RefreshReporterWindows();
		}

		delete file;
	}
}

void CEdAnimationReporterWindow::Serialize( IFile& file )
{
	// 1. Records and nodes
	if ( file.IsWriter() )
	{
		Uint32 temp = 0;

		temp = m_animsetRecords.Size();
		file << temp;

		temp = m_behaviorRecords.Size();
		file << temp;

		temp = m_jobRecords.Size();
		file << temp;

		temp = m_acNodes.Size();
		file << temp;

		temp = m_apNodes.Size();
		file << temp;

		temp = m_externalAnims.Size();
		file << temp;
	}
	else
	{
		Uint32 temp = 0;

		file << temp;
		m_animsetRecords.Resize( temp );

		file << temp;
		m_behaviorRecords.Resize( temp );

		file << temp;
		m_jobRecords.Resize( temp );

		file << temp;
		m_acNodes.Resize( temp );

		file << temp;
		m_apNodes.Resize( temp );

		file << temp;
		m_externalAnims.Resize( temp );

		for ( Uint32 i=0; i<m_animsetRecords.Size(); ++i )
		{
			m_animsetRecords[ i ] = new EdAnimReportAnimset();
		}
		for ( Uint32 i=0; i<m_behaviorRecords.Size(); ++i )
		{
			m_behaviorRecords[ i ] = new EdAnimReportBehavior();
		}
		for ( Uint32 i=0; i<m_jobRecords.Size(); ++i )
		{
			m_jobRecords[ i ] = new EdAnimReportJobTree();
		}
	}

	for ( Uint32 i=0; i<m_animsetRecords.Size(); ++i )
	{
		m_animsetRecords[ i ]->Serialize( file );
	}

	for ( Uint32 i=0; i<m_behaviorRecords.Size(); ++i )
	{
		m_behaviorRecords[ i ]->Serialize( file );
	}

	for ( Uint32 i=0; i<m_jobRecords.Size(); ++i )
	{
		m_jobRecords[ i ]->Serialize( file );
	}

	for ( Uint32 i=0; i<m_acNodes.Size(); ++i )
	{
		m_acNodes[ i ].Serialize( file );
	}

	for ( Uint32 i=0; i<m_apNodes.Size(); ++i )
	{
		m_apNodes[ i ].Serialize( file );
	}

	for ( Uint32 i=0; i<m_externalAnims.Size(); ++i )
	{
		m_externalAnims[ i ].Serialize( file );
	}

	// 2. File list
	file << m_cookList;

	// 3. Special anim list
	file << m_doNotRemoveAnimationList;

	// 3. Todo list
	m_todoList.Serialize( file );
}
