#include <hidef.h>     /* common defines and macros */
#include "derivative.h"    /* derivative-specific definitions */
#include "lcd.h"             /*lcdyi surerken kullanilacak header*/
#include <stdio.h>           /*for sprintf*/

#define TotalCycle 65356     
#define RtiTime    90       /*10.240 ms for 1 tick and 90 tic counter for 1 sn*/	
#define distance   20       
#define MAX_GARAGE_SIZE 5  
#define BUZZER_F 800		/*Using for output compare*/
#define BUZZER_TIME 2       /*Calling 2 second for Buzzer song*/
#define HIGH_SPEED 50		

/*for timing*/
volatile unsigned int first = 0, second = 0, time = 0;
volatile unsigned int rotation = 0; /*arabanin yonu icin*/
volatile int done = 1;

/*input capture flag for sensors flag1 and flag2*/
unsigned int flag1 = 0, flag2 = 0;
unsigned int timerCounter =0, avarageTimer = 0, timerTempCounter =0;
/*flag garage is full*/
unsigned int flagIsFull = 0;
/*for buzzer flags*/
unsigned int buzzerEnable=0, targetBuzzerTime = 0;

unsigned int sec = 0,pre_sec = 0,hour = 0,minute = 0;
unsigned int option=0, tempOption=0;
unsigned int inCounter = 0, outCounter = 0, preInCounter = 0, preOutCounter=0;
char msg1[100];

int totalCarInGarage = 0;

/*interrupt handler*/
interrupt (((0x10000 - Vtimch0)/2)-1) void portakal1(void);
interrupt (((0x10000 - Vtimch1)/2)-1) void portakal2(void);
interrupt (((0x10000 - Vrti)/2)-1) void realtime(void);
interrupt (((0x10000 - Vtimch4)/2)-1) void buzzerInterrupt(void);

/**
	taking input from keyboard
	return -1 for nothing
		    choise num for otherwise	
*/	 
int getKey();

/* menu display */
void initialScreen();
void clearScreen();

/*setting funciton*/
void initialize();
/**
 checking flag 1 and flag2 
   and setting first and second speeds	
*/
void controlSensorFlag();

/* Three diff. option for display menu*/
void showMenu(int i);
/*  Checking input from keypad and 
	  run different action according to input	
*/
void runOption();

/* calculating and display speed  */
void runSpeedOption();
/* display car num in garage*/
void runNumCarOption();
/* display clock*/
void runClockOption();
/* display in and out car for last one minute*/
void runAvarageOption();
void runExitOption();

void main(void)
{

	/******* Setting ******/
  int option = 0, tempOption=0;
  unsigned int speed;
  volatile unsigned int iCounterx;
  int flag = 0;

  __asm(sei);

  initialize();

  __asm(cli);

  initialScreen();


    /******** RUN PROGRAM ********/

  while(done){

     controlSensorFlag();
     runOption();

     /*Eğer highspeed olmuşsa, targetbuzzer time buffer_time ile set ediliyor. */
    if(targetBuzzerTime <= 0)
        buzzerEnable = 0;
    if(pre_sec < minute) {
    	/*her dakika yenilendidiginde elimdeki counterlar sıfırlanıyor. */	
       preOutCounter = outCounter;
       preInCounter = inCounter;
       inCounter = 0;
       outCounter = 0;
       pre_sec = minute;  
    }
    
  }


  __asm(swi);
}


void showMenu(int flag){

   int i=0,j=0;
   	/*optionlara gore outputlar gosterildi.*/

    if(flag == 1){

      sprintf(msg1,"Speed ---->[1]");
      puts2lcd(msg1); // Send first line
      put2lcd(0xC0,CMD); // move cursor to 2nd row, 1st column               
      sprintf(msg1,"N of Cars->[2]");
      puts2lcd(msg1); // Send second line
      delay_1ms(2);
    }

    if(flag == 2){

      sprintf(msg1,"Clock ---->[3]");
      puts2lcd(msg1); // Send first line
      put2lcd(0xC0,CMD); // move cursor to 2nd row, 1st column                
      sprintf(msg1,"Avarage--->[4]");
      puts2lcd(msg1); // Send second line
      delay_1ms(2);
    }
    
    if(flag == 3){
      sprintf(msg1,"Exit ---->[5]");
      puts2lcd(msg1); // Send first line
     
      delay_1ms(2);
    }  
      
}


void initialScreen(){
  int iCounterx;

  /*display menu */

   showMenu(1);

  for(iCounterx  = 0; iCounterx < 100; ++iCounterx)
     delay_1ms(1);

    clearScreen();
    put2lcd(0x80,CMD); // move cursor to 2nd row, 1st column
    clearScreen();
    put2lcd(0x80,CMD); // move cursor to 2nd row, 1st column

     showMenu(2);

   for(iCounterx  = 0; iCounterx < 100; ++iCounterx)
         delay_1ms(1);

    clearScreen();
    put2lcd(0x80,CMD); // move cursor to 2nd row, 1st column

    clearScreen();
    put2lcd(0x80,CMD); // move cursor to 2nd row, 1st column


       showMenu(3);

   for(iCounterx  = 0; iCounterx < 100; ++iCounterx)
         delay_1ms(1);

    clearScreen();
    put2lcd(0x80,CMD); // move cursor to 2nd row, 1st column

    clearScreen();
    put2lcd(0x80,CMD); // move cursor to 2nd row, 1st column

}


