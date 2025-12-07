#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> //pentru fct de baza gen read(),fork(),getpid(),sleep(),etc.
#include <fcntl.h> //defineste constante pt controlul fisierelor gen O_RDONLY etc.
#include <dirent.h> //pentru opendir,readdir,closedir
#include <signal.h> //pentru semnale
#include <sys/types.h> //pentru tipuri de date gen pid_t
#include <sys/wait.h> //pentru wait()
#include <sys/stat.h> //pentru stat()

#define MAX_LINIE 512
#define SIZE 204 //id-32B, username-32B, latitude-4B, longitude-4B, clue-128B, value-4B=32+32+4+4+128+4=204.
#define CMD_FILE "commandfile.txt"

int monitor_pid=-1; //pid ul monitorului, e -1 pt ca inca n a fost pornit
int monitor_running=0; //testam daca merge
int asteptam_terminare=0; //devine 1 dupa ce dam stop la monitor
int terminare=0; //devine 1 cand primeste sigterm

void handle_sigusr1(int sig)
{
    char path[256];
    strcpy(path,CMD_FILE); //stie unde e fisierul cu comanda

    int fd=open(path,O_RDONLY); //deschide fisierul pentru citire
    if(fd==-1)
    {
        perror("MONITOR-Eroare la deschiderea commandfile.txt!!\n");
        exit(-1);
    }

    char command[256]={0}; //initializeaza toate pozitiile cu '\0'
    read(fd,command,sizeof(command)-1); //citeste textul("ex: list_treasures minecraft")
    close(fd);
    command[strcspn(command,"\n")]=0; //scoate '\n' de la final

    char *cuv=strtok(command," "); //ia primul cuvant("list_treasures")
    if(!cuv) //verifica daca nu s a gasit niciun cuvant
    {
        printf("Comanda goala!!\n");
        return;
    }

    if(strcmp(cuv,"view_treasure")==0) //compara primu cuvant cu textul "view_treasure"
    {
        char *hunt=strtok(NULL," "); //ia al doilea cuvant din sir, adica nume hunt
        char *id=strtok(NULL," "); //ia al treilea cuvant din sir, adica id
        if(hunt&&id) //verifica ca ambele exista
        {
	  pid_t pid=fork(); //creeaza un proces copil
            if(pid==0)
            {
	      execlp("./treasure_manager","treasure_manager","view_treasure",hunt,id,NULL);//inlocuieste copilul cu treasure_manager etc..
                perror("MONITOR-Eroare la execlp view_treasure!!\n");
                exit(-1);
            }
            else
	    {
	      wait(NULL); //asteapta copilul sa termine
	    }
        }
    }
    else if(strcmp(cuv,"list_treasures")==0) //compara primu cuvant cu textul "list_treasures"
    {
        char *hunt=strtok(NULL," "); //ia al doilea cuvant din sir, adica nume hunt
        if(hunt) //ne asiguram ca exista
        {
            pid_t pid=fork(); //creeaza un proces copil
            if(pid==0)
            {
	        execlp("./treasure_manager","treasure_manager","list_treasures",hunt,NULL); //inlocuieste copilul cu treasure_manager etc..
                perror("Eroare la execlp list_treasures!!\n");
                exit(-1);
            }
            else
	    {
                wait(NULL); //asteapta copilul sa termine
	    }
        }
    }
    else if(strcmp(cuv,"list_hunts")==0) //compara cuv cu "list_hunts"
     {
       DIR *d=opendir("."); //incearca sa deschida directorul curent
       if(!d) //daca nu s a putut deschide directorul
	 {
	   perror("MONITOR-Nu s-a deschis directorul!!\n");
	   return;
	 }
       printf("MONITOR-Hunts si numarul de comori:\n");
       struct dirent *e; //folosit pentru a parcurge intrarile din director
       while((e=readdir(d))!=NULL) //se citeste cate o intrare de director
	 {
	   if(e->d_type!=DT_DIR) //verifica daca intrarea nu este un subdirector
	     {
	       continue;
	     }
	   //sarim peste cele doua directoare speciale
	   if(strcmp(e->d_name,".")==0) //director curent
	     {
	       continue;
	     }
	   if(strcmp(e->d_name,"..")==0) //director parinte
	     {
	       continue;
	     }

	   char fpath[256];
	   int n=snprintf(fpath,sizeof(fpath),"%s/treasures.dat",e->d_name);
	   if(n<0 || n>=(int)sizeof(fpath)) //daca se depaseste dimensiunea bufferului, se sare peste acest subdirector
	     {
	       continue;
	     }

	   struct stat st; //se declara atributele fisierului gen dimensiune, tip
	   if(stat(fpath,&st)==0 && S_ISREG(st.st_mode)) //apeleaza stat pe calea construita in fpath, daca fisierul este regulat(S_ISREG), atunci intra in bloc
	     {
	       //se imparte dimensiunea fisierului(st.size, in bytes) la SIZE
	       int count=st.st_size/SIZE; //calculeaza nr de comori din fisier
	       printf("%s: %d treasures\n",e->d_name,count);
	     }
	 }
       closedir(d);
     }
    else
    {
        printf("MONITOR-Comanda necunoscuta: %s!!\n",cuv);
    }
}

