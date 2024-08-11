/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once

namespace InGameConfig
{
	class IConfigGroup;

	class CConfigGroupBasicBuilder
	{
	public:
		virtual void SetName( const CName& name );
		virtual void SetDisplayName( const CName& name );
		virtual void AddTag( const CName& tag );
		virtual void SetVisibilityCondition( const String& conditionStr );

		virtual void Reset();
		virtual IConfigGroup* Construct() = 0;

	private:
		struct GroupParams
		{
			CName name;
			CName displayName;
			THashSet<CName> tags;
			String visibilityConditionStr;
		};	

	};
}
