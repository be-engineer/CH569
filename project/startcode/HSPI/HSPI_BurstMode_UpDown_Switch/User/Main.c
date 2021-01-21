/********************************** (C) COPYRIGHT *******************************
* File Name          : Main.c
* Author             : WCH
* Version            : V1.0
* Date               : 2020/07/31
* Description 		 : 
*******************************************************************************/
#define  FREQ_SYS   120000000

#include "CH56x_common.h"

//Mode
#define Host_MODE    0
#define Slave_MODE   1
/* HSPI Mode Selection */
//#define HSPI_MODE   Host_MODE
#define HSPI_MODE   Slave_MODE

//Data size
#define DataSize_8bit   0
#define DataSize_16bit   1
#define DataSize_32bit   2
/* HSPI Data Size Selection */
//#define Data_Size   DataSize_8bit
//#define Data_Size   DataSize_16bit
#define Data_Size   DataSize_32bit


//DMA_Len
#define DMA_Tx_Len0   512
#define DMA_Tx_Len1   512

//DMA_Addr0
#define DMA_TX_Addr0   0x20020000
#define DMA_RX_Addr0   0x20020000

//DMA_Addr1
#define DMA_TX_Addr1   0x20020000 + DMA_Tx_Len0
#define DMA_RX_Addr1   0x20020000 + DMA_Tx_Len1


volatile UINT8 Tx_End_Flag = 0;  //发送完成标志
volatile UINT8 Rx_End_Flag = 0;  //接收完成标志
volatile UINT8 BURST_End_Flag = 0;  //突发完成标志
volatile UINT8 BURST_cnt = 0;  //突发计数

void HSPI_IRQHandler (void) __attribute__((interrupt("WCH-Interrupt-fast")));


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
* Function Name  : HSPI_GPIO_Init
* Description    :
* Input          : None
* Return         : None
*******************************************************************************/
void HSPI_GPIO_Init(void)
{
	//TX GPIO PA9、11、21 推挽输出
	R32_PA_DIR |= (1<<9) | (1<<11) | (1<<21);

	//clk 16mA
	R32_PA_DRV |= (1<<11);

	//Rx GPIO PA10 推挽输出
	R32_PA_DIR |= (1<<10);
}


