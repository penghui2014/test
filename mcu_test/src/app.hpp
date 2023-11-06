#pragma once

#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>

#include "protocol.hpp"
#include "connect_tcp.hpp"
#include "common.hpp"
#include "mcu_type.h"


#define UPGRADE_READY_STRING "READY"
#define UPGRADE_START_STRING "START"
#define UPGRADE_END_STRING "END"

typedef enum
{
	MCU_CMD_START_UPGRADE          = 0x01, /* MCU升级开始 */
    MCU_CMD_SEND_UPGRADE_DATA      = 0x02, /* MCU升级数据 */
    MCU_CMD_END_UPGRADE            = 0x03, /* MCU升级结束 */
    
	MCU_CMD_VERSION				   = 0x04,
	MCU_CMD_SHUTDOWN			   = 0x05,
    MCU_CMD_REBOOT				   = 0x06,
    MCU_CMD_WATCHDOG 			   = 0x07,
    MCU_CMD_RTC					   = 0x08,
}SDK_CMD_ID;

typedef enum
{
	MSG_MCU_CTL_OUT_INFO 	= 0x01,
	MSG_MCU_AI_INFO 		= 0x02,
	MSG_MCU_VEH_INFO 		= 0x03,
	MSG_MCU_VEH_ERR_UP 		= 0x04,
	MSG_MCU_CAN_VERSION_UP 	= 0x05,
	MSG_MCU_SPEED_INFO_UP 	= 0x06,
	MSG_MCU_XFD_RADAR_UP 	= 07,

	MSG_SOC_DIAGNOSE_INFO 	= 0x81,
	MSG_SOC_CAMERA_INFO 	= 0x82,
	MSG_SOC_AI_PERCEPTION 	= 0x83,
	MSG_SOC_PLANNING_INFO 	= 0x84,
	MSG_SOC_LOCATION_INFO 	= 0x85,
	MSG_SOC_SWITCH_CMD 		= 0x86,
	MSG_SOC_AUTOMODE_INFO 	= 0x87,
	MSG_SOC_LIGHT_CTL_INFO 	= 0x88,
	MSG_SOC_CTL_DYNAMIC_PARAM = 0x89,
	MSG_SOC_S16_VEH_TEST 	= 0x8A,
	MSG_SOC_AEB_CMD_UP 		= 0x8B,
}AD_MSG_ID;

/*
*@brief 定位路径点信息
 */
typedef struct
{
    xdouble_t localization_X;
    xdouble_t localization_Y;
    xdouble_t localization_Yaw;
    xdouble_t localization_Pitch;
    xdouble_t Actual_VehSpeed;
    xdouble_t Actual_VehAccel;
    xuint32_t SN;
}__attribute__((packed))LSAD_SOC_LP_INFO_S;

/*
*@brief 规划路径点信息
 **/
//cmd 84  9600 length
typedef struct
{
    xdouble_t PathPoint_cur[200];
    xdouble_t PathPoint_X[200];
    xdouble_t PathPoint_Y[200];
    xdouble_t PathPoint_Yaw[200];
    xdouble_t PathPoint_vel[200];
    xdouble_t PathPoint_Direction[200];
    xuint8_t PreView_Num;
    xuint8_t Cur_PreView_Num;
    xuint8_t NearPointNum;
    xuint8_t Trajectory_Size;
    xuint32_t SN;
    xuint8_t reserves[8];
}__attribute__((packed))LSAD_SOC_PP_INFO_S;


/**
  * @brief 切换驾驶模式
  */
  //87 cmd  12 length
typedef struct
{
    xdouble_t VehicleStop;
	xdouble_t K_REQ_TORQUE_P;
	xdouble_t K_REQ_TORQUE_I;
	xdouble_t K_REQ_TORQUE_D;
	xdouble_t Signal_Unusual;
	xdouble_t SweepingBrushSW;
	xuint8_t reserve[16];
}__attribute__((packed))LADS_SOC_CTL_MODE_S;


