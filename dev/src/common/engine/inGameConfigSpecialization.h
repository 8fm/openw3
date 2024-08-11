/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../core/configVarSystem.h"
#include "boolExpression.h"
#include "inGameConfigInterface.h"
#include "ingameConfigListingFunction.h"

namespace InGameConfig
{
	/*********************** Config Var Specialization ***********************/

	// Listing function helper
	namespace ListingHelper
	{
		// Get all options listed by listing function
		void GetAllOptions( const CName& funcName, TDynArray< SConfigPresetOption >& output );

		// Get all entries for given option listed by listing function
		void GetEntries( const CName& funcName, Int32 optionIdx, TDynArray< SConfigPresetEntry >& output );

		// Get all entries for all options
		void GetAllEntries( const CName& funcName, TDynArray< SConfigPresetEntry >& output );

		// Get all entries for all options
		void GetAllOptionAndAllEntries( const CName& funcName, TDynArray< SConfigPresetOption >& outOptions, TDynArray< SConfigPresetEntry >& outEntries );
	}

	namespace CompareHelper
	{
		Bool Equals( const String& valueA, const String& valueB );
	}

	/******************** Generic Group ********************/
	class CConfigGroupEngineConfig : public IConfigDynamicGroup
	{
	public:
		CConfigGroupEngineConfig();
		CConfigGroupEngineConfig( CName& name, String& displayName, TDynArray< SConfigPresetOption >& presets,
			TDynArray< SConfigPresetEntry > presetEntries, TDynArray< IConfigVar* >& vars,
			THashSet<CName> tags, BoolExpression::IExp* visibilityCondition );

		// Interface implementation
		virtual void Discard();
		virtual const String GetDisplayName() const;
		virtual void ListConfigVars(TDynArray< IConfigVar* >& output);
		virtual void ListPresets(TDynArray< SConfigPresetOption >& output) const;
		virtual void ApplyPreset(const Int32 presetId, const EConfigVarSetType setType);
		virtual const Int32 GetActivePreset() const;
		virtual const CName GetConfigId() const;
		virtual Bool HasTag( CName tag ) const;
		virtual const THashSet<CName>& GetTags() const;
		virtual Bool IsVisible() const;
		virtual void ResetToDefault();

	private:
		void DoNeedToRefreshEngine();

	private:
		CName m_name;												// Engine id of this config group
		String m_displayName;										// Name exposed to UI
		TDynArray< SConfigPresetOption > m_presets;					// Presets
		TDynArray< SConfigPresetEntry > m_presetEntries;		// All entries for all presets
		TDynArray< IConfigVar* > m_vars;							// Config variables exposed to edit
		THashSet<CName> m_tags;										// Tags for that group
		BoolExpression::IExp* m_visibilityCondition;				// Returns true if group is visible (uses active tags from GInGameConfig)

	};

	/******************** Generic Variable *************************/

	// Desctiption for config var creation
	struct SConfigVarCreationDesc
	{
		SConfigVarCreationDesc()
			: groupName( CName::NONE )
			, name( CName::NONE )
			, visibilityCondition( nullptr )
			, listingFunction( CName::NONE )
		{
			/* Intentionally Empty */
		}

		CName groupName;
		CName name;
		String displayName;
		String displayType;
		TDynArray< SConfigPresetOption > options;
		TDynArray< SConfigPresetEntry > optionEntries;
		THashSet<CName> tags;
		BoolExpression::IExp* visibilityCondition;
		CName listingFunction;
	};

	// Generic config var
	class CConfigVarEngineConfig : public IConfigVar
	{
	public:
		CConfigVarEngineConfig();
		CConfigVarEngineConfig( SConfigVarCreationDesc& desc );
		~CConfigVarEngineConfig();

		// Interface implementation
		virtual const String GetDisplayType() const;
		virtual const String GetDisplayName() const;
		virtual const CConfigVarValue GetValue() const;
		virtual void SetValue(const CConfigVarValue value, const EConfigVarSetType setType);
		virtual void ListOptions( TDynArray< SConfigPresetOption >& output ) const;
		virtual const CName GetConfigId() const;
		virtual Bool HasTag( CName tag ) const;
		virtual const THashSet<CName>& GetTags() const;
		virtual Bool IsVisible() const;
		virtual void ResetToDefault() override;
		
	protected:
		Int32 GetMatchingOptionIdx( const TDynArray< SConfigPresetOption >& options, const TDynArray< SConfigPresetEntry >& entries ) const;

	private:
		void ResetToDefault( const CName& group, const CName& key );
		void DoNeedToRefreshEngine();

	private:
		CName m_groupName;										// Engine name
		CName m_name;											// Engine name
		String m_displayName;									// User name
		String m_displayType;									// User type
		TDynArray< SConfigPresetOption > m_options;				// Value options (optional)
		TDynArray< SConfigPresetEntry > m_optionEntries;	// Entries for options
		Int32 m_activeOption;									// Id of active option
		THashSet<CName> m_tags;									// Tags for that variable
		BoolExpression::IExp* m_visibilityCondition;			// Returns true if var is visible (uses active tags from GInGameConfig)
		CName m_listingFunction;								// Name of the option listing function, a function which lists options at runtime

	};
}
