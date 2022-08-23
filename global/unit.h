#pragma once

#define UNIT_INIT_FINI() \
        static struct InitFini { \
        InitFini() {                       \
            /*std::cout << __FILE__ << std::endl;*/ \
            initialization();                  \
        }       \
        ~InitFini() { finalization(); }        \
    } initFini;