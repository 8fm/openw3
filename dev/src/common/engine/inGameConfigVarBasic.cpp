/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "inGameConfigVarBasic.h"
#include "boolExpression.h"

namespace InGameConfig
{
	CConfigVarBasic::CConfigVarBasic(const ConstructParams& params)
		: m_name( params.name )
		, m_displayName( params.displayName )
		, m_tags( params.tags )
		, m_visibilityCondition( params.visibilityCondition )
	{
		/* Intentionally empty */
	}

	CConfigVarBasic::~CConfigVarBasic()
	{
		if( m_visibilityCondition != nullptr )
		{
			m_visibilityCondition->Destroy();
		}
	}

	const String CConfigVarBasic::GetDisplayName() const
	{
		return m_displayName;
	}

	void CConfigVarBasic::ListOptions(TDynArray< SConfigPresetOption >& output) const
	{
		/* Intentionally empty */
	}

	const CName CConfigVarBasic::GetConfigId() const
	{
		return m_name;
	}

	Bool CConfigVarBasic::HasTag(CName tag) const
	{
		return m_tags.Exist( tag );
	}

	const THashSet<CName>& CConfigVarBasic::GetTags() const
	{
		return m_tags;
	}

	Bool CConfigVarBasic::IsVisible() const
	{
		if( m_visibilityCondition != nullptr )
		{
			return m_visibilityCondition->Evaluate();
		}

		return true;
	}

}