void handle_sigterm(int sig)
{
  terminare=1; //cand monitorul primeste sigterm prin kill, se apeleaza aceasta functie
}

void monitor_loop()
{
    struct sigaction sa_usr1;
    sa_usr1.sa_handler=handle_sigusr1; //apel cand vine sigursr1
    sigemptyset(&sa_usr1.sa_mask); //fara semnale blocate
    sa_usr1.sa_flags=0;
    sigaction(SIGUSR1,&sa_usr1,NULL); //o inregistrare

    struct sigaction sa_term;
    sa_term.sa_handler=handle_sigterm; //apel cand vine sigterm
    sigemptyset(&sa_term.sa_mask); //fara semnale blocate
    sa_term.sa_flags=0;
    sigaction(SIGTERM,&sa_term,NULL); //o inregistrare

    printf("MONITOR-Pornit cu PID %d\n!!",getpid());

    while(!terminare) //o bucla de asteptare
    {
      pause(); //blocheazza procesul pana la urmatorul semnal
    }

    printf("MONITOR-Oprire...asteptam o secunda!!\n");
    sleep(1);
    printf("MONITOR-Monitor oprit!!\n");
    exit(-2);
}

void start_monitor()
{
    if(monitor_running)
    {
        printf("HUB-Monitorul ruleaza cu PID %d\n!!",monitor_pid); //se anunta ca monitorul ruleaza
        return;
    }

    pid_t p=fork(); //creeaza proces monitor
    if(p<0) //eroare
    {
        perror("HUB-Eroare la fork!!\n");
        exit(-1);
    }

    if(p==0) //suntem in copil
    {
        monitor_loop(); //se intra in bucla monitorului
        exit(-2);
    }
    else //suntem in parinte
    { 
        monitor_pid=p; //salveaza pid ul copilului
        monitor_running=1; //monitorul ruleaza
        asteptam_terminare=0; //nu asteptam oprirea inca
        printf("HUB-Monitor pornit cu PID %d !!\n",monitor_pid);
    }
}

void list_treasures()
{
  if(!monitor_running || asteptam_terminare) //verificam daca monitorul e pornit si nu asteapta terminarea
    {
        printf("HUB-Monitorul nu este activ!!\n");
        return;
    }

    char hunt[128];
    printf("Numele hunt-ului: ");
    fgets(hunt,sizeof(hunt),stdin);
    hunt[strcspn(hunt,"\n")]=0;

    char path[256];
    strcpy(path,CMD_FILE); //pun numele la fisier
    int fd=open(path,O_WRONLY | O_CREAT | O_TRUNC,0777);
    if(fd==-1)
    {
        perror("HUB-Eroare la scrierea in commandfile!!\n");
        exit(-1);
    }

    char linie[MAX_LINIE];
    snprintf(linie,sizeof(linie),"list_treasures %s",hunt); //formatez comanda
    write(fd,linie,strlen(linie));
    close(fd);

    kill(monitor_pid,SIGUSR1);
}

