/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "dialogEventGeneratorRuleFactory.h"

#include "dialogEventGeneratorConditions.inl"
#include "dialogEventGeneratorActions.inl"

#include "../../common/core/depot.h"

namespace CStorySceneEventGeneratorInternals
{
	const String FilterRule::xmlNodeName(TXT("Scene"));
	const String SectionChunkRule::xmlNodeName(TXT("SectionChunk"));
	const String SectionRule::xmlNodeName(TXT("Section"));
	const String SceneRule::xmlNodeName(TXT("Scene"));
	const String RootRule::xmlNodeName(TXT("DialogGenerator"));


	void ConfigLoader::ParseContent( TreeLeafRule & treeLeaf, CXMLFileReader& xmlReader )
	{
		String token;
		if( xmlReader.GetNodeName(token) )
		{
			if( token == TXT("AddCamera") )
			{
				treeLeaf.m_type = LT_AddCamera;
				if( xmlReader.AttributeT( TXT("weight"), treeLeaf.m_addCamera.weight ) )
				{
					treeLeaf.m_addCamera.weight = 1;
				}
				if( xmlReader.AttributeT( TXT("plane"), treeLeaf.m_addCamera.cameraPlane ) )
				{
					treeLeaf.m_addCamera.cameraPlane = CP_Any;
				}
			}
		}
	}

	void ConfigLoader::ParseContent( SectionChunkRule & sectionChRule, CXMLFileReader& xmlReader )
	{
		if( !xmlReader.AttributeT( TXT("minLength"), sectionChRule.m_minLength ) )
		{
			sectionChRule.m_minLength = 0.f;
		}
		if( !xmlReader.AttributeT( TXT("maxLength"), sectionChRule.m_maxLength ) )
		{
			sectionChRule.m_minLength = 0.f;
		}
	}

	void ConfigLoader::ParseContent( SectionRule & sectioneRule, CXMLFileReader& xmlReader )
	{
	}

	void ConfigLoader::ParseContent( SceneRule & sceneRule, CXMLFileReader& xmlReader )
	{
	}

	void ConfigLoader::ParseContent( RootRule & rootRule, CXMLFileReader& xmlReader )
	{
	}

	void ConfigLoader::ParseContent( FilterRule & rootRule, CXMLFileReader& xmlReader )
	{
	}

	template< class Node, class ChildNode >
	void ConfigLoader::ParseNode( Node * parentNode, CXMLFileReader& xmlReader )
	{
		if ( xmlReader.BeginNode( Node::xmlNodeName ) )
		{
			Uint32 childCount = xmlReader.GetChildCount();
			for( Uint32 i = 0; i < childCount; ++i )
			{
				ChildNode* newRule = new (parentNode->m_childs) ChildNode();
				ParseContent( *newRule, xmlReader );
				ParseNode< ChildNode, RemoveReference< decltype(*newRule->m_childs.TypedData()) >::Type >( newRule, xmlReader );			
			}
			xmlReader.EndNode();
		}
	}
	template<>
	void ConfigLoader::ParseNode< FilterRule, TreeLeafRule >( FilterRule * parentNode, CXMLFileReader& xmlReader )
	{
		if ( xmlReader.BeginNode( FilterRule::xmlNodeName ) )
		{
			Uint32 childCount = xmlReader.GetChildCount();
			while( xmlReader.BeginNextNode() )
			{
				TreeLeafRule* newRule = new (parentNode->m_childs) TreeLeafRule();
				ParseContent( *newRule, xmlReader );				
			}
			xmlReader.EndNode();
		}
	}


	void ConfigLoader::LoadRules()
	{
		IFile* fileReader = NULL;
		CDiskFile* xmlDiskFile = GDepot->FindFile( TXT( "gameplay\\globals\\dialogGeneratorConfig.xml" ) );
		if ( xmlDiskFile != NULL )
		{
			fileReader = xmlDiskFile->CreateReader();
		}
		if ( fileReader == NULL )
		{
			return;
		}
		CXMLFileReader xmlReader( fileReader );
		ParseNode<RootRule,SceneRule>( &m_rootRule, xmlReader );
	}

