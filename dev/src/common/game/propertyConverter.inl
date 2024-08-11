/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

namespace PropertyConverter
{


	template <> struct TNumericPropertiesConverter< Float > : public SFloatNumericPropertiesConverter { };
	template <> struct TNumericPropertiesConverter< Int32 > : public SIntNumericPropertiesConverter { };
	template <> struct TNumericPropertiesConverter< Bool > : public TGenericIntPropertiesConverter< Bool > { };
	template <> struct TNumericPropertiesConverter< Uint32 > : public TGenericIntPropertiesConverter< Uint32 > { };
	template <> struct TNumericPropertiesConverter< Int16 > : public TGenericIntPropertiesConverter< Int16 > { };
	template <> struct TNumericPropertiesConverter< Uint16 > : public TGenericIntPropertiesConverter< Uint16 > { };
	template <> struct TNumericPropertiesConverter< Int8 > : public TGenericIntPropertiesConverter< Int8 > { };
	template <> struct TNumericPropertiesConverter< Uint8 > : public TGenericIntPropertiesConverter< Uint8 > { };


	template < class T >
	struct TPointerPropertiesConverter< T* >
	{
		static Bool SupportType()
		{
			return true;
		}
		static Bool ConvertValue( const void* sourcePtr, const IRTTIType* sourceType, T*& outVal )
		{
			ERTTITypeType sourceTypeType = sourceType->GetType();
			if ( sourceTypeType == RT_Handle )
			{
				const CRTTIHandleType* sourceTypeHandle = static_cast< const CRTTIHandleType* >( sourceType );
				outVal = static_cast< T* >( sourceTypeHandle->GetPointed( const_cast< void* >( sourcePtr ) ) );
				return true;
			}
			else if ( sourceTypeType == RT_Pointer )
			{
				const CRTTIPointerType* sourceTypePtr = static_cast< const CRTTIPointerType* >( sourceType );
				outVal = static_cast< T* >( sourceTypePtr->GetPointed( const_cast< void* >( sourcePtr ) ) );
				return true;
			}
			return false;
		}

		static Bool ConvertPointerType( const IScriptable* container, const IRTTIType* destType, const IRTTIType* sourceType, CProperty* prop, T*& outVal )
		{
			ERTTITypeType sourceTypeType = sourceType->GetType();
			ERTTITypeType destTypeType = destType->GetType();

			if ( destTypeType == RT_Pointer && (sourceTypeType == RT_Handle || sourceTypeType == RT_Pointer) )
			{
				// read pointed types
				const IRTTIType* sourcePtrType = static_cast< const IRTTIPointerTypeBase* >( sourceType )->GetPointedType();
				const IRTTIType* destPtrType = static_cast< const IRTTIPointerTypeBase* >( destType )->GetPointedType();

				// compare pointed types to check inheritance
				if ( sourcePtrType && destPtrType && sourcePtrType->GetType() == RT_Class && destPtrType->GetType() == RT_Class )
				{
					const CClass* sourceClass = static_cast< const CClass* >( sourcePtrType );
					const CClass* destClass = static_cast< const CClass* >( destPtrType );

					if ( sourceClass->IsA( destClass ) )
					{
						// read type and save property
						if ( sourceTypeType == RT_Handle )
						{
							THandle< IScriptable > valHandle;
							prop->Get( const_cast< IScriptable* >( container ), &valHandle );
							IScriptable* ptrVal = valHandle.Get();
							outVal = ptrVal ? ( T* ) ptrVal : NULL;
						}
						else // sourceTypeType == RT_Pointer
						{
							prop->Get( const_cast< IScriptable* >( container ), &outVal );
						}
						return true;
					}
				}
			}
			return false;
		}

	};

	template < class T >
	struct TPointerPropertiesConverter< THandle< T > >
	{
		static Bool SupportType()
		{
			return true;
		}
		static Bool ConvertValue( const void* sourcePtr, const IRTTIType* sourceType, THandle< T >& outVal )
		{
			ERTTITypeType sourceTypeType = sourceType->GetType();
			if ( sourceTypeType == RT_Handle )
			{
				const CRTTIHandleType* sourceTypeHandle = static_cast< const CRTTIHandleType* >( sourceType );
				outVal = static_cast< T* >( sourceTypeHandle->GetPointed( const_cast< void* >( sourcePtr ) ) );
				return true;
			}
			else if ( sourceTypeType == RT_Pointer )
			{
				const CRTTIPointerType* sourceTypePtr = static_cast< const CRTTIPointerType* >( sourceType );
				outVal = static_cast< T* >( sourceTypePtr->GetPointed( const_cast< void* >( sourcePtr ) ) );
				return true;
			}
			return false;
		}

