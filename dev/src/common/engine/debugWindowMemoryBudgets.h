#pragma once

#if !defined( NO_RED_GUI ) && defined( ENABLE_EXTENDED_MEMORY_METRICS )

#include "redGuiScrollPanel.h"

class CDebugWindowMemoryBudgets : public RedGui::CRedGuiScrollPanel
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_RedGui, MC_RedGuiControls );
public:
	CDebugWindowMemoryBudgets( Uint32 left, Uint32 top, Uint32 width, Uint32 height );
	virtual ~CDebugWindowMemoryBudgets();

private:
	virtual void UpdateControl();
	void NotifySelectedPlatform( RedGui::CRedGuiEventPackage& eventPackage, Int32 selectedItem );
	void RebuildPanels();

	virtual void OnPendingDestruction() override;

	RedGui::CRedGuiScrollPanel* m_mainPanel;
	RedGui::CRedGuiComboBox* m_platformCombobox;
	Uint32 m_selectedPlatformId;
	Bool m_shouldRebuildWindow;
};

#endif