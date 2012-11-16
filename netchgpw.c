/**************************************************
Author: Giovambattista Vieri
e-mail: me@ziogianni.com
License: GPL V.2.0
Copyrigth (C)  Giovambattista Vieri all right reserved
Copyrigth (C)  Giovambattista Vieri tutti i diritti riservati
Version: 0.1

WARNING: THIS SOFTWARE IS IN ALPHA STATUS. 
No guarantee, it is only a demo, maybe the best description is "proof of concept" . It has surely some errors, and can seriously harm the integrity and security of your computer or computers. 
IT MUST NOT BE USED ON PRODUCTION COMPUTERS.
IT WILL MODIFY YOUR SHADOW PASSWORDS FILES. PAY ATTENTION. READ THE CODE. IT IS SURELY BUGGY. 


NetChgPw 
it will allow to change a user password via a tcp connection. So you can realize a web page to check and modify a linux user password. 

It will use two configuration files: 
- authuser 
- netchfpw.conf

These files must be located in the directory that contains the executable. 
- authuser is the list of users that are not allowed to change password. 
- conf.txt is the application configuration file. 

netchfpw.conf: 
bindaddress		ip to be used for binding default 127.0.0.1
bindport		port to be used for binding default 3000
group			group to be used
user 			user to be used 
groupthatcanconnect	group that can connect to this program

only bindaddress and bindport are used in this version 

Warning future version will need to be setuid enabled. 


*/

#include <stdio.h>
#include <stdlib.h> 
#include <stdarg.h>
#include <shadow.h>
#include <sys/types.h>
#include <pwd.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <errno.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <time.h>
#include <syslog.h>
#include <signal.h>

#define PW_MAX_LEN		14
#define USER_NOT_FOUND		1
#define PASSWD_NOT_MATCH	2
#define PWD_FILE 		"/etc/shadow"
#define OLD_PWD_FILE 		"/etc/shadow~"
#define CONFIG_FILE 		"netchfpw.conf"
#define DEFAULT_USER		"nobody"
#define DEFAULT_GROUP		"nobody"
#define AUTH_USER_FILE  	"authuser"
#define CONF_PATH		"."
#define MAXLINE 		3000 
#define DEF_SERVER_PORT 	3000
#define LISTENQ			5
#define PROGRAM_NAME		"netchgpw"
#define CONF_STRING_LEN		100

uid_t	initial_r_uid; /* real uid */
uid_t	initial_e_uid; /* effective uid */
gid_t	initial_r_gid;
gid_t	initial_e_gid;

struct conf {
	char bindaddress[CONF_STRING_LEN];
	char bindport[CONF_STRING_LEN];
	char group[CONF_STRING_LEN];
	char user[CONF_STRING_LEN];
	char groupThatCanConnect[CONF_STRING_LEN];
} configuration ;


char **userAllowed;   //data structure as an array with the latest item at zero
int  maxUserAllowed=0; 

/********************************************************
* configuration file processing method 
* BEGIN 
*/

void setBindAddress(char *buffer) {
	strncpy(buffer,configuration.bindaddress,CONF_STRING_LEN-1);
}

void setBindPort(char *buffer) {

	strncpy(buffer,configuration.bindport,CONF_STRING_LEN-1);
}


void setGroup(char *buffer) {
	strncpy(buffer,configuration.group,CONF_STRING_LEN-1);
}

void setUser(char *buffer) {
	strncpy(buffer,configuration.user,CONF_STRING_LEN-1);
}

void setGroupThatCanConnect (char *buffer) {
	strncpy(buffer,configuration.groupThatCanConnect,CONF_STRING_LEN-1);
}


struct confProc {
	char *token;
	void (*procToken)(char *);
};


struct confProc confProcedures[]={ {"bindaddress",setBindAddress},{"bindport",setBindPort},{"group",setGroup},{"user",setUser},{"groupthatcanconnect",setGroupThatCanConnect}, {NULL,NULL} };

/*************************************/
/* END */
/*************************************/

void sig_chld(int signo) 
{
	pid_t	pid;
	int	stat;
	char 	line[MAXLINE];

	while ((pid=waitpid(-1, &stat,WNOHANG)) >0) {
		sprintf(line,"child %d terminated",pid);
		syslog(LOG_INFO, line);	
	}
	return ; 
}

/*************************************/

void readUserFile(void) {
FILE *fi; 
char buffer[300];

strcpy(buffer,CONF_PATH);
strcat(buffer,"/");
strcat(buffer,AUTH_USER_FILE);

fi = fopen(buffer, "rb");
 
	if (fi != NULL) {
		maxUserAllowed = 0;
		userAllowed=malloc(1*sizeof(char*));
		while (fgets(buffer,100,fi)) {
			buffer[strlen(buffer)-1]='\0';
			*(userAllowed+maxUserAllowed)=malloc(strlen(buffer)+1);
			strcpy(*(userAllowed+maxUserAllowed),buffer);
			userAllowed=realloc(userAllowed,++maxUserAllowed);
		}
		fclose(fi);
	} else {
		syslog(LOG_INFO, "%s", "cannot read user file");

	}

}

