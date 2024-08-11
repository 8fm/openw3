#include "build.h"
#include "propertyRecurrentBrowser.h"

SPropertyRecurrentBrowserConf::SPropertyRecurrentBrowserConf( CClass* type, TDynArray< const CObject* >* parents, TDynArray< void* >* instances, TDynArray< const CClass* >* filterOut )
		: m_onlyEditable( true )
		, m_filterOut( filterOut )
		, m_desiredParentType( CObject::GetStaticClass() )
		, m_type( type )
		, m_parents( parents )
		, m_instances( instances )
{
}

void CPropertyRecurrentBrowser::Find( CObject* obj, const SPropertyRecurrentBrowserConf& conf  )
{
	ASSERT( conf.m_type->GetType() == RT_Class );
	TSet< void* > visited;
	Find( true, *obj->GetClass(), obj, NULL, visited, conf );
}
void CPropertyRecurrentBrowser::Find( Bool canUseObjProperty, const IRTTIType& objectClass, void* obj, CObject* parent, TSet< void* >& visited, const SPropertyRecurrentBrowserConf& conf )
{
	if ( obj == NULL || visited.Exist( obj ) ) 
	{
		return;
	}

	switch ( objectClass.GetType() )
	{
	case RT_Class :
		{
			visited.Insert( obj );
			if ( conf.m_filterOut )
			{
				for ( TDynArray< const CClass* >::const_iterator it = conf.m_filterOut->Begin(); it != conf.m_filterOut->End(); ++it )
				{
					if ( ( ( CClass* ) &objectClass )->IsA( *it ) ) 
					{
						return;
					}
				}
			}
			if ( ( ( CClass* ) &objectClass )->IsA( conf.m_type ) && ( canUseObjProperty || !conf.m_onlyEditable ) )
			{
				conf.m_parents->PushBack( parent );
				conf.m_instances->PushBack( obj );
			}
			const auto& properties = ( ( const CClass* ) &objectClass )->GetCachedProperties();
			for ( Uint32 i = 0; i < properties.Size(); i++ )
			{
				CProperty* prop = properties[i];
				void* propData = prop->GetOffsetPtr( obj );
				if ( ( ( CClass* ) &objectClass )->IsA( conf.m_desiredParentType ) )
				{
					Find( prop->IsEditable(), *prop->GetType(), propData, (CObject*)obj, visited, conf );
				}
				else
				{
					Find( prop->IsEditable(), *prop->GetType(), propData, parent, visited, conf );
				}
			}
		}
		break;
	case RT_Pointer:
		{
			//CRTTIPointerType* ht = (CRTTIPointerType*)&objectClass;
			//Find( canUseObjProperty, *ht->GetPointedType(), * ( void** )obj, parent, visited, conf );

			CRTTIPointerType* ht = (CRTTIPointerType*)&objectClass;
			CObject* object = (CObject*)(ht->GetPointed( obj ));
			CClass* objectClass = object ? object->GetClass() : NULL;
			Find( canUseObjProperty, *objectClass, object, parent, visited, conf );
		}
		break;
	case RT_SoftHandle :
		{
			CRTTISoftHandleType* ht = (CRTTISoftHandleType*)&objectClass;
			BaseSoftHandle& handle = * ( BaseSoftHandle* )( obj );
			Find( canUseObjProperty, *ht->GetPointedType(), handle.Get(), parent, visited, conf );
			break;
		}
	case RT_Handle :
		{
			CRTTIHandleType* ht = (CRTTIHandleType*)&objectClass;
			BaseSafeHandle& handle = * ( BaseSafeHandle* )( obj );
			Find( canUseObjProperty, *ht->GetPointedType(), handle.Get(), parent, visited, conf );
			break;
		}
	case RT_Array :
		{
			CRTTIArrayType* at = (CRTTIArrayType*)&objectClass;
			for ( Uint32 i = 0; i < at->GetArraySize( obj ); ++i )
			{
				Find( canUseObjProperty, *at->GetInnerType(), at->GetArrayElement( obj, i ), parent, visited, conf );
			}
			break;
		}
	case RT_NativeArray:
		{
			CRTTINativeArrayType* at = (CRTTINativeArrayType*)&objectClass;
			for ( Uint32 i = 0; i < at->GetArraySize( obj ); ++i )
			{
				Find( canUseObjProperty, *at->GetInnerType(), at->GetArrayElement( obj, i ), parent, visited, conf );
			}
			break;
		}
	default:
		break;
	}
}