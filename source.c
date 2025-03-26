/*
 * source.c
 *
 *  Created on: Feb 1, 2025
 *      Author: Greg Valijan
 */

void initialize();
void EnableGPIO();
void EnableUART();
void EnableTimer();
void sendChar(char);
char readChar();
void delay(int);
char keyPadOutput();
void TimerHandler();
void ButtonHandler();

void initialize(){
    #define RCGCUART (*((volatile unsigned long *) 0x400FE618))
    #define RCGCGPIO (*((volatile unsigned long *) 0x400FE608))
    #define GPIODEN_A (*((volatile unsigned long *) 0x4000451C))
    #define GPIOAFSEL (*((volatile unsigned long *) 0x40004420))
    #define GPIOPCTL (*((volatile unsigned long *) 0x40004528))
    #define UARTCTL (*((volatile unsigned long *) 0x4000C030))
    #define UARTIBRD (*((volatile unsigned long *) 0x4000C024))
    #define UARTFBRD (*((volatile unsigned long *) 0x4000C028))
    #define UARTLCRH (*((volatile unsigned long *) 0x4000C02C))
    #define UARTCC (*((volatile unsigned long *) 0x4000CFC8))
    #define UARTDR (*((volatile unsigned long *) 0x4000C000)) //Define the UART Data Register
    #define UARTFR (*((volatile unsigned long *) 0x4000C018)) //Define the UART Flag Register

    #define GPIODATA_F (*((volatile unsigned long *) 0x40025048))
    #define GPIODEN_F (*((volatile unsigned long *) 0x4002551C))
    #define GPIODIR_F (*((volatile unsigned long *) 0x40025400))
    #define GPIOIS_F (*((volatile unsigned long *) 0x40025404))
    #define GPIOIEV_F (*((volatile unsigned long *) 0x4002540C))
    #define GPIOIM_F (*((volatile unsigned long *) 0x40025410))

    #define GPIODATA_C (*((volatile unsigned long *) 0x400063C0))
    #define GPIODEN_C (*((volatile unsigned long *) 0x4000651C))
    #define GPIODIR_C (*((volatile unsigned long *) 0x40006400))
    #define GPIOPDR_C (*((volatile unsigned long *) 0x40006514))

    #define GPIODATA_D (*((volatile unsigned long *) 0x4000703C))
    #define GPIODEN_D (*((volatile unsigned long *) 0x4000751C))
    #define GPIODIR_D (*((volatile unsigned long *) 0x40007400))
    //#define GPIOPDR_D (*((volatile unsigned long *) 0x40007514))

    #define EN0 (*((volatile unsigned long *) 0xE000E100))

    #define RCGCTIMER (*((volatile unsigned long *) 0x400FE604))
    #define GPTMCTL (*((volatile unsigned long *) 0x4003000C))
    #define GPTMCFG (*((volatile unsigned long *) 0x40030000))
    #define GPTMTAMR (*((volatile unsigned long *) 0x40030004))
    #define GPTMTAPR (*((volatile unsigned long *) 0x40030038))
    #define GPTMTAILR (*((volatile unsigned long *) 0x40030028))
    #define GPTMRIS (*((volatile unsigned long *) 0x4003001C)) //Timer Interrupt Flag Register
    #define GPTMIMR (*((volatile unsigned long *) 0x40030018))
    #define GPTMICR (*((volatile unsigned long *) 0x40030024))
    #define GPIOICR_F (*((volatile unsigned long *) 0x4002541C))
}

void EnableGPIO()
{
    //Port F
    RCGCGPIO |= 0x20;       //Port F Clock
    GPIODEN_F |= 0x2;       //PF1 & PF4 Digital Enable
    GPIODIR_F |= 0x10;    //PF4 output
    GPIOIS_F = 0x0;         //PF1 is edge sensitive
    GPIOIEV_F |= 0x0;       // PF1 rising edge sensitive
    GPIOIM_F |= 0x2;        //unmasked PF1 interrupt
    EN0 |= 0x40000000;

    //Port C
    RCGCGPIO |= 0x4;        //Port C Clock
    GPIODEN_C |= 0xF0;      //PC4 - PC7 Digital Enable
    GPIODIR_C |= 0x0;       //Port C input
    GPIOPDR_C |= 0xF0;      //PC4 - PC7 Weak PD resistor

    //Port D
    RCGCGPIO |= 0x8;        //Port D Clock
    GPIODEN_D |= 0xF;       //PD0 - PD3 Digital Enable
    GPIODIR_D |= 0xF;       //Port D output
    //GPIOPDR_D |= 0xF;        //PD0 - PD3 Weak PD resistor
}

