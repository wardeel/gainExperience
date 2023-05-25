#include "stdio.h"
#include "string.h"
#include "stdlib.h"


#define MAX_RECV_CAP  256
#define MAX_RECV_LEN  64
unsigned char recv_idx = 0;    //缓存计数器
unsigned char recv_pos = 0;         //解码计数器
unsigned char recv_buf[MAX_RECV_CAP] = { 0 }; //大缓存buff
unsigned char send_buf[MAX_RECV_LEN] = { 0 }; //每个消息的

/***************************以下为消息命令ID************************************************/
//send/ack
#define BT_TYPE_SELF_CLEANING_TYPE_REQ      0x1     //自清洁类型信令
#define BT_TYPE_SELF_CLEANING_TYPE_RSP      0x2     //自清洁类型回复
#define BT_TYPE_FILL_CLEAN_WATER_REQ        0x3     //加水信令
#define BT_TYPE_FILL_CLEAN_WATER_RSP        0x4     //加水回复
#define BT_TYPE_WATER_SPRAY_REQ             0x5     //基站喷水信令
#define BT_TYPE_WATER_SPRAY_RSP             0x6     //基站喷水回复
#define BT_TYPE_STOVING_REQ                 0x7     //烘干信令
#define BT_TYPE_STOVING_RSP                 0x8     //烘干回复
#define BT_TYPE_ELECTRODE_REQ               0x9     //充电极片查询信令
#define BT_TYPE_ELECTRODE_RSP               0xa     //充电极片回复
#define BT_TYPE_BLE_RECONECT_REQ            0xb     //开启蓝牙配对信令
#define BT_TYPE_BLE_RECONECT_RSP            0xc     //开启蓝牙配对回复
#define BT_TYPE_BLE_GET_CONN_STATUS_REQ     0xd     //获取蓝牙配对状态信令
#define BT_TYPE_BLE_GET_CONN_STATUS_RSP     0xe     //获取蓝牙配对状态回复
#define BT_TYPE_BASE_UPGRADE_REQ            0xf     //蓝牙升级请求信令(询问升级)
#define BT_TYPE_BASE_UPGRADE_RSP            0x10    //蓝牙升级请求回复
#define BT_TYPE_BASE_CHARGE_CURRENT_REQ     0x11    //基站充电信令
#define BT_TYPE_BASE_CHARGE_CURRENT_RSP     0x12    //基站充电回复
#define BT_TYPE_BASE_WIFI_INFO_REQ          0x13    //基站wifi:ssid/passward/token信令
#define BT_TYPE_BASE_WIFI_INFO_RSP          0x14    //基站wifi信息接受回复
#define BT_TYPE_BASE_HEARTBEAT_REQ          0x15    //心跳包信令(保证链路正常)
#define BT_TYPE_BASE_HEARTBEAT_RSP          0x16    //心跳包回复
#define BT_TYPE_BASE_WIFI_SEC_REQ           0x17    //wifi加密查询
#define BT_TYPE_BASE_WIFI_SEC_RSP           0x18    //wifi加密查询回复
#define BT_TYPE_BASE_START_UPGRADE_REQ      0x19    //通知蓝牙模块开始升级
#define BT_TYPE_BASE_START_UPGRADE_RSP      0x1a    //通知蓝牙模块开始升级回复
#define BT_TYPE_BASE_END_UPGRADE_REQ        0x1b    //通知蓝牙模块结束升级
#define BT_TYPE_BASE_END_UPGRADE_RSP        0x1c    //通知蓝牙模块结束升级回复
#define BT_TYPE_BASE_UPGRADE_PACKAGES_REQ   0x1d    //蓝牙升级包发送
#define BT_TYPE_BASE_UPGRADE_PACKAGES_RSP   0x1e    //蓝牙升级包发送回复

