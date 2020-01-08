#include "mote_sensors.h"
#include "routing.h"

#include <dev/button-sensor.h>
#include <dev/adc-zoul.h>	 // ADC
#include <dev/zoul-sensors.h> // Sensor functions
#include <dev/sys-ctrl.h>
#include "cfs/cfs.h"
#include "clock.h"

#define TEMP_READ_INTERVAL CLOCK_SECOND * 5

//static int threshold_light = 100;

static struct etimer sensor_data_read_timer;

const uint16_t get_light_sensor_value(float m, float b, uint16_t adc_input)
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
	set_num_sensor_data(get_num_sensor_data() + 1);
	uint16_t timestamp = (uint16_t)clock_time();

	// timestamps
	int f_timestamps = cfs_open(FILE_SENSOR_DATA_TIMESTAMP, CFS_WRITE + CFS_APPEND);
	if (f_timestamps != -1)
	{
		cfs_write(f_timestamps, &timestamp, sizeof(uint16_t));
		cfs_close(f_timestamps);
        printf("timestamp: %u\r\n", timestamp);
	}

	// temperature
	int f_temp = cfs_open(FILE_TEMPERATURE, CFS_WRITE + CFS_APPEND);
	if (f_temp != -1)
	{
		cfs_write(f_temp, &temp, sizeof(uint16_t));
		cfs_close(f_temp);
        printf("temp: %u\r\n", temp);
	}

	// humidity
	int f_hum = cfs_open(FILE_HUMIDITY, CFS_WRITE + CFS_APPEND);
	if (f_hum != -1)
	{
		cfs_write(f_hum, &hum, sizeof(uint16_t));
		cfs_close(f_hum);
        printf("hum: %u\r\n", hum);
	}

	// light
	int f_light = cfs_open(FILE_LIGHT, CFS_WRITE + CFS_APPEND);
	if (f_light != -1)
	{
		cfs_write(f_light, &light, sizeof(uint16_t));
		cfs_close(f_light);
        printf("light: %u\r\n", light);
	}
}

const uint16_t fetch_sensor_data(const char *filename, const uint16_t index)
{
	uint16_t data = 0;
	int file = cfs_open(FILE_NUM_HOPS, CFS_READ);
	if (file != -1)
	{
		cfs_seek(file, sizeof(uint16_t) * index, CFS_SEEK_SET); // jump to right position
		cfs_read(file, &data, sizeof(uint16_t));
		cfs_close(file);
	}
	return data;
}

//Prozesse
PROCESS(p_sensors, "");
PROCESS_THREAD(p_sensors, ev, data)
{
	PROCESS_BEGIN();
	leds_off(LEDS_ALL);

	uint16_t light_raw; // adc1
	uint16_t hum;		//adc3
	uint16_t temp;		//onboard

	/* Configure the ADC ports */
	adc_zoul.configure(SENSORS_HW_INIT, ZOUL_SENSORS_ADC1 | ZOUL_SENSORS_ADC3);

	etimer_set(&sensor_data_read_timer, TEMP_READ_INTERVAL);

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
		else if (ev == PROCESS_EVENT_TIMER)
		{
			// Read ADC values. Data is in the 12 MSBs
			light_raw = adc_zoul.value(ZOUL_SENSORS_ADC1) >> 4; // Light Sensor
			hum = adc_zoul.value(ZOUL_SENSORS_ADC3) >> 4;		// Humidity Sensor
			temp = cc2538_temp_sensor.value(CC2538_SENSORS_VALUE_TYPE_CONVERTED);

			//Save values
			write_sensor_data(temp, hum, get_light_sensor_value(1.2, 0.0, light_raw));

			//Check Thresholds
			/*
			if (adc1_value < threshold_light)
			{
				leds_on(LEDS_RED);
			}
			else
			{
				leds_off(LEDS_RED);
			}
			*/
			etimer_set(&sensor_data_read_timer, TEMP_READ_INTERVAL);
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
		print_sensor_data();
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
		cfs_write(f_num_sensor_data, &num_sensor_data, sizeof(uint16_t));
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
	for (uint16_t i = 0; i < get_num_sensor_data(); ++i)
	{
		printf(" %8u | %8u | %8u | %8u\r\n",
			   fetch_sensor_data(FILE_SENSOR_DATA_TIMESTAMP, i),
			   fetch_sensor_data(FILE_LIGHT, i),
			   fetch_sensor_data(FILE_HUMIDITY, i),
			   fetch_sensor_data(FILE_TEMPERATURE, i));
	}
}