/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "inGameConfigGroupBasic.h"
#include "boolExpression.h"

namespace InGameConfig
{
	CConfigGroupBasic::CConfigGroupBasic( const ConstructParams& params )
		: m_name( params.name )
		, m_displayName( params.displayName )
		, m_tags( params.tags )
		, m_visibilityCondition( params.visibilityCondition )
	{
		/* Intentionally empty */
	}

	const CName CConfigGroupBasic::GetConfigId() const
	{
		return m_name;
	}

	const Int32 CConfigGroupBasic::GetActivePreset() const
	{
		return -1;
	}

	void CConfigGroupBasic::ApplyPreset(const Int32 id, const EConfigVarSetType setType)
	{
		/* Intentionally empty */
	}

	void CConfigGroupBasic::ListPresets(TDynArray< SConfigPresetOption >& output) const
	{
		/* Intentionally empty */
	}

	const String CConfigGroupBasic::GetDisplayName() const
	{
		return m_displayName;
	}

	Bool CConfigGroupBasic::HasTag(CName tag) const
	{
		return m_tags.Exist( tag );
	}

	const THashSet<CName>& CConfigGroupBasic::GetTags() const
	{
		return m_tags;
	}

	Bool CConfigGroupBasic::IsVisible() const
	{
		if( m_visibilityCondition != nullptr )
		{
			return m_visibilityCondition->Evaluate();
		}

		return true;
	}

	void CConfigGroupBasic::Discard()
	{
		if( m_visibilityCondition != nullptr )
		{
			m_visibilityCondition->Destroy();
		}
	}

}
