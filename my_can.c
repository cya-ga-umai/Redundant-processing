/******************************************************************************
 *  ファイル名： my_can.c
 * 　 作成日時： 2015/11/09
 * 　　　 機能： CAN通信
 ******************************************************************************/

/******************************************************************************
 * ヘッダファイル
 ******************************************************************************/
#include <xc.h>
#include "mycan.h"

/******************************************************************************
 * グローバル変数
 ******************************************************************************/
static unsigned char CAN1RX0Flag = 0;
static unsigned char CAN1RX1Flag = 0;

/******************************************************************************
 * 関数プロトタイプ
 ******************************************************************************/
static void CAN1_send_buffer_8(unsigned char buffer_no, unsigned char *data, unsigned char datalen);
static void CAN1_send_buffer_16(unsigned char buffer_no, unsigned int *data, unsigned char datalen);
static void CAN1_send_buffer_32(unsigned char buffer_no, unsigned long *data, unsigned char datalen);
static void CAN1_send_id(unsigned char buffer_no, unsigned char id_type, unsigned char msg_type, 
        unsigned char priority, unsigned int sid, unsigned long eid, unsigned char datalen);

static void CAN1_read_buffer_8(unsigned char buffer_no, unsigned char *data);
static void CAN1_read_buffer_16(unsigned char buffer_no, unsigned int *data);
static void CAN1_read_buffer_32(unsigned char buffer_no, unsigned long *data);
static void CAN1_read_flag(unsigned char buffer_no);
/******************************************************************************
 * CAN1モジュール初期化関数
 ******************************************************************************/
void CAN1_init(void) {
    CAN1_set_OPmode(CAN_REQ_OPERMODE_CONFIG);
    C1CTRLbits.CANCKS = 1;          /* Fcan = Fcy = 29.48[MHz]に設定 */

    C1CFG1bits.SJW = 00;            /* ジャンプ幅は1 x TQに設定 */

    C1CFG1bits.BRP = BRP_VAL;       /* ((FCY/(2*NTQ*BITRATE))-1) */

    C1CFG2 = 0x03F5;                // SEG1PH=6Tq, SEG2PH=3Tq, PRSEG=5Tq 
                                    // Sample 3 times
                                    // Each bit time is 15Tq

    C1INTF = 0;                     /* CAN割り込みフラグレジスタをクリア */
    IFS1bits.C1IF = 0;              /* 入力キャプチャ1 割り込みフラグをクリア */
    C1INTE = 0x00FF;                /* CAN1全割り込み有効 */
    IEC1bits.C1IE = 1;              /* CAN割り込み許可 */
    C1RX0CON = C1RX1CON = 0x0000;   /* 受信バッファレジスタを初期化 */
}

/******************************************************************************
 * CAN1モジュール動作モード設定関数
 ******************************************************************************/
void CAN1_set_OPmode(unsigned char mode){
    C1CTRLbits.REQOP = mode;
    while (C1CTRLbits.OPMODE != mode);
}

/******************************************************************************
 * CAN1モジュールマスク設定関数
 ******************************************************************************/
void CAN1_set_mask(unsigned char mask_no, unsigned int sid, unsigned long eid, unsigned char mide){
    switch (mask_no){
        default: /* mask_no = 0 */
            C1RXM0SIDbits.SID       = sid;
            C1RXM0SIDbits.MIDE      = mide;         /* 一致したメッセージの処遇 */
            C1RXM0EIDH              = eid >> 6;     /* EIDの上位ビット */
            C1RXM0EIDLbits.EID5_0   = eid & 0x3F;   /* EIDの下位ビット */
            break;
        case 1:
            C1RXM1SIDbits.SID       = sid;
            C1RXM1SIDbits.MIDE      = mide;
            C1RXM1EIDH              = eid >> 6;
            C1RXM1EIDLbits.EID5_0   = eid & 0x3F;
            break;
    }
}

