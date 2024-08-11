#include "build.h"
#include "behTreeVarsUtils.h"

#include "behTreeVarsEnums.h"
#include "behTreeVars.h"


namespace BehTreeVarsUtils
{
	///////////////////////////////////////////////////////////////////////////
	// helper function templates
	template < class TBehTreeVar >
	Bool TBehTreeVarMatch( const IRTTIType* typeDefinition, const IRTTIType* typeInstance  )
	{
		if ( typeDefinition->GetName() == ::GetTypeName< TBehTreeVar >() )
		{
			if ( typeInstance->GetType() == RT_Simple || typeInstance->GetType() == RT_Fundamental )
			{
				CName insName = typeInstance->GetName();
				CName defBaseName = ::GetTypeName< typename TBehTreeVar::VarType >();
				if (
					( defBaseName == CNAME( Int32 ) )
					? insName == CNAME( Int32 ) || insName == CNAME( Uint32 )
					: defBaseName == insName )
				{
					return true;
				}
			}
			
		}

		return false;
	}

	template <>
	Bool TBehTreeVarMatch< IBehTreeValueEnum >( const IRTTIType* typeDefinition, const IRTTIType* typeInstance  )
	{
		if ( typeDefinition->IsPointerType() )
		{
			const IRTTIPointerTypeBase* ptrType = static_cast< const IRTTIPointerTypeBase* >( typeDefinition );
			const IRTTIType* pointedType = ptrType->GetPointedType();
			if ( pointedType->GetType() == RT_Class && static_cast< const CClass* >( pointedType )->IsA< IBehTreeValueEnum >()								// check pointed class
				&& ( typeInstance->GetType() == RT_Simple || typeInstance->GetType() == RT_Fundamental || typeInstance->GetType() == RT_Enum ) )			// check instance type
			{
				return true;
			}
		}
		return false;
	}

	template <>
	Bool TBehTreeVarMatch< CBehTreeValString >( const IRTTIType* typeDefinition, const IRTTIType* typeInstance  )
	{		
		if ( typeDefinition->GetType() == RT_Class )
		{		
			if( static_cast< const CClass* >( typeDefinition )->IsA< CBehTreeValString >() && typeInstance->GetName() == ::GetTypeName< String >() )
			{
				return true;
// 				ERTTITypeType typeInstanceType = typeInstance->GetType();
// 				if( typeInstanceType == RT_Simple || typeInstanceType == RT_Fundamental ) 		// check instance type
// 				{
// 					return true;
// 				}
			}		
		}
		return false;
	}

	template < class TBehTreeVar >
	Bool TConvertProperty( const IScriptable* definition, IScriptable* instance, const CProperty* propDefinition, const CProperty* propInstance, CBehTreeSpawnContext& context )
	{
		const IRTTIType* typeDefinition = propDefinition->GetType();
		const IRTTIType* typeInstance = propInstance->GetType();

		if ( typeDefinition->GetName() == ::GetTypeName< TBehTreeVar >() )
		{
			if ( typeInstance->GetType() == RT_Simple || typeInstance->GetType() == RT_Fundamental )
			{
				CName insName = typeInstance->GetName();
				CName defBaseName = ::GetTypeName< typename TBehTreeVar::VarType >();
				if (
					( defBaseName == CNAME( Int32 ) )
					? insName == CNAME( Int32 ) || insName == CNAME( Uint32 )
					: defBaseName == insName )
				{
					const TBehTreeVar* var = static_cast< const TBehTreeVar* >( propDefinition->GetOffsetPtr( definition ) );
					typename TBehTreeVar::VarType val;
					var->GetValRef( context, val );
					propInstance->Set( instance, &val );
					return true;
				}
			}

		}

		return false;
	}

	template <>
	Bool TConvertProperty< IBehTreeValueEnum >( const IScriptable* definition, IScriptable* instance, const CProperty* propDefinition, const CProperty* propInstance, CBehTreeSpawnContext& context )
	{
		const IRTTIType* typeDefinition = propDefinition->GetType();
		const IRTTIType* typeInstance = propInstance->GetType();

		if ( typeDefinition->IsPointerType() )
		{
			const IRTTIPointerTypeBase* ptrType = static_cast< const IRTTIPointerTypeBase* >( typeDefinition );
			const IRTTIType* pointedType = ptrType->GetPointedType();
			if ( pointedType->GetType() == RT_Class && static_cast< const CClass* >( pointedType )->IsA< IBehTreeValueEnum >()								// check pointed class
				&& ( typeInstance->GetType() == RT_Simple || typeInstance->GetType() == RT_Fundamental || typeInstance->GetType() == RT_Enum ) )			// check instance type
			{
				
				CPointer ptr = ptrType->GetPointer( propDefinition->GetOffsetPtr( definition ) );
				
				if ( !ptr.IsNull() )
				{
					IBehTreeValueEnum* e = static_cast< IBehTreeValueEnum* >( ptr.GetPointer() );

					Int64 val = e->GetVal( context );

					propInstance->Set( instance, &val );
				}

				return true;
			}
		}
		return false;
	}

