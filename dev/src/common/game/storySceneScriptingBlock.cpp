/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "storySceneScriptingBlock.h"
#include "storySceneGraphSocket.h"
#include "storyScenePlayer.h"
#include "storyScene.h"
#include "storySceneControlPartsUtil.h"
#include "../core/scriptingSystem.h"
#include "../core/feedback.h"
#include "../engine/graphConnectionRebuilder.h"

IMPLEMENT_ENGINE_CLASS( CStorySceneScript )
IMPLEMENT_ENGINE_CLASS( CStorySceneScriptingBlock );
IMPLEMENT_ENGINE_CLASS( StorySceneScriptParam );

const Uint32 CStorySceneScript::TRUE_LINK_INDEX	 = 1;
const Uint32 CStorySceneScript::FALSE_LINK_INDEX = 0;

CStorySceneScript::CStorySceneScript()
{
}

CScriptThread* CStorySceneScript::Execute( CStoryScenePlayer* player, void* threadReturnValue, Int32& result ) const
{
	// Get function
	CFunction* function = GetFunction();
	ASSERT( function );

	// Create fake stack
	const Uint32 stackSize = function->GetStackSize();
	void* stackData = RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_ScriptObject, stackSize );
	Red::System::MemorySet( stackData, 0, stackSize );

	// Setup scene player property
	{
		THandle< CStoryScenePlayer > playerHandle( player );
		ASSERT( function->GetParameter(0)->GetDataOffset() == 0 );
		*( THandle< CStoryScenePlayer >* ) stackData = playerHandle;
	}

	// Setup function parameters
	for ( Uint32 i=0; i<m_parameters.Size(); i++ )
	{
		const StorySceneScriptParam& param = m_parameters[i];
		const CName paramName = param.m_name;

		// Find matching function parameter
		const Uint32 numFunctionParams = static_cast< Uint32 >( function->GetNumParameters() );
		for ( Uint32 j=0; j<numFunctionParams; j++ )
		{
			CProperty* funcParam = function->GetParameter(j);
			if ( funcParam->GetName() == paramName && param.m_value.GetRTTIType() == funcParam->GetType() )
			{
				// Copy value
				void* destData = funcParam->GetOffsetPtr( stackData );
				param.m_value.GetRTTIType()->Copy( destData, param.m_value.GetData() );
			}
		}
	}

	// Clean result
	result = -1;

	// It's not a latent function, execute in place
	if ( !function->IsLatent() )
	{
		// Call function
		CPropertyDataBuffer retData( function->GetReturnValue() );
		function->Call( NULL, stackData, retData.Data() );

		// Use return data
		if ( retData.GetType() && retData.GetType()->GetName() == GetTypeName< Bool >() )
		{
			Bool retValue = *( Bool* ) retData.Data();
			result = retValue ? 1 : 0;			
		}
		else if ( retData.GetType() && retData.GetType()->GetType() == RT_Enum )
		{
			result = *( Int32* ) retData.Data();		
		}

		// Cleanup stack frame
		RED_MEMORY_FREE( MemoryPool_Default, MC_ScriptObject, stackData );
	}
	else
	{
		// Create new thread, LEAK LEAK LEAK
		CScriptStackFrame* frame = new CScriptStackFrame( NULL, player, function, stackData, stackData );
		return GScriptingSystem->CreateThreadUseGivenFrame( player, function, *frame, threadReturnValue );
	}

	// No thread is returned
	return NULL;
}