/******************************************************************************
 * CAN1モジュールフィルタ設定関数
 ******************************************************************************/
void CAN1_set_filter(unsigned char filter_no, unsigned char id_type, unsigned int sid, unsigned long eid){
    switch (filter_no){
        default:/* fillter_no = 0 */
            C1RXF0SIDbits.EXIDE     = id_type;      /* EIDフィルタの設定 */
            C1RXF0SIDbits.SID       = sid;
            C1RXF0EIDH              = eid >> 6;     /* EID上位ビット */
            C1RXF0EIDLbits.EID5_0   = eid & 0x3F;   /* EID下位ビット */
            break;
        case 1:
            C1RXF1SIDbits.EXIDE     = id_type;
            C1RXF1SIDbits.SID       = sid;
            C1RXF1EIDH              = eid >> 6;
            C1RXF1EIDLbits.EID5_0   = eid & 0x3F;
            break;
        case 2:
            C1RXF2SIDbits.EXIDE     = id_type;
            C1RXF2SIDbits.SID       = sid;
            C1RXF2EIDH              = eid >> 6;
            C1RXF2EIDLbits.EID5_0   = eid & 0x3F;
            break;
        case 3:
            C1RXF3SIDbits.EXIDE     = id_type;
            C1RXF3SIDbits.SID       = sid;
            C1RXF3EIDH              = eid >> 6;
            C1RXF3EIDLbits.EID5_0   = eid & 0x3F;
            break; 
        case 4:
            C1RXF4SIDbits.EXIDE     = id_type;
            C1RXF4SIDbits.SID       = sid;
            C1RXF4EIDH              = eid >> 6;
            C1RXF4EIDLbits.EID5_0   = eid & 0x3F;
            break;
        case 5:
            C1RXF5SIDbits.EXIDE     = id_type;
            C1RXF5SIDbits.SID       = sid;
            C1RXF5EIDH              = eid >> 6;
            C1RXF5EIDLbits.EID5_0   = eid & 0x3F;
            break;
    }
}

/******************************************************************************
 * CAN1モジュールメッセージ送信関数
 ******************************************************************************/
void CAN1_send_message_8(unsigned char buffer_no, unsigned char id_type, unsigned char msg_type, 
        unsigned char priority, unsigned int sid, unsigned long eid, unsigned char *data, unsigned char datalen){
    CAN1_send_buffer_8(buffer_no, data, datalen);
    CAN1_send_id(buffer_no, id_type, msg_type, priority, sid, eid, datalen);
}

void CAN1_send_message_16(unsigned char buffer_no, unsigned char id_type, unsigned char msg_type, 
        unsigned char priority, unsigned int sid, unsigned long eid, unsigned int *data, unsigned char datalen){
    CAN1_send_buffer_16(buffer_no, data, datalen);
    CAN1_send_id(buffer_no, id_type, msg_type, priority, sid, eid, datalen);
}

void CAN1_send_message_32(unsigned char buffer_no, unsigned char id_type, unsigned char msg_type, 
        unsigned char priority, unsigned int sid, unsigned long eid, unsigned long *data, unsigned char datalen){
    CAN1_send_buffer_32(buffer_no, data, datalen);
    CAN1_send_id(buffer_no, id_type, msg_type, priority, sid, eid, datalen);
}

/******************************************************************************
 * CAN1モジュール送信バッファ書き込み関数（スタティック）
 ******************************************************************************/
