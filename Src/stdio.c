//------------------------------------------------------------------------------
//         Headers
//------------------------------------------------------------------------------

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "stm32f7xx_hal.h"
#include "main.h"

//unsigned int g_PrintLevel = HMI_PRINT_INFO;

extern  UART_HandleTypeDef huart1;
signed int vprintf2(const char *pFormat, va_list ap);

//------------------------------------------------------------------------------
//         Local Definitions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//         Global Variables
//------------------------------------------------------------------------------

// Required for proper compilation.
//struct _reent r = {0, (FILE *) 0, (FILE *) 1, (FILE *) 0};
//struct _reent *_impure_ptr = &r;

//------------------------------------------------------------------------------
//         Local Functions
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/// Outputs a formatted string on the DBGU stream, using a variable number of
/// arguments.
/// \param pFormat  Format string.
//------------------------------------------------------------------------------
signed int print(const char *pFormat, ...)
{
    va_list ap;
    signed int result;

    // Forward call to vprintf
    va_start(ap, pFormat);
    result = vprintf2(pFormat, ap);
    va_end(ap);

    return result;
}

//------------------------------------------------------------------------------
/// Outputs a string on stdout.
/// \param pStr  String to output.
//------------------------------------------------------------------------------
signed int puts(const char *pStr)
{
    return fputs(pStr, stdout);
}

#define DMA_BUF_SIZE 1024
unsigned char g_dma_buf[2][DMA_BUF_SIZE+16];
unsigned int b_pingpong = 0;
unsigned char *dma_TNPR = g_dma_buf[0];     // Transmit Next Pointer Register
int            dma_TNCR = 0;                // Transmit Next Counter Register
int            dma_lock = FALSE;

//------------------------------------------------------------------------------
/// Outputs a formatted string on the given stream. Format arguments are given
/// in a va_list instance.
/// \param pStream  Output stream.
/// \param pFormat  Format string
/// \param ap  Argument list.
//------------------------------------------------------------------------------
signed int vprintf2(const char *pFormat, va_list ap)
{
    UART_HandleTypeDef *huart = &huart1;

    unsigned char *pDst;
    int  size = 0;
    int  length = 0;

    dma_lock = TRUE;
    if(dma_TNCR < DMA_BUF_SIZE)
    {
        pDst = (dma_TNPR + dma_TNCR);
        length = DMA_BUF_SIZE - dma_TNCR;
        // Write formatted string in buffer   
        size = vsnprintf((char*)pDst, length, pFormat, ap);
        if(size <= length)
            {dma_TNCR = dma_TNCR + size;}
    }

    if(huart->gState == HAL_UART_STATE_READY)
    {   
        if(dma_TNCR > DMA_BUF_SIZE)
                while(1);
        HAL_UART_Transmit_DMA(huart, dma_TNPR, dma_TNCR);
        b_pingpong = !b_pingpong;
        dma_TNPR = g_dma_buf[b_pingpong];
        dma_TNCR = 0;  
    }

    dma_lock = FALSE;
    return size;
}


void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{

    if((dma_lock==FALSE)&&(huart == &huart1))
    {
        if(dma_TNCR>0)
        {
            if(dma_TNCR > DMA_BUF_SIZE)
                while(1);
            HAL_UART_Transmit_DMA(huart, dma_TNPR, dma_TNCR);
            b_pingpong = !b_pingpong;
            dma_TNPR = g_dma_buf[b_pingpong];
            dma_TNCR = 0;
        }
    }
}


