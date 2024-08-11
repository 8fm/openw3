#include "build.h"
#include "aiParameters.h"

#include "behTree.h"
#include "behTreeNode.h"
#include "behTreeVarsUtils.h"

#include "commonGame.h"
#include "moveSteeringBehavior.h"

#include "../../common/core/depot.h"

///////////////////////////// INTERFACE /////////////////////////////
IMPLEMENT_ENGINE_CLASS( IAIParameters );

IAIParameters::IAIParameters()
{
#ifdef DEBUG_AI_INITIALIZATION
	m_initialized = false;
#endif

	EnableReferenceCounting( true );
}


////////////////////////////////////////////////////////////////////


#ifndef NO_EDITOR
void IAIParameters::OnCreatedInEditor()
{
	IScriptable* context = this;
	CallFunction( context, CNAME( Init ) );

	InitializeAIParameters();
}
void IAIParameters::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	InitializeAIParameters();
}
#endif

void IAIParameters::InitializeAIParameters()
{
#ifdef DEBUG_AI_INITIALIZATION
	m_initialized = true;
#endif
	m_paramsMap.ClearFast();
	m_subParameters.ClearFast();

	auto funIsSubparamsType =
		[] ( const IRTTIType* propType ) -> Bool
		{
			ERTTITypeType propTypeType = propType->GetType();
			if ( propTypeType == RT_Pointer || propTypeType == RT_Handle )
			{
				const IRTTIType* pointedType = static_cast< const IRTTIPointerTypeBase* >( propType )->GetPointedType();
				return pointedType->GetType() == RT_Class && static_cast< const CClass* >( pointedType )->IsA( IAIParameters::GetStaticClass() ) && !static_cast< const CClass* >( pointedType )->IsA( IAITree::GetStaticClass() );
			}
			return false;
		};

	auto funStoreSubparams =
		[ this ] ( IAIParameters* subparams )
		{
			//subparams->LazyInitialize();
			m_subParameters.PushBack( subparams );
		};

	auto funCollectSubparams =
		[ funStoreSubparams ] ( const void* offsetPointer, const IRTTIType* propType )
		{
			ERTTITypeType propTypeType = propType->GetType();
			if ( propTypeType == RT_Pointer )
			{
				IAIParameters* subparams = NULL;
				propType->Copy( &subparams, offsetPointer );
				if ( subparams )
				{
					funStoreSubparams( subparams );
				}
			}
			else // ( propTypeType == RT_Handle )
			{
				THandle< IAIParameters > valHandle;
				propType->Copy( &valHandle, offsetPointer );
				IAIParameters* subparams = valHandle.Get();
				if ( subparams )
				{
					funStoreSubparams( subparams );
				}
			}
		};

	auto funIterateProperties =
		[ this, funIsSubparamsType, funCollectSubparams ] ( CProperty* prop )
		{
			if ( !prop->IsEditable() )
			{
				return;
			}

			const IRTTIType* propType = prop->GetType();

			if ( funIsSubparamsType( propType ) )
			{
				void* offsetPointer = const_cast< void* >( prop->GetOffsetPtr( this ) );
				funCollectSubparams( offsetPointer, propType );
				return;
			}

			if ( propType->GetType() == RT_Array )
			{
				const CRTTIArrayType* arrayType = static_cast< const CRTTIArrayType* >( propType );
				const IRTTIType* arrayFieldType = arrayType->GetInnerType();
				if ( funIsSubparamsType( arrayFieldType ) )
				{
					void* offsetPointer = const_cast< void* >( prop->GetOffsetPtr( this ) );
					for ( Uint32 i = 0, n = arrayType->GetArraySize( offsetPointer ); i < n; ++i )
					{
						void* arrayElement = arrayType->GetArrayElement( offsetPointer, i );
						funCollectSubparams( arrayElement, arrayFieldType );
					}
					return;
				}
			}

			m_paramsMap.PushBack( TArrayMap< CName, CProperty* >::value_type( prop->GetName(), prop ) );
		};

	GetClass()->IterateProperties( funIterateProperties );

	m_paramsMap.Sort();
}

Bool IAIParameters::HasParameter( CName name ) const
{
#ifdef DEBUG_AI_INITIALIZATION
	ASSERT( m_initialized );
#endif
	auto itFind = m_paramsMap.Find( name );
	if ( itFind != m_paramsMap.End() )
	{
		return true;
	}
	for ( auto it = m_subParameters.Begin(), end = m_subParameters.End(); it != end; ++it )
	{
		if ( (*it)->HasParameter( name ) )
		{
			return true;
		}
	}
	return false;
}

void IAIParameters::OnScriptReloaded()
{
	InitializeAIParameters();
}

