/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"
#include "flashScriptFunctionCalling.h"
#include "../engine/flashValue.h"
#include "../engine/guiGlobals.h"

namespace // anonymous
{
#ifdef CHECK_SCRIPT_FUNCTION_CALLS
	Bool CheckFlashScriptEventCall( IScriptable* context, const CFunction* event, const TDynArray< CFlashValue >& args )
	{
		RED_FATAL_ASSERT( context, "" );
		RED_FATAL_ASSERT( event, "" );

		Bool isValidEvent = true;

		const CName eventName = event->GetName();

		if ( ! CheckFunction( context, event, args.Size() ) )
		{
			GUI_ERROR( TXT("Flash::CallEvent - event '%ls' failed basic script check and not called. See the ScriptApi log channel for details"), eventName.AsString().AsChar() );
			return false;
		}

		const Uint32 numArgs = args.Size();
		for ( Uint32 i = 0; i < numArgs; ++i )
		{
			const CProperty* param = event->GetParameter(i);
			const Uint32 paramSize = param->GetType()->GetSize();

			const CFlashValue& flashValue = args[ i ];
			switch( flashValue.GetFlashValueType() )
			{
			case FVT_Int:
				{
					IRTTIType* objectType = nullptr;
					switch ( paramSize )
					{
					case sizeof(Int8):
						objectType = ::GetTypeObject<Int8>();
						break;
					case sizeof(Int16):
						objectType = ::GetTypeObject<Int16>();
						break;
					default:
						objectType = ::GetTypeObject<Int32>();
						break;
					}
					
					if ( ! CheckFunctionParameter( event, i, objectType ) )
					{
						GUI_ERROR( TXT("Flash::CallEvent - event '%ls', Flash int parameter #'%u'. Failed script check and not called. See the ScriptApi log channel for details"), eventName.AsString().AsChar(), i );
						isValidEvent = false;
					}
				}
				break;

			case FVT_UInt:
				{
					const CName paramName = param->GetType()->GetName();

					IRTTIType* objectType = nullptr;
					switch ( paramSize )
					{
					case sizeof(Int8):
						objectType = ::GetTypeObject<Int8>();
						break;
					case sizeof(Int16):
						objectType = ::GetTypeObject<Int16>();
						break;
					default:
						objectType = ::GetTypeObject<Int32>();
						break;
					}

					const Bool silentCheck = true;
					if
					(	! CheckFunctionParameter( event, i, objectType, silentCheck ) &&
						! CheckFunctionParameter( event, i, ::GetTypeObject<CName>(), silentCheck ) &&
						! CheckFunctionParameter( event, i, ::GetTypeObject<SItemUniqueId>(), silentCheck )
					)
					{
						(void)CheckFunctionParameter( event, i, objectType );
						(void)CheckFunctionParameter( event, i, ::GetTypeObject<CName>() );
						(void)CheckFunctionParameter( event, i, ::GetTypeObject<SItemUniqueId>() );
						GUI_ERROR( TXT("Flash::CallEvent - event '%ls', Flash uint parameter #'%u'. Failed script check and not called. See the ScriptApi log channel for details"), eventName.AsString().AsChar(), i );
						isValidEvent = false;
					}
				}
				break;

			case FVT_Number:
				{
					if ( ! CheckFunctionParameter( event, i, ::GetTypeObject<Float>() ) )
					{
						GUI_ERROR( TXT("Flash::CallEvent - event '%ls', Flash Number parameter #'%u'. Failed script check and not called. See the ScriptApi log channel for details"), eventName.AsString().AsChar(), i );
						isValidEvent = false;
					}
				}
				break;

			case FVT_Bool:
				{
					if ( ! CheckFunctionParameter( event, i, ::GetTypeObject<Bool>() ) )
					{
						GUI_ERROR( TXT("Flash::CallEvent - event '%ls', Flash Boolean parameter #'%u'. Failed script check and not called. See the ScriptApi log channel for details"), eventName.AsString().AsChar(), i );
						isValidEvent = false;
					}
				}
				break;

			case FVT_String:
				{
					if ( ! CheckFunctionParameter( event, i, ::GetTypeObject<String>() ) )
					{
						GUI_ERROR( TXT("Flash::CallEvent - event '%ls', Flash String parameter #'%u'. Failed script check and not called. See the ScriptApi log channel for details"), eventName.AsString().AsChar(), i );
						isValidEvent = false;
					}
				}
				break;

			case FVT_Null:			/* fall through */
			case FVT_Undefined:
				{
					GUI_ERROR( TXT("Flash::CallEvent - event '%ls', parameter #'%u'. Flash null or undefined instead of a strongly typed value. Event not called. One possiblity is passing null instead of an empty string"), eventName.AsString().AsChar(), i );
					isValidEvent = false;
				}
				break;

			default:
				{
					GUI_ERROR( TXT("Flash::CallEvent - event '%ls', parameter #'%u'. Unknown Flash type. Event not called"), eventName.AsString().AsChar(), i );
					isValidEvent = false;
				}
				break;
			}
		}

		return isValidEvent;
	}
#endif // CHECK_SCRIPT_FUNCTION_CALLS

#ifdef RED_LOGGING_ENABLED
	struct SScopedRecurseCheck: Red::NonCopyable
	{
		static THashMap< IScriptable*, TDynArray<CName, MC_Debug>, DefaultHashFunc< IScriptable* >, DefaultEqualFunc< IScriptable* >, MC_Debug > s_perContextRecurseCheck;