/**
  * @brief 爱瑞特清扫刷控制指令
  */
  //86 cmd  12 length
typedef struct
{
    xuint8_t frontBrushUpdown;
    xuint8_t backBrushUpdown;
    xuint8_t backBrushRotate;
    xuint8_t frontBrushRotate;
    xuint8_t eletricBrake;
    xuint8_t waterPump;
    xuint8_t shaker;
    xuint8_t mainBrushUpDown;
    xuint8_t draughtFan;
    xuint8_t mainBrushRotate;
    xuint8_t reserve[2];
}__attribute__((packed))LSAD_SOC_SWITCH_CMD_S;

/**
  * @brief 爱瑞特CAN协议输出清扫刷信号
  */
typedef struct
{
	xuint8_t EPSSteeringMode;
	xuint8_t EPSSteeringAngleCalibration;
	xuint8_t BMSsoc;
	xuint8_t ActualThrottleControlMode;
	xuint8_t MotorControllerGearInfo;
	xuint8_t ActualGearInfo;
	xuint8_t IsRxBrakePedalSignal;//0x211 是否收到制动踏板信息
	xuint8_t HoldMotorInfo;
	xuint8_t CurrentThrottleValue;
	xuint8_t BrakePedalAct;//0x243 脚刹信号
	xuint8_t DrivingMode;
	xuint16_t MotorSpeed;//电机转速
	int16_t EPSSteeringAngle;
	xsingle_t EPSSteeringTorque;
	xsingle_t MotorCurrent;
	xdouble_t SteerWheelAngle;

    xuint8_t FrontBrushUpdownStat;
    xuint8_t FrontBrushUpdownFault;
    xuint8_t FrontBrushUpdownInFault;
    xuint8_t BackBrushUpdownStat;
    xuint8_t BackBrushUpdownFault;
    xuint8_t BackBrushUpdownInFault;
    xuint8_t FrontBrushRotateStat;
    xuint8_t FrontBrushRotateInFault;
    xuint8_t BackBrushRotateStat;
    xuint8_t BackBrushRotateInFault;
    xuint8_t LackWaterSig;

    xuint8_t WaterPumpStat;
    xuint8_t WaterPumpFault;
    xuint8_t ShakerStat;
    xuint8_t ShakerFault;
    xuint8_t MainBrushUpDownStat;
    xuint8_t MainBrushUpDownFault;
    xuint8_t MainBrushUpDownInFault;
    xuint8_t DraughtFanStat;
    xuint8_t DraughtFanFault;
    xuint8_t DraughtFanInFault;
    xuint8_t MainBrushRotateStat;
    xuint8_t MainBrushRotateFault;
    xuint8_t MainBrushRotateInFault;

    xuint8_t ElectronicBrakeInfo; /*电子手刹状态0x240*/

	xuint8_t BackBrushInputFault;/*后边刷输入故障*/
	xuint8_t FrontBrushRotateFault;/*前边刷旋转故障*/

	xuint8_t EleBrakeStatu;/*暂时无用(电子手刹)*/
	xuint8_t EleBrakeFault; /*电子手刹故障*/
	xuint8_t EleBrakeInFault;/*电子手刹输入故障*/
	xuint8_t WaterLevel;/*水位信号 水位百分比 0~100*/

	xuint16_t InstrumentSpeed;/*仪表盘速度 单位0.001m/s*/
	xuint16_t InstrumentMilage;/*仪表盘里程 KM*/
	xuint16_t BMSVotlage; /*电池电压V*/
    xuint8_t PowerStat;/*电源状态 0：正常 1：欠压 2：过压*/

    xuint8_t DBSWorkMode;/*线控制动模式*/
    xuint8_t BrakePressureReqACK;/*线控制动ACK反馈*/
    xuint8_t MasterCylinderPress;/*线控制动压力*/

    xuint8_t reserve[32];
}__attribute__((packed))LSAD_MCU_VEH_INFO_S;

