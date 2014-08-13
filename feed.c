#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>

#define DRV8830_SLAVE_1 0x63
#define DRV8830_SLAVE_2 0x64
#define DRV8830_SLAVE_3 0x65

#define TOUCH1_SENSOR 0
#define TOUCH2_SENSOR 1
#define LIGHT1_SENSOR 7
#define LIGHT2_SENSOR 2

#define DEBUG 1

void startMotor(int fd,uint8_t accel,int direction) {
  uint8_t command;
  command = (accel << 2) | (direction > 0 ? 0b10 : 0b01);
  wiringPiI2CWriteReg8(fd,0x00,command);
}

void stopMotor(int fd) {
  wiringPiI2CWriteReg8(fd,0x00,0x00);
  wiringPiI2CWriteReg8(fd,0x00,0x03);
}


/* main */
int main(int argc,char *argv[]) {
  int feed_d;
  uint8_t accel  = 0x3f;
  int result;
  int stop = 0;
  int time = 1000;

  /* get opt */
  while((result=getopt(argc,argv,"a:t:s")) != -1) {
    switch(result){
    case 'a': // Accel
      accel = (uint8_t)strtol(optarg,NULL,0);
#ifdef DEBUG
      fprintf(stderr,"%c %x\n",result,accel);
#endif
      break;
    case 't': // Time in millisec
      time = (int)strtol(optarg,NULL,0) ;
#ifdef DEBUG
      fprintf(stderr,"%c time = %d\n",result,time);
#endif
      break;
    case 's': // force stop
      stop = 1;
      break;
    }
  }

  if (accel > 0x3f || accel < 0x06) {
    fprintf(stderr,"Invalid range of accel: 0x06 - 0x3f\n");
    return -1;
  }

  if (time <= 0 || time > 5000) {
    fprintf(stderr,"Invalid range of time: 1ms ~ 5000ms\n");
    return -1;
  }

  if (wiringPiSetup() == -1) {
    fprintf(stderr,"Cannot open wiring Pi\n");
    return -1;
  }

  feed_d = wiringPiI2CSetup(DRV8830_SLAVE_3) ;
  if (feed_d < 0) {
    fprintf(stderr,"Cannot open DRV8830_SLAVE_3\n");
    return -1;
  }

  if (stop == 1) {
    stopMotor(feed_d);
    printf("STOP\n");
    return 0;
  }

  /* clear fault regisiter */
  wiringPiI2CWriteReg8(feed_d,0x01,0x00);
  /* break */
  wiringPiI2CWriteReg8(feed_d,0x00,0x03);
  // start motor 
  startMotor(feed_d,accel,1);
  delay(time);
  stopMotor(feed_d);
  wiringPiI2CWriteReg8(feed_d,0x00,0x00);
  delay(500);
  startMotor(feed_d,accel,-1);
  delay(time/2);
  stopMotor(feed_d);
  return 0;
}
