#pragma once

extern const Char* SGetCommandLine();

Bool SIsMainThread();
const Red::System::Internal::ThreadId& SGetMainThreadId();

extern Int32 CountLeadingZeros( Uint32 data );