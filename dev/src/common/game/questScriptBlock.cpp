#include "build.h"
#include "questScriptBlock.h"
#include "questGraphSocket.h"
#include "questThread.h"
#include "../../common/core/function.h"
#include "../../common/core/scriptThread.h"
#include "../../common/core/scriptThreadSerializer.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../core/scriptingSystem.h"
#include "../core/gameSave.h"
#include "../engine/graphConnectionRebuilder.h"

#ifdef DEBUG_BLACKSCREEN
#include "storySceneIncludes.h"
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( QuestScriptParam );
IMPLEMENT_ENGINE_CLASS( CQuestScriptBlock )
IMPLEMENT_RTTI_ENUM( EQuestScriptSaveMode );

CQuestScriptBlock::CQuestScriptBlock()
	: m_choiceOutput( false )
	, m_saveMode( QSCSM_SaveBlocker )
{
	m_name = TXT("Script");
}

#ifndef NO_EDITOR_GRAPH_SUPPORT


void CQuestScriptBlock::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	if ( property->GetName() == TXT("functionName") )
	{
		OnRebuildSockets();
	}
}

void CQuestScriptBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	CFunction* function = GetFunction();
	UpdateSockets( function );
	UpdateCaption( function );
	UpdateParameters( function );
}

void CQuestScriptBlock::OnPostLoad()
{
	TBaseClass::OnPostLoad();

	CFunction* function = GetFunction();
	UpdateParameters( function );
}

void CQuestScriptBlock::UpdateSockets( CFunction* function )
{
	if ( !function )
	{
		return;
	}

	// Create mandatory sockets
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( In ), LSD_Input, LSP_Left ) );
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Cut ), LSD_Input, LSP_Center ) );

	// Create outputs
	if ( function && function->GetReturnValue() && function->GetReturnValue()->GetType()->GetName() == GetTypeName< Bool >() )
	{
		// Boolean output
		CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( False ), LSD_Output, LSP_Right ) );
		CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( True ), LSD_Output, LSP_Right ) );
		m_choiceOutput = true;
	}
	else
	{
		// Normal output
		CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Out ), LSD_Output, LSP_Right ) );
		m_choiceOutput = false;
	}
}

void CQuestScriptBlock::UpdateCaption( CFunction* function )
{
	if ( function )
	{
		m_caption = String::Printf( TXT("Script [%s]"), function->GetName().AsString().AsChar() );
	}
	else
	{
		m_caption = TXT( "Script" );
	}
}

