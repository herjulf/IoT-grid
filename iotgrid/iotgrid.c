/*
 *  IoT grid control, monitor and daemon for KTH small grid project.
 *
 *  KTH IoT grid team. Project site:
 *  https://github.com/herjulf/IoT-grid.git
 *
 *  Robert Olsson  <robert@roolss.kth.se> 
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <sys/fcntl.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <libgen.h> /* basename */
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <curses.h>

#define VERSION "1.0 2013-05-07"

#define INV_IOTGRID     (1<<0)
#define INV_IOTGRIDD    (1<<1)

unsigned int invocation; 

void usage_iotgridd(void)
{
  printf("\niotgridd daemon and monitor program\n");
  printf("Version %s\n", VERSION);
  printf("iotgridd [-d mask] [-b]\n");
  printf(" Example. Reads all incoming CoAP reports: iotgridd\n");
  printf("iotgridd [-p 2000::121f:e0ff:fe12:1d04] [-t 1000]\n");
  printf(" Example. Reads state vector from specified IP address every one second");
}

void usage_iotgrid(void)
{
  printf("\niotgrid controller program\n");
  printf("Version %s\n", VERSION);
  printf("\niotgrid [-d mask] [-h] get|post addr uri [payload]\n");
  printf(" Example 1: iotgrid  get 2000::121f:e0ff:fe12:1d04 actuators/toggle\n");
  printf(" Example 2: iotgrid  get 2000::121f:e0ff:fe12:1d04 dc-dc/stateVector\n");
  printf(" Example 3: iotgrid  get 2000::121f:e0ff:fe12:1d04 dc-dc/controlParameters\n");
  printf(" Example 4: iotgrid  get 2000::121f:e0ff:fe12:1d04 .well-known/core\n");
  printf(" Example 5: iotgrid  post 2000::121f:e0ff:fe12:1d04 dc-dc/controlParameters Vref=12\n");
  exit(-1);
}

void usage(void)
{
  if(invocation & INV_IOTGRID) 
    usage_iotgrid();

  if(invocation & INV_IOTGRIDD) 
    usage_iotgridd();

  exit(-1);
}

#define PORT   5683 /* CoAP default */
#define BUZSIZ 1024

#define D_COAP_PKT      (1<<0)
#define D_COAP_REPORT   (1<<1)
#define D_COAP_STRING   (1<<2)

unsigned int debug = 0; /* default debug */

#define M_RAW           (1<<0)
#define M_GET           (1<<1)
#define M_POST          (1<<2)
#define M_PULL			 (1<<3)

unsigned int mode = 0; /* default mode */

int date, utime, utc, background = 0;

/* Time in millieseconds to pull the state vector */
unsigned long pull_interval;

struct sockaddr_in6 server_addr, client_addr;

#define MAX_URI_LEN 50
char uri[MAX_URI_LEN];
char payload[MAX_URI_LEN];

struct udp_hdr {
 unsigned short int sport;
 unsigned short int dport;/* default mode */ 
 unsigned short int len;
 unsigned short int csum;
};

#if WITH_COAP == 7
struct coap_hdr {
  unsigned int oc:4;
  unsigned int type:2;
  unsigned int ver:2;
  unsigned int code:8;
  unsigned short id; 
};
#elif WITH_COAP == 13
struct coap_hdr {
  unsigned int tkl:4;
  unsigned int type:2;
  unsigned int ver:2;
  unsigned int code:8;
  unsigned short id;
};
#else
#error "CoAP version defined by WITH_COAP is not supported"
#endif

/* length < 15 for CoAP-07 and length < 13 for CoAP-13*/
struct coap_opt_s {
  unsigned int len:4;
  unsigned int delta:4;
};

/* extended form, to be used when length==15 */
struct coap_opt_l {
  unsigned int flag:4;  /* either 15 or 13 depending on the CoAP version */
  unsigned int delta:4; /* option type (expressed as delta) */
  unsigned int len:8;   /* MAX_URI_LEN = 50 so one byte is enough for extended length */
};

void print_date(char *datebuf)
{
  time_t raw_time;
  struct tm *tp;
  char buf[256];

  *datebuf = 0;
  time ( &raw_time );

  if(utc)
    tp = gmtime ( &raw_time );
  else
    tp = localtime ( &raw_time );

  if(date) {
	  sprintf(buf, "%04d-%02d-%02d %2d:%02d:%02d ",
		  tp->tm_year+1900, tp->tm_mon+1, 
		  tp->tm_mday, tp->tm_hour, 
		  tp->tm_min, tp->tm_sec);
	  strcat(datebuf, buf);
  }
  if(utime) {
	  sprintf(buf, "UT=%ld ", raw_time);
	  strcat(datebuf, buf);
  }
}

