/************************************
 * @file main.c
 * @author your name (you@domain.com)
 * @brief RGB彩灯调色程序
 * @version 0.1
 * @date 2022-08-07
 *
 * @copyright Copyright (c) 2022
 *
 ************************************/

#include "STC12C5A60S2.h"
#include <intrins.h> // 声明了void _nop_(void);

/************************************
 * @brief 端口定义
 *
 ************************************/
sbit RED = P1 ^ 0;
sbit GREEN = P1 ^ 1;
sbit BLUE = P1 ^ 2;

/************************************
 * @brief 定时器满溢值
 *
 ************************************/
#define RGB_TIME 0xFFFF

/************************************
 * @brief 灯序计数与彩灯保持计时
 *
 ************************************/
const unsigned short int TIME_SUM = 1000;
char RGB_NUMBER = 0;
char status = 0;

/************************************
 * @brief 按键扫描相关及彩灯保持时间改变
 *
 ************************************/
unsigned char KEY_STATUS = 0;
unsigned short int KEY_RED_TIMES = 0;
unsigned short int KEY_GREEN_TIMES = 0;
unsigned short int KEY_BLUE_TIMES = 0;
unsigned short int KEY_RED_TIME = 0;
unsigned short int KEY_GREEN_TIME = 0;
unsigned short int KEY_BLUE_TIME = 0;

/************************************
 * @brief 按键定义
 *
 ************************************/
#define KEY_RED_UP 0x10
#define KEY_RED_DOWN 0x04
#define KEY_GREEN_UP 0x20
#define KEY_GREEN_DOWN 0x02
#define KEY_BLUE_UP 0x40
#define KEY_BLUE_DOWN 0x08
#define KEY_CHANGE_NUMBER 1
#define LED_ON 1
#define LED_OFF 0

void Delay10ms() //@11.0592MHz
{
    unsigned char i, j;

    _nop_();
    _nop_();
    i = 108;
    j = 144;
    do
    {
        while (--j)
            ;
    } while (--i);
}

void Timer0Init(void) // @11.0592MHz
{
    AUXR |= 0x80; //定时器时钟1T模式
    TMOD &= 0xF0; //设置定时器模式
    TF0 = 0;      //清除TF0标志

    ET0 = 1; //定时器0溢出中断
    EX0 = 0;
}

void UartInit(void) // 4800bps@11.0592MHz
{
    PCON &= 0x7F; //波特率不倍速
    SCON = 0x50;  // 8位数据,可变波特率
    AUXR |= 0x04; //定时器时钟1T模式
    BRT = 0xB8;   //设置定时重载值
    AUXR |= 0x01; //串口1使用独立波特率发射器为波特率发生器
    AUXR |= 0x10; //启动独立波特率发射器
    ES = 1;       //使能串口中断
}

/************************************
 * @brief IO初始化
 *
 ************************************/
void init(void)
{
    P2 = 0;
    Delay10ms();
    RED = 0;
    GREEN = 0;
    BLUE = 0;
    EA = 1;
}

/************************************
 * @brief 定时器初值数据处理
 *
 ************************************/
