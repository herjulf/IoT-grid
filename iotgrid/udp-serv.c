#include<stdio.h>
#include<string.h> 
#include<stdlib.h> 
#include <time.h>
#include <unistd.h>
#include<arpa/inet.h>
#include<sys/socket.h>

#define BUFLEN 512
#define PORT 5683

#define VERSION "1.0 2018-02-08"

#define D_COAP_PKT      (1<<0)
#define D_COAP_REPORT   (1<<1)
#define D_COAP_STRING   (1<<2)

#define BROKER_BASE_URI "</ps/>;rt=\"core.ps\";ct=40"
char *broker_base_uri = BROKER_BASE_URI;

#define MAX_URI_LEN 50
char uri[MAX_URI_LEN];

/* CoAP message types */
typedef enum {
  COAP_TYPE_CON,                /* confirmables */
  COAP_TYPE_NON,                /* non-confirmables */
  COAP_TYPE_ACK,                /* acknowledgements */
  COAP_TYPE_RST                 /* reset */
} coap_message_type_t;

/* CoAP request method codes */
enum {
COAP_GET = 1,
  COAP_POST,
  COAP_PUT,
  COAP_DELETE
  };

/* CoAP response codes */
typedef enum {
  NO_ERROR = 0,

  CREATED_2_01 = 65,            /* CREATED */
  DELETED_2_02 = 66,            /* DELETED */
  VALID_2_03 = 67,              /* NOT_MODIFIED */
  CHANGED_2_04 = 68,            /* CHANGED */
  CONTENT_2_05 = 69,            /* OK */
  CONTINUE_2_31 = 95,           /* CONTINUE */

  BAD_REQUEST_4_00 = 128,       /* BAD_REQUEST */
  UNAUTHORIZED_4_01 = 129,      /* UNAUTHORIZED */
  BAD_OPTION_4_02 = 130,        /* BAD_OPTION */
  FORBIDDEN_4_03 = 131,         /* FORBIDDEN */
  NOT_FOUND_4_04 = 132,         /* NOT_FOUND */
  METHOD_NOT_ALLOWED_4_05 = 133,        /* METHOD_NOT_ALLOWED */
  NOT_ACCEPTABLE_4_06 = 134,    /* NOT_ACCEPTABLE */
  PRECONDITION_FAILED_4_12 = 140,       /* BAD_REQUEST */
  REQUEST_ENTITY_TOO_LARGE_4_13 = 141,  /* REQUEST_ENTITY_TOO_LARGE */
  UNSUPPORTED_MEDIA_TYPE_4_15 = 143,    /* UNSUPPORTED_MEDIA_TYPE */

  INTERNAL_SERVER_ERROR_5_00 = 160,     /* INTERNAL_SERVER_ERROR */
  NOT_IMPLEMENTED_5_01 = 161,   /* NOT_IMPLEMENTED */
  BAD_GATEWAY_5_02 = 162,       /* BAD_GATEWAY */
  SERVICE_UNAVAILABLE_5_03 = 163,       /* SERVICE_UNAVAILABLE */
  GATEWAY_TIMEOUT_5_04 = 164,   /* GATEWAY_TIMEOUT */
  PROXYING_NOT_SUPPORTED_5_05 = 165,    /* PROXYING_NOT_SUPPORTED */

  /* Erbium errors */
  MEMORY_ALLOCATION_ERROR = 192,
  PACKET_SERIALIZATION_ERROR,

  /* Erbium hooks */
  MANUAL_RESPONSE,
  PING_RESPONSE
} coap_status_t;

/* CoAP header option numbers */
typedef enum {
  COAP_OPTION_IF_MATCH = 1,     /* 0-8 B */
  COAP_OPTION_URI_HOST = 3,     /* 1-255 B */
  COAP_OPTION_ETAG = 4,         /* 1-8 B */
  COAP_OPTION_IF_NONE_MATCH = 5,        /* 0 B */
  COAP_OPTION_OBSERVE = 6,      /* 0-3 B */
  COAP_OPTION_URI_PORT = 7,     /* 0-2 B */
  COAP_OPTION_LOCATION_PATH = 8,        /* 0-255 B */
  COAP_OPTION_URI_PATH = 11,    /* 0-255 B */
  COAP_OPTION_CONTENT_FORMAT = 12,      /* 0-2 B */
  COAP_OPTION_MAX_AGE = 14,     /* 0-4 B */
  COAP_OPTION_URI_QUERY = 15,   /* 0-255 B */
  COAP_OPTION_ACCEPT = 17,      /* 0-2 B */
  COAP_OPTION_LOCATION_QUERY = 20,      /* 0-255 B */
  COAP_OPTION_BLOCK2 = 23,      /* 1-3 B */
  COAP_OPTION_BLOCK1 = 27,      /* 1-3 B */
  COAP_OPTION_SIZE2 = 28,       /* 0-4 B */
  COAP_OPTION_PROXY_URI = 35,   /* 1-1034 B */
  COAP_OPTION_PROXY_SCHEME = 39,        /* 1-255 B */
  COAP_OPTION_SIZE1 = 60,       /* 0-4 B */
} coap_option_t;

