#include <stdio.h>
#include <stdlib.h>

#include "const.h"
#include "debug.h"

#ifdef _STRING_H
#error "Do not #include <string.h>. You will get a ZERO."
#endif

#ifdef _STRINGS_H
#error "Do not #include <strings.h>. You will get a ZERO."
#endif

#ifdef _CTYPE_H
#error "Do not #include <ctype.h>. You will get a ZERO."
#endif

int main(int argc, char **argv)
{
    int ret = 0;
    if(validargs(argc, argv))
        USAGE(*argv, EXIT_FAILURE);
    if(global_options & 1)
        USAGE(*argv, EXIT_SUCCESS);
    if(global_options & 0x2) {
        ret = serialize();
        if (ret == -1) {
            return EXIT_FAILURE;
        }
    }
    if(global_options & 0x4) {
        ret = deserialize();
        if (ret == -1) {
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}

/*
 * Just a reminder: All non-main functions should
 * be in another file not named main.c
 */
