#include "build.h"

#include "ticketGlobalSource.h"

IMPLEMENT_ENGINE_CLASS( CGlabalTicketSourceProvider );

void CGlabalTicketSourceProvider::Initialize()
{
	ClearTickets();
}