typedef struct
{
	xuint8_t VehicleStop;
	xuint8_t SignalUnusual;
	xuint8_t SweepingBrushSW;
	xsingle_t SteerWheelAngle;
	xuint8_t EPSSteeringMode;
	xsingle_t EPSSteeringTorque;
	xsingle_t EPSSteeringAngle;
	xuint8_t EPSSteeringAngleCalibration;
	xsingle_t ActualVehSpeed;
	xsingle_t ActualVehAccel;
	xuint16_t MotorSpeed;
	xuint8_t ActualThrottleControlMode;
	xuint8_t CurrentThrottleValue;
	xuint8_t BrakePedalAct;
	xuint8_t IsRxBrakePedalSignal;
	xuint8_t HoldMotorInfo;
	xsingle_t MotorCurrent;
	xuint8_t BMSsoc;
	xuint8_t MotorControllerGearInfo;
	xuint8_t ActualGearInfo;
	xuint8_t ElectronicBrakeInfo;
	xuint8_t DBSWorkMode;
	xuint8_t BrakePressureReqACK;
	xuint8_t MasterCylinderPress;
	xuint16_t Ultrasonic_Radar1;
	xuint16_t Ultrasonic_Radar2;
	xuint16_t Ultrasonic_Radar3;
	xuint16_t Ultrasonic_Radar4;
	xuint16_t Ultrasonic_Radar5;
	xuint16_t Ultrasonic_Radar6;
	xsingle_t Reserved1;
	xsingle_t Reserved2;
	xsingle_t Reserved3;
	xsingle_t Reserved4;
	xsingle_t Reserved5;
	xsingle_t Reserved6;
	xuint8_t reservesIn[32];

	xuint8_t IsEnterAuto;
	xuint8_t GearControl;
	xuint8_t ThrottleControlMode;
	xuint8_t ThrottleOpening;
	xuint8_t EnergyRecovery;
	xuint8_t ElectronicBrakeControl;
	xuint8_t VCU_DBS_Valid;
	xuint8_t VCU_DBS_Work_Mode;
	xuint8_t VCU_DBS_HP_Pressure;
	xuint8_t SteeringMode;
	int16_t SteeringAngle;
	int16_t SteeringAngleRateCtrl;
	xuint8_t LonState;
	xuint8_t LockState;
	xsingle_t VehSpeedKph;
	xsingle_t TarSpeedKph;
	xsingle_t SpeedLimit;
	xint32_t S16_CtrlVersion;
	xuint8_t Waining_Info1;
	xuint8_t Waining_Info2;
	xuint8_t Waining_Info3;
	xuint8_t DbgUintSignal1;
	xuint8_t DbgUintSignal2;
	xuint8_t DbgUintSignal3;
	xuint8_t DbgUintSignal4;
	xuint8_t DbgUintSignal5;
	xuint8_t DbgUintSignal6;
	xsingle_t DbgSingleSignal1;
	xsingle_t DbgSingleSignal2;
	xsingle_t DbgSingleSignal3;
	xsingle_t DbgSingleSignal4;
	xsingle_t DbgSingleSignal5;
	xsingle_t DbgSingleSignal6;
	xsingle_t DbgSingleSignal7;
	xsingle_t DbgSingleSignal8;
	xuint32_t SN;
	xuint8_t reservesOut[32];
}__attribute__((packed)) MCU_CTL_OUT_INFO_S;

