//-avem de facut un program treasure_manager care stocheaza date despre o vanato//are de comori
//informatii despre comoara: ID, nume utilizator, coordonate GPS, indiciu, valoa//re, unde implementam 
//operatii in linia de comanda care sa adauge o comoara la un hunt, sa vedem toa//te comorile, sa vedem 
//doar o comoara, sa stergem o comoara si sa stergem un hunt. 
//-problema la codul meu este ca atunci cand introduc un tip de date diferit fat/a de cel dat in structura
//imi trece la celelalte informatii despre comoara fara a mi arata eroare.

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h> //pentru open,O_CREAT,O_WRONLY,O_APPEND
#include <unistd.h> //pentru read,write,close
#include <string.h>
#include <sys/stat.h> //pentru stat,mkdir,chmod
#include <sys/types.h> //defineste tipuri de date
#include <time.h> //pentru time,ctime()
#include <dirent.h> //pentru opendir,readdir,closedir
#include <errno.h> //pentru errno, erori si perror

#define username_len 32 
#define clue_len 128
#define id_len 32
#define treasure_file "treasures.dat" //fisier ctreasures
#define log_file "logged_hunt" //fisier de log(aici arata toate actiunile)

//aici stabilim informatii despre fiecare treasure
typedef struct
{
    char id[id_len];
    char username[username_len];
    float latitude;
    float longitude;
    char clue[clue_len];
    int value;
} TREASURE;

//in logs, vor aparea modificari realizate, exemplu "Adaugare treasure"
void logs(char *hunt,char *operation)
{
  char path[256]; //stocheaza calea catre un director
  snprintf(path,sizeof(path),"%s/%s",hunt,log_file); //exemplu : vanatoare/logged_hunt

  int log_fd=open(path, O_APPEND|O_CREAT|O_WRONLY ,0777);
  if(log_fd==-1)
    {
      perror("Eroare la deschiderea fisierului log!!");
      exit(-1);
    }

  time_t now = time(NULL); //timpul curent
  char log_inreg[512]; //inregistrari din fisier log
  snprintf(log_inreg,sizeof(log_inreg),"%s%s\n",ctime(&now),operation); //timp+operatie

  //se creeaza linkul simbolic
  char sym[256];
  snprintf(sym,sizeof(sym),"logged_hunt-%s",hunt); //o sa apara logged_hunt-vanatoare
  symlink(path,sym);//se creeaza link simbolic
  
  if(write(log_fd,log_inreg,strlen(log_inreg))==-1)
    {
      perror("Nu se poate scrie in fisierul de log!!\n");
      exit(-1);
    }
  
  close(log_fd);
}

//in add_treasure, efectuam realizarea informatiilor despre treasure

void add_treasure(char *hunt)
{
    DIR *dir=opendir(hunt);
    if(dir==NULL)
    {
        if(mkdir(hunt,0777)==0)
        {
            printf("Directorul %s s-a creat!!\n",hunt);
        }
        else
        {
            perror("Directorul nu s-a creat!!\n");
            exit(-2);
        }
    }

    char path[256];
    sprintf(path,"%s/%s",hunt,treasure_file);

    int fd=open(path,O_APPEND | O_CREAT | O_WRONLY,0777); //deschide fisierul pt scriere
    if(fd==-1)
    {
        perror("Nu se poate deschide fisierul!!\n");
        exit(-2);
    }

    TREASURE t;
    printf("-------Treasure's details:-------\n");

    printf("Id: ");
    if(fgets(t.id,id_len,stdin)==NULL)
    {
        printf("Eroare la citirea id-ului!!\n");
        close(fd);
        exit(-1);
    }
    t.id[strcspn(t.id,"\n")]='\0';

    printf("Username: ");
    if(fgets(t.username,username_len,stdin)==NULL)
    {
        printf("Eroare la citirea username-ului!!\n");
        close(fd);
        exit(-1);
    }
    t.username[strcspn(t.username,"\n")]='\0';

    printf("Clue: ");
    if(fgets(t.clue,clue_len,stdin)==NULL)
    {
        printf("Eroare la citirea indiciului!!\n");
        close(fd);
        exit(-1);
    }
    t.clue[strcspn(t.clue,"\n")]='\0';

    printf("Latitude: ");
    if(scanf("%f",&t.latitude)!=1)
    {
        printf("Eroare..Introdu un float!!\n");
        close(fd);
        exit(-1);
    }

    printf("Longitude: ");
    if(scanf("%f",&t.longitude)!=1)
    {
        printf("Eroare..Introdu un float!!\n");
        close(fd);
        exit(-1);
    }

    printf("Value: ");
    if(scanf("%d",&t.value)!=1)
    {
        printf("Eroare..Introdu un int!!\n");
        close(fd);
        exit(-1);
    }

    getchar(); //elimina newline-ul lasat de scanf

    if(write(fd,&t,sizeof(TREASURE))==-1)
    {
        perror("Treasure nu a putut fi adaugat!!\n");
        close(fd);
        exit(-1);
    }
    else
    {
        printf("Treasure a fost adaugat!!\n");
    }

    close(fd);
    logs(hunt, "--Adaugare treasure--");
    printf("Treasure adaugat!!!\n");
}

