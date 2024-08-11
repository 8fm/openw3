/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "variant.h"

/// Flags that modify property behavior
enum EPropertyFlags
{
	PF_Editable				= FLAG( 0 ),	//!< Property is visible in the editor's property browser
	PF_ReadOnly				= FLAG( 1 ),	//!< Property is read only
	PF_Inlined				= FLAG( 2 ),	//!< Inline property edition ( for object properties only )
	PF_NotSerialized		= FLAG( 3 ),	//!< Use this flag to grant RTTI access to the field, but prevent serializing it
	PF_NotCooked			= FLAG( 4 ),	//!< Property with this flag is not cooked to final build packages
	PF_Scripted				= FLAG( 5 ),	//!< Property is script property of CLASS
	PF_FuncRetValue			= FLAG( 6 ),	//!< Return property of function
	PF_FuncParam			= FLAG( 7 ),	//!< Function parameter
	PF_FuncLocal			= FLAG( 8 ),	//!< Function local variable
	PF_FuncOutParam			= FLAG( 9 ),	//!< Function parameter that is passed by reference ( can by modified by function )
	PF_FuncOptionaParam		= FLAG( 10 ),	//!< Function parameter is optional, does not need to be specified
	PF_FuncSkipParam		= FLAG( 11 ),	//!< Function parameter which evaluation can be skipped, used in native functions
	PF_Config				= FLAG( 12 ),	//!< Property is saved/loaded from config
	PF_Exported				= FLAG( 13 ),	//!< Property was exported from C++ and can be used in script code
	PF_Native				= FLAG( 14 ),	//!< Property is defined in C++
	PF_Saved				= FLAG( 15 ),	//!< Property that will be saved to a gamesave file
	PF_Private				= FLAG( 16 ),	//!< Property is private
	PF_Protected			= FLAG( 17 ),	//!< Property is protected
	PF_Public				= FLAG( 18 ),	//!< Property is public
	PF_AutoBind				= FLAG( 19 ),	//!< Property is automatically bindable
	PF_AutoBindOptional		= FLAG( 20 ),	//!< Failed autobind will not result in runtime script errors

	PF_AccessModifiers		= PF_Private | PF_Protected | PF_Public, 
};

class IRTTIType;
class IPropertySetter;
class CEnum;
class CClass;

#define IProperty CProperty

/// Property value setter
class IPropertySetter
{
public:
	//! Virtual destructor
	virtual ~IPropertySetter() {};

	//! Set the value of property
	virtual void SetValue( void* context, IRTTIType* type, const void* valueData ) const=0;
};

/// Class property
class CProperty : public IRTTIBaseObject
{
friend class CRTTISerializer;

protected:
	IRTTIType*			m_type;							//!< Data type of the property
	CName				m_name;							//!< Name of the property
	CClass*				m_parent;						//!< Parent class that owns this property
	Uint32				m_offset;						//!< Offset to data in the parent class
	Uint32				m_flags;						//!< Flags
	Uint64				m_hash;							//!< Property hash

#ifndef NO_EDITOR_PROPERTY_SUPPORT
protected:
	String				m_hint;							//!< Editor only hint
	String				m_customEditor;					//!< Name of the custom editor to use
	Bool				m_arrayCustomEditor;			//!< Name of the custom editor for array
	IPropertySetter*	m_setter;						//!< Property value setter
#endif

public:
	//! Get data type
	RED_INLINE IRTTIType* GetType() const { return m_type; }

	//! Get class this property was defined in
	RED_INLINE CClass* GetParent() const { return m_parent; }	

	//! Get name of the property
	RED_INLINE const CName& GetName() const { return m_name; }

	//! Get offset of the property to the data
	RED_INLINE Uint32 GetDataOffset() const { return m_offset; }	

	//! Get property flags
	RED_INLINE Uint32 GetFlags() const { return m_flags; }

	//! Add property flag
	RED_INLINE void AddFlag( EPropertyFlags flag ) { m_flags |= flag; }

public:
	//! Is the property editable by the user
	RED_INLINE Bool IsEditable() const { return ( m_flags & PF_Editable ) != 0; }

	//! Is the property exported to script code ?
	RED_INLINE Bool IsExported() const { return ( m_flags & PF_Exported ) != 0; }

	//! Is the property defined in C++ code
	RED_INLINE Bool IsNative() const { return ( m_flags & PF_Native ) != 0; }

