/*
 * File:   main.c
 * Author: PRIYANKA GONI
 *
 * Created on 8 October, 2026, 1:03 PM
 */

#include "clcd.h"
#include "adc.h"
#include "matrix_keypad.h"
#include "external_eeprom.h"
#include "ds1307.h"
#include "I2C.h"
#include "uart.h"

char Gear[9][3] = {"ON","GN","G1", "G2", "G3","G4", "G5","GR","CO"};
unsigned char gear = 0;
unsigned char key;
unsigned int speed;
unsigned char speed_str[3];
unsigned char eeprom_addr=0;//where next log record begins
//unsigned char event_count=0;
char time[9]="00:00:00";
unsigned char clock_reg[3];//rtc reg values
unsigned char log_count = 0;
unsigned char oldest_addr = 0;//where oldest event currently starts
void view_log(void);
void download_log(void);
void clear_log(void);
void set_time(void);

unsigned char get_record_addr(unsigned char index);


//recording an event
void store_event(char *time, char *event, char *speed)
{
    unsigned char i;
    //to store time
    for (i = 0; i < 8; i++)
        write_eeprom(eeprom_addr++, time[i]);
    
    //to store gear event
    for (i = 0; i < 2; i++)
        write_eeprom(eeprom_addr++, event[i]);
    
    //to store speed
    for (i = 0; i < 2; i++)
        write_eeprom(eeprom_addr++, speed[i]);
    
    if (log_count < 10)
    {
        log_count++;
    }
    else //if log count is greater than 10  
    {
        oldest_addr += 12;//as one complete log occupies 12 bytes ,moving oldest_addr forward by 12 to next record in circular buffer
        if (oldest_addr >= 120)//checking for end of records
            oldest_addr = 0;//changes pointer back to beginning
    }

    if (eeprom_addr >= 120)
        eeprom_addr = 0;//start writing from beginning again

   // event_count = log_count;
}


//reading the rtc
static void get_time(void)
{
	clock_reg[0] = read_ds1307(HOUR_ADDR);
	clock_reg[1] = read_ds1307(MIN_ADDR);
	clock_reg[2] = read_ds1307(SEC_ADDR);

	if (clock_reg[0] & 0x40)
	{
		time[0] = '0' + ((clock_reg[0] >> 4) & 0x01);
		time[1] = '0' + (clock_reg[0] & 0x0F);
	}
	else
	{
		time[0] = '0' + ((clock_reg[0] >> 4) & 0x03);
		time[1] = '0' + (clock_reg[0] & 0x0F);
	}
	time[2] = ':';
	time[3] = '0' + ((clock_reg[1] >> 4) & 0x0F);
	time[4] = '0' + (clock_reg[1] & 0x0F);
	time[5] = ':';
	time[6] = '0' + ((clock_reg[2] >> 4) & 0x0F);
	time[7] = '0' + (clock_reg[2] & 0x0F);
	time[8] = '\0';
}    

unsigned char main_menu = 0;
unsigned char menu_pos = 0;

const char menu_items[4][17] = {"View Log","Download Log","Clear Log","Set Time"};


//to display main menu
static void display_menu(void)
{
    switch (menu_pos)
    {
        case 0:
            clcd_print("->View Log      ", LINE1(0));
            clcd_print("  Download Log  ", LINE2(0));
            break;

        case 1:
            clcd_print("  View Log      ", LINE1(0));
            clcd_print("->Download Log  ", LINE2(0));
            break;

        case 2:
            clcd_print("  Download Log  ", LINE1(0));
            clcd_print("->Clear Log     ", LINE2(0));
            break;

        case 3:
            clcd_print("  Clear Log     ", LINE1(0));
            clcd_print("->Set Time      ", LINE2(0));
            break;
    }
}

