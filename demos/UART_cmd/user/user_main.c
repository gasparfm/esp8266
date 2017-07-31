/* Picked up a blinky.c and converted it in an ECHO por ESP8266 UART. */
#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_config.h"
#include "driver/uart.h"
#include "user_interface.h"
#include "mem.h"
#include "espmissingincludes.h"
#include <ctype.h>
#include "utils.h"

#define BLINK_BIT BIT2
#define MAX_RX_DATA_SIZE 32
#define MAX_CMD_LEN 9
#define MAX_COMMANDS 10

#define user_procTaskPrio        2
#define user_procTaskQueueLen    1
os_event_t    user_procTaskQueue[user_procTaskQueueLen];

static void user_procTask(os_event_t *events);
static volatile os_timer_t some_timer;

typedef int (*command_function)(char*);

struct TCommand
{
	int len;
	char command[MAX_CMD_LEN];
	command_function callback;
};

struct TCommand* commands[MAX_COMMANDS];
uint8_t Ncommands = 0;

int command_hello(char* args)
{
	ets_printf("Hello dude\n");
}

int command_repeat(char* args)
{
	if ( (args) && (*args) )
		ets_printf("Repeating: %s\n", args);
	else
		ets_printf("Nothing to repeat\n");
}

char* humanSize(char* buffer, size_t bufferSize, long double size, short precission)
{
	static const char* units[10]={"bytes","Kb","Mb","Gb","Tb","Pb","Eb","Zb","Yb","Bb"};

	uint8_t i= 0;

	while (size>1024) {
		size = size /1024;
		i++;
	}

	if (precission < 0)
		precission=3;

	char temp[32];
	
	os_snprintf(buffer, bufferSize, "%s%s", dtostrf(size, 6, precission, temp), units[i]);
	return buffer;
}

int command_info(char* args)
{
	char tempbuffer[32];
	ets_printf ("SDK version: %s \n", system_get_sdk_version());
	ets_printf ("Chip ID: %d \n", system_get_chip_id());
	ets_printf ("Free heap: %lu\n", system_get_free_heap_size());
	ets_printf ("Free heap: %s\n", humanSize(tempbuffer, 32, system_get_free_heap_size(), 3)); 
	ets_printf ("System time: %lu (%s)\n", system_get_time(), timeInterval(tempbuffer, 32, system_get_time()/1000000));
	ets_printf ("CPU Frequency: %uMHz\n", system_get_cpu_freq());
}

void some_timerfunc(void *arg)
{
	//Do blinky stuff
	if (GPIO_REG_READ(GPIO_OUT_ADDRESS) & BLINK_BIT)
    {
			//Set GPIO2 to LOW
			gpio_output_set(0, BLINK_BIT, BLINK_BIT, 0);
    }
	else
    {
			//Set GPIO2 to HIGH
			gpio_output_set(BLINK_BIT, 0, BLINK_BIT, 0);
    }
}

void add_command(char* command, command_function callback)
{
	
	if (Ncommands<MAX_COMMANDS) {
		commands[Ncommands] = (struct TCommand*)os_malloc(sizeof(struct TCommand));
		commands[Ncommands]->len = strlen(command);
		strcpy(commands[Ncommands]->command, command);
		commands[Ncommands]->callback = callback;
		Ncommands++;
	}
}

// https://totaki.com/poesiabinaria/2010/03/trim-un-gran-amigo-php-c-c/
char *trim(char *s)
{
  char *start = s;

  /* Nos comemos los espacios al inicio */
  while(*start && isspace(*start))
    ++start;

  char *i = start;
  char *end = start;

  /* Nos comemos los espacios al final */
  while(*i)
  {
    if( !isspace(*(i++)) )
      end = i;
  }

  /* Escribimos el terminados */
  *end = 0;

  return start;
}

void post_rx_action(char* data)
{
	char* _data = trim(data);
	char* command=_data;
	char* args = strchr(_data, ' ');
	if (args!=NULL)
		{
			*args = '\0';
			++args;
		}
	size_t cmdlen = os_strlen(_data);
	uint8_t i;
	for (i=0; i<Ncommands; ++i)
		{
			if ( (commands[i]->len == cmdlen) && (strcmp (commands[i]->command, _data)==0) )
				commands[i]->callback(args);																
		}
}

void ICACHE_FLASH_ATTR uart_rx_task(os_event_t *events)
{
	if (events->sig == 0) {
		// Sig 0 is a normal receive. Get how many bytes have been received.
		uint8_t rx_len = (READ_PERI_REG(UART_STATUS(UART0)) >> UART_RXFIFO_CNT_S) & UART_RXFIFO_CNT;

		// Parse the characters, taking any digits as the new timer interval.
		char rx_data[MAX_RX_DATA_SIZE];
		uint8_t i;
		for (i=0; i < MAX_RX_DATA_SIZE-1; i++) {
			rx_data[i] = READ_PERI_REG(UART_FIFO(UART0)) & 0xFF;
		}
		rx_data[(rx_len>MAX_RX_DATA_SIZE-1)?MAX_RX_DATA_SIZE-1:rx_len]='\0';
		post_rx_action(rx_data);

		// Clear the interrupt condition flags and re-enable the receive interrupt.
		WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_FULL_INT_CLR | UART_RXFIFO_TOUT_INT_CLR);
		uart_rx_intr_enable(UART0);
	}
}

//Do nothing function
static void ICACHE_FLASH_ATTR
user_procTask(os_event_t *events)
{
    os_delay_us(10);
}

//Init function 
void ICACHE_FLASH_ATTR user_init()
{
	// Disable debug
	system_set_os_print(1);

	// Initialize the GPIO subsystem.
	gpio_init();
	uart_init(BIT_RATE_115200, BIT_RATE_115200);

	// Disable WiFi
	wifi_station_disconnect();
	wifi_set_opmode(NULL_MODE);
	wifi_set_sleep_type(MODEM_SLEEP_T);

	// Welcome message
	ets_printf("\n\nInitialized... please insert command\n");
	// Add commands
	add_command("HELLO", command_hello);
	add_command("REPEAT", command_repeat);
	add_command("INFO", command_info);
	
	//Set GPIO2 to output mode (BLINK_BIT)
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);

	//Set GPIO2 low
	gpio_output_set(0, BLINK_BIT, BLINK_BIT, 0);

	//Disarm timer
	os_timer_disarm(&some_timer);

	//Setup timer
	os_timer_setfn(&some_timer, (os_timer_func_t *)some_timerfunc, NULL);

	//Arm the timer
	os_timer_arm(&some_timer, 1000, 1);
    
	//Start os task
	/* system_os_task(user_procTask, user_procTaskPrio,user_procTaskQueue, user_procTaskQueueLen); */
}
