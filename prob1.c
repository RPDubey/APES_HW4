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

#define MY_MQ "/send_receive_mq"
#define BUF_SIZE (4096) //max size for atomic read and write
#define MSG_Q
//#define SHARED_MEM
//#define SOCKETS
int main(){
        printf("Entering main\n");

/*linux has Half duplex setting, thus file_descriptor[0] is always used for reading,file_descriptor[1] always used for writing*/

        char message_buf[BUF_SIZE];
        char child_message[] = "Hello form child";
        char parent_message[] = "Hello from parent";
        int ret;


/************************************PIPE***********************************/
#ifdef PIPES
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
                else printf("Messag rxd in child:%s\n",message_buf);
                close(file_descriptor[0]); close(file_descriptor[1]);
                printf("Exit child\n");
                break;

        default://parent process(receives PID as return for fork) executes this
                printf("Parent Process, PID:%d\n",getpid());

                ret = read(file_descriptor[0],message_buf,BUF_SIZE);
                if(ret == -1) {printf("Message Not rxd\n"); return -1;}
                else printf("Messag rxd in parent:%s\n",message_buf);

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

/*variables used in each process. after fork() each process will have its own copy*/
        struct mq_attr msgq_attr = {.mq_maxmsg = 10,
                                    .mq_msgsize = BUF_SIZE,
                                    .mq_flags = 0};
        mqd_t msgq;
        int msg_prio;
        int num_bytes;


        switch(fork()) {
        case -1://fork failed
                printf("Fork Error:%s\n",strerror(errno));
                return -1;

        case 0://child process receives message
                printf("Entered Child Process\n");
                msgq = mq_open(MY_MQ,
                               O_RDWR,
                               S_IRWXU,//non zero for O_CREAT flags
                               &msgq_attr);
                if(msgq < 0) { printf("mq_open Error:%s\n",strerror(errno)); return -1;}
                else printf("Messgae Que Opened by rxr\n");
                num_bytes=mq_receive(msgq,
                                     message_buf,
                                     BUF_SIZE,
                                     &msg_prio);
                if(num_bytes<0) {printf("mq_rcv Error:%s\n",strerror(errno)); return -1;}
                else {
                        message_buf[num_bytes] = '\0';
                        printf("received:%s;\n length:%d; priority:%d\n",
                               message_buf,num_bytes,msg_prio);
                }
                break;


        default://parent process sends the message
                printf("Entered Parent Process\n");
                msg_prio = 30;
                msgq = mq_open(MY_MQ,           //name
                               O_CREAT | O_RDWR,//flags
                               S_IRWXU,         //mode-read,write and execute permission
                               &msgq_attr);     //attribute
                if(msgq < 0) { printf("mq_open Error:%s\n",strerror(errno)); return -1;}
                else printf("Messgae Que Opened by sender\n");

                num_bytes = mq_send(msgq,
                                    parent_message,
                                    sizeof(parent_message),
                                    msg_prio);
                if(num_bytes<0) {printf("mq_send Error:%s\n",strerror(errno)); return -1;}
                else printf("Message of length %d bytes sent\n",num_bytes );
        }

#endif


/**********************************Shared Memory*******************************/
#ifdef SHARED_MEM
        const int SIZE = 4096;
        int shmem_fd;/*shared memory file desxriptor*/
        const char name[] = "OS";/*name of the shared mem object*/
        const char message1[] = "Hello";
        const char message2[] = "World";
        void* pshmem_obj;

        switch(fork()) {
        case -1: printf("fork error\n");
                return -1;
                break;

        case 0://child process-reads the shared memory
                printf("Entering Child, PID:%d\n",getpid());
                shmem_fd = shm_open(name,O_CREAT|O_RDWR,0666);/*create shared mem obj*/
                ftruncate(shmem_fd,SIZE);/*config size of shared mem object*/
                pshmem_obj = mmap(0,SIZE,PROT_READ,MAP_SHARED,shmem_fd,0);
                printf("Read from shared memory:%s\n",(char*)pshmem_obj);
                shm_unlink(name);
                break;


        default://parent process- writes to the shared memory
                printf("Entering Parent, PID:%d\n",getpid());
                shmem_fd = shm_open(name,O_CREAT|O_RDWR,0666);/*create shared mem obj*/
                ftruncate(shmem_fd,SIZE);/*config size of shared mem object*/
/*memory map the shared memory in the address space of the calling process*/
                pshmem_obj = mmap(0,SIZE,PROT_WRITE,MAP_SHARED,shmem_fd,0);
/*Write to the shared memory object*/
                sprintf((char*)pshmem_obj,"%s",message1);
                sprintf( (char*)( pshmem_obj + strlen(message1) ),"%s",message2);
                //shm_unlink(name);
                break;
        }
#endif


/**********************************Sockets*************************************/
#ifdef Sockets
        int sockfd;       // listening FD
        int newsockfd;    // Client connected FD
        int portno;
        int client;       //size of address of client
        int n;            //No. of characters red/written
        char buffer[256]; //data buffer
        struct sockaddr_in server_addr, client_addr;
//??????validate inputs

        sockfd = socket(
                AF_INET,
                SOCK_STREAM,
                0);
        if(sockfd < 0) {perror(errno); return -1;}



#endif

        return 0;
}
