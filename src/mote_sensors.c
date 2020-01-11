#include "mote_sensors.h"
#include "routing.h"

#include <dev/button-sensor.h>
#include <dev/adc-zoul.h>     // ADC
#include <dev/zoul-sensors.h> // Sensor functions
#include <dev/sys-ctrl.h>
#include "cfs/cfs.h"
#include "clock.h"

#define TEMP_READ_INTERVAL CLOCK_SECOND * 3
static struct etimer sensor_data_read_timer;

//Prozesse
PROCESS(p_sensors, "");
PROCESS_THREAD(p_sensors, ev, data)
{
    PROCESS_BEGIN();

    init_sensor_data();

    uint16_t light; // adc1
    uint16_t hum;       //adc3
    uint16_t temp;      //onboard

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
            light = get_light_sensor_value(1.2, 0.0, adc_zoul.value(ZOUL_SENSORS_ADC1) >> 4); // Light Sensor
            hum = adc_zoul.value(ZOUL_SENSORS_ADC3) >> 4;       // Humidity Sensor
            temp = cc2538_temp_sensor.value(CC2538_SENSORS_VALUE_TYPE_CONVERTED);

            //Save values
            write_sensor_data(temp, hum, light);
            //print_sensor_data();
            
            //Check Thresholds
            uint16_t l_low = get_threshold(LIGHT_LOW);
            uint16_t l_high = get_threshold(LIGHT_HIGH);
            uint16_t t_low = get_threshold(TEMP_LOW);
            uint16_t t_high = get_threshold(TEMP_HIGH);
            uint16_t h_low = get_threshold(HUM_LOW);
            uint16_t h_high = get_threshold(HUM_HIGH);

            if (light < l_low || light > l_high)
            {
                leds_on(LEDS_RED);
            }
            else if (temp < t_low || temp > t_high)
            {
                leds_on(LEDS_RED);
            }
            else if (hum < h_low || hum > h_high)
            {
                leds_on(LEDS_RED);
            }
            else // alles fit im schritt!
            {
                leds_off(LEDS_RED);
            }

            etimer_set(&sensor_data_read_timer, TEMP_READ_INTERVAL);
        }
    }
    PROCESS_END();
}

void init_sensor_data()
{
    // set default thresholds if necessary
    // probably needs fix: needs new way to figure out when to initialize or if already initialized??!! check if again
    if (get_threshold(LIGHT_LOW) < 0 || get_threshold(LIGHT_HIGH) < 0 || get_threshold(TEMP_LOW) < 0 || get_threshold(TEMP_HIGH) < 0 || get_threshold(HUM_LOW) < 0 || get_threshold(HUM_HIGH) < 0) 
    {
        int fd = cfs_open(FILE_SENSORS, CFS_WRITE + CFS_APPEND);
        if (fd != -1)
        {
            int32_t init_thresholds[6] = {0,50000,0,100,0,50000};
            uint16_t init_sensordata[4 * MAX_NUM_OF_VALUES] = {0};

            cfs_write(fd, init_thresholds, sizeof(int32_t) * 6);
            cfs_seek(fd, sizeof(int32_t) * 6, CFS_SEEK_SET);
            cfs_write(fd, init_sensordata, sizeof(uint16_t) * 4 * MAX_NUM_OF_VALUES);
            cfs_close(fd);
        }
    }
}

void write_sensor_data(const uint16_t temp, const uint16_t hum, const uint16_t light)
{
    uint16_t timestamp = clock_time();
    int32_t thresholds[6];
    uint16_t sensordata[4 * MAX_NUM_OF_VALUES];
    uint16_t new_sensordata[4 * MAX_NUM_OF_VALUES];

    printf("Write: timestamp = %i; temp = %i; hum = %i; light = %i\n", timestamp, temp, hum, light);

    // Put existing data from file in buffer
    int fd_open = cfs_open(FILE_SENSORS, CFS_READ);
    if (fd_open != -1)
    {
        cfs_seek(fd_open, 0, CFS_SEEK_SET); // Redundant?
        cfs_read(fd_open, thresholds, sizeof(int32_t) * 6);
        cfs_seek(fd_open, sizeof(int32_t) * 6, CFS_SEEK_SET);
        cfs_read(fd_open, sensordata, sizeof(uint16_t) * 4 * MAX_NUM_OF_VALUES);
        cfs_close(fd_open);

        for (uint16_t i = 0; i < 4 * (MAX_NUM_OF_VALUES - 1); i++)
        {
            new_sensordata[i] = sensordata[i+4];
        }
        new_sensordata[4 * MAX_NUM_OF_VALUES - 4] = timestamp;
        new_sensordata[4 * MAX_NUM_OF_VALUES - 3] = temp;
        new_sensordata[4 * MAX_NUM_OF_VALUES - 2] = hum;
        new_sensordata[4 * MAX_NUM_OF_VALUES - 1] = light;
    }

    // Remove file
    cfs_remove(FILE_SENSORS);

    // Write adjusted data with new values to file
    int fd_write = cfs_open(FILE_SENSORS, CFS_WRITE + CFS_APPEND);
    if (fd_write != -1)
    {
        cfs_write(fd_write, thresholds, sizeof(int32_t) * 6);
        cfs_seek(fd_write, sizeof(int32_t) * 6, CFS_SEEK_SET);
        cfs_write(fd_write, new_sensordata, sizeof(uint16_t) * 4 * MAX_NUM_OF_VALUES);
        cfs_close(fd_write);
    }
}

