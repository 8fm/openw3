/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "presets.h"
#include "../../common/core/depot.h"

IFile& operator<<( IFile& file, CEdPresets::PresetEntry& val )
{
	file << val.m_name;
	file << val.m_value;
	return file;
}

CEdPresets::CEdPresets()
	: m_hook( nullptr )
{
}

CEdPresets::~CEdPresets()
{
}

Int32 CEdPresets::FindPreset( const String& name, Bool addNew )
{
	// Scan the array to find the preset
	for ( Int32 i=0; i < m_presets.SizeInt(); ++i )
	{
		if ( m_presets[i].m_name == name )
		{
			return i;
		}
	}

	// Preset not found, return -1 if we weren't asked to add a new one
	if ( !addNew )
	{
		return -1;
	}

	// We were asked to add a new one, so do it now
	m_presets.Grow();
	m_presets.Back().m_name = name;

	// Inform the hook that we've got new presets
	if ( m_hook )
	{
		m_hook->OnPresetsChanged( this );
	}

	// Give back the brand new shiny index
	return m_presets.SizeInt() - 1;
}

Int32 CEdPresets::FindPreset( const String& name ) const
{
	return const_cast< CEdPresets* >( this )->FindPreset( name, false );
}

Int32 CEdPresets::FindKey( Preset& preset, const String& keyName, Bool addNew )
{
	// Scan the preset to find the key
	for ( Int32 i=0; i < preset.m_entries.SizeInt(); ++i )
	{
		if ( preset.m_entries[i].m_name == keyName )
		{
			return i;
		}
	}

	// Key not found, return -1 if we weren't asked to add a new key
	if ( !addNew )
	{
		return -1;
	}

	// We were asked to add a new key, so do it now now now
	preset.m_entries.Grow();
	preset.m_entries.Back().m_name = keyName;

	// Give back the new index
	return preset.m_entries.SizeInt() - 1;
}

Int32 CEdPresets::FindKey( const Preset& preset, const String& keyName ) const
{
	return const_cast< CEdPresets* >( this )->FindKey( const_cast< Preset& >( preset ), keyName, false );
}

Bool CEdPresets::SavePresetToFile( const Preset& preset, const String& absolutePath )
{
	// Try to create a file writer
	IFile* writer = GFileManager->CreateFileWriter( absolutePath, FOF_AbsolutePath|FOF_Buffered );
	if ( writer == nullptr )
	{
		return false;
	}

	// Inform hook
	if ( m_hook != nullptr )
	{ 
		m_hook->OnPresetSaving( this, preset.m_name );
	}

	// Save a version number just in case we change the format later
	Uint32 version = 0xD1C30001;
	*writer << version;

	// Save the preset data
	*writer << const_cast< Preset* >( &preset )->m_name;
	*writer << const_cast< Preset* >( &preset )->m_entries;

	// Done
	delete writer;
	return true;
}

Bool CEdPresets::LoadPresetFromFile( Preset& preset, const String& absolutePath )
{
	// Try to create a file reader
	IFile* reader = GFileManager->CreateFileReader( absolutePath, FOF_AbsolutePath|FOF_Buffered );
	if ( reader == nullptr )
	{
		return false;
	}

	// Load and check the version number
	Uint32 version = 0;
	*reader << version;
	if ( version != 0xD1C30001 )
	{
		delete reader;
		return false;
	}

	// Load the preset data
	*reader << preset.m_name;
	*reader << preset.m_entries;

	// Done
	delete reader;

	// Inform hook
	if ( m_hook != nullptr )
	{ 
		m_hook->OnPresetLoaded( this, preset.m_name );
	}

	return true;
}

void CEdPresets::RemoveAllPresets()
{
	// Loop through all presets so that the hook receives the remove event for all of them
	while ( !m_presets.Empty() )
	{
		RemovePreset( m_presets[0].m_name );
	}
}

