#ifndef FAKE_FREERTOS_TASK_H
#define FAKE_FREERTOS_TASK_H

// Stands in for FreeRTOS's task.h. A task is never run: the tests call the functions a task
// would call.

int xTaskCreate(void (*task)(void *arg), const char *name, unsigned stack_size, void *arg, int priority, void *handle);

#endif