	//! Is the property read only ( user can see the value but cannot modify it )
	RED_INLINE Bool IsReadOnly() const { return ( m_flags & PF_ReadOnly ) != 0; }

	//! Is the property inlined ( objects can be created inside parent objects )
	RED_INLINE Bool IsInlined() const { return ( m_flags & PF_Inlined ) != 0; }

	//! Should the property be serialized to file
	RED_INLINE Bool IsSerializable() const { return ( m_flags & PF_NotSerialized ) == 0; } 

	//! Is this property saved in cooked builds ?
	RED_INLINE Bool IsSerializableInCookedBuilds() const { return ( m_flags & PF_NotCooked ) == 0; } 

	//! Is this property scripted ?
	RED_INLINE Bool IsScripted() const { return ( m_flags & PF_Scripted ) != 0; }

	//! Is this property saved to a gamesave ?
	RED_INLINE Bool IsSaved() const { return ( m_flags & PF_Saved ) != 0; }

	//! Is this property from function ?
	RED_INLINE Bool IsInFunction() const { return ( m_flags & ( PF_FuncLocal | PF_FuncParam ) ) != 0; }

	//! Is this property a function local ?
	RED_INLINE Bool IsFuncLocal() const { return ( m_flags & ( PF_FuncLocal ) ) != 0; }

	//! Is the property saved to configuration 
	RED_INLINE Bool IsConfig() const { return ( m_flags & PF_Config ) != 0; }

	//! Is the property private
	RED_INLINE Bool IsPrivate() const { return ( m_flags & PF_Private ) != 0; }

	//! Is the property protected
	RED_INLINE Bool IsProtected() const { return ( m_flags & PF_Protected ) != 0; }

	//! Is the property public
	RED_INLINE Bool IsPublic() const { return ( m_flags & PF_Public ) != 0; }

	//! Is this property auto bindable
	RED_INLINE Bool IsAutoBindable() const { return ( m_flags & PF_AutoBind ) != 0; }

	//! Can this property fail autobinding without warning messages ?
	RED_INLINE Bool IsAutoBindOptional() const { return ( m_flags & PF_AutoBindOptional ) != 0; }	

	//! Get property hash (global ID for the property)
	RED_INLINE const Uint64 GetHash() const { return m_hash; }

#ifndef NO_EDITOR_PROPERTY_SUPPORT

public:
	//! Get editor only hint that is displayed with the property
	RED_INLINE const String& GetHint() const { return m_hint; }

	//! Does this property have custom editor for it's value ?
	RED_INLINE Bool HasCustomEditor() const { return !m_customEditor.Empty(); }

	//! Does this property have custom editor for array value it's in ?
	RED_INLINE Bool HasArrayCustomEditor() const { return m_arrayCustomEditor; } 

	//! Get the name of property custom editor
	RED_INLINE const String& GetCustomEditorType() const { return m_customEditor; }

	//! Get the property value setter
	RED_INLINE IPropertySetter* GetSetter() const { return m_setter; }

#endif

public:
	CProperty( IRTTIType *type, 
			   CClass* owner, 			   
			   Uint32 offset, 			   
			   const CName& name, 
			   const String& hint, 
			   Uint32 flags, 
			   const String &editorType = String::EMPTY,
			   Bool arrayCustomEditor = false
			   );

public:
	//! Get min range for ranged properties
	//! dex_fix: this is kinda hacky
	virtual Float GetRangeMin() const
	{
		return -FLT_MAX;
	}

	//! Get max range for ranged properties
	//! dex_fix: this is kinda hacky
	virtual Float GetRangeMax() const
	{
		return FLT_MAX;
	}

	//! Set property value for given object
	void Set( void *object, const void *buffer ) const
	{
		// Use value setter if given
#ifndef NO_EDITOR_PROPERTY_SUPPORT
		if ( m_setter )
		{
			m_setter->SetValue( object, m_type, buffer );
		}
		else
#endif
		{
			void* ptr = GetOffsetPtr( object );
			m_type->Copy( ptr, buffer );
		}
	}

	//! Get property value for given object
	void Get( const void *object, void *buffer ) const
	{
		const void* ptr = GetOffsetPtr( object );
		m_type->Copy( buffer, ptr );
	}

	//! Assign hash to the property
	void AssignHash( const CName className )
	{
		RED_FATAL_ASSERT( m_hash == 0, "Property '%ls' in '%ls' already has a hash", m_name.AsChar(), className.AsChar() );
		m_hash = ComputePropertyHash( className, m_name );
	}

public:
	//! Get offset to data
	void* GetOffsetPtr( void* base ) const;

