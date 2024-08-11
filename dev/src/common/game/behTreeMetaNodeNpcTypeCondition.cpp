#include "build.h"
#include "behTreeMetaNodeNpcTypeCondition.h"

RED_DEFINE_STATIC_NAME( GetNPCType );

IMPLEMENT_RTTI_ENUM( ENPCGroupType );


Bool CBehTreeNodeConditionalTreeNPCTypeDefinition::CheckCondition( CBehTreeInstance* owner, CBehTreeSpawnContext& context ) const
{
	CNewNPC* npc = owner->GetNPC();
	return npc && npc->GetGroupType() == m_npcType;	
}

String CBehTreeNodeConditionalTreeNPCTypeDefinition::GetNodeCaption() const
{	
	const Char* npcType;

	switch (m_npcType)
	{
	case ENGT_Enemy:
		npcType = TXT( "Enemy" );
		break;
	case ENGT_Commoner:
		npcType = TXT( "Commoner" );
		break;
	case ENGT_Quest:
		npcType = TXT( "Quest" );
		break;
	case ENGT_Guard:
		npcType = TXT( "Guard" );
		break;
	}

	return String::Printf( TXT( "if ( npcGroupType %s %s )" ), m_invert ? TXT("!=") : TXT("="), npcType );	
}