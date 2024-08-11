/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "serializable.h"

#include "dependencyLoader.h"
#include "dependencySaver.h"
#include "memoryFileReader.h"
#include "memoryFileWriter.h"


IMPLEMENT_RTTI_CLASS( ISerializable );

ISerializable* ISerializable::ANY_PARENT = nullptr;

ISerializable::ISerializable()
{
}

ISerializable::~ISerializable()
{
}

void ISerializable::OnSerialize( IFile& file )
{
	// Native (static) properties
	// Although the ISerializable is not used in GC directly we are used by the CObject a lot
	// in order to save some crappy virtualization an if is added here instead.
	if ( file.IsGarbageCollector() )
	{
		// Simple GC only serialization
		GetClass()->SerializeGC( file, this );
	}
	else
	{
		// Full differential serialization
		if ( file.IsReader() )
		{
			GetClass()->SerializeDiff( this, file, this, nullptr, nullptr );
		}
		else
		{
			const void* defaultData = GetBaseObjectData();
			CClass* defaultDataClass = GetBaseObjectClass();
			GetClass()->SerializeDiff( this, file, this, defaultData, defaultDataClass );
		}
	}

	// Dynamic properties
	SerializeDynamicProperties( file );

	// Old flag for script properties (from the times of LUA scripting, 6 years old and still wasting a byte with every saved object)
	if ( !file.IsGarbageCollector() && file.GetVersion() < VER_SERIALIZATION_DATA_CLEANUP )
	{
		// Do we have script properties
		Bool hasScriptProperties = false;
		file << hasScriptProperties;
	}
}

Bool ISerializable::OnPropertyMissing( CName propertyName, const CVariant& readValue )
{
	RED_UNUSED(propertyName); 
	RED_UNUSED(readValue); 
	return false;
}

Bool ISerializable::OnPropertyTypeMismatch( CName propertyName, IProperty* existingProperty, const CVariant& readValue )
{
	RED_UNUSED(propertyName);
	RED_UNUSED(existingProperty);
	RED_UNUSED(readValue);
	return false;
}

Bool ISerializable::OnPropertyCanSave( IFile& file, CName propertyName, IProperty* propertyObject ) const
{
	RED_UNUSED(file);
	RED_UNUSED(propertyName);
	RED_UNUSED(propertyObject);
	return true;
}

#ifndef NO_DEBUG_PAGES
void ISerializable::OnDebugPageInfo( class CDebugPageHTMLDocument& )
{
}
#endif

THandle< ISerializable > ISerializable::Clone( ISerializable* newParent, Bool cloneWithTransientObjects, Bool cloneWithReferencedObjects ) const
{
	TDynArray< Uint8 > buffer;
	// Reserve some memory for object - 1k is safe and should be enough
	buffer.Reserve( 1024 );

	// Save object to memory
	{
		// Setup writer stream
		CMemoryFileWriter writer( buffer );
		CDependencySaver saver( writer, NULL );

		// Setup save context
		DependencySavingContext context( const_cast< ISerializable* >( this ) );
		context.m_saveReferenced = cloneWithReferencedObjects;
		context.m_saveTransient = cloneWithTransientObjects;
		if ( !saver.SaveObjects( context ) )
		{
			return NULL;
		}
	}

	// Load new copy from memory
	{
		CMemoryFileReader reader( buffer, 0 );
		CDependencyLoader loader( reader, NULL );

		// Load objects
		DependencyLoadingContext loadingContext;
		loadingContext.m_parent = newParent;
		loadingContext.m_getAllLoadedObjects = true;
		if ( !loader.LoadObjects( loadingContext ) )
		{
			return NULL;
		}

		// Post load cloned objects
		loader.PostLoad();

		// return loaded object
		return loadingContext.m_loadedObjects[0].GetSerializablePtr();
	}
	return nullptr;
}

#ifndef NO_EDITOR
void ISerializable::OnCreatedInEditor()
{
}
#endif

const void* ISerializable::GetBaseObjectData() const
{
	return GetClass()->GetDefaultObjectImp();
}

CClass* ISerializable::GetBaseObjectClass() const
{
	return GetClass();
}

void ISerializable::SerializeDynamicProperties( IFile& file )
{
	// GC serialization
	if ( file.IsGarbageCollector() )
	{
		// Collect dynamic properties
		IDynamicPropertiesSupplier* dynamicProperties = QueryDynamicPropertiesSupplier();
		if ( NULL != dynamicProperties )
		{
			dynamicProperties->SerializeDynamicPropertiesForGC( file );
		}

		// Nothing more
		return;
	}

	// Dynamic properties
	IDynamicPropertiesSupplier* dynamicProperties = QueryDynamicPropertiesSupplier();
	if ( NULL != dynamicProperties )
	{
		// Writer
		if ( file.IsWriter() )
		{
			// Get properties
			TDynArray< CName > properties;
			dynamicProperties->GetDynamicProperties( properties );

			// Save properties
			for ( Uint32 i=0; i<properties.Size(); i++ )
			{
				CName propName = properties[i];

				// Read value
				CVariant currentValue;
				if ( dynamicProperties->ReadDynamicProperty( propName, currentValue ) )
				{
					if ( currentValue.IsValid() )
					{
						// Write the name and value
						file << propName;

						// Write value
						currentValue.Serialize( file );
					}
				}
			}

			// End of list
			CName endOfList = CName::NONE;
			file << endOfList;
		}
		else if ( file.IsReader() )
		{
			for ( ;; )
			{
				// Read property name
				CName propName;
				file << propName;

				// End of list
				if ( !propName )
				{
					break;
				}

				// Load value
				CVariant propValue;
				propValue.Serialize( file );

				// Restore data
				if ( propValue.IsValid() )
				{
					dynamicProperties->WriteDynamicProperty( propName, propValue );
				}
			}
		}
	}
}