/* CoAP Content-Formats */
typedef enum {
  TEXT_PLAIN = 0,
  TEXT_XML = 1,
  TEXT_CSV = 2,
  TEXT_HTML = 3,
  IMAGE_GIF = 21,
  IMAGE_JPEG = 22,
  IMAGE_PNG = 23,
  IMAGE_TIFF = 24,
  AUDIO_RAW = 25,
  VIDEO_RAW = 26,
  APPLICATION_LINK_FORMAT = 40,
  APPLICATION_XML = 41,
  APPLICATION_OCTET_STREAM = 42,
  APPLICATION_RDF_XML = 43,
  APPLICATION_SOAP_XML = 44,
  APPLICATION_ATOM_XML = 45,
  APPLICATION_XMPP_XML = 46,
  APPLICATION_EXI = 47,
  APPLICATION_FASTINFOSET = 48,
  APPLICATION_SOAP_FASTINFOSET = 49,
  APPLICATION_JSON = 50,
  APPLICATION_X_OBIX_BINARY = 51
} coap_content_format_t;


unsigned int debug = 3;
int date, utime, utc;

struct udp_hdr {
 unsigned short int sport;
 unsigned short int dport;/* default mode */ 
 unsigned short int len;
 unsigned short int csum;
};

struct coap_hdr {
  unsigned int tkl:4;
  unsigned int type:2;
  unsigned int ver:2;
  unsigned int code:8;
  unsigned short id;
};

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
  unsigned opt = 0, old_opt = 0;

  printf("COAP DUMP pkt lengh=%d\n", len); 
  printf("v=%u t=%u tkl=%u code=%u id=%x\n", ch->ver, ch->type, ch->tkl, ch->code, ch->id);


  for(i = 0; i < len; i++) {
    if(!i) 
      printf("[%3d]", i);

    printf(" %02x", d[i] & 0xFF);
    
    if(! ((i+1)%16) )
      printf("\n[%3d]", i);
  }
    printf("\n");

  printf("COAP DUMP Option/Payload\n"); 

  for(i = 4; i < len; i++) {
    unsigned olen;

    /* Option delta handling */
    opt = (d[i]>>4) & 0xF;

    if(opt > 12 ) {
      if(opt == 13) {
	i++;
	opt = d[i] + 13;
      }
      else if(opt == 14) {
	printf("UNTESTED OPT 14\n");
	i++;
	opt = d[i]<<8;
	i++;
	opt += d[i];
	opt += 269;
      }
      else if(opt == 15) {
	printf("PAYLOAD=%s\n", &d[i+1]);
	return;
      }
    }
    opt += old_opt;
    
    olen = (d[i]) & 0xF; 
    if(olen > 12 ) {
      if(olen == 13) {
	i++;
	olen = d[i] + 13;
      }
      else if(olen == 14) {
	printf("UNTESTED OLEN 14\n");
	i++;
	olen = d[i]<<8;
	i++;
	olen += d[i];
	olen += 269;
      }
      else if(olen == 15) {
	printf("ERR OPT FORMAT LEN=15\n");
      }
    }

    printf("Options: opt=%u, len=%u ", opt, olen);

    if( olen ) {
      if(opt == COAP_OPTION_URI_PATH) {
	unsigned ii;
	for(ii = 1; ii <= olen; ii++) 
	  printf("%c", d[ii+i]);
      }
      else if(opt == COAP_OPTION_URI_QUERY) {
	unsigned ii;
	for(ii = 1; ii <= olen; ii++) 
	  printf("%c", d[ii+i]);
      }
      else if(opt == COAP_OPTION_CONTENT_FORMAT) {
	printf("cf=%d", d[i+1]);
      }
      else if(opt == COAP_OPTION_MAX_AGE) {
	printf("Max-Age=%u", ((unsigned) d[i+1]<<8 + d[i+2]));
      }
    }
    printf("\n");

    old_opt = opt;
    i = i + olen;
  }
}