/*******************************************************************************
* Function Name  : HSPI_Init
* Description    : HSPI 初始化
* Input          : None
* Return         : None
*******************************************************************************/
void HSPI_Init(void)
{
	//GPIO Cfg
	HSPI_GPIO_Init();

	R8_HSPI_CFG &= ~(RB_HSPI_MODE | RB_HSPI_MSK_SIZE);  //Clear

#if (HSPI_MODE==Host_MODE)
	R8_HSPI_CFG |= RB_HSPI_MODE;   //Most

#elif (HSPI_MODE==Slave_MODE)
	R8_HSPI_CFG &= ~(RB_HSPI_MODE);   //Slave

#endif

	//data size
#if (Data_Size == DataSize_8bit)
	R8_HSPI_CFG |= RB_HSPI_DAT8_MOD;

#elif (Data_Size == DataSize_16bit)
	R8_HSPI_CFG |= RB_HSPI_DAT16_MOD;

#elif (Data_Size == DataSize_32bit)
	R8_HSPI_CFG |= RB_HSPI_DAT32_MOD;

#endif

    //ACk mode  1   (硬件自动应答模式 用于突发模式，不用于正常模式)
	R8_HSPI_CFG |= RB_HSPI_HW_ACK;

#if (HSPI_MODE==Slave_MODE)
    //Rx ToG En  1
	R8_HSPI_CFG |= RB_HSPI_RX_TOG_EN;

#elif (HSPI_MODE==Host_MODE)
    //Tx ToG En  1
	R8_HSPI_CFG |= RB_HSPI_TX_TOG_EN;

#endif

	//TX 采样边沿
	R8_HSPI_AUX |= RB_HSPI_TCK_MOD;  //下降沿采样

	//硬件Auto ack time
	R8_HSPI_AUX |= RB_HSPI_ACK_TX_MOD;

	//延时时间
	R8_HSPI_AUX &= ~RB_HSPI_ACK_CNT_SEL;   //  延迟2T

	//clear ALL_CLR  TRX_RST  (reset)
	R8_HSPI_CTRL &= ~(RB_HSPI_ALL_CLR|RB_HSPI_TRX_RST);

	//Enable Interupt
#if (HSPI_MODE==Host_MODE)
	R8_HSPI_INT_EN |= RB_HSPI_IE_T_DONE;
	R8_HSPI_INT_EN |= RB_HSPI_IE_FIFO_OV;
	R8_HSPI_INT_EN |= RB_HSPI_IE_B_DONE;
    R8_HSPI_INT_EN |= RB_HSPI_IE_R_DONE;

	//config TX 自定义  Header
	R32_HSPI_UDF0 = 0x3ABCDEF;      //UDF0
	R32_HSPI_UDF1 = 0x3ABCDEF;      //UDF1

#elif (HSPI_MODE==Slave_MODE)
	R8_HSPI_INT_EN |= RB_HSPI_IE_R_DONE;
	R8_HSPI_INT_EN |= RB_HSPI_IE_FIFO_OV;
	R8_HSPI_INT_EN |= RB_HSPI_IE_B_DONE;

#endif

	//addr0 DMA TX RX addr
	R32_HSPI_TX_ADDR0 = DMA_TX_Addr0;
	R32_HSPI_RX_ADDR0 = DMA_RX_Addr0;

	//addr1 DMA TX RX addr
	R32_HSPI_TX_ADDR1 = DMA_TX_Addr1;
	R32_HSPI_RX_ADDR1 = DMA_RX_Addr1;

	//addr0 DMA TX addr
	R16_HSPI_DMA_LEN0 = DMA_Tx_Len0 - 1;
	//addr1 DMA TX addr
	R16_HSPI_DMA_LEN1 = DMA_Tx_Len1 - 1;

	R16_HSPI_BURST_CFG = (1<<8)|RB_HSPI_BURST_EN;

	//Enable HSPI  DMA
	R8_HSPI_CTRL |= RB_HSPI_ENABLE | RB_HSPI_DMA_EN;

	PFIC_EnableIRQ(HSPI_IRQn);
}


/*******************************************************************************
* Function Name  : main
* Description    : Main program.
* Input          : None
* Return         : None
*******************************************************************************/
int main()
{  
    UINT32 i,j ;
	UINT8 val;
	UINT8 Rx_Verify_Flag = 0;

    SystemInit(FREQ_SYS);
    Delay_Init(FREQ_SYS);

/* 配置串口调试 */
	DebugInit(921600);
	PRINT("Start @ChipID=%02X\r\n", R8_CHIP_ID );
	PRINT("System Clock=%d\r\n", FREQ_SYS );

#if (HSPI_MODE==Host_MODE)
	PRINT("HSPI Host MODE\r\n");

    mDelaymS(100);
    HSPI_Init();

	//写RAM2
	for(i=0; i<0x2000; i++){   //0x2000*4 = 32K
      *(UINT32*)(0x20020000+i*4) = i;
	}

#elif (HSPI_MODE==Slave_MODE)
	PRINT("HSPI Slave MODE\r\n");

    HSPI_Init();
    mDelaymS(1000);

    //写RAM2
    for(i=0; i<0x2000; i++){   //0x2000*4 = 32K
      *(UINT32*)(0x20020000+i*4) = i;
    }

    R16_HSPI_BURST_CFG = (1<<8)|RB_HSPI_BURST_EN;
    R8_HSPI_INT_FLAG = 0xF;   //发送之前将所有 HSPI中断标志清 0
    R8_HSPI_CTRL |= RB_HSPI_SW_ACT;  //软件，触发发送

#endif

#if (HSPI_MODE==Host_MODE)

    while(1){
        while(Rx_End_Flag == 0);
        Rx_End_Flag = 0;
        PRINT("host Rx suc\r\n");

        R16_HSPI_BURST_CFG = (64<<8)|RB_HSPI_BURST_EN;
        R8_HSPI_INT_FLAG = 0xF;   //发送之前将所有 HSPI中断标志清 0
        R8_HSPI_CTRL |= RB_HSPI_SW_ACT;  //软件，触发发送

        while(BURST_End_Flag == 0);
        BURST_End_Flag = 0;
        PRINT("BURST suc\r\n");

    }

#elif (HSPI_MODE==Slave_MODE)
    while(1){
        while(BURST_End_Flag == 0);
        BURST_End_Flag = 0;
        PRINT("BURST suc\r\n");
        while(Rx_End_Flag == 0);
        Rx_End_Flag = 0;
        PRINT("slave Rx suc\r\n");

        if(BURST_cnt < 10){
            BURST_cnt++;
            R16_HSPI_BURST_CFG = (1<<8)|RB_HSPI_BURST_EN;
            R8_HSPI_INT_FLAG = 0xF;   //发送之前将所有 HSPI中断标志清 0
            R8_HSPI_CTRL |= RB_HSPI_SW_ACT;  //软件，触发发送
        }
        else{
            PRINT("Tx End...\r\n");
            while(1);
        }
    }

#endif


}

