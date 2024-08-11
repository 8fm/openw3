/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once


class CSimpleBufferWriter;
class CSimpleBufferReader;

///////////////////////////////////////////////////////////////////////////////
// General tree dummy interface
///////////////////////////////////////////////////////////////////////////////
// Interface specyfication for general tree operations.
// May be used as base class for concrete tree interface classes, providing
// also some default (eg. empty) implementation for custom functionalities.
///////////////////////////////////////////////////////////////////////////////
class IGeneralTreeDummyInterface : public Red::System::NonCopyable
{
public:
	static const Uint32			TREE_MAX_DEPTH													= 256;

	typedef void* GeneralNodePtr;

	GeneralNodePtr		GetRootNode()															{ return nullptr; }
	Uint32				GetChildNodesCount( GeneralNodePtr node )								{ return 0; }
	GeneralNodePtr		GetChildNode( GeneralNodePtr node, Uint32 index )						{ return nullptr; }

	// returns true if node state should be saved
	Bool				IsNodeStateSaving( GeneralNodePtr node )								{ return false; }
	void				SaveNodeState( GeneralNodePtr node, IGameSaver* writer )				{}
	Bool				LoadNodeState( GeneralNodePtr node, IGameLoader* reader )				{ return true; }

	Uint32				GetPersistantChildsCount( GeneralNodePtr node )							{ return 0; }
	GeneralNodePtr		GetPersistantChild( GeneralNodePtr node, Uint32 index )					{ return nullptr; }
};