void dump_pkt(struct coap_hdr *ch, int len)
{
  int i;
  char *d = (char *) ch; 

#if WITH_COAP == 7
  printf("v=%u t=%u oc=%u code=%u id=%x\n", ch->ver, ch->type, ch->oc, ch->code, ch->id);
#elif WITH_COAP == 13
  printf("v=%u t=%u tkl=%u code=%u id=%x\n", ch->ver, ch->type, ch->tkl, ch->code, ch->id);
#endif
  
  for(i = 0; i < len; i++) {
    if(!i) 
      printf("[%3d]", i);

    printf(" %02x", d[i] & 0xFF);
    
    if(! ((i+1)%16) )
      printf("\n[%3d]", i);
  }
    printf("\n");
}

/* Reads raw UDP COAP pkts */

int unsolicited_grid_reports(void)
{
  int sock, len;
  char addrbuf[INET6_ADDRSTRLEN];
  char buf[BUFSIZ];
  char outbuf[512];
  socklen_t clilen;
  struct coap_hdr *ch_rx;
  struct udp_hdr *uh;

  sock = socket(PF_INET6, SOCK_RAW, IPPROTO_UDP);
  
  if (sock < 0) {
      perror("creating socket");
      return -1;
  }
  
  memset(&server_addr, 0, sizeof(server_addr));
  memset(&client_addr, 0, sizeof(client_addr));
  clilen = sizeof(client_addr);
  
  while(1) {
    len = recvfrom(sock, buf, BUFSIZ, 0, (struct sockaddr *)&client_addr, 
		   &clilen);
    
    if (len < 0) {
      perror("recvfrom failed");
      return -1;
    }
    
    uh = (struct udp_hdr *) &buf[0];
    if(PORT != ntohs(uh->dport))  
      continue;
    
    ch_rx = (struct coap_hdr*) &buf[8]; /* UDP HDR len */
    
    print_date(outbuf);
    printf("%s ", outbuf);
    printf("Message from %s, len=%d\n", inet_ntop(AF_INET6, &client_addr.sin6_addr, 
						  addrbuf, INET6_ADDRSTRLEN), len);
    
    if(debug & D_COAP_REPORT)
      printf("%s\n", &buf[5+8]);  

    if(debug & D_COAP_PKT)
      dump_pkt(ch_rx, len);
  }    
  close(sock);
  return 0;
}

struct _sVector{
	char status[50];
	char vOut[50];
	char iOut[50];
	char vIn[50];
	char iIn[50];
	char prio[50];
} sVector;

int pull_state_vector(void)
{
    WINDOW * mainwin;
    int row, col;
//    char ch;
    char addrbuf[INET6_ADDRSTRLEN];

    /*  Initialize ncurses  */
	if ((mainwin = initscr()) == NULL){
		fprintf(stderr, "Error initialising ncurses.\n");
		exit(EXIT_FAILURE);
	}

    /*  Initialize colours  */
    start_color();

	init_pair(1,  COLOR_RED,     COLOR_BLACK);
	init_pair(2,  COLOR_GREEN,   COLOR_BLACK);
	init_pair(3,  COLOR_YELLOW,  COLOR_BLACK);
	init_pair(4,  COLOR_BLUE,    COLOR_BLACK);
	init_pair(5,  COLOR_MAGENTA, COLOR_BLACK);
	init_pair(6,  COLOR_CYAN,    COLOR_BLACK);
	init_pair(7,  COLOR_BLUE,    COLOR_WHITE);
	init_pair(8,  COLOR_WHITE,   COLOR_RED);
	init_pair(9,  COLOR_BLACK,   COLOR_GREEN);
	init_pair(10, COLOR_BLUE,    COLOR_YELLOW);
	init_pair(11, COLOR_WHITE,   COLOR_BLUE);
	init_pair(12, COLOR_WHITE,   COLOR_MAGENTA);
	init_pair(13, COLOR_BLACK,   COLOR_CYAN);

	strcpy(uri, "dc-dc/stateVector");
	inet_ntop(AF_INET6, &server_addr.sin6_addr, addrbuf, INET6_ADDRSTRLEN);

	while (1)
	{
//		fflush(stdout);
//		read(STDIN_FILENO, &ch, 1);
//		if (ch=='q')
//			break;
		explicit_query_control();

		color_set(7, NULL);
		getmaxyx(mainwin, row, col);
		mvprintw(2, (col - strlen("GRID MONITOR")) / 2, "%s", "GRID MONITOR");
		mvprintw(5, 10, "%s", "IP");
		mvprintw(5, 40, "%s", "STATE");
		mvprintw(5, 60, "%s", "V-OUT");
		mvprintw(5, 80, "%s", "I-OUT");
		mvprintw(5, 100, "%s", "V-IN");
		mvprintw(5, 120, "%s", "I-IN");
		mvprintw(5, 140, "%s", "PRIO");

		color_set(8, NULL);

		mvprintw(6, 10, "%s", addrbuf);
		mvprintw(6, 40, "%s", sVector.status);
		mvprintw(6, 60, "%s", sVector.vOut);
		mvprintw(6, 80, "%s", sVector.iOut);
		mvprintw(6, 100, "%s", sVector.vIn);
		mvprintw(6, 120, "%s", sVector.iIn);
		mvprintw(6, 140, "%s", sVector.prio);

		refresh();
		/* milieseconds */
		usleep(pull_interval*1000);
	}

    delwin(mainwin);
    endwin();
    refresh();

    return EXIT_SUCCESS;
}

