/**
* Copyright c 2014 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "../../common/core/commandlet.h"
#include "../../common/game/questDebuggerCommands.h"

class CQuestLayoutDumpCommandlet : public ICommandlet
{
	DECLARE_ENGINE_CLASS( CQuestLayoutDumpCommandlet, ICommandlet, 0 );

public:
	CQuestLayoutDumpCommandlet();

	virtual const Char* GetOneLiner() const { return TXT( "Saves quest layout for use in external QuestDebugger." ); }

	virtual bool Execute( const CommandletOptions& options );

	virtual void PrintHelp() const;
};

BEGIN_CLASS_RTTI( CQuestLayoutDumpCommandlet )
	PARENT_CLASS( ICommandlet )
	END_CLASS_RTTI()

	IMPLEMENT_ENGINE_CLASS( CQuestLayoutDumpCommandlet );

RED_DEFINE_STATIC_NAME( questlayoutdump )


CQuestLayoutDumpCommandlet::CQuestLayoutDumpCommandlet()
{
	m_commandletName = CNAME( questlayoutdump );
}

bool CQuestLayoutDumpCommandlet::Execute( const CommandletOptions& options )
{
	String questPath;
	if( !options.GetSingleOptionValue( TXT( "quest" ), questPath ) )
	{
		ERR_WCC( TXT( "Quest file not specified!" ) );
		PrintHelp();
		return false;
	}

	String outputPath;
	if( !options.GetSingleOptionValue( TXT( "out" ), outputPath ) )
	{
		ERR_WCC( TXT( "Output file not specified!" ) );
		PrintHelp();
		return false;
	}

	return CQuestLayoutDumper::Dump( questPath, outputPath );
}

void CQuestLayoutDumpCommandlet::PrintHelp() const
{
	LOG_WCC( TXT( "Use: " ) );
	LOG_WCC( TXT( "wcc questlayoutdump -quest quest_path -out dump_path" ) );
}
