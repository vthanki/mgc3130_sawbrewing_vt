#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sstream>
#include <iostream>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <termios.h>
#include <linux/input.h>
#include <linux/uinput.h>

#define die(str, args...) do { \
	perror(str); \
	exit(EXIT_FAILURE); \
} while(0)


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

static bool interrupted;

static void mgc_handler(int signum)
{
	interrupted = true;
}
#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))
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
		MGC(bool tagxyz = false, bool dumpxyz = false, bool mapped = false, const char *tty = "/dev/ttyACM0")
			: m_curPos (-1)
			  , m_oldstring("")
			  , m_tagxyz(tagxyz)
			  , m_showxyz(dumpxyz)
			  , m_mapped(mapped)

		{
			int rc;

			m_serial_fd = open (tty, O_RDONLY | O_NOCTTY);

			if (m_serial_fd < 0) {
				perror("open");
				exit(-1);
			}

			/* Set the baudrate and mark the TTY port blocking for read */
			rc = mgc_set_tty_attr(m_serial_fd, B115200, 0);
			if (rc)
				exit(-1);

			if (mapped) {
				m_uinput_fd = mgc_setup_dev(keys, ARRAY_SIZE(keys));
				if (m_uinput_fd < 0)
					exit (-1);
			}

			std::cout << "Device initialized done.\n";
		}

		~MGC(void)
		{
			close(m_serial_fd);
			if (m_mapped)
				mgc_cleanup_dev(m_uinput_fd);

			std::cout << "Device cleanup done.\n";
		}

		void addVal(unsigned char value)
		{
			if (m_curPos == -1) {
				if (value == 0xfe)
					m_curPos = 0;
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

		void detectGesture(void)
		{
			int rc = 0, i;
			unsigned char buffer[32];

			while(!interrupted) {
				rc = read(m_serial_fd, buffer, sizeof(buffer));
				if (rc) {
					for (i = 0;  i < rc; i++)
						addVal(buffer[i]);
				} else {
					perror("read");
					break;
				}
			}
		}

	protected:
		MGCData m_payload;

	private:
		int m_curPos;
		int m_serial_fd, m_uinput_fd;
		std::string m_oldstring;
		bool m_tagxyz;
		bool m_showxyz;
		bool m_mapped;
		bool ev_state[8];

		int mgc_register_keys(int fd, int *key_arr, int size)
		{
			int i;

			if(ioctl(fd, UI_SET_EVBIT, EV_KEY) < 0)
				die("error: ioctl");

			for (i = 0; i < size; i++)
				if(ioctl(fd, UI_SET_KEYBIT, key_arr[i]) < 0)
					die("error: ioctl");

			return 0;
		}

		int mgc_gen_keystroke(int fd, int key)
		{
			struct input_event     ev;

			memset(&ev, 0, sizeof(struct input_event));
			ev.type = EV_KEY;
			ev.code = key;
			ev.value = 1;
			if(write(fd, &ev, sizeof(struct input_event)) < 0)
				die("error: write");

			memset(&ev, 0, sizeof(struct input_event));
			ev.type = EV_KEY;
			ev.code = key;
			ev.value = 0;
			if(write(fd, &ev, sizeof(struct input_event)) < 0)
				die("error: write");

			memset(&ev, 0, sizeof(struct input_event));
			ev.type = EV_SYN;
			ev.code = 0;
			ev.value = 0;
			if(write(fd, &ev, sizeof(struct input_event)) < 0)
				die("error: write");

			return 0;
		}

		int mgc_setup_dev(int *keys, int num_keys)
		{
			int fd;
			struct uinput_user_dev uidev;

			fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
			if(fd < 0)
				die("error: open");

			mgc_register_keys(fd, keys, num_keys);

			memset(&uidev, 0, sizeof(uidev));
			snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "libmisc-uinput");
			uidev.id.bustype = BUS_VIRTUAL;

			if(write(fd, &uidev, sizeof(uidev)) < 0)
				die("error: write");

			if(ioctl(fd, UI_DEV_CREATE) < 0)
				die("error: ioctl");

			sleep(2);

			return fd;
		}

		void mgc_cleanup_dev(int fd)
		{
			if(ioctl(fd, UI_DEV_DESTROY) < 0)
				die("error: ioctl");

			close(fd);

		}
		int mgc_set_tty_attr(int fd, int speed, int parity)
		{
			struct termios tty;

			if (!isatty(fd)) {
				printf("Not a TTY file descriptor\n");
				return -EBADF;
			}

			memset (&tty, 0, sizeof tty);

			if (tcgetattr (fd, &tty) != 0) {
				perror("tcgetattr");
				return -1;
			}

			if (cfsetospeed (&tty, speed)) {
				perror("cfsetospeed");
				return -1;
			}
			if (cfsetispeed (&tty, speed)) {
				perror("cfsetispeed");
				return -1;
			}

			tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
			tty.c_iflag &= ~IGNBRK;
			tty.c_lflag = 0;
			tty.c_oflag = 0;
			/* Make read blocking on at least 1 char */
			tty.c_cc[VMIN]  = 1;
			tty.c_cc[VTIME] = 0;
			tty.c_iflag &= ~(IXON | IXOFF | IXANY);
			tty.c_cflag |= (CLOCAL | CREAD);
			tty.c_cflag &= ~(PARENB | PARODD);
			tty.c_cflag |= parity;
			tty.c_cflag &= ~CSTOPB;
			tty.c_cflag &= ~CRTSCTS;

			if (tcsetattr (fd, TCSANOW, &tty)) {
				perror("tcsetattr");
				return -1;
			}

			return 0;
		}
		void emitKeys(int id, int event, int keycode)
		{
			if (ev_state[id] ^ event) {
				ev_state[id] = event;
				if (event) {
					std::cout << "keycode:" << keycode << std::endl;
					mgc_gen_keystroke(m_uinput_fd, keycode);
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
	bool dumpxyz = false;
	bool tapxyz = false;
	bool mapped = false;
	struct sigaction sig;

	if (argc < 2) {
		fprintf(stderr, "Usage %s device [--allxyz -tapxyz --mapped]\n", argv[0]);
		fprintf(stderr, "--allxyz show continues coordinates\n");
		fprintf(stderr, "--tapxyz show coordinate with event\n");
		fprintf(stderr, "--mapped generates multimedia keystrokes\n");
		exit(2);
	}

	if (argc == 3) {
		if (std::string(argv[2]) == std::string("--tapxyz")) {
			tapxyz = true;
		} else if (std::string(argv[2]) == std::string("--allxyz")) {
			dumpxyz = true;
		} else if (std::string(argv[2]) == std::string("--mapped")) {
			mapped = true;
		} else {
			printf("unknown option %s", argv[2]);
			exit(2);
		}
	}

	MGC mgc(tapxyz, dumpxyz, mapped, argv[1]);

	sig.sa_handler = mgc_handler;
	sigaction(SIGINT, &sig, NULL);
	sigaction(SIGKILL, &sig, NULL);

	mgc.detectGesture();

	return 0;
}
