/********************************** (C) COPYRIGHT *******************************
* File Name          : Main.c
* Author             : WCH
* Version            : V1.0
* Date               : 2020/07/31
* Description 		 : CH565/569 ǧ����̫����·���շ��ӿڣ�ʹ����̫��ʱ����Ƶ���õ���60MHz
*******************************************************************************/

/*-----------------------------------ͷ�ļ�����--------------------------------------*/
#include "CH56x_common.h"
#include "ethernet_driver.h"
#include "ethernet_config.h"
#include "timer.h"

#ifdef USE_ETH_PHY_INTERRUPT
#include "PHY_interrupt.h"
#endif

/*-------------------------------ȫ�ֱ���--------------------------------------*/

/* MAC�շ������ڴ����� */
__attribute__ ((aligned(16)))
__attribute__((section(".dmadata")))
UINT8 MAC_QUEUE[TxDscrOffset*TX_Des_Num+                     /* ������������Ҫ�Ŀռ� */
				RxDscrOffset*RX_Des_Num+                     /* ������������Ҫ�Ŀռ� */
				TX_Des_Num*TX_Buf_Size+                      /* ���ͻ�������Ҫ�Ŀռ� */
				RX_Des_Num*RX_Buf_Size];                     /* ���ջ�������Ҫ�Ŀռ� */

ETH_DMADESCTypeDef  *DMATxDescToSet;                        /* ��ǰ����������ָ�� */
ETH_DMADESCTypeDef  *DMARxDescToGet;                        /* ��ǰ����������ָ�� */

extern Globe_RxDes_status_t Globe_RxDes_status;
UINT8 local_mac[6]={0x84,0xc2,0xe4,0x01,0x02,0x03,};
volatile UINT8 enable_send;
volatile UINT8 rece_timeout_cnt;    /* ���ճ�ʱ��ʱ�� */
UINT32 TIMEOUT_CNT=0;               /* ���ճ�ʱ�ƴ��� */


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

#if 1
UINT8 ARP_package[]=
{
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff,/* Ŀ��MAC */
	0x84, 0xc2, 0xe4, 0x01, 0x02, 0x03,/* ԴMAC */
	0x08,0x06,/* ARP�� */
	0x00,0x01,/* Ӳ������ */
	0x08,0x00,/* Э������ */
	0x06,0x04,/* Ӳ����ַ���Ⱥ�Э���ַ���� */
	0x00,0x01,/* ����� */
	0x84, 0xc2, 0xe4, 0x01, 0x02, 0x03,/* ԴMAC��ַ */
	0xc0,0xa8,0x1,0x0f,/* ԴЭ���ַ */
	0x00,0x00,0x00,0x00,0x00,0x00,/* Ŀ��MAC��ַ */
	0xc0,0xa8,0x1,0x0a,/* ԴЭ���ַ */
#if 1 /*padding 16bytes*����   */
	0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,
	0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,
	0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,
	0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,
	0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,
	0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,
	0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,
	0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,
	0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,
	0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,
	0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,
	0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,
	0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,
	0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,
	0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,
	0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,
	0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,
	0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,
	0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,
	0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,
	0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,
	0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,
	0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,
	0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,
	0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,
	0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,
	0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,
	0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55
#endif
};
#endif

#define  target_msg_ptr ARP_package /* ������Է��͵����� */

