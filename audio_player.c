

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <system.h>
#include <sys/alt_alarm.h>
#include <io.h>
#include "fatfs.h"
#include "diskio.h"
#include "ff.h"
#include "monitor.h"
#include "uart.h"
#include "alt_types.h"
#include <altera_up_avalon_audio.h>
#include <altera_up_avalon_audio_and_video_config.h>
#include "altera_avalon_pio_regs.h"
#include "altera_avalon_lcd_16207_regs.h"
#include "sys/alt_irq.h"
#include "altera_avalon_lcd_16207.h"

#define PSTR(_a)  _a

uint32_t acc_size;                 /* Work register for fs command */
uint16_t acc_files, acc_dirs;
FILINFO Finfo;
char Line[256];                 /* Console input buffer */
FATFS Fatfs[_VOLUMES];          /* File system object for each logical drive */
FIL File1, File2;               /* File objects */
DIR Dir;                        /* Directory object */
uint8_t Buff[8192] __attribute__ ((aligned(4)));  /* Working buffer */
int Play;
int Track;
int Music_Count;
int Speed;
int READ_BUFF; //buffer size
int Bytes_Remaining;
int Paused_Bytes_Remaining;
char Music_Array[20][20];
uint32_t Music_Size[20];
alt_up_audio_dev * Audio_Dev;
uint32_t Cnt;
int Skip;
int Pause;


static void put_rc(FRESULT rc)
{
    const char *str =
        "OK\0" "DISK_ERR\0" "INT_ERR\0" "NOT_READY\0" "NO_FILE\0" "NO_PATH\0"
        "INVALID_NAME\0" "DENIED\0" "EXIST\0" "INVALID_OBJECT\0" "WRITE_PROTECTED\0"
        "INVALID_DRIVE\0" "NOT_ENABLED\0" "NO_FILE_SYSTEM\0" "MKFS_ABORTED\0" "TIMEOUT\0"
        "LOCKED\0" "NOT_ENOUGH_CORE\0" "TOO_MANY_OPEN_FILES\0";
    FRESULT i;

    for (i = 0; i != rc && *str; i++) {
        while (*str++);
    }
    xprintf("rc=%u FR_%s\n", (uint32_t) rc, str);
}

void write_top_lcd(char * firstline, int track) {
	int i =0;
	IOWR(LCD_DISPLAY_BASE, 0, 0x80);
	usleep(50);
	char tmp[10];
	sprintf(tmp, "%d", track);
	for (i = 0; i < strlen(tmp); i++) {
		IOWR(LCD_DISPLAY_BASE, 2, tmp[i]);
		usleep(50);
	}
	IOWR(LCD_DISPLAY_BASE, 2, ':');
	usleep(50);
	for (i = 0; i < strlen(firstline); i++) {
		IOWR(LCD_DISPLAY_BASE, 2, firstline[i]);
		usleep(50);
	}
	for (i = 0; i < 10; i++) {
		IOWR(LCD_DISPLAY_BASE, 2, ' ');
		usleep(50);
	}
}

void write_bottom_lcd(char * secondline) {
	int i =0;
	IOWR(LCD_DISPLAY_BASE, 0, 0xC0);
	usleep(50);
	for (i = 0; i < strlen(secondline); i++) {
		IOWR(LCD_DISPLAY_BASE, 2, secondline[i]);
		usleep(50);
	}
	for (i = 0; i < 10; i++) {
		IOWR(LCD_DISPLAY_BASE, 2, ' ');
		usleep(50);
	}
}

int isWav(char *filename) {
	const char *last_four = &filename[strlen(filename)-4];
	if (strcasecmp(last_four, ".wav") == 0)
		return 1;
	else
		return 0;
}

int determineSpeed() {
	if ((IORD(SWITCH_PIO_BASE,0)&0b0011)==0)
		Speed = -4;
	else if ((IORD(SWITCH_PIO_BASE,0)&0b0011)==1){
		Speed = 8;
	}
	else if ((IORD(SWITCH_PIO_BASE,0)&0b0011)==2)
		Speed = 2;
	else if ((IORD(SWITCH_PIO_BASE,0)&0b0011)==3){
		Speed = 4;
	}
	return 0;
}

char* speedName() {
	switch (Speed) {
		case -4:
			return "PBACK-MONO";
			break;
		case 4:
			return "PBACK-NORM SPD";
			break;
		case 2:
			return "PBACK-HALF SPD";
			break;
		case 8:
			return "PBACK-DBL SPD";
			break;
	}
}

static void button_pressed(void* context, alt_u32 id){
	  IOWR(TIMER_0_BASE,2,0x000F);
	  IOWR(TIMER_0_BASE,3,0x004F);
	  IOWR(TIMER_0_BASE,1,0x0005);
	  IOWR(BUTTON_PIO_BASE,3,0x0);
}

