#ifndef TMPLUGIN_H
#define TMPLUGIN_H

#define TM_PLUGIN_VERSION 0x00000004

#include "tmtypes.h"

#ifndef EXPTYPE
#define EXPTYPE
#endif
#ifndef EXPAPI
#define EXPAPI
#endif
#ifndef EXPIN
#define EXPIN
#endif
#ifndef EXPOUT
#define EXPOUT
#endif
#ifndef EXPGROUP
#define EXPGROUP(val)
#endif

EXPGROUP(PIAPI)

EXPAPI typedef void PI_IMP_DRAWTYPE( char const * text, float const x, float const y, float const r, float const g, float const b );
/*
Text drawing function provided by the Visualizer to plugins.

$:text pointer to text string
$:x coordinate of the text
$:y coordinate of the text
$:r red value
$:g green value
$:b blue value

This is a simple helper function for plugins to draw some text into the Visualizer.
*/

EXPTYPE typedef struct TmPluginImport
{
    EXPIN PI_IMP_DRAWTYPE * draw_text;
} TmPluginImport;
/*
Plugin import functions structure passed to Visualizer plugins.

$:draw_text this is a text drawing function provided by the Visualizer

This structure is passed to your $PI_INIT function, and contains functions
that you can call (back into the Visualizer).  Over time, we will add more
and more functions here.
*/

EXPAPI typedef int PI_INIT( EXPIN void* hwnd, EXPIN TmPluginImport* pi_import );
/*
Plugin initialization function.

$:hwnd opaque pointer to system specific window handle (HWND on Windows)
$:pi_import pointer to a plugin import structure passed from the Visualizer

Each plugin provides an initialization function called by the Visualizer once during plugin startup.
*/

EXPAPI typedef void PI_SHUTDOWN( void );
/*
Plugin shutdown function.

Called when the plugin is released/shutdown.
*/

EXPAPI typedef int PI_BEGIN_SETUP( TmI32 const kStartFrame, TmI32 const kNumFrames );
/*
Called once at the start of each frame of rendering.

$:kStartFrame the first frame that will be rendered
$:kNumFrames  the total number of frames that will be rendered
$:return 1 if it's okay to render, 0 if not

This function is called so that your plugin can do any per-render-frame setup necessary.
*/

EXPAPI typedef int  PI_FRAME_SETUP( TmI32 const kFrame, TmI32 const kFrameIndex, TmI32 const kNumBlobs,
                                   TmI32 const kSequence, 
                                   char const * kpName, 
                                   float const kX, float const kY, float const kW, 
                                   float const kCyclesToPixelsScale );
/*
Called once at the start of rendering for each captured frame.

$:kFrame the frame that's about to be rendered
$:kFrameIndex 0-based frame index for this render set
$:kNumBlobs number of blobs that will be rendered this frame
$:kSequence sequence number
$:kpName name of the frame
$:kX x location of the frame
$:kY y location of the frame
$:kW width of the frame
$:kCyclesToPixelsScale scale factor to convert from clock cycles to pixels
$:return 1 if it's okay to render, 0 if not

This function provides basic information about the actual Telemetry frame that's being
rendered.  Use this to setup any information you need to render the blobs within that
one captured frame.
*/

EXPAPI typedef int PI_BLOB_SETUP( TmI32 const kFrame, TmI32 const kFrameIndex,
                                 TmI32 const kBlobIndex,
                                 TmU64 const kBlobTime, 
                                 int const kNumChunks, 
                                 void **blobChunks,
                                 int const kChunkSizes[] );
/*
Plugin function called before each blob is rendered.

$:kFrame render frame
$:kFrameIndex 0-based index of the frame for this render set
$:kBlobIndex kindex of the blob
$:kBlobTime time when the blob arrived (relative to frame start)
$:return 1 if it's okay to render, 0 if not

Before a blob is rendered this function is called.
*/

EXPAPI typedef void PI_BEGIN_DRAW( TmI32 const kStartFrame, TmI32 const kNumFrames );
/*
Plugin function called before we're about to begin drawing.

$:kStartFrame the first frame that will be rendered
$:kNumFrames the total number of frames that will be rendered

This function is called right before the start of actual rendering.
*/