void CEdPresets::SetPresetValue( const String& presetName, const String& keyName, const String& keyValue )
{
	// Find or create the preset
	Int32 presetIndex = FindPreset( presetName, true );

	// Find or create the key in the preset
	Int32 keyIndex = FindKey( m_presets[presetIndex], keyName, true );

	// Set the value... (this comment is useless)
	m_presets[presetIndex].m_entries[keyIndex].m_value = keyValue;

	// Inform the hook if we have one
	if ( m_hook )
	{
		m_hook->OnPresetKeySet( this, presetName, keyName, keyValue );
	}
}

void CEdPresets::RemovePresetValue( const String& presetName, const String& keyName )
{
	// Find the preset
	Int32 presetIndex = FindPreset( presetName );
	if ( presetIndex == -1 )
	{
		return;
	}

	// Find the key
	Int32 keyIndex = FindKey( m_presets[presetIndex], keyName );
	if ( keyIndex == -1 )
	{
		return;
	}

	// Remove the key
	m_presets[presetIndex].m_entries.RemoveAt( keyIndex );

	// Inform the hook that the key is gone
	if ( m_hook )
	{
		m_hook->OnPresetKeyRemoved( this, m_presets[presetIndex].m_name, keyName );
	}

	// If the preset is now empty remove it to save space in the UI
	if ( m_presets[presetIndex].m_entries.Empty() )
	{
		RemovePreset( presetName );
	}
}

String CEdPresets::GetPresetValue( const String& presetName, const String& keyName, const String& defaultValue ) const
{
	// Try to locate the preset
	Int32 presetIndex = FindPreset( presetName );

	// Preset not found, return default
	if ( presetIndex == -1 )
	{
		return defaultValue;
	}

	// Preset was found, try to locate the key
	Int32 keyIndex = FindKey( m_presets[presetIndex], keyName );
	
	// No key in sight, return default
	if ( keyIndex == -1 )
	{
		return defaultValue;
	}

	// We've got a preset and a key, return the value
	return m_presets[presetIndex].m_entries[keyIndex].m_value;
}

void CEdPresets::RemovePreset( const String& presetName )
{
	// Find the preset in presetland
	Int32 presetIndex = FindPreset( presetName, false );
	if ( presetIndex == -1 )
	{
		return;
	}

	// Inform the hook about every single key that was removed
	if ( m_hook )
	{
		for ( Uint32 i=0; i < m_presets[presetIndex].m_entries.Size(); ++i )
		{
			m_hook->OnPresetKeyRemoved( this, m_presets[presetIndex].m_name, m_presets[presetIndex].m_entries[i].m_name );
		}
	}

	// Remove it from the presets array
	m_presets.RemoveAt( presetIndex );

	// Inform the hook that the preset is no more
	if ( m_hook )
	{
		m_hook->OnPresetsChanged( this );
	}
}

void CEdPresets::SetPresetFromObject( const String& presetName, const CObject* object, const String& keyPrefix/*=String::EMPTY*/, const TDynArray< CName >* names/*=nullptr */ )
{
	// Grab the object's properties
	TDynArray< CProperty* > properties;
	object->GetClass()->GetProperties( properties );

	// Scan them for editable properties we can save
	for ( auto it=properties.Begin(); it != properties.End(); ++it )
	{
		CProperty* prop = *it;
		if ( !IsPropertySavable( prop ) )
		{
			continue;
		}

		// If we have names, check if the property is one of them and if not skip it
		if ( names && !names->Exist( prop->GetName() ) )
		{
			continue;
		}

		// We have a savable property, let's save it
		const void* propValue = prop->GetOffsetPtr( object );
		String value;

		// We need special support for pointers/handles to resources
		switch ( prop->GetType()->GetType() )
		{
		case RT_Pointer:	// Actually a CResource (checked by IsPropertySavable), store its path
			{
				const CResource* res = *(const CResource**)propValue;
                if ( res != nullptr )
                {
    				SetPresetValue( presetName, keyPrefix + prop->GetName().AsString(), res->GetDepotPath() );
                }
                else
                {
    				SetPresetValue( presetName, keyPrefix + prop->GetName().AsString(), String::EMPTY );
                }
				break;
			}
		case RT_Handle:		// Handle to CResource, store path or empty string
			{
				const THandle< CResource >& res = *(const THandle< CResource >*)propValue;
				if ( res.IsValid() && res.Get() != nullptr )
				{
					SetPresetValue( presetName, keyPrefix + prop->GetName().AsString(), res.Get()->GetDepotPath() );
				}
				else
				{
					SetPresetValue( presetName, keyPrefix + prop->GetName().AsString(), String::EMPTY );
				}
				break;
			}
		default:
			if ( prop->GetType()->ToString( propValue, value ) )
			{
				SetPresetValue( presetName, keyPrefix + prop->GetName().AsString(), value );
			}
		}
	}
}

