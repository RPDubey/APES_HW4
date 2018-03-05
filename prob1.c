/*******************************************************************************
   This code implements IPC mechanisms as required in Problem 1, HW4
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <mqueue.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PRINT_ERR(err) {printf("%s Error: %s\n",err, strerror(errno)); return -1;}

#define MY_MQ "/send_receive_mq"
#define PORT 8080
#define BUF_SIZE (4096) //max size for atomic read and write

typedef struct {
        uint8_t led_status;
        char message[BUF_SIZE - sizeof(uint8_t)];
} msg_struct;

//#define PIPES
//#define MSG_Q
//#define SHARED_MEM
//#define SOCKETS

int main(){

/*linux has Half duplex setting, thus file_descriptor[0] is always used for reading,file_descriptor[1] always used for writing*/


        msg_struct child_message[1];
        strcpy(child_message->message,"Hello from Child");
        child_message->led_status = 0;

        msg_struct parent_message[1];
        strcpy(parent_message->message,"Hello from Parent");
        parent_message->led_status = 1;

        msg_struct* message_buf = (msg_struct*)malloc(sizeof(msg_struct));
        if(message_buf==NULL) {printf("malloc Error: %s\n", strerror(errno)); return -1;}
        int ret;


/************************************PIPE***********************************/
#ifdef PIPES
        printf("PIPES Example\n");
        int file_descriptor[2];
        ret = pipe(file_descriptor);
        if (ret==-1) {printf("Error: %s\n", strerror(errno)); return -1;}
        switch(fork()) {

        case -1://error in fork call
                printf("Error: %s\n", strerror(errno)); return -1;
                break;
        case 0://child process(has 0 as return for fork) executes this
                printf("Child Process, PID:%d\n",getpid());
                //close(file_descriptor[0]);
                ret =write(file_descriptor[1],
                           child_message,
                           sizeof(child_message));
                if(ret == -1) {printf("Message Not sent\n"); return -1;}
                else printf("Message Sent from child\n");
                sleep(1);
                ret = read(file_descriptor[0],message_buf,BUF_SIZE);
                if(ret == -1) {printf("Message Not rxd\n"); return -1;}
                else printf("Messag rxd in child:%s;led_status:%d\n",
                            message_buf->message,message_buf->led_status);
                close(file_descriptor[0]); close(file_descriptor[1]);
                printf("Exit child\n");
                break;

        default://parent process(receives PID as return for fork) executes this
                printf("Parent Process, PID:%d\n",getpid());

                ret = read(file_descriptor[0],message_buf,BUF_SIZE);
                if(ret == -1) {printf("Message Not rxd\n"); return -1;}
                else printf("Messag rxd in parent:%s;led_status:%d\n",
                            message_buf->message,message_buf->led_status);

                ret = write(file_descriptor[1],parent_message,sizeof(parent_message));
                if(ret == -1) {printf("Message Not sent\n"); return -1;}
                else printf("Message Sent from parent\n");
                sleep(1);
                close(file_descriptor[0]); close(file_descriptor[1]);
                printf("Exit parent\n");


        }
#endif

