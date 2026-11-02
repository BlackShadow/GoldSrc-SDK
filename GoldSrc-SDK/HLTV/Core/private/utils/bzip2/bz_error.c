#include <stdio.h>
#include <stdlib.h>

/* libbzip2 calls this only when an internal invariant fails. */
void bz_internal_error(int errcode)
{
	fprintf(stderr, "libbzip2 internal error: %d\n", errcode);
	abort();
}
