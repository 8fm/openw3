#pragma once
#include "inputManager.h"


class IInputListener
{
public:
	// Returns true to unregister from input manager
	virtual Bool OnGameInputEvent( const SInputAction & action ) = 0;
	
protected:

	virtual ~IInputListener() {}
};


class CScriptInputListener : public IInputListener
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_SmallObjects, MC_Engine );

	THandle< IScriptable >	m_listener;
	CName					m_eventName;
	CName					m_actionName;
public:

	CScriptInputListener( IScriptable *	_listener, CName _eventName, CName _actionName ) : m_listener( _listener ), m_eventName( _eventName ), m_actionName(_actionName)
	{}

	virtual ~CScriptInputListener()
	{}
	
	RED_INLINE THandle< IScriptable > GetHandle()		{ return m_listener; }
	RED_INLINE CName					GetActionName()	{ return m_actionName; }

	virtual Bool OnGameInputEvent( const SInputAction & action );


};