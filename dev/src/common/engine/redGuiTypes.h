/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI 

#include "redGuiDelegates.h"
#include "redGuiEnumerates.h"
#include "redGuiEventPackage.h"
#include "redGuiForwardDeclarations.h"
#include "viewportHook.h"

class CRenderFrame;

namespace RedGui
{
	//////////////////////////////////////////////////////////////////////////
	// TYPEDEF
	typedef THashMap< String, String >												MapString;
	typedef THashMap< CName, IRedGuiTheme* >										ThemesContainer;
	typedef TDynArray< IRedGuiLayer*, MC_RedGuiContainer, MemoryPool_RedGui >		ArrayLayerPtr;
	typedef TDynArray< CRedGuiControl*, MC_RedGuiContainer, MemoryPool_RedGui >		ArrayControlPtr;
	typedef TDynArray< CRedGuiDesktop*, MC_RedGuiContainer, MemoryPool_RedGui >		ArrayDesktopPtr;
	typedef TDynArray< IRedGuiLayerItem*, MC_RedGuiContainer, MemoryPool_RedGui >	ArrayLayerItemPtr;
	typedef TDynArray< IRedGuiLayerNode*, MC_RedGuiContainer, MemoryPool_RedGui >	ArrayLayerNodePtr;
	typedef TDynArray< CRedGuiListItem*, MC_RedGuiContainer, MemoryPool_RedGui >	ListItemCollection;
	typedef TDynArray< Uint32, MC_RedGuiContainer, MemoryPool_RedGui >				ListItemIndicies;
	typedef TDynArray< CRedGuiTimeline*, MC_RedGuiContainer, MemoryPool_RedGui >	TimelineCollection;
	typedef TPair< String, String >													FileExtensionInfo;
	typedef void*																	RedGuiAny;



	//////////////////////////////////////////////////////////////////////////
	// DELEGATES 1
	typedef TRedGuiDelegate1		Event1_Package;
	
	//////////////////////////////////////////////////////////////////////////
	// DELEGATES 2
	typedef TRedGuiDelegate2< Float >					Event2_PackageFloat;
	typedef TRedGuiDelegate2< Bool >					Event2_PackageBool;
	typedef TRedGuiDelegate2< const CMousePacket& >		Event2_PackageMousePacket;
	typedef TRedGuiDelegate2< Int32 >					Event2_PackageInt32;
	typedef TRedGuiDelegate2< Float >					Event2_PackageValue;
	typedef TRedGuiDelegate2< Uint32 >					Event2_PackageUint32;
	typedef TRedGuiDelegate2< const Vector2& >			Event2_PackageVector2;
	typedef TRedGuiDelegate2< CRedGuiControl* >			Event2_PackageControl;
	typedef TRedGuiDelegate2< CRedGuiMenuItem* >		Event2_PackageMenuItem;
	typedef TRedGuiDelegate2< enum EInputKey >			Event2_PackageInputKey;
	typedef TRedGuiDelegate2< CRedGuiTreeNode* >		Event2_PackageTreeNode;	
	typedef TRedGuiDelegate2< const String& >			Event2_PackageString;

	//////////////////////////////////////////////////////////////////////////
	// DELEGATES 3
	typedef TRedGuiDelegate3< Bool				, const Vector2& >		Event3_PackageBoolVector2;
	typedef TRedGuiDelegate3< enum EInputKey	, Char >				Event3_PackageInputKeyChar;
	typedef TRedGuiDelegate3< const Vector2&	, const Vector2& >		Event3_PackageVector2Vector2;
	typedef TRedGuiDelegate3< const Vector2&	, enum EMouseButton >	Event3_PackageVector2MouseButton;
	typedef TRedGuiDelegate3< IViewport*		, CRenderCamera& >		Event3_PackageViewportCamera;
	typedef TRedGuiDelegate3< IViewport*	 	, CRenderFrame* >		Event3_PackageViewportGenerateFragments;

	//////////////////////////////////////////////////////////////////////////
	// DELEGATES 4
	typedef TRedGuiDelegate4< IViewport*, Int32, Int32 >				Event4_PackageViewportInt32Int32;

	//////////////////////////////////////////////////////////////////////////
	// DELEGATES 5
	typedef TRedGuiDelegate5< IViewport*		, enum EInputKey, enum EInputAction	, Float >			Event5_PackageViewportInput;
	typedef TRedGuiDelegate5< IViewport*		, Int32			, Bool				, const Vector2& >	Event5_PackageViewportInt32BoolVector2;

}	// namespace RedGui

#endif	// NO_RED_GUI
