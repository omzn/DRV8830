#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>

#define DRV8830_SLAVE_1 0x63
#define DRV8830_SLAVE_2 0x64
#define DRV8830_SLAVE_3 0x65

#define LEFT_DIR -1
#define UP_DIR 1

#define TOUCH1_SENSOR 0
#define TOUCH2_SENSOR 1
#define LIGHT1_SENSOR 7
#define LIGHT2_SENSOR 2

volatile static int v_pos;
volatile static int h_pos;
volatile static int v_stop;
volatile static int h_stop;
volatile static int lift_d;
volatile static int move_d;

void startMotor(int fd,uint8_t accel,int direction) {
  uint8_t command;
  command = (accel << 2) | (direction > 0 ? 0b10 : 0b01);
  wiringPiI2CWriteReg8(fd,0x00,command);
}

void stopMotor(int fd) {
  wiringPiI2CWriteReg8(fd,0x00,0x00);
  wiringPiI2CWriteReg8(fd,0x00,0x03);
}

/* Interrupt handlers */
void doNothing(void) {
}

void onHTouch(void) {
  wiringPiISR(TOUCH1_SENSOR, INT_EDGE_RISING,  &doNothing);
#ifdef DEBUG
  fprintf(stderr,"INT H_Touch!\n");
#endif
  --h_pos;
  h_stop = 1;
}

void onHLight(void) {
#ifdef DEBUG
  fprintf(stderr,"INT H_Light!\n");
#endif
  --h_pos;
}

void onVTouch(void) {
  wiringPiISR(TOUCH2_SENSOR, INT_EDGE_RISING,  &doNothing);
#ifdef DEBUG
  fprintf(stderr,"INT V_Touch!\n");
#endif
  --v_pos;
  v_stop = 1;
}

void onVLight(void) {
#ifdef DEBUG
  fprintf(stderr,"INT V_Light!\n");
#endif
  --v_pos;
}

/* main */
int main(int argc,char *argv[]) {
  uint8_t accel  = 0x3f;
  int h_direction  = 0;
  int v_direction  = 0;
  int stop       = 0;
  int result;

  /* get opt */
  while((result=getopt(argc,argv,"a:r:l:u:d:s")) != -1) {
    switch(result){
    case 'a': // Accel
      accel = (uint8_t)strtol(optarg,NULL,0);
#ifdef DEBUG
      fprintf(stderr,"%c %x\n",result,accel);
#endif
      break;
    case 'r': // Left n positions
      h_direction = (int)strtol(optarg,NULL,0) * LEFT_DIR * -1;
#ifdef DEBUG
      fprintf(stderr,"%c H direction = %d\n",result,h_direction);
#endif
      break;
    case 'l': // Right n positions
      h_direction = (int)strtol(optarg,NULL,0) * LEFT_DIR;
#ifdef DEBUG
      fprintf(stderr,"%c H direction = %d\n",result,h_direction);
#endif
      break;
    case 'u': // Up n positions
      v_direction = (int)strtol(optarg,NULL,0) * UP_DIR;
#ifdef DEBUG
      fprintf(stderr,"%c V direction = %d\n",result,v_direction);
#endif
      break;
    case 'd': // Down n positions
      v_direction = (int)strtol(optarg,NULL,0) * UP_DIR * -1;
#ifdef DEBUG
      fprintf(stderr,"%c V direction = %d\n",result,v_direction);
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

  /* Should I do anything? */
  if (h_direction == 0 && v_direction== 0) {
    printf("H:0\n");
    printf("V:0\n");
    return 0;
  }

  if (wiringPiSetup() == -1) {
    fprintf(stderr,"Cannot open wiring Pi\n");
    return -1;
  }

  lift_d = wiringPiI2CSetup(DRV8830_SLAVE_1) ;
  if (lift_d < 0) {
    fprintf(stderr,"Cannot open DRV8830_SLAVE_1\n");
    return -1;
  }

  move_d = wiringPiI2CSetup(DRV8830_SLAVE_2) ;
  if (move_d < 0) {
    fprintf(stderr,"Cannot open DRV8830_SLAVE_2\n");
    return -1;
  }

  if (stop == 1) {
    stopMotor(lift_d);
    stopMotor(move_d);
    printf("STOP\n");
    return 0;
  }

  h_pos = abs(h_direction);
  v_pos = abs(v_direction);
  h_stop = h_pos == 0 ? 2 : 0;
  v_stop = v_pos == 0 ? 2 : 0;

  if (h_direction != 0) {
    /* set interrupt for sensors */
    /*
      in my implementation, 
        - the light sensor gives Low when sensored
        - the touch sensor gives High when touched
      that's why triggers varies.
     */
    wiringPiISR(LIGHT1_SENSOR, INT_EDGE_FALLING, &onHLight);
    wiringPiISR(TOUCH1_SENSOR, INT_EDGE_RISING,  &onHTouch);

    /* clear fault regisiter */
    wiringPiI2CWriteReg8(move_d,0x01,0x00);
    /* break */
    wiringPiI2CWriteReg8(move_d,0x00,0x03);
    // start motor 
    startMotor(move_d,accel,h_direction * LEFT_DIR);
  }

  if (v_direction != 0) {
    /* set interrupt for sensors */
    wiringPiISR(LIGHT2_SENSOR, INT_EDGE_FALLING, &onVLight);
    wiringPiISR(TOUCH2_SENSOR, INT_EDGE_RISING,  &onVTouch);

    /* clear fault regisiter */
    wiringPiI2CWriteReg8(lift_d,0x01,0x00);
    /* float */
    wiringPiI2CWriteReg8(lift_d,0x00,0x00);
    /* break */
    wiringPiI2CWriteReg8(lift_d,0x00,0x03);
    // start motor 
    startMotor(lift_d,accel,v_direction * UP_DIR);
  }

  // loop 
  while(h_stop < 2 || v_stop < 2) {
    if (h_stop == 1) {
#ifdef DEBUG
      fprintf(stderr,"A h_stop = %d\n",h_stop);
#endif
      stopMotor(move_d);
      h_stop = 3;
    }
    if (h_pos == 0 && h_stop == 0) {
#ifdef DEBUG
      fprintf(stderr,"B h_stop = %d\n",h_stop);
#endif
      stopMotor(move_d);
      h_stop = 2;
    }
    if (v_stop == 1) {
#ifdef DEBUG
      fprintf(stderr,"C v_stop = %d\n",v_stop);
#endif
      stopMotor(lift_d);
      v_stop = 3;
    }
    if (v_pos == 0 && v_stop == 0) {
#ifdef DEBUG
      fprintf(stderr,"D v_stop = %d\n",v_stop);
#endif
      stopMotor(lift_d);
      v_stop = 2;
    }
  } 
  
#ifdef DEBUG
  fprintf(stderr,"h_stop = %d\n",h_stop);
#endif

  printf("%d %d\n",(abs(h_direction)-h_pos)*(h_direction>0?1:-1),(abs(v_direction)-v_pos)*(v_direction>0?1:-1));

  if (h_stop == 3) {
    startMotor(move_d,accel,-h_direction * LEFT_DIR);
    delay(300);
    stopMotor(move_d);
  }
  if (v_stop == 3) {
    startMotor(lift_d,accel,-v_direction * UP_DIR);
    delay(600);
    stopMotor(lift_d);
  }

  return 0;
}