//report
#define BT_TYPE_CLEAN_WATER_STATUS          0x1     //加注清水状态(200:加注完成；201：异常中断)
#define BT_TYPE_POLLUTION_DISCHARGE_STATUS  0x2     //排污状态(200:排污完成；201：异常中断)
#define BT_TYPE_CHANGE_ELECTRODE_STATUS     0x3     //充电极片状态(200:准备就绪)
#define BT_TYPE_CHANGE_SELF_CLEAN_KEY       0x4     //基站自清洁按键消息
#define BT_TYPE_CHANGE_PWR_KEY              0x5     //基站启停按键消息
#define BT_TYPE_CHANGE_IN_OUT_KEY           0x6    //基站进出按键消息
#define BT_TYPE_BLE_CONN_STATUS_RPT         0x7    //蓝牙配对状态
/******************************************************************************************/


#define BT_CMD_STOP                         0xa0    //关闭
#define BT_CMD_START                        0xa1    //开启
#define BT_CMD_QUERY                        0xa2    //查询

//以下为消息类型
#define BT_MSG_SEND                         0x0     //发送
#define BT_MSG_ACK                          0x1     //ACK
#define BT_MSG_REPORT                       0x2     //上传


#define BT_SELF_CLEANING_TYPE_MAIN_ENGINE   0x1     //主机自清洁
#define BT_SELF_CLEANING_TYPE_BASE_STATION  0x2     //基站自清洁


//发送消息头
typedef struct
{
    unsigned char start[2];                         //0xfc 0x00；升级数据：0xfc 0x10
    unsigned char length;                           //包长度
    unsigned char type;                             //0x0：send；0x1：ack；0x2：report
    unsigned char cmd_id;                           //消息命令ID
    unsigned char cmd_param;                        //0xa0：关闭，0xa1：开启，0xa2：查询
}robot_bluetooth_send_msg_head;

typedef struct
{
    char bbc;     //bbc检验
    char end;     //固定包尾0xfe
}robot_bluetooth_send_msg_tail;

typedef struct
{
    robot_bluetooth_send_msg_head *msg_head;
    unsigned char *buffer;
    robot_bluetooth_send_msg_tail *msg_tail;
}robot_bluetooth_msg;

typedef struct
{
    robot_bluetooth_msg btmsg;
    unsigned char buffer[256];
}robot_bluetooth_msg_ctl;

typedef struct
{
    unsigned char self_cleaning_type;
}self_cleaning_msg;

typedef struct
{
    unsigned char detergent;                        //是否添加清洁剂 (0x0:不添加，0x1:添加)
    unsigned char germicide;                        //是否添加杀菌剂 (0x0:不添加，0x1:添加)
    unsigned char Wate_volume;                      //待添加的水量  单位：100ml
    unsigned char total_volume;                     //总水量
}fill_clean_water_msg;

typedef struct
{
    unsigned char time;
}stoving_water_spray_msg;