/*************************************/

void dropPrivilege() {


}

/*************************************/

void upPrivilege() {



}

/*************************************/

void daemonize(void)
{
    pid_t pid, sid;

    /* already a daemon */
    if ( getppid() == 1 ) return;

    /* Fork off the parent process */
    pid = fork();
    if (pid < 0) {
        exit(EXIT_FAILURE);
    }
    /* If we got a good PID, then we can exit the parent process. */
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    /* At this point we are executing as the child process */

    /* Change the file mode mask */
    umask(0);

    /* Create a new SID for the child process */
    sid = setsid();
    if (sid < 0) {
        exit(EXIT_FAILURE);
    }

    /* Change the current working directory.  This prevents the current
       directory from being locked; hence not being able to remove it. */
    if ((chdir("/")) < 0) {
        exit(EXIT_FAILURE);
    }

    /* Redirect standard files to /dev/null */
    freopen( "/dev/null", "r", stdin);
    freopen( "/dev/null", "w", stdout);
    freopen( "/dev/null", "w", stderr);
}

/*************************************/

void houseKeeping() {
FILE *fi;
char buffer[300];
char line[MAXLINE];
char tok[MAXLINE];
char val[MAXLINE];

	initial_r_uid=getuid();
	initial_e_uid=geteuid();

	initial_r_gid=getgid();
	initial_e_gid=getegid();
	openlog(PROGRAM_NAME, LOG_NDELAY, LOG_USER);
	syslog(LOG_INFO, "%s", "starting");

	strcpy (configuration.bindaddress,"127.0.0.1");
	strcpy (configuration.bindport,"3000");
	strcpy (configuration.group,"nobody");
	strcpy (configuration.user,"nobody");
	strcpy (configuration.groupThatCanConnect,"nobody");

/* open configuration file */

	strcpy(buffer,CONF_PATH);
	strcat(buffer,"/");
	strcat(buffer,CONFIG_FILE);

	if(fi=fopen(buffer,"r")) {
/* read an process it */
	while(fgets(line,MAXLINE-1,fi)) {
		if((line[0]='#') || (strlen(line)<5)) {
			// line discharged
			sprintf(line,"line discharged %s\n",line );
			syslog(LOG_INFO, line);
			
		} else  { 
			sscanf(line,"%s %s",tok,val);	
			struct confProc *cnfPrcpt=confProcedures;
			while (cnfPrcpt->token!=NULL) {
				if (strcpmp(cnfPrcpt->token,tok)==0 ) {
					cnfPrcpt->procToken(val);					}
				cnfPrcpt++;

			}
		}


	
	}
	fclose(fi);
	}
/* close it */
}

/*************************************/

int checkPassword(char *user, char *password) {
struct spwd 	*pspwd; 
char 		*epasswd;
int		ret=99;
uid_t		uid;
int		status;
	uid=getuid();	
	status=setuid(uid);
	pspwd=getspnam(user); 

	if (pspwd == NULL ) {
		ret=USER_NOT_FOUND;
		syslog(LOG_INFO, "User %s not FOUND", user);
	} else {
		epasswd = (char *) crypt( password, pspwd->sp_pwdp);
		if (strncmp ( epasswd, pspwd->sp_pwdp,PW_MAX_LEN)!=0){ 
			ret=PASSWD_NOT_MATCH;
			syslog(LOG_INFO, "User %s password does not match", user);
		} else {
			ret=0;
		}
			
	}
	setuid(uid);
return(ret);
}

/*************************************/

int changePassword(char *username, char *password) {
struct spwd 	*pspwd; 
char 		*epasswd;
int		ret=99;
FILE 		*old, *new;
struct spwd 	*spw_orig, spw_copy;
int 		retval;
uid_t		uid;
int		status;
int		dummy;
	uid=getuid();	
	status=setuid(0);
	dummy=errno;
	pspwd=getspnam(username); 

	if (pspwd == NULL ) {
		ret=USER_NOT_FOUND;
	} else {
		epasswd = (char *) crypt(password, pspwd->sp_pwdp);
/*******************************************/
		lckpwdf();
		link("/etc/shadow", "/etc/shadow~");
		unlink("/etc/shadow");
		
		old = fopen("/etc/shadow~", "r");	
		new = fopen("/etc/shadow", "w");
		spw_orig = fgetspent(old);
		while (spw_orig != NULL) {
			if (strcmp(username, spw_orig->sp_namp) == 0) {
				memcpy(&spw_copy, spw_orig, sizeof(struct spwd));
				spw_copy.sp_pwdp = epasswd;
				retval=putspent(&spw_copy, new);
				/* retval must be checked */
				memset(&spw_copy, 0, sizeof(struct spwd));
			} else { 
				retval=putspent(spw_orig, new);
				/* retval must be checked */
			}
			spw_orig = fgetspent(old);
		}
		fclose(old);
		fclose(new);
		unlink("/etc/shadow~");
		ulckpwdf();
	}
	setuid(uid);

return(ret);
}

