#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/watchdog.h>

#define WATCHDOG "/dev/watchdog"
#define WATCHDOG0 "/dev/watchdog0"
#define WATCHDOG1 "/dev/watchdog1"

int main(int argc, char* argv[])
{
    int preset_timeout;
    int config_timeout;
    int timeleft;
    int testcycle = 0;
    int dummy;
    int devnum = 0;
    int wfd;
    int config_testcycle;
    int flag;
    char stopmagic = 'V';

    devnum = atoi(argv[1]);
    switch (devnum) {
    case 0:
        wfd = open(WATCHDOG0, O_RDWR);
        if (wfd == -1) {
	    printf("Unable to open %s \n", WATCHDOG0);
	    return -1;
        }       
        break;
    case 1:
        wfd = open(WATCHDOG1, O_RDWR);
        if (wfd == -1) {
	    printf("Unable to open %s \n", WATCHDOG1);
	    return -1;
        }
	break;
    case 2:
        wfd = open(WATCHDOG, O_RDWR);
        if (wfd == -1) {
	    printf("Unable to open %s \n", WATCHDOG);
	    return -1;
        }
	break;
    default:
	printf("watchdog devnum error \n");
	return -1;
    }

    if (ioctl(wfd, WDIOC_GETTIMEOUT, &preset_timeout) == -1) {
	printf("Watchdog gettimeout ioctl error %s\n", strerror(errno));
	if (write(wfd, &stopmagic, sizeof(stopmagic)) != sizeof(stopmagic)) {
	    printf("Watchdog magic stop write error \n");
	}
	close(wfd);
	return -1;
    }
    printf("Watchdog preset timeout is %d \n", preset_timeout);

    config_timeout = atoi(argv[2]);
    if (config_timeout > 178) {
        printf("Watchdog config timeout maximum is 178sec in single mode\n");
	if (write(wfd, &stopmagic, sizeof(stopmagic)) != sizeof(stopmagic)) {
	    printf("Watchdog magic stop write error \n");
	}
	close(wfd);
	return -1;
    }
    if (ioctl(wfd, WDIOC_SETTIMEOUT, &config_timeout) == -1) {
        printf("Watchdog settimeout ioctl error %s\n", strerror(errno));
	if (write(wfd, &stopmagic, sizeof(stopmagic)) != sizeof(stopmagic)) {
	    printf("Watchdog magic stop write error \n");
	}
	close(wfd);
        return -1;
    }
    printf("Watchdog config timeout %d \n", config_timeout);
    
    if (ioctl(wfd, WDIOC_GETTIMELEFT, &timeleft) == -1) {
        printf("Watchdog gettimeleft ioctl error %s\n", strerror(errno));
	if (write(wfd, &stopmagic, sizeof(stopmagic)) != sizeof(stopmagic)) {
	    printf("Watchdog magic stop write error \n");
	}
	close(wfd);
	return -1;
    }
    printf("Watchdog time left is %d \n", timeleft);

    if (!strncasecmp(argv[3], "-oneshot", 8)) {
	printf("Watchdog one time timeout test\n");
	sleep(config_timeout / 2);
	while(1); 
    }

    if (!strncasecmp(argv[3], "-onerel", 7)) {
	printf("Watchdog one time release test\n");
	flag = WDIOS_DISABLECARD;
	if (ioctl(wfd, WDIOC_SETOPTIONS, &flag) == -1) {
	    printf("Watchdog disable option error %s\n", strerror(errno));
	    if (write(wfd, &stopmagic, sizeof(stopmagic)) != sizeof(stopmagic)) {
		printf("Watchdog magic stop write error \n");
	    }
	    close(wfd);
	    return -1;
	}
	sleep(config_timeout + 3);
        close(wfd);
	return 0;
    }

    if (!strncasecmp(argv[3], "-cyc", 4)) {
	config_testcycle = atoi(argv[4]);
    } else {
	config_testcycle = 200;
    }

    /* test disable and reenable option */
    flag = WDIOS_DISABLECARD;
    if (ioctl(wfd, WDIOC_SETOPTIONS, &flag) == -1) {
        printf("Watchdog disable option error %s\n", strerror(errno));
        if (write(wfd, &stopmagic, sizeof(stopmagic)) != sizeof(stopmagic)) {
            printf("Watchdog magic stop write error \n");
	}
	close(wfd);
	return -1;
    }
    sleep(3);
    flag = WDIOS_ENABLECARD;
    if (ioctl(wfd, WDIOC_SETOPTIONS, &flag) == -1) {
        printf("Watchdog enable option error %s\n", strerror(errno));
        if (write(wfd, &stopmagic, sizeof(stopmagic)) != sizeof(stopmagic)) {
            printf("Watchdog magic stop write error \n");
	}
	close(wfd);
	return -1;
    }
    
    while (1) {
        if (ioctl(wfd, WDIOC_KEEPALIVE, &dummy) == -1) {
	    printf("Watchdog keeplive ioctl error %s\n", strerror(errno));
	    if (write(wfd, &stopmagic, sizeof(stopmagic)) != sizeof(stopmagic)) {
		printf("Watchdog magic stop write error \n");
	    }
	    close(wfd);
	    return -1;
	}
	sleep(config_timeout / 2);
	printf("Watchdog test cycle is %d \n", testcycle);
	if (testcycle++ > config_testcycle) {
	    printf("Watchdog test cycle %d finish \n", config_testcycle);
	    if (write(wfd, &stopmagic, sizeof(stopmagic)) != sizeof(stopmagic)) {
		printf("Watchdog magic stop write error \n");
	    }
	    close(wfd);
	    break;
	}
    }

    return 0;
}