static void timer_complete(void* context, alt_u32 id){
//check the button pressed and output

	switch(IORD(BUTTON_PIO_BASE,0))
	{
		case 0b1110://next
			Paused_Bytes_Remaining = 0;
			if (Track >= Music_Count){
				Track = 1;
				seven_seg_opt(Track);
			}
			else{
				Track++;
				seven_seg_opt(Track);
			}
			write_top_lcd(Music_Array[Track-1], Track);
			if (Play == 1){ //song is playing and prev pressed
				Bytes_Remaining = 0;
				Skip = 1;
			}
			break;
		case 0b1101://play/pause
			if (Play == 1) {
				Play = 0;
				write_bottom_lcd("PAUSED          ");
				Paused_Bytes_Remaining = Bytes_Remaining;
				Bytes_Remaining = 0;
			}
			else {
				Play = 1;
				determineSpeed();
				write_bottom_lcd(speedName());
				if (Paused_Bytes_Remaining != 0) {
					Bytes_Remaining = Paused_Bytes_Remaining;
					Paused_Bytes_Remaining = 0;
					Pause = 1;
				}
				else {
					select_song();
				}
			}
			break;
		case 0b1011://stop
			Play = 0;
			write_bottom_lcd("STOPPED         ");
			Bytes_Remaining = 0;
			Paused_Bytes_Remaining = 0;
			break;
		case 0b0111://prev
			Paused_Bytes_Remaining = 0;
			if (Track == 1){
				Track = Music_Count;
				seven_seg_opt(Track);
			}
			else{
				Track-=1;
				seven_seg_opt(Track);
			}
			write_top_lcd(Music_Array[Track-1], Track);
			if (Play == 1) { //song is playing and next pressed
				Bytes_Remaining = 0;
				Skip = 1;
			}
			break;
	}
	IOWR(TIMER_0_BASE,0,0x0000);
}

void select_song() {
	put_rc(f_open(&File1, Music_Array[Track-1], (uint8_t) 1));
	Bytes_Remaining = Music_Size[Track-1];
}

void seven_seg_opt (int a){
	if(a<99 && a>=0){
		int digit1 = a/10;
		int digit2 = a%10;
		int output = seven_seg(digit1);
		output = output << 8;
		output += seven_seg(digit2);
		IOWR(SEVEN_SEG_PIO_BASE,1,output);
	}
}

int seven_seg (int a){
	switch (a){
	case 0:
		return 0x3F;
		break;
	case 1:
		return 0x06;
		break;
	case 2:
		return 0x5B;
		break;
	case 3:
		return 0x4F;
		break;
	case 4:
		return 0x66;
		break;
	case 5:
		return 0x6D;
		break;
	case 6:
		return 0x7D;
		break;
	case 7:
		return 0x07;
		break;
	case 8:
		return 0x7F;
		break;
	case 9:
		return 0x67;
		break;
	default :
		return 0x00;
		break;
	}
}

