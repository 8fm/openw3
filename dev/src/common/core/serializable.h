#pragma once

#include "class.h"
#include "classBuilder.h"

#include "handleMap.h"

class IFile;
class IProperty;

//---------------------------------------------------------------------------
/// Basic serializable class - not CObject based
//---------------------------------------------------------------------------

#include "referencable.h"

///--------------------------------------------------------------------------

/// All serializable objects can handle dynamic properties, just return this interface in QueryDynamicPropertiesSupplier() method
class IDynamicPropertiesSupplier
{
public:
	virtual ~IDynamicPropertiesSupplier() {};

	//! Get names of dynamic properties
	virtual void GetDynamicProperties( TDynArray< CName >& properties ) const = 0;

	//! Read value of dynamic property
	virtual Bool ReadDynamicProperty( const CName& propName, class CVariant& propValue ) const = 0;

	//! Write value of dynamic property
	virtual Bool WriteDynamicProperty( const CName& propName, const class CVariant& propValue ) = 0;

	//! Serialize dynamic properties for Garbage Collecting
	virtual void SerializeDynamicPropertiesForGC( IFile& file ) = 0;
};

///--------------------------------------------------------------------------

/// Basic serializable class
class ISerializable : public IReferencable
{
	DECLARE_RTTI_SIMPLE_CLASS( ISerializable );

public:
	ISerializable();
	virtual ~ISerializable();

	// Called before object is dependency mapped ( much earlier than OnPreSave() )
	virtual void OnPreDependencyMap( IFile& mapper ) { RED_UNUSED(mapper); };

	// Called before object is saved
	virtual void OnPreSave() {};

	// Called after object is loaded
	virtual void OnPostLoad() {};

	// Called before object's property is changed in the editor
	virtual void OnPropertyPreChange( IProperty* property ) { RED_UNUSED(property); };

	// Called after object's property is changed in the editor
	virtual void OnPropertyPostChange( IProperty* property ) { RED_UNUSED(property); };

	// Called when property is externally changed for eg. by animated props system
	virtual void OnPropertyExternalChanged( const CName& propertyName ) { RED_UNUSED(propertyName); };

	// Object serialization interface
	virtual void OnSerialize( class IFile& file );

	// Property was read from file that is no longer in the object, returns true if data was handled
	virtual Bool OnPropertyMissing( CName propertyName, const CVariant& readValue );

	// Property was read from file that has different type than current property, returns true if data was handled
	virtual Bool OnPropertyTypeMismatch( CName propertyName, IProperty* existingProperty, const CVariant& readValue );

	// Called to check if we can save given property
	virtual Bool OnPropertyCanSave( IFile& file, CName propertyName, IProperty* propertyObject ) const;

	// Clone this object
	THandle< ISerializable > Clone( ISerializable* newParent, Bool cloneWithTransientObjects = true, Bool cloneWithReferencedObjects = true ) const;

#ifndef NO_EDITOR
	// Called after object was constructed in the editor
	virtual void OnCreatedInEditor();
#endif

#ifndef NO_DEBUG_PAGES
	// Provide additional extended debug information about the object
	virtual void OnDebugPageInfo( class CDebugPageHTMLDocument& doc );
#endif

public:
	//! Get base object data (NULL for serializable types not null for CObjects)
	virtual const void* GetBaseObjectData() const;

	//! Get base object class (NULL for serializable types not null for CObjects)
	virtual CClass* GetBaseObjectClass() const;

public:
	//! Query an interface for accessing dynamic properties
	virtual IDynamicPropertiesSupplier* QueryDynamicPropertiesSupplier() { return NULL; }

	// Query dynamic properties supplier
	virtual const IDynamicPropertiesSupplier* QueryDynamicPropertiesSupplier() const { return NULL; }

	// Calc object dynamic data size in memory
	virtual Uint32 CalcObjectDynamicDataSize() const { return 0; }

	// Can we ignore this object in GC tests ?
	virtual const Bool CanIgnoreInGC() const { return false; }

	// Get object index (implemented only for CObjects), 0 is invalid index
	virtual const Uint32 GetObjectIndex() const { return 0; }

public:
	static ISerializable* ANY_PARENT;

	// Get serialization parent object (similar to old CObject::GetParent())
	// When ISerialable objects are saved only the ones reachable from within the saved hierarchy are mapped and saved
	// You can return ISerializable::ANY_PARENT to indicate that this ISerializable should be saved regardless of the actual parent
	// If you return NULL here the ISerializable object will be saved only if it's specified directly in the list of object to save
	virtual ISerializable* GetSerializationParent() const { return ISerializable::ANY_PARENT; }

	// Called when deserializing ISerializable object - allows the object to restore the parent
	virtual void RestoreSerializationParent( ISerializable* parent ) { RED_UNUSED( parent ); }

	// Trivial test to check if ISerializable is an object
	virtual const Bool IsObject() const { return false; }

private:
	//! Serialize (save/load) dynamic properties
	void SerializeDynamicProperties( IFile& file );

public:
	//! RTTI class check
	Bool IsA( const CClass* rttiClass ) const
	{
		return GetClass()->IsA( rttiClass );
	}

	//! RTTI exact class check
	Bool IsExactlyA( const CClass* rttiClass ) const
	{
		return GetClass() == rttiClass;
	}

	//! RTTI class check
	template< class T >
	Bool IsA() const; // ctremblay: I had to move this away to brak cycling dependency

	//! RTTI class check
	template< class T >
	Bool IsExactlyA() const;
};

//-----------------------------------------------------------------------------

BEGIN_ABSTRACT_CLASS_RTTI( ISerializable );
	PARENT_CLASS( IReferencable );
END_CLASS_RTTI();

//-----------------------------------------------------------------------------