void die(char *s)
{
    perror(s);
    exit(1);
}
 
int do_packet(char *buf, char *uri, unsigned char type, unsigned char code, unsigned char *payload)
{
  int i, len = 0;

  struct coap_hdr *ch_tx;
  struct coap_opt_s *ch_os;
  struct coap_opt_l *ch_ol;
  char *xx = "\"core.ps\"";
  
  ch_tx = (struct coap_hdr*) &buf[0];
  ch_tx->ver = 1;
  ch_tx->type = type;
  ch_tx->tkl = 0;
  ch_tx->code = code;

  len = sizeof(struct coap_hdr);
  if(strlen(uri) <= 12) {
    ch_os = (struct coap_opt_s*) &buf[len];
    ch_os->delta = 11; /* COAP_OPTION_URI_PATH = 11 */
    ch_os->len = strlen(uri);
    len++;
    strcpy(&buf[len], uri); /* Short opt */
    len += strlen(uri);
    printf("SHORT delta=%d, len=%d\n", ch_os->delta, ch_os->len); 
  }
  else if(strlen(uri) > 12) {
    ch_ol = (struct coap_opt_l*) &buf[len];
    ch_ol->delta = 11;  /* Uri-Patch */
    ch_ol->flag = 13;   /* 1 byte extension */
    ch_ol->len = strlen(uri) - 13;
    len += 2;
    strcpy(&buf[len], uri); /* Long opt */
    len += strlen(uri);
    printf("LONG flg=%d , delta=%d, len=%d\n", ch_ol->flag, ch_ol->delta, ch_ol->len); 
  }

  ch_os = (struct coap_opt_s*) &buf[len];
  ch_os->delta = 1; /* COAP_OPTION_CONTENT_FORMAT = 12 */
  ch_os->len = 1;
  len++;
  buf[len] = APPLICATION_LINK_FORMAT; 
  len++;
  
  ch_os = (struct coap_opt_s*) &buf[len];
  ch_os->delta = 3; /* COAP_OPTION_URI_QUERY = 15 */
  ch_os->len = strlen(xx);
  len++;
  strcpy(&buf[len], xx); /* Short opt */
  len += strlen(xx);

  if(payload) {
    buf[len] = 0xff;
    len++;
    strcpy(&buf[len], payload);
    len += strlen(payload);
  }

      return len;
  }

  int main(void)
{
    struct sockaddr_in si_me, si_other;
    int s, slen = sizeof(si_other) , recv_len, send_len;
    char buf[BUFLEN];
    struct coap_hdr *co;
    //char *discover = "</ps/>;rt=core.ps";
    char *discover = "</ps/>";

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
      memset((char *) &buf, 0, sizeof(buf));

      if ((recv_len = recvfrom(s, buf, BUFLEN, 0, 
			       (struct sockaddr *) &si_other, &slen)) == -1) {
	die("recvfrom()");
      }
         
      printf("Got from %s:%d\n", inet_ntoa(si_other.sin_addr), 
	     ntohs(si_other.sin_port));

      co = (struct coap_hdr*) &buf[0]; 

      if(debug & D_COAP_PKT)
	dump_pkt(co, recv_len);

      if(co->ver != 1) {
	die("coap format");
      }

      /* DISCOVER */
      if((co->type == COAP_TYPE_CON) && (co->code == COAP_GET)) {
	send_len = do_packet(buf, discover, COAP_TYPE_ACK, CONTENT_2_05, broker_base_uri);
      }	

      /* CREATE */
      if((co->type == COAP_TYPE_CON) && (co->code == COAP_POST)) {
	send_len = do_packet(buf, discover, COAP_TYPE_ACK, CREATED_2_01, broker_base_uri);
      }	

      /* SUBSCRIBE -- PUT OR POST */
      if((co->type == COAP_TYPE_CON) && (co->code == COAP_PUT)) {
	send_len = do_packet(buf, discover, COAP_TYPE_ACK, CHANGED_2_04, NULL);
      }	

      if(debug & D_COAP_PKT)
	dump_pkt(co, send_len);


        if (sendto(s, buf, send_len, 0, (struct sockaddr*) &si_other, 
		   slen) == -1)  {
	  die("sendto()");
        }
    }
    close(s);
    return 0;
}

