/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI 

#include "redGuiButton.h"
#include "redGuiTextBox.h"
#include "redGuiComboBox.h"
#include "redGuiTreeView.h"
#include "redGuiTreeNode.h"
#include "redGuiList.h"
#include "redGuiPanel.h"
#include "redGuiLabel.h"
#include "redGuiManager.h"
#include "redGuiOpenFileDialog.h"
#include "../core/depot.h"

namespace RedGui
{
	CRedGuiOpenFileDialog::CRedGuiOpenFileDialog()
		: CRedGuiModalWindow(100, 100, 900, 450)
	{
		GRedGui::GetInstance().RegisterWindowInActiveDesktop( this );

		SetCaption(TXT("Open file"));

		SetVisibleCaptionButton(CB_Minimize, false);
		SetVisibleCaptionButton(CB_Maximize, false);
		SetBackgroundColor(Color::GRAY);

		// create bottom panel
		{
			CRedGuiPanel* bottomPanel = new CRedGuiPanel(0,0, GetWidth(), 70);
			bottomPanel->SetPadding(Box2(5, 10, 5, 5));
			AddChild(bottomPanel);
			bottomPanel->SetDock(DOCK_Bottom);

			// create controls for file name
			{
				CRedGuiPanel* fileInfoPanel = new CRedGuiPanel(0,0, GetWidth(), 20);
				fileInfoPanel->SetBorderVisible(false);
				fileInfoPanel->SetBackgroundColor(Color::CLEAR);
				bottomPanel->AddChild(fileInfoPanel);
				fileInfoPanel->SetDock(DOCK_Top);

				m_openedTypes = new CRedGuiComboBox(0,0, 190, 25);
				m_openedTypes->SetMargin(Box2(10, 0, 10, 0));
				fileInfoPanel->AddChild(m_openedTypes);
				m_openedTypes->SetDock(DOCK_Right);

				m_fileNameTextBox = new CRedGuiTextBox(0,0, 500, 20);
				m_fileNameTextBox->SetMultiLine(false);
				m_fileNameTextBox->SetMargin(Box2(10, 0, 10, 0));
				fileInfoPanel->AddChild(m_fileNameTextBox);
				m_fileNameTextBox->SetDock(DOCK_Right);

				CRedGuiLabel* nameLabel = new CRedGuiLabel(0,0, 50, 20);
				nameLabel->SetText(TXT("File name: "));
				fileInfoPanel->AddChild(nameLabel);
				nameLabel->SetDock(DOCK_Right);
			}

			// create panel for buttons
			{
				CRedGuiPanel* buttonsPanel = new CRedGuiPanel(0,0, GetWidth(), 20);
				buttonsPanel->SetMargin(Box2(5, 5, 5, 5));
				buttonsPanel->SetBorderVisible(false);
				buttonsPanel->SetBackgroundColor(Color::CLEAR);
				bottomPanel->AddChild(buttonsPanel);
				buttonsPanel->SetDock(DOCK_Bottom);

				m_cancel = new CRedGuiButton(0, 0, 100, 25);
				m_cancel->SetMargin(Box2(10, 0, 10, 0));
				m_cancel->SetText(TXT("Cancel"));
				m_cancel->EventButtonClicked.Bind(this, &CRedGuiOpenFileDialog::NotifyButtonClicked);
				buttonsPanel->AddChild(m_cancel);
				m_cancel->SetDock(DOCK_Right);

				m_open = new CRedGuiButton(0, 0, 100, 25);
				m_open->SetMargin(Box2(10, 0, 10, 0));
				m_open->SetText(TXT("Open"));
				m_open->EventButtonClicked.Bind(this, &CRedGuiOpenFileDialog::NotifyButtonClicked);
				buttonsPanel->AddChild(m_open);
				m_open->SetDock(DOCK_Right);
			}
		}

		// create panel for directory tree and files list
		{
			CRedGuiPanel* filesPanel = new CRedGuiPanel(0,0, GetWidth(), GetHeight());
			filesPanel->SetBorderVisible(false);
			filesPanel->SetBackgroundColor(Color::CLEAR);
			AddChild(filesPanel);
			filesPanel->SetDock(DOCK_Fill);

			m_directoryTreeView = new CRedGuiTreeView(0, 0, 200, 400);
			m_directoryTreeView->EventSelectedNode.Bind(this, &CRedGuiOpenFileDialog::NotifyEventNodeSelectedChanged);
			m_directoryTreeView->EventNodeExpanded.Bind(this, &CRedGuiOpenFileDialog::NotifyEventNodeExpanded);
			filesPanel->AddChild(m_directoryTreeView);
			m_directoryTreeView->SetDock(DOCK_Left);

			m_directoryContent = new CRedGuiList(0,0, 600, 400);
			m_directoryContent->SetColLabelsVisible( true );
			m_directoryContent->SetSorting( true );
			m_directoryContent->SetTextAlign( IA_MiddleLeft );
			m_directoryContent->AppendColumn( TXT("Name"), 400 );
			m_directoryContent->AppendColumn( TXT("Date modified"), 200 );
			m_directoryContent->AppendColumn( TXT("Size"), 100 );
			m_directoryContent->SetColumnSortType( 2, RedGui::SA_Integer );
			m_directoryContent->EventSelectedItem.Bind(this, &CRedGuiOpenFileDialog::NotifyEventSelectedItemChanged);
			filesPanel->AddChild(m_directoryContent);
			m_directoryContent->SetDock(DOCK_Fill);
		}

		// add default filter
		AddFilter(TXT("All types"), TXT("*"));
		m_openedTypes->SetSelectedIndex(0);
	}

