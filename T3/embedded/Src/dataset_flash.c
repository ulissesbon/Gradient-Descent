#include "dataset_flash.h"
#include "stm32f0xx_hal.h"
#include "stm32f0xx_hal_flash.h"
#include "stm32f0xx_hal_flash_ex.h"
#include <string.h>

void flash_dataset_erase(void)
{
    HAL_FLASH_Unlock();	// Desbloqueia a Flash

    FLASH_EraseInitTypeDef erase;
    uint32_t page_error;

    erase.TypeErase   = FLASH_TYPEERASE_PAGES;	// Apagar por Páginas
    erase.PageAddress = DATASET_FLASH_BASE;     // Endereço inicial
    erase.NbPages     = 8;   // 8 KB Quantidade de páginas (1 KB)

    HAL_FLASHEx_Erase(&erase, &page_error);	// Executa o comando para apagar

    HAL_FLASH_Lock();	// Bloqueia a Flash novamente
}

void flash_dataset_write_float(uint32_t index, float value)
{
	// Calcula o endereço de destino
    uint32_t address = DATASET_FLASH_BASE + index * 4;

    HAL_FLASH_Unlock();	// Desbloqueia a Flash
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address, *(uint32_t*)&value); // Escreve o dado
    HAL_FLASH_Lock();	// Bloqueia a Flash novamente
}

float flash_dataset_read_float(uint32_t index)
{
    uint32_t address = DATASET_FLASH_BASE + index * 4;	// Calcula o endereço
    uint32_t raw = *(uint32_t*)address;					// Lê o dado bruto (32 bits)
    return *(float*)&raw;								// Reinterpreta os bits como float e retorna
}