//清洁检测
typedef enum
{
    ROBOT_CHECK_IDLE,                               //空闲
    ROBOT_CHECK_ABNORMAL,                           //异常
    ROBOT_CHECK_MAIN_ENGINE_SELF_CLEANING,          //告知基站当前自清洁类型:主机自清洁
    ROBOT_CHECK_BASE_STATION_SELF_CLEANING,         //告知基站当前自清洁类型：基站自清洁
    ROBOT_CHECK_OPEN_SEWAGE_PUMP,                   //打开主机污水泵
    ROBOT_CHECK_CLOSE_SEWAGE_PUMP,                  //关闭主机污水泵
    ROBOT_CHECK_OPEN_INJECTION_CLEAN_WATER,         //开始加注清水
    ROBOT_CHECK_CLOSE_INJECTION_CLEAN_WATER,        //停止加注清水
    ROBOT_CHECK_OPEN_BASE_STATION_WATER_SPRAY,      //开启基站喷水
    ROBOT_CHECK_CLOSE_BASE_STATION_WATER_SPRAY,     //关闭基站喷水
    ROBOT_CHECK_OPEN_ROTATING_BRUSH,                //开启滚刷
    ROBOT_CHECK_ROTATING_BRUSH_HIGH,                //滚刷高速
    ROBOT_CHECK_ROTATING_BRUSH_LOW,                 //滚刷低速
    ROBOT_CHECK_CLOSE_ROTATING_BRUSH,               //关闭滚刷
    ROBOT_CHECK_OPEN_WATER_JET_PUMP,                //开启主机喷水泵
    ROBOT_CHECK_CLOSE_WATER_JET_PUMP,               //关闭主机喷水泵
    ROBOT_CHECK_OPEN_DRAUGHT_FAN,                   //开启主机风机
    ROBOT_CHECK_CLOSE_DRAUGHT_FAN,                  //关闭主机风机
    ROBOT_CHECK_OPEN_SELF_DRYING,                   //开启基站烘干
    ROBOT_CHECK_CLOSE_SELF_DRYING,                  //关闭基站烘干
    ROBOT_CHECK_BLE_CONNECT,                        //开启蓝牙配对
    ROBOT_CHECK_BLE_GET_CONNECT_STATUS,             //获取蓝牙配对状态
    ROBOT_CHECK_START_CHARGE,                       //开始充电
    ROBOT_CHECK_STOP_CHARGE,                        //停止充电
    /*升级相关*/
    ROBOT_ASK_BTMASTER_UPGRADE,                     //蓝牙主模块升级确认
    ROBOT_ASK_BTSLAVE_UPGRADE,                      //蓝牙从模块升级确认
    ROBOT_START_BTMASTER_UPGRADE,                   //蓝牙主模块开始升级
    ROBOT_START_BTSLAVE_UPGRADE,                    //蓝牙从模块开始升级
    ROBOT_END_BTMASTER_UPGRADE,                     //蓝牙主模块结束升级
    ROBOT_END_BTSLAVE_UPGRADE,                      //蓝牙从模块结束升级
    /*升级相关*/
    ROBOT_CHECK_CHARGE_CURRENT,                     //通过电池温度告知充电电流
    ROBOT_CHECK_OK                                  //检测完成
}ROBOT_CHECK_STATUS;


int bt_msg_init(robot_bluetooth_msg_ctl *ctl)
{
    memset(ctl, 0, sizeof(robot_bluetooth_msg_ctl));
    ctl->btmsg.msg_head = (robot_bluetooth_send_msg_head*)&ctl->buffer[0];
    ctl->btmsg.msg_head->start[0] = 0xfc;
    ctl->btmsg.msg_head->start[1] = 0x01;
    return 0;
}

robot_bluetooth_msg *bt_msg_alloc(robot_bluetooth_msg_ctl* ctl, unsigned char type, unsigned char cmdid, unsigned char cmdparam, unsigned char len)
{
    if(len)
    {
        ctl->btmsg.buffer = &ctl->buffer[6];
    }
    ctl->btmsg.msg_head->cmd_id = cmdid;
    ctl->btmsg.msg_head->cmd_param = cmdparam;
    ctl->btmsg.msg_head->type = type;
    ctl->btmsg.msg_head->length = len + sizeof(robot_bluetooth_send_msg_head) + sizeof(robot_bluetooth_send_msg_tail);
    ctl->btmsg.msg_tail = (robot_bluetooth_send_msg_tail*)&ctl->buffer[6 + len];
    return &ctl->btmsg;
}

int bt_msg_send(robot_bluetooth_msg_ctl* ctl)
{
    int i = 0;
    int j = 0;
	int ret = -1;
    unsigned char len = 0;
    ctl->btmsg.msg_tail->bbc = 0;
    len = ctl->btmsg.msg_head->length;
    for(i = 2; i < len - 2; i++)
    {
        ctl->btmsg.msg_tail->bbc ^= ctl->buffer[i];
    }
    ctl->btmsg.msg_tail->end = 0xfe;
    for (j = 0; j < len; j++)
    {
        printf("%x ", ctl->buffer[j]);
    }
    memcpy(send_buf, ctl->buffer, sizeof(send_buf));
    printf("\n");
    return 0;
}



