/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

class CEdPresets;

//! Hook for handling CEdPresets events
class IEdPresetsHook
{
	// NOTE: If you add new events here make sure you also update the presetsBox.cpp
	// to forward the new event to the original hook!

public:
	virtual ~IEdPresetsHook(){}

	//! Called when the presets list has changed (preset added, removed, etc)
	virtual void OnPresetsChanged( CEdPresets* /* source */ ){}

	//! Called when a new value was set for the given key
	virtual void OnPresetKeySet( CEdPresets* /* source */, const String& /* presetName */, const String& /* keyName */, const String& /* newValue */ ){}

	//! Called when a key was removed
	virtual void OnPresetKeyRemoved( CEdPresets* /* source */, const String& /* presetName */, const String& /* keyName */ ){}

	//! Called after the passed presets was loaded from a file
	virtual void OnPresetLoaded( CEdPresets* /* source */, const String& /* presetName */ ){}

	//! Called before the passed preset is saved on a file
	virtual void OnPresetSaving( CEdPresets* /* source */, const String& /* presetName */ ){}
};

//! Contains presets in the form of key:value pairs
class CEdPresets
{
	// Single preset entry
	struct PresetEntry
	{
		String		m_name;							//!< Name for the entry
		String		m_value;						//!< Value for the entry in serializable form
	};

	// A single preset
	struct Preset
	{
		String						m_name;			//!< Preset's name
		TDynArray< PresetEntry >	m_entries;		//!< Preset's entries
	};

	TDynArray< Preset >		m_presets;				//!< All presets
	IEdPresetsHook*			m_hook;					//!< Presets hook

	//! Scans for a preset with the given name and returns its index or -1 if not found
	//! If addNew is true, it adds a new preset if one is not found with that name
	Int32 FindPreset( const String& name, Bool addNew );

	//! Like FindPreset above but never adds new
	Int32 FindPreset( const String& name ) const;

	//! Scans the given preset for a key with that name and returns its index or -1 if not found
	//! Like FindPreset, if addNew is true it adds a new preset if one is found
	Int32 FindKey( Preset& preset, const String& keyName, Bool addNew );

	//! Like FindKey above but never adds new
	Int32 FindKey( const Preset& preset, const String& keyName ) const;

	//! Saves the presets to the given file
	Bool SavePresetToFile( const Preset& preset, const String& absolutePath );

	//! Loads the presets from the given file
	Bool LoadPresetFromFile( Preset& preset, const String& absolutePath );

public:
	CEdPresets();
	~CEdPresets();

	//! Removes all the presets from this preset container
	void RemoveAllPresets();

	//! Sets a value for the preset - if the preset does not exist, it is created
	void SetPresetValue( const String& presetName, const String& keyName, const String& keyValue );

	//! Removes a value for the preset (it will not create a new preset).  You can use this to "restore" a value
	void RemovePresetValue( const String& presetName, const String& keyName );

	//! Returns the value of the given preset or the given default value (it will not create a new preset)
	String GetPresetValue( const String& presetName, const String& keyName, const String& defaultValue ) const;

	//! Removes the given preset
	void RemovePreset( const String& presetName );

	//! Sets the preset using the properties of the given object.  If names is supplied it uses only the given property names
	void SetPresetFromObject( const String& presetName, const CObject* object, const String& keyPrefix=String::EMPTY, const TDynArray< CName >* names=nullptr );

	//! Applies the preset using the properties of the given object, if they exist.  If names is supplied it only restores the given property names
	void ApplyPresetToObject( const String& presetName, CObject* object, const String& keyPrefix=String::EMPTY, const TDynArray< CName >* names=nullptr ) const;

	//! Fills the passed array with the preset names
	void GetPresetNames( TDynArray< String >& names ) const;

	//! Saves the presets to the given file
	Bool SavePresetToFile( const String& presetName, const String& absolutePath );

	//! Loads the presets from the given file
	Bool LoadPresetFromFile( const String& presetName, const String& absolutePath );

	//! Saves the preset files to the given directory.  Returns false if any file failed to save
	//! If removeExistingPresetFiles is set to true, the directory will be scanned for .redpreset
	//! files which will be deleted before the presets are saved
	Bool SavePresetFiles( const String& absolutePath, Bool removeExistingPresetFiles );

	//! Scans the given directory for preset files.  Returns true if any preset file was loaded
	Bool LoadPresetFiles( const String& absolutePath );

	//! Sets the hook that will receive events for these presets
	void SetHook( IEdPresetsHook* hook );

	//! Returns the current presets hook
	RED_INLINE IEdPresetsHook* GetHook() const { return m_hook; }
	
	//! Returns true if the given property can be saved in a preset
	static Bool IsPropertySavable( CProperty* prop );

	// Some friends
	friend IFile& operator<<( IFile& file, CEdPresets::PresetEntry& val );
};