		IScriptable* m_context;

		void DumpEventChain( TDynArray< CName, MC_Debug >& eventChain )
		{
			GUI_WARN( TXT("==================") );
			GUI_WARN( TXT("Calling multiple Flash events on the same object in one stack frame: ")
			TXT("Flash->Witchersript->Flash->Witcherscript etc. ")
			TXT("Please consider making your game logic more event driven and easier to understand.") );
			GUI_WARN( TXT("WitcherScript event chain for context '%ls'"), m_context->GetFriendlyName().AsChar() );
			for ( Uint32 i = 0; i < eventChain.Size(); ++i )
			{
				GUI_WARN( TXT("->\t'%ls'"), eventChain[i].AsString().AsChar() );
			}
			GUI_WARN( TXT("==================") );
		}

		SScopedRecurseCheck( IScriptable* context, CName eventName )
			: m_context( context )
		{
			RED_FATAL_ASSERT( ::SIsMainThread(), "" );
			RED_FATAL_ASSERT( m_context, "" );

			Bool wasRecursive = false;
			TDynArray<CName, MC_Debug>* pEventChain = s_perContextRecurseCheck.FindPtr( m_context );
			if ( ! pEventChain )
			{
				s_perContextRecurseCheck.Insert( m_context, TDynArray<CName, MC_Debug>() );
				pEventChain = s_perContextRecurseCheck.FindPtr( m_context );
			}
			else
			{
				wasRecursive = true;
			}

			pEventChain->PushBack( eventName );

			if ( wasRecursive )
			{
				DumpEventChain( *pEventChain );
			}
		}

		~SScopedRecurseCheck()
		{
			RED_FATAL_ASSERT( ::SIsMainThread(), "" );

			TDynArray<CName,MC_Debug>* pEventChain = s_perContextRecurseCheck.FindPtr( m_context );
			RED_FATAL_ASSERT( pEventChain, "" );
			RED_FATAL_ASSERT( ! pEventChain->Empty(), "" );
			pEventChain->EraseFast( pEventChain->End() - 1 );
			if ( pEventChain->Empty() )
			{
				s_perContextRecurseCheck.Erase( m_context );
			}
		}
	};

	THashMap< IScriptable*, TDynArray<CName, MC_Debug>, DefaultHashFunc< IScriptable* >, DefaultEqualFunc< IScriptable* >, MC_Debug > SScopedRecurseCheck::s_perContextRecurseCheck;

#endif // RED_LOGGING_ENABLED

} // namespace anonymous

