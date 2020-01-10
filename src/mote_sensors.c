#include "mote_sensors.h"
#include "routing.h"

#include <dev/button-sensor.h>
#include <dev/adc-zoul.h>     // ADC
#include <dev/zoul-sensors.h> // Sensor functions
#include <dev/sys-ctrl.h>
#include "cfs/cfs.h"
#include "clock.h"

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
    uint16_t timestamp = (uint16_t)clock_time();
    uint16_t newLength = get_num_sensor_data() + 1; // New amount of sensor values
    //printf("Write: timestamp = %i; temp = %i; hum = %i; light = %i\n", timestamp, temp, hum, light);

    if (newLength <= MAX_NUM_OF_VALUES)
    { // Array wasn't full yet
        set_num_sensor_data(newLength);
        uint16_t new_data[4] = {timestamp, temp, hum, light};

        int fd = cfs_open(FILE_SENSOR_DATA, CFS_WRITE + CFS_APPEND);
        if (fd != -1)
        {
            cfs_write(fd, &new_data, 4 * sizeof(uint16_t));
            cfs_close(fd);
        }
    }
    else
    { // Array was already full
        // Get old values (except oldest)
        uint16_t data[4 * MAX_NUM_OF_VALUES];
        int file = cfs_open(FILE_SENSOR_DATA, CFS_READ);
        if (file != -1)
        {
            cfs_seek(file, sizeof(uint16_t) * 4, CFS_SEEK_SET);                   // jump to right position
            cfs_read(file, data, sizeof(uint16_t) * (MAX_NUM_OF_VALUES - 1) * 4); // copy the last 4 timestamps
            cfs_close(file);
        }

        data[4 * MAX_NUM_OF_VALUES - 4] = timestamp;
        data[4 * MAX_NUM_OF_VALUES - 3] = temp;
        data[4 * MAX_NUM_OF_VALUES - 2] = hum;
        data[4 * MAX_NUM_OF_VALUES - 1] = light;

        // Remove and subsequently replace files with new data array
        cfs_remove(FILE_SENSOR_DATA);
        int fd = cfs_open(FILE_SENSOR_DATA, CFS_WRITE);
        if (fd != -1)
        {
            cfs_write(fd, &data, 4 * MAX_NUM_OF_VALUES * sizeof(uint16_t));
            cfs_close(fd);
        }
    }
}

const uint16_t fetch_sensor_data(const char *filename, const uint16_t index)
{
    uint16_t data = 0;
    int file = cfs_open(filename, CFS_READ);
    if (file != -1)
    {
        cfs_seek(file, sizeof(uint16_t) * index, CFS_SEEK_SET); // jump to right position
        cfs_read(file, &data, sizeof(uint16_t));
        cfs_close(file);
    }
    return data;
}

#define TEMP_READ_INTERVAL CLOCK_SECOND * 3
static struct etimer sensor_data_read_timer;

