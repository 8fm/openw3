/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "dynarray.h"
#include "memory.h"
#include "handleMap.h"
#include "referencable.h"

class CScriptThreadSerializer;
class CScriptStackFrame;
class IScriptable;

/// Script thread, a latent execution of function code
class CScriptThread : public IReferencable
{
	DECLARE_RTTI_SIMPLE_CLASS_WITH_ALLOCATOR( CScriptThread, MC_ScriptObject );

	friend class CScriptingSystem;
	friend class CFunction;

public:
	typedef TDynArray< CScriptStackFrame*, MC_ScriptObject >	 TFrameStack;
	typedef THandle< IScriptable >								TScriptableHandle;

	class IListener
	{
	public:
		virtual ~IListener(){}
		virtual void OnScriptThreadKilled( CScriptThread*, Bool ) {}
		virtual String GetDebugName() const = 0;
	};

protected:	
	TFrameStack							m_frames;			//!< Current stack frames, top one is the active one
	TFrameStack							m_deletedFrames;	//!< Deleted script frames
	TScriptableHandle					m_context;			//!< Thread context
	String								m_contextName;		//!< Name of the thread context
	Bool								m_contextUsed;		//!< Thread has been run in context
	const Uint8*						m_lastYield;		//!< Last position we were yielding at
	Bool								m_isKilled;			//!< Thread was killed
	Bool								m_isYielding;		//!< Thread is yielding execution
	Uint32								m_id;				//!< Thread ID
	Float								m_timer;			//!< Internal timer of some kind, reset to 0 every time latent function starts executing
	Uint32								m_latentIndex;		//!< Generated latent index related to this thread
	void*								m_returnValue;		//!< Value returned to the caller	
	IListener*							m_listener;			//!< Thread end listener
	Uint32								m_totalTicks;		//!< Total number of non yielding ticks
	Float								m_totalTime;		//!< Total time this is ticked
	CScriptThreadSerializer*			m_serializer;		//!< Thread state serializer

private:
	static Uint32							s_latentIndex;		//!< Internal latent function index

public:
	//! Get thread ID
	RED_INLINE Uint32 GetID() const { return m_id; }

	//! Get context
	RED_INLINE const TScriptableHandle& GetContext() const { return m_context; }

	//! Get current callstack
	RED_INLINE const TFrameStack& GetFrames() const { return m_frames; }

	//! Get top stack frame
	RED_INLINE CScriptStackFrame* GetTopFrame() const { return ( m_frames.Size() > 0 ) ? m_frames.Back() : NULL; }

	//! Is this thread killed ?
	RED_INLINE Bool IsKilled() const { return m_isKilled; }

	//! Get time spent on current latent function
	RED_INLINE Float GetCurrentLatentTime() const { return m_timer; }

	//! Get latent index of this thread
	RED_INLINE Uint32 GetLatentIndex() const { return m_latentIndex; }

	//! Get the total number of non yielding ticks for this thread
	RED_INLINE Uint32 GetTotalTicks() const { return m_totalTicks; }

	//! Get the total time this thread was ticked
	RED_INLINE Float GetTotalTime() const { return m_totalTime; }

protected:
	CScriptThread( Uint32 id, IScriptable* context, const CFunction* function, CScriptStackFrame& topFrame, void * returnValue = NULL );
	~CScriptThread();

public:
	//! Install thread listener
	void SetListener( IListener * listener );

	//! Compare listeners
	Bool CompareListener( IListener* listener ) const;

	//! Generate latent function index
	Uint32 GenerateLatentIndex();

	//! Advance, returns true when thread finished
	Bool Advance( Float timeDelta );

	//! Kill this thread
	void ForceKill();

	//! Yield execution
	void ForceYield();

	//! Delete pending frames
	void DeletePendingFrames();

	//! Add frame to script thread
	void PushFrame( CScriptStackFrame * frame );

	//! Enables script thread serialization
	void EnableSerialization( bool enable = true );

	//! Get an interface implementation capable of serializing a SavePoint contents.
	CScriptThreadSerializer* QuerySerializer();

	//! Get listener's name - for debug
	String GetListenerName() const;

private:
	CScriptThread() {}; // not used directly
	CScriptThread& operator=( const CScriptThread& rhs );
	CScriptThread( const CScriptThread& );
};

BEGIN_NOCOPY_CLASS_RTTI( CScriptThread );
	PARENT_CLASS( IReferencable );
END_CLASS_RTTI();
