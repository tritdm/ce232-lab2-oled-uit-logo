#include "UIT_logo.h"

void convert() 
{
    for (int8_t i = 0; i < 8; ++ i)
        for (uint8_t j = 0; j < 128; ++ j) 
        {
            uint8_t t = 0;
            for (int8_t k = 7; k >= 0; -- k)
            {
                t = t * 2 + UIT_string[i * 8 + k][j] - 48;
            }
            UIT_page[i][j] = t;
        }       
}