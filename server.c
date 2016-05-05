#include<netinet/in.h>
#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<unistd.h>
#include<fcntl.h>

char headOK[]=
"HTTP/1.1 200 OK\r\n"
"Server: RandServer\r\n";
char headNotFound[]=
"HTTP/1.1 404 Not Found\r\n"
"Server: RandServer\r\n\r\n\0";

char headContentTypeHTML[]=
"Content-Type: text/html; charset=utf-8\r\n\r\n\0";
char headContentTypePNG[]=
"Content-Type: image/png; charset=utf-8\r\n\r\n\0";
char headContentTypeJPEG[]=
"Content-Type: image/jpeg; charset=utf-8\r\n\r\n\0";
char headContentTypeJS[]=
"Content-Type: text/javascript; charset=utf-8\r\n\r\n\0";
char headContentTypeCSS[]=
"Content-Type: text/css; charset=utf-8\r\n\r\n\0";
char headContentTypeMP4[]=
"Content-Type: video/mp4; charset=utf-8\r\n\r\n\0";


int getLen(char * fileName){
  struct stat st;
  stat(fileName, &st);
  return st.st_size;
}

char * getType(char * type){
    if (!strncmp(type, ".jpeg",5)){
        return headContentTypeJPEG;
    } else if (!strncmp(type, ".png",4)){
        return headContentTypePNG;
    } else if (!strncmp(type, ".js",3)){
        return headContentTypeJS;
    } else if (!strncmp(type, ".css",4)){
        return headContentTypeCSS;
    }else if (!strncmp(type, ".mp4",4)){
        return headContentTypeMP4;
    } else{
        return headContentTypeHTML;
    }
}

int main(){
    struct sockaddr_in serverAddr, clientAddr;
    int fdServer, fdClient;
    char buf[2048];
    socklen_t addrlen=sizeof(clientAddr);

    fdServer=socket(AF_INET, SOCK_STREAM,0);
    if (fdServer<0){
        perror("Socket creation failed\n");
        exit(1);
    }

    setsockopt(fdServer, SOL_SOCKET, SO_REUSEADDR, 1, sizeof(int));

    serverAddr.sin_family=AF_INET;
    serverAddr.sin_addr.s_addr=INADDR_ANY;
    serverAddr.sin_port= htons(3000);

    if(bind(fdServer, (struct sockaddr *) &serverAddr, sizeof(serverAddr))==-1){
        perror("Binding failed");
        close(fdServer);
        exit(1);
    }
    if (listen(fdServer,10)==-1){
        perror("Listen failed");
        close(fdServer);
        exit(1);
    }

    while (1) {
        fdClient=accept(fdServer, (struct sockaddr*) &clientAddr, &addrlen);
        if (fdClient==-1){
            perror("Connection failed");
            continue;
        }

        printf("%s\n","Client connected");

        if (!fork()){
            //Child
            close(fdServer);
            memset(buf, 0, 2048);
            read(fdClient, buf, 2047);
            printf("%s\n", buf);

            if (!strncmp(buf, "GET", 3)){
                char fileName[64], ext[6], arg[255];
                int k=5;
                int extL=0;
                int argL=0;
                while(buf[k]!=' '){
                    if (buf[k]=='?'){
                        argL=1;
                    }
                    if (!argL) {
                        fileName[k-5]=buf[k];
                    }
                    k=k+1;
                    if (buf[k]=='.'){
                        extL=1;
                    }
                    if (extL && !argL){
                        ext[extL-1]=buf[k];
                        extL=extL+1;
                    }
                    if (argL) {
                        arg[argL-1]=buf[k];
                        argL=argL+1;
                    }
                }
                fileName[k-5]='\0';
                ext[extL-1]='\0';
                arg[argL-1]='\0';

                int fdFile=open(fileName, O_RDONLY);

                if (fdFile < 0) {
                    write(fdClient, headNotFound, strlen(headNotFound));
                    printf("file name : %s and ext : %s and arg : %s and fd : %d\n", fileName, ext, arg, fdFile);
                } else {
                    int size =0;
                    size = getLen(fileName);
                    printf("size of file %s : %d\n", fileName, size);
                    write(fdClient, headOK, strlen(headOK));

                    if (!strncmp(ext, ".php", 4)){
                        int status;
                        int link[2];
                        char * args[4]={"php5-cgi", fileName, arg, 0};
                        if (pipe(link)==-1)
                        perror("pipe");
                        int pid=fork();

                        if (!pid) {
                            //child of child
                            dup2 (link[1], 1);
                            close(link[0]);
                            close(link[1]);
                            execvp(args[0], args);
                        } else {
                            close(link[1]);
                            char reading_buf[1];

                            while(read(link[0], reading_buf, 1) > 0) {
                                write(fdClient, reading_buf, 1);
                            }
                            waitpid(pid, &status,0);
                            close(link[0]);
                            close(fdFile);
                        }

                    } else {
                        int size =0;
                        size = getLen(fileName);
                        write(fdClient, getType(ext), strlen(getType(ext)));
                        sendfile(fdClient, fdFile, NULL, size);
                        close(fdFile);
                    }
                    printf("file name : %s and ext : %s and arg : %s and fd : %d\n", fileName, ext, arg, fdFile);
                }
            }

            close(fdClient);
            exit(0);
        }
        close(fdClient);

    }
    return 0;
}
