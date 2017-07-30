/* Picked up a blinky.c and converted it in an ECHO por ESP8266 UART. */
#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_config.h"
#include "driver/uart.h"
#include "user_interface.h"

#define BLINK_BIT BIT2
#define MAX_RX_DATA_SIZE 32

#define user_procTaskPrio        2
#define user_procTaskQueueLen    1
os_event_t    user_procTaskQueue[user_procTaskQueueLen];

static void user_procTask(os_event_t *events);

static volatile os_timer_t some_timer;

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

void post_rx_action(char* data)
{
	ets_printf ("Received: %s\n", data);
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
	system_set_os_print(0);

	// Initialize the GPIO subsystem.
	gpio_init();
	uart_init(BIT_RATE_115200, BIT_RATE_115200);

	// Disable WiFi
	wifi_station_disconnect();
	wifi_set_opmode(NULL_MODE);
	wifi_set_sleep_type(MODEM_SLEEP_T);
		
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
