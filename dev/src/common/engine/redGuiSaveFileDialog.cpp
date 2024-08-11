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
#include "redGuiLabel.h"
#include "redGuiPanel.h"
#include "redGuiManager.h"
#include "redGuiSaveFileDialog.h"
#include "../core/depot.h"

namespace RedGui
{
	CRedGuiSaveFileDialog::CRedGuiSaveFileDialog()
		: CRedGuiModalWindow(100,100, 900, 450)
	{
		GRedGui::GetInstance().RegisterWindowInActiveDesktop( this );

		SetCaption( TXT("Save file") );

		SetVisibleCaptionButton(CB_Minimize, false);
		SetVisibleCaptionButton(CB_Maximize, false);
		SetBackgroundColor(Color::GRAY);

		// create bottom panel
		{
			CRedGuiPanel* bottomPanel = new CRedGuiPanel(0,0, GetWidth(), 90);
			bottomPanel->SetPadding(Box2(5, 10, 5, 5));
			AddChild(bottomPanel);
			bottomPanel->SetDock(DOCK_Bottom);
	
			// create controls for file name
			{
				CRedGuiPanel* fileNamePanel = new CRedGuiPanel(0,0, GetWidth(), 20);
				fileNamePanel->SetBorderVisible(false);
				fileNamePanel->SetBackgroundColor(Color::CLEAR);
				bottomPanel->AddChild(fileNamePanel);
				fileNamePanel->SetDock(DOCK_Top);

				m_fileNameTextBox = new CRedGuiTextBox(0,0, 750, 20);
				m_fileNameTextBox->SetMultiLine(false);
				m_fileNameTextBox->SetMargin(Box2(10, 0, 10, 0));
				fileNamePanel->AddChild(m_fileNameTextBox);
				m_fileNameTextBox->SetDock(DOCK_Right);

				CRedGuiLabel* nameLabel = new CRedGuiLabel(0,0, 50, 20);
				nameLabel->SetText(TXT("File name: "));
				fileNamePanel->AddChild(nameLabel);
				nameLabel->SetDock(DOCK_Right);
			}

			// create controls for file type
			{
				CRedGuiPanel* fileTypePanel = new CRedGuiPanel(0,0, GetWidth(), 20);
				fileTypePanel->SetMargin(Box2(5, 5, 5, 5));
				fileTypePanel->SetBorderVisible(false);
				fileTypePanel->SetBackgroundColor(Color::CLEAR);
				bottomPanel->AddChild(fileTypePanel);
				fileTypePanel->SetDock(DOCK_Top);

				m_saveAsType = new CRedGuiComboBox(0,0, 750, 20);
				m_saveAsType->SetMargin(Box2(10, 0, 10, 0));
				fileTypePanel->AddChild(m_saveAsType);
				m_saveAsType->SetDock(DOCK_Right);

				CRedGuiLabel* nameLabel = new CRedGuiLabel(0,0, 50, 20);
				nameLabel->SetText(TXT("Save as type: "));
				fileTypePanel->AddChild(nameLabel);
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
				m_cancel->EventButtonClicked.Bind(this, &CRedGuiSaveFileDialog::NotifyButtonClicked);
				buttonsPanel->AddChild(m_cancel);
				m_cancel->SetDock(DOCK_Right);

				m_save = new CRedGuiButton(0, 0, 100, 25);
				m_save->SetMargin(Box2(10, 0, 10, 0));
				m_save->SetText(TXT("Save"));
				m_save->EventButtonClicked.Bind(this, &CRedGuiSaveFileDialog::NotifyButtonClicked);
				buttonsPanel->AddChild(m_save);
				m_save->SetDock(DOCK_Right);
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
			m_directoryTreeView->EventSelectedNode.Bind(this, &CRedGuiSaveFileDialog::NotifyEventNodeSelectedChanged);
			m_directoryTreeView->EventNodeExpanded.Bind(this, &CRedGuiSaveFileDialog::NotifyEventNodeExpanded);
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
			m_directoryContent->EventSelectedItem.Bind(this, &CRedGuiSaveFileDialog::NotifyEventSelectedItemChanged);
			filesPanel->AddChild(m_directoryContent);
			m_directoryContent->SetDock(DOCK_Fill);
		}

		// add default filter
		AddFilter(TXT("All types"), TXT("*"));
		m_saveAsType->SetSelectedIndex(0);
	}

	CRedGuiSaveFileDialog::~CRedGuiSaveFileDialog()
	{
		
	}

	void CRedGuiSaveFileDialog::OnPendingDestruction()
	{
		m_cancel->EventButtonClicked.Unbind(this, &CRedGuiSaveFileDialog::NotifyButtonClicked);
		m_save->EventButtonClicked.Unbind(this, &CRedGuiSaveFileDialog::NotifyButtonClicked);
		m_directoryTreeView->EventSelectedNode.Unbind(this, &CRedGuiSaveFileDialog::NotifyEventNodeSelectedChanged);
		m_directoryTreeView->EventNodeExpanded.Unbind(this, &CRedGuiSaveFileDialog::NotifyEventNodeExpanded);
		m_directoryContent->EventSelectedItem.Unbind(this, &CRedGuiSaveFileDialog::NotifyEventSelectedItemChanged);
	}

	void CRedGuiSaveFileDialog::NotifyButtonClicked( RedGui::CRedGuiEventPackage& eventPackage )
	{
		RedGui::CRedGuiControl* sender = eventPackage.GetEventSender();

		CRedGuiWindow::NotifyButtonClicked( eventPackage );

		if(sender == m_cancel)
		{
			SetVisible(false);
		}
		else if(sender == m_save)
		{
			if( m_directoryTreeView->GetSelectedNode() != nullptr )
			{
				if(m_fileNameTextBox->GetText().Empty() == false)
				{
					String ext = TXT("");
					ext = m_filters[m_saveAsType->GetSelectedIndex()].m_second;
	
					m_fileName = String::Printf(TXT("%s%s.%s"), m_directoryTreeView->GetSelectedNode()->GetFullPath().AsChar(), 
						m_fileNameTextBox->GetText().AsChar(), m_filters[m_saveAsType->GetSelectedIndex()].m_second.AsChar() );
	
					// remove 'depot' from path
					m_fileName.Replace(TXT("depot\\"), TXT(""));
	
					// remove * mark
					m_fileName.Replace(TXT(".*"), TXT(""));
	
					EventFileOK(this);
	
					SetVisible(false);
				}
			}
		}
	}

	void CRedGuiSaveFileDialog::FillTreeView()
	{
		if(m_directoryTreeView->GetNodeCount(false) == 0)
		{
			CRedGuiTreeNode* depotRoot = m_directoryTreeView->AddRootNode( TXT("Depot") );
			depotRoot->SetUserString( TXT("Filled"), TXT("false") );

			AddFolderToTree( depotRoot, GDepot );
		}
	}

	void CRedGuiSaveFileDialog::AddFolderToTree( CRedGuiTreeNode* rootNode, CDirectory* rootDir )
	{
		for (CDirectory* dir : rootDir->GetDirectories() )
		{
			CRedGuiTreeNode* dirNode = rootNode->AddNode( dir->GetName() );
			dirNode->SetUserString( TXT("Filled"), TXT("false") );
		}
	}

	String CRedGuiSaveFileDialog::GetFileName() const
	{
		return m_fileName;
	}

	const TDynArray<FileExtensionInfo, MC_RedGuiControls, MemoryPool_RedGui>& CRedGuiSaveFileDialog::GetFilters() const
	{
		return m_filters;
	}

	void CRedGuiSaveFileDialog::SetFilters( const TDynArray<FileExtensionInfo, MC_RedGuiControls, MemoryPool_RedGui>& filters )
	{
		m_filters = filters;
	}

	void CRedGuiSaveFileDialog::AddFilter( const String& description, const String& extension )
	{
		FileExtensionInfo info(description, extension);
		m_filters.PushBack(info);
	}

	Bool CRedGuiSaveFileDialog::GetAddExtension() const
	{
		return m_addExtension;
	}

	void CRedGuiSaveFileDialog::SetAddExtension( Bool value )
	{
		m_addExtension = value;
	}

	void CRedGuiSaveFileDialog::OnWindowOpened( CRedGuiControl* control )
	{
		m_saveAsType->ClearAllItems();
		for(Uint32 i=0; i<m_filters.Size(); ++i )
		{
			{
				String itemText = m_filters[i].m_first + TXT("   (*.") + m_filters[i].m_second + TXT(")");
				m_saveAsType->AddItem(itemText);
			}

		}
		m_saveAsType->SetSelectedIndex(0);
		
		FillTreeView();

		m_fileNameTextBox->SetText( m_defaultFileName );
	}

	void CRedGuiSaveFileDialog::NotifyEventNodeSelectedChanged( RedGui::CRedGuiEventPackage& eventPackage, CRedGuiTreeNode* node )
	{
		RED_UNUSED( eventPackage );

		m_directoryContent->RemoveAllItems();
		String path = node->GetFullPath();
		path.Replace(TXT("depot\\"), TXT(""));

		Uint32 index = 0;
		CDirectory * directory = GDepot->FindPath( path.AsChar() );
		if ( directory )
		{
			const TFiles & depotFiles2 = directory->GetFiles();
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
		}

		SetCaption( TXT("Save file     ") + node->GetFullPath() );
	}

	void CRedGuiSaveFileDialog::NotifyEventNodeExpanded( RedGui::CRedGuiEventPackage& eventPackage, CRedGuiTreeNode* node )
	{
		RED_UNUSED( eventPackage );

		if( node->GetUserString(TXT("Filled")) == TXT("false") )
		{
			for( Uint32 i=0; i<node->GetNodeCount(); ++i )
			{
				String path = node->GetNodeAt(i)->GetFullPath();
				path.Replace( TXT("depot\\"), TXT("") );
				CDirectory* dir =  GDepot->FindPath( path.AsChar() );

				if(dir != nullptr)
				{
					AddFolderToTree(node->GetNodeAt(i), dir);
				}
			}

			node->SetUserString( TXT("Filled"), TXT("true") );
		}
	}

	void CRedGuiSaveFileDialog::NotifyEventSelectedItemChanged( RedGui::CRedGuiEventPackage& eventPackage, Int32 selectedIndex )
	{
		RED_UNUSED( eventPackage );

		if(selectedIndex != -1)
		{
			m_fileNameTextBox->SetText( m_directoryContent->GetItemText( selectedIndex ) );
		}
	}

	void CRedGuiSaveFileDialog::OnWindowClosed( CRedGuiControl* control )
	{
		SetCaption(TXT("Save file"));
		/* TODO: CollapseAll is weirdly deleting a whole directory tree view from the control.
		         The whole system is created in a way that it doesn't delete itself after termination of the App.
				 Since the SaveFileDialog exists all the time during the app lifespan (just getting invisible sometimes)
				 and there's no infrastructure (or desire?) to free m_directoryContent & m_directoryTreeView we don't have to
				 remove the items and collapse treeview. We'd rather keep last open directory for later use.
		*/
		//m_directoryContent->RemoveAllItems();
		//m_directoryTreeView->CollapseAll();
		m_fileNameTextBox->SetText(TXT(""));
	}

	String CRedGuiSaveFileDialog::GetDefaultFileName() const
	{
		return m_defaultFileName;
	}

	void CRedGuiSaveFileDialog::SetDefaultFileName( const String& filename )
	{
		m_defaultFileName = filename;
	}

}	// namespace RedGui

#endif	// NO_RED_GUI