void Time_Date_Dispose(void)
{
    unsigned short int sum = 0;
    unsigned short int key_sum = 0;

    //计算时间次数和
    key_sum = KEY_RED_TIMES + KEY_GREEN_TIMES + KEY_BLUE_TIMES;

    //判断数据和是否越界
    if (key_sum > TIME_SUM)
    {
        //越界全部归零
        KEY_BLUE_TIMES = 0;
        KEY_GREEN_TIMES = 0;
        KEY_RED_TIMES = 0;
    }

    //计算三值比例(千分位计算)
    if (key_sum != 0)
    {
        KEY_RED_TIME = ((float)KEY_RED_TIMES / key_sum) * TIME_SUM;
        KEY_GREEN_TIME = ((float)KEY_GREEN_TIMES / key_sum) * TIME_SUM;
        KEY_BLUE_TIME = ((float)KEY_BLUE_TIMES / key_sum) * TIME_SUM;
    }
    else
    {
        //为0直接赋值0
        KEY_BLUE_TIME = KEY_BLUE_TIMES;
        KEY_GREEN_TIME = KEY_GREEN_TIMES;
        KEY_RED_TIME = KEY_RED_TIMES;
    }

    //计算比例值和(总值1000)
    sum = KEY_RED_TIME + KEY_GREEN_TIME + KEY_BLUE_TIME;
    //根据比例计算定时器重载值，此前需先处理比例和异常问题
    if (sum == TIME_SUM)
    {
        //至少有一值不为零
        KEY_RED_TIME = RGB_TIME - KEY_RED_TIME * 11;
        KEY_GREEN_TIME = RGB_TIME - KEY_GREEN_TIME * 11;
        KEY_BLUE_TIME = RGB_TIME - KEY_BLUE_TIME * 11;
    }
    else if (sum != 0 && sum < TIME_SUM)
    {
        //计数和不为0也不为1000，但不可能有两值为0
        //判断按下的是那个灯有关的按键
        if (KEY_STATUS == KEY_RED_UP || KEY_STATUS == KEY_RED_DOWN)
        {
            //判断红色比例是否为0
            if (KEY_RED_TIME != 0)
            {
                //红色不为0，直接计算
                KEY_RED_TIME = RGB_TIME - (TIME_SUM - KEY_BLUE_TIME - KEY_GREEN_TIME) * 11;
                KEY_BLUE_TIME = RGB_TIME - KEY_BLUE_TIME * 11;
                KEY_GREEN_TIME = RGB_TIME - KEY_GREEN_TIME * 11;
            }
            else
            {
                //红色为0，剩下两值一定不为0
                KEY_GREEN_TIME = RGB_TIME - (TIME_SUM - KEY_BLUE_TIME) * 11;
                KEY_BLUE_TIME = RGB_TIME - KEY_BLUE_TIME * 11;
            }
        }
        else if (KEY_STATUS == KEY_GREEN_UP || KEY_STATUS == KEY_GREEN_DOWN)
        {
            //判断绿色比例是否为0
            if (KEY_GREEN_TIME != 0)
            {
                //绿色不为0，直接计算
                KEY_GREEN_TIME = RGB_TIME - (TIME_SUM - KEY_BLUE_TIME - KEY_RED_TIME) * 11;
                KEY_BLUE_TIME = RGB_TIME - KEY_BLUE_TIME * 11;
                KEY_RED_TIME = RGB_TIME - KEY_RED_TIME * 11;
            }
            else
            {
                //绿色为0，剩下两值一定不为0
                KEY_BLUE_TIME = RGB_TIME - (TIME_SUM - KEY_RED_TIME) * 11;
                KEY_RED_TIME = RGB_TIME - KEY_RED_TIME * 11;
            }
        }
        else if (KEY_STATUS == KEY_BLUE_UP || KEY_STATUS == KEY_BLUE_DOWN)
        {

            //判断蓝色比例是否为0
            if (KEY_BLUE_TIME != 0)
            {
                //蓝色不为0，直接计算
                KEY_BLUE_TIME = RGB_TIME - (TIME_SUM - KEY_GREEN_TIME - KEY_RED_TIME) * 11;
                KEY_RED_TIME = RGB_TIME - KEY_RED_TIME * 11;
                KEY_GREEN_TIME = RGB_TIME - KEY_GREEN_TIME * 11;
            }
            else
            {
                //蓝色为0，剩下两值一定不为0
                KEY_RED_TIME = RGB_TIME - (TIME_SUM - KEY_GREEN_TIME) * 11;
                KEY_GREEN_TIME = RGB_TIME - KEY_GREEN_TIME * 11;
            }
        }
    }
    else
    {
        //等于0直接赋满值
        KEY_RED_TIME = RGB_TIME;
        KEY_GREEN_TIME = RGB_TIME;
        KEY_BLUE_TIME = RGB_TIME;
    }
}

/************************************
 * @brief 按键扫描
 *
 ************************************/
void key_scan()
{
    int i = 0;

    if (P2 != 0x00)
    {
        TR0 = 0;
        Delay10ms();
        //消抖
        if (P2 != 0x00)
        {
            //获取按键信息
            KEY_STATUS = P2;
            switch (KEY_STATUS)
            {
                //对不同值做不同处理
            case KEY_RED_UP:
                KEY_RED_TIMES += KEY_CHANGE_NUMBER;
                break;
            case KEY_RED_DOWN:
                if (KEY_RED_TIMES >= KEY_CHANGE_NUMBER)
                {
                    KEY_RED_TIMES -= KEY_CHANGE_NUMBER;
                }
                break;
            case KEY_GREEN_UP:
                KEY_GREEN_TIMES += KEY_CHANGE_NUMBER;
                break;
            case KEY_GREEN_DOWN:
                if (KEY_GREEN_TIMES >= KEY_CHANGE_NUMBER)
                {
                    KEY_GREEN_TIMES -= KEY_CHANGE_NUMBER;
                }
                break;
            case KEY_BLUE_UP:
                KEY_BLUE_TIMES += KEY_CHANGE_NUMBER;
                break;
            case KEY_BLUE_DOWN:
                if (KEY_BLUE_TIMES >= KEY_CHANGE_NUMBER)
                {
                    KEY_BLUE_TIMES -= KEY_CHANGE_NUMBER;
                }
                break;
            }

            Time_Date_Dispose();

            //如需设置不连续按键，删掉i即可，反之亦然，i越大，按键值自增时间越长
            while (!(i == 50 || P2 == 0X00))
            {
                Delay10ms();
                i++;
            }
        }
        if ((KEY_BLUE_TIMES + KEY_GREEN_TIMES + KEY_RED_TIMES) == 0)
        {
            TR0 = 0;
            status = 0;
            RGB_NUMBER = 0;
        }
        else
        {
            TR0 = 0;
            status = 1;
            RGB_NUMBER = 0;
        }
    }
}

