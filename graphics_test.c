#include "src/sdl2_basics.c"
#include <stdio.h>
#include <unistd.h>

int main()
{
    if(init_graphics_silent())
        printf("woops no sdl2 here");
    
    black_frame();

    while(handle_events())
    {
        sleep(1);
    }
    
    exit_graphics();
    return 0;
}