/*******************************************************************************
* Function Name  : HSPI_IRQHandler
* Description    :
* Input          : None
* Return         : None
*******************************************************************************/
void HSPI_IRQHandler(void)
{
	static UINT32 Tx_Cnt = 0;
	static UINT32 Rx_Cnt = 0;
	static UINT32 addr_cnt = 0;

#if (HSPI_MODE==Host_MODE)
	if(R8_HSPI_INT_FLAG & RB_HSPI_IF_T_DONE){   //单包发送完成
		R8_HSPI_INT_FLAG = RB_HSPI_IF_T_DONE;  //Clear Interrupt
		Tx_Cnt++;
		addr_cnt++;

		if(Tx_Cnt<64){  // 发 32K
			if(addr_cnt%2){
				R32_HSPI_TX_ADDR0 += 512*2;
			}
			else{ //地址0
				R32_HSPI_TX_ADDR1 += 512*2;
			}
		}
		else{ //发送完成
			Tx_End_Flag = 1;
			addr_cnt = 0;
		}
	}

#endif

	if(R8_HSPI_INT_FLAG & RB_HSPI_IF_R_DONE){  //单包接收完成
		R8_HSPI_INT_FLAG = RB_HSPI_IF_R_DONE;  //Clear Interrupt

		//判断CRC是否正确
        if(R8_HSPI_RTX_STATUS & RB_HSPI_CRC_ERR){  //CRC 校验  err
        	PRINT("CRC err\r\n");

        	//addr0 DMA TX RX addr
        	R32_HSPI_TX_ADDR0 = DMA_TX_Addr0;
        	R32_HSPI_RX_ADDR0 = DMA_RX_Addr0;

        	//addr1 DMA TX RX addr
        	R32_HSPI_TX_ADDR1 = DMA_TX_Addr1;
        	R32_HSPI_RX_ADDR1 = DMA_RX_Addr1;

        	addr_cnt = 0;
        	Tx_Cnt = 0;
        	Rx_Cnt = 0;
        	R8_HSPI_RX_SC = 0;  //接收端出现CRC错误或MIS错误 将  RB_HSPI_RX_NUM 清零
        }

        //接收序列号是否 匹配， (不匹配，修改包序列号)
        if(R8_HSPI_RTX_STATUS & RB_HSPI_NUM_MIS){  //不匹配
        	PRINT("NUM_MIS err\r\n");

        	//addr0 DMA TX RX addr
        	R32_HSPI_TX_ADDR0 = DMA_TX_Addr0;
        	R32_HSPI_RX_ADDR0 = DMA_RX_Addr0;

        	//addr1 DMA TX RX addr
        	R32_HSPI_TX_ADDR1 = DMA_TX_Addr1;
        	R32_HSPI_RX_ADDR1 = DMA_RX_Addr1;

        	addr_cnt = 0;
        	Tx_Cnt = 0;
        	Rx_Cnt = 0;

            if((R8_HSPI_RX_SC & (1<<4)) == (1<<4)){
            	R8_HSPI_RX_SC = (1<<4);
            }
            if((R8_HSPI_TX_SC & (1<<4)) == (1<<4)){
            	R8_HSPI_TX_SC = (1<<4);
            }

        }

        //CRC 正确， 接收序列号 匹配 （数据正确接收）
        if((R8_HSPI_RTX_STATUS & (RB_HSPI_CRC_ERR|RB_HSPI_NUM_MIS))==0){
#if (HSPI_MODE==Slave_MODE)
        	Rx_Cnt++;
        	addr_cnt++;

    		if(Rx_Cnt<64){  // 收 32K
    			if(addr_cnt%2){
    				R32_HSPI_RX_ADDR0 += 512*2;
    			}
    			else{ //地址0
    				R32_HSPI_RX_ADDR1 += 512*2;
    			}
    		}
    		else{ //接收完成
    			Rx_End_Flag = 1;
    			addr_cnt = 0;
    			Rx_Cnt  = 0;
    		}

#elif (HSPI_MODE==Host_MODE)
    		Rx_End_Flag = 1;
#endif


        }
	}

	if(R8_HSPI_INT_FLAG & RB_HSPI_IF_FIFO_OV){   //FIFO OV
		R8_HSPI_INT_FLAG = RB_HSPI_IF_FIFO_OV;  //Clear Interrupt

		PRINT("FIFO OV\r\n");
	}


	if(R8_HSPI_INT_FLAG & RB_HSPI_IF_B_DONE){   //突发模式下，突发序列包发送完成
		R8_HSPI_INT_FLAG = RB_HSPI_IF_B_DONE;  //Clear Interrupt

#if (HSPI_MODE==Slave_MODE)
        Tx_Cnt++;
#endif

		if(Tx_Cnt != (R16_HSPI_BURST_CFG>>8)){  // 判断突发模式是否发送成功
			PRINT("BURST Tx Err\r\n");

        	addr_cnt = 0;
        	Tx_Cnt = 0;
			Rx_Cnt = 0;
          	//addr0 DMA TX RX addr
			R32_HSPI_TX_ADDR0 = DMA_TX_Addr0;
			R32_HSPI_RX_ADDR0 = DMA_RX_Addr0;

			//addr1 DMA TX RX addr
			R32_HSPI_TX_ADDR1 = DMA_TX_Addr1;
			R32_HSPI_RX_ADDR1 = DMA_RX_Addr1;



            if((R8_HSPI_TX_SC & (1<<4)) == (1<<4)){
            	R8_HSPI_TX_SC = (1<<4);
            }
            if((R8_HSPI_RX_SC & (1<<4)) == (1<<4)){
            	R8_HSPI_RX_SC = (1<<4);
            }

            while(1);
#if (HSPI_MODE==Host_MODE)
			R16_HSPI_BURST_CFG = (64<<8)|RB_HSPI_BURST_EN;

#elif (HSPI_MODE==Slave_MODE)
			R16_HSPI_BURST_CFG = (1<<8)|RB_HSPI_BURST_EN;
#endif

			R8_HSPI_INT_FLAG = 0xF;  //发送之前将所有 HSPI中断标志清 0
			R8_HSPI_CTRL |= RB_HSPI_SW_ACT;  //软件，触发发送
		}
		else{
		    BURST_End_Flag = 1;
        	addr_cnt = 0;
        	Tx_Cnt = 0;
			Rx_Cnt = 0;
          	//addr0 DMA TX RX addr
			R32_HSPI_TX_ADDR0 = DMA_TX_Addr0;
			R32_HSPI_RX_ADDR0 = DMA_RX_Addr0;

			//addr1 DMA TX RX addr
			R32_HSPI_TX_ADDR1 = DMA_TX_Addr1;
			R32_HSPI_RX_ADDR1 = DMA_RX_Addr1;

            if((R8_HSPI_TX_SC & (1<<4)) == (1<<4)){
            	R8_HSPI_TX_SC = (1<<4);
            }
            if((R8_HSPI_RX_SC & (1<<4)) == (1<<4)){
            	R8_HSPI_RX_SC = (1<<4);
            }
		}

	}


}