		static Bool ConvertPointerType( const IScriptable* container, const IRTTIType* destType, const IRTTIType* sourceType, CProperty* prop, THandle< T >& outVal )
		{
			ERTTITypeType sourceTypeType = sourceType->GetType();
			ERTTITypeType destTypeType = destType->GetType();

			if ( destTypeType == RT_Handle && (sourceTypeType == RT_Handle || sourceTypeType == RT_Pointer) )
			{
				// read pointed types
				const IRTTIType* sourcePtrType = static_cast< const IRTTIPointerTypeBase* >( sourceType )->GetPointedType();
				const IRTTIType* destPtrType = static_cast< const IRTTIPointerTypeBase* >( destType )->GetPointedType();

				// compare pointed types to check inheritance
				if ( sourcePtrType && destPtrType && sourcePtrType->GetType() == RT_Class && destPtrType->GetType() == RT_Class )
				{
					const CClass* sourceClass = static_cast< const CClass* >( sourcePtrType );
					const CClass* destClass = static_cast< const CClass* >( destPtrType );

					if ( sourceClass->IsA( destClass ) )
					{
						// read type and save property
						if ( sourceTypeType == RT_Handle )
						{
							prop->Get( const_cast< IScriptable* >( container ), &outVal );
						}
						else // sourceTypeType == RT_Pointer
						{
							T* p;
							prop->Get( const_cast< IScriptable* >( container ), &p );
							outVal = p;
						}
						return true;
					}
				}
			}
			return false;
		}
	};

	template < class T >
	struct TArrayPropertiesConverter< TDynArray< T > >
	{
		static RED_INLINE Bool ConvertArrayType( const IScriptable* container, const IRTTIType* destType, const IRTTIType* sourceType, CProperty* prop, TDynArray< T >& outVal )
		{
			if ( !TPointerPropertiesConverter< T >::SupportType() )
			{
				return false;
			}

			ERTTITypeType sourceTypeType = sourceType->GetType();
			ERTTITypeType destTypeType = destType->GetType();

			if ( destTypeType == RT_Array && sourceTypeType == RT_Array )
			{
				const CRTTIArrayType* sourceArrayType = static_cast< const CRTTIArrayType* >( sourceType );
				const CRTTIArrayType* destArrayType = static_cast< const CRTTIArrayType* >( destType );
				// read array types
				const IRTTIType* sourceInnerType = sourceArrayType->GetInnerType();
				const IRTTIType* destInnerType = destArrayType->GetInnerType();

				ERTTITypeType sourceInnerTypeType = sourceInnerType->GetType();
				ERTTITypeType destInnerTypeType = sourceInnerType->GetType();

				if ( (sourceInnerTypeType == RT_Pointer || sourceInnerTypeType == RT_Handle) &&
					(destInnerTypeType == RT_Pointer || destInnerTypeType == RT_Handle) )
				{
					// read pointed types
					const IRTTIType* sourcePtrType = static_cast< const IRTTIPointerTypeBase* >( sourceInnerType )->GetPointedType();
					const IRTTIType* destPtrType = static_cast< const IRTTIPointerTypeBase* >( destInnerType )->GetPointedType();

					// compare pointed types to check inheritance
					if ( sourcePtrType && destPtrType && sourcePtrType->GetType() == RT_Class && destPtrType->GetType() == RT_Class )
					{
						const CClass* sourceClass = static_cast< const CClass* >( sourcePtrType );
						const CClass* destClass = static_cast< const CClass* >( destPtrType );

						if ( sourceClass->IsA( destClass ) )
						{
							const void* dataBuffer = prop->GetOffsetPtr( container );
							//sourceArrayType->Construct( dataBuffer );
							//prop->Get( container, dataBuffer );

							Uint32 arraySize = sourceArrayType->GetArraySize( dataBuffer );
							outVal.Resize( arraySize );

							for ( Uint32 i = 0; i < arraySize; ++i )
							{
								const void* arrayFieldData = sourceArrayType->GetArrayElement( dataBuffer, i );
								TPointerPropertiesConverter< T >::ConvertValue( arrayFieldData, sourceInnerType, outVal[ i ] );
							}

							//sourceArrayType->Destruct( dataBuffer );
							return true;
						}
					}
				}
			}

			return false;
		}
	};

	template < class T >
	struct TTypeSpecifiedCode< T* >
	{
		static RED_INLINE Bool PostConversionAcceptance( const T* val )
		{
			return val != NULL;
		}
	};

	template < class T >
	struct TTypeSpecifiedCode< THandle< T > >
	{
		static RED_INLINE Bool PostConversionAcceptance( const THandle< T >& val )
		{
			return val.Get() != NULL;
		}
	};

	template < class T >
	Bool ConvertProperty( const IScriptable* obj, CProperty* prop, T& outVal )
	{
		// check property type
		static const IRTTIType* destType( SRTTI::GetInstance().FindType( GetTypeName< T >() ) );
		const IRTTIType* sourceType = prop->GetType();
		if ( destType == sourceType )
		{
			prop->Get( const_cast< IScriptable* >( obj ), &outVal );
			return true;
		}

		// TODO: Think if we should have conversion numerics->enums (but maybe not)
		//if ( destType->GetType() == RT_Enum )
		//{
		//
		//}

		// conversion enums->numerics (only one sided) & between numerical types
		if ( TNumericPropertiesConverter< T >::ConvertNumericType( obj, destType, sourceType, prop, outVal ) )
		{
			return true;
		}

		// conversion pointers <-> handles + inheritance
		if ( TPointerPropertiesConverter< T >::ConvertPointerType( obj, destType, sourceType, prop, outVal ) )
		{
			return true;
		}

		if ( TArrayPropertiesConverter< T >::ConvertArrayType( obj, destType, sourceType, prop, outVal ) )
		{
			return true;
		}

		return false;
	}

	






};