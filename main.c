// CAPCACITOR READER (Capacitor_Reader.mk)

/* Pinout for DIP28 PIC32MX130:
                                          --------
                                   MCLR -|1     28|- AVDD 
  VREF+/CVREF+/AN0/C3INC/RPA0/CTED1/RA0 -|2     27|- AVSS 
        VREF-/CVREF-/AN1/RPA1/CTED2/RA1 -|3     26|- AN9/C3INA/RPB15/SCK2/CTED6/PMCS1/RB15
   PGED1/AN2/C1IND/C2INB/C3IND/RPB0/RB0 -|4     25|- CVREFOUT/AN10/C3INB/RPB14/SCK1/CTED5/PMWR/RB14
  PGEC1/AN3/C1INC/C2INA/RPB1/CTED12/RB1 -|5     24|- AN11/RPB13/CTPLS/PMRD/RB13
   AN4/C1INB/C2IND/RPB2/SDA2/CTED13/RB2 -|6     23|- AN12/PMD0/RB12
     AN5/C1INA/C2INC/RTCC/RPB3/SCL2/RB3 -|7     22|- PGEC2/TMS/RPB11/PMD1/RB11
                                    VSS -|8     21|- PGED2/RPB10/CTED11/PMD2/RB10
                     OSC1/CLKI/RPA2/RA2 -|9     20|- VCAP
                OSC2/CLKO/RPA3/PMA0/RA3 -|10    19|- VSS
                         SOSCI/RPB4/RB4 -|11    18|- TDO/RPB9/SDA1/CTED4/PMD3/RB9
         SOSCO/RPA4/T1CK/CTED9/PMA1/RA4 -|12    17|- TCK/RPB8/SCL1/CTED10/PMD4/RB8
                                    VDD -|13    16|- TDI/RPB7/CTED3/PMD5/INT0/RB7
                    PGED3/RPB5/PMD7/RB5 -|14    15|- PGEC3/RPB6/PMD6/RB6
                                          --------
*/

// Information here:
// http://umassamherstm5.org/tech-tutorials/pic32-tutorials/pic32mx220-tutorials/1-basic-digital-io-220

#include <XC.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lcd.h"
 
// Configuration Bits (somehow XC32 takes care of this)
#pragma config FNOSC = FRCPLL       // Internal Fast RC oscillator (8 MHz) w/ PLL
#pragma config FPLLIDIV = DIV_2     // Divide FRC before PLL (now 4 MHz)
#pragma config FPLLMUL = MUL_20     // PLL Multiply (now 80 MHz)
#pragma config FPLLODIV = DIV_2     // Divide After PLL (now 40 MHz)
#pragma config FWDTEN = OFF         // Watchdog Timer Disabled
#pragma config FPBDIV = DIV_1       // PBCLK = SYCLK
#pragma config FSOSCEN = OFF        // Secondary Oscillator Enable (Disabled)

// Defines
#define SYSCLK 40000000L
#define Baud2BRG(desired_baud)( (SYSCLK / (16*desired_baud))-1)

#define CHANGE_BUTTON 	PORTAbits.RA1
#define BUY_BUTTON	 	PORTBbits.RB1

 
void UART2Configure(int baud_rate)
{
    // Peripheral Pin Select
    U2RXRbits.U2RXR = 4;    //SET RX to RB8
    RPB9Rbits.RPB9R = 2;    //SET RB9 to TX

    U2MODE = 0;         // disable autobaud, TX and RX enabled only, 8N1, idle=HIGH
    U2STA = 0x1400;     // enable TX and RX
    U2BRG = Baud2BRG(baud_rate); // U2BRG = (FPb / (16*baud)) - 1
    
    U2MODESET = 0x8000;     // enable UART2
}



// Needed to by scanf() and gets()
int _mon_getc(int canblock)
{
	char c;
	
    if (canblock)
    {
	    while( !U2STAbits.URXDA); // wait (block) until data available in RX buffer
	    c=U2RXREG;
        while( U2STAbits.UTXBF);    // wait while TX buffer full
        U2TXREG = c;          // echo
	    if(c=='\r') c='\n'; // When using PUTTY, pressing <Enter> sends '\r'.  Ctrl-J sends '\n'
		return (int)c;
    }
    else
    {
        if (U2STAbits.URXDA) // if data available in RX buffer
        {
		    c=U2RXREG;
		    if(c=='\r') c='\n';
			return (int)c;
        }
        else
        {
            return -1; // no characters to return
        }
    }
}