void list_hunts()
{
    if(!monitor_running || asteptam_terminare)
      {
        printf("HUB-Monitorul nu este activ!!\n");
        return;
      }
    int fd=open(CMD_FILE, O_WRONLY|O_CREAT|O_TRUNC, 0666);
    if(fd==-1)
      {
	perror("HUB-open cmd");
	return;
      }
    write(fd, "list_hunts", 11);
    close(fd);
    kill(monitor_pid, SIGUSR1);
}

void view_treasure()
{
    if(!monitor_running || asteptam_terminare) //verificam daca monitorul e pornit si nu asteapta terminarea
    {
        printf("HUB-Monitorul nu este activ!!\n");
        return;
    }

    char hunt[128];
    char id[128];
    
    printf("Hunt: ");
    fgets(hunt,sizeof(hunt),stdin);
    hunt[strcspn(hunt,"\n")]=0;

    printf("ID comoara: ");
    fgets(id,sizeof(id),stdin);
    id[strcspn(id,"\n")]=0;

    char path[256];
    strcpy(path,CMD_FILE); //pun numele la fisier
    int fd=open(path, O_WRONLY | O_CREAT | O_TRUNC,0777);
    if(fd==-1)
    {
        perror("HUB-Eroare la scrierea in commandfile!!\n");
        exit(-1);
    }

    char linie[MAX_LINIE];
    snprintf(linie,sizeof(linie),"view_treasure %s %s",hunt,id); //formatez comanda
    write(fd,linie,strlen(linie));
    close(fd);

    kill(monitor_pid,SIGUSR1);
}

void stop_monitor()
{
    if(!monitor_running) //verificam daca monitorul chiar ruleaza
      {
          printf("HUB-Monitorul nu ruleaza!!\n");
          return;
      }

    asteptam_terminare=1; //impiedica trimiterea de noi comenzi
    kill(monitor_pid,SIGTERM);

    //asteptam ca procesul monitor sa se termine si citesc codul de iesire
    int status;
    waitpid(monitor_pid,&status,0);

    if(WIFEXITED(status)) //verificam daca s a incheiat normal
    {
        printf("HUB-Monitor s-a oprit cu cod %d!!\n",WEXITSTATUS(status));
    }
    else
    {
        printf("HUB-Monitor oprit anormal!!\n");
    }

    monitor_running=0; //nu mai ruleaza
    monitor_pid=-1; //pid invalid
    asteptam_terminare=0; //nu mai asteapta oprirea
}

int main(void)
{
    char input[MAX_LINIE];

    printf("----------------Treasure Hub----------------\n");
    printf("Comenzi: start_monitor, list_treasures, view_treasure, stop_monitor, exit\n\n");

    int running=1;
    
    while(running)
    {
        printf("hub-> ");
        fflush(stdout); //forteaza afisare prompt

        if(!fgets(input,sizeof(input),stdin)) //citesc linie
	  {
              break;
	  }

        input[strcspn(input,"\n")]=0; //elimin '\n'

        if(strcmp(input,"start_monitor")==0) //porneste monitor
	  {
              start_monitor();
	  }
        else if(strcmp(input,"list_treasures")==0) //cere listare
	  {
              list_treasures();
	  }
        else if(strcmp(input,"view_treasure")==0) //cere vizualizare
	  {
              view_treasure();
	  }
	else if(strcmp(input,"list_hunts")==0) //cere listarea hunt-urilor
          {
	      list_hunts();
          }
        else if(strcmp(input,"stop_monitor")==0) //opreste monitor
	  {
              stop_monitor();
	  }
        else if(strcmp(input,"exit")==0) //incheie HUB
        {
            if(monitor_running)
	      {
                  printf("HUB-Mai intai opreste monitorul cu stop_monitor!!\n");
	      }
            else
	      {
                  printf("HUB-Iesire din program!!\n");
                  break;
	      }
        }
        else
        {
            printf("HUB-Comanda necunoscuta!!\n");
        }
    }
    return 0;
}