void CStorySceneScript::CollectControlPartsImpl( TDynArray< CStorySceneControlPart* >& controlParts, TDynArray< const CStorySceneControlPart* >& visitedControlParts )
{
	const Bool alreadyVisited = visitedControlParts.Exist( this );
	if( !alreadyVisited )
	{
		visitedControlParts.PushBack( this );

		// Normal output
		{
			CStorySceneControlPart* nextContolPart = StorySceneControlPartUtils::GetControlPartFromLink( GetNextElement() );
			if ( nextContolPart != NULL && nextContolPart != this )
			{
				nextContolPart->CollectControlParts( controlParts, visitedControlParts );
			}
		}

		for( Uint32 i = 0; i < m_links.Size(); i++ )
		{
			CStorySceneControlPart* contolPart = StorySceneControlPartUtils::GetControlPartFromLink( m_links[i]->GetNextElement() );
			if ( contolPart != NULL && contolPart != this )
			{
				contolPart->CollectControlParts( controlParts, visitedControlParts );
			}
		}
	}
}

CFunction* CStorySceneScript::GetFunction() const
{
	if ( !m_function )
	{
		m_function = SRTTI::GetInstance().FindGlobalFunction( m_functionName );
		if ( m_function && !m_function->IsScene() )
		{
			m_function = NULL;
		}
	}

	return m_function;
}

void CStorySceneScript::OnScriptReloaded()
{
	TBaseClass::OnScriptReloaded();
	ResetFunctionLayout();
}

Bool CStorySceneScript::ResetFunctionLayout()
{
	m_function = NULL;
	CFunction* func = GetFunction();
	IRTTIType* newRetType = func && func->GetReturnValue() ? func->GetReturnValue()->GetType() : nullptr;	

	TDynArray<CStorySceneLinkElement*>	oldLinks = m_links;
	CStorySceneLinkElement*				oldNLE = m_nextLinkElement;
	TDynArray< StorySceneScriptParam >	oldParams = m_parameters;

	m_parameters.Clear();
	m_nextLinkElement = nullptr;
	m_links.Clear();

	Uint32 paramMatch = 0;
	if ( func )
	{
		// Create parameters
		const Uint32 numParams = static_cast< Uint32 >( func->GetNumParameters() );

		for ( Uint32 i=1; i<numParams; ++i )
		{
			// Create parameter entry
			CProperty* prop = func->GetParameter( i );
			StorySceneScriptParam* newParam = ::new ( m_parameters ) StorySceneScriptParam( prop->GetName(), prop->GetType()->GetName() );

			// Copy previous value
			for ( Uint32 j=0; j<oldParams.Size(); j++ )
			{
				const StorySceneScriptParam& oldParam = oldParams[j];
				if ( oldParam.m_name == prop->GetName() )
				{
					// Copy value
					if ( oldParam.m_value.GetRTTIType() == prop->GetType() )
					{
						newParam->m_value = oldParam.m_value;
						paramMatch++;
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
							paramMatch++;
						}
					}
					break;
				}
			}
		}		
		if ( func->IsScene() && newRetType && newRetType->GetName() == GetTypeName< Bool >() )
		{
			if( oldLinks.Size() == 2 )
			{
				m_links = oldLinks;
			}
			else
			{
				m_links.PushBack( CreateObject< CStorySceneLinkElement >( this ) );
				m_links.PushBack( CreateObject< CStorySceneLinkElement >( this ) );			
			}		
		}
		else if ( func->IsScene() && newRetType && newRetType->GetType() == RT_Enum )
		{
			const TDynArray< CName >& options = static_cast< CEnum* >( func->GetReturnValue()->GetType() )->GetOptions();			
			if( options.Size() == oldLinks.Size() )
			{
				m_links = oldLinks;
			}
			else
			{
				for( Uint32 i = 0; i < options.Size(); i++ )
				{
					m_links.PushBack( CreateObject< CStorySceneLinkElement >( this ) );
				}	
			}					
		}
		else if( !func->GetReturnValue() )
		{		
			if( oldLinks.Size() == 2 )
			{
				m_nextLinkElement = oldLinks[ TRUE_LINK_INDEX ];
			}
			else
			{
				m_nextLinkElement = oldNLE;
			}
		}
	}
	
	// Update dynamic properties browser
	EDITOR_DISPATCH_EVENT( CNAME( UpdateDynamicProperties ), CreateEventData< CObject* >( this ) );
	return m_links != oldLinks || m_nextLinkElement != oldNLE  || paramMatch != oldParams.Size();
}