int robot_bluetooth_message_send(int status)
{
    robot_bluetooth_msg_ctl msgctl;
    robot_bluetooth_msg *msg;
    
    msg = &msgctl.btmsg;
    self_cleaning_msg selfCleaningMsg;
    fill_clean_water_msg fillCleanWaterMsg;
    stoving_water_spray_msg stovingWaterSparayMsg;
    

    memset(&selfCleaningMsg, 0, sizeof(self_cleaning_msg));
    memset(&fillCleanWaterMsg, 0, sizeof(fill_clean_water_msg));
    memset(&stovingWaterSparayMsg, 0, sizeof(stoving_water_spray_msg));



    switch (status)
    {
        case ROBOT_CHECK_MAIN_ENGINE_SELF_CLEANING:         //通知基站自清洁类型:主机自清洁
            selfCleaningMsg.self_cleaning_type = BT_SELF_CLEANING_TYPE_MAIN_ENGINE;
            bt_msg_init(&msgctl);
            msg = bt_msg_alloc(&msgctl, BT_MSG_SEND, BT_TYPE_SELF_CLEANING_TYPE_REQ, BT_CMD_START, 1);
            memcpy(msg->buffer,&selfCleaningMsg,sizeof(selfCleaningMsg));
            bt_msg_send(&msgctl);

            break;

        case ROBOT_CHECK_BASE_STATION_SELF_CLEANING:         //通知基站自清洁类型:基站自清洁
            selfCleaningMsg.self_cleaning_type = BT_SELF_CLEANING_TYPE_BASE_STATION;
            bt_msg_init(&msgctl);
            msg = bt_msg_alloc(&msgctl, BT_MSG_SEND, BT_TYPE_SELF_CLEANING_TYPE_REQ, BT_CMD_START, 1);
            memcpy(msg->buffer,&selfCleaningMsg,sizeof(selfCleaningMsg));
            bt_msg_send(&msgctl);
            break;

        case ROBOT_CHECK_OPEN_INJECTION_CLEAN_WATER:        //开始加注清水
            fillCleanWaterMsg.detergent = 0x1;
            fillCleanWaterMsg.germicide = 0x1;
            fillCleanWaterMsg.Wate_volume = 10;
            fillCleanWaterMsg.total_volume = 30;
            bt_msg_init(&msgctl);
            msg = bt_msg_alloc(&msgctl, BT_MSG_SEND, BT_TYPE_FILL_CLEAN_WATER_REQ, BT_CMD_START, 4);
            memcpy(msg->buffer,&fillCleanWaterMsg,sizeof(fillCleanWaterMsg));
            bt_msg_send(&msgctl);
            break;


        case ROBOT_CHECK_CLOSE_INJECTION_CLEAN_WATER:       //停止加注清水
            bt_msg_init(&msgctl);
            msg = bt_msg_alloc(&msgctl, BT_MSG_SEND, BT_TYPE_FILL_CLEAN_WATER_REQ, BT_CMD_STOP, 0);
            bt_msg_send(&msgctl);
            break;

        case ROBOT_CHECK_OPEN_BASE_STATION_WATER_SPRAY:     //开启基站喷水
            stovingWaterSparayMsg.time = 60/60;
            bt_msg_init(&msgctl);
            msg = bt_msg_alloc(&msgctl, BT_MSG_SEND, BT_TYPE_WATER_SPRAY_REQ, BT_CMD_START, 1);
            memcpy(msg->buffer,&stovingWaterSparayMsg,sizeof(stovingWaterSparayMsg));
            bt_msg_send(&msgctl);
            break;

        case ROBOT_CHECK_CLOSE_BASE_STATION_WATER_SPRAY:    //关闭基站喷水
            bt_msg_init(&msgctl);
            msg = bt_msg_alloc(&msgctl, BT_MSG_SEND, BT_TYPE_WATER_SPRAY_REQ, BT_CMD_STOP, 0);
            bt_msg_send(&msgctl);
            break;

        case ROBOT_CHECK_OPEN_SELF_DRYING:                  //开启基站烘干
            stovingWaterSparayMsg.time = 0x1E;
            bt_msg_init(&msgctl);
            msg = bt_msg_alloc(&msgctl, BT_MSG_SEND, BT_TYPE_STOVING_REQ, BT_CMD_START, 1);
            memcpy(msg->buffer,&stovingWaterSparayMsg,sizeof(stovingWaterSparayMsg));
            bt_msg_send(&msgctl);
            break;

        case ROBOT_CHECK_CLOSE_SELF_DRYING:                  //关闭基站烘干
            bt_msg_init(&msgctl);
            msg = bt_msg_alloc(&msgctl, BT_MSG_SEND, BT_TYPE_STOVING_REQ, BT_CMD_STOP, 0);
            bt_msg_send(&msgctl);
            break;

        case ROBOT_CHECK_BLE_CONNECT:                       //开启蓝牙配对
            bt_msg_init(&msgctl);
            msg = bt_msg_alloc(&msgctl, BT_MSG_SEND, BT_TYPE_BLE_RECONECT_REQ, BT_CMD_START, 0);
            bt_msg_send(&msgctl);
            break;

        case ROBOT_CHECK_BLE_GET_CONNECT_STATUS:            //获取蓝牙配对状态
            bt_msg_init(&msgctl);
            msg = bt_msg_alloc(&msgctl, BT_MSG_SEND, BT_TYPE_BLE_GET_CONN_STATUS_REQ, BT_CMD_QUERY, 0);
            bt_msg_send(&msgctl);
            break;

        default:
            printf("send status is invalid\n");
            break;
    }

    return 0;
}




