/********************************** (C) COPYRIGHT *******************************
* File Name          : Main.c
* Author             : WCH
* Version            : V1.0
* Date               : 2020/07/31
* Description 		 : 
*******************************************************************************/

#include "CH56x_common.h"

#define  FREQ_SYS   80000000

UINT8 spiBuff[]={1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6};
UINT8 spiBuffrev[16];

/*******************************************************************************
* Function Name  : DebugInit
* Description    : Initializes the UART1 peripheral.
* Input          : baudrate: UART1 communication baud rate.
* Return         : None
*******************************************************************************/
void DebugInit(UINT32 baudrate)		
{
	UINT32 x;
	UINT32 t = FREQ_SYS;
	
	x = 10 * t * 2 / 16 / baudrate;
	x = ( x + 5 ) / 10;
	R8_UART1_DIV = 1;
	R16_UART1_DL = x;
	R8_UART1_FCR = RB_FCR_FIFO_TRIG | RB_FCR_TX_FIFO_CLR | RB_FCR_RX_FIFO_CLR | RB_FCR_FIFO_EN;
	R8_UART1_LCR = RB_LCR_WORD_SZ;
	R8_UART1_IER = RB_IER_TXD_EN;
	R32_PA_SMT |= (1<<8) |(1<<7);
	R32_PA_DIR |= (1<<8);
}

/*******************************************************************************
* Function Name  : main
* Description    : Main program.
* Input          : None
* Return         : None
*******************************************************************************/
int main()
{  

	UINT8 i;

	SystemInit(FREQ_SYS);
    Delay_Init(FREQ_SYS);

    /*配置串口调试 */
	DebugInit(115200);
	printf("Start @ChipID=%02X\r\n", R8_CHIP_ID );

	/* 从机模式 */
	printf( "1.spi1 mul slave mode \n");

	SPI1_SlaveInit();
	i = SPI1_SlaveRecvByte();
	printf("receive=%2x\n",i);


	SPI1_SlaveRecv( spiBuffrev, 9 );
    for(i=0;i<9;i++)
    {
	  printf("%2x\n",spiBuffrev[i]);
    }

	while(1);


}