typedef struct
{
	xuint8_t LB:1;	//近光灯开启
	xuint8_t HB:1;	//远光灯开启
	xuint8_t CLC:1;	//示廓灯灯开启
	xuint8_t Wiper:2;//雨刮控制信号
	xuint8_t WiperSpray:1;//雨刮喷水开启
	xuint8_t Horn:1;		//行车喇叭开启
	xuint8_t TLLeft:1;	//左转灯开启

	xuint8_t TLRight:1;	//右转灯开启
	xuint8_t Brake:1;	//刹车信号开启
	xuint8_t HBrake:1;	//手刹信号开启
	xuint8_t	Back:1;		//倒车信号开启
	xuint8_t	Fan:1;		//风扇开启
	xuint8_t EBLamp:1;	//边刷灯开启
	xuint8_t	FogLamp:1;	//雾灯开关开启
	xuint8_t ESSLamp:1;	//双闪开启

	xuint8_t Byte2Reserve:6;	//预留开关2~7开启
	xuint8_t HYD01:1;		//液压开关1
	xuint8_t HYD02:1;		//液压开关2

	xuint8_t HYD03:1;		//液压开关3
	xuint8_t HYD04:1;		//液压开关4
	xuint8_t HYD05:1;		//液压开关5
	xuint8_t HYD06:1;		//液压开关6
	xuint8_t HYD07:1;		//液压开关7
	xuint8_t HYD08:1;		//液压开关8
	xuint8_t HYD09:1;		//液压开关9
	xuint8_t HYD10:1;		//液压开关10

	xuint8_t HYD11:1;		//液压开关11
	xuint8_t HYD12:1;		//液压开关12
	xuint8_t	Byte4Reserve:5;

	xuint8_t Byte5;

	xuint8_t	Byte6;

	xuint8_t Byte7Reserve:7;
	xuint8_t Lock:1;			//锁机
}__attribute__((packed))MCU_VEH_LIGHT_CTL_S;//灯控制信息

typedef struct
{
	xuint8_t HighPedal:1;   	//高踏板故障
	xuint8_t Precharge:1;	 	//预充电故障
	xuint8_t MotorOT:1;     	//电机过热
	xuint8_t CTLOT:1;			//控制器过热
	xuint8_t MPwrOut:1;		//主回路断电
	xuint8_t CurrentSample:1;	//电流采样电路故障
	xuint8_t BMSErr:1;			//BMS故障
	xuint8_t CTLUVP:1;			//控制器输入欠压

	xuint8_t CTLOVP:1;			//控制器呼入过压
	xuint8_t MotolOVHT:1;		//电机过热
	xuint8_t ACCErr:1;			//加速器故障
	xuint8_t MOSErr:1;			//MOS故障
	xuint8_t MotolTempSens:1;	//电机温度传感器故障
	xuint8_t GearErr:1;		//挡位故障
	xuint8_t EncodeErr:1;		//编码器故障
	xuint8_t CTLTempSens:1;	//控制器温度传感器故障

	xuint8_t MotolTempSens2:1; //电机温度传感器故障
	xuint8_t Lock:1;			//锁机状态
	xuint8_t Byte2Reserve:6; //扩展

	xuint8_t CTLTemp;		//控制器温度

	xuint8_t MotorTemp;		//电机温度

	xuint8_t FactorySN:1;	//厂家编号
	xuint8_t CRCErr:1;			//CRC单帧错误
	xuint8_t CRCErrS:1;			//CRC多帧错误
	xuint8_t HB:1;			//心跳单帧错误
	xuint8_t HBS:1;			//心跳多帧错误
	xuint8_t Tran:1;			//通信单帧错误
	xuint8_t Trans:1;		//通信多帧错误

	xuint8_t HeartBeat;		//心跳信号

	xuint8_t CRC;			//CRC
}__attribute__((packed))MCU_VEH_CTL_ERR_S;//车辆控制器故障信息

typedef struct
{
	xuint8_t ERR1; 	//故障码 1
	xuint8_t ERR2;	//和故障码 2
}__attribute__((packed))MCU_EPS_ERR_S;//转向故障信息