void initialize(){
  TSCR1 = 0x80;     /* Turn on timer subsystem */
  TSCR2 = 0x05;     /* Set prescaler for divide by 32 */


  RTICTL = 0x54;	/*Set realtime 10.240 ms*/
  CRGINT = 0x80;	/*taking interrupt for rti*/
  CRGFLG = 0x80;	/*taking flag for rti*/

  DDRB = 0xff;		/*for debugging*/
  PORTB = 1;		/*for debugging*/

  DDRT = DDRT | 0x20;	/*buzzer enable*/

  DDRA = 0x0F;  // pb0-3 is out, others input


  openlcd(); // Initialize LCD display

  /****************** Input Capture ***********/
 /* Setup for IC1 */
  TIOS = TIOS & ~0x01;     /* Configure PT1 as IC */
  TCTL4 = (TCTL4 | 0x02) & ~0x01;   /* Capture Rising Edge */
  TFLG1 = 0x01;       /* Clear IC1 Flag */

  /* Set interrupt vector for Timer Channel 1 */
  TIE = TIE | 0x01;       /* Enable IC1 Interrupt */

  /* Setup for IC2 */
  TIOS = TIOS & ~0x02;     /* Configure PT2 as IC */
  TCTL4 = (TCTL4 | 0x08) & ~0x04;   /* Capture Rising Edge */
  TFLG1 = 0x02;         /* Clear IC2 Flag */

  /* Set interrupt vector for Timer Channel 2 */
  TIE = TIE | 0x02;        /* Enable IC2 Interrupt */

  /********* Output Compare *********************/

  /* Setup for OC4 */
  TIOS = TIOS | 0x10;     /* Configure PT1 as OC */
  TCTL1 = (TCTL1 | 0x01) & ~0x02;   /* Capture toggle  */
  TFLG1 = 0x10;       /* Clear OC4 Flag */

  /* Set interrupt vector for Timer Channel 1 */
  TIE = TIE | 0x10;       /* Enable OC4 Interrupt */
}

void controlSensorFlag()
{
      if(flag1 == 1){
      	  /*sensor1 den gecildigi andaki zaman counterlarim ms. cinsinden firste konuldu*/
          first = timerCounter*90;
          first += sec * 60;
          flag1 = 0;

          /*yonum ayarlanmadiysa*/	
          if(rotation == 0)
          {
            rotation = 1;  /*cikan olarak ayarlandi*/
          }else if( rotation == 2 )
          {
          	/*eger girense ve max. gar. size 'i na ulasilmissa counter arttirilmadi*/	
           if(totalCarInGarage < MAX_GARAGE_SIZE)
              totalCarInGarage += 1;
              
              inCounter +=1;
                
              rotation = 0;
          }

      }
      if(flag2 == 1){

          second = timerCounter *90;
          second += sec * 60;
          flag2 = 0;

          if(rotation == 0)
          {
            rotation = 2;
          }
          else if( rotation == 1)
          {
        	  /*eger cikan tarafsa*/	
              if(totalCarInGarage > 0)
                totalCarInGarage -= 1;

              outCounter+=1;


              flagIsFull = 0;
              rotation =0;
          }


      }


}


void runOption()
{	
	/*eger max. size a erisilmisse garage full denildi*/
   if(MAX_GARAGE_SIZE == totalCarInGarage)
      {     clearScreen();
            put2lcd(0x80,CMD); /* clear screen, move cursor to home */

            sprintf(msg1,"Garage is FULL");
            puts2lcd(msg1); // Send first line

            delay_1ms(2); /* wait until "clear display" command is complete */
            flagIsFull = 1;
      }else
       flagIsFull = 0;

       /*optionlar run edildi*/
      runSpeedOption();
      runNumCarOption();
      runClockOption();
      runAvarageOption();
      runExitOption();


     /*input alindi*/ 
     tempOption = getKey();
     if(tempOption != -1){
          option = tempOption;
          

      clearScreen();
       put2lcd(0x80,CMD); /* clear screen, move cursor to home */
      
      sprintf(msg1,"option :%d",option);
      puts2lcd(msg1); // Send first line

     }

   /*   if(avarageTimer == 1){
       avarageTimer =0;
       preOutCounter = outCounter;
       preInCounter = inCounter;
       inCounter = 0;
       outCounter = 0;
     }*/

}

