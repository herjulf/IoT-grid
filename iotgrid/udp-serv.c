#include<stdio.h>
#include<string.h> 
#include<stdlib.h> 
#include <time.h>
#include <unistd.h>
#include<arpa/inet.h>
#include<sys/socket.h>


#define BUFLEN 512
#define PORT 5683

#define VERSION "1.0 2018-01-30"
#define WITH_COAP  13

#define D_COAP_PKT      (1<<0)
#define D_COAP_REPORT   (1<<1)
#define D_COAP_STRING   (1<<2)

unsigned int debug = 3;
int date, utime, utc;

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

void die(char *s)
{
    perror(s);
    exit(1);
}
 
int main(void)
{
    struct sockaddr_in si_me, si_other;
    int s, slen = sizeof(si_other) , recv_len;
    char buf[BUFLEN];
    struct coap_hdr *ch_rx;

    if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)  {
        die("socket");
    }
     
    memset((char *) &si_me, 0, sizeof(si_me));
     
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(PORT);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
     
    if( bind(s , (struct sockaddr*)&si_me, sizeof(si_me) ) == -1) {
        die("bind");
    }
     
    while(1)
    {
        printf("Reading data: ");
        fflush(stdout);
         
        if ((recv_len = recvfrom(s, buf, BUFLEN, 0, 
				 (struct sockaddr *) &si_other, &slen)) == -1) {
            die("recvfrom()");
        }
         
        printf("Got from %s:%d\n", inet_ntoa(si_other.sin_addr), 
	       ntohs(si_other.sin_port));
        printf("Data: %s\n" , buf);

	ch_rx = (struct coap_hdr*) &buf[0]; 

	if(debug & D_COAP_REPORT)
	  printf("%s\n", &buf[5+8]);  

	if(debug & D_COAP_PKT)
	  dump_pkt(ch_rx, recv_len);
         
        if (sendto(s, buf, recv_len, 0, (struct sockaddr*) &si_other, 
		   slen) == -1)  {
	  die("sendto()");
        }
    }
    close(s);
    return 0;
}
