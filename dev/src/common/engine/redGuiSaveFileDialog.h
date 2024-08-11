/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI 

#include "redGuiModalWindow.h"

class CDirectory;

namespace RedGui
{
	class CRedGuiSaveFileDialog : public CRedGuiModalWindow
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_RedGui, MC_RedGuiControls );
	public:
		CRedGuiSaveFileDialog();
		virtual ~CRedGuiSaveFileDialog();

		// Events
		Event1_Package EventFileOK;

		String GetFileName() const;

		const TDynArray<FileExtensionInfo, MC_RedGuiControls, MemoryPool_RedGui>& GetFilters() const;
		void SetFilters(const TDynArray<FileExtensionInfo, MC_RedGuiControls, MemoryPool_RedGui>& filters);
		void AddFilter(const String& description, const String& extension);

		Bool GetAddExtension() const;
		void SetAddExtension(Bool value);		

		String GetDefaultFileName() const;
		void SetDefaultFileName( const String& filename );

	private:
		void NotifyButtonClicked( RedGui::CRedGuiEventPackage& eventPackage );
		void NotifyEventNodeSelectedChanged( RedGui::CRedGuiEventPackage& eventPackage, CRedGuiTreeNode* node );
		void NotifyEventNodeExpanded( RedGui::CRedGuiEventPackage& eventPackage, CRedGuiTreeNode* node);
		void NotifyEventSelectedItemChanged( RedGui::CRedGuiEventPackage& eventPackage, Int32 selectedIndex);

		void FillTreeView();
		void AddFolderToTree( CRedGuiTreeNode* rootNode, CDirectory* rootDir );
		void OnWindowOpened( CRedGuiControl* control );
		void OnWindowClosed( CRedGuiControl* control );

		virtual void OnPendingDestruction() override final;

		CRedGuiButton*		m_cancel;
		CRedGuiButton*		m_save;
		CRedGuiTextBox*		m_fileNameTextBox;
		CRedGuiComboBox*	m_saveAsType;
		CRedGuiList*		m_directoryContent;
		CRedGuiTreeView*	m_directoryTreeView;

		Bool m_addExtension;
		String m_fileName;		
		String m_initialDirectory;
		String m_defaultFileName;
		TDynArray<FileExtensionInfo, MC_RedGuiControls, MemoryPool_RedGui> m_filters;

	};

}	// namespace RedGui

#endif	// NO_RED_GUI