#ifndef NO_EDITOR_GRAPH_SUPPORT
void CStorySceneScript::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	if ( property->GetName() == TXT("functionName") )
	{
		// Reset internal layout
		ResetFunctionLayout();

		// Rebuild block layout
		EDITOR_DISPATCH_EVENT( CNAME( ScenePartBlockSocketsChanged ), CreateEventData( this ) );
	}
}
#endif

IDynamicPropertiesSupplier* CStorySceneScript::QueryDynamicPropertiesSupplier()
{
	return static_cast< IDynamicPropertiesSupplier* >( this );
}

const IDynamicPropertiesSupplier* CStorySceneScript::QueryDynamicPropertiesSupplier() const
{
	return static_cast< const IDynamicPropertiesSupplier* >( this );
}

void CStorySceneScript::GetDynamicProperties( TDynArray< CName >& properties ) const
{
	for ( Uint32 i=0; i<m_parameters.Size(); i++ )
	{
		properties.PushBack( m_parameters[i].m_name );
	}
}

Bool CStorySceneScript::ReadDynamicProperty( const CName& propName, CVariant& propValue ) const
{
	// Read from parameters
	for ( Uint32 i=0; i<m_parameters.Size(); i++ )
	{
		const StorySceneScriptParam& param = m_parameters[i];
		if ( param.m_name == propName )
		{
			propValue = param.m_value;
			return true;
		}
	}

	// Not found
	return false;
}

Bool CStorySceneScript::WriteDynamicProperty( const CName& propName, const CVariant& propValue )
{
	// Write to parameters
	for ( Uint32 i=0; i<m_parameters.Size(); i++ )
	{
		StorySceneScriptParam& param = m_parameters[i];
		if ( param.m_name == propName )
		{
			param.m_value = propValue;
			return true;
		}
	}

	// Not found, add
	::new ( m_parameters ) StorySceneScriptParam( propName, propValue );
	return true;
}

void CStorySceneScript::SerializeDynamicPropertiesForGC( IFile& file )
{
	// Collect all dynamic properties
	for ( Uint32 i = 0; i < m_parameters.Size(); ++i )
	{
		StorySceneScriptParam & param = m_parameters[i];
		file << param.m_value;
	}
}

RED_DEFINE_STATIC_NAME(trueLink);
RED_DEFINE_STATIC_NAME(falseLink);