#define PIN_PERIOD (PORTB&(1<<6))

// GetPeriod() seems to work fine for frequencies between 200Hz and 700kHz.
long int GetPeriod (int n)
{
	int i;
	unsigned int saved_TCNT1a, saved_TCNT1b;
	
    _CP0_SET_COUNT(0); // resets the core timer count
	while (PIN_PERIOD!=0) // Wait for square wave to be 0
	{
		if(_CP0_GET_COUNT() > (SYSCLK/4)) return 0;
	}

    _CP0_SET_COUNT(0); // resets the core timer count
	while (PIN_PERIOD==0) // Wait for square wave to be 1
	{
		if(_CP0_GET_COUNT() > (SYSCLK/4)) return 0;
	}
	
    _CP0_SET_COUNT(0); // resets the core timer count
	for(i=0; i<n; i++) // Measure the time of 'n' periods
	{
		while (PIN_PERIOD!=0) // Wait for square wave to be 0
		{
			if(_CP0_GET_COUNT() > (SYSCLK/4)) return 0;
		}
		while (PIN_PERIOD==0) // Wait for square wave to be 1
		{
			if(_CP0_GET_COUNT() > (SYSCLK/4)) return 0;
		}
	}

	return  _CP0_GET_COUNT();
}

int getCapacitor( unsigned long C)
{
	if(C < 1300 && C > 800)
		return 102;
	else if(C < 13000 && C > 8000)
		return 103;
	else if(C < 130000 && C > 80000)
		return 104;
	else if(C < 1300000 && C > 80000)
		return 105;
	else
		return 100;
}

void LCD_CreateChar(unsigned char location, unsigned char *map) {
    unsigned char i;
    // Location is 0-7. CGRAM addresses start at 0x40.
    // Each character takes 8 bytes of memory.
    WriteCommand(0x40 + (location * 8));
    
    for (i = 0; i < 8; i++) {
        WriteData(map[i]);
    }
    
    // Crucial: Move the cursor back to the visible screen area (DDRAM)
    WriteCommand(0x80); 
}

//------------------------------------------------------------------------------
// PRINT FUNCTIONS
//------------------------------------------------------------------------------

void buyModePrint()
{
	printf("\x1b[2J\x1b[H"); // clears terminal
	
    printf(" /$$$$$$$  /$$   /$$ /$$     /$$       /$$      /$$                 /$$          \r\n");
    printf("| $$__  $$| $$  | $$|  $$   /$$/      | $$$    /$$$                | $$          \r\n");
    printf("| $$  \\ $$| $$  | $$ \\  $$ /$$/       | $$$$  /$$$$  /$$$$$$   /$$$$$$$  /$$$$$$ \r\n");
    printf("| $$$$$$$ | $$  | $$  \\  $$$$/        | $$ $$/$$ $$ /$$__  $$ /$$__  $$ /$$__  $$\r\n");
    printf("| $$__  $$| $$  | $$   \\  $$/         | $$  $$$| $$| $$  \\ $$| $$  | $$| $$$$$$$$\r\n");
    printf("| $$  \\ $$| $$  | $$    | $$          | $$\\  $ | $$| $$  | $$| $$  | $$| $$_____/\r\n");
    printf("| $$$$$$$/|  $$$$$$/    | $$          | $$ \\/  | $$|  $$$$$$/|  $$$$$$$|  $$$$$$$\r\n");
    printf("|_______/  \\______/     |__/          |__/     |__/ \\______/  \\_______/ \\_______/\r\n\n");
    
    printf("-----------------------------------------------------------------------------------\r\n\n");	    
}

void calModePrint()
{
	printf("\x1b[2J\x1b[H"); // clears terminal
	
	printf("                      ,,                                      ,,          \r\n");
	printf("  .g8\"\"\"bgd         `7MM      `7MMM.     ,MMF'              `7MM          \r\n");
	printf(".dP'     `M           MM        MMMb    dPMM                  MM          \r\n");
	printf("dM'       ` ,6\"Yb.    MM        M YM   ,M MM  ,pW\"Wq.    ,M\"\"bMM  .gP\"Ya  \r\n");
	printf("MM         8)   MM    MM        M  Mb  M' MM 6W'   `Wb ,AP    MM ,M'   Yb \r\n");
	printf("MM.         ,pm9MM    MM        M  YM.P'  MM 8M     M8 8MI    MM 8M\"\"\"\"\"\" \r\n");
	printf("`Mb.     ,'8M   MM    MM        M  `YM'   MM YA.   ,A9 `Mb    MM YM.    , \r\n");
	printf("  `\"bmmmd' `Moo9^Yo..JMML.    .JML. `'  .JMML.`Ybmd9'   `Wbmd\"MML.`Mbmmd' \r\n\n");
	
	printf("-------------------------------------------------------------------------\r\n\n");
}

