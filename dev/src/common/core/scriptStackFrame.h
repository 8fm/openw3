/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "math.h"
#include "handleMap.h"
#include "object.h"

class IScriptable;
class CFunction;
class CProperty;
class CScriptThread;

#ifndef NO_SCRIPT_DEBUG
	#define	SCRIPT_DUMP_STACK_TO_LOG( stack ) stack.DumpToLog()
#else
	#define	SCRIPT_DUMP_STACK_TO_LOG( stack )
#endif

/// Stack frame for script execution
class CScriptStackFrame
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_ScriptObject );

public:
	// Param info
	struct OutputParam
	{
		IRTTIType*	m_type;
		void*		m_dataOffset; 
		void*		m_localOffset;
	};

private:
	IScriptable*			m_rawContext;		//!< 'this', base object context - stored only for performance reasons
	THandle< IScriptable >	m_context;			//!< 'this', base object context

public:
	CScriptStackFrame*		m_parent;			//!< Parent stack frame ( for building call stack )
	const CFunction*		m_function;			//!< Function we are in
	Uint8*					m_locals;			//!< Pointer to local function variables
	Uint8*					m_params;			//!< Pointer to function params
	const Uint8*			m_code;				//!< Code pointer
	Uint32					m_line;				//!< Line we are in
	Uint8					m_debugFlags;		//!< Local debug flags
	CScriptThread*			m_thread;			//!< Thread this frame is working under, NULL for main thread
	OutputParam				m_outputParams[8];	//!< Where to save out params when returning from the latent function
	Uint32					m_numOutputParams;	//!< Number of output params in child function
	void*					m_parentResult;		//!< Pointer to write result of this function call
	void*					m_perfData;			//!< Performance capture data in case of function profiling

public:
	//! Get stack frame context
#if defined( RED_PLATFORM_WINPC ) && !defined( RED_FINAL_BUILD )
	RED_INLINE IScriptable* GetContext() const { return m_context.Get(); }
#else
	RED_INLINE IScriptable* GetContext() const { return m_rawContext; }
#endif

public:
	static void*			s_offset;			//!< L-value data offset

public:
	CScriptStackFrame( CScriptStackFrame* parentFrame, IScriptable *context, const CFunction *function, void *locals, void *params );

	//! Execute one code token
#if defined( RED_PLATFORM_WINPC ) && !defined( RED_FINAL_BUILD )
	void Step( IScriptable *context, void* result );
#else
	RED_INLINE void Step( IScriptable* context, void* result )
	{
		// Read native function index
		Uint8 functionIndex = *m_code++;

		// Get the function
		TNativeGlobalFunc function = CScriptNativeFunctionMap::GetGlobalNativeFunction( functionIndex );

		// Execute code
		( *function )( context, *this, result );
	}
#endif

	//! Get entry function
	const CFunction* GetEntryFunction() const;

#ifndef NO_SCRIPT_DEBUG
	//! Dump to log
	void DumpToLog();

	//! Dump to string
	Uint32 DumpToString( Char* str, Uint32 strSize );
	Uint32 DumpRawToAnsiString( AnsiChar* stf, Uint32 strSize );

	//! Dump top to string
	size_t DumpTopToString( Char* str, Uint32 strSize );
	Uint32 DumpRawTopToAnsiString( AnsiChar* str, Uint32 strSize );

	static Uint32 DumpScriptStack( Char* str, Uint32 strSize );

	static Uint32 GetScriptStackRaw( AnsiChar* buffer, Uint32 bufferSize );

private:
	static String s_tempPropertyValueBuffer;
	static const Uint32 SCRIPTS_STACK_FRAME_ARRAY_MAX_SIZE = 4096;
	static CScriptStackFrame* GScriptStackFrameArray[ SCRIPTS_STACK_FRAME_ARRAY_MAX_SIZE ];
	static Uint32 GScriptStackFrameArraySize;
#endif

public:
	//! Read embedded data from code stream
	template < typename T >
	RED_INLINE T Read()
	{
		const T *data = reinterpret_cast< const T* >( m_code );
		m_code += sizeof(T);
		return *data;
	}

	RED_INLINE void Read( String& result )
	{
		result = *((String*)m_code);
		m_code += sizeof(String);
	}
};

//! Read embedded data from code stream, special cases. Problem with misalignment on the X360.
template <>
RED_INLINE float CScriptStackFrame::Read()
{
	const float *data = reinterpret_cast< const float* >( m_code );
	m_code += sizeof(float);
	float _aligned_data;
	unsigned char* _misaligned_data_src_ptr = (unsigned char*)data;
	unsigned char* _aligned_data_dst_ptr = (unsigned char*)&_aligned_data;
	_aligned_data_dst_ptr[0] = _misaligned_data_src_ptr[0];
	_aligned_data_dst_ptr[1] = _misaligned_data_src_ptr[1];
	_aligned_data_dst_ptr[2] = _misaligned_data_src_ptr[2];
	_aligned_data_dst_ptr[3] = _misaligned_data_src_ptr[3];
	return _aligned_data;
}

