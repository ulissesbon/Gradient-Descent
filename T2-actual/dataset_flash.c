/**
 * @file dataset_flash.c
 * @brief Gerenciamento de Persistência de Dados na Memória Flash (STM32F0).
 *
 * @section desc_sec Apresentação Geral
 * Este módulo fornece uma camada de abstração para realizar operações de leitura e escrita
 * na memória Flash interna do microcontrolador. Ele permite salvar o dataset de treinamento
 * de forma não-volátil, garantindo que os dados persistam mesmo após o desligamento ou
 * reinicialização do sistema.
 *
 * O módulo lida com as especificidades do hardware (HAL), como desbloqueio da memória,
 * apagamento de páginas e conversão de tipos (float <-> uint32_t) para gravação física.
 *
 * @section copyright_sec Permissões de Uso
 * Copyright (c) 2025. Todos os direitos reservados.
 * Código desenvolvido para fins acadêmicos. A redistribuição é permitida mantendo-se os créditos.
 *
 * @section usage_sec Como Usar
 * 1. Defina o endereço base `DATASET_FLASH_BASE` no arquivo de cabeçalho (.h).
 * 2. Antes de gravar um NOVO dataset, chame `flash_dataset_erase()` para limpar a região.
 * 3. Use `flash_dataset_write_float()` num laço para preencher a memória.
 * 4. Use `flash_dataset_read_float()` para recuperar os dados durante o treinamento.
 *
 * @section io_sec Entrada e Saída
 * - **Entrada (Escrita):** Índices sequenciais e valores ponto flutuante.
 * - **Saída (Leitura):** Valores ponto flutuante recuperados da memória física.
 *
 * @author Ulisses Gonçalves Bonfim e Raquel Maciel Coelho de Sousa
 * @date 19 de Novembro de 2025
 * @context Trabalho da disciplina de Sistemas Embarcados
 * @platform ARM Cortex-M0 (STM32 NUCLEO-F030R8) utilizando drivers HAL.
 */

#include "dataset_flash.h"
#include "stm32f0xx_hal.h"
#include "stm32f0xx_hal_flash.h"
#include "stm32f0xx_hal_flash_ex.h"
#include <string.h>

/* ==========================================================================
 * Implementação das Funções de Acesso à Flash
 * ========================================================================== */

/**
 * @brief Apaga a região de memória reservada para o Dataset.
 *
 * A memória Flash não pode ser sobrescrita diretamente (bits só mudam de 1 para 0 na gravação).
 * Portanto, é obrigatório apagar as páginas (setar tudo para 0xFFFFFFFF) antes de gravar novos dados.
 *
 * @note Esta função apaga 8 páginas da Flash a partir de `DATASET_FLASH_BASE`.
 * Em MCUs STM32F0 padrão, cada página tem 1KB ou 2KB. Verifique seu datasheet.
 *
 * @warning Esta operação bloqueia a CPU momentaneamente e interrompe a execução de código
 * se ele estiver rodando na mesma região de flash (o que não deve ocorrer se o linker script estiver correto).
 *
 * @param void Não recebe parâmetros.
 * @return void Não retorna valor.
 */
void flash_dataset_erase(void)
{
    // Desbloqueia o controlador da Flash para permitir modificações
    // O hardware possui uma trava de segurança para evitar corrupção acidental
    HAL_FLASH_Unlock();

    FLASH_EraseInitTypeDef erase;
    uint32_t page_error;

    // Configuração da estrutura de apagamento
    erase.TypeErase   = FLASH_TYPEERASE_PAGES;  // Define que apagaremos por páginas (não em massa)
    erase.PageAddress = DATASET_FLASH_BASE;     // Endereço inicial definido no .h
    erase.NbPages     = 8;                      // Quantidade de páginas a apagar (ex: 8KB se página for 1KB)

    // Chama a função HAL estendida para executar o apagamento
    // Se houver erro, a página problemática é salva em 'page_error'
    HAL_FLASHEx_Erase(&erase, &page_error);

    // Bloqueia a Flash novamente para proteção contra escritas espúrias
    HAL_FLASH_Lock();
}

/**
 * @brief Grava um valor ponto flutuante (float) na memória Flash.
 *
 * Como a memória Flash é endereçada em palavras de 32 bits (Word), fazemos um "cast"
 * do ponteiro do float para uint32_t. Isso copia a representação binária exata (IEEE 754)
 * para a memória, sem tentar converter o valor numérico.
 *
 * @param index Índice lógico do dado (0, 1, 2...). O endereço físico é calculado como (Base + Index * 4).
 * @param value Valor float a ser persistido.
 * @return void Não retorna valor.
 */
void flash_dataset_write_float(uint32_t index, float value)
{
    // Calcula o endereço físico absoluto de destino
    // Cada float ocupa 4 bytes, por isso multiplicamos o índice por 4
    uint32_t address = DATASET_FLASH_BASE + index * 4;

    HAL_FLASH_Unlock(); // Permite escrita

    // HAL_FLASH_Program espera um uint32_t ou uint64_t.
    // *(uint32_t*)&value: Pega o endereço do float, finge que é um endereço de inteiro
    // e lê os bits. Isso preserva o padrão de bits do float.
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address, *(uint32_t*)&value);

    HAL_FLASH_Lock();   // Protege novamente
}

/**
 * @brief Lê um valor ponto flutuante da memória Flash.
 *
 * A leitura da Flash em MCUs ARM Cortex-M é Memory Mapped, ou seja,
 * pode-se ler o endereço de memória diretamente como se fosse uma variável RAM const.
 *
 * @param index Índice lógico do dado a ser recuperado.
 * @return float O valor recuperado da memória.
 */
float flash_dataset_read_float(uint32_t index)
{
    // Calcula onde o dado está
    uint32_t address = DATASET_FLASH_BASE + index * 4;

    // Lê o conteúdo de 32 bits daquele endereço
    // (uint32_t*)address transforma o número do endereço em um ponteiro
    uint32_t raw = *(uint32_t*)address;

    // Reinterpreta os bits brutos de volta para formato float
    return *(float*)&raw;
}