static void CAN1_send_buffer_8(unsigned char buffer_no, unsigned char *data, unsigned char datalen){
    switch (buffer_no){
        default:
            C1TX0B1 = C1TX0B2 = C1TX0B3 = C1TX0B4 = 0x0000; /* 送信バッファ初期化 */
            switch (datalen){
                case 8:
                    C1TX0B4 = data[7];
                    C1TX0B4 <<= 8;
                case 7:
                    C1TX0B4 |= data[6];
                case 6:
                    C1TX0B3 = data[5];
                    C1TX0B3 <<= 8;
                case 5:
                    C1TX0B3 |= data[4];
                case 4:
                    C1TX0B2 = data[3];
                    C1TX0B2 <<= 8;
                case 3:
                    C1TX0B2 |= data[2];
                case 2:
                    C1TX0B1 = data[1];
                    C1TX0B1 <<= 8;
                default:
                    C1TX0B1 |= data[0];
                    break;
            }
            break;
        case 1:
            C1TX1B1 = C1TX1B2 = C1TX1B3 = C1TX1B4 = 0x0000;
            switch (datalen){
                case 8:
                    C1TX1B4 = data[7];
                    C1TX1B4 <<= 8;
                case 7:
                    C1TX1B4 |= data[6];
                case 6:
                    C1TX1B3 = data[5];
                    C1TX1B3 <<= 8;
                case 5:
                    C1TX1B3 |= data[4];
                case 4:
                    C1TX1B2 = data[3];
                    C1TX1B2 <<= 8;
                case 3:
                    C1TX1B2 |= data[2];
                case 2:
                    C1TX1B1 = data[1];
                    C1TX1B1 <<= 8;
                default:
                    C1TX1B1 |= data[0];
                    break;
            }
            break;
        case 2:
            C1TX2B1 = C1TX2B2 = C1TX2B3 = C1TX2B4 = 0x0000;
            switch (datalen){
                case 8:
                    C1TX2B4 = data[7];
                    C1TX2B4 <<= 8;
                case 7:
                    C1TX2B4 |= data[6];
                case 6:
                    C1TX2B3 = data[5];
                    C1TX2B3 <<= 8;
                case 5:
                    C1TX2B3 |= data[4];
                case 4:
                    C1TX2B2 = data[3];
                    C1TX2B2 <<= 8;
                case 3:
                    C1TX2B2 |= data[2];
                case 2:
                    C1TX2B1 = data[1];
                    C1TX2B1 <<= 8;
                default:
                    C1TX2B1 |= data[0];
                    break;
            }
            break;
    }
}

static void CAN1_send_buffer_16(unsigned char buffer_no, unsigned int *data, unsigned char datalen){
    switch (buffer_no){
        default:
            C1TX0B1 = C1TX0B2 = C1TX0B3 = C1TX0B4 = 0x0000;
            switch (datalen){
                case 8: case 7:
                    C1TX0B4 = data[3];
                case 6: case 5:
                    C1TX0B3 = data[2];
                case 4: case 3:
                    C1TX0B2 = data[1];
                default:
                    C1TX0B1 = data[0];
                    break;
            }
            break;
        case 1:
            C1TX1B1 = C1TX1B2 = C1TX1B3 = C1TX1B4 = 0x0000;
            switch (datalen){
                case 8: case 7:
                    C1TX1B4 = data[3];
                case 6: case 5:
                    C1TX1B3 = data[2];
                case 4: case 3:
                    C1TX1B2 = data[1];
                default:
                    C1TX1B1 = data[0];
                    break;
            }
            break;
        case 2:
            C1TX2B1 = C1TX2B2 = C1TX2B3 = C1TX2B4 = 0x0000;
            switch (datalen){
                case 8: case 7:
                    C1TX2B4 = data[3];
                case 6: case 5:
                    C1TX2B3 = data[2];
                case 4: case 3:
                    C1TX2B2 = data[1];
                default:
                    C1TX2B1 = data[0];
                    break;
            }
            break;
    }
}