//Prozesse
PROCESS(p_sensors, "");
PROCESS_THREAD(p_sensors, ev, data)
{
    PROCESS_BEGIN();

    // set default thresholds if necessary
    if (get_threshold(LIGHT_LOW) < 0 || get_threshold(LIGHT_HIGH) < 0 || get_threshold(TEMP_LOW) < 0 || get_threshold(TEMP_HIGH) < 0 || get_threshold(HUM_LOW) < 0 || get_threshold(HUM_HIGH) < 0) 
    {
        write_thresholds("0:50000:0:100:0:50000");
    }

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

            //Check Thresholds
            uint16_t l_low = (uint16_t) get_threshold(LIGHT_LOW);
            uint16_t l_high = (uint16_t) get_threshold(LIGHT_HIGH);
            uint16_t t_low = (uint16_t) get_threshold(TEMP_LOW);
            uint16_t t_high = (uint16_t) get_threshold(TEMP_HIGH);
            uint16_t h_low = (uint16_t) get_threshold(HUM_LOW);
            uint16_t h_high = (uint16_t)get_threshold(HUM_HIGH);

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

void on_button_pressed(void)
{
    if (button_sensor.value(0) == 0) // pressed
    {
        //init_network(); // for testing
        //init_rreq_reply(find_best_route());
        add_device_to_network();
    }

    /*
    else if (button_sensor.value(0) == 8) // released
    {
        //get_threshold()
        //leds_off(LEDS_ALL);
    }*/
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
    int f_num_sensor_data = cfs_open(FILE_SENSOR_DATA_LENGTH, CFS_WRITE);
    if (f_num_sensor_data != -1)
    {
        cfs_write(f_num_sensor_data, &num_sensor_data, sizeof(uint16_t));
        cfs_close(f_num_sensor_data);
    }
}

void clear_sensor_data()
{
    set_num_sensor_data(0);
    cfs_remove(FILE_SENSOR_DATA);
}

void print_sensor_data()
{
    printf("Thresholds: \r\n");
    printf("          | -%7li | -%7li | -%7li \r\n", get_threshold(TEMP_LOW), get_threshold(HUM_LOW), get_threshold(LIGHT_LOW));
    printf("          | +%7li | +%7li | +%7li \r\n", get_threshold(TEMP_HIGH), get_threshold(HUM_HIGH), get_threshold(LIGHT_HIGH));
    printf("----------+----------+----------+-----------\r\n");
    printf("Timestamp |  Temp    | Humidity | Light     \r\n");
    printf("----------+----------+----------+-----------\r\n");

    for (uint16_t i = 0; i < get_num_sensor_data(); ++i)
    {
        printf(" %8u | %8u | %8u | %8u\r\n",
               fetch_sensor_data(FILE_SENSOR_DATA, i * 4),      // time
               fetch_sensor_data(FILE_SENSOR_DATA, i * 4 + 1),  // temperature
               fetch_sensor_data(FILE_SENSOR_DATA, i * 4 + 2),  // humidity
               fetch_sensor_data(FILE_SENSOR_DATA, i * 4 + 3)); // light
    }
}

void write_thresholds(const char *str)
{
    int32_t data[6];
    char* tmp = strtok(str, ":");
    for (uint8_t i = 0; i < 6; i++)
    {
        data[i] = atoi(tmp);
        tmp = strtok(NULL, ":");
    }

    int32_t th[6] = {-1};
    int f_th = cfs_open(FILE_THRESHOLD, CFS_READ);
    if (f_th != -1)
    {
        cfs_seek(f_th, 0, CFS_SEEK_SET); // jump to first position
        cfs_read(f_th, th, sizeof(int32_t)*6);
        cfs_close(f_th);
    }

    cfs_remove(FILE_THRESHOLD);

    if (data[TEMP_LOW] >= 0) { th[TEMP_LOW] = data[TEMP_LOW]; }
    if (data[TEMP_HIGH] >= 0) { th[TEMP_HIGH] = data[TEMP_HIGH]; }
    if (data[HUM_LOW] >= 0) { th[HUM_LOW] = data[HUM_LOW]; }
    if (data[HUM_HIGH] >= 0) { th[HUM_HIGH] = data[HUM_HIGH]; }
    if (data[LIGHT_LOW] >= 0) { th[LIGHT_LOW] = data[LIGHT_LOW]; }
    if (data[LIGHT_HIGH] >= 0) { th[LIGHT_HIGH] = data[LIGHT_HIGH]; }

    f_th = cfs_open(FILE_THRESHOLD, CFS_WRITE);
    if (f_th != -1)
    {
        cfs_write(f_th, th, sizeof(int32_t)*6);
        cfs_close(f_th);
    }
}

const int32_t get_threshold(enum thresh id)
{
    int32_t threshold = -1;
    int f_th = cfs_open(FILE_THRESHOLD, CFS_READ);
    if (f_th != -1)
    {
        cfs_seek(f_th, id * sizeof(int32_t), CFS_SEEK_SET);
        cfs_read(f_th, &threshold, sizeof(int32_t));
        cfs_close(f_th);
    }

    return threshold;
}
