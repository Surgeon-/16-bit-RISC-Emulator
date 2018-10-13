
#include "CPrint.hpp"

#include <iostream>
#include <stdarg.h>
#include <ncurses.h>

int cprint(const char * format, ...) {
    
    static char buffer[1024];
    
    va_list args;
    va_start(args, format);
    
    vsnprintf(buffer, 1024u, format, args);
    
    va_end(args);
    
    for (size_t i = 0; i < 1024u; i += 1) {
        
        if (buffer[i] == '\0') break;
        
        addch(buffer[i]);
        
    }
    
    refresh();
    
    // JUST PRINTF:
    /*va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);*/
    
}