#include <unistd.h>

#include <stdio.h>
#include <stdint.h>
#include <string>
#include <sstream>
#include <iostream>
#include <fcntl.h>

#define WIRING_PI
#ifdef WIRING_PI
#include <wiringSerial.h>
#endif


#define NO_GESTURE                 0x00
#define GESTURE_GARBAGE            0x01
#define GESTURE_WEST_EAST          0x02
#define GESTURE_EAST_WEST          0x03
#define GESTURE_SOUTH_NORTH        0x04
#define GESTURE_NORTH_SOUTH        0x05
#define GESTURE_CLOCK_WISE         0x06
#define GESTURE_COUNTER_CLOCK_WISE 0x07

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
        uint8_t gestureCode:8;		//	0 -> No Gesture
        uint8_t reserved:4;
        uint8_t gestureType:4;
        uint8_t edgeFlick:1;
        uint16_t reserved2:14;
        uint8_t gestureInProgress:1; //	If "1" Gesture recognition in progress
    } gestureInfo;
    struct {
        uint8_t touchSouth:1;
        uint8_t touchWest:1;	//:Bit 01
        uint8_t touchNorth:1;	//:Bit 02
        uint8_t touchEast:1;	//:Bit 03
        uint8_t touchCentre:1;	//:Bit 04
        uint8_t tapSouth:1;	//:Bit 05
        uint8_t tapWest:1;	//:Bit 06
        uint8_t tapNorth:1;	//:Bit 07
        uint8_t tapEast	:1;	//:Bit 08
        uint8_t tapCentre:1;	//:Bit 09
        uint8_t doubleTapSouth:1;	//:Bit 10
        uint8_t doubleTapWest:1;	//:Bit 11
        uint8_t doubleTapNorth:1;	//:Bit 12
        uint8_t doubleTapEast:1;	//:Bit 13
        uint8_t doubleTapCentre:1; //:Bit 14
        uint8_t nada:1; //:Bit 15
        uint32_t free:16;
    } touchInfo;
    uint16_t	AirWheelInfo;

    uint8_t  xyzArray[6];
    char padding[32]; // if we do some crap, do it here...
};

class MGC
{
public:
    MGC(bool dumpxyz=false)
      : m_curPos (0)
      , m_oldstring("")
      , m_showxyz(dumpxyz)
    {

    }

    void addVal(int value)
    {
        if (m_curPos == -1) {
            if (value == 0xfe) {
                m_curPos = 0;
            }
            return;
        }
        else if (m_curPos == 0) {
            if (value == 0xff) {
                m_curPos = 1;
            }
            else {
                m_curPos = -1;
                return;
            }
        }
        if (m_curPos == 4)
        {
        }

        if (m_curPos < 28) {
            uint8_t * p = (uint8_t *) &m_payload;
            p[m_curPos] = static_cast<uint8_t>(value);
            ++m_curPos;
        }
        if (m_curPos == 28) {
            std::stringstream ss;
            if (m_payload.gestureInfo.gestureCode) {
                ss << " GestCode=" << static_cast<int>(m_payload.gestureInfo.gestureCode);
                switch(m_payload.gestureInfo.gestureCode)
                {
                    case GESTURE_GARBAGE: ss << " garbage"; break;
                    case GESTURE_WEST_EAST: ss << " west-east"; break;
                    case GESTURE_EAST_WEST: ss << " east-west"; break;
                    case GESTURE_SOUTH_NORTH: ss << " south-north"; break;
                    case GESTURE_NORTH_SOUTH: ss << " north-south"; break;
                    case GESTURE_CLOCK_WISE: ss << " clock wise"; break;
                    case GESTURE_COUNTER_CLOCK_WISE: ss << " counter clock wise"; break;
                }
            }
            if (m_payload.gestureInfo.gestureType == 1)
                ss << " Flick";
            if (m_payload.gestureInfo.gestureType == 2)
                ss << " Circular";
            if (m_payload.gestureInfo.edgeFlick == 2)
                ss << " EdgeFlick";
            if (m_payload.touchInfo.touchSouth)
                ss << " TchS";
            if (m_payload.touchInfo.touchWest)
                ss << " TchW";
            if (m_payload.touchInfo.touchNorth)
                ss << " TchN";
            if (m_payload.touchInfo.touchEast)
                ss << " TchE";
            if (m_payload.touchInfo.touchCentre)
                ss << " TchC";
            if (m_payload.touchInfo.tapSouth)
                ss << " TapS";
            if (m_payload.touchInfo.tapNorth)
                ss << " TapN";
            if (m_payload.touchInfo.tapWest)
                ss << " TapW";
            if (m_payload.touchInfo.tapEast)
                ss << " TapE";
            if (m_payload.touchInfo.tapCentre)
                ss << " TapC";
            if (m_payload.touchInfo.doubleTapSouth)
                ss << " DTapS";
            if (m_payload.touchInfo.doubleTapNorth)
                ss << " DTapN";
            if (m_payload.touchInfo.doubleTapWest)
                ss << " DTapW";
            if (m_payload.touchInfo.doubleTapEast)
                ss << " DTapE";
            if (m_payload.touchInfo.doubleTapCentre)
                ss << " DTapC";
            if (m_payload.AirWheelInfo)
            {

            }
            if (m_showxyz)
            {
                ss << " xyz=[" << m_payload.xyzArray[1] * 256 + m_payload.xyzArray[0];
                ss << "," << m_payload.xyzArray[3] * 256 + m_payload.xyzArray[2];
                ss << "," << m_payload.xyzArray[5] * 256 + m_payload.xyzArray[4] << "]";
            }
            if (m_oldstring != ss.str())
            {
                m_oldstring = ss.str();

                if (ss.str().length ())
                {
                    std::cout << ss.str () << std::endl;
                }
            }
            m_curPos = -1;
        }
    }

protected:
    MGCData m_payload;

private:
    int m_curPos;
    std::string m_oldstring;
    bool m_showxyz;
};

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Usage %s device [--xyz]\n", argv[0]);
        exit(2);
    }
    bool dumpxyz=false;
    if (argc == 3)
    {
        if (std::string(argv[2]) == std::string("--xyz"))
        {
            dumpxyz = true;
        }
        else
        {
            printf("unknown option %s", argv[2]);
            exit(2);
        }
    }
    
    MGC mgc(dumpxyz);

#ifdef WIRING_PI
    int handler = serialOpen (argv[1], 115200);
    while(1)
    {
        int val = serialGetchar (handler);
        if (val != -1)
        {
            mgc.addVal(val);
        }
    }
#else
    int handler = open (argv[1], O_RDONLY | O_NOCTTY);
    while(1)
    {
        char buffer[32];
        int n = read(handler, buffer, sizeof(buffer));
        if (n>0)
        {
            for (int i=0; i < n; ++i)
                mgc.addVal(buffer[i]);
        }
    }

#endif

}