void main(void)
{
    init_clcd();
    init_adc();
    init_matrix_keypad();
    init_i2c();
    init_ds1307();
    init_uart();

    while (1)
    {
        get_time();//updates the string

        speed = read_adc(4) / 10.3;
        if (speed > 99)
        {
            speed = 99;
        }
        //conversion of speed to characters
        speed_str[0] = (speed / 10) + '0';
        speed_str[1] = (speed % 10) + '0';
        speed_str[2] = '\0';

        key = read_switches(STATE_CHANGE);
        if (main_menu == 1)//if we are in main menu part
        {
            if (key == MK_SW1)//increment i.e move forward to menu options
            {
                if (menu_pos < 3)
                {
                    menu_pos++;
                }
            }
            else if (key == MK_SW2)//move backward
            {
                if (menu_pos > 0)
                {
                    menu_pos--;
                }
            }
            else if(key == MK_SW12)//exit the main menu
            {
                main_menu = 0;
                CLEAR_DISP_SCREEN;
            }
            else if (key == MK_SW11)//enter into selected option of main menu
            {
                if (menu_pos == 0)//if position of main is 0..its view log
                {
                    view_log();
                }
                else if (menu_pos == 1)
                {
                    download_log();
                }
                else if (menu_pos == 2)
                {
                    clear_log();
                }
                else if (menu_pos == 3)
                {
                    set_time();
                }
            }
            if(key ==MK_SW1 ||key ==  MK_SW2)
            {
                CLEAR_DISP_SCREEN;
                display_menu();
            }
            continue;  
        }
        if (key == MK_SW11)//enter main menu from dashboard
        {
            main_menu = 1;
            menu_pos = 0;
            CLEAR_DISP_SCREEN;	
            display_menu();
            continue;
        }
        //changes gear on dashboard
        if (key == MK_SW1)
        {
            if(gear == 8)
            {
                gear = 1;
            }
            else if (gear < 7)
            {
                gear++;
            }
            store_event(time, Gear[gear], speed_str);  
        }
        else if (key == MK_SW2)
        {
            if(gear == 8)
            {
                gear = 1;
            }
            else if (gear > 0)
            {
                gear--;
            }
            store_event(time, Gear[gear], speed_str);  
        }
        else if(key == MK_SW3)
        {
            gear = 8;
            store_event(time, Gear[gear], speed_str);
        }
        clcd_print("Time     EV SP  ", LINE1(0));
        clcd_print(time, LINE2(0));
        clcd_print(Gear[gear], LINE2(9));
        clcd_print(speed_str, LINE2(12));
    }
}
 


#define RECORD_SIZE 12//12bytes of total records
#define MAX_LOGS    10//total of 10 records
//to find log in eeprom
unsigned char get_record_addr(unsigned char index)//index tells function which log we need to access
{
    unsigned char addr;
    //when eeprom is not full
    if (log_count < MAX_LOGS)
        addr = index * RECORD_SIZE;//index 0x12=12 addr....10x12=120
    else
        //(starting addr of oldest log + how many bytes to move forward to reach the required log) % to keep addr within 120 bytes
        addr = (oldest_addr + index * RECORD_SIZE) % 120;
    while(addr >= 120)
    {
        addr-=120;
    }
    return (unsigned char)addr;
}


void view_log(void)
{
    unsigned char page = 0;//identify current page..page 0 : record 1 n record 2
    unsigned char key;
    unsigned char addr;
    unsigned char i;
    unsigned char index;
    unsigned char record_count;
    unsigned char row;
    unsigned char col;
 
    char line[17];

    record_count = log_count;
    //if no logs found
    if (record_count == 0)
    {
        CLEAR_DISP_SCREEN;
        clcd_print("No Logs Found", LINE1(0));
        clcd_print("SW12: Back", LINE2(0));

        while (1)
        {
            key = read_switches(STATE_CHANGE);
            if (key == 12)
                return;
        }
    }

    while (1)
    {
        for (row = 0; row < 2; row++)
        {
            //ex:page=1,row =0--> index=2(3rd record)
            index = page * 2 + row;
            //check if record exist
            if (index < record_count)
            {
                addr = get_record_addr(index);
                //convert index into record number displayed on lcd
                if(index < 9)
                {
                    line[0] = '0' + (index + 1);
                    line[1] = ' ';//add space after single digit
                }
                else
                {
                    //to display record number 10 using two characters
                    line[0] = '1';
                    line[1] = '0';
                }
                //read 8 time characters from eeprom
                for (col = 0; col < 8; col++)
                    line[2 + col] = read_eeprom(addr + col);
                line[10] = ' ';
                //read event 2 characters from eeprom
                line[11] = read_eeprom(addr + 8);
                line[12] = read_eeprom(addr + 9);
                line[13] = ' ';
                //read speed 2 characters from eeprom
                line[14] = read_eeprom(addr + 10);
                line[15] = read_eeprom(addr + 11);
                line[16] = '\0';
                if (row == 0)//display first record in line 1
                    clcd_print(line, LINE1(0));
                else
                    clcd_print(line, LINE2(0));//display record 2
            }
        }
        key = read_switches(STATE_CHANGE);
        if (key == MK_SW1)//move to next page
        {
            if ((page + 1) * 2 < record_count)//check if next page contains record
                page++;
        }
        else if (key == MK_SW2)
        {
            if (page > 0)
                page--;
        }
        else if (key == 12)
        {
            return;
        }
    }
}