	CRedGuiOpenFileDialog::~CRedGuiOpenFileDialog()
	{
		m_cancel->EventButtonClicked.Unbind(this, &CRedGuiOpenFileDialog::NotifyButtonClicked);
		m_open->EventButtonClicked.Unbind(this, &CRedGuiOpenFileDialog::NotifyButtonClicked);
		m_directoryTreeView->EventSelectedNode.Unbind(this, &CRedGuiOpenFileDialog::NotifyEventNodeSelectedChanged);
		m_directoryTreeView->EventNodeExpanded.Unbind(this, &CRedGuiOpenFileDialog::NotifyEventNodeExpanded);
		m_directoryContent->EventSelectedItem.Unbind(this, &CRedGuiOpenFileDialog::NotifyEventSelectedItemChanged);
	}

	void CRedGuiOpenFileDialog::NotifyButtonClicked( RedGui::CRedGuiEventPackage& eventPackage )
	{
		RedGui::CRedGuiControl* sender = eventPackage.GetEventSender();

		CRedGuiWindow::NotifyButtonClicked( eventPackage );

		if(sender == m_cancel)
		{
			SetVisible(false);
		}
		else if(sender == m_open)
		{
			if( m_directoryContent->GetSelection() != -1 )
			{
				String ext = TXT("");
				ext = m_filters[m_openedTypes->GetSelectedIndex()].m_second;
				

				if( m_directoryContent->GetSelection() != -1 )
				{
					m_fileName = String::Printf(TXT("%s%s"), m_directoryTreeView->GetSelectedNode()->GetFullPath().AsChar(), 
						m_directoryContent->GetItemText( m_directoryContent->GetSelection() ).AsChar());

					EventFileOK(this);
				}

				SetVisible(false);
			}
		}
	}

	void CRedGuiOpenFileDialog::FillTreeView()
	{
		if(m_directoryTreeView->GetNodeCount(false) == 0)
		{
			CRedGuiTreeNode* node = m_directoryTreeView->AddRootNode( TXT("Depot") );
			node->SetUserString( TXT("Filled"), TXT("false") );

			AddFolderToTree( node, GDepot );
		}
	}

	void CRedGuiOpenFileDialog::AddFolderToTree( CRedGuiTreeNode* rootNode, CDirectory* rootDir )
	{
		for (CDirectory* dir : rootDir->GetDirectories() )
		{
			CRedGuiTreeNode* dirNode = rootNode->AddNode( dir->GetName() );
			dirNode->SetUserString( TXT("Filled"), TXT("false") );
		}
	}

	String CRedGuiOpenFileDialog::GetFileName() const
	{
		return m_fileName;
	}

	const TDynArray<FileExtensionInfo, MC_RedGuiControls, MemoryPool_RedGui>& CRedGuiOpenFileDialog::GetFilters() const
	{
		return m_filters;
	}

