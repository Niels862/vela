#ifndef VEMU_UTIL_H
#define VEMU_UTIL_H

#include <stdlib.h>
#include <assert.h>

#define VEMU_UNREACHED()    (assert("unreached"), exit(1))
#define VEMU_ASSERT(c)      assert(c)

#endif