int explicit_query_control(void)
{
  int i, j, sock, len;
  socklen_t clilen;

  struct coap_hdr *ch_rx, *ch_tx;
  struct coap_opt_s *ch_os;
  struct coap_opt_l *ch_ol;

  char buf[BUZSIZ];
  char addrbuf[INET6_ADDRSTRLEN];

  sock = socket(PF_INET6, SOCK_DGRAM, 0);

  if (sock < 0) {
    perror("creating socket");
    exit(1);
  }

  memset(&client_addr, 0, sizeof(client_addr));
  server_addr.sin6_family = AF_INET6;
  server_addr.sin6_port = htons(PORT);

  ch_tx = (struct coap_hdr*) &buf;
  ch_tx->ver = 1;
  ch_tx->type = 0;
#if WITH_COAP == 7
  ch_tx->oc = 1;
#elif WITH_COAP == 13
  ch_tx->tkl = 0;
#endif
  if((mode & M_GET) || (mode & M_PULL))
    ch_tx->code = 1;
  if(mode & M_POST)
    ch_tx->code = 2;

  ch_tx->id = 0xBEEF;

#if WITH_COAP == 7
  if(strlen(uri) <= 14) {
    ch_os = (struct coap_opt_s*) &buf[4];
    ch_os->delta = 9; // 9; /* Uri-Patch */
    ch_os->len = strlen(uri);
    strcpy(&buf[5], uri); /* Short opt */
    len = sizeof(struct coap_hdr) + strlen(uri) + 1;
  }
  else if(strlen(uri) > 14) {
    ch_ol = (struct coap_opt_l*) &buf[4];
    ch_ol->flag = 0xf;
    ch_ol->delta = 9; // 9; /* Uri-Patch */
    ch_ol->len = strlen(uri) - 0xf;
    strcpy(&buf[6], uri); /* Long opt */
    len = sizeof(struct coap_hdr) + strlen(uri) + 2;
  }
#elif WITH_COAP == 13
  if(strlen(uri) <= 12) {
    ch_os = (struct coap_opt_s*) &buf[4];
    ch_os->delta = 11; // 11; /* Uri-Patch */
    ch_os->len = strlen(uri);
    strcpy(&buf[5], uri); /* Short opt */
    len = sizeof(struct coap_hdr) + strlen(uri) + 1;
  }
  else if(strlen(uri) > 12) {
    ch_ol = (struct coap_opt_l*) &buf[4];
    ch_ol->flag = 0xd;
    ch_ol->delta = 11; // 11; /* Uri-Patch */
    ch_ol->len = strlen(uri) - 0xd;
    strcpy(&buf[6], uri); /* Long opt */
    len = sizeof(struct coap_hdr) + strlen(uri) + 2;
  }
#endif

  if(mode & M_POST) {
    strncat(&buf[len], payload, MAX_URI_LEN);
    len += strlen(payload);
  }

  if(debug & D_COAP_PKT)
    dump_pkt(ch_tx, len);

  /* now send a datagrauri */
  if (sendto(sock, ch_tx, len, 0,
             (struct sockaddr *)&server_addr,
	     sizeof(server_addr)) < 0) {
      perror("sendto failed");
      exit(4);
  }

  clilen = sizeof(client_addr);

  len = recvfrom(sock, buf, 1024, 0, (struct sockaddr *)&client_addr,
		 &clilen);

  if ( len < 0) {
      perror("recvfrom failed");
      return 1;
  }

  ch_rx = (struct coap_hdr*) &buf;

  if(ch_tx->id != ch_rx->id) {
    printf("Error: Wrong messaage ID\n");
    return 1;
  }

	if (mode & M_PULL)
	{
		memset(sVector.vIn, 0, sizeof(sVector.vIn));
		memset(sVector.vOut, 0, sizeof(sVector.vOut));
		memset(sVector.iIn, 0, sizeof(sVector.iIn));
		memset(sVector.iOut, 0, sizeof(sVector.iOut));
		memset(sVector.prio, 0, sizeof(sVector.prio));

		/* parsing the state vector (expected values are status[on/off], Vout, Iout, Vin, Iin, Priority)
		 * looks ugly though
		*/
		for (i = 5; i < len; i++)
		{
			if (buf[i] == 'O')
			{
				if (buf[i + 1] == 'N')
					strcpy(sVector.status, "ON");
				else if (buf[i + 1] == 'F')
					strcpy(sVector.status, "OFF");
				continue;
			}
			if (buf[i] == '\n')
			{
				j++;
				continue;
			}

			if ((buf[i] >= 0x30 && buf[i] <= 0x39) || buf[i] == '.')
			{
				char tmp[2];
				tmp[0] = buf[i];
				tmp[1] = '\0';
				switch (j)
				{
				case 1:
					strcat(sVector.vOut, tmp);
					break;
				case 2:
					strcat(sVector.iOut, tmp);
					break;
				case 3:
					strcat(sVector.vIn, tmp);
					break;
				case 4:
					strcat(sVector.iIn, tmp);
					break;
				case 5:
					strcat(sVector.prio, tmp);
					break;
				}
			}
		}
		close(sock);
		return 0;
	}

  printf("Message from %s, len=%d\n", inet_ntop(AF_INET6, &client_addr.sin6_addr,
					 addrbuf, INET6_ADDRSTRLEN), len);

  if(debug & D_COAP_PKT)
    dump_pkt(ch_rx, len);

  for(i = 5; i < len; i++)
    printf("%c", (buf[i] & 0xFF));
  printf("\n");
  
  close(sock);
  return 0;
}