void CQuestScriptBlock::UpdateParameters( CFunction* function )
{
	// Clear parameters
	TDynArray< QuestScriptParam > oldParams = m_parameters;
	m_parameters.Clear();

	if ( function )
	{
		// Create parameters
		const Uint32 numParams = static_cast< Uint32 >( function->GetNumParameters() );
		for ( Uint32 i = 0; i<numParams; ++i )
		{
			// Create parameter entry
			CProperty* prop = function->GetParameter( i );
			CRTTISoftHandleType* softHandleConverter = NULL;
			CRTTIHandleType* handleType = NULL;

			if ( prop->GetType()->GetType() == RT_Handle )
			{
				handleType = reinterpret_cast<CRTTIHandleType*>( prop->GetType() );
				if ( handleType->GetName() == GetTypeName< THandle< CEntityTemplate > >() )
				{
					softHandleConverter = new CRTTISoftHandleType( handleType->GetPointedType() );
				}
			}

			QuestScriptParam* newParam = ::new ( m_parameters ) QuestScriptParam( prop->GetName(), softHandleConverter ? softHandleConverter->GetName() : prop->GetType()->GetName() );

			// Copy previous value
			for ( Uint32 j=0; j<oldParams.Size(); j++ )
			{
				const QuestScriptParam& oldParam = oldParams[j];
				if ( oldParam.m_name == prop->GetName() )
				{
					if ( (oldParam.m_value.GetRTTIType() == newParam->m_value.GetRTTIType() || softHandleConverter) )
					{
						if ( !(oldParam.m_value.GetRTTIType() == newParam->m_value.GetRTTIType() ) && softHandleConverter )
						{
							CResource* resource = reinterpret_cast< CResource*>( handleType->GetPointed( const_cast< void* >( oldParam.m_value.GetData() ) ) );
							if ( resource )
							{
								ASSERT( resource->IsA<CResource>() );
							}

							if ( !resource || !resource->IsA<CResource>() )
							{
								continue;
							}

							BaseSoftHandle* baseSoftHandle = new BaseSoftHandle( resource );
							newParam->m_value = CVariant( softHandleConverter->GetName(), baseSoftHandle );			
							newParam->m_softHandle = true;
							delete baseSoftHandle;
						}
						else
						{
							ASSERT( oldParam.m_value.GetRTTIType() == newParam->m_value.GetRTTIType() );
							// Copy value
							newParam->m_value = oldParam.m_value;
							newParam->m_softHandle = oldParam.m_value.GetRTTIType()->GetType() == RT_SoftHandle;
						}
					}
					else if ( oldParam.m_value.GetRTTIType() && newParam->m_value.GetRTTIType() &&
						(oldParam.m_value.GetRTTIType()->GetType() == RT_Array) && (newParam->m_value.GetRTTIType()->GetType() == RT_Array) )
					{
						const IRTTIBaseArrayType* oldArrayType = static_cast< const IRTTIBaseArrayType* >( oldParam.m_value.GetRTTIType() );
						const IRTTIBaseArrayType* newArrayType = static_cast< const IRTTIBaseArrayType* >( newParam->m_value.GetRTTIType() );
					
						// array data conversion is possible only if the data type matches
						if ( oldArrayType->ArrayGetInnerType() == newArrayType->ArrayGetInnerType() )
						{
							newArrayType->Copy( newParam->m_value.GetData(), oldParam.m_value.GetData() ); // usues the newArray memory class
						}
					}

					break;
				}
			}
#ifndef NO_EDITOR
			if( i == 0 )
			{
				String valueString = String::EMPTY;
				if ( newParam->m_value.ToString(valueString) )
				{
					m_searchCaption = String::Printf( TXT("Script [%s], paramName: [%s], paramValue: [%s]"), function->GetName().AsString().AsChar(),newParam->m_name.AsChar(), valueString.AsChar() );
				}

			}
#endif
		}
	}

	// Update dynamic properties browser
	EDITOR_DISPATCH_EVENT( CNAME( UpdateDynamicProperties ), CreateEventData< CObject* >( this ) );
}

#endif

CFunction* CQuestScriptBlock::GetFunction() const
{
	CFunction* function = SRTTI::GetInstance().FindGlobalFunction( m_functionName );
	if ( function && function->IsQuest() )
	{
		return function;
	}
	else
	{
		return NULL;
	}
}

void CQuestScriptBlock::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_runtimeData;
	compiler << i_saveLock;
	compiler << i_functionStarted;
	compiler << i_softHandles;
}

void CQuestScriptBlock::OnInitInstance( InstanceBuffer& instanceData ) const
{
	TBaseClass::OnInitInstance( instanceData );

	instanceData[ i_runtimeData ] = 0;
	instanceData[ i_saveLock ] = CGameSessionManager::GAMESAVELOCK_INVALID;
	instanceData[ i_functionStarted] = 0;
}

void CQuestScriptBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	TBaseClass::OnActivate( data, inputName, parentThread );

	CFunction* function = GetFunction();
	if ( !function )
	{
		Exit( data );
		return;
	}

	for ( Uint32 i = 0; i < m_parameters.Size(); ++i )
	{
		if ( m_parameters[i].m_softHandle )
		{
			if ( m_parameters[i].m_value.GetRTTIType()->GetType() == RT_SoftHandle )
			{
				const BaseSoftHandle* handle = reinterpret_cast<const BaseSoftHandle*>( m_parameters[i].m_value.GetData() );
				if ( handle )
				{
					handle->GetAsync();
					data[ i_softHandles].PushBack( TSoftHandle< CResource >( handle->GetPath() ) );
				}
			}
		}
	}

	// No saves during latent functions
	if ( parentThread->CanBlockSaves() )
	{
		if ( data[ i_runtimeData ] )
		{
			// No saves during behavior scenes
			if ( m_saveMode == QSCSM_SaveBlocker  )
			{
#ifdef NO_EDITOR_GRAPH_SUPPORT
				String lockReason = TXT("Script");
#else
				String lockReason = String::Printf( TXT("Latent '%ls'"), GetCaption().AsChar() );
#endif
				SGameSessionManager::GetInstance().CreateNoSaveLock( lockReason, data[ i_saveLock ] );
			}
		}
	}
}

