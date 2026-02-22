/*
 * atca_trace_helper.c
 *
 * Thin C-language wrapper around atca_trace_config().
 *
 * Problem: atca_debug.h has no extern "C" guards, so when it is included
 * transitively (via cryptoauthlib.h) from a .cpp file the compiler assigns
 * C++ linkage to atca_trace_config. The compiled library exports the symbol
 * with plain C linkage. The mismatch causes a linker error when the function
 * is called from C++.
 *
 * Solution: call atca_trace_config from this .c file (where C linkage is the
 * default), and expose a new C-linkage symbol that atecc608b.cpp can forward-
 * declare with extern "C" without any conflict.
 */

#include "cryptoauthlib.h"
#include <stdio.h>

#ifdef ATCA_PRINTF
void atecc608b_trace_suppress(FILE* fp)
{
    atca_trace_config(fp);
}
#else
void atecc608b_trace_suppress(FILE* fp)
{
    (void)fp;
}
#endif