/***********************************************
 *函数名称：Uart1_SendChar
 *功    能：串口1发送单个字符函数
 *入口参数：Udat：欲发送的数据
 *返 回 值：无
 *备    注：无
 ************************************************/
void Uart1_SendChar(unsigned char Udat)
{
    SBUF = Udat; //将要发送的数据放入串口数据发送区
    while (!TI)
        ;   //等待发送完成
    TI = 0; //清零TI
}

void Rgb_Change(void)
{
    char i = 0;

    //判断RGB持续时间是否为0
    if ((KEY_BLUE_TIMES + KEY_GREEN_TIMES + KEY_RED_TIMES) != 0)
    {
        //判断是否允许RGB状态改变
        if (status)
        {
            TR0 = 0;
            i = RGB_NUMBER;
            status = 0;

            if (RGB_NUMBER == 0)
            {
                //定时器赋初值
                TH0 = KEY_RED_TIME >> 8;
                TL0 = (KEY_RED_TIME << 8) >> 8;

                //改变LED状态
                if (KEY_RED_TIMES > 0)
                {
                    P1 = 0x01;
                }
                else
                {
                    P1 = LED_OFF;
                }

                //灯序+1
                RGB_NUMBER++;
            }
            else if (RGB_NUMBER == 1)
            {
                TH0 = KEY_GREEN_TIME >> 8;
                TL0 = (KEY_GREEN_TIME << 8) >> 8;

                if (KEY_GREEN_TIMES != 0)
                {
                    P1 = 0x02;
                }
                else
                {
                    P1 = LED_OFF;
                }

                RGB_NUMBER++;
            }
            else if (RGB_NUMBER == 2)
            {
                TH0 = KEY_BLUE_TIME >> 8;
                TL0 = (KEY_BLUE_TIME << 8) >> 8;

                if (KEY_BLUE_TIMES != 0)
                {
                    P1 = 0x04;
                }
                else
                {
                    P1 = LED_OFF;
                }

                RGB_NUMBER = 0;
            }

            TR0 = 1;
        }
    }
    else
    {
        //持续时间为0，灯灭
        P1 = LED_OFF;
    }
}

void main()
{
    init();
    Timer0Init();
    UartInit();
    status = 1;
    Time_Date_Dispose();

    Uart1_SendChar(0x88); //开机发送测试数据

    while (1)
    {
        //检测按键有没有按下
        if (P2 == 0x00)
        {
            Rgb_Change();
        }
        else
        {
            key_scan();
        }
    }
}

/************************************
 * @brief 定时器中断函数
 *
 ************************************/
void Time_link() interrupt 1
{
    TR0 = 0;
    status = 1;
}

/***********************************************
 *函数名称：Uart1_ISR
 *功    能：串口1中断处理函数
 *入口参数：无
 *返 回 值：无
 *备    注：无
 ************************************************/
void Uart1_ISR(void) interrupt 4
{
    unsigned char i = 0;
    TR0 = 0;

    if (RI) //发送和接收共用一个中断向量，需在程序中判断
    {
        i = SBUF;
        if (i == 'a')
        {
            //调试用
            Uart1_SendChar((unsigned char)(KEY_RED_TIMES >> 8));
            Uart1_SendChar((unsigned char)((KEY_RED_TIMES << 8) >> 8));
            Uart1_SendChar(' ');
            Uart1_SendChar((unsigned char)(KEY_GREEN_TIMES >> 8));
            Uart1_SendChar((unsigned char)((KEY_GREEN_TIMES << 8) >> 8));
            Uart1_SendChar(' ');
            Uart1_SendChar((unsigned char)(KEY_BLUE_TIMES >> 8));
            Uart1_SendChar((unsigned char)(((KEY_BLUE_TIMES << 8) >> 8)));
            Uart1_SendChar('\n');
            Uart1_SendChar((unsigned char)(KEY_RED_TIME >> 8));
            Uart1_SendChar((unsigned char)((KEY_RED_TIME << 8) >> 8));
            Uart1_SendChar(' ');
            Uart1_SendChar((unsigned char)(KEY_GREEN_TIME >> 8));
            Uart1_SendChar((unsigned char)((KEY_GREEN_TIME << 8) >> 8));
            Uart1_SendChar(' ');
            Uart1_SendChar((unsigned char)(KEY_BLUE_TIME >> 8));
            Uart1_SendChar((unsigned char)(((KEY_BLUE_TIME << 8) >> 8)));
            Uart1_SendChar('\n');
            Uart1_SendChar(P2);
            Uart1_SendChar('\n');
            Uart1_SendChar(P1);
            Uart1_SendChar('\n');
        }

        RI = 0; //串口接收中断标记需软件清零
    }
}