void CQuestScriptBlock::SetupFunctionCallstack( CFunction* function, void* stackData,  InstanceBuffer& data ) const
{
	// Setup function parameters
	for ( Uint32 i = 0; i < m_parameters.Size(); ++i )
	{
		const QuestScriptParam& param = m_parameters[i];
		CName paramName = param.m_name;
		Bool loadedAsync = false;
		void* asyncResourcePointer = NULL; // :( 

		if ( param.m_softHandle )
		{
			if ( param.m_value.GetRTTIType()->GetType() == RT_SoftHandle ) 
			{
				// If we are here, we MUST be able to get the resources synchronously - it simplifies things a lot.
				const BaseSoftHandle* handle = reinterpret_cast<const BaseSoftHandle*>( param.m_value.GetData() );
				if ( handle )
				{
					handle->Get();
					asyncResourcePointer = const_cast<BaseSafeHandle*>(&handle->GetHandle());
				}
				loadedAsync = true;
			}
		}

		// Find matching function parameter
		Uint32 numFunctionParams = static_cast< Uint32 >( function->GetNumParameters() );
		for ( Uint32 j = 0; j < numFunctionParams; ++j )
		{
			CProperty* funcParam = function->GetParameter(j);
			if ( funcParam->GetName() == paramName )
			{
				if ( param.m_value.GetRTTIType() == funcParam->GetType() )
				{
					// Copy value
					void* destData = funcParam->GetOffsetPtr( stackData );
					param.m_value.GetRTTIType()->Copy( destData, param.m_value.GetData() );
				}
				else if ( loadedAsync && asyncResourcePointer && funcParam->GetType()->GetType() == RT_Handle )
				{
					// Copy value
					void* destData = funcParam->GetOffsetPtr( stackData );

					BaseSafeHandle& handleDest = * ( BaseSafeHandle* )( destData );
					const BaseSafeHandle& handleSrc = * ( const BaseSafeHandle* )( asyncResourcePointer );
					handleDest = handleSrc;
				}
			}
		}
	}
}

Bool CQuestScriptBlock::RunImmediateFunction( CFunction* function, void* stackData ) const
{
	Bool result = true;

#ifdef DEBUG_BLACKSCREEN
	if ( function->GetName() == TXT("FadeOutQuest") || function->GetName() == TXT("FadeInQuest") )
	{
		SCENE_ASSERT( 0 );
	}
#endif

	// Call function
	CPropertyDataBuffer retData( function->GetReturnValue() );
	function->Call( NULL, stackData, retData.Data() );

	// Use return data
	if ( retData.GetType() && retData.GetType()->GetName() == GetTypeName< Bool >() )
	{
		result = *( Bool* ) retData.Data();
	}

	// Cleanup stack frame
	RED_MEMORY_FREE( MemoryPool_Default, MC_ScriptObject, stackData );
	return result;
}

void CQuestScriptBlock::RunLatentFunction( InstanceBuffer& data, CFunction* function, void* stackData ) const
{
	CPropertyDataBuffer* scriptReturnValue = new CPropertyDataBuffer( function->GetReturnValue() );
	CScriptStackFrame* frame = new CScriptStackFrame( NULL, NULL, function, stackData, stackData );
	
	CScriptThread* thread = GScriptingSystem->CreateThreadUseGivenFrame( NULL, function, *frame, scriptReturnValue->Data() );
	thread->EnableSerialization();

	LatentScriptData* runtimeData = new LatentScriptData( function, frame, thread, stackData, scriptReturnValue );
	data[ i_runtimeData ] = reinterpret_cast< TGenericPtr >( runtimeData );
}

void CQuestScriptBlock::OnExecute( InstanceBuffer& data ) const
{
	TBaseClass::OnExecute( data );

	if ( !data[ i_functionStarted ] )
	{
		if ( CheckIfAllDataLoaded( data ) )
		{
			data[ i_softHandles ].Clear();

			CFunction* function = GetFunction();

			// Create fake stack
			const Uint32 stackSize = function->GetStackSize();
			void* stackData = RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_ScriptObject, stackSize );
			Red::System::MemorySet( stackData, 0, stackSize );
			SetupFunctionCallstack( function, stackData, data );

			if ( !function->IsLatent() )
			{
				bool result = RunImmediateFunction( function, stackData );
				Exit( data, result );
			}
			else
			{
				RunLatentFunction( data, function, stackData );
				data[ i_functionStarted ] = true;
			}
		}
	}
	else
	{
		LatentScriptData* runtimeData = reinterpret_cast< LatentScriptData* >( data[ i_runtimeData ] );
		if ( !runtimeData )
		{
			ThrowError( data, TXT("Invalid runtime data for a script thread") );
			return;
		}

		if ( runtimeData->m_isFinished )
		{
			Exit( data, runtimeData->m_result );
		}
	}
}