static void CAN1_send_buffer_32(unsigned char buffer_no, unsigned long *data, unsigned char datalen){
    switch (buffer_no){
        default:
            C1TX0B1 = C1TX0B2 = C1TX0B3 = C1TX0B4 = 0x0000;
            switch (datalen){
                case 8: case 7:
                    C1TX0B4 = (unsigned int)((data[1] & 0xFFFF0000) >> 16);
                case 6: case 5:
                    C1TX0B3 = (unsigned int)(data[1] & 0xFFFF);
                case 4: case 3:
                    C1TX0B2 = (unsigned int)((data[0] & 0xFFFF0000) >> 16);
                default:
                    C1TX0B1 = (unsigned int)(data[0] & 0xFFFF);
                    break;
            }
            break;
        case 1:
            C1TX1B1 = C1TX1B2 = C1TX1B3 = C1TX1B4 = 0x0000;
            switch (datalen){
                case 8: case 7:
                    C1TX1B4 = (unsigned int)((data[1] & 0xFFFF0000) >> 16);
                case 6: case 5:
                    C1TX1B3 = (unsigned int)(data[1] & 0xFFFF);
                case 4: case 3:
                    C1TX1B2 = (unsigned int)((data[0] & 0xFFFF0000) >> 16);
                default:
                    C1TX1B1 = (unsigned int)(data[0] & 0xFFFF);
                    break;
            }
            break;
        case 2:
            C1TX2B1 = C1TX2B2 = C1TX2B3 = C1TX2B4 = 0x0000;
            switch (datalen){
                case 8: case 7:
                    C1TX2B4 = (unsigned int)((data[1] & 0xFFFF0000) >> 16);
                case 6: case 5:
                    C1TX2B3 = (unsigned int)(data[1] & 0xFFFF);
                case 4: case 3:
                    C1TX2B2 = (unsigned int)((data[0] & 0xFFFF0000) >> 16);
                default:
                    C1TX2B1 = (unsigned int)(data[0] & 0xFFFF);
                    break;
            }
            break;
    }
}

/******************************************************************************
 * CAN1モジュール送信バッファ設定関数（スタティック）
 ******************************************************************************/
static void CAN1_send_id(unsigned char buffer_no, unsigned char id_type, unsigned char msg_type, 
        unsigned char priority, unsigned int sid, unsigned long eid, unsigned char datalen){
    switch (buffer_no){
        default:
            C1TX0CONbits.TXPRI              = priority;     /* メッセージの優先順位 */
            C1TX0SIDbits.TXIDE              = id_type;      /* EID使用か */
            
            C1TX0SIDbits.SRR                = msg_type;     /* 標準かリモートか */
            if (id_type) C1TX0DLCbits.TXRTR = msg_type;     /* EIDが有効ならばTXRTRも有効 */
            
            C1TX0SIDbits.SID10_6            = sid >> 6;
            C1TX0SIDbits.SID5_0             = sid & 0x3F;
            
            C1TX0EIDbits.EID17_14           = eid >> 14;
            C1TX0EIDbits.EID13_6            = (eid >> 6) & 0xFF;
            C1TX0DLCbits.EID5_0             = eid & 0x3F;
            
            C1TX0DLCbits.DLC                = datalen;      /* データバイト数 */   
            C1TX0DLCbits.TXRB0 = C1TX0DLCbits.TXRB1 = 1;    /* プロトコル上1にセットする必要あり */
            
            while(1){
                C1TX0CONbits.TXREQ          = 1;           /* 送信要求 */
                while(C1TX0CONbits.TXREQ);                  /* 送信処理終了まで待つ */
                if(!C1TX0CONbits.TXABT) break;              /* 送信成功したら終了 */
            }
            
            break;
        case 1:
            C1TX1CONbits.TXPRI              = priority;
            C1TX1SIDbits.TXIDE              = id_type;
            
            C1TX1SIDbits.SRR                = msg_type;
            if (id_type) C1TX1DLCbits.TXRTR = msg_type;
            
            C1TX1SIDbits.SID10_6            = sid >> 6;
            C1TX1SIDbits.SID5_0             = sid & 0x3F;
            
            C1TX1EIDbits.EID17_14           = eid >> 14;
            C1TX1EIDbits.EID13_6            = (eid >> 6) & 0xFF;
            C1TX1DLCbits.EID5_0             = eid & 0x3F;
            
            C1TX1DLCbits.DLC                = datalen;
            C1TX1DLCbits.TXRB0 = C1TX1DLCbits.TXRB1 = 1;
            
            while(1){
                C1TX1CONbits.TXREQ          = 1;
                while(C1TX1CONbits.TXREQ);
                if(!C1TX1CONbits.TXABT) break;
            }
            
            break;
        case 2:
            C1TX2CONbits.TXPRI              = priority;
            C1TX2SIDbits.TXIDE              = id_type;
            
            C1TX2SIDbits.SRR                = msg_type;
            if (id_type) C1TX2DLCbits.TXRTR = msg_type;
            
            C1TX2SIDbits.SID10_6            = sid >> 6;
            C1TX2SIDbits.SID5_0             = sid & 0x3F;
            
            C1TX2EIDbits.EID17_14           = eid >> 14;
            C1TX2EIDbits.EID13_6            = (eid >> 6) & 0xFF;
            C1TX2DLCbits.EID5_0             = eid & 0x3F;
            
            C1TX2DLCbits.DLC                = datalen;
            C1TX2DLCbits.TXRB0 = C1TX2DLCbits.TXRB1 = 1;
            
            while(1){
                C1TX2CONbits.TXREQ          = 1;
                while(C1TX2CONbits.TXREQ);
                if(!C1TX2CONbits.TXABT) break;
            }
            
            break;
    }
}