typedef struct
{
	xuint8_t HeartBeat;	//心跳

	xuint8_t Wiper:2;	//雨刮状态
	xuint8_t WiperMotor:1; //雨刮喷水电机状态
	xuint8_t Horn:1;			//行车喇叭状态
	xuint8_t LBLeft:1;		//近光灯（左）状态
	xuint8_t LBRight:1;		//近光灯（右）状态
	xuint8_t HBLeft:1;		//远光灯（左）状态
	xuint8_t HBRight:1;		//远光灯（右）状态

	xuint8_t TLLeftFront:1;	//左转灯（前）状态
	xuint8_t TLLeftRear:1;	//左转灯（后）状态
	xuint8_t TLRightFront:1;	//右转灯（前）状态
	xuint8_t TLRightRear:1;	//右转灯（后）状态
	xuint8_t Alarmer:1;		//报警器输出状态
	xuint8_t	EBLamp:1;		//边刷灯状态
	xuint8_t BrakeLamp:1;	//刹车灯状态
	xuint8_t BackLamp:1;		//倒车灯状态

	xuint8_t TrimLamp:1;		//内饰灯状态
	xuint8_t	PumpMotor:1;	//水泵电机状态
	xuint8_t EleFan:1;		//电风扇状态
	xuint8_t DCFan:1;		//散热风扇状态
	xuint8_t FogLampFront:1;	//雾灯（前）状态
	xuint8_t	FogLampRear:1;	//雾灯（后）状态
	xuint8_t SeatRLY:1;		//座椅继电器状态
	xuint8_t RLY02:1;		//预留继电器2状态

	xuint8_t RLY03:1;		//预留继电器3状态
	xuint8_t RLY04:1;		//预留继电器4状态
	xuint8_t RLY05:1;		//预留继电器5状态
	xuint8_t RLY06:1;		//预留继电器6状态
	xuint8_t RLY07:1;		//预留继电器7状态
	xuint8_t HYDRLY01:1;		//液压继电器1状态
	xuint8_t HYDRLY02:1;		//液压继电器2状态
	xuint8_t HYDRLY03:1;		//液压继电器3状态

	xuint8_t HYDRLY04:1;		//液压继电器4状态
	xuint8_t HYDRLY05:1;		//液压继电器5状态
	xuint8_t HYDRLY06:1;		//液压继电器6状态
	xuint8_t HYDRLY07:1;		//液压继电器7状态
	xuint8_t HYDRLY08:1;		//液压继电器8状态
	xuint8_t HYDRLY09:1;		//液压继电器9状态
	xuint8_t HYDRLY10:1;		//液压继电器10状态
	xuint8_t HYDRLY11:1;		//液压继电器11状态

	xuint8_t HYDRLY12:1;		//液压继电器12状态
	xuint8_t LicFront:1;		//左小灯及前牌照灯状态
	xuint8_t LicRear:1;		//右小灯及后牌照灯状态
	xuint8_t AlarmLamp:1;	//警示灯状态
	xuint8_t BackCamBAT:1;	//倒车影像供电状态
	xuint8_t POBAT:1;		//推杆板供电状态
	xuint8_t ENTBAT:1;		//娱乐供电状态
	xuint8_t CIGBAT:1;		//点烟器供电状态

	xuint8_t CLCLamp:1;		//示宽灯状态
	xuint8_t	resBAT:1;		//预留供电状态
	xuint8_t Byte7Reserve:5;
	xuint8_t	Lock:1;			//锁机状态
}__attribute__((packed))MCU_VEH_LIGHT_ST_S;//灯状态

typedef struct
{
	xuint8_t CTLPwr:2; 	//控制电源状态
	xuint8_t LoadPwr1:2;	//负载电源1状态
	xuint8_t LoadPwr2:2;	//负载电源2状态
	xuint8_t LoadGWire:1;//负载地线开路
	xuint8_t Byte0Reserve:1;

	xuint8_t LBLeft:2;	//近光灯左过流
	xuint8_t	LBRight:2;	//近光灯右状态
	xuint8_t HBLeft:2;	//远光灯左状态
	xuint8_t HBRight:2;	//远光灯右状态

	xuint8_t FogLampFront:2; //前雾灯状态
	xuint8_t FogLampRear:2;	//后雾灯状态
	xuint8_t TLLeftFront:2;	//左转灯（前）状态
	xuint8_t TLLeftRear:2;	//左转灯（后）状态

	xuint8_t	TLRightFront:2;	//右转灯（前）状态
	xuint8_t TLRightRear:2;	//右转灯（后）状态
	xuint8_t EBLamp:2;		//边刷灯状态
	xuint8_t LicFront:2;		//左小灯和前牌照灯状态

	xuint8_t LicRear:2;		//右小灯和前牌照灯状态
	xuint8_t Horn:2;			//喇叭输出状态
	xuint8_t Alarm:2;		//报警输出状态
	xuint8_t Pump:2;			//水泵电机状态

	xuint8_t Byte5;

	xuint8_t Byte6;

	xuint8_t Byte7;
}__attribute__((packed))MCU_VEH_LIGHT_ERR_S;//灯故障反馈