const Bool CQuestScriptBlock::CheckIfAllDataLoaded( InstanceBuffer& data ) const
{
	Bool ready = true;
	for ( Uint32 i = 0; i < data[ i_softHandles ].Size(); ++i )
	{
		if ( data[ i_softHandles ][i].GetAsync() == BaseSoftHandle::ALR_InProgress )
		{
			ready = false;
			break;
		}
	}

	return ready;
}

void CQuestScriptBlock::OnDeactivate( InstanceBuffer& data ) const
{
	TBaseClass::OnDeactivate( data );

	// Release all soft handles
	for ( Uint32 i = 0; i < m_parameters.Size(); ++i )
	{
		const QuestScriptParam& param = m_parameters[i];
		CName paramName = param.m_name;

		if ( param.m_softHandle )
		{
			if ( param.m_value.GetRTTIType()->GetType() == RT_SoftHandle ) 
			{
				// If we are here, we MUST be able to get the resources synchronously - it simplifies things a lot.
				const BaseSoftHandle* handle = reinterpret_cast<const BaseSoftHandle*>( param.m_value.GetData() );
				handle->Release();
			}
		}
	}

	// Release save lock
	if ( data[ i_saveLock ] >= 0 )
	{
		SGameSessionManager::GetInstance().ReleaseNoSaveLock( data[ i_saveLock ] );
		data[ i_saveLock ] = CGameSessionManager::GAMESAVELOCK_INVALID;
	}

	LatentScriptData* runtimeData = reinterpret_cast< LatentScriptData* >( data[ i_runtimeData ] );
	delete runtimeData;
	data[ i_runtimeData ] = 0;
	data[ i_functionStarted ] = 0;
	data[ i_softHandles ].Clear();
}

void CQuestScriptBlock::Exit( InstanceBuffer& data, Bool result ) const
{
	if ( m_choiceOutput )
	{
		ActivateOutput( data, result ? CNAME( True ) : CNAME( False ) );
	}
	else
	{
		ActivateOutput( data, CNAME( Out ) );
	}
}

IDynamicPropertiesSupplier* CQuestScriptBlock::QueryDynamicPropertiesSupplier()
{
	return static_cast< IDynamicPropertiesSupplier* >( this );
}

const IDynamicPropertiesSupplier* CQuestScriptBlock::QueryDynamicPropertiesSupplier() const
{
	return static_cast< const IDynamicPropertiesSupplier* >( this );
}

void CQuestScriptBlock::GetDynamicProperties( TDynArray< CName >& properties ) const
{
	for ( Uint32 i=0; i<m_parameters.Size(); i++ )
	{
		properties.PushBack( m_parameters[i].m_name );
	}
}

Bool CQuestScriptBlock::ReadDynamicProperty( const CName& propName, CVariant& propValue ) const
{
	// Read from parameters
	for ( Uint32 i=0; i<m_parameters.Size(); i++ )
	{
		const QuestScriptParam& param = m_parameters[i];
		if ( param.m_name == propName )
		{
			propValue = param.m_value;
			return true;
		}
	}

	// Not found
	return false;
}

Bool CQuestScriptBlock::WriteDynamicProperty( const CName& propName, const CVariant& propValue )
{
	// Write to parameters
	for ( Uint32 i=0; i<m_parameters.Size(); i++ )
	{
		QuestScriptParam& param = m_parameters[i];
		if ( param.m_name == propName )
		{
			param.m_value = propValue;
			return true;
		}
	}

	// Not found, add
	::new ( m_parameters ) QuestScriptParam( propName, propValue );
	return true;
}

