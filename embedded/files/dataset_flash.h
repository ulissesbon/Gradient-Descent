#ifndef DATASET_FLASH_H
#define DATASET_FLASH_H

#include <stdint.h>

#define DATASET_FLASH_ADDR  0x08004000
#define DATASET_FLASH_SIZE  0x00004000   // 16 KB

void flash_write_page(uint32_t addr, uint8_t *data, uint32_t len);
void flash_erase_dataset_area(void);

#endif
