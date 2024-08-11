#ifndef TMS_SERVER_H
#define TMS_SERVER_H

#include "rrCore.h"

#ifdef EXPGROUP
#undef EXPGROUP
#endif
#define EXPGROUP(x)
EXPGROUP(serverapi) 

#ifdef EXPAPI
#undef EXPAPI
#endif
#define EXPAPI

EXPTYPE enum TmsError
{
	TMS_OK      = 0x0000,
	TMSERR_COULD_NOT_ALLOCATE_PORT = 0x0001, 

	TMSERR_BAD_PARAMETER          = 0x1004,
	TMSERR_COULD_NOT_MAKE_DIR     = 0x1101,  //could not make data file directory at startup

//	TMSERR_UNKNOWN_NETWORK = 0x1201,    //tmc failed somehow

	TMSERR_ALREADY_STARTED = 0x2000,    //tmsStart called multiple times without a tmsShutdown   

    TMSERR_ALREADY_SENT = 0x3000, // the requested frame was already sent

	TMSERR_UNKNOWN = 0xFFFF
};

//NOTE: These MUST match the same definitions for TmcInterface in tmcomm.h!!!!!!
EXPTYPE enum TmsInterfaceType
{
    TMSINT_FIRST = 0,
    TMSINT_LOCAL = TMSINT_FIRST,      // a local memory-to-memory interprocess communication interface
    TMSINT_TCP,                       // standard TCP/IP

    TMSINT_MAX
};
/*
Specifies the type of interface to use when passed to $tmsInit.
*/

EXPTYPE enum TmsStartFlag
{
    TMSF_DEFAULT = 0,
    TMSF_KEEP_TMP_FILES = 1,      // Keep temp files
    TMSF_DENY_PROFILER = 2,       // Deny connections from the run-time profiler (Telemetry API)
    TMSF_DENY_VISUALIZER = 4      // Deny connections from the Visualizer
};
/*
Specifies the startup flags for the server.
*/
 
RADEXPFUNC EXPAPI TmsError RADEXPLINK tmsHalt( void );
/*
Halts the server.

$:return TMSERR_OK on success, otherwise error

After starting a server with $tmsStart you should shut it down with tmsHalt.  This closes network connections and shuts down
the server thread.
*/
RADEXPFUNC EXPAPI TmsError RADEXPLINK tmsStart( EXPOUT char *desc, int const desc_length, 
				   EXPOUT char *ip, int const ip_length, 
				   TmU16 const kPort, 
				   int const kbLocalHost,
				   char const *kpDataDir,
                   TmU32 const kStartFlags,
                   int const kTimeOut,
                   int const kMaxSessions );
/*
Starts a Telemetry server instance.

$:desc pointer to a buffer in which to store the name/description of this server
$:desc_length size of the buffer specified by "desc"
$:ip pointer to a buffer in which to store the IP address of this server
$:ip_length size of the buffer specified by "ip"
$:kPort port number on which to start the server.  The default is 9180, and specifying 0 will automatically use the default port.
$:kbLocalHost indicates whether you wish to start the instance as localhost -- this will prevent processes outside the local machine from seeing this server.
$:kpDataDir is the name of the directory in which you wish to store the Telemetry data files
$:kStartFlags list of flags, may be a logical combination of $(TmsStartFlag) bit flags
$:kTimeOut timeout duration, in seconds, when a profiler connection should be kicked off.  A value of 0 indicates never time out.
$:kMaxSessions maximum number of sessions to record before exiting.  Default is -1 (no limit).
$:return TMSERR_OK on success, error code otherwise

Before calling this you must ensure that the Telemetry server subsystem has been initialized with $tmsInit.
*/

RADEXPFUNC EXPAPI TmsError RADEXPLINK tmsInit( TmsInterfaceType const k_iface, HTELEMETRY tmcx, char const *kTelemetryServer, TmU16 const kTelemetryPort, int const kTimeoutMS );
/*
Initializes the Telemetry server subsystem.

$:k_iface specifies the interface type (currently TCP is the only supported interface standard)
$:tmcx an (optional) Telemetry context handle in the event you want to benchmark the Telemetry server instance
$:kTelemetryServer the IP address or name of an optional Telemetry server you want to connect to (if you're benchmarking the server itself).  Pass 0 otherwise.
$:kTelemetryPort port number of an optional Telemetry server you want to connect to (if you're benchmarking the server itself).  Pass 0 otherwise.
$:kTimeoutMS timeout before aborting the connection to an external Telemetry server (if you're benchmarking the server itself).  Pass 0 otherwise.

This initializes the Telemetry server subsystem but does not start an actual server instance -- that is accomplished with $tmsStart.

Since the Telemetry server code is already "Telemetrized" it's possible to pass in a Telemetry handle and/or server information if you want to generate Telemetry data for the server process.  This is not
necessary and in most cases isn't useful unless you're running an embedded Telemetry server and you need to see how it's interacting with your own program.
*/

RADEXPFUNC EXPAPI TmsError RADEXPLINK tmsShutdown( void );
/*
Shuts down the Telemetry server subsystem.

$:return TMSERR_OK on success, error code otherwise

After calling $tmsInit you should call tmsShutdown before exiting.
*/

RADEXPFUNC EXPAPI char const * RADEXPLINK tmsErrorString( TmsError const kErr );
/*
Returns human readable error string for the given error.

$:return human readable error string
$:kErr error value to convert to string
*/

RADEXPFUNC EXPAPI int RADEXPLINK tmsGetNumActiveVisualizerConnections( void );
/*
Returns number of active Visualizer clients connected to this server.

$:return number of connected Visualizer clients
*/

RADEXPFUNC EXPAPI int RADEXPLINK tmsGetNumActiveProfilerConnections( void );
/*
Returns number of active profiler clients connected to this server.

$:return number of connected profiler clients
*/

RADEXPFUNC EXPAPI int RADEXPLINK tmsIsRunning( void );
/*
Indicates if a local Telemetry server thread is running.

$:return 1 on success, 0 on failure

*/

RADEXPFUNC EXPAPI int RADEXPLINK tmsProcessFiles( char const *kpFnameBase );
/*
Processes previously recorded tmp files.  Do not use, internal use only!!

$:kpFnameBase filename base of files to process.  
$:return 1 on success, 0 on failure
*/

RADEXPFUNC EXPAPI int RADEXPLINK tmsIsStillCooking( void );
/*
Indicates if a local Telemetry server thread is running.

$:return 1 on success, 0 on failure

*/

RADEXPFUNC EXPAPI HTELEMETRY RADEXPLINK tmsGetTelemetryContext( void );
/*
Returns the Telemetry context in use by the server lib.

$:returns Telemetry context in use by the server (NULL if none)

*/

EXPAPI typedef void TmsMessageHandler(char const *s);
/*
Message handler function pointer type passed to $tmsRegisterMessageHandler

s: message to print

Message handler function pointer type passed to $tmsRegisterMessageHandler
*/

RADEXPFUNC EXPAPI void RADEXPLINK tmsRegisterMessageHandler( TmsMessageHandler *fnc );
/*
Registers a user message handler callback with the server.

$:fnc pointer to function

When a fatal error requiring user notification occurs in the server library, the user
registered notification function (if any) is called. No handler is specified by default.
*/


#endif // TMS_SERVER_H