	/*
	void RuleFactory::ParseRules( CXMLFileReader &xmlReader, TDynArray< Rule > &rules )
	{
		xmlReader.BeginNode( TXT( "Rules" ) );

		Uint32 numberOfRules = xmlReader.GetChildCount();

		for( Uint32 i = 0; i < numberOfRules; ++i )
		{
			ParseRuleNode(xmlReader, rules);
		}

		xmlReader.EndNode();
	}

	void RuleFactory::ParseRuleNode( CXMLFileReader &xmlReader, TDynArray< Rule > &rules )
	{
		if ( xmlReader.BeginNode( TXT( "Rule" ) ) == false )
		{
			return;
		}

		rules.Grow( 1 );

		Rule& rule = rules.Back();

		ParseRuleConditions( xmlReader, rule );
		ParseRuleActions( xmlReader, rule );

		xmlReader.EndNode();
	}

	//////////////////////////////////////////////////////////////////////////

	void RuleFactory::ParseRuleConditions( CXMLFileReader &xmlReader, Rule &rule )
	{
		if ( xmlReader.BeginNode( TXT( "Conditions" ), true ) == false )
		{
			return;
		}

		Uint32 numberOfConditions = xmlReader.GetChildCount();

		for ( Uint32 i = 0; i < numberOfConditions; ++i )
		{
			Condition* condition = ParseConditionNode( xmlReader );

			if ( condition != NULL )
			{
				rule.m_conditions.PushBack( condition );
			}
		}

		xmlReader.EndNode( false );
	}

	Condition* RuleFactory::ParseConditionNode( CXMLFileReader &xmlReader )
	{
		Condition* condition = NULL;

		if ( xmlReader.BeginNode( TXT( "Condition" ) ) == false )
		{
			return NULL;
		}

		String conditionType;
		xmlReader.Attribute( TXT( "type" ), conditionType );

		if ( conditionType == TXT( "first_element" ) )
		{
			condition = new FirstElementCondition();
		}
		else if ( conditionType == TXT( "line_element" ) )
		{
			condition = new SceneLineCondition();
		}
		else if ( conditionType == TXT( "negative" ) )
		{
			Condition* nestedCondition = ParseConditionNode( xmlReader );
			condition = new NegativeCondition( nestedCondition );
		}
		else if ( conditionType == TXT( "length" ) )
		{
			Float length = 0.0f;
			ParseAttribute< Float >( TXT( "Length" ), length, xmlReader );
			condition = new ElementLengthCondition( length );
		}
		else if ( conditionType == TXT( "element_index" ) )
		{
			Uint32 index = 0;
			ParseAttribute< Uint32 >( TXT( "Index" ), index, xmlReader );
			condition = new ElementIndexCondition( 0 );
		}
		else if ( conditionType == TXT( "element_type" ) )
		{
			String elementType;
			ParseAttribute< String >( TXT( "Type" ), elementType, xmlReader );
			condition = new ElementTypeCondition( CName( elementType ) );
		}
		else if ( conditionType == TXT( "speaker_was_speaking" ) )
		{
			condition = new SpeakerWasSpeakingCondition();
		}

		xmlReader.EndNode();

		return condition;
	}

	//////////////////////////////////////////////////////////////////////////

	void RuleFactory::ParseRuleActions( CXMLFileReader &xmlReader, Rule &rule )
	{
		xmlReader.BeginNode( TXT( "Actions" ), true );

		Uint32 numberOfActions = xmlReader.GetChildCount();

		for( Uint32 i = 0; i < numberOfActions; ++i )
		{
			Action* action = ParseActionNode(xmlReader);

			if ( action != NULL )
			{
				rule.m_actions.PushBack( action );
			}
		}

		xmlReader.EndNode( false );
	}

	Action* RuleFactory::ParseActionNode( CXMLFileReader &xmlReader )
	{
		Action* action = NULL;

		if ( xmlReader.BeginNode( TXT( "Action" ) ) == false )
		{
			return NULL;
		}

		String actionType;
		xmlReader.Attribute( TXT( "type" ), actionType );

		if ( actionType == TXT( "create_camera" ) )
		{
			EventPositionProvider* positionProvider = ParsePositionProviderNode( xmlReader );

			action = new CreateCameraAction( positionProvider );
		}
		else if ( actionType == TXT( "create_gesture" ) )
		{
			EventPositionProvider* positionProvider = ParsePositionProviderNode( xmlReader );

			action = new CreateGestureAction( positionProvider );
		}

		xmlReader.EndNode();

		return action;
	}

	//////////////////////////////////////////////////////////////////////////

	EventPositionProvider* RuleFactory::ParsePositionProviderNode( CXMLFileReader &xmlReader )
	{
		EventPositionProvider* positionProvider = NULL;

		if ( xmlReader.BeginNode( TXT( "EventPosition" ), true ) == false )
		{
			return NULL;
		}

		String positionType;
		xmlReader.Attribute( TXT( "type" ), positionType );

		if ( positionType == TXT( "static" ) )
		{
			Float position = 0.0f;
			Float offset = 0.0f;

			ParseAttribute< Float >( TXT( "Position" ), position, xmlReader );
			ParseAttribute< Float >( TXT( "Offset" ), offset, xmlReader );

			positionProvider = new StaticEventPositionProvider( position, offset );
		}
		else if ( positionType == TXT( "time" ) )
		{
			Float time = 0.0f;
			Float offset = 0.0f;

			ParseAttribute< Float >( TXT( "Time" ), time, xmlReader );
			ParseAttribute< Float >( TXT( "Offset" ), offset, xmlReader );

			positionProvider = new AtTimeEventPositionProvider( time, offset );
		}

		xmlReader.EndNode( false );

		return positionProvider;
	}
	*/

}