#include "mote_sensors.h"
#include "routing.h"
#include "contiki.h"
#include "net/rime/rime.h"     // Establish connections.
#include "net/netstack.h"      // Wireless-stack definitions
#include "dev/leds.h"          // Use LEDs.
#include "dev/button-sensor.h" // User Button
#include "dev/adc-zoul.h"      // ADC
#include "dev/zoul-sensors.h"  // Sensor functions
#include "dev/sys-ctrl.h"
#include "cfs/cfs.h"
// Standard C includes:
#include <stdio.h>      // For printf.
#include <string.h>
#include <time.h>
#include <stdlib.h>

#define TEMP_READ_INTERVAL CLOCK_SECOND*1

static int threshold_light = 100;
typedef struct{
	int counter;
	int temp;
	int light;
}measure;

//Wert hier anpassen
static measure debug[10];
static measure empty_measure[10];

static int getLightSensorValue(float m, float b, uint16_t adc_input){
	//Read voltage from the phidget interface
	double SensorValue = adc_input/4.096;
	//Convert the voltage in lux with the provided formula
	double lum = m * SensorValue + b;
	//Return the value of the light with maximum value equal to 1000
	return lum > 1000.0 ? 1000.0 : lum;
}

static void save_to_flash(char *filename, int fd_write, char *message, int n){

	fd_write = cfs_open(filename, CFS_WRITE);
	if(fd_write != -1) {
	    n = cfs_write(fd_write, message, n);
	    cfs_close(fd_write);
	    printf("\r\nSuccessfully written to cfs. wrote %i bytes", n);
	}
	else {
	    printf("\r\nERROR: could not write to memory.");
	}
}

static void read_from_flash(char *filename, int fd_read,  char *buf, int n){
	strcpy(buf,"empty string");
	  fd_read = cfs_open(filename, CFS_READ);
	  if(fd_read!=-1) {
	    cfs_read(fd_read, buf, n);
	    printf("\r\n%s", buf);
	    cfs_close(fd_read);
	  } else {
	    printf("\r\nERROR: could not read from memory");
	  }
}

//Prozesse
PROCESS (ext_sensors_process, "External Sensors process");
PROCESS (call_data_storage, "Debug: Call data storage");
PROCESS(p_sensors, "Sensors Event Listener");
AUTOSTART_PROCESSES (&ext_sensors_process, &call_data_storage);

PROCESS_THREAD(p_sensors, ev, data)
{
    PROCESS_BEGIN();
    leds_off(LEDS_ALL);

    while (1)
    {
        PROCESS_WAIT_EVENT();

        if (ev == sensors_event)
        {
            if (data == &button_sensor)
            {
                on_button_pressed();
            }
        }
    }
    PROCESS_END();
}

PROCESS_THREAD (ext_sensors_process, ev, data) {

	/* variables to be used */
	static struct etimer temp_reading_timer;
	static uint16_t adc1_value, adc3_value;
	static int counter = 0;
	
	//Test Daten fuer Flashspeicher
	char message[32];
	char buf[100];
	strcpy(message,"#1.hello world.");
	strcpy(buf,message);
	char *filename = "msg_file";
	int fd_read;
	int n = sizeof(message);
	
	PROCESS_BEGIN ();

	/* Configure the ADC ports */
	adc_zoul.configure(SENSORS_HW_INIT, ZOUL_SENSORS_ADC1 | ZOUL_SENSORS_ADC3);

	etimer_set(&temp_reading_timer, TEMP_READ_INTERVAL);

	while (1) {

		PROCESS_WAIT_EVENT();  // let process continue

		/* If timer expired, print sensor readings */
	    if(ev == PROCESS_EVENT_TIMER) {
		
	    	//Aus Flashspeicher lesen
		read_from_flash(filename, fd_read, buf, n);
	    	/*
	    	 * Read ADC values. Data is in the 12 MSBs
	    	 */
	    	adc1_value = adc_zoul.value(ZOUL_SENSORS_ADC1) >> 4;
	    	adc3_value = adc_zoul.value(ZOUL_SENSORS_ADC3) >> 4;

	    	/*
	    	 * Print Raw values
	    	 */

	    	printf("\r\nLight Sensor[Raw] = %d", adc1_value);
	        printf("\r\nHumidity/Temperature [Raw] = %d", adc3_value);
            printf("\r\nLuminosity [ADC1]: %d lux", getLightSensorValue(1.2, 0.0, adc1_value));

            printf("\r\n%i", counter);

            //Save values
            debug[counter].counter = counter;
            debug[counter].light= adc1_value;
            debug[counter].temp = adc3_value;

            counter += 1;
    		if(counter > 10){
    			counter = 0;
    		}

    		//Check Thresholds
    		if(adc1_value < threshold_light){
    			leds_on(LEDS_RED);
    		}
    		else{
    			leds_off(LEDS_RED);
    		}
    		etimer_set(&temp_reading_timer, TEMP_READ_INTERVAL);
	    }
    }

	PROCESS_END ();
}

PROCESS_THREAD (call_data_storage, ev, data) {
	//Daten auslesen, aktuell nach Event Button pressed, spaeter auf Anfrage GUI
	int i;
	
	//Testdaten fuer Flashspeicher
	char message[32];
	char buf[100];
	strcpy(message,"#1.hello world.");
	strcpy(buf,message);
	char *filename = "msg_file";
	int fd_write
	int n = sizeof(message);
	PROCESS_BEGIN ();

	while (1) {
		PROCESS_WAIT_EVENT();
		if(ev == sensors_event) {
			if(data == &button_sensor){
				if (button_sensor.value(BUTTON_SENSOR_VALUE_TYPE_LEVEL) == BUTTON_SENSOR_PRESSED_LEVEL) {
					printf("\r\nStorage called");
					for(i=0;i<10;i++){
						printf("\r\nNum_%i: Light: %i, Temp: %i", debug[i].counter, debug[i].light, debug[i].temp);
					}
					
				    //Speichern in Flashspeicher
				    save_to_flash(filename, fd_write, message, n);
				    //memset
				    //debug = empty_measure;
					

				}
		}
	  }
	}
	PROCESS_END ();
}

void on_button_pressed(void)
{
    if (button_sensor.value(0) == 0) // pressed
    {
        init_network(); // for testing
    }

    /*
    else if (button_sensor.value(0) == 8) // released
    {
        leds_off(LEDS_ALL);
    }
    */
}
