#include <heapHelper.h>
#include <esp_heap_task_info.h>
#include "esp_heap_caps.h"
#include <esp_log.h>

#ifdef CONFIG_HEAP_TASK_TRACKING

#define MAX_TASK_NUM 20                         // Max number of per tasks info that it can store
#define MAX_BLOCK_NUM 20                        // Max number of per block info that it can store

static const char *TAG = "heapHelper";
static size_t s_prepopulated_num = 0;
static heap_task_totals_t s_totals_arr[MAX_TASK_NUM];
static heap_task_block_t s_block_arr[MAX_BLOCK_NUM];

void esp_dump_per_task_heap_info(void)
{
    heap_task_info_params_t heap_info = {0};
    heap_info.caps[0] = MALLOC_CAP_8BIT;        // Gets heap with CAP_8BIT capabilities
    heap_info.mask[0] = MALLOC_CAP_8BIT;
    heap_info.caps[1] = MALLOC_CAP_32BIT;       // Gets heap info with CAP_32BIT capabilities
    heap_info.mask[1] = MALLOC_CAP_32BIT;
    heap_info.tasks = NULL;                     // Passing NULL captures heap info for all tasks
    heap_info.num_tasks = 0;
    heap_info.totals = s_totals_arr;            // Gets task wise allocation details
    heap_info.num_totals = &s_prepopulated_num;
    heap_info.max_totals = MAX_TASK_NUM;        // Maximum length of "s_totals_arr"
    heap_info.blocks = s_block_arr;             // Gets block wise allocation details. For each block, gets owner task, address and size
    heap_info.max_blocks = MAX_BLOCK_NUM;       // Maximum length of "s_block_arr"

    ESP_LOGW(TAG, "Free heap memory -> %d", xPortGetFreeHeapSize());

    heap_caps_get_per_task_info(&heap_info);

    for (int i = 0 ; i < *heap_info.num_totals; i++) {
        ESP_LOGW(TAG, "Task: %18s -> 8-bit capability: %6d 32-bit capability: %6d Stack high water mark: %6d",
                heap_info.totals[i].task ? pcTaskGetName(heap_info.totals[i].task) : "Initials" ,
                heap_info.totals[i].size[0],    // Heap size with CAP_8BIT capabilities
                heap_info.totals[i].size[1],    // Heap size with CAP32_BIT capabilities
                uxTaskGetStackHighWaterMark(heap_info.totals[i].task));
    }
}
#endif

