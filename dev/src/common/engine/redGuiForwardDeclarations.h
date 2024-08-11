/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI 

namespace RedGui
{
	// Controls
		
		// Control type
			
			// Abstract control
			class CRedGuiControl;
			class CRedGuiLayerItem;
			class CRedGuiControlInput;
			class CRedGuiControlSkin;
			class CRedGuiUserData;

			// Advanced
			class CRedGuiAdvancedSlider;

			// Charts
			class CRedGuiHistogram;
			class CRedGuiOccupancyChart;
			class CRedGuiAreaChart;
			class IRedGuiTimelineEvent;
			class CRedGuiTimeline;
			class CRedGuiTimelineChart;

			// Containers
			class CRedGuiDesktop;
			class CRedGuiGroupBox;
			class CRedGuiPanel;
			class CRedGuiScrollPanel;
			class CRedGuiTab;
			class CRedGuiWindow;
			class CRedGuiModalWindow;

			// Dialogs
			class CRedGuiOpenFileDialog;
			class CRedGuiSaveFileDialog;

			// Layouts
			class CRedGuiGridLayout;

			// Menu
			class CRedGuiMenu;
			class CRedGuiMenuBar;
			class CRedGuiMenuItem;

			// Misc
			class CRedGuiLine;
			class CRedGuiButton;
			class CRedGuiCheckBox;
			class CRedGuiComboBox;
			class CRedGuiImage;
			class CRedGuiLabel;
			class CRedGuiList;
			class CRedGuiListItem;
			class CRedGuiProgressBar;
			class CRedGuiScrollBar;
			class CRedGuiSeparator;
			class CRedGuiSlider;
			class CRedGuiSpin;
			class CRedGuiTextBox;

			// Prefabs
			class CRedGuiMessageBox;
			class CRedGuiProgressBox;
			class CRedGuiLoginBox;

			// Tree view
			class CRedGuiTreeNode;
			class CRedGuiTreeView;

		// Layers
		class IRedGuiLayer;
		class IRedGuiLayerItem;
		class IRedGuiLayerNode;
		class CRedGuiOverlappedLayer;

		// Misc
		class CRedGuiAnchor;
		class CRedGuiCroppedRect;
		class CRedGuiDock;
		class CRedGuiGraphicContext;

		// Theme
		class IRedGuiTheme;
		class CRedGuiDefaultTheme;
		class CRedGuiGradientTheme;

	// Managers
	class CRedGuiFontManager;
	class CRedGuiInputManager;
	class CRedGuiLayerManager;
	class CRedGuiManager;
	class CRedGuiRenderManager;
	class CRedGuiThemeManager;
	class CRedGuiToolTipManager;

}	// namespace RedGui

#endif	// NO_RED_GUI
