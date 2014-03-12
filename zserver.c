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
#define KONSTANT 1.35

#define MOT1 7 // LEFT TURN (ie) RIGHT MOTOR
#define PIN1 0
#define MOT2 3 // RIGHT TURN (ie) LEFT MOTOR
#define PIN2 2

// deviation
int devi = 30;

int node_id = 4;
int des_x = -1;
int des_y = -1;

int src_x = -1;
int src_y = -1;

char serialDat[7];

int avg_speed1 = 40;
int avg_speed2 = 40;

int delAngle=0;

pthread_t pwm1;
pthread_t pwm2;


void *pwmGen1(){

        while(1)
            softPwmWrite(MOT1,avg_speed1);

        pthread_exit(NULL);
}

void *pwmGen2(){

        while(1){
            softPwmWrite(MOT2,avg_speed2+2);
        }

        pthread_exit(NULL);
}


// global vars
    int sockfd, newsockfd,newsockfd1, portno, clilen,clilen1;
    char buffer[20],buffer1[20];
//    char gBuff[15];
    struct sockaddr_in serv_addr, cli_addr,cli_addr1;
    int  n;

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

    printf("\nNewsockfd : %d",newsockfd);
       
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
void *continueComm(){

while(1){

    bzero(buffer,20);
    n = read( newsockfd,buffer,20 );
    if (n < 0)
    {
        perror("ERROR reading from socket");
        exit(1);
    }

	gAngle = atoi(buffer);

        if(abs(gAngle - cAngle) < abs(360 + gAngle - cAngle) )
                delAngle = gAngle - cAngle;

        else 
                delAngle = (360 + gAngle - cAngle);

        
//        printf("%d\n",delAngle);
        
      //  if(node_id != -1)
       //         printf("Node : %d ; Dest : (%d,%d)\n",node_id,des_x,des_y);

        if( delAngle > 0 && abs(delAngle) > 5 ) 
        {
                avg_speed1 = KONSTANT*delAngle;
                avg_speed2 = 0;

                if(avg_speed1 < 25)
                        avg_speed1 = 25;

        }

        else if( delAngle < 0 && abs(delAngle) > 5 ) 
        {
                avg_speed2 = KONSTANT * abs(delAngle);
                avg_speed1 = 0;

                if(avg_speed2 < 25)
                        avg_speed2 = 25;

        }

        else
        {
                if( src_x == des_x && src_y == des_y ){
                        avg_speed1=0;
                        avg_speed2=0;
                }

                else{
                        avg_speed1=35;
                        avg_speed2=35;
                }
        }
   
}
   pthread_exit(NULL); 

}// end of communication thread...


/*
	*** Motor control functions ***


*/

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


void initPwm(){


        softPwmCreate(MOT1,0,95);
        softPwmCreate(MOT2,0,95);

        pthread_create(&pwm1,NULL,pwmGen1,NULL);
        pthread_create(&pwm2,NULL,pwmGen2,NULL);
      

}


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

int main( int argc, char *argv[] )
{
        // initialize WiringPi
        if (wiringPiSetup () == -1)
                return 1 ;
 
        pinMode(PIN1,OUTPUT);
        pinMode(PIN2,OUTPUT);
        pinMode(MOT1,OUTPUT);
        pinMode(MOT2,OUTPUT);

        digitalWrite(PIN1,0);
        digitalWrite(PIN2,0);


        printf("\nGPIO Initialized!\n");
       


        //initGPIO();

    // initiate communication
    initComm();

    printf("\nComm Initialized!\n");

    initCompass();
    printf("\nCompass Initialized!\n");

    pthread_t continueComm00;
    pthread_create(&continueComm00, NULL, continueComm, NULL);

    printf("\nContinueComm thread started!\n");

    initPwm();
    printf("pwm initialized!");

    pthread_t serialComm;
    pthread_create(&serialComm, NULL, readSerial, NULL);



   while(1){
       
        int i;

        if(serialDat[0] == '0'){
                src_x = serialDat[2] - '0';
                src_y = serialDat[4] - '0';

                devi = 18;

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
        printf("Current Loc : (%d,%d) ; Destination : (%d,%d) ; Angle : %d\n",src_x,src_y,des_x,des_y,cAngle);
        printf("\n");

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

    pthread_exit(NULL);

        return 0;



}