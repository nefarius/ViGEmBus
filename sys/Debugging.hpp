//
// Don't compile in verbose tracing on release builds
// 
#ifndef DBG
#ifdef TraceDbg
#undef TraceDbg
#define TraceDbg(...) { /* nothing to see here :) */ };
#endif
#endif