typedef struct
{
	xuint8_t BrakeLamp:2;	//刹车灯状态
	xuint8_t BackLamp:2;		//倒车灯状态
	xuint8_t TrimLamp:2;		//内饰灯状态
	xuint8_t WiperMotor:2; 	//雨刮喷水电机状态

	xuint8_t Wiper:2;		//雨刮水壶状态
	xuint8_t EleFan:2;		//电风扇状态
	xuint8_t DCFan:2;		//散热风扇状态
	xuint8_t CLCLamp:2;		//示宽灯状态

	xuint8_t MCMBAT:2;		//MCM供电状态
	xuint8_t BackCamBAT:2;	//倒车影像供电状态
	xuint8_t CIGBAT:2;		//点烟器供电状态
	xuint8_t ENTBAT:2;		//娱乐供电状态

	xuint8_t HYDRLY01:2;		//液压继电器1状态
	xuint8_t HYDRLY02:2;		//液压继电器2状态
	xuint8_t HYDRLY03:2;		//液压继电器3状态
	xuint8_t HYDRLY04:2;		//液压继电器4状态

	xuint8_t HYDRLY05:2;		//液压继电器5状态
	xuint8_t HYDRLY06:2;		//液压继电器6状态
	xuint8_t HYDRLY07:2;		//液压继电器7状态
	xuint8_t HYDRLY08:2;		//液压继电器8状态

	xuint8_t HYDRLY09:2;		//液压继电器9状态
	xuint8_t HYDRLY10:2;		//液压继电器10状态
	xuint8_t HYDRLY11:2;		//液压继电器11状态
	xuint8_t HYDRLY12:2;		//液压继电器12状态

	xuint8_t AlarmLamp:2;	//警示灯状态
	xuint8_t Byte6Reseve:6;

	xuint8_t Byte7;
}__attribute__((packed))MCU_VEH_RLY_ERR_S;//继电器故障反馈

typedef struct
{
	xuint8_t Insulation:1; 	//绝缘故障
	xuint8_t PreCharge:1;		//预充电故障
	xuint8_t MPotContor:1;	//总正接触器故障
	xuint8_t MNetContor:1;	//总负接触器故障
	xuint8_t PreChaContor:1;	//预充接触器故障
	xuint8_t ChargeContor:1;	//充电机接触器故障
	xuint8_t LowTempe:1;		//温度过低故障
	xuint8_t HighTempe:1;	//温度过高故障

	xuint8_t DiffTempe:1;	//温度差异故障
	xuint8_t OutIChar:1;	//充电过流故障
	xuint8_t OutIDisC:1;	//放电过流故障
	xuint8_t SiglOutV:1;	//单体过压故障
	xuint8_t SiglUnderV:1;//单体欠压故障
	xuint8_t SiglDiffV:1;//单体电压差异故障
	xuint8_t ArrayOutV:1;	//电池组过压故障
	xuint8_t ArrayUnderV:1;//电池组欠压故障

	xuint8_t LowSOC:1;	//SOC 过低故障
	xuint8_t HightSOC:1;	//SOC 过高故障
	xuint8_t CtlPwl:1;	//控制板电源故障
	xuint8_t SubCan:1;	//子板CAN通讯故障
	xuint8_t VCUCan:1;	//与VCU间CAN通讯故障
	xuint8_t ChargeCan:1;//与充电机CAN通讯故障
	xuint8_t CoolSys:1;	//电池冷却系统故障
	xuint8_t HeatSys:1;	//电池加热系统故障

	xuint8_t SiglVSens:1;//单体电压传感器故障
	xuint8_t ISens:1;	//电流传感器故障
	xuint8_t TempeSens:1;//温度传感器故障
	xuint8_t VSens:1;	//总电压传感器故障
	xuint8_t HumiditySens:1;//湿度传感器故障
	xuint8_t ChargerErr:1;	//充电机故障
	xuint8_t ServiceSwitch:1;//维修开关接通状态
	xuint8_t BatteryPackSwitch:1;//电池包揭盖开关

	xuint8_t Byte[4];
}__attribute__((packed))MCU_VEH_BAT_ERR_S;//电池故障反馈

