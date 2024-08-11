/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "functionFlags.h"
#include "scriptCompiledCode.h"
#include "scriptNativeFunctionMap.h"
#include "dynarray.h"
#include "names.h"
#include "rttiType.h"
#include "profiler.h"

class IScriptable;
class CScriptCompiler;
class CScriptStackFrame;
class CFunction;
class CProperty;

/// Function performance data
class FuncPerfData
{
public:
	CFunction*					m_function;		//!< Function this data is for
	Uint32						m_numCalls;		//!< Number of calls to this function
	Uint64						m_numTicks;		//!< Time spent in this function
	Uint64						m_numExTicks;	//!< Time spent in function called by this function
	Uint64						m_recusion;		//!< Recursion level
	TDynArray< CFunction* >		m_called;		//!< Other functions called from this function
	CName						m_nestedMarker;	//!< Marks a function that is called to avoid double addition of nested functions
												//	example: AbsF( AngleDistance( angle1, angle2 ) ); - ticks of AngleDistance() would be added twice
												//	since it is included inside AbsF()
	Uint8						m_markerCounter;//!< Need this in case a function is nested in itself ex:{ MaxF( MaxF(a, b), c ); }

	FuncPerfData( CFunction* func )
		: m_function( func )
		, m_numCalls(0)
		, m_numTicks(0)
		, m_numExTicks(0)
		, m_recusion(0)
		, m_nestedMarker( CName::NONE )
		, m_markerCounter(0)
	{};
};

/// RTTI Function definition
class CFunction : public IRTTIBaseObject
{
	friend class CScriptCompiler;
	friend class CScriptFunctionCompiler;
	friend class CScriptSyntaxNode;
	friend class FunctionCallProfiler;
	friend class CRTTISystem;
	friend class CRTTISerializer;
	friend class CScriptCodeRewriterLoader;

protected:
	CClass*						m_class;			//!< Class this function is defined in
	CName						m_name;				//!< Name of the function
	Uint32						m_flags;			//!< Function flags
	CProperty*					m_returnProperty;	//!< If not NULL function returns value via this property
	TDynArray< CProperty*, MC_RTTI >		m_parameters;		//!< Function input parameters
	TDynArray< CProperty*, MC_RTTI >		m_localVars;		//!< Local variables
	TDynArray< CProperty*, MC_RTTI >		m_paramsToDestroy;	//!< Function parameters that needs to be cleaned up
	TDynArray< CProperty*, MC_RTTI >		m_localsToDestroy;	//!< Local function variables that needs to be cleaned up
	Uint32						m_size;				//!< Size of data ( for scripted function )
	CScriptCompiledCode			m_code;				//!< Compiled code ( for scripted function )
	Uint32						m_nativeFunction;	//!< Index to mapped native function
	const CFunction*			m_superFunction;	//!< Virtual function with the same name and params in base class
	FuncPerfData*				m_perfData;			//!< Performance data

#ifdef NEW_PROFILER_ENABLED
	NewRedProfiler::InstrumentedFunction* m_profilerHandle;
#endif

public:
	//! Get the function name
	RED_INLINE const CName& GetName() const { return m_name; }

	//! Get function flags
	RED_INLINE Uint32 GetFlags() const { return m_flags; }

	//! Get class this function is defined in
	RED_INLINE CClass* GetClass() const { return m_class; }

	//! Get size of needed stack data
	RED_INLINE Uint32 GetStackSize() const { return m_size; }

	//! Is this a native function ?
	RED_INLINE Bool IsNative() const { return 0 != ( m_flags & FF_NativeFunction ); }

	//! Is this a static function ?
	RED_INLINE Bool IsStatic() const { return 0 != ( m_flags & FF_StaticFunction ); }

	//! Is this an operator function ?
	RED_INLINE Bool IsOperator() const { return 0 != ( m_flags & FF_OperatorFunction ); }

	//! Is this an exported function ?
	RED_INLINE Bool IsExported() const { return 0 != ( m_flags & FF_ExportedFunction ); }

	//! Is this final function ?
	RED_INLINE Bool IsFinal() const { return 0 != ( m_flags & FF_FinalFunction ); }

	//! Is this a private function ?
	RED_INLINE Bool IsPrivate() const { return 0 != ( m_flags & FF_PrivateFunction ); }

	//! Is this a protected function ?
	RED_INLINE Bool IsProtected() const { return 0 != ( m_flags & FF_ProtectedFunction ); }

	//! Is this a public function ?
	RED_INLINE Bool IsPublic() const { return 0 != ( m_flags & FF_PublicFunction ); }

	//! Is this an event function ?
	RED_INLINE Bool IsEvent() const { return 0 != ( m_flags & FF_EventFunction ); }

	//! Is this a latent function ?
	RED_INLINE Bool IsLatent() const { return 0 != ( m_flags & FF_LatentFunction ); }

	//! Is this a state entry function ?
	RED_INLINE Bool IsEntry() const { return 0 != ( m_flags & FF_EntryFunction ); }

	//! Is this an exec function ?
	RED_INLINE Bool IsExec() const { return 0 != ( m_flags & FF_ExecFunction ); }

	//! Is this a scene function
	RED_INLINE Bool IsScene() const { return 0 != ( m_flags & FF_SceneFunction ); }

