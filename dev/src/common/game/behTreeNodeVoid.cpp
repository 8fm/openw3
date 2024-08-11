#include "build.h"
#include "behTreeNodeVoid.h"

//            :              _   _
//            :               `v'
//   `.       ;       .'                 ,~-.
//     `.  .-'''-.  .',~')       _ _    (    )~.
//       ;'       `; (    `-.   ' V `  ,'       )~.  _ _
//      /           ;-`      )        (          __)' v `
//     |           (_         )~./\    `~'--~'`~' _
//'''''|            (_    __    /##\            ,' )_
//      \           /(   _HH_,~/#/\#\          ( c'  `._
//       `.       .'  `~[____]/#/==\#\       ,-' -' (_c )
//      .' `-,,,-' `.    |=_|/#/= _=\#\      `>o  ~    '-.
//    .'      ;      `.  |-=/#/=____=\#\     ( ~ ,~.~.,`-'),
//     _  _   :        ` |=/#/=/,~~.\=\#\    ,' (\\||,' `-'),
//   .' \/ `. :          |/#/=(/_)(_\)=\#\  (`~ (_\`|)o ~   )
//                       /#/_= |_\/_|  _\#\  >(   `~' ._,~ '-.
//                      /#/ _=[______]= _\#\(' `~,  c    ~. c)
//                     /#/=,---. _ = ___ =\#\`( (  ~ _.'   <'
//                    /#/ /_____\ ==/,-.\ =\#\(  c   c___ ) )
//                    `|=(/_|_|_\) //.-.\\=_|' `-.__,' //`-'
//                  `v@|==| : : |=(/:|_|:\)=|   `,-\ `'/`,-
//                `v@'~|= |_;_;_| =|     | =|       \ |
//        _/\_/\_,(c`@'|=[_______]=|  =()|==|_/\_/\_| |/\_
//        -||-||-@~'(_@|= _o@&8o_ =|     | =|-||-||-( |||-
//       _,@`v-@'~ c@._|_['%8o&8']_|_____|__| || || | )||
//     ,@C @,~' @,-v~'    """""""`[_______]         | |
//       'v-~,@,`    (\-/)        /     / `*~   _.-'   `-.
//                  ={   }=   ~*'(     (          `*
//           `*       ) (         \     `.~*'
//                  _/   \_     `*~\      `.
//            `*    \     /         \       `.`*'
//      *'           `-))'       `*' \        `.       *'
//                    ((     `*       \         `.
//                     \)

Bool IBehTreeVoidDefinition::RemoveNullChildren()
{
	Bool modified = false;
#ifndef RED_FINAL_BUILD
	for( Int32 i = m_voidNodes.Size()-1; i >= 0; i-- )
	{
		if( !m_voidNodes[i] )
		{
			m_voidNodes.RemoveAt( i );
			modified = true;
		}
	}
#endif

	return modified;
}

IBehTreeNodeInstance* IBehTreeVoidDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return NULL;
}
Bool IBehTreeVoidDefinition::OnSpawn( IBehTreeNodeInstance* node, CBehTreeSpawnContext& context ) const
{
	return false;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT
void IBehTreeVoidDefinition::OffsetNodesPosition( Int32 offsetX, Int32 offsetY )
{
	RemoveNullChildren();
#ifndef RED_FINAL_BUILD
	for( Int32 i = m_voidNodes.Size()-1; i >= 0; i-- )
	{
		m_voidNodes[i]->OffsetNodesPosition( offsetX, offsetY );
	}
#endif
	TBaseClass::OffsetNodesPosition( offsetX, offsetY );
}
#endif

Bool IBehTreeVoidDefinition::IsTerminal() const
{
	return false;
}

Bool IBehTreeVoidDefinition::IsValid() const
{
	return true;
}

Bool IBehTreeVoidDefinition::CanAddChild() const
{
	return true;
}
void IBehTreeVoidDefinition::RemoveChild( IBehTreeNodeDefinition* node )
{
#ifndef RED_FINAL_BUILD
	for( Uint32 i = 0, n = m_voidNodes.Size(); i != n; ++i )
	{
		if ( m_voidNodes[i] == node )
		{
			m_voidNodes.RemoveAt( i );
			break;
		}
	}
#endif
}
Int32 IBehTreeVoidDefinition::GetNumChildren() const
{
#ifndef RED_FINAL_BUILD
	return m_voidNodes.Size();
#else
	return 0;
#endif 
}
IBehTreeNodeDefinition* IBehTreeVoidDefinition::GetChild( Int32 index ) const
{
#ifndef RED_FINAL_BUILD
	return m_voidNodes[ index ];
#else
	return nullptr;
#endif
}
void IBehTreeVoidDefinition::AddChild( IBehTreeNodeDefinition* node )
{
#ifndef RED_FINAL_BUILD
	ASSERT( node->GetParent() == this );
	m_voidNodes.PushBack( node );
#endif
}
void IBehTreeVoidDefinition::OnPostLoad()
{
	TBaseClass::OnPostLoad();

	RemoveNullChildren();
}