void healthModePrint()
{
	printf("\x1b[2J\x1b[H"); // clears terminal

	printf("    __  __           ____  __       __  ___          __   \r\n");
	printf("   / / / /__  ____ _/ / /_/ /_     /  |/  /___  ____/ /__ \r\n");
	printf("  / /_/ / _ \\/ __ `/ / __/ __ \\   / /|_/ / __ \\/ __  / _ \\\r\n");
    printf(" / __  /  __/ /_/ / / /_/ / / /  / /  / / /_/ / /_/ /  __/\r\n");
	
    printf("/_/ /_/\\___/\\__,_/_/\\__/_/ /_/  /_/  /_/\\____/\\__,_/\\___/ \r\n\n");

	
	printf("-------------------------------------------------------------------------\r\n\n");
}

void readModePrint()
{
	printf("\x1b[2J\x1b[H"); // clears terminal

	printf(" ____     ___   ____  ___        ___ ___   ___   ___      ___ \r\n");
	printf("|    \\   /  _] /    T|   \\      |   T   T /   \\ |   \\    /  _]\r\n");
	printf("|  D  ) /  [_ Y  o  ||    \\     | _   _ |Y     Y|    \\  /  [_ \r\n");
	printf("|    / Y    _]|     ||  D  Y    |  \\_/  ||  O  ||  D  YY    _]\r\n");
	printf("|    \\ |   [_ |  _  ||     |    |   |   ||     ||     ||   [_ \r\n");
	printf("|  .  Y|     T|  |  ||     |    |   |   |l     !|     ||     T\r\n");
	printf("l__j\\_jl_____jl__j__jl_____j    l___j___j \\___/ l_____jl_____j\r\n\n");

	printf("-------------------------------------------------------------------------\r\n\n");
}


//------------------------------------------------------------------------------
// MAIN FUNCTION
//------------------------------------------------------------------------------


