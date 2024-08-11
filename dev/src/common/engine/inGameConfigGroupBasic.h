/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "inGameConfigInterface.h"

namespace BoolExpression
{
	class IExp;
}

namespace InGameConfig
{
	class CConfigGroupBasic : public IConfigDynamicGroup
	{
	public:
		struct ConstructParams
		{
			CName name;
			String displayName;
			THashSet<CName> tags;
			BoolExpression::IExp* visibilityCondition;
		};

		CConfigGroupBasic( const ConstructParams& params );

		virtual const String GetDisplayName() const;
		virtual void ListPresets(TDynArray< SConfigPresetOption >& output) const;
		virtual void ApplyPreset(const Int32 id, const EConfigVarSetType setType);
		virtual const Int32 GetActivePreset() const;
		virtual const CName GetConfigId() const;
		virtual Bool HasTag(CName tag) const;
		virtual const THashSet<CName>& GetTags() const;
		virtual Bool IsVisible() const;
		virtual void Discard();

	protected:
		CName m_name;												// Engine id of this config group
		String m_displayName;										// Name exposed to UI
		THashSet<CName> m_tags;										// Tags for that group
		BoolExpression::IExp* m_visibilityCondition;				// Returns true if group is visible (uses active tags from GInGameConfig)

	};

}
