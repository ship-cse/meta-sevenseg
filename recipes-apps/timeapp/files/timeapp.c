#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#define MAJOR_NUM 202

/* ioctl for setting the mode */
//TODO this needs to move to a .h that is shared with user space code
#define IOCTL_SET_MODE _IOW(MAJOR_NUM, 0, uint32_t)
#define IOCTL_SET_TMPD _IOW(MAJOR_NUM, 1, uint32_t)
#define IOCTL_SET_COLPD _IOW(MAJOR_NUM, 2, uint32_t)

int main(int argc, char **argv)
{

   time_t now;

   time(&now);
   struct tm *tm_now = gmtime(&now);

   printf("%02d:%02d:%02d\n", 
	tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec);


  //TODO ioctl set the mode
  int fid = open("/dev/seven",O_RDWR);
 
  int rc;

  printf("Updating IOCTL\n");

  rc = ioctl(fid, IOCTL_SET_MODE, 3);
  rc = ioctl(fid, IOCTL_SET_TMPD, 100000000);
  rc = ioctl(fid, IOCTL_SET_COLPD, 100000000);

  //TODO set the time
  char msg[32];
  sprintf(msg,"0x%x%x%x%x%x%x",  
     tm_now->tm_hour / 10, tm_now->tm_hour % 10,
     tm_now->tm_min / 10, tm_now->tm_min % 10,
     tm_now->tm_sec / 10, tm_now->tm_sec % 10);


  printf("Writing: (%s)", msg);

  write(fid, msg, strlen(msg)+1);


  close(fid);


  while(1) {

     fid = open("/dev/seven",O_RDWR);
     read(fid, msg, 1023);
     close(fid);
 

     printf("Read back: %s\n", msg);

     char *ptr = strstr(msg,"time=");
     if (ptr == NULL) {
       printf("Error - did not get expected string from seven module\n");
       exit(-1);
     }

     // ptr points at "time=", the actual vals are 5 bytes later
     int hour, min, sec;
     sscanf(ptr+5,"%d:%d:%d", &hour, &min, &sec);
     
     time(&now);
     gmtime(&now);

     printf("%d %d / %d %d / %d %d = %d\n", 
	hour,  tm_now->tm_hour,
	min, tm_now->tm_min,
        sec, tm_now->tm_sec,
        (int) fabs(sec - tm_now->tm_sec));

     sleep(30);
  }

  close(fid);
}
 