	//! Get offset to data
	const void* GetOffsetPtr( const void* base ) const;

public:
	//! Calculate data layout of properties
	static Uint32 CalcDataLayout( const TDynArray< CProperty*, MC_RTTI >& properties, const Uint32 initialOffset, const Uint32 initialAlignment = 4 );

	//! Install property setter
	static void InstallPropertySetter( CClass* objectClass, const Char* propertyName, IPropertySetter* setter );

	//! Disable cooking for property
	static void ChangePropertyFlag( CClass* objectClass, const CName& propertyName, Uint32 clearFlags, Uint32 setFlags );

	//! Compute property hash
	static Uint64 ComputePropertyHash( const CName className, const CName propertyName );
};

/// Range property - has the ability to clamp inputed values to min/max range
template<typename T>
class CRangedProperty : public CProperty
{
protected:
	T	m_rangeMin;		//!< Minimal value of property
	T	m_rangeMax;		//!< Maximum value of property

public:
	CRangedProperty( IRTTIType *type, 
				     CClass* owner, 			   
				     Uint32 offset, 			   
				     const CName& name, 
				     const String& hint, 
				     Uint32 flags,
					 T* /* tag */, // used only to match the template in classbuilder macro
					 T rangeMin = (T)-FLT_MAX,
					 T rangeMax = (T)FLT_MAX, 
					 const String &editorType = String::EMPTY,
					 Bool arrayCustomEditor = false
					 )
		: CProperty( type, owner, offset, name, hint, flags, editorType, arrayCustomEditor )
		, m_rangeMin( rangeMin )
		, m_rangeMax( rangeMax )
	{
	}

	virtual void Set( void* object, const void* buffer) 
	{ 
		// Clamp the value to set
		T valueToSet = *((T*)buffer);
		valueToSet = Clamp( valueToSet, m_rangeMin, m_rangeMax );

		// Pass to base class
		CProperty::Set( object, &valueToSet );
	}

	//! Get min range for ranged properties
	//! dex_fix: this is kinda hacky
	virtual Float GetRangeMin() const
	{
		return (Float)m_rangeMin;
	}

	//! Get max range for ranged properties
	//! dex_fix: this is kinda hacky
	virtual Float GetRangeMax() const
	{
		return (Float)m_rangeMax;
	}
};

// buffer holding property data
class CPropertyDataBuffer
{
public:
	CPropertyDataBuffer()
		: m_type( nullptr )
	{
	}

	CPropertyDataBuffer( IRTTIType* type )
		: m_type( nullptr )
	{		
		Reset( type );
	}


	CPropertyDataBuffer( CProperty* prop )
		: m_type( nullptr )
	{
		if ( prop )
		{
			Reset( prop->GetType() );
		}
	}

	CPropertyDataBuffer( const CPropertyDataBuffer &buf )
		: m_type( nullptr )
	{
		Reset( buf.m_type );

		if ( m_type )
		{
			m_type->Copy( Data(), buf.Data() );
		}
    }

	CPropertyDataBuffer& operator = ( const CPropertyDataBuffer &buf )
	{
		CPropertyDataBuffer( buf ).SwapWith( *this );
		return *this;
	}

	void Reset( IRTTIType* type )
	{
		if ( m_type )
		{
			m_type->Destruct( Data() );
		}

		m_type = type;

		if ( m_type )
		{
			m_data.Resize( m_type->GetSize() );
			m_type->Construct( Data() );
		}
		else
		{
			m_data.Clear();
		}
	}

	void SwapWith( CPropertyDataBuffer &buf )
	{
		m_data.SwapWith( buf.m_data );
		::Swap( m_type, buf.m_type );
	}

	~CPropertyDataBuffer()
	{
		if ( m_type )
		{
			m_type->Destruct( Data() );
		}
	}

	class CVariant ToVariant() const
	{
		if ( m_type && m_data.Size() )
		{		
			return CVariant( m_type->GetName(), m_data.TypedData() );
		}
		else
		{			
			return CVariant();
		}
	}
	
	void* Data()
	{
		return m_data.Data();
	}

	const void* Data() const
	{
		return m_data.Data();
	}

	IRTTIType* GetType() const
	{ 
		return m_type;	
	}

private:
	IRTTIType* m_type;	
	TDynArray< Uint8 > m_data;
};
