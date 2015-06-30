#include <hidef.h>      /* common defines and macros */
#include "derivative.h"      /* derivative-specific definitions */

#pragma CODE_SEG __NEAR_SEG NON_BANKED

#define enable_intr() __asm(cli)
#define disable_intr() __asm(sei)

#define ROW_NUM 4
#define COL_NUM 4
#define MAGIC 12
#define START_KEY 10
#define TRUE 1
#define FALSE 0

#define MIN 1
#define SEC 0

int GetKey(void);

void DelayMSec(unsigned short);
void GetInitialTime(void);

void EnableClock(void);
void DisableClock(void);


const unsigned char SEG_CODES[] = { 0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F };
 

int GetKey() 
{
    int i,j;
                
    unsigned int row[4] = { 0xFE,0xFD,0xFB,0xF7 };
    unsigned int col[4] = { 0x10,0x20,0x40,0x80 };        
    unsigned int keys[4][4] = { 1,2,3,10,
                                4,5,6,11,
                                7,8,9,12,
                                14,0,15,13 };
    
    unsigned int temp ;

    for(i = 0; i< 4; ++i){
         PORTA = 0xFF;// Reset PortA

      for(j = 0; j< 4; ++j){
        temp = row[i] & col [j];

        Delay(50);     

        if((PORTA & temp) == PORTA)
          return keys[i][j];


      }
    }            
   
    
    return -1;
    
}

void Delay(unsigned short num)
{
    volatile unsigned short i;
    while (--num > 0){
        i = 2000;            
        while (--i > 0);
    }
}