typedef struct
{
	xuint8_t LPErr:1; //MCU未接收到定位数据
	xuint8_t PPErr:1; //MCU未接收到规划数据
	xuint8_t ModeErr:1; //MCU未接收到驾驶模式数据
	xuint8_t CANErr:1; //MCU未接收到车辆底盘can总线数据
	xuint8_t CANCrcErr:1;//MCU接收到车辆底盘can总线数据crc异常
	xuint8_t CANHeartErr:1;//MCU接收到车辆底盘can总线数据心跳异常
	xuint8_t Updating:1; //MCU正在升级
	xuint8_t Signal_Unusual:1;

	xuint8_t aebErr:1;//MCU未接收到AEB数据
	xuint8_t reserv:7;
	xuint8_t Byte[6];
}__attribute__((packed))MCU_SYS_COMM_S;//MCU系统通讯故障反馈

typedef struct
{
	xuint8_t overDuty:1;//过载故障
	xuint8_t overTemp:1;//过温或过压故障
	xuint8_t mosOut:1;//MOS短路故障
	xuint8_t pwrUnderV:1;//电源欠压故障
	xuint8_t pwrOverV:1;//电源过压故障
	xuint8_t underV:1;//压力不足
	xuint8_t phaseLoss:1;//缺相故障
	xuint8_t comitErr:1;//通讯故障

	xuint8_t curSampe:1;//电流采样故障
	xuint8_t driveErr:1;//驱动故障
	xuint8_t mknitt:1;//磁编故障
	xuint8_t presSenErr:1;//压力传感器故障
	xuint8_t padSenErr:1;//踏板位置传感器故障
	xuint8_t mechanErr:1;//机械故障
	xuint8_t otherErr:1;//其他故障
	xuint8_t reserve:1;//预留

	xuint8_t reserves;//预留
}__attribute__((packed))MCU_VCU_ERR_S;//线控刹车VCU故障反馈

typedef struct
{
	xuint32_t mask;
	MCU_VEH_CTL_ERR_S controlErr213;
	MCU_EPS_ERR_S EPSErr401;
	MCU_VEH_LIGHT_ST_S lightST227;
	MCU_VEH_LIGHT_ERR_S lightErr229;
	MCU_VEH_RLY_ERR_S RLYErr22A;
	MCU_VEH_BAT_ERR_S battary218;
	MCU_SYS_COMM_S sysComm;
	xuint8_t safetyEdge;//安全触边 0：未触边 1：触边
	xuint8_t emeryPowerOff;//紧急断电按钮状态  0：未按下  1：按下
	MCU_VCU_ERR_S vcuErr143;
	xuint8_t reserve[203];
}__attribute__((packed))MCU_VEH_ERR_S;//故障总信息


