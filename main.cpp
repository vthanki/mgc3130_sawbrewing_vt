#include <unistd.h>
#include <errno.h>      // Error number definitions
#include <stdint.h>
#include <string>

#include <wiringSerial.h>
#include <sstream>
#include <iostream>

#define NO_GESTURE						0x00
#define GESTURE_GARBAGE					0x01
#define GESTURE_WEST_EAST				0x02
#define GESTURE_EAST_WEST				0x03
#define GESTURE_SOUTH_NORTH				0x04
#define GESTURE_NORTH_SOUTH				0x05
#define GESTURE_CLOCK_WISE				0x06
#define GESTURE_COUNTER_CLOCK_WISE		0x07

#pragma pack(1)
struct MGCData
{
    uint8_t tag[2];
    uint16_t dataOutputConfig;
    uint8_t counter;
    uint8_t id; // 0x91
    uint16_t outputConfigMask;
    uint8_t timestamp;
    uint8_t systemInfo;

    uint16_t dspInfo;
    struct {
        uint8_t GestureCode			:8;		//	0 -> No Gesture
        //	1 -> Garbage Model
        //  2 -> Flick West To East
        //	3 -> Flick East to West
        //	4 -> Flick South to North
        //	5 -> Flick North to South
        //	6 -> Circle Clockwise
        //	7 -> Circle Counter-Clockwise
        uint8_t Reserved			:4;
        uint8_t GestureType			:4;		//	0 -> Garbage Model
        //  1 -> Flick Gesture
        //	2 -> Circular Gesture
        uint8_t EdgeFlick			:1;		//	If "1" Edge Flick
        uint16_t Reserved2			:14;
        uint8_t GestureInProgress	:1;		//	If "1" Gesture recognition in progress
    } gestureInfo;
    struct {
        uint8_t TouchSouth			:1;
        uint8_t TouchWest			:1;	//	Bit 01
        uint8_t TouchNorth			:1;	//	Bit 02
        uint8_t TouchEast			:1;	//	Bit 03
        uint8_t TouchCentre			:1;	//	Bit 04
        uint8_t TapSouth			:1;	//	Bit 05
        uint8_t TapWest				:1;	//	Bit 06
        uint8_t TapNorth			:1;	//	Bit 07
        uint8_t TapEast				:1;	//	Bit 08
        uint8_t TapCentre			:1;	//	Bit 09
        uint8_t DoubleTapSouth		:1;	//	Bit 10
        uint8_t DoubleTapWest		:1;	//	Bit 11
        uint8_t DoubleTapNorth		:1;	//	Bit 12
        uint8_t DoubleTapEast		:1;	//	Bit 13
        uint8_t DoubleTapCentre		:1; //	Bit 14
        uint8_t nada		        :1; //	Bit 15
        uint32_t FreeBit				:16;
    } touchInfo;
    uint16_t	AirWheelInfo;

    uint8_t  xyzArray[6];
    char dump[222];
};

class MGC
{
public:
    MGC() :
            curPos(0)
    {

    }

    void addVal(int value)
    {
        if (curPos == -1) {
            if (value == 0xfe) {
                curPos = 0;
            }
            return;
        }
        else if (curPos == 0) {
            if (value == 0xff) {
                curPos = 1;
            }
            else {
                curPos = -1;
                return;
            }
        }

        if (curPos < 28) {
            uint8_t * p = (uint8_t *) &data;
            p[curPos] = value;
            ++curPos;
        }
        if (curPos == 28) {
            std::stringstream ss;
            if (data.gestureInfo.GestureCode) {
                ss << " GestCode=" << static_cast<int>(data.gestureInfo.GestureCode);
                switch(data.gestureInfo.GestureCode)
                {
                    case GESTURE_GARBAGE: ss << "garbage"; break;
                    case GESTURE_WEST_EAST: ss << "west-east"; break;                               
                    case GESTURE_EAST_WEST: ss << "east-west"; break;                              
                    case GESTURE_SOUTH_NORTH: ss << "south-north"; break;                        
                    case GESTURE_NORTH_SOUTH: ss << "north-south"; break;                         
                    case GESTURE_CLOCK_WISE: ss << "clock wise"; break;                           
                    case GESTURE_COUNTER_CLOCK_WISE: ss << "Counter clock wise"; break;              
                }
            }
            if (data.gestureInfo.GestureType==1)
                ss << " Flick";
            if (data.gestureInfo.GestureType==2)
                ss << " Circular";
            if (data.gestureInfo.EdgeFlick==2)
                ss << " EdgeFlick";
            if (data.touchInfo.TouchSouth)
                ss << " TchS";
            if (data.touchInfo.TouchWest)
                ss << " TchW";
            if (data.touchInfo.TouchNorth)
                ss << " TchN";
            if (data.touchInfo.TouchEast)
                ss << " TchE";
            if (data.touchInfo.TouchCentre)
                ss << " TchC";
            if (data.touchInfo.TapSouth)
                ss << " TapS";
            if (data.touchInfo.TapNorth)
                ss << " TapN";
            if (data.touchInfo.TapWest)
                ss << " TapW";
            if (data.touchInfo.TapEast)
                ss << " TapE";
            if (data.touchInfo.TapCentre)
                ss << " TapC";
            if (data.touchInfo.DoubleTapSouth)
                ss << " DTapS";
            if (data.touchInfo.DoubleTapNorth)
                ss << " DTapN";
            if (data.touchInfo.DoubleTapWest)
                ss << " DTapW";
            if (data.touchInfo.DoubleTapEast)
                ss << " DTapE";
            if (data.touchInfo.DoubleTapCentre)
                ss << " DTapC";
            if (oldstring != ss.str())
            {
                oldstring = ss.str();

            	//ss << " xyz=[" << data.xyzArray[1]*256+data.xyzArray[0];
            	//ss << "," <<  data.xyzArray[3]*256+data.xyzArray[2];
            	//ss << "," << data.xyzArray[5]*256+data.xyzArray[4] << "]";
                std::cout << ss.str() << std::endl;
            }
            curPos = -1;
        }
    }

protected:
    MGCData data;

private:
    std::string oldstring;
    int curPos;
};

int main(int argc, char *argv[])
{

    if (argc != 2)
    {
        printf("Missing device\n");
    }
    MGC mgc;
    int handler = serialOpen (argv[1], 115200);
    while(1)
    {
        int val = serialGetchar (handler);
        if (val != -1)
        {
            mgc.addVal(val);
        }
    }

}