EXPAPI typedef void  PI_FRAME_DRAW( TmI32 const kFrame, TmI32 const kFrameIndex, TmI32 const kNumBlobs,
                      TmI32 const kSequence, char const * kpName, float const _kX, float const _kY, float const _kW, float const kCyclesToPixelsScale );
/*
Plugin function called before we're about to render a single captured frame.

$:kFrame frame number
$:kFrameIndex frame index in this render set
$:kNumBlobs number of blobs that will be rendered
$:kSequence sequence number
$:kName of this frame
$:kX x location of the frame
$:kY y location of the frame
$:kW width of the frame
$:kCyclesToPixelsScale scalar for converting clock cycles to pixels

Plugin function called before we're about to render a single captured frame.
*/


EXPAPI typedef float PI_BLOB_DRAW( TmI32 const kFrame, TmI32 const kFrameIndex,
                                  TmI32 const kBlobIndex,
                                  TmU64 const kBlobTime, 
                                  int const kNumChunks, 
                                  void **blobChunks,
                                  int const kChunkSizes[] );
/*
Plugin function for drawing a single blob.

$:kFrame frame number being rendered
$:kFrameIndex frame index being rendered (0-based from the start frame specified originally)
$:kBlobIndex index of the blob
$:kBlobTime time when the blob arrived (relative to frame start)
$:kNumChunks number of blob chunks to draw
$:blobChunks array of pointers to blob chunks
$:kChunkSizes size of each blob chunk
$:return the height of the rendered area

This is the core blob drawing function, called once for each blob.
*/

EXPAPI typedef void PI_END_DRAW( TmI32 const kStartFrame, TmI32 const kNumFrames );
/*
Plugin function called when done rendering all blobs on all frames.

$:kStartFrame first frame that was rendered
$:kNumFrames number of frames that were rendered

Plugin function called when done rendering all blobs on all frames.
*/

EXPTYPE typedef struct TmPluginInfo
{
	EXPIN TmU32 pi_size;
	EXPIN TmU32 pi_version;

    EXPOUT char const     *pi_identifier;

    EXPOUT PI_INIT        *pi_init;
    EXPOUT PI_SHUTDOWN    *pi_shutdown;

    EXPOUT PI_BEGIN_SETUP *pi_begin_setup;
    EXPOUT PI_FRAME_SETUP *pi_frame_setup;
    EXPOUT PI_BLOB_SETUP  *pi_blob_setup;

    EXPOUT PI_BEGIN_DRAW  *pi_begin_draw;
    EXPOUT PI_FRAME_DRAW  *pi_frame_draw;

    EXPOUT PI_BLOB_DRAW   *pi_blob_draw;

    EXPOUT PI_END_DRAW    *pi_end_draw;

} TmPluginInfo;
/*
Plugin information structure passed back to the Visualizer.

$:pi_size size of the structure.  This will be set by the Visualizer before calling pi_init.
$:pi_init initialization function.
$:pi_version plugin API version expected by the Visualizer.  Your plugin should verify against this value in pi_init.
$:pi_shutdown plugin shutdown function.

$:pi_begin_setup plugin setup function called at the start of each render cycle
$:pi_frame_setup frame setup function called once per Telemetry frame that is about to be drawn
$:pi_blob_setup blob setup function called once for each blob that will be drawn
$:pi_begin_draw this is called one time per render frame right before actual rendering begins
$:pi_frame_draw called one time per Telemetry frame, per render frame, during rendering
$:pi_blob_draw called one time per blob during rendering
$:pi_end_draw notifies plugin that all rendering is complete for this render cycle

Each plugin is responsible for populating and returning a function pointer table to the Visualizer when the $PI_ENTRY
function is called.  See $(blob_plugins) for more information.
*/

EXPAPI typedef int PI_ENTRY( EXPOUT TmPluginInfo* pinfo );
/*
Plugin entry function.

$:pinfo Information about the plugin should be stored here.

This should be the only function exported directly by your plugin when writing a $(blob_plugins).
*/

#endif // TMPLUGIN_H
