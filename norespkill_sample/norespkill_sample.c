#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#define NORESPKILL "/dev/norespkill"

#define NORESPKILL_IOC_MAGIC 'N'
#define NORESPKILL_IOCSTIME _IOW(NORESPKILL_IOC_MAGIC, 1, int)
#define NORESPKILL_IOCGTIME _IOR(NORESPKILL_IOC_MAGIC, 2, int)

int main(int argc, char* argv[])
{
    int ops = 0;
    int timeconfig = 0;
    int timecurr = 0;
    int fd;

    fd = open(NORESPKILL, O_RDWR);
    if (fd == -1) {
        printf("Unable to open %s \n", NORESPKILL);
        return -1;
     }       
    
    ops = atoi(argv[1]);

    switch (ops) {
    case 1: /* set time */
	timeconfig = atoi(argv[2]); /* 10min granularity */
	printf(" norespkill time config is %d \n", timeconfig);
        if (ioctl(fd, NORESPKILL_IOCSTIME, &timeconfig) != 0) {
	    printf("norespkill set time ioctl error %s\n", strerror(errno));
	    close(fd);
	    return -1;
	}
        break;

    case 2: /* get current time */
        if (ioctl(fd, NORESPKILL_IOCGTIME, &timecurr) != 0) {
	    printf("norespkill get time ioctl error %s\n", strerror(errno));
	    close(fd);
	    return -1;
	}
        printf("norespkill curr time is %d \n", timecurr);
	break;
    
    default:
	printf("norespkill ops error %d \n", ops);
	close(fd);
	return -1;
    }
    close(fd);
    return 0;
}
