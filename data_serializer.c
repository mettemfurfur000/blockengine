#include "game_types.h"
#include <stdlib.h>
#include <string.h>

//..(letter)(length)[data]..

void data_get(block* b,char letter_to_get,byte* output,int* out_size)
{
    byte* data = b->data;
    int data_size = b->data_size;
    int index = 0;

    char readed_letter;
    byte readed_size;

    while(index < data_size)
    {
        //get letter
        readed_letter = data[index];
        if(readed_letter == 0)
        {
            *out_size = 0;
            return;
        }
        index++;
        //get length
        readed_size = data[index];
        index++;
        //if found
        if(readed_letter == letter_to_get)
        {
            //copy data and return
            output = malloc(readed_size);
            memcpy(output,data + index,readed_size);
            *out_size = readed_size;
            return;
        }
        //else skip data
        index += readed_size;
    }
}

void update_block(block* b)
{
    
}