typedef struct
{
	xdouble_t K_StartTorque_CONF;
	xdouble_t K_REQ_TORQUE_ADD_CONF;
	xdouble_t K_PITCH2TRQ_TBL_GAIN_CONF;
	xdouble_t K_STEER_Torque_GAIN_CONF;
	xdouble_t K_MAXThrottle_CONF;
	xdouble_t K_MINThrottle_CONF;
	xdouble_t K_Over_Speed_CONF;
	xdouble_t K_MaxSpeedLimit_CONF;
	xdouble_t K_Increment_PID_P_CONF;
	xdouble_t K_Increment_PID_I_CONF;
	xdouble_t K_Increment_Gain_CONF;
	xdouble_t K_Start_Gain_CONF;
	xdouble_t K_BackIncrement_Gain_CONF;
	xdouble_t K_BackWardStart_Gain_CONF;
	xdouble_t K_ThtottleChangeCycle_CONF;
	xdouble_t K_StartThrottle_CalSW_CONF;
	xdouble_t K_CalSpeed_ThresholdValue_CONF;
	xdouble_t K_CalThrottle_ChangePeriod_CONF;
	xdouble_t K_S16_matrix_q1_CONF;
	xdouble_t K_S16_matrix_q3_CONF;
	xdouble_t K_PreView_Num_MAX_CONF;
	xdouble_t K_PreView_Num_MIN_CONF;
	xdouble_t K_Predicted_Distance_CONF;
	xdouble_t K_LatDisError_I_CONF;
	xdouble_t K_LatDisError_P_CONF;
	xdouble_t K_LatDis_ErrGain_CONF;
	xdouble_t K_LatHeading_ErrGain_CONF;
	xdouble_t K_Max_LatAcc_CONF;
	xdouble_t K_FeedForward_Gain_CONF;
	xdouble_t K_Curvature_Gain_CONF;
	xdouble_t K_SteeringAngle_CalSW_CONF;
	xdouble_t K_SteeringAngle_CalValue_CONF;
	xdouble_t K_PreView_Num_Dis_CONF;
	xdouble_t K_CurPreView_Num_Dis_CONF;
	xdouble_t K_SpeedPreTime_CONF;
	xdouble_t K_Cur_Torque_Gain_CONF;
}__attribute__((packed))SOC_CTL_DYNAMIC_PARAM_S;

typedef struct
{
	xuint32_t Switch1;
	xuint32_t Switch2;
	xuint32_t BCM;
	xuint32_t Battery;
	xuint32_t EPS;
	xuint32_t Motive;
	xuint8_t  reserve[32];
}__attribute__((packed))MCU_CAN_VERSION_S;

typedef struct
{
	xuint16_t type;
	xuint16_t start; //0:stop 1:start
	xuint16_t mask;
	xuint16_t response;
	int32_t param0;
	int32_t param1;
	int32_t param2;
	int32_t param3;
	int32_t param4;
	int32_t param5;
	int32_t param6;
	xint8_t reserve[64];
}__attribute__((packed))LSAD_TEST_S;

typedef struct
{
	xsingle_t leftSpeed;
	xsingle_t rightSpeed;
	xsingle_t motorSpeed;
	xint8_t reserve[32];
}__attribute__((packed))LSAD_WHEEL_SPEED_S;


typedef struct
{
	xuint16_t L2;
	xuint16_t L3;
	xuint16_t L4;
	xuint16_t R2;
	xuint16_t R3;
	xuint16_t R4;
	xuint8_t precision;//精度 0:1cm 1:2cm
	xuint8_t error;
	xuint8_t reserve[8];
}__attribute__((packed))LSAD_XFD_RADAR_S;


typedef struct
{
	uint64_t time;//ms
	uint64_t brakeTime;//ms

	xdouble_t dbgEgoPath;
	xdouble_t dgbSwAngle;

	xdouble_t dbgInfo1;
	xdouble_t dbgInfo2;
	xdouble_t dbgInfo3;

	xuint8_t egoPathValid;
	xuint8_t dbgInfo4;
	xuint8_t dbgInfo5;
	xuint8_t dbgInfo6;
	xuint8_t brake;//1:break

	xuint8_t reserve[15];
}__attribute__((packed))LSAD_AEB_CMD_S;




int StartUpgrade(CProtocol* proto,const char* filepath,int32_t offset = 180);

void SendRTCTime(CProtocol* proto);

void SendReboot(CProtocol* proto);

int SendWatchDog(CProtocol* proto,bool open = true);

void SendShutDown(CProtocol* proto,uint8_t type = 0x30/*定时关机*/);

std::string GetVersion(CProtocol* proto);