/*************************************/

int isUserAllowed(char *user) {
int i; 
	for (i=0; i<maxUserAllowed ; i++) {
		if(strcmp(user,*(userAllowed+i))==0 ) {
			return(0);
		}
	}
	return(1);
}

/*************************************/

void web_child(int connfd){
int     ntowrite;
ssize_t nread;
char    line[MAXLINE];
struct spwd *pspwd; 
char *epasswd;
char *pusername, *oldpwd, *newpwd1, *newpwd2;
char *p;
int lennewpwd1;
int lennewpwd2;
char username[MAXLINE];
char pwd[MAXLINE];
char pwd1[MAXLINE];
char pwd2[MAXLINE];
int dummy;

/* now get the command line */ 
	read (connfd, &line, MAXLINE-1);
	// only paranoid will survive so we will check
	// \0 final
	// \n 
	// other characters between 32 and 127 ...


	//if we are here  
	pusername = strtok (line,";");	
	strcpy (username,pusername);
	oldpwd = strtok (NULL, ";");
	strcpy (pwd,oldpwd);
	newpwd1 = strtok (NULL, ";");
	strcpy (pwd1,newpwd1);
	newpwd2 = strtok (NULL, ";");
	for(p=newpwd2;*p!='\0';p++) { 
		if(*p=='\n') { *p='\0'; break; }
	}
	strcpy (pwd2,newpwd2);
	// now test the strings for dangerous characters... 

	syslog (LOG_INFO, "username:%s\n\n",username);
	syslog (LOG_INFO, "oldpwd:%s\n\n",pwd);
	syslog (LOG_INFO, "newpwd1:%s<-",pwd1);
	syslog (LOG_INFO, "newpwd2:%s<-",pwd2);

/* process it  */
	// newpwd1 and newpwd2 must be equal
	lennewpwd1=strlen(pwd1);
	lennewpwd2=strlen(pwd2);
	syslog (LOG_INFO, "len newpwd1:%d",lennewpwd1);
	strcpy (line,"400 unknown error");
	if (isUserAllowed(username)==0 ) {
		sprintf (line,"401 user %s not allowed",username );
		syslog(LOG_INFO, line);
	} else if ( lennewpwd1 != lennewpwd2 ) {
		// length of passwordw does not coincide
		sprintf (line, "402 different password length");
		syslog(LOG_INFO, line);
	} else if( strncmp(pwd1,pwd2,lennewpwd1)!=0 ) {
		sprintf (line, "403 different passwords for %s",username);
		syslog(LOG_INFO, line);
	} else if( checkPassword(username,pwd) != 0) {
		sprintf (line, "404 wrong old password");
		syslog(LOG_INFO, line);
	} else  {
		dummy=changePassword(username,pwd1);
		sprintf (line, "return value %d of changePassword %s pwd=%s",dummy,username,pwd1);
		syslog(LOG_INFO, line);
		sprintf (line,"200 password probably changed for user %s",username);
		syslog(LOG_INFO, line);
	}
	write(connfd,line,strlen(line)); 	
}

/*************************************/

int  main (int argc, char *argv)  {
int listenfd;
int connfd;
int n;
socklen_t clilen;
socklen_t addrlen;
char buf[MAXLINE];
struct sockaddr_in servaddr;
struct sockaddr cliaddr;
pid_t   childpid;
int i;

	houseKeeping();

/* 
read the authorized user file 
will not proceed if the given user is not in the authorized file
*/

	readUserFile();
	daemonize();
/* socket */

//creation of the socket
	listenfd = socket (AF_INET, SOCK_STREAM, 0);

//preparation of the socket address
	servaddr.sin_family=AF_INET;
//servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
	servaddr.sin_addr.s_addr=inet_addr(configuration.bindaddress);
	servaddr.sin_port=htons(atoi(configuration.bindport));

	bind(listenfd,(struct sockaddr *) &servaddr, sizeof(servaddr));
	listen(listenfd,LISTENQ);

	syslog(LOG_INFO,"Server running...waiting for connections.");

	signal(SIGCHLD,sig_chld);
/* accept , cycle , then fork exec */
	for ( ; ; ) {
		 clilen = addrlen = sizeof(struct sockaddr_in );
		 if  ( (connfd = accept(listenfd, &cliaddr, &clilen)) < 0) {
			syslog(LOG_INFO, "accept <0");
			if (errno == EINTR)
				continue;      /* back to for() */
			else {
				syslog(LOG_INFO, "accept error");
				continue;
			}	
		 }

		 if ( (childpid = fork()) == 0) { /* child process */
		     close(listenfd);     /* close listening socket */
		     web_child(connfd);   /* process request */
	//	     close(connfd);
		     exit(0);
		 }
		 close(connfd);            /* parent closes connected socket */
	}
}