int main()
{

	//INITIALIZE
	uint8_t res;



	/* used for audio record/playback */
	unsigned int l_buf;
	unsigned int r_buf;
	// open the Audio port
	Audio_Dev = alt_up_audio_open_dev ("/dev/Audio");
	if ( Audio_Dev == NULL)
		alt_printf ("Error: could not open audio device \n");
	else
		alt_printf ("Opened audio device \n");

	//IoInit();

	//xputs(PSTR("FatFs module test monitor\n"));
	//xputs(_USE_LFN ? "LFN Enabled" : "LFN Disabled");
	//xprintf(", Code page: %u\n", _CODE_PAGE);++


	//f_mount
	xprintf("rc=%d\n", (uint16_t) disk_initialize((uint8_t) 0));

	//force initialize
	put_rc(f_mount((uint8_t) 0, &Fatfs[0]));

	//Song Index

	res = f_opendir(&Dir, 0);
	if (res) // if res in non-zero there is an error; print the error.
		put_rc(res);
	for (;;)
	{
		res = f_readdir(&Dir, &Finfo);
		if ((res != FR_OK) || !Finfo.fname[0])
			break;
		if (isWav(&(Finfo.fname[0]))){
			strcpy(Music_Array[Music_Count], &(Finfo.fname[0]));
			Music_Size[Music_Count] = Finfo.fsize;
			xprintf("%9lu", Finfo.fsize);
			xprintf("%s \n", Music_Array[Music_Count]);
			Music_Count++;
		}
	}
	//enable irqs
    alt_irq_register(BUTTON_PIO_IRQ, (void *)0, button_pressed);
    IOWR(BUTTON_PIO_BASE,2,0xf);

    alt_irq_register(TIMER_0_IRQ, (void *)0, timer_complete);
    IOWR(LED_PIO_BASE, 0, 0x00);

	//select music
	Play = 0;
	Track = 1;
	Speed = 1;
	READ_BUFF=1280;
	determineSpeed();
	write_top_lcd(Music_Array[Track-1], Track);
	write_bottom_lcd("STOPPED");


	seven_seg_opt(Track);


	//play song

	while(1){
		if (Play) {
			determineSpeed();
			if (Paused_Bytes_Remaining == 0 && !Pause)
				select_song();
			//IOWR(SEVEN_SEG_PIO_BASE,1,0x0006);
			int speed = abs(Speed);
			while (Bytes_Remaining)
			{
				if ((uint32_t) Bytes_Remaining >= READ_BUFF)
				{
					Cnt = READ_BUFF;
					Bytes_Remaining -= READ_BUFF;
				}
				else
				{
					Cnt = Bytes_Remaining;
					Bytes_Remaining = 0;
				}
				res = f_read(&File1, Buff, Cnt, &Cnt);
				if (res != FR_OK)
				{
					put_rc(res);
					break;
				}
				if (!Cnt)
					break;
				int n = 0;
				l_buf = 0;
				r_buf = 0;

				if (Speed == 8){
					for(n = 0; n < Cnt; n+=speed){
						l_buf = ((Buff[n+5] << 24) | (Buff[n+4] << 16) | (Buff[n+1] << 8) | Buff[n]); //opposite endian
						r_buf = ((Buff[n+7] << 24) | (Buff[n+6] << 16) | (Buff[n+3] << 8) | Buff[n+2]);
						while (1){
							int fifospace = alt_up_audio_write_fifo_space (Audio_Dev, ALT_UP_AUDIO_RIGHT);
							if ( fifospace > 0 ) // check if there is empty space
								break;
						}
						// write audio buffer
						alt_up_audio_write_fifo (Audio_Dev, &(r_buf), 1, ALT_UP_AUDIO_RIGHT);
						alt_up_audio_write_fifo (Audio_Dev, &(l_buf), 1, ALT_UP_AUDIO_LEFT);
					}
				}
				else if (Speed == 4){
					for(n = 0; n < Cnt; n+=speed){
						/*for (k=speed/4; k > 0; k--)
						{
							l_buf = (l_buf << k*16) | ((Buff[n+1+k*4] << 8) | Buff[n+k*4]); //opposite endian, increasing the write size
							r_buf = (r_buf << k*16) | ((Buff[n+3+k*4] << 8) | Buff[n+2+k*4]);
						}*/
						l_buf = ((Buff[n+1] << 8) | Buff[n]); //opposite endian
						r_buf = ((Buff[n+3] << 8) | Buff[n+2]);
						while (1){
							int fifospace = alt_up_audio_write_fifo_space (Audio_Dev, ALT_UP_AUDIO_RIGHT);
							if ( fifospace > 0 ) // check if there is empty space
								break;
						}
						// write audio buffer
						alt_up_audio_write_fifo (Audio_Dev, &(r_buf), 1, ALT_UP_AUDIO_RIGHT);
						alt_up_audio_write_fifo (Audio_Dev, &(l_buf), 1, ALT_UP_AUDIO_LEFT);
					}
				}
				else if (speed == 2) {
					for(n = 0; n < Cnt; n+=(speed*2)){
						l_buf = (Buff[n+1] << 8);
						r_buf = (Buff[n+3] << 8);
						while (1){
							int fifospace = alt_up_audio_write_fifo_space (Audio_Dev, ALT_UP_AUDIO_RIGHT);
							if ( fifospace > 0 ) // check if there is empty space
								break;
						}
						// write audio buffer
						alt_up_audio_write_fifo (Audio_Dev, &(r_buf), 1, ALT_UP_AUDIO_RIGHT);
						alt_up_audio_write_fifo (Audio_Dev, &(l_buf), 1, ALT_UP_AUDIO_LEFT);

						l_buf = (Buff[n]);
						r_buf = (Buff[n+2]);
						while (1){
							int fifospace = alt_up_audio_write_fifo_space (Audio_Dev, ALT_UP_AUDIO_RIGHT);
							if ( fifospace > 0 ) // check if there is empty space
								break;
						}

						alt_up_audio_write_fifo (Audio_Dev, &(r_buf), 1, ALT_UP_AUDIO_RIGHT);
						alt_up_audio_write_fifo (Audio_Dev, &(l_buf), 1, ALT_UP_AUDIO_LEFT);
					}
				}
				else if (Speed == -4){ //mono
					r_buf = 0;
					for(n = 0; n < Cnt; n+=speed){
						l_buf = ((Buff[n+1] << 8) | Buff[n]); //opposite endian, increasing the write size
						while (1){
							int fifospace = alt_up_audio_write_fifo_space (Audio_Dev, ALT_UP_AUDIO_RIGHT);
							if ( fifospace > 0 ) // check if there is empty space
								break;
						}
						// write audio buffer
						alt_up_audio_write_fifo (Audio_Dev, &(l_buf), 1, ALT_UP_AUDIO_LEFT);
						alt_up_audio_write_fifo (Audio_Dev, &(r_buf), 1, ALT_UP_AUDIO_RIGHT);
					}
				}
			}
			if (Pause && Paused_Bytes_Remaining == 0) {
				Pause = 0;
			}
			if (Skip == 1) {
				Skip = 0;
			}
			else {
				Play = 0;
				if (Paused_Bytes_Remaining == 0)
					write_bottom_lcd("STOPPED         ");
			}
			xprintf("done\n");
		}
	}


    return 0;
}
