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
char headContentTypeHTML[]=
"Content-Type: text/html; charset=utf-8\r\n\r\n";
char headEnd[]=
"Transfer-Encoding: chunked\r\n"
"Connection: keep-alive\r\n\r\n";


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
    printf("%s%d\n","Client connected");

    if (!fork()){
      //Child
      close(fdServer);
      memset(buf, 0, 2048);
      read(fdClient, buf, 2047);
      printf("%s\n", buf);
      if (!strncmp(buf, "GET", 3)){
        char fileName[64], ext[6];
        int k=5;
        int extL=0;
        while(buf[k]!=' '){
          fileName[k-5]=buf[k];
          k=k+1;
          if (buf[k]=='.'){
            extL=1;
          }
          if (extL){
            ext[extL-1]=buf[k];
            extL=extL+1;
          }
        }
        fileName[k-5]='\0';
        ext[extL-1]='\0';

        int fdFile=open(fileName, O_RDONLY);
        write(fdClient, headOK, strlen(headOK));
        write(fdClient, headContentTypeHTML, strlen(headContentTypeHTML));
        // write(fdClient, headEnd, strlen(headEnd));
        sendfile(fdClient, fdFile, NULL, 300);
        printf("file name : %s and ext : %s\n", fileName, ext);
      }
      // write(fdClient, "hello world\n", 12);
      close(fdClient);
      exit(0);
    }
      close(fdClient);

  }
  return 0;
}

char* getDate() {
  time_t t = time(NULL);
  struct tm *tm = gmtime(&t);
  char date[32];
  strcpy(&date, asctime(tm));
  date[24]=' ';
  return strcat(&date, "GMT");
}