Bool IAIParameters::OnPropertyTypeMismatch( CName propertyName, IProperty* existingProperty, const CVariant& readValue )
{
	if ( BehTreeVarsUtils::ConvertPointerTypes( this, existingProperty, readValue.GetRTTIType(), readValue.GetData() ) )
	{
		return true;
	}

	return TBaseClass::OnPropertyTypeMismatch( propertyName, existingProperty, readValue );
}

void IAIParameters::OnPostLoad()
{
	InitializeAIParameters();

	TBaseClass::OnPostLoad();
}

void IAIParameters::funcLoadSteeringGraph( CScriptStackFrame& stack, void* result )
{
	// functionality meant to be used only in parameters Init() function (used in editor)
	GET_PARAMETER( String, fileName, String::EMPTY );
	FINISH_PARAMETERS;

	THandle< CMoveSteeringBehavior > steeringBehavior;
	if ( fileName.EndsWith( CMoveSteeringBehavior::GetFileExtension() ) )
	{
		CResource* res = GDepot->LoadResource( fileName );	
		steeringBehavior = Cast< CMoveSteeringBehavior >( res );
	}
	
	RETURN_HANDLE( CMoveSteeringBehavior, steeringBehavior );
}

void IAIParameters::OnManualRuntimeCreation()
{
	InitializeAIParameters();

	for ( auto it = m_subParameters.Begin(), end = m_subParameters.End(); it != end; ++it )
	{
		(*it)->InitializeAIParameters();
	}
}

void IAIParameters::funcOnManualRuntimeCreation( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	OnManualRuntimeCreation();
}

void IAIParameters::MergeParams( IAIParameters* params )
{
	TArrayMap< CName, CProperty* > injectedParams = params->GetParams();
	// [ Step ] Iterate through all param to inject
	for( TArrayMap< CName, CProperty* >::iterator paramToInjectIt = injectedParams.Begin(); paramToInjectIt != injectedParams.End(); ++paramToInjectIt )
	{
		const CName & paramToInjectName		= paramToInjectIt->m_first;
		auto destParamIt					= m_paramsMap.Find( paramToInjectName );
		CProperty* sourceParam				= paramToInjectIt->m_second;

		// [ Step ] Check if destination class has a param of similar name
		if ( destParamIt != m_paramsMap.End() )
		{
			CProperty* destParam			= destParamIt->m_second;
			// [ Step ] Sanity check
			if( sourceParam->GetType()->GetType() == RT_Handle && destParamIt->m_second->GetType()->GetType() == RT_Handle )
			{
				// [ Step ] Read wrapped types
				const IRTTIType* sourcePtrType	= static_cast< const IRTTIPointerTypeBase* >( sourceParam->GetType() )->GetPointedType();
				const IRTTIType* destPtrType	= static_cast< const IRTTIPointerTypeBase* >( destParam->GetType() )->GetPointedType();

				// [ Step ] Compare wrapped types to check inheritance, more sanity check
				if ( sourcePtrType && destPtrType && sourcePtrType->GetType() == RT_Class && destPtrType->GetType() == RT_Class )
				{
					const CClass* sourceClass	= static_cast< const CClass* >( sourcePtrType );
					const CClass* destClass		= static_cast< const CClass* >( destPtrType );

					if ( sourceClass->IsA( destClass ) )
					{
						//CVariant sourceParamData;
						//CObject * sourceParamData = nullptr;
						THandle< IScriptable > sourceParamHandle;
						sourceParam->Get( params, &sourceParamHandle );
						//IAIParameters * o = (IAIParameters *)sourceParamData.GetData();
						
						if( sourceParamHandle.Get() )
						{
							THandle< ISerializable > destParamHandle;
							destParamHandle = sourceParamHandle.Get()->Clone( this );
							destParam->Set( this, &destParamHandle );

							IAITree *const aiTree = static_cast< IAITree * >( destParamHandle.Get() );
							aiTree->InitializeAIParameters();
						}			
					}
				}	
			}				
		}
	}
}


IAIParameters *const  IAIParameters::GetAiParametersByClassName( CName contextName ) const
{
	if ( GetClass()->GetName() == contextName )
	{
		return const_cast< IAIParameters *const >( this );
	}

	for ( auto it = m_subParameters.Begin(), end = m_subParameters.End(); it != end; ++it )
	{
		const IAIParameters *const params = (*it);
		if ( params->GetClass()->GetName() == contextName )
		{
			return const_cast< IAIParameters *const >( params );
		}
	}
	return nullptr;
}


///////////////////////////// BASE /////////////////////////////////
IMPLEMENT_ENGINE_CLASS( CAIParameters );


///////////////////////////// DEFAULTS /////////////////////////////
IMPLEMENT_ENGINE_CLASS( CAIDefaults );

///////////////////////////// PARAM TREES //////////////////////////
IMPLEMENT_ENGINE_CLASS( IAITree );
IMPLEMENT_ENGINE_CLASS( CAIBaseTree );
IMPLEMENT_ENGINE_CLASS( CAITree );
IMPLEMENT_ENGINE_CLASS( CAIPerformCustomWorkTree );

