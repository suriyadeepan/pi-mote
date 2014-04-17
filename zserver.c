/*
 *   Author : Suriyadeepan R
 *   TAGS:
 *   PWM based motor control,
 *   Multithreading,
 *   Socket Communication,
 *   Multiple Client socket Handling,
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h> 

#include <sys/socket.h>
#include <netinet/in.h>

#include <pthread.h>

#include <string.h>
#include <unistd.h>

#include <wiringPi.h>
#include <wiringSerial.h>
#include <math.h>

#define DELAY_TIME 300
#define AVG_SPEED 0
#define KONSTANT 0.8

// Motor Pins
#define MOT1 7 
#define PIN1 0
#define MOT2 3
#define PIN2 2

// deviation
//  probably useless now
int devi = 0;

// Info on nodes
int node_id = 4;
// Destination node coordinates
int des_x = 1;
int des_y = 1;
// Src Node coordinates
int src_x = 2;
int src_y = 2;

// String obtained serially 
//  from TelosB
char serialDat[7];

// AVG speed for each motor
int avg_speed1 = 30;
int avg_speed2 = 30;

// delAngle => difference in angle => ( cAngle - gAngle )
//  deviation from ideal
int delAngle=0;

// PWM generation threads
pthread_t pwm1;
pthread_t pwm2;

// PWM for Motor 1
void *pwmGen1(){

				// send pulses continuously to motor
        while(1){
            softPwmWrite(MOT1,avg_speed1);
						//softPwmWrite(MOT1,80);
				}

				// exit thread
        pthread_exit(NULL);
}

// PWM for Motor 2
void *pwmGen2(){

				// send pulses continuously to motor
        while(1){
						// a value of '2' is added to prevent the bot to 
						//  be one one sided
						//   need a better generic way to solve this 
            softPwmWrite(MOT2,avg_speed2);
						//softPwmWrite(MOT2,80);
        }

        pthread_exit(NULL);
}


// global vars for socket communication
    int sockfd, newsockfd,newsockfd1, portno, clilen,clilen1;
    struct sockaddr_in serv_addr, cli_addr,cli_addr1;
 
// buffers for storing data from socket
    char buffer[20],buffer1[20];

    int  n;

		// Ideal angle and current angle
    int gAngle = -1;
    int cAngle = 290;


// ***
// initialization for all communications...
// ***
void initCompass(){

      listen(sockfd,5);
      clilen = sizeof(cli_addr);
        
     /* Accept actual connection from the client */
    newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, 
                                &clilen);
    if (newsockfd < 0) 
    {
        perror("ERROR on accept");
        exit(1);
    }

       
}

void initComm(){

    /* First call to socket() function */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
    {
        perror("ERROR opening socket");
        exit(1);
    }

int on = 0;
int status = 0;

        status = setsockopt(sockfd, SOL_SOCKET,
              SO_REUSEADDR,
              (const char *) &on, sizeof(on));

    /* Initialize socket structure */
//    setsockopt(SO_REUSEADDR);
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = 5004;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
 
 status = setsockopt(sockfd, SOL_SOCKET,
              SO_REUSEADDR,
              (const char *) &on, sizeof(on));

 
    /* Now bind the host address using bind() call.*/
    if (bind(sockfd, (struct sockaddr *) &serv_addr,
                          sizeof(serv_addr)) < 0)
    {
         perror("ERROR on binding");
         exit(1);
    }

}


void getNewSocket(){

        printf("\nInside getNewSocket()...\n");
        listen(sockfd,5);
        clilen1 = sizeof(cli_addr);
        
       /* Accept actual connection from the client */
        newsockfd1 = accept(sockfd, (struct sockaddr *)&cli_addr1, 
                                &clilen1);
        if (newsockfd1 < 0){

                perror("ERROR on accept");
                exit(1);
        }
}

         
// the communication thread
//  continue to obtain angle from socket
//   (ie) from compass
void *continueComm(){

while(1){

    bzero(buffer,20);
    n = read( newsockfd,buffer,20 );
    if (n < 0)
    {
        perror("ERROR reading from socket");
        exit(1);
    }

	// convert to int
	gAngle = atoi(buffer);

	// print direction from compass
	//printf("%d\n",gAngle);

        if(abs(gAngle - cAngle) < abs(360 + gAngle - cAngle) )
                delAngle = gAngle - cAngle;

        else 
                delAngle = (360 + gAngle - cAngle);

        
        printf("%d %d %d\n",delAngle,avg_speed1,avg_speed2);
        
      //  if(node_id != -1)
       //         printf("Node : %d ; Dest : (%d,%d)\n",node_id,des_x,des_y);
			 //

				
				// Based on difference in angle adjust the speed of motors
        if( delAngle < 0 && abs(delAngle) > 5 ) 
        {
                avg_speed1 = KONSTANT * abs(delAngle);
                avg_speed2 = 0;

                if(avg_speed1 < 25)
                        avg_speed1 = 25;

        }

        else if( delAngle > 0 && abs(delAngle) > 5 ) 
        {
                avg_speed2 = KONSTANT * abs(delAngle);
                avg_speed1 = 0;

                if(avg_speed2 < 25)
                        avg_speed2 = 25;

        }

        else
        {
								// if src location = destionation location
								//  we have reached the dest.
								//   stop movin
                if( src_x == des_x && src_y == des_y ){
                        avg_speed1=0;
                        avg_speed2=0;
                }

                else{
                        avg_speed1=60;
                        avg_speed2=60;
                }
        }
   
}
	// exit thread
   pthread_exit(NULL); 

}// end of communication thread...


