#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string>
#include<iostream>
#include <sstream>
#include <fstream>

#define BUFLEN 350 //destul pentru max_double si altele

using namespace std;

string get_message(int type,string func=""){
  string ret;
  switch(type){
    case -1:
      ret="-1 : Clientul nu este autentificat\n\n";
      break;
    case -2:
      ret="-2 : Sesiune deja deschisa\n\n";
      break;
    case -10:
      cout<<"-10 : Eroare la apel "<<func<<endl;
      perror(func.data());
      exit(-1);
  }
  return ret;
}


int main(int argc,char *argv[]){
  //Safeguard pentru argumentele de rulare
  if(argc!=3) {
    fprintf(stderr,"Usage %s <server_IP> <server_PORT> \n", argv[0]);
    exit(0);
  }
  //deschidem log_fileu'
  ofstream log_file;
  string logfile_name="client-";
  logfile_name+=to_string(getpid())+".log";
  log_file.open(logfile_name);

  int tcp_fd,n,udp_fd;
  struct sockaddr_in serv_addr;

  fd_set read_fds;
  fd_set tmp_fds;

  //initializam multimea de fid-uri
  FD_ZERO(&read_fds);
  FD_ZERO(&tmp_fds);

  int fdmax;

  char buffer[BUFLEN];
  string logged="";
  string last_logg_attempt="";

  //dechidem socketul TCP
  tcp_fd=socket(AF_INET,SOCK_STREAM,0);
  if(tcp_fd<0){
    get_message(-10,"socket");
  }

  //dechidem socketul UDP
  udp_fd=socket(AF_INET,SOCK_DGRAM,0);
  if(udp_fd<0){
    get_message(-10,"socket");
  }

  //indesam infromatiile serverlui
  serv_addr.sin_family=AF_INET;
  serv_addr.sin_port=htons(atoi(argv[2]));
  inet_aton(argv[1],&serv_addr.sin_addr);

  if(connect(tcp_fd,(struct sockaddr*)&serv_addr,sizeof(serv_addr))<0){
    get_message(-10,"connect");
  }

  //indesam fid-urile in multime
  FD_SET(STDIN_FILENO,&read_fds);
  FD_SET(tcp_fd,&read_fds);
  fdmax=tcp_fd;

  while(1){
    tmp_fds=read_fds;
    if(select(fdmax+1,&tmp_fds,NULL,NULL,NULL)==-1){
      get_message(-10,"select");
    }

    for(int i=0;i<=fdmax;i++){
      if(FD_ISSET(i,&tmp_fds)){

        //Inidicatie ca serveru' se inchide
        if(i==tcp_fd){
          bzero(buffer,BUFLEN);
          n=read(tcp_fd,buffer,sizeof(buffer));
          //Safeguard
          if(n<0){
            get_message(-10,"read");
          }
          cout<<buffer<<endl;
          close(tcp_fd);
          close(udp_fd);
          log_file.close();
          return 0;
        }

        //Comanda de la tastatura
        if(i==STDIN_FILENO){
          //citesc de la tastatura
          bzero(buffer,BUFLEN);
          cin.getline(buffer, BUFLEN);
          //nu mai scriem si quit in log_file
          if(strcmp(buffer,"quit")!=0){
            log_file<<buffer<<endl;
          }
          istringstream input(buffer);
          string token;
          getline(input, token, ' ');
          //Tratam serviciul de unlock separat-ish
          if(token=="unlock"){
            strcat(buffer," ");
            strcat(buffer,last_logg_attempt.data());

            //Trimitem requestu'
            n=sendto(udp_fd, buffer, strlen(buffer) , 0 ,
                (struct sockaddr *) &serv_addr, sizeof(serv_addr));
            //Safeguard
            if(n<0){
              get_message(-10,"sendto");
            }

            bzero(buffer,BUFLEN);
            socklen_t src_addr_len=sizeof(serv_addr);

            //Primim raspunsu'
            n=recvfrom(udp_fd, buffer, BUFLEN, 0,
                  (struct sockaddr *)&serv_addr, &src_addr_len);
            //Safeguard
            if(n<0){
              get_message(-10,"recvfrom");
            }

            cout<<buffer<<endl;
            log_file<<buffer<<endl;

            string response(buffer);
            //Verificam daca e pasul urmator sau o eroare
            if(response.find("Trimite parola secreta")== string::npos){
              continue;
            }else{//Nisan cerut parola secreta
              string secret_pass;
              cin>>secret_pass;
              log_file<<secret_pass<<endl;

              bzero(buffer,BUFLEN);
              strcpy(buffer,last_logg_attempt.data());
              strcat(buffer," ");
              strcat(buffer,secret_pass.data());

              //Trimitem requestu'
              n=sendto(udp_fd, buffer, strlen(buffer) , 0 ,
                  (struct sockaddr *) &serv_addr, sizeof(serv_addr));
              //Safeguard
              if(n<0){
                get_message(-10,"sendto");
              }
              //Primim raspunsu'
              n= recvfrom(udp_fd, buffer, BUFLEN, 0,
                    (struct sockaddr *)&serv_addr, &src_addr_len);
              //Safeguard
              if(n<0){
                get_message(-10,"recvfrom");
              }
              cout<<buffer<<endl<<endl;
              log_file<<buffer;
              bzero(buffer,BUFLEN);
              token.clear();
              continue;
            }

            token.clear();
            continue;
            //}else
          }else{//Serviciul de tip bancomat
            if(token.empty()){continue;}//Work safe or go home
            //nu e logat si vrea logout
            if(logged.empty() && token=="logout"){
              cout<<get_message(-1);
              log_file<<get_message(-1);
              fflush(stdin);
              continue;
            }
            //e logat si incerca iar sa login
            if(!logged.empty()  && token=="login"){
              cout<<get_message(-2);
              log_file<<get_message(-2);
              fflush(stdin);
              continue;
            }
            //Nu e logat=> are voie decat login si quit
            if(token!="login" && token!="quit" && logged.empty()){
              cout<<get_message(-1);
              log_file<<get_message(-1);
              fflush(stdin);
              continue;
            }

            if(token=="login"){
              getline(input, token, ' ');
              //salvam cardu'
              last_logg_attempt=token;
            }

            if(token=="logout"){
              strcat(buffer," ");
              strcat(buffer,logged.data());
              //golim ultimul card
              logged.clear();
            }

            if(token=="listsold"){
              strcat(buffer," ");
              strcat(buffer,logged.data());
            }

            if(token=="getmoney"){
              strcat(buffer," ");
              strcat(buffer,logged.data());
            }

            if(token=="putmoney"){
              strcat(buffer," ");
              strcat(buffer,logged.data());
            }

            if(token=="quit"){//salvam cardu' poate reuseste logarea
              strcat(buffer," ");
              strcat(buffer,logged.data());

              //Trimitem requestu'
              n=write(tcp_fd,buffer,strlen(buffer));
              //Safeguard
              if(n<0){
                get_message(-10,"write");
              }

              //Inchidem conexiunile
              close(tcp_fd);
              close(udp_fd);
              log_file.close();
              return 0;
            }

            n=write(tcp_fd,buffer,strlen(buffer));
            if(n<0){
              get_message(-10,"write");
            }

            bzero(buffer,BUFLEN);
            //Primim raspnsu'
            n=read(tcp_fd,buffer,sizeof(buffer));
            //Safeguard
            if(n<0){
              get_message(-10,"read");
            }
            string response(buffer);
            //Am primim mesaj de Welcome
            if(response.find("Welcome")!= string::npos){
              logged=token;
            }
            cout<<buffer<<endl<<endl;
            log_file<<buffer<<endl;
          }
        }//STDIN
      }//FD_ISSET
    }//FOR
  }//WHILE
  return 0;
}