/******************************************************************************
 * CAN1モジュールメッセージ受信関数
 ******************************************************************************/
void CAN1_read_message_8(unsigned char buffer_no, unsigned char *data){
    while(!(CAN1RX0Flag || CAN1RX1Flag));
    CAN1_read_buffer_8(buffer_no, data);
    CAN1_read_flag(buffer_no);
}

void CAN1_read_message_16(unsigned char buffer_no, unsigned int *data){
    while(!(CAN1RX0Flag || CAN1RX1Flag));
    CAN1_read_buffer_16(buffer_no, data);
    CAN1_read_flag(buffer_no);
}

void CAN1_read_message_32(unsigned char buffer_no, unsigned long *data){
    while(!(CAN1RX0Flag || CAN1RX1Flag));
    CAN1_read_buffer_32(buffer_no, data);
    CAN1_read_flag(buffer_no);
}

/******************************************************************************
 * CAN1モジュール受信バッファ読み出し関数（スタティック）
 ******************************************************************************/
static void CAN1_read_buffer_8(unsigned char buffer_no, unsigned char *data){
    switch (buffer_no){
        default:
            switch (C1RX0DLCbits.DLC){
                case 8:
                    data[7] = (C1RX0B4 >> 8) & 0xFF;
                case 7:
                    data[6] = C1RX0B4 & 0xFF;
                case 6:
                    data[5] = (C1RX0B3 >> 8) & 0xFF;
                case 5:
                    data[4] = C1RX0B3 & 0xFF;
                case 4:
                    data[3] = (C1RX0B2 >> 8) & 0xFF;
                case 3:
                    data[2] = C1RX0B2 & 0xFF;
                case 2:
                    data[1] = (C1RX0B1 >> 8) & 0xFF;
                default:
                    data[0] = C1RX0B1 & 0xFF;
                    break;
            }
            break;
        case 1:
            switch (C1RX1DLCbits.DLC){
                case 8:
                    data[7] = (C1RX1B4 >> 8) & 0xFF;
                case 7:
                    data[6] = C1RX1B4 & 0xFF;
                case 6:
                    data[5] = (C1RX1B3 >> 8) & 0xFF;
                case 5:
                    data[4] = C1RX1B3 & 0xFF;
                case 4:
                    data[3] = (C1RX1B2 >> 8) & 0xFF;
                case 3:
                    data[2] = C1RX1B2 & 0xFF;
                case 2:
                    data[1] = (C1RX1B1 >> 8) & 0xFF;
                default:
                    data[0] = C1RX1B1 & 0xFF;
                    break;
            }
            break;
    }
}