const uint16_t fetch_sensor_data(const uint16_t index)
{
    uint16_t data = 0;
    int file = cfs_open(FILE_SENSORS, CFS_READ);
    if (file != -1)
    {
        cfs_seek(file, sizeof(int32_t) * 6 + sizeof(uint16_t) * index, CFS_SEEK_SET); // jump to right position
        cfs_read(file, &data, sizeof(uint16_t));
        cfs_close(file);
    }
    return data;
}

void on_button_pressed(void)
{
    if (button_sensor.value(0) == 0) // pressed
    {
        //init_network(); // for testing
        //init_rreq_reply(find_best_route());
        //add_device_to_network();
        print_sensor_data();
    }
    /*
    else if (button_sensor.value(0) == 8) // released
    {
        //get_threshold()
        //leds_off(LEDS_ALL);
    }*/
}

void clear_sensor_data()
{
    cfs_remove(FILE_SENSORS);
}

void print_sensor_data()
{
    printf("Thresholds: \r\n");
    printf("          | -%7li | -%7li | -%7li \r\n", get_threshold(TEMP_LOW), get_threshold(HUM_LOW), get_threshold(LIGHT_LOW));
    printf("          | +%7li | +%7li | +%7li \r\n", get_threshold(TEMP_HIGH), get_threshold(HUM_HIGH), get_threshold(LIGHT_HIGH));
    printf("----------+----------+----------+-----------\r\n");
    printf("Timestamp |  Temp    | Humidity | Light     \r\n");
    printf("----------+----------+----------+-----------\r\n");

    for (uint16_t i = 0; i < MAX_NUM_OF_VALUES; ++i)
    {
        printf(" %8u | %8u | %8u | %8u\r\n",
               fetch_sensor_data(i * 4),      // time
               fetch_sensor_data(i * 4 + 1),  // temperature
               fetch_sensor_data(i * 4 + 2),  // humidity
               fetch_sensor_data(i * 4 + 3)); // light
    }
}

void write_thresholds(const char *str)
{
    int32_t new_thresholds[6];
    char* tmp = strtok(str, ":");
    for (uint8_t i = 0; i < 6; i++)
    {
        new_thresholds[i] = atoi(tmp);
        tmp = strtok(NULL, ":");
    }

    int32_t thresholds[6] = {-1};
    int f_th = cfs_open(FILE_SENSORS, CFS_READ);
    if (f_th != -1)
    {
        cfs_seek(f_th, 0, CFS_SEEK_SET); // jump to first position
        cfs_read(f_th, thresholds, sizeof(int32_t)*6);
        cfs_close(f_th);
    }

    cfs_remove(FILE_SENSORS);

    if (new_thresholds[TEMP_LOW] >= 0) { thresholds[TEMP_LOW] = new_thresholds[TEMP_LOW]; }
    if (new_thresholds[TEMP_HIGH] >= 0) { thresholds[TEMP_HIGH] = new_thresholds[TEMP_HIGH]; }
    if (new_thresholds[HUM_LOW] >= 0) { thresholds[HUM_LOW] = new_thresholds[HUM_LOW]; }
    if (new_thresholds[HUM_HIGH] >= 0) { thresholds[HUM_HIGH] = new_thresholds[HUM_HIGH]; }
    if (new_thresholds[LIGHT_LOW] >= 0) { thresholds[LIGHT_LOW] = new_thresholds[LIGHT_LOW]; }
    if (new_thresholds[LIGHT_HIGH] >= 0) { thresholds[LIGHT_HIGH] = new_thresholds[LIGHT_HIGH]; }

    uint16_t sensordata[4 * MAX_NUM_OF_VALUES];

    // Put existing data from file in buffer
    int fd_open = cfs_open(FILE_SENSORS, CFS_READ);
    if (fd_open != -1)
    {
        cfs_seek(fd_open, 0, CFS_SEEK_SET); // Redundant?
        cfs_read(fd_open, thresholds, sizeof(int32_t) * 6);
        cfs_seek(fd_open, sizeof(int32_t) * 6, CFS_SEEK_SET);
        cfs_read(fd_open, sensordata, sizeof(uint16_t) * 4 * MAX_NUM_OF_VALUES);
        cfs_close(fd_open);
    }
    // Remove file
    cfs_remove(FILE_SENSORS);

    // Write adjusted data with new values to file
    int fd_write = cfs_open(FILE_SENSORS, CFS_WRITE + CFS_APPEND);
    if (fd_write != -1)
    {
        cfs_write(fd_write, thresholds, sizeof(int32_t) * 6);
        cfs_seek(fd_write, sizeof(int32_t) * 6, CFS_SEEK_SET);
        cfs_write(fd_write, sensordata, sizeof(uint16_t) * 4 * MAX_NUM_OF_VALUES);
        cfs_close(fd_write);
    }
}

const int32_t get_threshold(enum thresh id)
{
    int32_t threshold = -1;
    int f_th = cfs_open(FILE_SENSORS, CFS_READ);
    if (f_th != -1)
    {
        cfs_seek(f_th, id * sizeof(int32_t), CFS_SEEK_SET);
        cfs_read(f_th, &threshold, sizeof(int32_t));
        cfs_close(f_th);
    }

    return threshold;
}

const uint16_t get_light_sensor_value(float m, float b, uint16_t adc_input)
{
    double SensorValue = adc_input / 4.096; //Read voltage from the phidget interface
    double lum = m * SensorValue + b; //Convert the voltage in lux with the provided formula
    return lum > 1000.0 ? 1000.0 : lum; //Return the value of the light with maximum value equal to 1000
}