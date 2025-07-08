#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_HEAP_TASK_TRACKING
void esp_dump_per_task_heap_info(void);
#endif

#ifdef __cplusplus
}
#endif