/*
	*** Motor control functions ***


*/

// completely stop the motors
void stop_it(int delay_step){

digitalWrite (7, 0) ;       // Off
digitalWrite (0, 0) ;       // Off
digitalWrite (3, 0) ;       // Off
digitalWrite(2,0);

delay(delay_step);

digitalWrite (7, 0) ;       // Off
digitalWrite (0, 0) ;       // Off
digitalWrite (3, 0) ;       // Off
digitalWrite (2, 0) ;       // Off
                                        
}


// Initilialize PWM
//  start the pwm generation threads
void initPwm(){

        softPwmCreate(MOT1,0,95);
        softPwmCreate(MOT2,0,95);

        pthread_create(&pwm1,NULL,pwmGen1,NULL);
        pthread_create(&pwm2,NULL,pwmGen2,NULL);
      

}


// read from serial port
//  (ie) from telosB
void* readSerial ()
{
  int fd ;

  if ((fd = serialOpen ("/dev/ttyUSB0", 115200)) < 0)
  {
//    fprintf (stderr, "Unable to open serial device: %s\n", strerror (errno)) ;
 //   return 1 ;

        printf("Cant open serial device file...");
        return;
  }

// Loop, getting and printing characters

  for (;;)
  {

        char ch1 = serialGetchar(fd);

        if(ch1 != ' '){

                if( (int)ch1 == 100 ){

                int i = 0;

                        // get the next 7 charecters
                        while( (int)ch1!= 0 ){


                                ch1 = serialGetchar(fd);
                                
                                serialDat[i] = ch1;

                                //printf("%c:%d ",ch1,i);

                                i++;

                                fflush (stdout) ;
                        }
/*     
                        for(i=0;i<7;i++)
                                printf("%c ",serialDat[i]);
*/
                    //    printf("\n");


                }

        }
            fflush (stdout) ;

  }

  pthread_exit(NULL);

}

// MAIN
int main( int argc, char *argv[] )
{
        // initialize WiringPi
        if (wiringPiSetup () == -1)
                return 1 ;

			  // GPIO pin setup	
        pinMode(PIN1,OUTPUT);
        pinMode(PIN2,OUTPUT);
        pinMode(MOT1,OUTPUT);
        pinMode(MOT2,OUTPUT);

        digitalWrite(PIN1,0);
        digitalWrite(PIN2,0);


        printf("\n>> GPIO Initialized!\n");
       


        //initGPIO();

    // initiate communication
    initComm();

    printf("\n>> Comm Initialized!\n");

		// initiate compass
    initCompass();
    printf("\n>> Compass Initialized!\n");

		// continue communication
    pthread_t continueComm00;
    pthread_create(&continueComm00, NULL, continueComm, NULL);

    printf("\n>> ContinueComm thread started!\n");

		// initialize PWM
    initPwm();
    printf("\n>> PWM initialized!");

		// initialize serial comm
    pthread_t serialComm;
    pthread_create(&serialComm, NULL, readSerial, NULL);

		printf("\n>> Serial Comm thread started\n");



		// based on the data obtained from telosB - src and dest
		//  set the direction of the bot 
		//   Code section below is a bit inefficient and complicated
		//    FIX : need to be simplified
   while(1){
       
        int i;

        if(serialDat[0] == '0'){
                src_x = serialDat[2] - '0';
                src_y = serialDat[4] - '0';

//                devi = 18;

        }

        else if(serialDat[0] > '0'){
                des_x = serialDat[2] - '0';
                des_y = serialDat[4] - '0';
        }

        
        float del_xy ;
        float del_x,del_y;

        if(src_x != -1 && src_y != -1 && des_x != -1 && des_y != -1){
       
                del_x = des_x - src_x;
                del_y = des_y - src_y;
              
        if(des_x - src_x != 0){
                del_xy = (float) del_y / del_x ;
        }

        

        cAngle = 90 + devi -  (atan( del_xy) * 180 * 7 / 22) ;

        if(del_x > 0 && del_y < 0)
                cAngle = 90 + cAngle ;

        else if ( del_x < 0 && del_y < 0)
                cAngle = 180 + cAngle;

        else if (del_x < 0 && del_y > 0)
                cAngle = 180 + cAngle;

        else if ( del_y ==0)
                if(del_x < 0)
                        cAngle = 270 + devi;

         if ( del_x == 0){
                if(del_y > 0)
                        cAngle = 25;
                if(del_y < 0)
                        cAngle = 180 + devi;
        }
 
        //printf("node : %c x : %c y : %c",serialDat[0],serialDat[2],serialDat[4]);
        //printf("Current Loc : (%d,%d) ; Destination : (%d,%d) ; Angle : %d\n",src_x,src_y,des_x,des_y,cAngle);
        //printf("\n");

//        printf("Angle : %d; delAngle : %d\n",cAngle,delAngle);

        }

        else if(src_x == des_x && src_y == des_y){
                avg_speed1 = 0;
                avg_speed2 = 0;
        }

        else
        {
                avg_speed1 = 0;
                avg_speed2 = 0;
        }


        delay(1000);
   
   }

	 // kill all threads
    pthread_exit(NULL);

    return 0;



}// END OF MAIN