/*******************************************************************************
* Function Name  : main
* Description    : Main program.
* Input          : None
* Return         : None
*******************************************************************************/
int main()
{  
	UINT8 rc,i;/* ״̬ */
	UINT8 *read_ptr,**p=&read_ptr;
	UINT16 read_length;
	UINT32 DIFF=0;

    /* ϵͳ��ʼ�� */
    SystemInit(FREQ_SYS);
    Delay_Init(FREQ_SYS);

    /* ���ô��ڵ��� */
	DebugInit(921600);
	PRINT("Start @ChipID=%02X\r\n", R8_CHIP_ID );
	PRINT("WCH Gigabits Ethernet MAC driver program demo! \n");
	PRINT("System frequence is :%d Hz,Complair at  %s,%s\n",FREQ_SYS,__DATE__,__TIME__);

	/* MAC��ʼ������Ҫ��֤����/�����Ѿ����� */
	ETH_init();

//	TMR0_init(FREQ_SYS/1000);/* ��ʱ����ʼ��,�������շ��߼��г�ʱ������»ָ����� */

	/* ���÷���ʱ���ӳ٣��ڽ��ղ࣬RGMII��ʱ����Ҫ�������ӳ�1/4���ڡ��û���Ҫ�����Լ���PCB�����֤ */
#ifdef USE_RTL8211FS
	RGMII_TXC_Delay(0,2);/* RGMII�����ӳٺ���λ���� */
#endif
#ifdef USE_RTL8211DN
	RGMII_TXC_Delay(0,3);/* RGMII�����ӳٺ���λ���� */
#endif
	Check_TxDes();/* ������������� */
#ifdef USE_ETH_PHY_INTERRUPT
	phy_int_cheak_init();
#endif
	/* Ԥ������������ͻ���������ʡ����ʱ�� */
	memcpy(pTx_Buff,target_msg_ptr,sizeof(target_msg_ptr));
	memcpy((UINT8*)(pTx_Buff+RX_Buf_Size),target_msg_ptr,sizeof(target_msg_ptr));
	enable_send=1;/* ʹ�ܷ��� */
	printf("enter main loop!\n");

    while(1)
    {
#if 1/* ���� */
		mDelayuS(1);/* ���������ʡȥ����ı��� */
		if(Globe_RxDes_status.pengding_RxDes_cnt)
		{
			read_length=mac_rece(p);
//			if(ETH->MMCRGUFCR%100000==0)
//			{
//				PRINT("T:%d.R:%d.\n",ETH->MMCTGFCR,ETH->MMCRGUFCR);
//				printf("C:%d,+:%d.\n",ETH->MMCRFCECR,ETH->MMCRFCECR-DIFF);
//				DIFF=ETH->MMCRFCECR;
//			}
//			printf("0x%08x��0x%08x\n",&(DMARxDescToGet->Status),DMARxDescToGet->Status);
//			printf("0x%08x��0x%08x\n",&(DMARxDescToGet->ControlBufferSize),DMARxDescToGet->ControlBufferSize);
//			printf("0x%08x��0x%08x\n",&(DMARxDescToGet->Buffer1Addr),DMARxDescToGet->Buffer1Addr);
//			printf("0x%08x��0x%08x\n",&(DMARxDescToGet->Buffer2NextDescAddr),DMARxDescToGet->Buffer2NextDescAddr);
			/* ������read_ptrָ��������У�����Ϊread_length */
			printf("read_length:%d\n",read_length);
			PRINT("rece@0x%08x\n",read_ptr);
			for(i=0;i<6;i++)
				PRINT("%02x ",read_ptr[i]);
				printf("\n");
			for(i=6;i<12;i++)
				PRINT("%02x ",read_ptr[i]);
				printf("\n");
			for(i=12;i<22;i++)
				PRINT("%02x ",read_ptr[i]);
			PRINT("END!\n\n");
		}
#endif

#if 1/* ���� */
		if(enable_send==0x03)
		{
//			memcpy(get_txbuff_addr(),target_msg_ptr,sizeof(target_msg_ptr));
			mac_send(sizeof(target_msg_ptr));
			enable_send=0;
//			printf("main:DMATxDescToSet:0x%08x\n",DMATxDescToSet);
//			printf("main:0x%08x:0x%08x\n",&(DMATxDescToSet->Status),DMATxDescToSet->Status);
//			printf("main:0x%08x:0x%08x\n",&(DMATxDescToSet->ControlBufferSize),DMATxDescToSet->ControlBufferSize);
//			printf("main:0x%08x:0x%08x\n",&(DMATxDescToSet->Buffer1Addr),DMATxDescToSet->Buffer1Addr);
//			printf("main:0x%08x:0x%08x\n",&(DMATxDescToSet->Buffer2NextDescAddr),DMATxDescToSet->Buffer2NextDescAddr);
//			if(ETH->MMCTGFCR%1000==0)
//			{
//				printf("RGUFCR:%d.\n",ETH->MMCRGUFCR);
//				printf("RFCECR:%d.\n",ETH->MMCRFCECR);
//				PRINT("CNT overflow.T:%d.R:%d.\n",ETH->MMCTGFCR,ETH->MMCRGUFCR);
//			}
			printf("MMCTGFCR:%d.\n",ETH->MMCTGFCR);
		}
#endif
    }
}