void CEdPresets::ApplyPresetToObject( const String& presetName, CObject* object, const String& keyPrefix/*=String::EMPTY*/, const TDynArray< CName >* names/*=nullptr */ ) const
{
	// Make sure we can really modify the objec
	if ( !object->MarkModified() )
	{
		return;
	}

	// Grab the object's properties
	TDynArray< CProperty* > properties;
	object->GetClass()->GetProperties( properties );

	// Scan them for editable properties we can load
	for ( auto it=properties.Begin(); it != properties.End(); ++it )
	{
		CProperty* prop = *it;
		if ( !IsPropertySavable( prop ) )
		{
			continue;
		}

		// If we have names, check if the property is one of them and if not skip it
		if ( names && !names->Exist( prop->GetName() ) )
		{
			continue;
		}

		// We have a savable property, let's load it
		String value = GetPresetValue( presetName, keyPrefix + prop->GetName().AsString(), String::EMPTY );

		// We need special support for pointers/handles to resources
		switch ( prop->GetType()->GetType() )
		{
		case RT_Pointer:
			{
				CResource* res = GDepot->LoadResource( value );
				SetPropertyValue( object, prop->GetName().AsString(), res );
				break;
			}
		case RT_Handle:
			{
				CResource* res = GDepot->LoadResource( value );
				BaseSafeHandle handle( res );
				SetPropertyValue( object, prop->GetName().AsString(), handle );
				break;
			}
		default:
			{
				// Convert to an RTTI value
				void* tempValue = RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Default, prop->GetType()->GetSize() );
				prop->GetType()->Construct( tempValue );
				if ( prop->GetType()->FromString( tempValue, value ) )
				{
					// Set the property to the RTTI value
					prop->Set( object, tempValue );
				}

				// Release the value
				prop->GetType()->Destruct( tempValue );
				RED_MEMORY_FREE( MemoryPool_Default, MC_Default, tempValue );
			}
		}
	}
}

void CEdPresets::GetPresetNames( TDynArray< String >& names ) const
{
	// Scan the array and fill the names
	for ( auto it=m_presets.Begin(); it != m_presets.End(); ++it )
	{
		names.PushBack( (*it).m_name );
	}
}

Bool CEdPresets::SavePresetToFile( const String& presetName, const String& absolutePath )
{
	// Find the preset
	Int32 presetIndex = FindPreset( presetName );
	if ( presetIndex == -1 )
	{
		return false;
	}

	// Save it
	return SavePresetToFile( m_presets[presetIndex], absolutePath );
}

Bool CEdPresets::LoadPresetFromFile( const String& presetName, const String& absolutePath )
{
	// Find the preset and load it
	Int32 presetIndex = FindPreset( presetName, true );

	// Try to load the preset
	if ( LoadPresetFromFile( m_presets[presetIndex], absolutePath ) )
	{
		return true;
	}

	// Failed, check if the preset is empty and remove it if so
	if ( m_presets[presetIndex].m_entries.Empty() )
	{
		RemovePreset( presetName );
	}

	return false;
}

