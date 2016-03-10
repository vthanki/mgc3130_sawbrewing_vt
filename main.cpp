#include <unistd.h>

#include <stdio.h>
#include <stdint.h>
#include <string>
#include <sstream>
#include <iostream>
#include <fcntl.h>
#include <errno.h>

#include "serial.h"
#include "uinput.h"

#define NO_GESTURE                 0x00
#define GESTURE_GARBAGE            0x01
#define GESTURE_WEST_EAST          0x02
#define GESTURE_EAST_WEST          0x03
#define GESTURE_SOUTH_NORTH        0x04
#define GESTURE_NORTH_SOUTH        0x05
#define GESTURE_CLOCK_WISE         0x06
#define GESTURE_COUNTER_CLOCK_WISE 0x07

static int keys[] = {
	KEY_PLAYPAUSE,
	KEY_NEXTSONG,
	KEY_PREVIOUSSONG,
	KEY_STOP,
	KEY_FORWARD,
	KEY_REWIND,
	KEY_VOLUMEUP,
	KEY_VOLUMEDOWN,
	KEY_MUTE,
};


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
		MGC(bool tagxyz = false, bool dumpxyz = false, bool mapped = false, int ufd = -1)
			: m_curPos (-1)
			  , m_oldstring("")
			  , m_tagxyz(tagxyz)
			  , m_showxyz(dumpxyz)
			  , m_mapped(mapped)
			  , m_ufd(ufd)

	{

	}

	void addVal(unsigned char value)
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
			if (m_mapped) {
				mapVal();
			} else {
				std::stringstream ss;
				if (m_payload.gestureInfo.gestureType == 1)
					ss << " Flick";
				if (m_payload.gestureInfo.gestureType == 2)
					ss << " Circular";
				if (m_payload.gestureInfo.edgeFlick == 2)
					ss << " EdgeFlick";
				if (m_payload.gestureInfo.gestureCode) {
					switch(m_payload.gestureInfo.gestureCode)
					{
						case GESTURE_GARBAGE: ss << " garbage"; break;
						case GESTURE_WEST_EAST: ss << " west-east"; break;
						case GESTURE_EAST_WEST: ss << " east-west"; break;
						case GESTURE_SOUTH_NORTH: ss << " south-north"; break;
						case GESTURE_NORTH_SOUTH: ss << " north-south"; break;
						case GESTURE_CLOCK_WISE: ss << " clock wise"; break;
						case GESTURE_COUNTER_CLOCK_WISE: ss << " counter clock wise"; break;
						default:
										 ss << " GestCode=" << static_cast<int>(m_payload.gestureInfo.gestureCode);
										 break;
					}
				}
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
					ss << " [" << m_payload.xyzArray[1] * 256 + m_payload.xyzArray[0];
					ss << "," << m_payload.xyzArray[3] * 256 + m_payload.xyzArray[2];
					ss << "," << m_payload.xyzArray[5] * 256 + m_payload.xyzArray[4] << "]";
				}
				if (m_oldstring != ss.str())
				{
					m_oldstring = ss.str();

					if (ss.str().length ())
					{
						if (m_tagxyz)
						{
							ss << " [" << m_payload.xyzArray[1] * 256 + m_payload.xyzArray[0];
							ss << "," << m_payload.xyzArray[3] * 256 + m_payload.xyzArray[2];
							ss << "," << m_payload.xyzArray[5] * 256 + m_payload.xyzArray[4] << "]";
						}
						std::cout << ss.str () << std::endl;
					}
				}
			}
			m_curPos = -1;
		}
	}

	protected:
		MGCData m_payload;

	private:
		int m_curPos, m_ufd;
		std::string m_oldstring;
		bool m_tagxyz;
		bool m_showxyz;
		bool m_mapped;
		bool ev_state[8];

		void emitKeys(int id, int event, int keycode)
		{
			if (ev_state[id] ^ event) {
				ev_state[id] = event;
				if (event) {
					printf("keycode:0x%x\n", keycode);
					lm_gen_keystroke(m_ufd, keycode);
				}
			}
		}

		void mapVal()
		{
			emitKeys(0, m_payload.touchInfo.touchSouth, KEY_VOLUMEDOWN);
			emitKeys(1, m_payload.touchInfo.touchNorth, KEY_VOLUMEUP);
			emitKeys(2, m_payload.touchInfo.touchEast, KEY_NEXTSONG);
			emitKeys(3, m_payload.touchInfo.touchWest, KEY_PREVIOUSSONG);
			emitKeys(4, m_payload.gestureInfo.gestureCode == GESTURE_WEST_EAST, KEY_FORWARD);
			emitKeys(5, m_payload.gestureInfo.gestureCode == GESTURE_EAST_WEST, KEY_REWIND);
		}

};

int main(int argc, char *argv[])
{
	int uinput_fd;
	if (argc < 2)
	{
		fprintf(stderr, "Usage %s device [--allxyz -tapxyz --mapped]\n", argv[0]);
		fprintf(stderr, "--allxyz show continues coordinates\n");
		fprintf(stderr, "--tapxyz show coordinate with event\n");
		exit(2);
	}
	bool dumpxyz = false;
	bool tapxyz = false;
	bool mapped = false;
	if (argc == 3)
	{
		if (std::string(argv[2]) == std::string("--tapxyz")) {
			tapxyz = true;
		} else if (std::string(argv[2]) == std::string("--allxyz")) {
			dumpxyz = true;
		} else if (std::string(argv[2]) == std::string("--mapped")) {
			mapped = true;
		} else
		{
			printf("unknown option %s", argv[2]);
			exit(2);
		}
	}

	if (mapped) {
		uinput_fd = lm_setup_dev(keys, sizeof(keys)/sizeof(keys[0]));
	}

	MGC mgc(tapxyz, dumpxyz, mapped, uinput_fd);

	unsigned char buffer[32];
	int handler, i, rc = 0;

	handler = open (argv[1], O_RDONLY | O_NOCTTY);

	if (handler < 0) {
		perror("open");
		goto exit;
	}

	/* Set the baudrate and mark the TTY port blocking for read */
	rc = lm_set_tty_attr(handler, B115200, 0);
	if (rc)
		goto cleanup;

	while(1) {
		rc = read(handler, buffer, sizeof(buffer));
		if (rc) {
			for (i = 0;  i < rc; i++)
				mgc.addVal(buffer[i]);
		} else {
			perror("read");
			break;
		}
	}

cleanup:
	close(handler);
	if (mapped)
		lm_cleanup_dev(uinput_fd);
exit:
	return rc;

}
