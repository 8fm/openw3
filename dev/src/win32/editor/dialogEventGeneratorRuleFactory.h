/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "dialogEventGeneratorInternals.h"
#include "..\..\common\core\xmlFileReader.h"

namespace CStorySceneEventGeneratorInternals
{

	enum ELeafType
	{
		LT_AddCamera
	};

	struct TreeLeafRule
	{
		ELeafType	m_type;

		union
		{
			struct 
			{
				ECameraPlane	cameraPlane;
				Int32			weight;
			} m_addCamera;
		};

	};

	struct FilterRule
	{
		const static String		xmlNodeName;
		TDynArray<TreeLeafRule> m_childs;
	};

	struct SectionChunkRule
	{
		const static String		xmlNodeName;
		TDynArray<FilterRule>	m_childs;
		Float					m_minLength;
		Float					m_maxLength;
	};

	struct SectionRule
	{
		const static String xmlNodeName;
		TDynArray<SectionChunkRule> m_childs;
	};

	struct	SceneRule
	{
		const static String xmlNodeName;
		TDynArray<SectionRule> m_childs;
	};

	struct RootRule
	{
		const static String xmlNodeName;
		TDynArray<SceneRule> m_childs;
	};

	class ConfigLoader
	{
	public:
		void LoadRules();
		const RootRule& GetRoot() const { return m_rootRule; }

	protected:
		void ParseContent( TreeLeafRule & treeLeaf, CXMLFileReader& xmlReader );
		void ParseContent( SectionChunkRule & sectionChRule, CXMLFileReader& xmlReader );
		void ParseContent( SectionRule & sectioneRule, CXMLFileReader& xmlReader );
		void ParseContent( SceneRule & sceneRule, CXMLFileReader& xmlReader );
		void ParseContent( RootRule & rootRule, CXMLFileReader& xmlReader );
		void ParseContent( FilterRule & rootRule, CXMLFileReader& xmlReader );

		template< class Node, class ChildNode >
		void ParseNode( Node * parentNode, CXMLFileReader& xmlReader );
		template<>
		void ParseNode< FilterRule, TreeLeafRule >( FilterRule * parentNode, CXMLFileReader& xmlReader );

		RootRule m_rootRule;
	};
}