/************************************QUEUS*************************************/
#ifdef MSG_Q
        printf("Messgae Que Example\n");
        /****variables used in each process*********/
        /***** after fork(), each process will have its own copy*****/
        struct mq_attr msgq_attr = {.mq_maxmsg = 10, //max # msg in queue
                                    .mq_msgsize = BUF_SIZE,//max size of msg in bytes
                                    .mq_flags = 0};
        mqd_t msgq;
        int msg_prio;
        int num_bytes;


        switch(fork()) {
        case -1://fork failed
                printf("Fork Error:%s\n",strerror(errno));
                return -1;
        case 0:        //child process receives message
                printf("Entering Child, PID:%d\n",getpid());
                sleep(1);//if child process runs first, this allows parent to be completed before
                msgq = mq_open(MY_MQ,
                               O_RDWR,
                               S_IRWXU, //non zero for O_CREAT flags
                               &msgq_attr);
                if(msgq < 0) { printf("mq_open Error:%s\n",strerror(errno)); return -1;}
                else printf("Messgae Que Opened by rxr\n");
                num_bytes=mq_receive(msgq,
                                     (char*)message_buf,
                                     BUF_SIZE,
                                     &msg_prio);
                if(num_bytes<0) {printf("mq_rcv Error:%s\n",strerror(errno)); return -1;}
                else {
                        printf("rxd in child:%s;led_status:%d\nlength:%d; priority:%d\n",
                               ((msg_struct*)message_buf)->message,
                               ((msg_struct*)message_buf)->led_status,
                               num_bytes,msg_prio);
                }
                num_bytes = mq_send(msgq,
                                    (const char*)child_message,
                                    sizeof(child_message),
                                    msg_prio);
                if(num_bytes<0) {printf("mq_send Error:%s\n",strerror(errno)); return -1;}
                else printf("Message sent from child\n");


                break;


        default://parent process sends the message
                printf("Parent Process, PID:%d\n",getpid());
                msg_prio = 30;
                msgq = mq_open(MY_MQ, //name
                               O_CREAT | O_RDWR,//flags
                               S_IRWXU, //mode-read,write and execute permission
                               &msgq_attr); //attribute
                if(msgq < 0) { printf("mq_open Error:%s\n",strerror(errno)); return -1;}
                else printf("Messgae Que Opened by sender\n");
                num_bytes = mq_send(msgq,
                                    (const char*)parent_message,
                                    sizeof(parent_message),
                                    msg_prio);
                if(num_bytes<0) {printf("mq_send Error:%s\n",strerror(errno)); return -1;}
                else printf("Message sent from parent\n");
                sleep(3);//allows child process to run and send msg
                num_bytes=mq_receive(msgq,
                                     (char*)message_buf,
                                     BUF_SIZE,
                                     &msg_prio);
                if(num_bytes<0) {printf("mq_rcv Error:%s\n",strerror(errno)); return -1;}
                else {
                        printf("rxd in parent:%s;led_status:%d\nlength:%d; priority:%d\n",
                               ((msg_struct*)message_buf)->message,
                               ((msg_struct*)message_buf)->led_status,
                               num_bytes,msg_prio);
                }


        }

        mq_unlink(MY_MQ);
#endif


/**********************************Shared Memory*******************************/
#ifdef SHARED_MEM
        printf("Shared Memory Example\n");
        //const int SIZE = 4096;
        int shmem_fd;/*shared memory file desxriptor*/
        const char name[] = "OS";/*name of the shared mem object*/
        //const char message1[] = "Hello";
        //const char message2[] = "World";
        void* pshmem_obj;

        switch(fork()) {
        case -1: printf("fork error\n");
                return -1;
                break;

        case 0://child process-reads the shared memory
                sleep(1);//let the parent write first
                printf("Entering Child, PID:%d\n",getpid());
                shmem_fd = shm_open(name,O_CREAT|O_RDWR,0666);/*create shared mem obj*/
                ftruncate(shmem_fd,BUF_SIZE);/*config size of shared mem object*/
                pshmem_obj = mmap(0,
                                  BUF_SIZE,PROT_READ|PROT_WRITE,
                                  MAP_SHARED,
                                  shmem_fd,
                                  0);
                if(pshmem_obj == NULL) PRINT_ERR("mmap");

                printf("Read in child:%s; led_status:%d\n",
                       ((msg_struct*)pshmem_obj)->message,
                       ((msg_struct*)pshmem_obj)->led_status );

                /**clear memory***/
                bzero(pshmem_obj,sizeof(msg_struct));
                if(memcpy(pshmem_obj,child_message,sizeof(child_message)) == NULL) {PRINT_ERR("memcpy");}
                else printf("Written from child\n");
                sleep(1);
                shm_unlink(name);
                break;


        default://parent process- writes to the shared memory
                printf("Entering Parent, PID:%d\n",getpid());
                shmem_fd = shm_open(name,O_CREAT|O_RDWR,0666);/*create shared mem obj*/
                ftruncate(shmem_fd,BUF_SIZE);/*config size of shared mem object*/
                /***memory map the shared memory in the address space of the calling process***/
                pshmem_obj = mmap(0,
                                  BUF_SIZE,PROT_WRITE | PROT_READ,
                                  MAP_SHARED,shmem_fd,
                                  0);
                if(pshmem_obj == NULL) PRINT_ERR("mmap");

                /****Write to the shared memory object******/
                /**clear memory***/
                bzero(pshmem_obj,sizeof(msg_struct));

                if(memcpy(pshmem_obj,parent_message,sizeof(parent_message))==NULL) {PRINT_ERR("memcpy");}
                else printf("Written from parent\n");
                sleep(2);  //allow child to complete read and write before parent reads
                printf("Read in Parent :%s; led_status:%d\n",
                       ((msg_struct*)pshmem_obj)->message,
                       ((msg_struct*)pshmem_obj)->led_status );
                shm_unlink(name);
                break;
        }
