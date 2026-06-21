#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "definitions.h"

static void uart_print(const char *s)
{
    while (*s != '\0')
    {
        /* Veri register'i (DRE) boşalana kadar bekle */
        while ((SERCOM2_REGS->USART_INT.SERCOM_INTFLAG & SERCOM_USART_INT_INTFLAG_DRE_Msk) == 0U)
        {
        }
        SERCOM2_REGS->USART_INT.SERCOM_DATA = (uint8_t)(*s);
        s++;
    }
    /* Son bayt tamamen gönderilene kadar bekle (TXC) */
    while ((SERCOM2_REGS->USART_INT.SERCOM_INTFLAG & SERCOM_USART_INT_INTFLAG_TXC_Msk) == 0U)
    {
    }
}

int main(void)
{
    SYS_Initialize(NULL);

    uart_print("\r\n=== SAM E54 calisiyor! Merhaba Furkan ===\r\n");

    uint32_t counter = 0;
    char buf[48];

    while (true)
    {
        SYS_Tasks();

        int len = sprintf(buf, "Sayac: %lu\r\n", (unsigned long)counter++);
        uart_print(buf);

        for (volatile uint32_t d = 0; d < 3000000U; d++) { /* basit gecikme */ }
    }

    return EXIT_FAILURE;
}