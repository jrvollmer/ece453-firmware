#ifndef __TASK_CAR__
#define __TASK_CAR__

extern QueueHandle_t q_car;

// initializes the task
void task_car_init();

void task_car(void *pvParameters);


#endif