void runSpeedOption(){
  unsigned int speed;

    if(second > 0 && first > 0){

          if(option == 1 & !flagIsFull){
             clearScreen();
             put2lcd(0x80,CMD); /* clear screen, move cursor to home */
              
            /*hangi taraftan geldigini control ettikten sonra hiz hesaplandi*/  
            if(second >first){
               speed = distance * 1000;
               speed = speed/(second-first);

               sprintf(msg1,"speed:%u",speed);
               puts2lcd(msg1); // Send first line
               put2lcd(0xC0,CMD); // move cursor to 2nd row, 1st column                
               sprintf(msg1,"diff time:%u",second-first);
               puts2lcd(msg1); // Send first line
            }else{
                clearScreen();
                put2lcd(0x80,CMD); /* clear screen, move cursor to home */
                speed = distance * 1000;
                speed = speed/(first-second);
                sprintf(msg1,"speed:%u",speed);
               puts2lcd(msg1); // Send first line
                put2lcd(0xC0,CMD); // move cursor to 2nd row, 1st column                
                sprintf(msg1,"diff time:%u",first-second);
               puts2lcd(msg1); // Send first line
            }


            delay_1ms(2); /* wait until "clear display" command is complete  */
            /*high speed ise*/
            if(speed > HIGH_SPEED){
              targetBuzzerTime = BUZZER_TIME;
              buzzerEnable = 1;
            }

            speed = 0;
          }

          second = 0;
          first = 0;
      }


}

void runNumCarOption(){
   if(option == 2 && !flagIsFull) {
            clearScreen();
            put2lcd(0x80,CMD); /* clear screen, move cursor to home */
           
            sprintf(msg1,"Car Num:%d",totalCarInGarage);
            puts2lcd(msg1); // Send first line


            delay_1ms(2); /* wait until "clear display" command is complete */
     }
}

void runClockOption(){
   if(option == 3 && !flagIsFull){
            clearScreen();
            put2lcd(0x80,CMD); /* clear screen, move cursor to home */
          
            sprintf(msg1,"%d:%d:%d",hour,minute,sec);
            puts2lcd(msg1); // Send first line


            delay_1ms(2); /* wait until "clear display" command is complete */

     }
}

void runAvarageOption(){
    unsigned int in, out;

    if(option == 4){
            clearScreen();
            put2lcd(0x80,CMD); /* clear screen, move cursor to home */
           
           
              in = (90-timerCounter)*(preInCounter/90)+inCounter;
              out = (90-timerCounter)*(preOutCounter/90)+outCounter;
        
              sprintf(msg1,"IN: %d",in);
              puts2lcd(msg1); // Send first line
              put2lcd(0xC0,CMD); // move cursor to 2nd row, 1st column                 puts2lcd(msg1); // Send first line
              sprintf(msg1,"Out :%d",out);
              puts2lcd(msg1); // Send first line

            delay_1ms(2); /* wait until "clear display" command is complete */
            option= 4;

     }
}

void runExitOption(){
   if(option == 5){
            clearScreen();
            put2lcd(0x80,CMD); /* clear screen, move cursor to home */
          
            
              sprintf(msg1,"End Of Program");
              puts2lcd(msg1); // Send first line
             

            delay_1ms(100); /* wait until "clear display" command is complete */
            done = 0;
     }
}

interrupt (((0x10000 - Vtimch0)/2)-1) void portakal1(void){
      flag1 = 1;
      TFLG1 = 0x01;
}


interrupt (((0x10000 - Vtimch1)/2)-1) void portakal2(void){
      flag2 = 1;
      TFLG1= 0x02;
}

interrupt (((0x10000 - Vrti)/2)-1) void realtime(void){
     /*timer counter imizi sec. minute ve hour olarak 3 parcaya ayirdik*/
     ++timerCounter;
     if(timerCounter == 90){
        sec += 1;
        if(targetBuzzerTime >0){
          targetBuzzerTime-=1;
        }

        if(sec == 60){
          minute += 1;
          if(minute == 60){
            hour+=1;
            minute = 0;
          }
          sec =0;
        }
        timerCounter =0;
     }
     CRGFLG = 0x80;
}


interrupt (((0x10000 -Vtimch4)/2)-1) void buzzer(void){
	/*buzzer calindi.*/
  if(buzzerEnable){
    PTT = PTT ^ 0b00100000;

  }

  TFLG1 = 0x10;
  TC4 = TC4 + BUZZER_F;
}



int getKey()
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
        PORTA = PORTA & row[i]; // Set specific row to 0

        if( (PORTA & col[j]) == 0 )
          return keys[i][j];


      }
    }


    return -1;

}


void clearScreen()
{
     char *msg1 = "               ";
     int i,j ;

     puts2lcd(msg1); // Send first line
     delay_1ms(2); /* wait until "clear display" command is complete*/
     put2lcd(0xC0,CMD); // move cursor to 2nd row, 1st column
     puts2lcd(msg1); // Send first line
     
    
    delay_1ms(2); /* wait until "clear display" command is complete */
}

















