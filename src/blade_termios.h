#ifndef BLADE_COMPAT_TERMIOS_H
#define BLADE_COMPAT_TERMIOS_H

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>

#ifndef	__MINGW32_MAJOR_VERSION
#pragma comment(lib, "Ws2_32.lib")
#endif

ssize_t read_serial(int fd, void* buffer, size_t count);
ssize_t write_serial(int fd, const void* buffer, size_t count);
int open_serial(const char* portname, int opt);
int close_serial(int fd);
int select_serial(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);

#define read read_serial
#define write write_serial
#define open open_serial
#define close close_serial
#define select select_serial

// i_flags
#define INPCK		0x00004000 	
#define IGNPAR	0x00001000	
#define PARMRK	0x00040000	
#define ISTRIP	0x00008000	
#define IGNBRK	0x00000400	
#define BRKINT	0x00000100	
#define IGNCR		0x00000800	
#define ICRNL		0x00000200	
#define INLCR		0x00002000	
#define IXOFF		0x00010000	
#define IXON		0x00020000	

// l_flags
#define ICANON	0x00001000	
#define ECHO		0x00000100	
#define ECHOE		0x00000200	
#define ECHOK		0x00000400	
#define ECHONL	0x00000800	
#define ISIG		0x00004000	
#define IEXTEN	0x00002000	
#define NOFLSH	0x00008000	
#define TOSTOP	0x00010000	

//c_flags
#define CSTOPB	0x00001000	
#define PARENB	0x00004000	
#define PARODD	0x00008000	
#define CSIZE		0x00000c00	
#define CS5			0x00000000	
#define CS6			0x00000400	
#define CS7			0x00000800	
#define CS8			0x00000c00	
#define CLOCAL 	0x00000000 	
#define CREAD 	0x00000000 

//o_flags
#define OPOST	0x00000100 	

// cc flags
#define VEOF 0
#define VEOL 1
#define VERASE 2
#define VINTR 3
#define VKILL 4
#define VMIN 5 
#define VQUIT 6
#define VSTART 7
#define VSTOP 8
#define VSUSP 9
#define VTIME 10


#define TIOMBIC DTR_CONTROL_DISABLE
#define TIOMBIS DTR_CONTROL_ENABLE
#define CRTSCTS RTS_CONTROL_ENABLE

#define NCCS 11

// baud speeds
#define B110 CBR_110
#define B300 CBR_300
#define B600 CBR_600
#define B1200 CBR_2400
#define B2400 CBR_2400
#define B4800 CBR_4800
#define B9600 CBR_9600
#define B19200 CBR_19200
#define B38400 CBR_38400
#define B57600 CBR_57600
#define B115200 CBR_115200

#define TCSANOW 0
#define TCSADRAIN 1
#define TCSAFLUSH 2

#define TCIFLUSH 0
#define TCOFLUSH 1
#define TCIOFLUSH 2

#define TCOOFF 0
#define TCOON 1
#define TCIOFF 2
#define TCION 3

typedef unsigned tcflag_t; 
typedef unsigned cc_t; 
typedef unsigned speed_t; 

typedef struct termios {
  tcflag_t c_iflag; 
  tcflag_t c_oflag; 
  tcflag_t c_cflag; 
  tcflag_t c_lflag; 
  speed_t  c_ispeed;
  speed_t  c_ospeed;
  cc_t c_cc[NCCS];
} termios;

int tcgetattr(int fd, struct termios *termios_p);
int tcsetattr(int fd, int optional_actions, const struct termios *termios_p);
int tcflush(int fd, int queue_selector);
int tcflow(int fd, int action);
void cfmakeraw(struct termios *termios_p);
speed_t cfgetispeed(const struct termios *termios_p);
speed_t cfgetospeed(const struct termios *termios_p);
int cfsetispeed(struct termios *termios_p, speed_t speed);
int cfsetospeed(struct termios *termios_p, speed_t speed);
int cfsetspeed(struct termios * termios_p, speed_t speed);

//get Handle out of the COM structure
HANDLE getHandle();

#endif

#endif // BLADE_COMPAT_TERMIOS_H