void CQuestScriptBlock::SerializeDynamicPropertiesForGC( IFile& file )
{
	// Collect all dynamic properties
	for ( Uint32 i = 0; i < m_parameters.Size(); ++i )
	{
		QuestScriptParam & param = m_parameters[i];
		file << param.m_value;
	}
}

void CQuestScriptBlock::SaveGame( InstanceBuffer& data, IGameSaver* saver ) const
{
	// Pass to base class
	TBaseClass::SaveGame( data, saver );

	// Data block
	{
		CGameSaverBlock block( saver, CNAME(questScriptData) );
	
		// Get script serializer
		CScriptThreadSerializer* serializer = NULL;
		LatentScriptData* runtimeData = reinterpret_cast< LatentScriptData* >( data[ i_runtimeData ] );
		if ( runtimeData && runtimeData->m_thread )
		{
			serializer = runtimeData->m_thread->QuerySerializer();
		}

		// Save true if we have valid serializer
		const Bool hasThreadData = serializer != NULL;
		saver->WriteValue( CNAME(hasThreadData), hasThreadData );

		// Save script data
		if ( serializer )
		{
			serializer->SaveState( saver );
		}
	}
}

void CQuestScriptBlock::LoadGame( InstanceBuffer& data, IGameLoader* loader ) const
{
	// Pass to base class
	TBaseClass::LoadGame( data, loader );

	// Block
	{
		CGameSaverBlock block( loader, CNAME(questScriptData) );

		// Do we have script data
		Bool hasThreadData = loader->ReadValue< Bool >( CNAME(hasThreadData) );
		if ( hasThreadData )
		{
			LatentScriptData* runtimeData = reinterpret_cast< LatentScriptData* >( data[ i_runtimeData ] );
			if ( runtimeData && runtimeData->m_thread )
			{
				CScriptThreadSerializer* serializer = runtimeData->m_thread->QuerySerializer();
				if ( serializer )
				{
					serializer->RestoreState( *runtimeData->m_frame, loader );
				}
			}
		}
	}
}

const CScriptThread* const CQuestScriptBlock::GetScriptThread( InstanceBuffer& data ) const
{
	if ( data[ i_runtimeData ] == 0 )
	{
		return NULL;
	}

	LatentScriptData* runtimeData = reinterpret_cast< LatentScriptData* >( data[ i_runtimeData ] );
	return runtimeData->m_thread;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

Bool CQuestScriptBlock::ReadDynamicPropForEditor( const CName& propName, CVariant& propValue ) const
{
	return ReadDynamicProperty( propName, propValue );
}

#endif

///////////////////////////////////////////////////////////////////////////////

LatentScriptData::LatentScriptData( CFunction* scriptFunction, CScriptStackFrame* frame, CScriptThread* scriptThread, void* stackData, CPropertyDataBuffer* scriptReturnValue )
  : m_function( scriptFunction )
  , m_frame( frame )
  , m_thread( scriptThread )
  , m_stackData( stackData )
  , m_scriptReturnValue( scriptReturnValue )
  , m_isFinished( false )
  , m_result( false )
{
	m_thread->SetListener( this );
}

LatentScriptData::~LatentScriptData()
{
	if (m_thread && !m_thread->IsKilled() )
	{
		m_thread->ForceKill();
	}
	m_thread = NULL;
	m_function = NULL;
	m_frame = NULL;
	m_scriptReturnValue = NULL;
	m_stackData = NULL;
	m_result = false;
}

void LatentScriptData::OnScriptThreadKilled( CScriptThread* scriptThread, Bool finished )
{
	if ( m_thread == scriptThread )
	{
		// Set flag
		m_isFinished = finished;

		// We support only boolean return type
		if ( m_scriptReturnValue->GetType() && m_scriptReturnValue->GetType()->GetName() == GetTypeName< Bool >() )
		{
			// Get result
			m_result = *( Bool* ) m_scriptReturnValue->Data();
		}
		else
		{
			// Assume true
			m_result = true;
		}
	}
	else if ( scriptThread )
	{
		LOG_GAME( TXT("OnScriptThreadKilled ERROR - LatentScriptData (Quests). Thread has %s listener"), scriptThread->GetListenerName().AsChar()  );
	}
	else
	{
		//HALT( TXT("CScriptThread is NULL inside OnScriptThreadKilled function") );
	}

	m_thread = NULL;
}

#ifdef DEBUG_BLACKSCREEN
#pragma optimize("",on)
#endif
