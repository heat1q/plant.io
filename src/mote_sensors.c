#include "mote_sensors.h"
#include "routing.h"

#include <dev/button-sensor.h>
#include <dev/adc-zoul.h>	 // ADC
#include <dev/zoul-sensors.h> // Sensor functions
#include <dev/sys-ctrl.h>
#include "cfs/cfs.h"
#include "clock.h"

#define TEMP_READ_INTERVAL CLOCK_SECOND * 1

static int threshold_light = 100;

//Wert hier anpassen
static measure debug[10];
static measure empty_measure[10];

static int getLightSensorValue(float m, float b, uint16_t adc_input)
{
	//Read voltage from the phidget interface
	double SensorValue = adc_input / 4.096;
	//Convert the voltage in lux with the provided formula
	double lum = m * SensorValue + b;
	//Return the value of the light with maximum value equal to 1000
	return lum > 1000.0 ? 1000.0 : lum;
}



void write_sensor_data(const uint16_t temp, const uint16_t hum, const uint16_t light)
{
	set_num_sensor_data(get_num_senor_data() + 1);
	uint16_t timestamp = (uint16_t) clock_time();

	// timestamps
    int f_timestamps = cfs_open(FILE_SENSOR_DATA_TIMESTAMP, CFS_WRITE + CFS_APPEND);
    if (f_timestamps != -1)
    {
        int n = cfs_write(f_timestamps, &timestamp, sizeof(uint16_t));
        cfs_close(f_timestamps);
	}

	// temperature
	int f_temp = cfs_open(FILE_TEMPERATURE, CFS_WRITE + CFS_APPEND);
    if (f_temp != -1)
    {
        int n = cfs_write(f_temp, &temp, sizeof(uint16_t));
        cfs_close(f_temp);
	}

	// humidity
	int f_hum = cfs_open(FILE_HUMIDITY, CFS_WRITE + CFS_APPEND);
    if (f_hum != -1)
    {
        int n = cfs_write(f_hum, &hum, sizeof(uint16_t));
        cfs_close(f_hum);
	}

	// light
	int f_light = cfs_open(FILE_HUMIDITY, CFS_WRITE + CFS_APPEND);
    if (f_light != -1)
    {
        int n = cfs_write(f_light, &light, sizeof(uint16_t));
        cfs_close(f_light);
	}
}

const uint16_t fetch_sensor_data(const char *filename, const uint16_t index)
{
    uint16_t data = 0;
    int file = cfs_open(FILE_NUM_HOPS, CFS_READ);
    if (filename != -1)
    {
        cfs_seek(file, sizeof(uint16_t) * index, CFS_SEEK_SET); // jump to right position
        cfs_read(file, &data, sizeof(uint8_t));
        cfs_close(file);
    }
    return data;
}


//Prozesse
PROCESS(ext_sensors_process, "External Sensors process");
PROCESS(call_data_storage, "Debug: Call data storage");
PROCESS(p_sensors, "Sensors Event Listener");
AUTOSTART_PROCESSES(&ext_sensors_process, &call_data_storage);

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

PROCESS_THREAD(ext_sensors_process, ev, data)
{

	/* variables to be used */
	static struct etimer temp_reading_timer;
	static uint16_t adc1_value, adc3_value, adc_on_board_temp;
	static int counter = 0;

	
	//Test Daten fuer Flashspeicher
	char message[32];
	char buf[100];
	strcpy(message, "#1.hello world.");
	strcpy(buf, message);
	char *filename = "msg_file";
	int fd_read;
	int n = sizeof(message);

	PROCESS_BEGIN();

	/* Configure the ADC ports */
	adc_zoul.configure(SENSORS_HW_INIT, ZOUL_SENSORS_ADC1 | ZOUL_SENSORS_ADC3);

	etimer_set(&temp_reading_timer, TEMP_READ_INTERVAL);

	while (1)
	{

		PROCESS_WAIT_EVENT(); // let process continue

		/* If timer expired, print sensor readings */
		if (ev == PROCESS_EVENT_TIMER)
		{
	    	// Read ADC values. Data is in the 12 MSBs
			adc1_value = adc_zoul.value(ZOUL_SENSORS_ADC1) >> 4; // Light Sensor
			adc3_value = adc_zoul.value(ZOUL_SENSORS_ADC3) >> 4; // Humidity Sensor
			adc_on_board_temp = cc2538_temp_sensor.value(CC2538_SENSORS_VALUE_TYPE_CONVERTED);
			/*
	    	 * Print Raw values
	    	 */

			//printf("\r\nLight Sensor[Raw] = %d", adc1_value);

			printf("\r\nHumidity [Raw] = %d", adc3_value);
			
			printf("\r\nLuminosity [ADC1]: %d lux", getLightSensorValue(1.2, 0.0, adc1_value));

			printf("\r\n%i", counter);

			//Save values
			write_sensor_data(adc_on_board_temp, adc3_value, adc1_value)

			//Check Thresholds
			if (adc1_value < threshold_light)
			{
				leds_on(LEDS_RED);
			}
			else
			{
				leds_off(LEDS_RED);
			}
			etimer_set(&temp_reading_timer, TEMP_READ_INTERVAL);
		}
	}

	PROCESS_END();
}

void on_button_pressed(void)
{
	if (button_sensor.value(0) == 0) // pressed
	{
		//init_network(); // for testing
		//init_rreq_reply(find_best_route());
	}

	/*
    else if (button_sensor.value(0) == 8) // released
    {
        leds_off(LEDS_ALL);
    }
    */
}

const uint16_t get_num_sensor_data()
{
    uint16_t num_sensor_data = 0;
    int f_num_sensor_data = cfs_open(FILE_SENSOR_DATA_LENGTH, CFS_READ);
    if (f_num_sensor_data != -1)
    {
        cfs_seek(f_num_sensor_data, 0, CFS_SEEK_SET); // jump to first position
        cfs_read(f_num_sensor_data, &num_sensor_data, sizeof(uint16_t));
        cfs_close(f_num_sensor_data);
    }
    // if file cannot be read, return 0
    return num_sensor_data;
}

void set_num_sensor_data(const uint16_t num_sensor_data)
{
    cfs_remove(FILE_SENSOR_DATA_LENGTH);
    int f_num_sensor_data = cfs_open(FILE_SENSOR_DATA_LENGTH, CFS_WRITE + CFS_APPEND);
    if (f_num_sensor_data != -1)
    {
        int n = cfs_write(f_num_sensor_data, &num_sensor_data, sizeof(uint16_t));
        cfs_close(f_num_sensor_data);
    }
}

void clear_sensor_data()
{
	set_num_sensor_data(0);
	cfs_remove(FILE_LIGHT);
	cfs_remove(FILE_TEMPERATURE);
	cfs_remove(FILE_HUMIDITY);
}

void print_sensor_data()
{
	printf("Thresholds: \r\n");
	printf("Timestamp |  Light   | Humidity | Temperatur\r\n");
	printf("----------+----------+----------+-----------\r\n");
	for (uint16_t i = 0; i < get_num_sensor_data; i++)
	{
		/* code */
	}
	
}