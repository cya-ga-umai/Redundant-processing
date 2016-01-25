/******************************************************************************
 *  ファイル名： main.c
 * 　 作成日時： 2015/11/21
 * 　　　 機能： 冗長処理システム
 ******************************************************************************/

/******************************************************************************
 * ヘッダファイル
 ******************************************************************************/
#include "mycan.h"
#include "rho.h"
#include <xc.h>
#include <libpic30.h>
#include <reset.h>
#include <string.h>

/******************************************************************************
 * マクロ
 ******************************************************************************/
#define MPU_NUM  1
#define WRONG_NUM 3

/******************************************************************************
 * コンフィギュレーション設定
 ******************************************************************************/
// FOSC
#pragma config FPR = FRC_PLL16          // Primary Oscillator Mode (FRC w/ PLL 16x)
#pragma config FOS = FRC                // Oscillator Source (Internal Fast RC)
#pragma config FCKSMEN = CSW_FSCM_OFF   // Clock Switching and Monitor (Sw Disabled, Mon Disabled)

// FWDT
#pragma config FWPSB = WDTPSB_3         // WDT Prescaler B (1:3)
#pragma config FWPSA = WDTPSA_512       // WDT Prescaler A (1:512)
#pragma config WDT = WDT_ON             // Watchdog Timer (Enabled)

// FBORPOR
#pragma config FPWRT = PWRT_64          // POR Timer Value (64ms)
#pragma config BODENV = BORV27          // Brown Out Voltage (2.7V)
#pragma config BOREN = PBOR_ON          // PBOR Enable (Enabled)
#pragma config LPOL = PWMxL_ACT_HI      // Low-side PWM Output Polarity (Active High)
#pragma config HPOL = PWMxH_ACT_HI      // High-side PWM Output Polarity (Active High)
#pragma config PWMPIN = RST_IOPIN       // PWM Output Pin Reset (Control with PORT/TRIS regs)
#pragma config MCLRE = MCLR_EN          // Master Clear Enable (Enabled)

// FGS
#pragma config GWRP = GWRP_OFF          // General Code Segment Write Protect (Disabled)
#pragma config GCP = CODE_PROT_OFF      // General Segment Code Protection (Disabled)

// FICD
#pragma config ICS = ICS_PGD            // Comm Channel Select (Use PGC/EMUC and PGD/EMUD)

/******************************************************************************
 * 関数プロトタイプ
 ******************************************************************************/
void init(void);
void send_and_receive_data(unsigned long data[][2]);
unsigned char check_data(unsigned long data[][2]);

/*******************************************************************************
 * メイン関数
 ******************************************************************************/
int main(void){
    /* 変数宣言 */
    int i;
    unsigned long dat[3][2];                /* 演算結果データ */
    unsigned char result = 0;               /* 比較結果フラグ */
    unsigned char wdt_flag = isWDTTO();
    
    init();                                 /* 初期化 */
    
    rho_method(25060027, dat[MPU_NUM - 1]);  /* 演算処理 */
    LATDbits.LATD0 = 0;                     /* 計算終了を示すLED */

    do {
        ClrWdt();
        
        /* 実験として異常な値をあえて代入 */
        if((MPU_NUM == WRONG_NUM) && (!(wdt_flag)) ){
            dat[MPU_NUM - 1][0] = 0xFF;
        }
        send_and_receive_data(dat);
        
        result = check_data(dat);
        if(result == MPU_NUM){
            LATDbits.LATD1 = 0;
            while(1);  /* 自分自身が不一致の場合、WDTタイムアウトまでループ */
        }else if(result){
            /* リセットしたMPUが受信できるようになるまで待機 */
            for (i = 0; i < 4; i++){
                ClrWdt();
                __delay_ms(100);
            }
        }
    } while (result);   /* 全一致(0)になるまで送受信からやり直す */
    
    /* 動作完了 */
    while(1){
        ClrWdt();
        LATDbits.LATD0 = 0;
        __delay_ms(50);
        LATDbits.LATD0 = 1;
        __delay_ms(50);
    }
    return 0;
}
void init(void){
    /* IO */
    TRISD = 0x0000;
    LATD = 0xFFFF;
    
    /* CAN */
    CAN1_init();
    CAN1_set_mask(0, 0x7FF, 0x3FFFF, CAN_MATCH_FILTER_TYPE);
    CAN1_set_mask(1, 0x7FF, 0x3FFFF, CAN_MATCH_FILTER_TYPE);
    switch(MPU_NUM){
        case 1:
            CAN1_set_filter(0, CAN_EID_DIS, 1, 0x00);
            CAN1_set_filter(2, CAN_EID_DIS, 2, 0x00);
            break;
        case 2:
            CAN1_set_filter(0, CAN_EID_DIS, 0, 0x00);
            CAN1_set_filter(2, CAN_EID_DIS, 2, 0x00);
            break;
        case 3:
            CAN1_set_filter(0, CAN_EID_DIS, 0, 0x00);
            CAN1_set_filter(2, CAN_EID_DIS, 1, 0x00);
            break;
    }
    CAN1_set_OPmode(CAN_REQ_OPERMODE_NOR);
}

/*******************************************************************************
 * データ送受信関数
 ******************************************************************************/
void send_and_receive_data(unsigned long data[][2]){
    CAN1_set_OPmode(CAN_REQ_OPERMODE_NOR);
    CAN1_send_message_32(0, CAN_EID_DIS, CAN_NOR_TX_REQ, MPU_NUM, MPU_NUM - 1, 0x00, data[MPU_NUM - 1], 8);
    switch(MPU_NUM){
        case 1:
            CAN1_read_message_32(0, data[1]);
            CAN1_read_message_32(1, data[2]);
            break;
        case 2:
            CAN1_read_message_32(0, data[0]);
            CAN1_read_message_32(1, data[2]);
            break;
        case 3:
            CAN1_read_message_32(0, data[0]);
            CAN1_read_message_32(1, data[1]);
            break;
    }
}

/*******************************************************************************
 * データ比較関数
 ******************************************************************************/
unsigned char check_data(unsigned long data[][2]){
    if( !(memcmp(data[0], data[1], sizeof(unsigned long) * 2)) ){
        if( !(memcmp(data[0], data[2], sizeof(unsigned long) * 2)) ){
            return 0;   /* 全一致 */
        } else {
            return 3;   /* MPU No.3が不一致 */
        }
    } else if( !(memcmp(data[1], data[2], sizeof(unsigned long) * 2)) ){
        return 1;       /* MPU No.1が不一致 */
    } else {
        return 2;       /* MPU No.2が不一致 */
    }
}