	template <>
	Bool TConvertProperty< CBehTreeValString >( const IScriptable* definition, IScriptable* instance, const CProperty* propDefinition, const CProperty* propInstance, CBehTreeSpawnContext& context )
	{		
		const IRTTIType* typeDefinition = propDefinition->GetType();
		const IRTTIType* typeInstance = propInstance->GetType();

		if ( typeDefinition->GetType() == RT_Class )
		{		
			if( static_cast< const CClass* >( typeDefinition )->IsA< CBehTreeValString >() && typeInstance->GetName() == ::GetTypeName< String >() )
			{
				const CBehTreeValString* defVar = reinterpret_cast< const CBehTreeValString* >( propDefinition->GetOffsetPtr( definition ) );

				String* instanceVariable = reinterpret_cast< String* >( propInstance->GetOffsetPtr( instance ) );

				defVar->GetValRef( context, *instanceVariable );
				//propInstance->Set( instance, instanceVariable );

				return true;
				// 				ERTTITypeType typeInstanceType = typeInstance->GetType();
				// 				if( typeInstanceType == RT_Simple || typeInstanceType == RT_Fundamental ) 		// check instance type
				// 				{
				// 					return true;
				// 				}
			}		
		}
		return false;
	}
	
	template < class TBehTreeVar >
	Bool TOnPropertyTypeMismatch( IScriptable* obj, CName propertyName, IProperty* instanceProperty, const CVariant& readValue )
	{
		if ( instanceProperty->GetType()->GetName() == ::GetTypeName< TBehTreeVar >() )
		{
			IRTTIType* prevType = readValue.GetRTTIType();
			if ( prevType->GetType() == RT_Simple || prevType->GetType() == RT_Fundamental )
			{
				CName prevTypeName = prevType->GetName();
				if ( (::GetTypeName< typename TBehTreeVar::VarType >() == CNAME( Int32 )) ? prevTypeName == CNAME( Int32 ) || prevTypeName == CNAME( Uint32 ) : ::GetTypeName< typename TBehTreeVar::VarType >() == prevTypeName )
				{
					typename TBehTreeVar::VarType val = *reinterpret_cast< const typename TBehTreeVar::VarType* >( readValue.GetData() );
					TBehTreeVar* ptr = (TBehTreeVar *)instanceProperty->GetOffsetPtr( obj ); // cast is safe here because of earlier check 
					ptr->m_value = val;
					return true;
				}
			}
		}

		return false;
	}
	///////////////////////////////////////////////////////////////////////////

	Bool OnPropertyTypeMismatch( IScriptable* obj, CName propertyName, IProperty* existingProperty, const CVariant& readValue )
	{
		if (	TOnPropertyTypeMismatch< CBehTreeValFloat >( obj, propertyName, existingProperty, readValue ) 
			||	TOnPropertyTypeMismatch< CBehTreeValBool >( obj, propertyName, existingProperty, readValue )
			||	TOnPropertyTypeMismatch< CBehTreeValInt >( obj, propertyName, existingProperty, readValue ) 
			||	TOnPropertyTypeMismatch< CBehTreeValCName >( obj, propertyName, existingProperty, readValue )
			||	TOnPropertyTypeMismatch< CBehTreeValEMoveType >( obj, propertyName, existingProperty, readValue)
			||	TOnPropertyTypeMismatch< CBehTreeValEExplorationType >( obj, propertyName, existingProperty, readValue)
			||	TOnPropertyTypeMismatch< CBehTreeValString >( obj, propertyName, existingProperty, readValue)
			||	TOnPropertyTypeMismatch< CBehTreeValEntityHandle >( obj, propertyName, existingProperty, readValue)
			||	TOnPropertyTypeMismatch< CBehTreeValSteeringGraph >( obj, propertyName, existingProperty, readValue)
			||	TOnPropertyTypeMismatch< CBehTreeValFormation >( obj, propertyName, existingProperty, readValue))
		{
				return true;
		}
		return false;
	}