	void CRedGuiOpenFileDialog::SetFilters( const TDynArray<FileExtensionInfo, MC_RedGuiControls, MemoryPool_RedGui>& filters )
	{
		m_filters = filters;
	}

	void CRedGuiOpenFileDialog::AddFilter( const String& description, const String& extension )
	{
		FileExtensionInfo info(description, extension);
		m_filters.PushBack(info);
	}

	Bool CRedGuiOpenFileDialog::GetAddExtension() const
	{
		return m_addExtension;
	}

	void CRedGuiOpenFileDialog::SetAddExtension( Bool value )
	{
		m_addExtension = value;
	}

	void CRedGuiOpenFileDialog::OnWindowOpened( CRedGuiControl* control )
	{
		m_openedTypes->ClearAllItems();
		for(Uint32 i=0; i<m_filters.Size(); ++i )
		{
			{
				String itemText = m_filters[i].m_first + TXT("   (*.") + m_filters[i].m_second + TXT(")");
				m_openedTypes->AddItem(itemText);
			}

		}
		m_openedTypes->SetSelectedIndex(0);

		FillTreeView();

		m_fileNameTextBox->SetText( m_defaultFileName );
	}

	void CRedGuiOpenFileDialog::NotifyEventNodeSelectedChanged( RedGui::CRedGuiEventPackage& eventPackage, CRedGuiTreeNode* node )
	{
		RED_UNUSED( eventPackage );

		m_directoryContent->RemoveAllItems();
		String path = node->GetFullPath();
		path.Replace(TXT("depot\\"), TXT(""));

		Uint32 index = 0;
		const TFiles& depotFiles2 = GDepot->FindPath( path.AsChar() )->GetFiles();
		for ( CDiskFile* file : depotFiles2 )
		{
			m_directoryContent->AddItem( TXT("") );

			m_directoryContent->SetItemText( file->GetFileName(), index, 0 );

			m_directoryContent->SetItemText( ToString( file->GetFileTime(), DateFormat_YearMonthDay ), index, 1 );

			Uint64 fileSize = GFileManager->GetFileSize( GDepot->GetRootDataPath() + path + file->GetFileName() );
			Float fileSizeKB = Red::Math::MCeil( fileSize / 1024.0f );
			m_directoryContent->SetItemText( ToString( (Uint32)fileSizeKB ) + TXT(" KB"), index, 2 );

			index += 1;
		}

		SetCaption(TXT("Open file     ") + node->GetFullPath());
	}

	void CRedGuiOpenFileDialog::NotifyEventNodeExpanded( RedGui::CRedGuiEventPackage& eventPackage, CRedGuiTreeNode* node )
	{
		RED_UNUSED( eventPackage );

		if( node->GetUserString(TXT("Filled")) == TXT("false") )
		{
			for( Uint32 i=0; i<node->GetNodeCount(); ++i )
			{
				String path = node->GetNodeAt( i )->GetFullPath();
				path.Replace( TXT("depot\\"), TXT("") );
				CDirectory* dir =  GDepot->FindPath( path.AsChar() );

				if( dir != nullptr )
				{
					AddFolderToTree( node->GetNodeAt(i), dir );
				}
			}

			node->SetUserString( TXT("Filled"), TXT("true") );
		}
	}

	void CRedGuiOpenFileDialog::NotifyEventSelectedItemChanged( RedGui::CRedGuiEventPackage& eventPackage, Int32 selectedIndex )
	{
		RED_UNUSED( eventPackage );

		if(selectedIndex != -1)
		{
			m_fileNameTextBox->SetText( m_directoryContent->GetItemText( selectedIndex ) );
		}
	}

	void CRedGuiOpenFileDialog::OnWindowClosed( CRedGuiControl* control )
	{
		SetCaption(TXT("Open file"));
		m_directoryContent->RemoveAllItems();
		m_directoryTreeView->CollapseAll();
		m_fileNameTextBox->SetText(TXT(""));
	}

	String CRedGuiOpenFileDialog::GetDefaultFileName() const
	{
		return m_defaultFileName;
	}

	void CRedGuiOpenFileDialog::SetDefaultFileName( const String& filename )
	{
		m_defaultFileName = filename;
	}

}	// namespace RedGui

#endif	// NO_RED_GUI
