#include "dataset_flash.h"

void flash_erase_dataset_area(void)
{
    HAL_FLASH_Unlock();

    for (uint32_t addr = DATASET_FLASH_ADDR;
         addr < DATASET_FLASH_ADDR + DATASET_FLASH_SIZE;
         addr += 1024)
    {
        FLASH_EraseInitTypeDef erase;
        uint32_t page_error = 0;

        erase.TypeErase = FLASH_TYPEERASE_PAGES;
        erase.PageAddress = addr;
        erase.NbPages = 1;

        HAL_FLASHEx_Erase(&erase, &page_error);
    }

    HAL_FLASH_Lock();
}

void flash_write_page(uint32_t addr, uint8_t *data, uint32_t len)
{
    HAL_FLASH_Unlock();

    for (uint32_t i = 0; i < len; i += 2) {
        uint16_t half = *(uint16_t *)&data[i];
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,
                          addr + i, half);
    }

    HAL_FLASH_Lock();
}