void ReadBuff(char *RxBuff, int len)
{
    printf("recv_idx:%d\n", recv_idx);
    int i;
    if (recv_idx >= MAX_RECV_CAP - 10) //防止接收的数据过快溢出
    {
        recv_idx = 0;
        recv_pos = 0;
    }
    for (i = 0; i < len; i++)
    {
        recv_buf[recv_idx++] = RxBuff[i];
        recv_idx %= MAX_RECV_CAP;
    }

}

unsigned char read_cmd(unsigned char*len, unsigned char*buff)
{
    unsigned char i, cur, lenidx;
    if (recv_idx - recv_pos < 2)
    {
        printf("too short\n");
        return 0; // 确定帧头和数据长度信息获取到了
    }
    for (i = recv_pos; i < recv_idx; i++) 
    {
        cur = i % MAX_RECV_CAP;
        if ((0xfc == recv_buf[cur]) && (0x01 == recv_buf[cur + 1])) //寻找帧头
        {
            break;
        }
        else 
        {
            recv_buf[cur] = 0;
            recv_pos++;
            recv_pos %= MAX_RECV_CAP;
            //break;
        }
    }

    lenidx = (cur + 2) % MAX_RECV_CAP;
    printf("lxy>>lenidx:%d\n",lenidx);
    printf("i:% d, recv_idx:% d\n", i, recv_idx);
    if (i >= recv_idx)   // 没找到帧头，或者只找到正头，没有获取到长度信息
    {
        printf("not find the head\n");
        return 0;
    }
    else if (recv_buf[lenidx] == 0 || recv_buf[lenidx] > 64) //数据长度信息为0或超过限制
    {
        printf("length of buffer is zero or over limit\n");
        return 0;
    }
    else if ((recv_buf[lenidx] + recv_pos) > recv_idx) // 一帧完整的数据没接收完
    {
        printf("recv not complete\n");
        return 0;
    }
    else 
    {
        // 复制完整的一帧数据出去w
        cur = recv_pos % MAX_RECV_CAP;
        *len = recv_buf[lenidx];
        for (i = 0; i < *len; i++) 
        {
            buff[i] = recv_buf[cur];
            recv_buf[cur] = 0;
            cur++;
            cur %= MAX_RECV_CAP;
        }
        recv_pos += *len;
    }
    // 如果缓存计数和解码计数重合，可清除两个计数，重新开始
    if (recv_pos == recv_idx) 
    {
        recv_idx = 0;
        recv_pos = 0;
    }
    return *len;

}

int main()
{
    int n, j;
    unsigned char buffer[64] = {0};
    unsigned char len = 0;
    while(scanf("%d",&n),n)
    {
        robot_bluetooth_message_send(n);
        ReadBuff(send_buf, MAX_RECV_LEN);
        read_cmd(&len, buffer);
        printf("%d\n",len);
        for (j = 0; j < len; j++)
        {
            printf("%x ", buffer[j]);
        }
    }

}