IAITree::IAITree()
{
}

void IAITree::InitializeTree()
{
	// Possibly heavy stuff, but should be run only for legacy assets and commented out on cooked builds
	if ( !m_aiTreeName.Empty() )
	{
		String temp;
		m_tree = ::LoadResource< CBehTree >( CFilePath::ConformPath( m_aiTreeName, temp ) );
	}
}

Bool IAITree::InitializeData()
{
	IScriptable* context = static_cast< IScriptable* >( this );
	CallFunction( context, CNAME( Init ) );

	return true;
}

void IAITree::OnManualRuntimeCreation()
{
	InitializeTree();

	TBaseClass::OnManualRuntimeCreation();
}

Bool IAITree::OnPropertyTypeMismatch( CName propertyName, IProperty* existingProperty, const CVariant& readValue )
{
	{
		static const CName PARAMS( TXT("params") );
		if ( propertyName == PARAMS )
		{
			if ( BehTreeVarsUtils::ConvertPointerTypes( this, existingProperty, readValue.GetRTTIType(), readValue.GetData() ) )
			{
				return true;
			}
		}
	}

	return TBaseClass::OnPropertyTypeMismatch( propertyName, existingProperty, readValue );
}

Bool IAITree::OnPropertyMissing( CName propertyName, const CVariant& readValue )
{
	{
		static const CName PARAMS( TXT("params") );
		if ( propertyName == PARAMS )
		{
			IScriptable* srcObj = NULL;
			if ( readValue.GetRTTIType()->GetType() == RT_Pointer )
			{
				readValue.GetRTTIType()->Copy( &srcObj, readValue.GetData() );
			}
			else if ( readValue.GetRTTIType()->GetType() == RT_Handle )
			{
				THandle< IScriptable > ptr;
				readValue.GetRTTIType()->Copy( &ptr, readValue.GetData() );
				srcObj = ptr.Get();
			}
			if ( srcObj )
			{
				const auto& srcParams = srcObj->GetClass()->GetCachedProperties();
				const auto& localParams = GetClass()->GetLocalProperties();

				for ( auto itSrc = srcParams.Begin(), endSrc = srcParams.End(); itSrc != endSrc; ++itSrc )
				{
					CProperty* srcProp = *itSrc;
					for ( auto itDst = localParams.Begin(), endDst = localParams.End(); itDst != endDst; ++itDst )
					{
						CProperty* dstProp = *itDst;
						if ( srcProp->GetName() == dstProp->GetName() )
						{
							if ( srcProp->GetType() == dstProp->GetType() )
							{
								dstProp->Set( this, srcProp->GetOffsetPtr( srcObj ) );
							}
							else
							{
								BehTreeVarsUtils::ConvertPointerTypes( this, dstProp, srcProp->GetType(), srcProp->GetOffsetPtr( srcObj ) );
							}
							break;
						}
					}
				}
			}
		}
	}

	return TBaseClass::OnPropertyMissing( propertyName, readValue );
}

void IAITree::OnPostLoad()
{
#if !defined( NO_EDITOR ) && !defined( RED_FINAL_BUILD )
	InitializeTree();
#endif

	TBaseClass::OnPostLoad();
}
#ifndef NO_EDITOR
void IAITree::OnCreatedInEditor()
{
	OnCreated();
}
Bool IAITree::RefactorAll()
{
	TDynArray< THandle< IScriptable > > allScriptables;
	IScriptable::CollectAllScriptableObjects( allScriptables );

	Bool b = false;

	for ( auto it = allScriptables.Begin(), end = allScriptables.End(); it != end; ++it )
	{
		IAITree* task = Cast< IAITree >( it->Get() );
		if ( task )
		{
			IScriptable* context = task;
			Bool funRet = false;
			if ( CallFunctionRet< Bool >( context, CNAME( OnPostLoad ), funRet ) )
			{
				b = funRet || b;
			}
		}
	}
	return b;
}
#endif

IAIParameters* IAITree::GetParams()
{
	if( !m_internalParameters )
	{
		auto funIterate =
			[ this ] ( CProperty* prop )
			{
				if ( prop->GetName() == CNAME( params ) )
				{
					IRTTIType* propType = prop->GetType();
					if( propType->GetType() == RT_Handle )
					{
						THandle<IAIParameters> val = NULL;
						prop->Get(const_cast<IAITree*>(this), &val );
						m_internalParameters = val.Get();
					}
					else if ( propType->GetType() == RT_Pointer )
					{
						prop->Get(const_cast<IAITree*>(this), &m_internalParameters );
					}
				}
			};
		GetClass()->IterateProperties( funIterate );
	}

	return m_internalParameters;
}

void IAITree::OnCreated()
{
	InitializeTree();
	InitializeData();
	InitializeAIParameters();
}
void IAITree::funcOnCreated( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	OnCreated();
}


