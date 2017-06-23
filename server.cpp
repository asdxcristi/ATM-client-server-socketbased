#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "lib.h"
#include <unordered_map>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <algorithm>

#define MAX_WAITING_CLIENTS	5
#define BUFLEN 350 //destul pentru max_double si altele

using namespace std;

//Functie pentru mesajele posibile transmise de server
string get_message(int type,string func="",string ret="ATM> "){
  switch (type) {
    case 1:
      ret=ret+ "Welcome ";
      break;
    case 2:
      ret=ret+ "Deconectare de la bancomat";
      break;
    case 3:
      ret=ret+ "Suma depusa cu succes";
      break;
    case 4:
      ret="UNLOCK> Trimite parola secreta";
      break;
    case 5:
      ret="UNLOCK> Client deblocat";
      break;
    case 6:
      ret="UNLOCK> Cod eroare -4";
      break;
    case -1:
      ret=ret+to_string(type)+" : Clientul nu este autentificat";
      break;
    case -2:
      ret=ret+to_string(type)+" : Sesiune deja deschisa";
      break;
    case -3:
      ret=ret+to_string(type)+" : Pin gresit";
      break;
    case -4:
      ret=ret+to_string(type)+" : Numar card inexistent";
      break;
    case -5:
      ret=ret+to_string(type)+" : Card blocat";
      break;
    case -6:
      ret=ret+to_string(type)+" : Operatie esuata";
      break;
    case -7:
      ret=ret+to_string(type)+" : Deblocare esuata";
      break;
    case -8:
      ret=ret+to_string(type)+" : Fonduri insuficiente";
      break;
    case -9:
      ret=ret+to_string(type)+" : Suma nu e multiplu de 10";
      break;
    case -10:
      ret=ret +  "-10 : Eroare la apel " + func;
      cout<<ret<<endl;
      perror(func.data());
      exit(-1);
    case -13:
      //Simple but efficient mind game :)
      ret=ret+  "No bad inputs,pretty please!!! *don't bypass me or i'll cry*";
      break;
  }
  return ret;
}

//Container pentru informatiile userilor
unordered_map<string,user_shit> user_info;

void load_info(string file){
  ifstream fin;
  fin.open(file);
  user_shit temp;
  int n;
  fin>>n;
  for(int i=0;i<n;i++){
    fin>>temp.nume;
    fin>>temp.prenume;
    fin>>temp.numar_card;
    fin>>temp.pin;
    fin>>temp.parola_secreta;
    fin>>temp.sold;
    user_info[temp.numar_card]=temp;
  }
  fin.close();
}