	Bool ConvertPointerTypes( IScriptable* container, const CProperty* prop, const IRTTIType* sourceType, const void* sourceData )
	{
		const IRTTIType* destType = prop->GetType();

		ERTTITypeType sourceTypeType = sourceType->GetType();
		ERTTITypeType destTypeType = destType->GetType();

		if ( (destTypeType == RT_Handle || destTypeType == RT_Pointer) && (sourceTypeType == RT_Handle || sourceTypeType == RT_Pointer) )
		{
			// read pointed types
			const IRTTIType* sourcePtrType = static_cast< const IRTTIPointerTypeBase* >( sourceType )->GetPointedType();
			const IRTTIType* destPtrType = static_cast< const IRTTIPointerTypeBase* >( destType )->GetPointedType();

			// compare pointed types to check inheritance
			if ( sourcePtrType && destPtrType && sourcePtrType->GetType() == RT_Class && destPtrType->GetType() == RT_Class )
			{
				const CClass* sourceClass = static_cast< const CClass* >( sourcePtrType );
				const CClass* destClass = static_cast< const CClass* >( destPtrType );

				if ( sourceClass->IsA( IScriptable::GetStaticClass() ) )
				{
					const IScriptable* obj = NULL;
					if( sourceTypeType == RT_Handle )
					{
						obj = static_cast< const IScriptable* >( static_cast< const CRTTIHandleType* >( sourceType )->GetPointed( const_cast< void* >( sourceData ) ) );
					}
					else if ( sourceTypeType == RT_Pointer )
					{
						obj = static_cast< const IScriptable* >( static_cast< const CRTTIPointerType* >( sourceType )->GetPointed( sourceData ) );
					}
					if ( obj && obj->IsA( destClass ) )
					{
						if ( destTypeType == RT_Handle )
						{
							THandle< IScriptable > h( obj );
							prop->Set( container, &h );
						}
						else
						{
							prop->Set( container, &obj );
						}
						return true;
					}
				}
			}
		}
		return false;
	}



	// Can property be tranlated from one type to other
	Bool IsPropertyTranslatable( const IRTTIType* typeDefinition, const IRTTIType* typeInstance )
	{
		if ( typeDefinition == typeInstance )
		{
			return true;
		}
		if ( typeDefinition->IsPointerType() && typeInstance->IsPointerType() )
		{
			if ( SRTTI::GetInstance().CanCast( typeDefinition, typeInstance ) )
			{
				return true;
			}
		}
		else if (
			TBehTreeVarMatch< CBehTreeValFloat >( typeDefinition, typeInstance ) 
			|| TBehTreeVarMatch< CBehTreeValBool >( typeDefinition, typeInstance )
			|| TBehTreeVarMatch< CBehTreeValInt >( typeDefinition, typeInstance ) 
			|| TBehTreeVarMatch< CBehTreeValCName >( typeDefinition, typeInstance )
			|| TBehTreeVarMatch< CBehTreeValEMoveType >( typeDefinition, typeInstance )
			|| TBehTreeVarMatch< CBehTreeValEExplorationType >( typeDefinition, typeInstance )			
			|| TBehTreeVarMatch< CBehTreeValEntityHandle >( typeDefinition, typeInstance )
			|| TBehTreeVarMatch< CBehTreeValSteeringGraph >( typeDefinition, typeInstance )
			|| TBehTreeVarMatch< CBehTreeValFormation >( typeDefinition, typeInstance )
			|| TBehTreeVarMatch< IBehTreeValueEnum >( typeDefinition, typeInstance )
			|| TBehTreeVarMatch< CBehTreeValString >( typeDefinition, typeInstance )
			)
		{
			return true;
		}
		return false;
	}

	// Translate definition property into instance property
	void Translate( const IScriptable* definition, IScriptable* instance, const CProperty* propDefinition, const CProperty* propInstance, CBehTreeSpawnContext& context )
	{
		const IRTTIType* typeDefinition = propDefinition->GetType();
		const IRTTIType* typeInstance = propInstance->GetType();

		if ( typeDefinition == typeInstance )
		{
			propInstance->Set( instance, propDefinition->GetOffsetPtr( definition ) );
			return;
		}
		if ( typeDefinition->IsPointerType() && typeInstance->IsPointerType() )
		{
			propInstance->Set( instance, propDefinition->GetOffsetPtr( definition ) );
			return;
		}
		else if (
			TConvertProperty< CBehTreeValFloat >( definition, instance, propDefinition, propInstance, context ) 
			|| TConvertProperty< CBehTreeValBool >( definition, instance, propDefinition, propInstance, context )
			|| TConvertProperty< CBehTreeValInt >( definition, instance, propDefinition, propInstance, context ) 
			|| TConvertProperty< CBehTreeValCName >( definition, instance, propDefinition, propInstance, context )
			|| TConvertProperty< CBehTreeValEMoveType >( definition, instance, propDefinition, propInstance, context )
			|| TConvertProperty< CBehTreeValEExplorationType >( definition, instance, propDefinition, propInstance, context )			
			|| TConvertProperty< CBehTreeValEntityHandle >( definition, instance, propDefinition, propInstance, context )
			|| TConvertProperty< CBehTreeValSteeringGraph >( definition, instance, propDefinition, propInstance, context )
			|| TConvertProperty< CBehTreeValFormation >( definition, instance, propDefinition, propInstance, context )
			|| TConvertProperty< IBehTreeValueEnum >( definition, instance, propDefinition, propInstance, context )
			|| TConvertProperty< CBehTreeValString >( definition, instance, propDefinition, propInstance, context )
			)
		{
			return;
		}
		ASSERT( TXT("AI. Problem while converting properties. Pretty weird as we collect only matching properties."))
		ASSUME( false );
	}
};