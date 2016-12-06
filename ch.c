/* ch.c --- Un shell pour les hélvètes.  

Réalisé par 
Ilia Chaikovskiy et Omar Halbouni

***Les sources pour les parties de code puisé d'internet sont cité
dans le rapport***
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h> 
#include <limits.h>

int execvp(const char *file, char *const argv[]);
char *strtok(char *str, const char *delim);
int strcmp(const char *s1, const char *s2);
pid_t fork(void);
pid_t waitpid(pid_t pid, int *status, int options);

void forkf(char** tokens, int* pipeLoc, int pipecount, int pp2, int* pipes){
	int i, pid = fork();
	if(pid == 0){
		if(pipecount == 1){
			dup2(pipes[1],1);
			for(i=0;i<2*pp2;i++)
				close(pipes[i]);
			if (execvp(tokens[0], tokens) == -1)	
				perror("Commande introuvable");
			exit(0);
		}
		else{
			forkf(tokens, pipeLoc, --pipecount, pp2, pipes);
			int diff;
			if(pipecount+2 < 4){
				diff = 4 - pipecount + 2;
				if(pipecount == 1)
					dup2(pipes[pipecount+2-diff], 0);
				else{
					dup2(pipes[pipecount+2-diff], 0);
					dup2(pipes[pipecount+2-diff+3], 1);
				}
			}
			else if(pipecount+2 == 4){
				if(pipecount == 1)
					dup2(pipes[4], 0);
				else{
					dup2(pipes[4], 0);
					dup2(pipes[7],1);
				}
			}
			else{
				diff = pipecount + 2 - 4;
				if(pipecount == 1)
				dup2(pipes[pipecount+2+diff], 0);
				else{
					dup2(pipes[pipecount+2+diff], 0);
					dup2(pipes[pipecount+2+diff+3], 1);
				}
			}
			for(i=0;i<2*pp2;i++)
				close(pipes[i]);
		if (execvp(tokens[pipeLoc[pipecount]],tokens+pipeLoc[pipecount]) == -1)	
				perror("Commande introuvable");
			exit(0);	
		}
			
	}
	else{
		if(pipecount == 1){
			dup2(pipes[0],0);
			if(pp2>1)
				dup2(pipes[3],1);
			for(i=0;i<2*pp2;i++)
				close(pipes[i]);
			if (execvp(tokens[pipeLoc[0]],tokens+pipeLoc[0]) == -1)	
				perror("Commande introuvable");
			exit(0);
		}
		else
			waitpid(pid, NULL, 0);		
	}
	
}

int main (void)
{

  	/* ¡REMPLIR-ICI! : Lire les commandes de l'utilisateur et les exécuter. */

  	int continuer;

  	do {
		char* cwd;
    	char buff[PATH_MAX + 1];
    	cwd = getcwd( buff, PATH_MAX + 1 );
  		fprintf (stdout, "~%s$ ", cwd);
  		char *ligne = NULL;
  		size_t taille_buffer = 0; //getline va determiner la taille du buffer				
  		getline(&ligne, &taille_buffer, stdin);

	  	/********************************
	  	*								*
	  	*  SEPARER LA LIGNE EN TOKENS   *
		*								*
		********************************/

  		int bufsize = 64, nbTokens = 0, star = 0, pipecount = 0;
  		int pipeTransform;
  		int in, out, savedstdin, savedstdout;
		int modifystdin = 0, modifystdout = 0;
  		int pipeLoc[10];

  		char *token; 
  		char **tokens_backup;
		char **tokens = malloc(bufsize * sizeof(char*));
		DIR *d;
		struct dirent *dir;
		
		if (!tokens) { 						
			fprintf(stderr, "error: allocation error\n");
			exit(0);
		}
		// On separe la phrase en mot, token est le pointeur qui pointe sur le premier element
		token = strtok(ligne, " \n");			

		while (token != NULL || pipeTransform) {
			int notValidArgument = 1;
			pipeTransform = 0;
			tokens[nbTokens++] = token;
			// Si on excede le buffer, aggrandir le array
			if (nbTokens >= bufsize) {			
	  			bufsize += 64;
	  			tokens_backup = tokens;
	  			tokens = realloc(tokens, bufsize * sizeof(char*));
	  			if (!tokens) {
					free(tokens_backup);
    				fprintf(stderr, "error: allocation error\n");
    				exit(0);
	  			}
			}
			// On suppose en partant que notre token n'est pas valide
			while(notValidArgument){
				// Cas nécessitant expansion d'arguments
				if(star && d){
	    			if((dir = readdir(d)) != NULL){
	    				token = dir->d_name;
						if(token[0] != '.')
							notValidArgument = 0;
					}
					else{
						star = 0;
      					closedir(d);
					}
				}
				else{
					/********************************
				  	*								*
				  	*     Vérification si token     *
			  	    *	     est *, <, > ou |       *
					*								*
					********************************/
					token = strtok(NULL, " \n");
					if(token != NULL)
					{
						if(strcmp(token,"*") == 0){
							star = 1;
							d = opendir(".");
						}
						else if(strcmp(token,"<") == 0){
							token = strtok(NULL, " \n");
							if(token != NULL)
								in = open(token, O_RDONLY); 
							modifystdin = 1;
						}
						else if(strcmp(token,">") == 0){
							token = strtok(NULL, " \n");
							if(token != NULL)
						out = open(token, O_WRONLY | O_TRUNC | O_CREAT, 
					S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);  
							modifystdout = 1;
						}
						else if(strcmp(token,"|") == 0){
							pipeLoc[pipecount++] = nbTokens+1;
							pipeTransform = 1;
							token = NULL;
							notValidArgument = 0;
						}
						else
							notValidArgument = 0;
					}
					else
						notValidArgument = 0;
				}	
				
			}
		}
		tokens[nbTokens] = NULL;

		// replace stdin with "in" file
		if(modifystdin)
		{
			savedstdin = dup(0);
			dup2(in, 0);
			close(in);
		}

		// replace stdout with "out" file
		if(modifystdout)
		{
			savedstdout = dup(1);
			dup2(out, 1);
			close(out);
		}

	  	/********************************
	  	*								*
	  	*     EXECUTE LES COMMANDES     *
		*								*
		********************************/

		// La commande est vide, la boucle continue		
		if (!tokens[0])						
			continuer = 1;

		// La commande est "exit", on sort de la boucle
		else if(!strcmp(tokens[0],"exit"))			
			continuer = 0;

		// La commande cd, commande interne du system
		else if(!strcmp(tokens[0],"cd"))
		{
			if (tokens[1] == NULL)
    			fprintf(stderr, "error: expected argument to \"cd\"\n");
  			else if (chdir(tokens[1]) != 0) 
      			perror("error");
      		continuer = 1;
  		}
  					
		else if(pipecount){
			int cpid;
			int pipes[pipecount*2], h;
					
			int pid = fork();

			if(pid == 0){
				for(h = 0; h < pipecount; h++)
					pipe(pipes + 2*h);

				cpid = fork();

			    if(cpid == 0) {

			      	dup2(pipes[1], 1);
			      	close(pipes[0]);
			      	close (pipes[1]);
			      	if (execvp(tokens[0], tokens) == -1)	
						perror("Commande introuvable");
					exit(0);
			    } else if(cpid>0) {	
					dup2(pipes[0],0);
					close(pipes[0]);
					close(pipes[1]);
			      	execvp(tokens[pipeLoc[0]], tokens+pipeLoc[0]);
			    }
		  	} else if (pid > 0) {
		   		waitpid(pid, NULL, 0);
		   		continuer = 1;
		  	}/*
			int pipes[pipecount*2], h;
			for(h = 0; h < pipecount; h++)
				pipe(pipes + 2*h);

			int pid = fork();

			if(pid == 0){
				forkf(tokens, pipeLoc, pipecount, pipecount, pipes);
		  	} else if (pid > 0) {
		   		waitpid(pid, NULL, 0);
		   		for(h = 0; h <2*pipecount; h++)
					close(pipes[h]);
		   		continuer = 1;
		  	}*/
		}	

		else{	

			pid_t pid;
			int status;
			pid = fork();

	  		if (pid == 0) {					// Enfant
    			if (execvp(tokens[0], tokens) == -1)		// Erreur, execvp() retourne -1 seulement si elle ne trouve par la commande, dans ce cas on tue l'enfant
      				perror("Commande introuvable");
	    		exit(0);	
	  		} 
	  		else if (pid < 0)					// Erreur, car fork() doit donner 0 ou un entier > 0
	    		perror("error");
	  		else					// Parent
    			do{
    				waitpid(pid, &status, WUNTRACED);		// Parent est mis en attente pendant l'execution de l'enfant
    				// replace stdin with "in" file
					if(modifystdin)
						dup2(savedstdin, 0);

					// replace stdout with "out" file
					if(modifystdout)
						dup2(savedstdout, 1);		
    			}
    			while (!WIFEXITED(status) && !WIFSIGNALED(status));
	  		continuer = 1;						// continuer = 1, on resume la boucle pour la prochaine entree de commande
		}

  	} while(continuer);						// Continuer tant que continuer est différent de zéro.

  	fprintf (stdout, "Bye!\n");
  	exit (0);
}