//in list_treasures apare lista cu treasures
void list_treasures(char *hunt)
{
  TREASURE t;
  DIR *dir;
  dir=opendir(hunt);
  if(dir==NULL)
    {
      printf("Fisierul nu merge!!\n");
      exit(-1);
    }
  
  char path[256];
  snprintf(path,sizeof(path),"%s/%s",hunt,treasure_file);
  
  struct stat st;
  if(stat(path,&st)==-1)//se obtin informatii despre fisier
    {
      perror("Eroare la citirea fisierului!!");
      exit(-3);
    }

  printf("Hunt: %s\nSize: %ld bytes\nLast modification: %s\n",hunt,st.st_size,ctime(&st.st_mtime));

  int fd=open(path,O_RDONLY);
  if(fd==-1)
    {
      perror("Eroare la deschiderea fisierului!!");
      exit(-4);
    }

  while(read(fd,&t,sizeof(TREASURE))) //citeste fiecare treasure
  {
      printf("ID: %s\nUsername: %s\nLatitude: %f\nLongitude: %f\nClue: %s\nValue: %d\n\n",t.id,t.username,t.latitude,t.longitude,t.clue,t.value);
  }

  close(fd);
  logs(hunt,"--Afisare treasures--");
}

//din view_treasure putem vizualiza informatii despre un treasure ales de noi
void view_treasure(char *hunt, char *id)
{
  char path[256];
  snprintf(path,sizeof(path),"%s/%s",hunt,treasure_file);

  TREASURE t;
  int fd=open(path,O_RDONLY);
  if(fd==-1)
    {
        perror("Eroare la deschidere!!");
        exit(-5);
    }

  while(read(fd,&t,sizeof(TREASURE)))//cauta treasure cu id ul ales de noi
    {
        if(strcmp(t.id,id)==0)
	  {
            printf("Id: %s\nUsername: %s\nLatitude: %f\nLongitude: %f\nClue: %s\nValue: %d\n",t.id,t.username,t.latitude,t.longitude,t.clue,t.value);
            logs((char *)hunt,"--Afisare treasure--");
            close(fd);
            return;
	  }
    }
    printf("Treasure-ul cu Id-ul %s nu exista!!\n",id);
    close(fd);
}

//in remove_treasure se sterge un treasure ales de noi
void remove_treasure(char *hunt,char *id)
{
    TREASURE t;
    char path[256];
    snprintf(path,sizeof(path),"%s/%s",hunt,treasure_file);

    int fd=open(path,O_RDONLY);
    if (fd==-1)
      {
         perror("Eroare la citire!!");
         exit(-6);
      }

    int temp_fd=open("temp.dat",O_WRONLY|O_CREAT|O_TRUNC, 0777);//cream un fisier temporar care ajuta la stergere
    if (temp_fd==-1)
      {
         perror("Eroare la crearea temp!!");
         close(fd);
         exit(-7);
      }

    while(read(fd,&t,sizeof(TREASURE)))
      {
        if(strcmp(t.id,id)!=0)
	  {
            write(temp_fd,&t,sizeof(TREASURE));
	  }
      }

    close(fd);
    close(temp_fd);
    rename("temp.dat",path);
    logs(hunt,"--Stergere treasure--");
    printf("Treasure-ul cu Id %s a fost sters!!\n",id);
}

//in remove_hunt se sterge complet tot hunt-ul
void remove_hunt(char *hunt)
{
    char path[256];
    snprintf(path,sizeof(path),"%s/%s",hunt,treasure_file);
    unlink(path);//sterge fisierului de treasures
    
    snprintf(path,sizeof(path),"%s/%s",hunt,log_file);
    unlink(path);//sterge fisierul de log
    rmdir(hunt);//sterge directorul
	     
    char linkname[256];
    snprintf(linkname,sizeof(linkname),"logged_hunt-%s",hunt);
    unlink(linkname);//sterge linkul simbolic
    printf("Hunt-ul a fost sters!!\n");
}

int main(int argc,char **argv)
{
    //verifica daca sunt destule argumente
    if(argc<3)
      {
        perror("Eroare numar argumente");
	exit(-6);
      }
    
    //aici scriem in linia de comanda, operatiile care dorim sa fie realizate
    if(strcmp(argv[1],"add_treasure")==0)
      {
        add_treasure(argv[2]);
      }
    else if(strcmp(argv[1],"list_treasures")==0)
      {
        list_treasures(argv[2]);
      }
    else if(strcmp(argv[1],"view_treasure")==0 && argc==4)
      {
        view_treasure(argv[2],argv[3]);
      }
    else if (strcmp(argv[1],"remove_treasure")==0 && argc==4)
      {
        remove_treasure(argv[2],argv[3]);
      }
    else if (strcmp(argv[1],"remove_hunt")==0)
      {
        remove_hunt(argv[2]);
      }
    else
      {
        perror("Aceasta comanda nu exista!!\n");
        exit(-8);
      }
    return 0;
}