#endif


/**********************************Sockets*************************************/
#ifdef SOCKETS
        int sockfd;                    // listening FD
        int newsockfd;                 // Client connected FD
        //int addrlen;                   //lenfgth of address
        int num_char;                  //No. of characters red/written
        //  char buffer[256];              //data buffer
        struct sockaddr_in server_addr;//structure containing internet addresss.

        switch(fork()) {
        case -1:
                printf("fork Error:%s\n",strerror(errno));
                return -1;

        case 0://child process - client
                printf("Entering Child-client, PID:%d\n",getpid());

/****create socket********/
                sockfd = socket(
                        AF_INET, // com domain - IPv4
                        SOCK_STREAM, //com type - TCP
                        0); //protocol
                if(sockfd < 0) {printf("socket Error:%s\n",strerror(errno)); return -1;}
/*****clear and initialize the server address structure*****/
                memset(&server_addr,'0',sizeof(server_addr));
                server_addr.sin_family = AF_INET;
                server_addr.sin_port = htons(PORT);

/*convert IP addr from text to binary*/
                ret = inet_pton(AF_INET,"127.0.0.1",&server_addr.sin_addr);
                if(ret<0) {printf("inet_pton Error:%s\n",strerror(errno)); return -1;}

/****connect socket to the address specified in server_addr******/
                ret = connect(sockfd,(struct sockaddr*)&server_addr,sizeof(server_addr));
                if(ret<0) {printf("connect Error:%s\n",strerror(errno)); return -1;}

/******write to socket******/
                num_char =  send(sockfd,child_message,sizeof(child_message),0 );
                printf("message sent from child\n");
/*****read from socket*****/
                num_char = read(sockfd,(char*)message_buf,sizeof(message_buf));
                printf("read in child:%s;led_status:%d\n",
                       ((msg_struct*)message_buf)->message,
                       ((msg_struct*)message_buf)->led_status);

                break;

        default://parent - server
                printf("Parent Process-server, PID:%d\n",getpid());
                int opt = 1;
/****Create a new socket*******/
                sockfd = socket(
                        AF_INET, // com domain - IPv4
                        SOCK_STREAM, //com type - TCP
                        0);   //protocol
                if(sockfd < 0) {printf("fork Error:%s\n",strerror(errno)); return -1;}
/*****set options for the socket***********/
                ret = setsockopt(sockfd,
                                 SOL_SOCKET,          //Socket Level Protocol
                                 SO_REUSEADDR|SO_REUSEPORT,
                                 &opt, //option is enabled
                                 sizeof(opt) );

                if(ret==-1) {printf("setsockopt Error:%s\n",strerror(errno)); return -1;}
/***initialize the address structure ****/
                bzero((char*)&server_addr, sizeof(server_addr));//sets all val to 0
                server_addr.sin_family=AF_INET;
                server_addr.sin_addr.s_addr=INADDR_ANY;
                server_addr.sin_port=htons(PORT);

/***bind socket to the adress and the port */
                ret = bind(sockfd,
                           (struct sockaddr*)&server_addr,
                           sizeof(server_addr));
                if(ret == -1) {printf("bind Error:%s\n",strerror(errno)); return -1;}
/**listen on socket for connections**/
                ret = listen(sockfd,5);
                if(ret == -1) {printf("listen Error:%s\n",strerror(errno)); return -1;}
/****block until the client connects to the server and gets its address*****/
                struct sockaddr_in client_addr;
                socklen_t addrlen = sizeof(client_addr); //size of address of client
                newsockfd = accept(sockfd,
                                   (struct sockaddr*)&client_addr,
                                   (socklen_t*)&addrlen);
                if(newsockfd<0) {printf("accept Error:%s\n",strerror(errno)); return -1;}

/*****beyond this, execution happens only after client is connected******/

/****read from the client and write to it*******/
                bzero(message_buf,sizeof(message_buf));

                num_char = read(newsockfd,(char*)message_buf,sizeof(message_buf));
                if(num_char<0) {printf("read Error:%s\n",strerror(errno)); return -1;}
                printf("read in parent:%s;led_status:%d\n",
                       ((msg_struct*)message_buf)->message,
                       ((msg_struct*)message_buf)->led_status);

                num_char = write(newsockfd,parent_message,sizeof(parent_message));
                if(num_char<0) {printf("write Error:%s\n",strerror(errno)); return -1;}
                else printf("Message sent from parent\n");

        }

#endif

        free(message_buf);
        return 0;
}