void EnableUART()
{
    RCGCUART |= 0x1;        //Enables UART module
    RCGCGPIO |= 0x1;        //Enables  GPIO Clock for Port A
    GPIOAFSEL |= 0x3;       //Enables alt Function selection for PA0 and PA1 by GPIOPCTRL
    GPIOPCTL |= 0x11;       //Sets PA0 and PA1 to their alt functions (UART RX0 and TX0 respectively)
    GPIODEN_A |= 0x3;       //Digital Enable for PA0 and PA1

    UARTCTL = 0x0;          //Temporarily Disable UART

    UARTIBRD = 104;         // Set the integer part of the baud rate
    UARTFBRD = 11;          // Set the fractional part of the baud rate
    UARTLCRH = (0x3<<5);    //Set UART to have 8 data bits (bits 5 and 6 are active), 1 stop bit and 0 parity bits
    UARTCC = 0x0;           //Set UART clock to System Clock (DEFAULT)

    UARTCTL = 0x301;        //Enable UART, Transmit and Receive
}

void EnableTimer()
{
    RCGCTIMER |= 0x1;       //Sets Clock for Timer0
    GPTMCTL = 0x0;          //Disable Timer0
    GPTMCFG = 0x4;          //Sets bit-width of timer
    GPTMTAMR = 0x2;         //sets to periodic counting down
    GPTMTAPR = 245;         //pre-scalar = 245
    GPTMTAILR = 65104;      //65104; //65104 periods for 1 second delay
    GPTMICR |= 0x1;         //Clear Flag in MRIS
    GPTMIMR |= 0x1;         //Unmask timer interrupt
    GPTMCTL |= 0x1;         //Enable Timer0
    EN0 |= 0x80000;         //Enable interrupt For Timer0 Sub-timer A
}

void sendChar(char data)
{
    while((UARTFR & 0x20) != 0); //Runs when receive holding register is not full
    UARTDR = data;               // data into DR
}

char readChar()
{
    char data;
    while((UARTFR & 0x10) != 0); //Runs when transmit holding register is not full
    data = UARTDR;               // data out of DR
    return data;
}

void delay(int delay){
    volatile int i = 0;
    for(i = 0; i < delay*10000; i++){}
}

char keyPadOutput()
{
    unsigned char symbol[4][4] = {
        {'1','2','3','A'},
        {'4','5','6','B'},
        {'7','8','9','C'},
        {'*','0','#','D'}
    };

    int i = 0;
    int j = 0;

    //D0-3 is row 1-4 //output to pad
    //C4-7 is column 1-4 //input from pad
    for(i = 0; i < 4; i++)
    {
        GPIODATA_D = (1 << i);
        for (j = 0; j < 4; j++)
        {
            if(GPIODATA_C & (1 << j+4))
            {
                return symbol[i][j];
            }
        }
        GPIODATA_D = 0x0;
    }
    return '\0';
}

void TimerHandler()
{
    RCGCGPIO |= 0x20;       //Port F Clock
    GPIODEN_F |= 0x12;      //PF1 & PF4 Digital Enable
    GPIODIR_F |= 0x10;      //PF4 output

    GPIODATA_F = (GPIODATA_F & 0x10) ? 0x0 : 0x10;

    GPTMICR = 0x1;          //Clear Flag
}

void ButtonHandler()
{
    sendChar('$');
    volatile int i = 0;
    for (i = 0; i < 270000; i++){}
    GPIOICR_F = 0x2;        //Clear Flag
}