Bool CStorySceneScript::OnPropertyMissing( CName propertyName, const CVariant& readValue )
{
	if( ( propertyName == CNAME( trueLink ) || propertyName == CNAME( falseLink ) ) )
	{
		CFunction*	fun = m_function;
		if ( !fun )
		{
			fun = SRTTI::GetInstance().FindGlobalFunction( m_functionName );
		}
		if ( fun && fun->IsScene() )
		{
			CStorySceneLinkElement* prop;
			readValue.AsType< CStorySceneLinkElement* >( prop );

			if ( fun->GetReturnValue() && fun->GetReturnValue()->GetType()->GetName() == GetTypeName< Bool >() )
			{
				if( m_links.Empty() ) 
				{
					m_links.Resize( 2 );
				}
				if( propertyName == CNAME( trueLink ) )
				{
					ASSERT( !m_links[ TRUE_LINK_INDEX ] );			
					m_links[ TRUE_LINK_INDEX ] = prop;
				}
				else
				{
					ASSERT( !m_links[ FALSE_LINK_INDEX ] );
					m_links[ FALSE_LINK_INDEX ] = prop;
				}	 
			}
			else if( !fun->GetReturnValue() && propertyName == CNAME( trueLink ) )	
			{
				m_nextLinkElement = prop;									
			}			
		}
		return true; 
	}
	return false;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CStorySceneScriptingBlock::UpdateScriptInfo()
{
	if ( GFeedback && m_sceneScript )
	{		
		CFunction* fun = m_sceneScript->GetFunction();
		const Uint32 numOutSoc = !fun || !fun->GetReturnValue() ? 1 : m_sceneScript->GetOutLinks().Size();
		const Uint32 numInSoc = 1;

		TDynArray<StorySceneScriptParam> oldParams = m_sceneScript->GetParameters();

		if( m_sceneScript->ResetFunctionLayout() || ( m_sockets.Size() != numOutSoc + numInSoc) )
		{
			String message = TXT( "Function layout for script block ") + GetBlockName() + TXT(" has changed. Please check if the links for following block are fine and resave the scene" );			
			message += TXT( "\n\nOld block parameters:\n" );			
			for( const StorySceneScriptParam& it : oldParams )
			{
				message += it.m_name.AsString() + TXT(" \t\t ") + it.m_value.ToString() + TXT("\n") ;
			}
			message += TXT( "\n\nNew block parameters:\n" );			
			for( const StorySceneScriptParam& it : m_sceneScript->GetParameters() )
			{
				message += it.m_name.AsString() + TXT(" \t\t ") + it.m_value.ToString() + TXT("\n") ;
			}

			
			GFeedback->ShowMsg( TXT("Resave needed"), message.AsChar() );
			OnRebuildSockets();
		}
	}
}

String CStorySceneScriptingBlock::GetBlockName() const
{
	// Use function name
	if ( m_sceneScript )
	{
		CFunction* function = m_sceneScript->GetFunction();
		if ( function )
		{
			return String::Printf( TXT("Script [%s]"), function->GetName().AsString().AsChar() );
		}
	}

	return TXT( "Script" );
}

void CStorySceneScriptingBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	if ( m_sceneScript )
	{
		CreateSocket( StorySceneGraphSocketSpawnInfo( CNAME( In ), m_sceneScript, LSD_Input ) );
		const TDynArray<CStorySceneLinkElement*>& links = m_sceneScript->GetOutLinks();

		CFunction* function = m_sceneScript->GetFunction();
		if ( function && function->IsScene() && function->GetReturnValue() && function->GetReturnValue()->GetType()->GetName() == GetTypeName< Bool >() )
		{
			SCENE_ASSERT( links.Size() == 2 );
			CreateSocket( StorySceneGraphSocketSpawnInfo( CNAME( False ), links[ CStorySceneScript::FALSE_LINK_INDEX ], LSD_Output ) );
			CreateSocket( StorySceneGraphSocketSpawnInfo( CNAME( True ), links[ CStorySceneScript::TRUE_LINK_INDEX ], LSD_Output ) );
		}
		else if ( function && function->IsScene() && function->GetReturnValue() && function->GetReturnValue()->GetType()->GetType() == RT_Enum )
		{
			const TDynArray< CName >& options = static_cast< CEnum* >( function->GetReturnValue()->GetType() )->GetOptions();			
			SCENE_ASSERT( options.Size() == links.Size() );
			for( Uint32 i = 0; i < options.Size(); i++ )
			{
				CreateSocket( StorySceneGraphSocketSpawnInfo( options[i], links[i] , LSD_Output ) );
			}
		}
		else
		{
			CreateSocket( StorySceneGraphSocketSpawnInfo( CNAME( Out ), m_sceneScript, LSD_Output ) );
		}
	}
}

Color CStorySceneScriptingBlock::GetTitleColor() const
{
	return Color::LIGHT_BLUE;
}

void CStorySceneScriptingBlock::OnDestroyed()
{
	if ( m_sceneScript != NULL )
	{
		m_sceneScript->GetScene()->RemoveControlPart( m_sceneScript );
	}
}

#endif

void CStorySceneScriptingBlock::SetStorySceneScript( CStorySceneScript* sceneScript )
{
	m_sceneScript = sceneScript;
}