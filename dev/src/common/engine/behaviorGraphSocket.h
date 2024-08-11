/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma  once

#include "../engine/graphSocket.h"
#include "../engine/graphBlock.h"

class CBehaviorGraphAnimationInputSocket;
class CBehaviorGraphAnimationOutputSocket;
class CBehaviorGraphStateNode;
class CBehaviorGraphStateTransitionNode;
class CBehaviorGraphValueNode;
class CBehaviorGraphVariableInputSocket;
class CBehaviorGraphVariableOutputSocket;
class CBehaviorGraphVectorVariableInputSocket;
class CBehaviorGraphVectorVariableOutputSocket;
class CBehaviorGraphTransitionSocket;
class CBehaviorGraphMimicAnimationOutputSocket;

class CBehaviorGraphAnimationInputSocket : public CGraphSocket
{
	DECLARE_RTTI_SIMPLE_CLASS( CBehaviorGraphAnimationInputSocket );

public:
	//! Can we connect to given socket
	virtual Bool CanConnectTo( CGraphSocket *otherSocket );

	//! Get socked connected to that input
	CBehaviorGraphAnimationOutputSocket* GetConnectedSocket() const; 

	//! Get node connected to that socket
	CBehaviorGraphNode* GetConnectedNode() const;
};

DEFINE_SIMPLE_RTTI_CLASS( CBehaviorGraphAnimationInputSocket, CGraphSocket );

#ifndef NO_EDITOR_GRAPH_SUPPORT

class CBehaviorGraphAnimationInputSocketSpawnInfo : public GraphSocketSpawnInfo
{
public:
	CBehaviorGraphAnimationInputSocketSpawnInfo( const CName& name );
};

#endif

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphAnimationOutputSocket : public CGraphSocket
{
	DECLARE_RTTI_SIMPLE_CLASS( CBehaviorGraphAnimationOutputSocket  );

public:
	//! Can we connect to given socket
	virtual Bool CanConnectTo( CGraphSocket *otherSocket );
};

DEFINE_SIMPLE_RTTI_CLASS( CBehaviorGraphAnimationOutputSocket, CGraphSocket );

#ifndef NO_EDITOR_GRAPH_SUPPORT

class CBehaviorGraphAnimationOutputSocketSpawnInfo : public GraphSocketSpawnInfo
{
public:
	CBehaviorGraphAnimationOutputSocketSpawnInfo( const CName& name );
};

#endif

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphExactlyAnimationInputSocket : public CBehaviorGraphAnimationInputSocket
{
	DECLARE_RTTI_SIMPLE_CLASS( CBehaviorGraphExactlyAnimationInputSocket );

public:
	//! Can we connect to given socket
	virtual Bool CanConnectTo( CGraphSocket *otherSocket );

	//! Get socked connected to that input
	CBehaviorGraphAnimationOutputSocket* GetConnectedSocket() const; 

	//! Get node connected to that socket
	CBehaviorGraphNode* GetConnectedNode() const;
};

DEFINE_SIMPLE_RTTI_CLASS( CBehaviorGraphExactlyAnimationInputSocket, CBehaviorGraphAnimationInputSocket );

#ifndef NO_EDITOR_GRAPH_SUPPORT

class CBehaviorGraphExactlyAnimationInputSocketSpawnInfo : public GraphSocketSpawnInfo
{
public:
	CBehaviorGraphExactlyAnimationInputSocketSpawnInfo( const CName& name );
};

#endif

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphVariableInputSocket : public CGraphSocket
{
	DECLARE_RTTI_SIMPLE_CLASS( CBehaviorGraphVariableInputSocket );

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	//! Can we connect to given socket
	virtual Bool CanConnectTo( CGraphSocket *otherSocket );
#endif

	//! Get socked connected to that input
	CBehaviorGraphVariableOutputSocket* GetConnectedSocket() const; 

	//! Get node connected to that socket
	CBehaviorGraphValueNode* GetConnectedNode() const;
};

DEFINE_SIMPLE_RTTI_CLASS( CBehaviorGraphVariableInputSocket, CGraphSocket );

#ifndef NO_EDITOR_GRAPH_SUPPORT

class CBehaviorGraphVariableInputSocketSpawnInfo : public GraphSocketSpawnInfo
{
public:
	CBehaviorGraphVariableInputSocketSpawnInfo( const CName& name, Bool visible = true );
};

#endif

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphVariableOutputSocket : public CGraphSocket
{
	DECLARE_RTTI_SIMPLE_CLASS( CBehaviorGraphVariableOutputSocket );

public:
	//! Can we connect to given socket
	virtual Bool CanConnectTo( CGraphSocket *otherSocket );
};

DEFINE_SIMPLE_RTTI_CLASS( CBehaviorGraphVariableOutputSocket, CGraphSocket );

#ifndef NO_EDITOR_GRAPH_SUPPORT

class CBehaviorGraphVariableOutputSocketSpawnInfo : public GraphSocketSpawnInfo
{
public:
	CBehaviorGraphVariableOutputSocketSpawnInfo( const CName& name, bool visible = true );
};

#endif

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphVectorVariableInputSocket : public CGraphSocket
{
	DECLARE_RTTI_SIMPLE_CLASS( CBehaviorGraphVectorVariableInputSocket );

public:
	//! Can we connect to given socket
	virtual Bool CanConnectTo( CGraphSocket *otherSocket );

	//! Get socked connected to that input
	CBehaviorGraphVectorVariableOutputSocket* GetConnectedSocket() const; 

	//! Get node connected to that socket
	CBehaviorGraphVectorValueNode* GetConnectedNode() const;
};

