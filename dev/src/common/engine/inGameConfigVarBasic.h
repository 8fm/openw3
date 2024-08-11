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
	class CConfigVarBasic : public IConfigVar
	{
	public:
		struct ConstructParams
		{
			CName name;
			String displayName;
			THashSet<CName> tags;
			BoolExpression::IExp* visibilityCondition;
		};

		CConfigVarBasic( const ConstructParams& params );
		~CConfigVarBasic();

		virtual const String GetDisplayName() const;
		virtual void ListOptions(TDynArray< SConfigPresetOption >& output) const;
		virtual const CName GetConfigId() const;
		virtual Bool HasTag(CName tag) const;
		virtual const THashSet<CName>& GetTags() const;
		virtual Bool IsVisible() const;

	protected:
		CName m_name;
		String m_displayName;
		THashSet<CName> m_tags;
		BoolExpression::IExp* m_visibilityCondition;

	};
}