Bool CEdPresets::SavePresetFiles( const String& absolutePath, Bool removeExistingPresetFiles )
{
	Bool everythingOk = true;

	// Build the path prefix 
	String prefix = absolutePath;
	if ( !prefix.EndsWith( TXT("\\") ) )
	{
		prefix += TXT("\\");
	}

	// Remove existing presets if we were asked to do so
	if ( removeExistingPresetFiles )
	{
		wxArrayString files;
		wxDir::GetAllFiles( absolutePath.AsChar(), &files, wxT("*.redpreset"), wxDIR_FILES );
		for ( wxArrayString::size_type i=0; i < files.GetCount(); ++i )
		{
			wxRemoveFile( String( files[i].wc_str() ).AsChar() );
		}
	}

	// Save all presets
	for ( auto it=m_presets.Begin(); it != m_presets.End(); ++it )
	{
		if ( !SavePresetToFile( *it, prefix + (*it).m_name + TXT(".redpreset") ) )
		{
			everythingOk = false;
		}
	}

	// Report back if everything went ok
	return everythingOk;
}

Bool CEdPresets::LoadPresetFiles( const String& absolutePath )
{
	Bool gotAny = false;

	// Build the path prefix 
	String prefix = absolutePath;
	if ( !prefix.EndsWith( TXT("\\") ) )
	{
		prefix += TXT("\\");
	}

	// Scan the given directory for redpreset files
	wxArrayString files;
	wxDir::GetAllFiles( absolutePath.AsChar(), &files, wxT("*.redpreset"), wxDIR_FILES );
	for ( wxArrayString::size_type i=0; i < files.GetCount(); ++i )
	{
		String presetAbsolutePath = files[i].wc_str();
		CFilePath path( presetAbsolutePath );

		// Find or create preset
		Int32 presetIndex = FindPreset( path.GetFileName(), true );

		// Load preset
		if ( !LoadPresetFromFile( m_presets[presetIndex], presetAbsolutePath ) )
		{
			// Load failed, delete preset if it is empty
			if ( m_presets[presetIndex].m_entries.Empty() )
			{
				RemovePreset( m_presets[presetIndex].m_name );
				continue;
			}
		}

		// Force the filename as preset's name
		m_presets[presetIndex].m_name = path.GetFileName();

		// Got a preset!
		gotAny = true;
	}

	// Inform hook
	if ( m_hook != nullptr )
	{
		m_hook->OnPresetsChanged( this );
	}

	// Inform our caller if we've got any preset
	return gotAny;
}

void CEdPresets::SetHook( IEdPresetsHook* hook )
{
	m_hook = hook;
}

Bool CEdPresets::IsPropertySavable( CProperty* prop )
{
	// We only support editable, serializable presets
	if ( !prop->IsEditable() || !prop->IsSerializable() )
	{
		return false;
	}

	// Special case for strings
	if ( prop->GetType()->GetName() == CNAME( String ) )
	{
		return true;
	}

	// Special case for resource references
	switch ( prop->GetType()->GetType() )
	{
	case RT_Pointer:
		{
			CRTTIPointerType* pointerType = (CRTTIPointerType*)prop->GetType();
			if ( pointerType->GetPointedType()->IsA< CResource >() )
			{
				return true;
			}
			return false; // only CResource pointer types are supported
		}
	case RT_Handle:
		{
			CRTTIHandleType* handleType = (CRTTIHandleType*)prop->GetType();
			CName handleTypeName = handleType->GetInternalType()->GetName();
			CClass* handleClass = SRTTI::GetInstance().FindClass( handleTypeName );
			if ( handleClass != nullptr && handleClass->IsA< CResource >() )
			{
				return true;
			}
			return false; // only Resource handles are supported
		}
	default:
		// Ok, we're fine with this property
		return true;
	}
}