DEFINE_SIMPLE_RTTI_CLASS( CBehaviorGraphVectorVariableInputSocket, CGraphSocket );

#ifndef NO_EDITOR_GRAPH_SUPPORT

class CBehaviorGraphVectorVariableInputSocketSpawnInfo : public GraphSocketSpawnInfo
{
public:
	CBehaviorGraphVectorVariableInputSocketSpawnInfo( const CName& name, Bool visible = true );
};

#endif

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphVectorVariableOutputSocket : public CGraphSocket
{
	DECLARE_RTTI_SIMPLE_CLASS( CBehaviorGraphVectorVariableOutputSocket );

public:
	//! Can we connect to given socket
	virtual Bool CanConnectTo( CGraphSocket *otherSocket );
};

DEFINE_SIMPLE_RTTI_CLASS( CBehaviorGraphVectorVariableOutputSocket, CGraphSocket );

#ifndef NO_EDITOR_GRAPH_SUPPORT

class CBehaviorGraphVectorVariableOutputSocketSpawnInfo : public GraphSocketSpawnInfo
{
public:
	CBehaviorGraphVectorVariableOutputSocketSpawnInfo( const CName& name, bool visible = true );
};

#endif

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphStateOutSocket : public CGraphSocket
{
	DECLARE_RTTI_SIMPLE_CLASS( CBehaviorGraphStateOutSocket );

public:
	//! Can we connect to given socket
	virtual Bool CanConnectTo( CGraphSocket *otherSocket );

	//! get number of connected transition nodes
	virtual Int32 GetNumConnectedTransitions() const;

	//! get connected transition node
	virtual CBehaviorGraphStateTransitionNode* GetConnectedTransition( Uint32 index ) const;
};

DEFINE_SIMPLE_RTTI_CLASS( CBehaviorGraphStateOutSocket, CGraphSocket );

#ifndef NO_EDITOR_GRAPH_SUPPORT

class CBehaviorGraphStateOutSocketSpawnInfo : public GraphSocketSpawnInfo
{
public:
	CBehaviorGraphStateOutSocketSpawnInfo( const CName& name, Bool visible = true );
};

#endif

class CBehaviorGraphStateInSocket : public CGraphSocket
{
	DECLARE_RTTI_SIMPLE_CLASS( CBehaviorGraphStateInSocket );

public:
	//! Can we connect to given socket
	virtual Bool CanConnectTo( CGraphSocket *otherSocket );
};

DEFINE_SIMPLE_RTTI_CLASS( CBehaviorGraphStateInSocket, CGraphSocket );

#ifndef NO_EDITOR_GRAPH_SUPPORT

class CBehaviorGraphStateInSocketSpawnInfo : public GraphSocketSpawnInfo
{
public:
	CBehaviorGraphStateInSocketSpawnInfo( const CName& name, Bool visible = true );
};

#endif

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphTransitionSocket : public CGraphSocket
{
	DECLARE_RTTI_SIMPLE_CLASS( CBehaviorGraphTransitionSocket );

public:
	//! Can we connect to given socket
	virtual Bool CanConnectTo( CGraphSocket *otherSocket );

	//! get node connected to this socket
	CBehaviorGraphStateNode* GetConnectedNode() const;
};

DEFINE_SIMPLE_RTTI_CLASS( CBehaviorGraphTransitionSocket, CGraphSocket );

#ifndef NO_EDITOR_GRAPH_SUPPORT

class CBehaviorGraphTransitionSocketSpawnInfo : public GraphSocketSpawnInfo
{
public:
	CBehaviorGraphTransitionSocketSpawnInfo( const CName& name, ELinkedSocketDirection direction );
};

#endif

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphMimicAnimationInputSocket : public CGraphSocket
{
	DECLARE_RTTI_SIMPLE_CLASS( CBehaviorGraphMimicAnimationInputSocket );

public:
	//! Can we connect to given socket
	virtual Bool CanConnectTo( CGraphSocket *otherSocket );

	//! Get socked connected to that input
	CBehaviorGraphMimicAnimationOutputSocket* GetConnectedSocket() const; 

	//! get node connected to this socket
	CBehaviorGraphNode* GetConnectedNode() const;
};

DEFINE_SIMPLE_RTTI_CLASS( CBehaviorGraphMimicAnimationInputSocket, CGraphSocket );

#ifndef NO_EDITOR_GRAPH_SUPPORT

class CBehaviorGraphMimicAnimationInputSocketSpawnInfo : public GraphSocketSpawnInfo
{
public:
	CBehaviorGraphMimicAnimationInputSocketSpawnInfo( const CName& name, Bool visible = true );
};

#endif

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphMimicAnimationOutputSocket : public CGraphSocket
{
	DECLARE_RTTI_SIMPLE_CLASS( CBehaviorGraphMimicAnimationOutputSocket );

public:
	//! Can we connect to given socket
	virtual Bool CanConnectTo( CGraphSocket *otherSocket );
};

DEFINE_SIMPLE_RTTI_CLASS( CBehaviorGraphMimicAnimationOutputSocket, CGraphSocket );

#ifndef NO_EDITOR_GRAPH_SUPPORT

class CBehaviorGraphMimicAnimationOutputSocketSpawnInfo : public GraphSocketSpawnInfo
{
public:
	CBehaviorGraphMimicAnimationOutputSocketSpawnInfo( const CName& name, bool visible = true );
};

#endif