int main(int ac, char *av[]) 
{
  int i;
  char *prog = basename (av[0]);
  
  if (strcmp(prog, "iotgridd") == 0) {
    invocation = INV_IOTGRIDD;
    background = 0;
    date = 1;
    utime = 0;
    utc = 0;
    mode = M_RAW;
  }

  else if (strcmp(prog, "iotgrid") == 0) {
    invocation = INV_IOTGRID;
    for(i = 1; i < ac; i++) {
      if (strcmp(av[i], "post") == 0)  {
	mode = M_POST;
	break;
      }

      if (strcmp(av[i], "get") == 0) {
	mode = M_GET;
	break;
      }
    }
    /* Pick up address */
    inet_pton(AF_INET6, av[i+1], &server_addr.sin6_addr);

    /* Pick up URI */
    if (strlen(av[i+2]) > MAX_URI_LEN) {
  	  printf("URI length exceeds the maximum. Maximum URI length is %d\n", MAX_URI_LEN);
  	  exit(0);
    }
    memset(uri, 0, sizeof(uri));
    strncpy((char *) uri, av[i+2], MAX_URI_LEN);

    /* Pick up payload if POST */
    if(mode & M_POST) {
      memset(payload, 0, sizeof(payload));
#if WITH_COAP == 7
      strcpy(payload, av[i+3]);
#elif WITH_COAP == 13
      sprintf(payload, "\xff%s", av[i+3]);
#endif
    }
  }

  for(i = 1; (i < ac) && (av[i][0] == '-'); i++) {

      if (strcmp(av[i], "-h") == 0) 
	usage();

      else if (strcmp(av[i], "-b") == 0) 
	background = 1;

      else if (strncmp(av[i], "-d", 2) == 0) {
	i++;
	debug = strtoul(av[i], NULL, 16);
      }

      else if (strncmp(av[i], "-p", 2) == 0) {
	i++;
    /* Pick up address */
    inet_pton(AF_INET6, av[i], &server_addr.sin6_addr);
	mode = M_PULL;
      }

      else if (strncmp(av[i], "-t", 2) == 0) {
	i++;
	pull_interval=strtoul(av[i], NULL, 10);
      }
  }

  if(background && !(mode & M_PULL)) {
    int i;

    if(getppid() == 1) 
      return 0; /* Already a daemon */

    i = fork();

    if (i < 0) 
      exit(1); /* error */

    if (i > 0) 
      _exit(0); /* parent exits */

    /* child */
          
    setsid(); /* obtain a new process group */
    for (i = getdtablesize(); i >= 0; --i) {
      close(i); /* close all descriptors */
    }

    i = open("/dev/null",O_RDWR); dup(i); dup(i); /* handle standard I/O */
    umask(027); /* set newly created file permissions */
    chdir("/"); /* change running directory */

  }

  if(mode & M_RAW) 
    unsolicited_grid_reports();

  if(mode & (M_GET | M_POST)) 
    explicit_query_control();

  if (mode & M_PULL){
    pull_state_vector();
  }
  exit(0);
}