int main(int argc,char *argv[]){
  //safeguard pentru argumentele de rulare
  if(argc!=3){
    fprintf(stderr,"Usage : %s <PORT> <users_data_file>\n", argv[0]);
    exit(1);
  }

//vector ce contine conxiunle active cu clienti
  vector<int> tcp_client_fids;

  int tcp_fd, new_sock_fd, port,udp_fd;
  socklen_t client_len;
  char buffer[BUFLEN];
  struct sockaddr_in serv_addr, cli_addr;
  int n,i;
  fd_set read_fds;
  fd_set tmp_fds;
  int fd_max;

  //incarcam datele userilor din fisier in container
  load_info(argv[2]);

  //initializam multimea de fid-uri
  FD_ZERO(&read_fds);
  FD_ZERO(&tmp_fds);

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

  port=atoi(argv[1]);
  bzero(&serv_addr,sizeof(serv_addr));

  //indesam infromatiile serverlui
  serv_addr.sin_family=AF_INET;
  serv_addr.sin_addr.s_addr=INADDR_ANY;
  serv_addr.sin_port=htons(port);

  //bind-uim socketul UDP
  if(bind(udp_fd,(struct sockaddr*)&serv_addr,sizeof(struct sockaddr))<0){
    get_message(-10,"bind");
  }

  //bind-uim socketul TCP
  if(bind(tcp_fd,(struct sockaddr*)&serv_addr,sizeof(struct sockaddr))<0){
    get_message(-10,"bind");
  }

  //punem socketul TCP pe listening
  listen(tcp_fd,MAX_WAITING_CLIENTS);

  //indesam fid-urile in multime
  FD_SET(STDIN_FILENO,&read_fds);
  FD_SET(udp_fd,&read_fds);
  FD_SET(tcp_fd,&read_fds);

  fd_max=tcp_fd;


  string response;

  while (1){
    tmp_fds=read_fds;
    if(select(fd_max+1,&tmp_fds,NULL,NULL,NULL)==-1){
      get_message(-10,"select");
    }

    for(i=0;i<=fd_max;i++){
      if(FD_ISSET(i,&tmp_fds)){
//######################## STDIN ############################
        if(i==STDIN_FILENO){
          bzero(buffer,BUFLEN);
          cin>>buffer;
          //nu e o comanda valida
          if(strcmp(buffer,"quit")!=0){
            cout<<"That wasn't very cash money of you"<<endl;
            continue;
          }

          //Anuntam toata lumea(clientii activi) ca inchidem serveru'
          for(unsigned int i=0;i<tcp_client_fids.size();i++){
            bzero(buffer,BUFLEN);
            strcpy(buffer,"Serverul se inchide curand");
            n=write(tcp_client_fids[i],buffer,strlen(buffer));
            //Safeguard
            if(n<0){
              get_message(-10,"write");
            }
          }

          //Ii omoram pe toti si inchidem usa
          for(unsigned int i=0;i<tcp_client_fids.size();i++){
            close(tcp_client_fids[i]);
            FD_CLR(tcp_client_fids[i],&read_fds);
          }
          //inchidem si celelalte conexiuni si golim si multimea de fid-uri
          close(tcp_fd);
          FD_CLR(tcp_fd,&read_fds);
          close(udp_fd);
          FD_CLR(udp_fd,&read_fds);
          return 0;
        }
//########################^ STDIN ^############################

//#########################  UDP  #############################
        if(i==udp_fd){
          bzero(buffer,BUFLEN);
          client_len=sizeof(cli_addr);
          n=recvfrom(udp_fd, buffer, BUFLEN, 0, (struct sockaddr *)&cli_addr,
              &client_len);
          //Safeguard
          if(n<0){
            get_message(-10,"recvfrom");
          }
          //Am primit ceva pe UDP
          cout<<"[UDP Request: "<<buffer<<" from : "<<inet_ntoa(cli_addr.sin_addr)<<":"<<ntohs(cli_addr.sin_port)<<endl;

          //Prelucram ce am primit
          istringstream request(buffer);
          string token;
          getline(request, token, ' ');

          //Request de unlock <card_nr>
          if(token=="unlock"){
            getline(request, token, ' ');//current_card_no
            auto search = user_info.find(token);
            //verificam daca exista cardu'
            if(search == user_info.end()){//nu exista cardu' ala
              bzero(buffer,BUFLEN);
              strcpy(buffer,get_message(6).data());
              strcat(buffer,"\n");
              n=sendto(i,buffer,strlen(buffer),0,
                  (struct sockaddr *)&cli_addr,sizeof(cli_addr));
              //Safeguard
              if(n<0){
                get_message(-10,"sendto");
              }
                continue;
            }
            //exista cardu' dar e si blocat?
            if(search->second.failed_attempts<3){//nu e blocat
              bzero(buffer,BUFLEN);
              strcpy(buffer,get_message(-6,"","UNLOCK> ").data());
              strcat(buffer,"\n");
              n=sendto(i, buffer, strlen(buffer) , 0 ,
                  (struct sockaddr *) &cli_addr,sizeof(cli_addr));
              //Safeguard
              if(n<0){
                get_message(-10,"sendto");
              }
              continue;
            }

            //exista si e blocat => cerem parola secreta
            bzero(buffer,BUFLEN);
            strcpy(buffer,get_message(4).data());//SEND NUDES
            n=sendto(udp_fd,buffer,strlen(buffer),0,
                (struct sockaddr *)&cli_addr,client_len);
            //Safeguard
            if(n<0){
              get_message(-10,"sendto");
            }
            continue;
          }else{//Request de <parola_secreta>
            auto search = user_info.find(token);
            getline(request, token, ' ');//parola

            if(search->second.parola_secreta!=token){//e gresita parola_secreta
              bzero(buffer,BUFLEN);
              strcpy(buffer,get_message(-7,"","UNLOCK> ").data());

              n=sendto(i, buffer, strlen(buffer) , 0 ,
                  (struct sockaddr *) &cli_addr,sizeof(cli_addr));
              //Safeguard
              if(n<0){
                get_message(-10,"sendto");
              }
              continue;
            }
            //e ok si parola_secreta
              search->second.failed_attempts=0;
              bzero(buffer,BUFLEN);
              strcpy(buffer,get_message(5).data());//E OK PAROLA

              n=sendto(i, buffer, strlen(buffer) , 0 ,
                (struct sockaddr *) &cli_addr,sizeof(cli_addr));
              //Safeguard
              if(n<0){
                get_message(-10,"sendto");
              }
              continue;
          }

        }
//#########################^ UDP ^#############################

//######################### TCP listener #############################
        if(i==tcp_fd){
          //am primit ceva pe listenerul TCP pt conexiuni noi
          client_len=sizeof(cli_addr);
          if((new_sock_fd=accept(tcp_fd,(struct sockaddr*)&cli_addr,
                    &client_len))==-1){
            get_message(-10,"accept");
          }
          else{
            //indesam fid-ul noi conexiuni in multime
            FD_SET(new_sock_fd,&read_fds);
            //bagam in lista fidurilor clientilora activi
            tcp_client_fids.push_back(new_sock_fd);
            if(new_sock_fd>fd_max){
              fd_max=new_sock_fd;
            }
          }
          cout<<"Noua conexiune de la "<<inet_ntoa(cli_addr.sin_addr)<<":"
            <<ntohs(cli_addr.sin_port)<<", socket_client "<<new_sock_fd<<endl;
//#########################^ TCP listener ^#############################
        }else{
          // am primit date pe unul din socketii cu care vorbesc cu clientii
          bzero(buffer,BUFLEN);
          if((n=read(i,buffer,sizeof(buffer)))<=0){
            if(n==0){
              //conexiunea s-a inchis
              cout<<"Clientul de pe socketul "<<i<<" a murit subit"<<endl;
            }else{
              get_message(-10,"read");
            }
            //erase-remove idiom magic pt scos clientul din lista celor activi
            tcp_client_fids.erase(remove(tcp_client_fids.begin(),
                          tcp_client_fids.end(), i), tcp_client_fids.end());
            close(i);
            FD_CLR(i,&read_fds); // scoatem din multimea de citire socketul pe care

          }else{ //am primit ceva bun pe listener
            cout<<"[TCP Request]: "<<buffer<<" de la clientul de pe socketul "<<i<<endl;
            istringstream request(buffer);
            string token;
            getline(request, token, ' '); // omoram primu spatiu

            //Request de login
            if(token=="login"){
              string current_pin;
              string current_card_no;
              getline(request, current_card_no, ' ');
              getline(request, current_pin, ' ');

              //Cautam cardul in baza de date
              auto search = user_info.find(current_card_no);

              if(search == user_info.end()){//cardul nu exista
                bzero(buffer,BUFLEN);
                strcpy(buffer,get_message(-4).data());

                //Trimitem raspunsu' catre client
                n=write(i,buffer,strlen(buffer));
                //Safeguard
                if(n<0){
                  get_message(-10,"write");
                }
                continue;
              }else{//exista cardu'
              //login magic
                if(search->second.active){//e deja activa o sesiune
                  bzero(buffer,BUFLEN);

                  strcat(buffer,get_message(-2).data());

                  //Trimitem raspunsu' catre client
                  n=write(i,buffer,strlen(buffer));
                  //Safeguard
                  if(n<0){
                    get_message(-10,"write");
                  }
                  continue;
                }else{//nu are o sesiune activa

                  if(search->second.pin!=current_pin){//pinu' gresit
                    search->second.failed_attempts++;
                    //verificam daca e blocat
                    if(search->second.failed_attempts>=3){
                      bzero(buffer,BUFLEN);
                      strcpy(buffer,get_message(-5).data());

                      //Trimitem raspunsu' catre client
                      n=write(i,buffer,strlen(buffer));
                      //Safeguard
                      if(n<0){
                        get_message(-10,"write");
                      }
                      continue;
                    }
                    //nu e blocat,dar e pinu' gresit
                    bzero(buffer,BUFLEN);

                    strcat(buffer,get_message(-3).data());

                    n=write(i,buffer,strlen(buffer));
                    //Safeguard
                    if(n<0){
                      get_message(-10,"write");
                    }
                    continue;
                  }else{//e bun pinu'
                    if(search->second.failed_attempts>=3){//e blocat cardu'
                      bzero(buffer,BUFLEN);
                      strcpy(buffer,get_message(-5).data());

                      //Trimitem raspunsu' catre client
                      n=write(i,buffer,strlen(buffer));
                      //Safeguard
                      if(n<0){
                       get_message(-10,"write");
                      }
                      continue;
                    }
                    //e bun pinu' si nu e blocat cardu'
                    search->second.active=true;//deschidem sesiunea
                    response=search->second.nume+" "+search->second.prenume;

                    bzero(buffer,BUFLEN);

                    strcat(buffer,get_message(1).data());
                    strcat(buffer,response.data());

                    //Trimitem raspunsu' catre client
                    n=write(i,buffer,strlen(buffer));
                    //Safeguard
                    if(n<0){
                      get_message(-10,"write");
                    }
                    continue;
                  }
                }
              }
            }//LOGIN

            //Request de logout
            if(token=="logout"){
              string current_card_no;
              getline(request, current_card_no, ' ');
              //se presupune ca userul exista daca e logat si a ajuns aici
              auto search = user_info.find(current_card_no);
              search->second.active=false;//ii oprim sesiunea
              bzero(buffer,BUFLEN);
              strcpy(buffer,get_message(2).data());
              //Trimitem raspunsu' catre client
              n=write(i,buffer,strlen(buffer));
              //Safeguard
              if(n<0){
                get_message(-10,"write");
              }
              continue;
            }

            //Request de listsold
            if(token=="listsold"){
              string current_card_no;
              getline(request, current_card_no, ' ');
              //se presupune ca userul exista daca e logat si a ajuns aici
              auto search = user_info.find(current_card_no);
              bzero(buffer,BUFLEN);
              stringstream stream;
              stream << fixed << setprecision(2) << search->second.sold;

              strcpy(buffer,stream.str().data());

              //Trimitem raspunsu' catre client
              n=write(i,buffer,strlen(buffer));
              //Safeguard
              if(n<0){
                get_message(-10,"write");
              }
              continue;
            }

            //Request de getmoney
            if(token=="getmoney"){
              string funds_for_drugs,current_card_no;
              getline(request, funds_for_drugs, ' ');
              getline(request, current_card_no, ' ');
              //suma nu e multiplu de 10
              if(atoi(funds_for_drugs.data()) % 10 != 0){
                bzero(buffer,BUFLEN);
                strcpy(buffer,get_message(-9).data());
                //Trimitem raspunsu' catre client
                n=write(i,buffer,strlen(buffer));
                //Safeguard
                if(n<0){
                  get_message(-10,"write");
                }
                continue;
              }

              //suma ceruta e buna, sa vedem daca o si avem
              auto search = user_info.find(current_card_no);
              //suma cerunta mai mare ca soldu'
              if(search->second.sold < atoi(funds_for_drugs.data())){
                bzero(buffer,BUFLEN);
                strcpy(buffer,get_message(-8).data());

                //Trimitem raspunsu' catre client
                n=write(i,buffer,strlen(buffer));
                //Safeguard
                if(n<0){
                  get_message(-10,"write");
                }
                continue;
              }
              //suma a fost retrasa cu succes
              search->second.sold -= atoi(funds_for_drugs.data());

              bzero(buffer,BUFLEN);

              strcpy(buffer,"Suma ");
              strcat(buffer,funds_for_drugs.data());
              strcat(buffer," retrasa cu succes");

              //Trimitem raspunsu' catre client
              n=write(i,buffer,strlen(buffer));
              //Safeguard
              if(n<0){
                get_message(-10,"write");
              }
              continue;
            }//getmoney

            //Request de putmoney
            if(token=="putmoney"){
              string funds_for_later_drugs,current_card_no;
              getline(request, funds_for_later_drugs, ' ');
              getline(request, current_card_no, ' ');
              //Se presupune ca useru' exista
              auto search = user_info.find(current_card_no);
              //Adaugam suma depusa la sold
              search->second.sold+=atof(funds_for_later_drugs.data());

              bzero(buffer,BUFLEN);
              strcpy(buffer,get_message(3).data());

              //Trimitem raspunsu' catre client
              n=write(i,buffer,strlen(buffer));
              //Safeguard
              if(n<0){
                get_message(-10,"write");
              }
              continue;
            }

            //Request de quit
            if(token=="quit"){
              string current_card_no;
              getline(request, current_card_no, ' ');
              if(!current_card_no.empty()){
                auto search = user_info.find(current_card_no);
                //inchidem sesiunea clientului
                search->second.active=false;
              }
              cout<<"Client "<<i<<" just died"<<endl;
              close(i);
              FD_CLR(i,&read_fds);
              //erase-remove idiom magic pt scos din lista de useri activi
              tcp_client_fids.erase(remove(tcp_client_fids.begin(),
                        tcp_client_fids.end(), i), tcp_client_fids.end());
              continue;
            }

            //Bad move moffo
            bzero(buffer,BUFLEN);
            strcpy(buffer,get_message(-13).data());
            //Really,wrong neighbourhood(comanda gresita)
            n=write(i,buffer,strlen(buffer));
            //Safeguard
            if(n<0){
              get_message(-10,"write");
            }
            continue;
          }//STDIN
        }//PRIMIT PE TCP
      }//FD_ISSET
    }//FOR
  }//WHILE
  return 0;
}