void download_log(void)
{
    unsigned char i;
    unsigned char j;
    unsigned char addr;

    CLEAR_DISP_SCREEN;
    clcd_print("Downloading...", LINE1(0));
    for(int k=0;k<400;k++);//delay

    puts("\r\nCAR BLACK BOX LOGS\r\n");

    for (i = 0; i < log_count; i++)
    {
        addr = get_record_addr(i);//get starting addr
        puts("Log ");
        putch('0' + (i + 1) / 10);//print tens digit of record number
        putch('0' + (i + 1) % 10);//print unit digit of record numbers
        puts(" Time: ");
        //read n transmit time
        for (j = 0; j < 8; j++)
            putch(read_eeprom(addr + j));

        puts(" Gear: ");
        //read n transmit gear event
        putch(read_eeprom(addr + 8));
        putch(read_eeprom(addr + 9));

        puts(" Speed: ");
        //read n transmit speed 
        putch(read_eeprom(addr + 10));
        putch(read_eeprom(addr + 11));

        puts("\r\n");//new line
    }

    puts("End of logs\r\n");

    CLEAR_DISP_SCREEN;
    clcd_print("Download Done", LINE1(0));
    clcd_print("SW12: Back", LINE2(0));

    while (read_switches(STATE_CHANGE) != 12)//wait until exit sw is pressed
        ;
}

void clear_log(void)
{
    log_count = 0;
    //event_count = 0;
    eeprom_addr = 0;
    oldest_addr = 0;

    CLEAR_DISP_SCREEN;
    clcd_print("Log Cleared", LINE1(0));
    clcd_print("                ", LINE2(0));

    for (unsigned long int k = 0; k < 90000; k++);
}



void set_time(void)
{
    unsigned char hr, min, sec;
    unsigned char field = 0;
    unsigned char key;
    unsigned char old_hr, old_min, old_sec;
    unsigned char cursor_pos;
    //read current hr,min,sec from ds1307
    old_hr = read_ds1307(HOUR_ADDR);
    old_min = read_ds1307(MIN_ADDR);
    old_sec = read_ds1307(SEC_ADDR);

    if (old_hr & 0x40)//check whether rtc is operating in 12/24 hr mode
    {
        unsigned char pm = old_hr & 0x20;
        //convert bcd hour value into decimal
        hr = ((old_hr & 0x1F) >> 4) * 10 + (old_hr & 0x0F);
        if (hr == 12)
            hr = 0;
        if (pm)
            hr += 12;
    }
    else
    {
        old_hr &= 0x3F;
        hr = ((old_hr >> 4) * 10) + (old_hr & 0x0F);
    }

    old_min &= 0x7F;
    old_sec &= 0x7F;

    min = ((old_min >> 4) * 10) + (old_min & 0x0F);//convert bcd minutes into decimal
    sec = ((old_sec >> 4) * 10) + (old_sec & 0x0F);//convert bcd sec in decimal

    if (hr > 23)
    {
        hr = 0;
    }
    if (min > 59) 
        min = 0;
    if (sec > 59) 
        sec = 0;

    CLEAR_DISP_SCREEN;
    clcd_print("Set Time:", LINE1(0));

    while (1)
    {
        //convert hr ,min,sec into ASCII digits
        time[0] = '0' + hr / 10;
        time[1] = '0' + hr % 10;
        time[2] = ':';
        time[3] = '0' + min / 10;
        time[4] = '0' + min % 10;
        time[5] = ':';
        time[6] = '0' + sec / 10;
        time[7] = '0' + sec % 10;
        time[8] = '\0';

        clcd_print(time, LINE2(0));
        //lcd cursor position based on selected feild
        if (field == 0)
            cursor_pos = LINE2(0);
        else if (field == 1)
            cursor_pos = LINE2(3);
        else
            cursor_pos = LINE2(6);
        //enable lcd display,cursor n blinking cursor by passing command 0x0F
        clcd_write(0x0F, INSTRUCTION_COMMAND);//instruction command to tell driver that its an instruction not display data
        clcd_write(cursor_pos, INSTRUCTION_COMMAND);//move cursor to selected field

        key = read_switches(STATE_CHANGE);
        if (key == MK_SW1)
        {
            if (field == 0)
                hr = (hr >= 23) ? 0 : hr + 1;//23-->0
            else if (field == 1)
                min = (min >= 59) ? 0 : min + 1;//59-->0
            else
                sec = (sec >= 59) ? 0 : sec + 1;//59-->0
        }
        else if (key == MK_SW2)
        {
            field = (field + 1) % 3;//cycle through fields 0,1 and 2
        }
        else if (key == MK_SW11)//save the edited time
        {
            write_ds1307(HOUR_ADDR,
                ((hr / 10) << 4) | (hr % 10));//convert decimal hr to bcd and write to rtc

            write_ds1307(MIN_ADDR,
                ((min / 10) << 4) | (min % 10));

            write_ds1307(SEC_ADDR,
                ((sec / 10) << 4) | (sec % 10));

            clcd_write(0x0C, INSTRUCTION_COMMAND);//turn off cursor n blinking
            CLEAR_DISP_SCREEN;
            return;
        }
        else if (key == MK_SW12)
        {
            clcd_write(0x0C, INSTRUCTION_COMMAND);
            CLEAR_DISP_SCREEN;
            return;
        }
    }
}


//Display
//on off control       000 0 0 0 1 D C B (Sets entire display (D) on/off,cursor on/off (C), and blinking of cursor position character (B))
//                               1 1 1 1 (as we want all to be in on state)
//                         0x0F