#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <unistd.h> //pentru fct de baza gen read(),fork(),getpid(),sleep(),etc.
#include <fcntl.h> //defineste constante pt controlul fisierelor gen O_RDONLY etc.
#include <dirent.h> //pentru opendir(),readdir,closedir
#include <sys/types.h> //pentru tipuri de date gen pid_t
#include <sys/wait.h> //pentru wait()
#include <sys/stat.h> //pentru stat()

#define SIZE 204 //id-32B, username-32B, latitude-4B, clue-128B, value-4B=32+32+4+4+128+4=204.
#define ID_LEN 32 
#define USERNAME_LEN 32
#define CLUE_LEN 128

//functie care calculeaza scorul unui user intr-un singur hunt, adica dintr-un director
int scor_din_hunt(char *hunt,char *user)
{
    char filepath[512];
    snprintf(filepath,sizeof(filepath),"%s/treasures.dat",hunt);

    int fd=open(filepath,O_RDONLY);
    if(fd==-1)
      {
	 perror("Eroare la deschidere!!");
	 exit(-1);
      }

    char buffer[SIZE];//buffer in care se citeste o comoara de dimensiune 204
    int scor=0;

    //citim fiecare inregistrare de 204 bytes si extragem username ul si valoarea
    while(read(fd,buffer,SIZE)==SIZE)
      {
	 char username[USERNAME_LEN+1];
         strncpy(username,buffer+ID_LEN,USERNAME_LEN); //copiaza din buffer numele user-ului in username
         username[USERNAME_LEN]='\0'; //asigur terminarea sirului cu \0
         int valoare; //variabila in care extrag valoarea comorii
         memcpy(&valoare,buffer+ID_LEN+USERNAME_LEN+4+4+CLUE_LEN,sizeof(int)); //extragem campul value, adica ultimii 4 bytes din buffer

	 if(strcmp(username,user)==0) //daca username din comoara este egal cu user ul dat ca argument
	   {
	     scor=scor+valoare;
	   }
      }
    close(fd);
    return scor;
}

//functie care parcurge toate directoarele si creeaza cate un proces pentru fiecare hunt
int calculeaza_total(char *user)
{
    DIR *dir=opendir(".");
    if(!dir)
      {
         perror("Eroare la deschiderea directorului curent!!");
         exit(-1);
      }

    struct dirent *intrare;
    int total=0;
    
    //citim fiecare intrare din directorul curent
    while((intrare=readdir(dir))!=NULL) //parcurge toate fisierele/directoarele din directorul curent
      {
        //verificam daca e director valid (excludem "." si "..")
        if(intrare->d_type==DT_DIR && strcmp(intrare->d_name,".")!=0 && strcmp(intrare->d_name,"..")!= 0)
	  {
            //verificam daca acel director contine fisierul "treasures.dat"
            char path[512];
            snprintf(path,sizeof(path),"%s/treasures.dat",intrare->d_name);
            if(access(path,F_OK)!=0) //verifica daca fisierul treasures.dat exista in directorul curent...daca nu, sare peste acel hunt
	      {
                 continue;
	      }
            //cream un pipe pentru comunicare intre procesul parinte si copil
            int pfd[2]; //capetele : 0-citire, 1-scriere
	    int pid;
	    if(pipe(pfd)<0)
	      {
	 	 perror("Eroare la crearea pipe-ului!!");
		 exit(-1);
	      }
	    if((pid=fork())<0)
	      {
		 perror("Eroare la fork!!");
		 exit(-2);
	      }
            if(pid==0)
	      {
                //proces copil
                close(pfd[0]); //inchidem capatul de citire, copilul doar scrie
                int scor_partial=scor_din_hunt(intrare->d_name,user);

                //trimitem scorul partial prin pipe catre parinte
                if(write(pfd[1],&scor_partial,sizeof(scor_partial))==-1) //procesul copil incearca sa scrie scorul calculat in pipe...daca scrierea esueaza, afiseaza eroare
		  {
                     perror("Copilul nu a putut scrie in pipe!!");
		  }
		
                close(pfd[1]); //inchidem capatul de scriere
                exit(-3); //terminam procesul copil
	      }
            else
	      {
		//proces parinte
                close(pfd[1]); //inchidem capatul de scriere, parintele doar citeste

                int scor_primit=0;
                if(read(pfd[0],&scor_primit,sizeof(scor_primit))>0) //procesul parinte citeste din pipe scorul trimis de copil, daca citirea a reusit
		  {
                     total=total+scor_primit;
		  }
		else
		  {
                     perror("Parintele nu a putut citi din pipe!!");
		  }

                close(pfd[0]); // inchidem capatul de citire
                waitpid(pid,NULL,0); //asteptam ca procesul copil sa termine
	      }
	  }
      }
    closedir(dir);
    return total;
}

int main(int argc,char **argv)
{
    if(argc!=2)
      {
	 fprintf(stderr,"Trebuie scris : %s username!!\n",argv[0]);
         return 1;
      }

    char *username=argv[1];
    int scor_total=calculeaza_total(username);

    printf("Scorul total pentru user-ul '%s' este: %d!!\n",username,scor_total);
    return 0;
}