static void CAN1_read_buffer_16(unsigned char buffer_no, unsigned int *data){
    switch (buffer_no){
        default:
            switch (C1RX0DLCbits.DLC){
                case 8: case 7:
                    data[3] = C1RX0B4;
                case 6: case 5:
                    data[2] = C1RX0B3;
                case 4: case 3:
                    data[1] = C1RX0B2;
                default:
                    data[0] = C1RX0B1;
                    break;
            }
            break;
        case 1:
            switch (C1RX1DLCbits.DLC){
                case 8: case 7:
                    data[3] = C1RX1B4;
                case 6: case 5:
                    data[2] = C1RX1B3;
                case 4: case 3:
                    data[1] = C1RX1B2;
                default:
                    data[0] = C1RX1B1;
                    break;
            }
            break;
    }
}

static void CAN1_read_buffer_32(unsigned char buffer_no, unsigned long *data){
    switch (buffer_no){
        default:
            data[0] = data[1] = 0x00000000;
            switch (C1RX0DLCbits.DLC){
                case 8: case 7:
                    data[1] = C1RX0B4;
                    data[1] <<= 16;
                case 6: case 5:
                    data[1] |= C1RX0B3;
                case 4: case 3:
                    data[0] = C1RX0B2;
                    data[0] <<= 16;
                default:
                    data[0] |= C1RX0B1;
                    break;
            }
            break;
        case 1:
            data[0] = data[1] = 0x00000000;
            switch (C1RX1DLCbits.DLC){
                case 8: case 7:
                    data[1] = C1RX1B4;
                    data[1] <<= 16;
                case 6: case 5:
                    data[1] |= C1RX1B3;
                case 4: case 3:
                    data[0] = C1RX1B2;
                    data[0] <<= 16;
                default:
                    data[0] |= C1RX1B1;
                    break;
            }
            break;
    }
}

/******************************************************************************
 * CAN1モジュール受信バッファフラグ設定関数（スタティック）
 ******************************************************************************/
static void CAN1_read_flag(unsigned char buffer_no){
    switch (buffer_no){
        default:
            C1INTFbits.RX0IF    = 0;
            C1RX0CONbits.RXFUL  = 0;
            CAN1RX0Flag         = 0;
            break;
        case 1:
            C1INTFbits.RX1IF    = 0;
            C1RX1CONbits.RXFUL  = 0;
            CAN1RX1Flag         = 0;
            break;
    }
}
/******************************************************************************
 * CAN1モジュール割り込み処理関数
 ******************************************************************************/
void __attribute__((interrupt, no_auto_psv)) _C1Interrupt(void) {
    /* 送信バッファ0割り込み */
    if (C1INTFbits.TX0IF) C1INTFbits.TX0IF = 0;
    
    /* 送信バッファ1割り込み */
    if (C1INTFbits.TX1IF) C1INTFbits.TX1IF = 0;
    
    /* 送信バッファ2割り込み */
    if (C1INTFbits.TX2IF) C1INTFbits.TX2IF = 0;
    
    /* 受信バッファ0割り込み */
    if (C1INTFbits.RX0IF) {
        CAN1RX0Flag = 1;
    }
    
    /* 受信バッファ1割り込み */
    if (C1INTFbits.RX1IF) {
        CAN1RX1Flag = 1;
    }
    
    /* バスウェイクアップ動作割り込み */
    if (C1INTFbits.WAKIF) C1INTFbits.WAKIF = 0; // Add wake-up handler code
    
    /* エラー割り込み */
    if (C1INTFbits.ERRIF) C1INTFbits.ERRIF = 0; // Add error handler code
    
    /* 無効メッセージ受信割り込み */
    if (C1INTFbits.IVRIF) C1INTFbits.IVRIF = 0;
    
    /* CAN1割り込みフラグを解除 */
    //if ((C1INTF & C1INTE) == 0) IFS1bits.C1IF = 0;
    IFS1bits.C1IF = 0;
}