void main(void)
{
	float F = 0;
	
	unsigned long C = 0;
	unsigned long C_times_100 = 0;
	unsigned long F_prev = 0;
	unsigned long F_prev2 = 0;
	unsigned long F_prev3 = 0;
	unsigned long average_health = 0;
	
	double K = 1.44;
	
	long int count;
	int counter = 0;
	int capacitor = 100;
	int i = 0;
	int health_check = 0;
	
	unsigned char solid[8] = {0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F};
	char buff[20];
	char temp[20];
	char C_unit = 'p';
	
	LCD_CreateChar(0, solid);
	
	CFGCON = 0;
  
    UART2Configure(115200);  // Configure UART2 for a baud rate of 115200
    LCD_4BIT();
    
    ANSELB &= ~(1<<6); // Set RB6 as a digital I/O
    TRISB |= (1<<6);   // configure pin RB6 as input
    CNPUB |= (1<<6);   // Enable pull-up resistor for RB6
    
    ANSELA &= ~0x0002;
    TRISA |= 0x0002;
    CNPUA |= 0x0002;
        
    ANSELB &= ~0x0002;
    TRISB |= 0x0002;
    CNPUB |= 0x0002;
 
	waitms(500);
	
	LCDprint("Capacitor Reader", 1, 1);
    
    while(1)
    {
		count=GetPeriod(10);
		if(count>0)
		{
			F_prev3 = F_prev2;
			F_prev2 = F_prev;
			F_prev = F;
			F=(count*2.0)/(SYSCLK*100.0);
			F=1/F;
			C=(K*10)/(5.1*3*F) * 1000000000;
			C_times_100=(K*100*10)/(5.1*3*F) * 1000000000;
			capacitor = getCapacitor(C);
			
			
			if(health_check == 1)
			{
				if(BUY_BUTTON == 0)
				{
					while(BUY_BUTTON == 0)
					{
						if(CHANGE_BUTTON == 0)
						{
							health_check = 0;
							while(BUY_BUTTON == 0){}
							while(CHANGE_BUTTON == 0){}
							goto done_editing;
						}
					}
				}
				
				healthModePrint();
				LCDprint("Health Mode:", 1, 1);
				
				if((F_prev3 + F_prev2 + F_prev) > 3*F)
					average_health = 100*(3*F)/(F_prev3 + F_prev2 + F_prev);
				else
					average_health = 100*(F_prev3 + F_prev2 + F_prev)/(3*F);
				
				sprintf(temp, "             %.0lu", average_health);
				printf("HEALTH = %.1lu%%\r\n\n", average_health);
				
				LCDprint(temp, 2, 1);
				
			    int num_hashes;
			    int j;
			
			    num_hashes = (average_health + 3) / 10;

			    WriteCommand(0xC0);
			    
			    WriteData('[');
			    printf("[");

			    for (j = 0; j < num_hashes; j++)
			    {
			        WriteData('#');
			        printf("#");
			    }
			
			    for (; j < 10; j++)
			    {
			        WriteData(' ');
			        printf(" ");
			    }
				
				printf("]");
				WriteData(']');
				
				goto end;
				
			}
			
			if(BUY_BUTTON == 0)
			{
				while(BUY_BUTTON == 0)
				{
					if(CHANGE_BUTTON == 0)
					{
						health_check = 1;
						while(BUY_BUTTON == 0){}
						while(CHANGE_BUTTON == 0){}
						goto done_editing;
					}
				}
				
				
				buy_mode_start:
				buyModePrint();
	
	
				if(capacitor == 100)
				{
					printf("Capacitance = %lu.%02lu %cF \r\nCapacitor: NULL\r\n\nNo Capacitor Found. Going Back.\r\n", C_times_100/100, C_times_100%100, C_unit);
					LCDprint("Capacitor : NULL", 1, 1);
					LCDprint("error going back", 2, 1);
					waitms(2000);
					goto button_start;
				}
				else
				{
					printf("Capacitance = %lu.%02lu %cF \r\nCapacitor: %d\r\n\nWhat do you want to: ", C_times_100/100, C_times_100%100, C_unit, capacitor);
					sprintf(temp, "Capacitor : %d", capacitor);
					LCDprint(temp, 1, 1);
					LCDprint("input command", 2, 1);
				}
				
				
				fflush(stdout); // GCC peculiarities: need to flush stdout to get string out without a '\n'
				fgets(buff, sizeof(buff)-1, stdin);	
				
				buff[strcspn(buff, "\r\n")] = 0;
				
				if(strcmp(buff, "buy") == 0)
				{
				
					buyModePrint();
					printf("   READING CAPACITOR [");
					for(i=0; i<10; i++) {
					    printf("#"); 
					    fflush(stdout);
					    waitms(75); 
					}
					printf("] DONE!\r\n\n");
					printf("-----------------------------------------------------------------------------------\r\n\n");
					waitms(1000);
					
					buyModePrint();
					printf("   BUY CAPACITOR: %d ( %lu.%02lu %cF ) HERE\r\n\n", capacitor, C_times_100/100, C_times_100%100, C_unit);
					printf("-----------------------------------------------------------------------------------\r\n\n");
					if(capacitor == 102)
					{
						printf("Link  : https://www.amazon.ca/dp/B09JWZG88Z\r\n\n");
						printf("Price Total : $6.89 for 100\r\n");
						printf("Price Per : $0.07 each\r\n\n");	
					}
					else if(capacitor == 103)
					{
						printf("Link  : https://www.amazon.ca/dp/B07SXRLHLR\r\n\n");
						printf("Price Total : $8.91 for 25\r\n");
						printf("Price Per : $0.36 each\r\n\n");
					}
					else if(capacitor == 104)
					{
						printf("Link  : https://www.amazon.ca/dp/B00W8TM7FW\r\n\n");
						printf("Price Total : $9.99 for 50\r\n");
						printf("Price Per : $0.20 each\r\n\n");
					}
					else if(capacitor == 105)
					{
						printf("Link  : https://www.amazon.ca/dp/B07JL9HJ54\r\n\n");
						printf("Price Total : $11.49 for 10\r\n");
						printf("Price Per : $1.15 each\r\n\n");
					}
					
					printf("Press button one to esc.");
					
				}
				else if(strcmp(buff, "exit") == 0)
				{
					buyModePrint();
					printf("Exit command. Going back now.");
					LCDprint("Exiting now...", 2, 1);
					waitms(2200);
					goto button_start;
				}
				else
				{
					buyModePrint();
					printf("Not a valid command. Try again.");
					LCDprint("error going back", 2, 1);
					waitms(1600);
					goto buy_mode_start;
				}
				
				
				
				
				waitms(50);
				while(BUY_BUTTON == 1)
				{
				
				}
				waitms(50);
				while(BUY_BUTTON == 0){}
			}
			
			button_start:
			
			
			if(CHANGE_BUTTON == 0)
			{
				counter++;
				while(CHANGE_BUTTON == 0){
					if(BUY_BUTTON == 0)
					{
						while(BUY_BUTTON == 0){}
						while(CHANGE_BUTTON == 0){}
						
						counter = 0;
						
						
						calModePrint();
						
						printf("Press button 1 to INC and button 2 to DEC.\r\n");
						
						LCDprint("Calibrating:", 1, 1);
						
						while(1)
						{
							C=(K)/(5.1*3*F_prev) * 1000000000;
							C_times_100=(K*100)/(5.1*3*F_prev) * 1000000000;
							
							sprintf(temp, "C = %lu.%02lu pF", C_times_100/100, C_times_100%100);
							C_unit = 'p';
							LCDprint(temp, 2, 1);
							
							if(BUY_BUTTON == 0)
							{
								while(BUY_BUTTON == 0){}
								K += 0.01;
								calModePrint();
								
								printf("Capacitance = %lu.%02lu %cF\r\n", C_times_100/100, C_times_100%100, C_unit);
							}
							if(CHANGE_BUTTON == 0)
							{
								while(CHANGE_BUTTON == 0)
								{
									if(BUY_BUTTON == 0)
									{
										goto done_editing;
									}
								}
								calModePrint();
										
								printf("Capacitance = %lu.%02lu %cF\r\n", C_times_100/100, C_times_100%100, C_unit);
								K -= 0.01;
							}
	
						}		
					}
				}
			}
			
			goto done_editing_2;
			done_editing:
				while(BUY_BUTTON == 0){}
				while(CHANGE_BUTTON == 0){}
			done_editing_2:
			
			F_prev = F_prev*10/F;
			
			if(F_prev > 12 || F_prev < 8)
				LCDprint("Unstable check! ", 1, 1);
			else
				LCDprint("Capacitor Reader", 1, 1);
			
			if(counter == 1)
			{
				C_unit = 'p';
			}
			if(counter == 2)
			{
				C_times_100=C_times_100/1000;
				C_unit = 'n';
			}
			if(counter == 3)
			{
				C_times_100=C_times_100/1000000;
				C_unit = 'u';
			}
			if(counter == 4)
				counter = 0;
				
			if(counter > 0)
			{
				sprintf(temp, "C = %lu.%02lu %cF", C_times_100/100, C_times_100%100, C_unit);
				goto end;
			}


			
			C_unit = 'p';	
			if(C >= 1000)
			{
				C_times_100=C_times_100/1000;
				C=C/1000;
				C_unit = 'n';
			}
			
			if(C >= 1000)
			{
				C_times_100=C_times_100/1000;
				C=C/1000;
				C_unit = 'u';
			}
			
			
			sprintf(temp, "C = %lu.%02lu %cF", C_times_100/100, C_times_100%100, C_unit);			
			
			end:
			
			if(health_check == 0)
			{
				readModePrint();
				
				if(capacitor == 100)
					printf("\rFrequency = %d.%02dHz \r\nCapacitance = %lu.%02lu %cF \r\nCapacitor: NULL\r\n", (int)F, (int)((F - (int)F) * 100), C_times_100/100, C_times_100%100, C_unit);
				else
					printf("\rFrequency = %d.%02dHz \r\nCapacitance = %lu.%02lu %cF \r\nCapacitor: %d\r\n", (int)F, (int)((F - (int)F) * 100), C_times_100/100, C_times_100%100, C_unit, capacitor);
			
				LCDprint(temp, 2, 1);
			}

		}
		else
		{
			LCDprint("Unstable check! ", 1, 1);
			printf("\x1b[2J\x1b[H"); // clears terminal
			printf("NO SIGNAL                                   \r");
		}
		fflush(stdout); // GCC peculiarities: need to flush stdout to get string out without a '\n'
		waitms(300);
    }
}
