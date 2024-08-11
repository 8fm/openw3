#include "build.h"
#include "../../common/core/commandlet.h"

class CTestCommandlet : public ICommandlet
{
	DECLARE_ENGINE_CLASS( CTestCommandlet, ICommandlet, 0 );

public:
	CTestCommandlet();

	// Executes commandlet command
	virtual bool Execute( const CommandletOptions& options );

	// Returns commandlet one-liner
	virtual const Char* GetOneLiner() const;

	// Prints commandlet help
	virtual void PrintHelp() const;
};

BEGIN_CLASS_RTTI( CTestCommandlet )
	PARENT_CLASS( ICommandlet)
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS(CTestCommandlet);

CTestCommandlet::CTestCommandlet()
{
	m_commandletName = CNAME( cookertest );
}

bool CTestCommandlet::Execute( const CommandletOptions& options )
{
	LOG_WCC( TXT("Performing test execute...") );
	WARN_WCC( TXT("Performing test warning...") );
	ERR_WCC( TXT("Performing test error...") );

	// Done
	return true;
}

const Char* CTestCommandlet::GetOneLiner() const
{
	return TXT("Test of wcc");
}

void CTestCommandlet::PrintHelp() const
{
	LOG_WCC( TXT("Simply tests wcc.") );
}