namespace Flash
{
	Bool CallEvent( IScriptable* context, CName eventName, const TDynArray< CFlashValue >& args )
	{
		RED_FATAL_ASSERT( ::SIsMainThread(), "Flash::CallEvent called from off the main thread" );

#ifdef RED_LOGGING_ENABLED
		// Per context because it's a bit too strict to just have it per anything
		SScopedRecurseCheck scopedRecurseCheck( context, eventName );
#endif // RED_LOGGING_ENABLED

		if ( ! context )
		{
			GUI_ERROR( TXT("Flash::CallEvent - no context object for event '%ls'."), eventName.AsString().AsChar() );
			return false;
		}

		const CFunction* event = nullptr;
		if ( ! FindFunction( context, eventName, event ) || ( event && ! event->IsEvent() ) )
		{
			GUI_ERROR( TXT("Flash::CallEvent - event '%ls' not found."), eventName.AsString().AsChar() );
			return false;
		}

#ifdef CHECK_SCRIPT_FUNCTION_CALLS
		if ( ! CheckFlashScriptEventCall( context, event, args ) )
		{
			return false;
		}
#endif

		const Uint32 numArgs = args.Size();
		const Uint32 scriptStackSize = event->GetStackSize();
		Uint8* scriptStack  = (Uint8*) RED_ALLOCA( scriptStackSize );

		Bool canCallEvent = true;
		// Create params
		if ( scriptStackSize != 0 )
		{
			for ( Uint32 i = 0; i < numArgs; ++i )
			{
				const CProperty* param = event->GetParameter(i);
				void* paramMem = param->GetOffsetPtr(scriptStack);
				const Uint32 paramSize = param->GetType()->GetSize();

				const CFlashValue& flashValue = args[ i ];

				switch( flashValue.GetFlashValueType() )
				{
					case FVT_Int:
					{
						// Because our script enums are variable length again
						switch ( paramSize )
						{
						case sizeof(Int8):
							*((Int8*)paramMem) = (Int8)flashValue.GetFlashInt();
							break;
						case sizeof(Int16):
							*((Int16*)paramMem) = (Int16)flashValue.GetFlashInt();
							break;
						case sizeof(Int32):
							*((Int32*)paramMem) = (Int32)flashValue.GetFlashInt();
							break;
						case sizeof(Int64):
							*((Int64*)paramMem) = (Int64)flashValue.GetFlashInt();
							break;
						default:
							RED_FATAL( "Unexpected integer paramSize %u", paramSize );
							break;
						}
					}
					break;

					case FVT_UInt:
					{
						const CName paramName = param->GetType()->GetName();
						
						if ( paramName == ::GetTypeName< SItemUniqueId >() )
						{
							RED_FATAL_ASSERT( paramSize == sizeof(SItemUniqueId), "Sizeof SItemUniqueId mismatch. Paramsize=%u, sizeof(SItemUniqueId)=%u", paramSize, sizeof(SItemUniqueId) );
							new (paramMem) SItemUniqueId( flashValue.GetFlashUInt() );
						}
						else if ( paramName == ::GetTypeName< CName >() )
						{
							RED_FATAL_ASSERT( paramSize == sizeof(CName), "Sizeof CName mismatch. Paramsize=%u, sizeof(CName)=%u", paramSize, sizeof(CName) );
							new (paramMem) CName( flashValue.GetFlashUInt() );
						}
						else
						{
							// Because our script enums are variable length again
							switch ( paramSize )
							{
							case sizeof(Int8):
								*((Int8*)paramMem) = (Int8)flashValue.GetFlashUInt();
								break;
							case sizeof(Int16):
								*((Int16*)paramMem) = (Int16)flashValue.GetFlashUInt();
								break;
							case sizeof(Int32):
								*((Int32*)paramMem) = (Int32)flashValue.GetFlashUInt();
								break;
							case sizeof(Int64):
								*((Int64*)paramMem) = (Int64)flashValue.GetFlashUInt();
								break;
							default:
								RED_FATAL( "Unexpected integer paramSize %u", paramSize );
								break;
							}
						}
					}
					break;

					case FVT_Number:
					{
						RED_FATAL_ASSERT( paramSize == sizeof(Float), "Sizeof float mismatch. Paramsize=%u, sizeof(Float)=%u", paramSize, sizeof(Float) );
						*((Float*)paramMem) = (Float)flashValue.GetFlashNumber();
					}
					break;

					case FVT_Bool:
					{
						// Or instead of Bool, some standard size, but depends what the script compiler makes
						RED_FATAL_ASSERT( paramSize == sizeof(Bool), "Sizeof bool mismatch. Paramsize=%u, sizeof(Bool)=%u", paramSize, sizeof(Bool) );
						*((Bool*)paramMem) = flashValue.GetFlashBool();
						break;
					}

					case FVT_String:
					{
						RED_FATAL_ASSERT( paramSize == sizeof(String), "Sizeof string mismatch. Paramsize=%u, sizeof(String)=%u", paramSize, sizeof(String) );
						new (paramMem) String( flashValue.GetFlashString() );
					}
					break;

					default:
					{
						GUI_ERROR( TXT("Unsupported Flash value passed as event argument") );
						canCallEvent = false;
					}
					break;
				}
			}
		}

		Bool eventSuccess = false;
		if ( canCallEvent )
		{
			eventSuccess = event->Call( context, scriptStack, nullptr );
		}

		if ( ! eventSuccess )
		{
			GUI_ERROR( TXT("Failed to call Flash event '%ls'"), eventName.AsString().AsChar() );
		}

		// Release params
		if ( scriptStackSize != 0 )
		{
			for ( Uint32 i = 0; i < numArgs; ++i )
			{
				const CProperty* param = event->GetParameter(i);
				void* paramMem = param->GetOffsetPtr(scriptStack);

				const CFlashValue& flashValue = args[ i ];
				switch( flashValue.GetFlashValueType() )
				{
					case FVT_UInt:
					{
						const CName paramName = param->GetType()->GetName();
						if ( paramName == ::GetTypeName< SItemUniqueId >() )
						{
							SItemUniqueId *itemUniqueId = (SItemUniqueId*)paramMem;
							itemUniqueId->~SItemUniqueId();
						}
						else if ( paramName == ::GetTypeName< CName >() )
						{
							CName* name = (CName*)paramMem;
							name->~CName();
						}
					}
					break;

					case FVT_String:
					{
						String* string = (String*) paramMem; // extract string pointer
						string->~String();
					}
					break;

					default:
						break;
				}
			}
		}

		return eventSuccess;
	}
} // namespace Flash