template <>
RED_INLINE int CScriptStackFrame::Read()
{
	const int *data = reinterpret_cast< const int* >( m_code );
	m_code += sizeof(int);
	int _aligned_data;
	unsigned char* _misaligned_data_src_ptr = (unsigned char*)data;
	unsigned char* _aligned_data_dst_ptr = (unsigned char*)&_aligned_data;
	_aligned_data_dst_ptr[0] = _misaligned_data_src_ptr[0];
	_aligned_data_dst_ptr[1] = _misaligned_data_src_ptr[1];
	_aligned_data_dst_ptr[2] = _misaligned_data_src_ptr[2];
	_aligned_data_dst_ptr[3] = _misaligned_data_src_ptr[3];
	return _aligned_data;
}

template <>
RED_INLINE Vector CScriptStackFrame::Read()
{
	const Vector *data = reinterpret_cast< const Vector* >( m_code );
	m_code += sizeof(Vector);
	Vector _aligned_data;
	unsigned char* _misaligned_data_src_ptr = (unsigned char*)data;
	unsigned char* _aligned_data_dst_ptr = (unsigned char*)&_aligned_data;
	_aligned_data_dst_ptr[0] = _misaligned_data_src_ptr[0];
	_aligned_data_dst_ptr[1] = _misaligned_data_src_ptr[1];
	_aligned_data_dst_ptr[2] = _misaligned_data_src_ptr[2];
	_aligned_data_dst_ptr[3] = _misaligned_data_src_ptr[3];
	_aligned_data_dst_ptr[4] = _misaligned_data_src_ptr[4];
	_aligned_data_dst_ptr[5] = _misaligned_data_src_ptr[5];
	_aligned_data_dst_ptr[6] = _misaligned_data_src_ptr[6];
	_aligned_data_dst_ptr[7] = _misaligned_data_src_ptr[7];
	_aligned_data_dst_ptr[8] = _misaligned_data_src_ptr[8];
	_aligned_data_dst_ptr[9] = _misaligned_data_src_ptr[9];
	_aligned_data_dst_ptr[10] = _misaligned_data_src_ptr[10];
	_aligned_data_dst_ptr[11] = _misaligned_data_src_ptr[11];
	_aligned_data_dst_ptr[12] = _misaligned_data_src_ptr[12];
	_aligned_data_dst_ptr[13] = _misaligned_data_src_ptr[13];
	_aligned_data_dst_ptr[14] = _misaligned_data_src_ptr[14];
	_aligned_data_dst_ptr[15] = _misaligned_data_src_ptr[15];
	return _aligned_data;
}

// Grab function parameter from code stream
#define GET_PARAMETER( _type, _var, _def )		\
	_type _var = _def;							\
	stack.Step( stack.GetContext(), &_var );

// Grab optional function parameter from code stream
#define GET_PARAMETER_OPT( _type, _var, _def )		\
	_type _var = _def;								\
	stack.Step( stack.GetContext(), &_var );

// Grab reference to function parameter from code stream
#define GET_PARAMETER_REF( _type, _var, _def )		\
	_type _var##T = _def;							\
	stack.Step( stack.GetContext(), &_var##T );		\
	_type& _var = stack.s_offset ? *(_type*)stack.s_offset : _var##T;

#define FINISH_PARAMETERS							\
	stack.m_code++;

#define RETURN_INT(_val) \
{ Int32 retVal = _val; if ( result ) *(Int32*)result = retVal; }

#define RETURN_UINT64(_val) \
{ Uint64 retVal = _val; if( result ) *(Uint64*)result = retVal; }

#define RETURN_ENUM RETURN_INT

#define RETURN_BYTE(_val) \
{ Uint8 retVal = _val; if ( result ) *(Uint8*)result = retVal; }

#define RETURN_BOOL(_val) \
{ Bool retVal = _val; if ( result ) *(Bool*)result = retVal; }

#define RETURN_FLOAT(_val) \
{ Float retVal = _val; if ( result ) *(Float*)result = retVal; }

#define RETURN_SHORT(_val) \
{ Int16 retVal = _val; if ( result ) *(Int16*)result = retVal; }

#define RETURN_STRING(_val) \
{ String retVal = _val; if ( result ) *(String *)result = retVal; }

#define RETURN_NAME(_val) \
{ CName retVal = _val; if ( result ) *(CName *)result = retVal; }

#define RETURN_OBJECT(_val) \
{ THandle<IScriptable> retVal = _val; if ( result ) *(THandle<IScriptable>*)result = retVal; }

#define RETURN_HANDLE(_type, _val) \
{ THandle<_type> retVal = _val; if ( result ) *(THandle<_type>*)result = retVal; }

#define RETURN_STRUCT(_type, _val) \
{ _type retVal = _val; if (result) *(_type *)result = retVal; }

#define RETURN_VOID() \
{ RED_UNUSED( result ); }