	//! Is this a scene function
	RED_INLINE Bool IsQuest() const { return 0 != ( m_flags & FF_QuestFunction ); }

	//! Is this a scene function
	RED_INLINE Bool IsReward() const { return 0 != ( m_flags & FF_RewardFunction ); }

	//! Is this a timer function ?
	RED_INLINE Bool IsTimer() const { return 0 != ( m_flags & FF_TimerFunction ); }

	//! Is this a cleanup function ?
	RED_INLINE Bool IsCleanup() const { return 0 != ( m_flags & FF_CleanupFunction ); }
	
	//! Is this function missing it's definition (meaning it's just a declaration, valid for native functions)
	RED_INLINE Bool HasUndefinedBody() const { return 0 != ( m_flags & FF_UndefinedBody ); }

	//! Get index of mapped native function
	RED_INLINE Uint32 GetNativeFunctionIndex() const { return m_nativeFunction; }

	//! Get the base function in super class
	RED_INLINE const CFunction* GetSuperFunction() const { return m_superFunction; }

	//! Get function code ( for script functions )
	RED_INLINE const CScriptCompiledCode& GetCode() const { return m_code; }

	//! Get function code ( for script functions )
	RED_INLINE CScriptCompiledCode& GetCode() { return m_code; }

	//! Get number of function parameters
	RED_INLINE size_t GetNumParameters() const { return m_parameters.Size(); }

	//! Get function parameter
	RED_INLINE CProperty* GetParameter( Uint32 index ) const { return m_parameters[ index ]; }

	//! Get number of local variables
	RED_INLINE size_t GetNumLocals() const { return m_localVars.Size(); }

	//! Get local variable
	RED_INLINE CProperty* GetLocal( Uint32 index ) const { return m_localVars[ index ]; }

	//! Get function return value
	RED_INLINE CProperty* GetReturnValue() const { return m_returnProperty; }

public:
	CFunction( CClass* parentClass, CName name, Uint32 flags );
	CFunction( CClass* parentClass, CName name, TNativeFunc func );
	CFunction( CName name, TNativeGlobalFunc func );
	~CFunction();

	//! Clear script related data
	void ClearScriptData();

	//! Get all function properties
	void GetProperties( TDynArray< CProperty*, MC_RTTI >& properties ) const;

	//! Call function, used from script
	Bool Call( IScriptable* context, CScriptStackFrame& stackFrame, void* result ) const;

	//! Call function, used from C++ code
	Bool Call( IScriptable* context, void* params, void* result ) const;

	//! Call function using list of parameters
	Bool Call( IScriptable* context, const TDynArray< String >& parameters, String* result ) const;

	//! Find property by name
	CProperty* FindProperty( CName propName ) const;

public:
	//! Create function entry frame
	CScriptStackFrame* CreateEntryFrame( IScriptable* context, CScriptStackFrame& parentStackFrame ) const;

	//! Destroy frame
	void DestroyEntryFrame( CScriptStackFrame* frame ) const;

protected:
	//! Create function entry frame that will emit output variables to parentStackFrame
	Bool PrepareEntryFrameWithOutputs( IScriptable* context, CScriptStackFrame& parentStackFrame, Uint8 * newStack ) const;

	//! Add native return value parameter
	void DefineNativeReturnType( CName typeName );

	//! Add native function parameter
	void DefineNativeParameter( CName typeName, Uint32 flags=0 );

	//! Calculate function data layout of properties
	void CalcDataLayout();

	//! Set compiled code
	void InitailizerCode( const CScriptCodeGenerator& generator );
};

class FunctionCallProfiler
{
private:
	CFunction*			m_function;
	Bool				m_saveTimer;
	Uint64				m_timerStart;
	Uint64				m_calledFunctionTicks;
	CScriptStackFrame*	m_parentStackFrame;
#ifdef NEW_PROFILER_BREAKPOINTS_ENABLED
	Uint64				m_newProfilerStartTime;
#endif //NEW_PROFILER_BREAKPOINTS_ENABLED
public:
	FunctionCallProfiler( CFunction* func, CScriptStackFrame* parentStackFrame );
	~FunctionCallProfiler();
};

#ifdef NEW_PROFILER_ENABLED

#define SCRIPT_PROFILER_MAX_PROFILES_COUNT PROFILER_MAX_PROFILES_COUNT/3

class CScriptProfilerManager
{
private:
	struct  SScriptProfileHandle
	{
		StringAnsi m_functionName;
		NewRedProfiler::InstrumentedFunction* m_instrumentedFunction;
	};

	SScriptProfileHandle			m_scriptHandles[SCRIPT_PROFILER_MAX_PROFILES_COUNT];
	Red::Threads::CAtomic<Uint32>	m_scriptHandlesCounter;
	Red::Threads::CAtomic<Bool> 	m_profileFunctionCalls;
		
public:

	CScriptProfilerManager();
	~CScriptProfilerManager();

	NewRedProfiler::InstrumentedFunction* RegisterScriptInstrFunc( const CFunction* func );

	// can not be set during profile session
	// enable/disable during profile session could cause inconsistent profile data in profile buffer
	void EnableProfileFunctionCalls( Bool enable );
	Bool IsEnableProfileFunctionCalls();
};

typedef TSingleton<CScriptProfilerManager> SScriptProfilerManager;

#endif
