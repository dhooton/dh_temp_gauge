#include <nrf_temp.h>


#define MY_TASK_PRIO        (OS_TASK_PRI_HIGHEST)
#define MY_STACK_SIZE       (64)
struct os_task temp_task;
os_stack_t temp_task_stack[MY_STACK_SIZE];
static struct os_mutex buffer_mutex;
static int16_t temperature_buff[10];
static uint8_t buff_write = 0;
static uint8_t buff_read = 0;

uint8_t advance_index(uint8_t index) {
  index++;
  if (index > 9) {
    index = 0;
  }
  return index;
}

/* Allows one value to be added to the ring buffer, forcing the read pointer to move if necessary */
void put_temp(int16_t temp) {
  os_mutex_pend(&buffer_mutex, OS_TIMEOUT_NEVER);
  temperature_buff[buff_write] = temp;
  buff_write = advance_index(buff_write);
  if (buff_write == buff_read) {
    buff_read = advance_index(buff_read);
  }
  os_mutex_release(&buffer_mutex);
}

/* Read all available temperatures from the buffer */
int read_temperatures(int16_t *buff) {
  int num_read = 0;
  os_mutex_pend(&buffer_mutex, OS_TIMEOUT_NEVER);


  while( buff_read != buff_write) {
    buff[num_read++] = temperature_buff[buff_read];
    buff_read = advance_index(buff_read);
  }
  os_mutex_release(&buffer_mutex);
  return num_read;
}  
  

/* Returns the internal temperature of the nRF52 in degC (2 decimal places, scaled) */
int16_t
get_temp_measurement(void)
{
    int16_t temp;
    /* Start the temperature measurement. */
    NRF_TEMP->TASKS_START = 1;
    while(NRF_TEMP->EVENTS_DATARDY != TEMP_INTENSET_DATARDY_Set) {};
    /* Temp reading is in units of 0.25degC, so divide by 4 to get in units of degC
     * (scale by 100 to avoid representing as decimal). */
    temp = (nrf_temp_read() * 100) / 4.0;

    return temp;
}

/* This is the task function */
void temp_task_func(void *arg) {
  if (OS_OK != os_mutex_init(&buffer_mutex)) {
    //log an error
    return;
  }
  /* The task is a forever loop that does not return */
  while (1) {
    /* Wait one tenth of a  second */
    os_time_delay(OS_TICKS_PER_SEC/10);

    /* Capture a temperature reading */
    put_temp(get_temp_measurement());
  }
}


int temp_task_init(void) {
    return os_task_init(&temp_task, "my_task", temp_task_func, NULL, MY_TASK_PRIO,
                 OS_WAIT_FOREVER, temp_task_stack, MY_STACK_SIZE);
}

