/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI 

namespace RedGui
{

	enum EMessabeBoxIcon
	{
		MESSAGEBOX_Warning,
		MESSAGEBOX_Error,
		MESSAGEBOX_Info,
	};

	enum EAnchor
	{
		ANCHOR_None = 0,

		ANCHOR_Left = (1 << 1),
		ANCHOR_Right = (1 << 2),
		ANCHOR_HStretch = ANCHOR_Left | ANCHOR_Right,

		ANCHOR_Top = (1 << 3),
		ANCHOR_Bottom = (1 << 4),
		ANCHOR_VStretch = ANCHOR_Top | ANCHOR_Bottom,

		ANCHOR_Stretch = ANCHOR_VStretch | ANCHOR_HStretch,

		ANCHOR_Default = ANCHOR_Left | ANCHOR_Top,
	};

	enum EMouseButton
	{
		MB_Left,
		MB_Right,
		MB_Middle,

		MB_Count,
	};

	enum EMousePointer
	{
		MP_Arrow,
		MP_Hand,
		MP_Wait,
		MP_Text,
		MP_Move,
		MP_HResize,
		MP_VResize,
		MP_SlashResize,
		MP_BackslashResize,

		MP_Count,
	};

	enum ERedGuiState
	{
		STATE_Pushed,
		STATE_Normal,
		STATE_Highlighted,
		STATE_Disabled,
	};

	enum EInternalAlign
	{
		IA_None,

		IA_TopLeft,
		IA_TopCenter,
		IA_TopRight,

		IA_MiddleLeft,
		IA_MiddleCenter,
		IA_MiddleRight,

		IA_BottomLeft,
		IA_BottomCenter,
		IA_BottomRight,
	};

	enum EDock
	{
		DOCK_None,
		DOCK_Left,
		DOCK_Right,
		DOCK_Top,
		DOCK_Bottom,
		DOCK_Fill,
	};

	enum ECaptionButton
	{
		CB_Help,
		CB_Minimize,
		CB_Maximize,
		CB_Exit,
	};

	enum ERedGuiMenuItemType
	{
		MENUITEM_Normal,
		MENUITEM_Separator,
		MENUITEM_SubMenu,
	};

	enum ESelectionMode
	{
		SM_None,
		SM_Single,
		SM_Multiple,
		SM_Extended,
	};

	enum ESortingType
	{
		ST_Ascending,
		ST_Descending
	};

	enum ESortingAs
	{
		SA_String,
		SA_Integer,
		SA_Real,

		SA_Count,
	};

	enum ERedGuiInputEvent
	{
		RGIE_NextControl,
		RGIE_PreviousControl,
		RGIE_NextWindow,
		RGIE_PreviousWindow,
		RGIE_Select,
		RGIE_Execute,
		RGIE_Back,
		RGIE_CatchAll,
		RGIE_UncatchAll,
		RGIE_MoveWindow,
		RGIE_ResizeWindow,
		RGIE_Left,
		RGIE_Right,
		RGIE_Up,
		RGIE_Down,
		RGIE_Move,
		RGIE_ActivateSpecialMode,
		RGIE_OptionsButton,

		RGIE_Count
	};

	enum ERedGuiExclusiveInput
	{
		RGEI_OnlyRedGui,
		RGEI_OnlyGame,
		RGEI_Both,

		RGEI_Count
	};

	enum ERedGuiFontType
	{
		RGFT_Default,
		RGFT_Parachute,

		RGFT_Count
	};

}	// namespace RedGui

#endif	// NO_RED_GUI
