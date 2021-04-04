#undef AN_SEM

#ifndef GRUPSTAT
	#define DEBUG
#else
	#undef DEBUG
	#ifdef AN_SEM
		#define MAUSPFAD "C:\\CAT\\"
		#define GRUPPE "OBERLEHRER"
	#else
		#define GRUPPE "FFM.TALK"
		#define MAUSPFAD "C:\\MAUS\\OF\\"
	#endif
#endif

#define min(x,y) (x<y?x:y)

#define CATVER 1
					  /* 0 - 1.21, 1 - >=1.3nû */
#if (CATVER==1)
	#define VERSION "Version 1.31 fÅr 2.x-Datenbank"
#else
	#define VERSION "Version 1.31 fÅr 1.21-Datenbank"
#endif

#define MAXGRP 512
#define MAXGRPCHARS 50
#define MAXBOXCHARS 60
#define MAXIDCHARS 256
#define MAXATCHARS 50
#define MAXNAMECHARS (15+MAXATCHARS)
#define MAXTOPICCHARS 256

#define WATERMARK 65900L

#include <stdio.h>

#ifndef PORTABEL
# include <tos.h>
# include <ext.h>
# define Fsetvbuf(a,b,c,d) setvbuf(a,b,c,d)
#else
# define Fopen(a,b) fopen(a,b)
# define FO_READ "rb"
# define FO_WRITE "wb"
# define Fread(a,b,c) fread(c,1,b,a)
# define Fwrite(a,b,c) fwrite(c,1,b,a)
# define Fclose(a) fclose(a)
# define Fsetvbuf(a,b,c,d) 0
# define Fprintf fprintf
#endif

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>

#ifndef GRUPSTAT
# ifdef USESEQ
#  include "seqfile.h"
# else
#  define SEQ_FILE FILE
#  define SEQ_READ "rb"
#  define SEQ_WRITE "wb"
#  define seqopen(a,b,c) fopen(a,b)
#  define seqclose(a) fclose(a)
#  define seqputs(a,b) fputs(a,b)
#  define seqgets(a,b) fgets(a,b)
#  define seqtell(a) ftell(a)
#  define seqseek(a,b,c) fseek(a,b,c)
# endif
#endif

unsigned short crc16 (char *text);

#define TRUE (0==0)
#define FALSE (!TRUE)

typedef struct duplist
{
	struct duplist *next;
	char dupat[MAXATCHARS+1];
} duplist;

typedef struct
{
	char name[MAXNAMECHARS+1];
	duplist *dupes;
	long msgs;
	long bytes;
} person;

typedef struct
{
	unsigned long maxdatum;
	long anzahl;
	long um[24];
} tag;

#if(CATVER==0)
typedef struct
{
	unsigned int nummer;		/* interne Messagenummer	*/
	char wegen[31];				/* Betreff					*/
	unsigned long datum;		/* Datum im Maustauschformat*/
	long start;					/* Offset in der Datendatei */
	unsigned int idLength,		/* LÑnge der ID				*/
	NameL,						/* LÑnge des Absendernamens */
	Length;						/* LÑnge der Mitteilung		*/
	char bits;					/* : BITSET					*/ 
	unsigned int	upMess,		/* Verkettungen				*/
		downMess;
/*		rightMess,
		leftMess,
		KomCount;				/* Anzahl Kommentare		*/
interessiert mich eh nicht, nur bei PMs brauche ich die entsprechenden Infos
*/

	unsigned long lesedatum;
	char status, locked;

} parblk;

#elif (CATVER==1)
typedef struct
{
	unsigned short crc;
	unsigned long datum; /* Datum im Maustauschformat */
	unsigned short items;
	unsigned short bits;
	unsigned short hLength;
	unsigned short idLength;
	unsigned short Length;
	long start; /* Offset in der Datendatei  */

	unsigned int	upMess,
					downMess,
					rightMess,
					leftMess,
					KomCount;
} parblk;

typedef struct
{
	char text[4];
	long magic;
} catbeg;

typedef struct
{
	short typ;		/* 0 ôM verkettet 1 PM verkettet 2 ôM neu 3 PM neu */
	unsigned short msgnum;
	long unknown;
	short versenden;/* 0 ZurÅckhalten 1 Versenden */
	char unknown2[6];
	unsigned short msglen;	/* DateilÑnge "MSG"msgnum".TXT" */
	char unknown3[16];
} infodat1;
#endif

#ifndef GRUPSTAT
typedef struct
{
	unsigned long lesedatum;
	char status, locked;
} privblk;
#endif

typedef struct
{
	char *buf;
#ifdef PORTABEL
	FILE *handle;
#else
	int handle;
#endif
	long buflen;
	long len;
	long lastseek;
	long highwater;
	long seekpos;
	int fail;
	int eof;
} filebuf;

#if (CATVER<1)
filebuf par;
#endif

filebuf dat, linpar;

#ifndef GRUPSTAT
SEQ_FILE *outfile;
char bufseq[16400];
#endif

FILE *file;

#ifdef GRUPSTAT
FILE *grupfile;
#endif

char buf[16400];
char gruppen[(long)MAXGRP][MAXGRPCHARS+1L];

parblk msg;
person *info;
person **psort;
unsigned int maxpers;

#ifndef GRUPSTAT
long seqbufsiz=32000L;
#endif

long schreiber=0;
long bytes=0;
long msgs=0;

int dmq=0, pms=0, mitvon=0, alle=1, flagsex=0, statusex=0, adrex=0, headerex=1;
int out=0, novon=0, unique=0, fast=0, taste=0;

char outname[128]="", username[128]="";
int unatpos=0;

unsigned short nichtflags=0, mussflags=0;
char betreff[256];
char statdatname[128];
int betrlen;

int parfail=0;
long parpos;

tag wtag[7];
long summe[24];

char daystr[7][3]={"So","Mo","Di","Mi","Do","Fr","Sa"};

time_t now, monday;
#ifndef GRUPSTAT
time_t aktday;
#endif
struct tm *ztm;

char database[128], ausgabe[128]="";

unsigned long mindatum,maxdatum;
char mins[15],maxs[15];

int vergleich(person *p1, person *p2)
{
	if(p1->msgs==p2->msgs)
		if(p1->bytes==p2->bytes)
			return 0;
		else
			return (p1->bytes>p2->bytes ? 0 : 1);
	else
		return (p1->msgs>p2->msgs ? 0 : 1);
}

void shellsort(person **arr, long anzahl, int (*compar)(person *p1, person *p2))
{
	long gap, i, j;
	person *temp;

	for(gap=anzahl>>1; gap>0; gap>>=1)
		for(i=gap; i<anzahl; i++)
			for(j=i-gap; j>=0 && (*compar)(arr[j], arr[j+gap]); j-=gap)
			{
				temp=arr[j];
				arr[j]=arr[j+gap];
				arr[j+gap]=temp;
			}
}

int alphabetisch(person *p1, person *p2)
{
	return (strcmpi(p1->name, p2->name)>0);
}

void init(void)
{
	long i;

	for(i=0;i<maxpers;i++)
	{
		info[i].name[0L]=0;
		info[i].dupes=NULL;
	}
}

#if (CATVER==1)
int headerokP(filebuf *f)
{
	char buf[20];

	if(Fread(f->handle, 8, buf)!=8)
		return FALSE;
	f->seekpos=8;

	if(memcmp(buf, "CAT ",4))
		return -121;

	return (memcmp(buf,"CAT \0\1\1\43",8) ? -2:0);
}
#endif

#ifdef GRUPSTAT
int schonzeitp(void)
{
	char *fehler;

	grupfile=fopen("grupstat.dat","r");

	if(grupfile!=0)
	{
		do
		{
			fehler=fgets(buf,100,grupfile);
			if(fehler==NULL)
			{
				printf("Achtung: Gruppe %s nicht in grupstat.dat gefunden\n", GRUPPE);
				getchar();
				return -1;
			}
		} while(strnicmp(GRUPPE, buf, strlen(GRUPPE)));

		maxdatum=atol(buf+strlen(GRUPPE)+1);
		fclose(grupfile);
		if(mindatum<=maxdatum)
		{
			return FALSE;
		}
	}

	return TRUE;
}
#else
long getdatum(char *s)
{
	long summe,i;
	char *monat, *jahr;

/* Hier soll auf Dauer ein relatives Datum hin
	if(!strchr(s,'.'))
	{
		summe=atol(s);
		
	}
*/

	monat=strchr(s,'.')+1;
	if(monat==(char *)1L)
		return 0;
	jahr=strchr(monat,'.')+1;
	if(jahr==(char *)1L)
		return 0;

	i=atol(jahr);
	if(i>1900)
		i-=1900;
	if(i>=90)
		i-=90;
	summe=i*100;

	i=atol(monat);
	summe+=i;
	summe*=100;

	i=atol(s);
	summe+=i;
	summe*=10000;

	if(strchr(jahr, ':')) /* Nicht irritieren lassen: Uhrzeit */
		summe+=atol(strchr(jahr, ':')+1)%10000;

	return summe;
}
#endif

#define DORTDAT(wo) ((unsigned char *)(dat.buf+(wo-dat.lastseek)))
#define DORTPAR(wo) ((unsigned char *)(par.buf+(wo-par.lastseek)))
#define DORTLIN(wo) ((unsigned char *)(linpar.buf+(wo-linpar.lastseek)))

#define DORTWDAT(was,wo) was=((*DORTDAT(wo)<<8) + *(DORTDAT(wo)+1));

#define CACHEDAT(castart)\
	if(cache(castart, &dat))					\
	{											\
		printf("Seek in Dat ging schief!\n");	\
		return -1;								\
	}

#define CACHEPAR(castart) (cache(castart, &par))
#define CACHELIN(castart) (cachelin(castart))

int cachelin(long castart)
{
	if(!linpar.eof && linpar.lastseek+linpar.len<castart+linpar.highwater)
	{
		if(linpar.seekpos!=castart &&
				Fseek(castart, linpar.handle, SEEK_SET)!=castart)
		{
			printf("Seek in Par auf %ld ging nicht\n", castart);
			return -1;
		}

		if((linpar.len=Fread(linpar.handle, linpar.buflen, linpar.buf))<=0)
		{
			if(linpar.len<0)
				printf("Par-Read ging nicht :-(\n");
			else
				linpar.eof=1;
			return -1;
		}
		linpar.seekpos=castart+linpar.len;
		if(linpar.len<linpar.buflen)
			linpar.eof=1;

		linpar.lastseek=castart;
	}
	if(castart<linpar.lastseek+linpar.len)
		return 0;
	else
		return -1;
}

int cache(long castart, filebuf *f)
{
	if(f->lastseek>castart
	  || (f->lastseek+f->len<castart+f->highwater && !f->eof)
	  || f->len==0)
	{
		printf(":");
		if(f->lastseek>castart)
		{
			/* tlen: Wieviel soll gelesen werden */
			/* flen: Wieviel wurde gelesen */
			long tlen, flen;

			if(f->eof)
			{
				tlen=f->lastseek-castart;
				if(f->buflen-f->len>=tlen)
				{
					tlen=f->buflen-f->len;
					castart=f->lastseek-tlen;
					if(castart<0)
					{
						tlen+=castart;
						castart=0;
					}
				}
				else
				{
					f->eof=0;
				}
			}
			else
			{
				tlen=f->lastseek-castart;
			}

			if(castart<0)
			{
				castart=0;
			}

			if(tlen>f->buflen)
			{
				tlen=2*f->highwater;
				if(tlen>f->buflen)
					tlen=f->buflen;
			}

			if(f->seekpos!=castart &&
						Fseek(castart, f->handle, SEEK_SET)!=castart)
			{
				printf("Fehler in %s\n", f==&dat?"DAT":"CACHE");
				printf("Seek Typ 1 auf %ld ging nicht :-(\n",
					 castart);
				return -1;
			}

			printf("(");

			if(castart+tlen==f->lastseek)
				memcpy(f->buf+tlen, f->buf, f->buflen-tlen);

			if((flen=Fread(f->handle, tlen, f->buf))<0)
			{
				printf("Read ging nicht :-(\n");
				f->len=0;
				return -1;
			}
			f->seekpos=castart+flen;

			if(flen==tlen && castart+tlen==f->lastseek)
				f->len+=tlen;
			else
				f->len=flen;

			if(flen!=tlen)
			{
				f->eof=1;
			}

			if(f->len>f->buflen)
				f->len=f->buflen;
		}
		else if(f!=&linpar && !f->eof && f->len 
					&& castart+f->highwater-(f->lastseek+f->len)<f->buflen)
		{
			/* tlen: Wieviel soll gelesen werden */
			/* flen: Wieviel wurde gelesen */
			/* vlen: Wieviel wird verschoben */
			long tlen, flen, vlen;

			if(f->seekpos!=f->lastseek+f->len &&
				Fseek(f->lastseek+f->len, f->handle, SEEK_SET)!=f->lastseek+f->len)
			{
				printf("Seek Typ 2 auf %ld ging nicht :-(\n",
					 f->lastseek+f->len);
				if(f->len<f->buflen)
				{
					f->eof=1;
					return (Fseek(castart, f->handle, SEEK_SET)!=castart);
				}
				else
					return -1;
			}

			printf(")");

			if((tlen=castart+f->highwater-(f->lastseek+f->len))
							<(f->buflen>>1))
			{
				tlen=f->buflen>>1;
			}

			vlen=f->len+tlen-f->buflen;
			if(vlen<0)
				vlen=0;

			if(vlen>0)
			{
				memcpy(f->buf, f->buf+vlen, f->len-vlen);
				castart=f->lastseek+vlen;
				f->len-=vlen;
			}
			else
				castart=f->lastseek;

			if((flen=Fread(f->handle, tlen, f->buf+f->buflen-tlen))<0)
			{
				printf("Read ging nicht :-(\n");
				f->len=0;
				return -1;
			}

			f->seekpos=f->lastseek+f->len+flen;

			f->len+=flen;

			if(flen!=tlen)
			{
				f->eof=1;
			}
		}
		else
		{
			if(f->eof && castart>=f->lastseek+f->len)
			{
				printf("Hinter das Ende der Datei lesen? Seltsam ...\n");
				return -1;
			}
			if(f->seekpos!=castart &&
					Fseek(castart, f->handle, SEEK_SET)!=castart)
			{
				printf("Seek Typ 3 auf %ld (%lx) ging nicht\n", castart, castart);
				return -1;
			}

			printf("|");

			if((f->len=Fread(f->handle, f->buflen, f->buf))<=0)
			{
				if(f->len<0)
				{
					printf("Read ging nicht :-(\n");
					return -1;
				}
			}
			f->seekpos=castart+f->len;
			if(f->len<f->buflen)
				f->eof=1;
		}
		f->lastseek=castart;
		printf("\r  \r");
	}

	if(castart<f->lastseek+f->len)
	{
		return 0;
	}
	else
	{
		return -1;
	}
}

unsigned long mausdatum(time_t stdatum)
{
	struct tm ztm;
/*	char buf[40];*/

	stdatum-=timezone;

	ztm=*localtime(&stdatum);

/*	sprintf(buf,"%02d%02d%02d%02d%02d",
		ztm.tm_year-90,
		ztm.tm_mon+1,
		ztm.tm_mday,
		ztm.tm_hour,
		ztm.tm_min
		);
	return atol(buf);
*/
	return ztm.tm_min+100L*(ztm.tm_hour+100L*(ztm.tm_mday+100L*(ztm.tm_mon+1+
		100L*(ztm.tm_year-90))));
}

time_t stdatum(unsigned long mausdatum)
{
	struct tm ztm;

	memset(&ztm, 0, sizeof(ztm));

	ztm.tm_min=(int)(mausdatum%100);
	mausdatum/=100;
	ztm.tm_hour=(int)(mausdatum%100);
	mausdatum/=100;
	ztm.tm_mday=(int)(mausdatum%100);
	mausdatum/=100;
	ztm.tm_mon=(int)(mausdatum%100)-1;
	mausdatum/=100;
	ztm.tm_year=90+(int)(mausdatum%100);

	return mktime(&ztm)+timezone;
}

char *mausdat2str(unsigned long mausdatum, char *buf)
{
	time_t l;

	l=stdatum(mausdatum)-timezone;
	ztm=localtime(&l);
	sprintf(buf,"%02d.%02d.%02d %02d:%02d",
		ztm->tm_mday,
		ztm->tm_mon+1,
		ztm->tm_year>99?ztm->tm_year+1900:ztm->tm_year,
		ztm->tm_hour,
		ztm->tm_min);
	return buf;
}

#ifndef GRUPSTAT
char *lmausdatum(unsigned long when, char *buf)
{
	long a, b;

	b=when;
	a=b/100000000L;
	b=b%100000000L;
	sprintf(buf, "%04ld%08ld", 1990L+a, b);

	return buf;
}

char *outfdatum(unsigned long when, char *buf)
{
	time_t l;

/* z. B. 'Fr, 12.06.92 02:07' */
	l=stdatum(when)-timezone;
	ztm=localtime(&l);
	sprintf(buf,"%s, %02d.%02d.%02d %02d:%02d",
		daystr[ztm->tm_wday],
		ztm->tm_mday,
		ztm->tm_mon+1,
		ztm->tm_year,
		ztm->tm_hour,
		ztm->tm_min);
	return buf;
}
#endif

void packliste(void)
{
	person *p1, *p2, *pmax;

	pmax=&info[(long)maxpers];
	for(p1=p2=&info[0L]; p1<pmax; p1++)
	{
		if(p1->name[0L])
		{
			memcpy(p2++, p1, sizeof(person));
		}
	}
}

char *fgets2( char *str, int n, FILE *stream )
{
	char *zwi, *pos;

	zwi=fgets(str, n, stream);
	if(!zwi)
		return zwi;
	pos=strchr(str, '\n');
	if(pos)
		*pos=0;
	pos=strchr(str, ';');
	if(pos)
		*pos=0;
	pos=strchr(str, '#');
	if(pos)
		*pos=0;

	pos=str+strlen(str)-1;
	while(pos>=str && *pos==' ')
		*pos--=0;

	if(*str)
		return zwi;
	else
		return fgets2(str, n, stream);
}

#ifndef GRUPSTAT
char *delspaces(char *s)
{
	char *zwi;
	int i;

	zwi=strchr(s, ':');
	if(zwi==NULL)
		return s;
	else
		zwi++;

	for(i=0; zwi[(long)i]==' '; i++);

	strcpy(zwi, zwi+i);

	return s;
}

void get_gruppen(FILE *file)
{
	int aktgruppe=0;

	for(;aktgruppe+pms<MAXGRP; aktgruppe++)
	{
		if(!(long)fgets2(buf, (int)sizeof(buf),file))
			break;
		if(strchr(buf,'\n'))
			*strchr(buf,'\n')=0;
		if(!strncmpi(buf, ":GRUPPEN", 8))
			break;
		if(!strncmpi(buf, "END:", 4))
			break;
		strncpy(gruppen[(long)aktgruppe], buf, MAXGRPCHARS);
		strupr(gruppen[(long)aktgruppe]);
	}
}

void setbit(char *buf, unsigned short *flags, char c, unsigned short wert)
{
	if(strchr(buf, c))
		*flags|=wert;
}

void getmask(char *buf, unsigned short *flags)
{
	strupr(buf);

	setbit(buf, flags, 'G', 1);
	setbit(buf, flags, 'X', 16);
	setbit(buf, flags, 'F', 4);
	setbit(buf, flags, 'L', 2);
	setbit(buf, flags, 'V', 512);
	setbit(buf, flags, 'D', 8);
	setbit(buf, flags, 'K', 32);
	setbit(buf, flags, 'B', 64);
	setbit(buf, flags, 'C', 128);
	setbit(buf, flags, 'M', 256);
}

int one_dat_par(FILE *file)
{
	if(!fgets2(buf,(int)sizeof(buf),file))
		return -1;

	delspaces(buf);

	if(!strncmpi(buf,"Von:",4))
		mindatum=getdatum(buf+4);
	else if(!strncmpi(buf,"Bis:",4))
		maxdatum=getdatum(buf+4);
	else if(!strncmpi(buf, "Outname:",8))
		strcpy(outname, buf+8);
	else if(!strncmpi(buf, "Username:",9))
	{
		strcpy(username, buf+9);
		unatpos=(int)(strchr(username, '@')-username);
	}
	else if(!strncmpi(buf, "Database:",9))
		strcpy(database,buf+9);
	else if(!strncmpi(buf, "Ausgabe:",8))
		strcpy(ausgabe,buf+8);
	else if(!strncmpi(buf, "Betreff:",8))
	{
		strcpy(betreff,buf+8);
		betrlen=(int)strlen(betreff);
	}
	else if(!strncmpi(buf, "Muss:",5))
		getmask(buf+5, &mussflags);
	else if(!strncmpi(buf, "Nicht:",6))
		getmask(buf+6, &nichtflags);
	else if(!strncmpi(buf, "Out:",4))
	{
		strupr(buf);
		out=1;
		if(strstr(buf, "FLAGS"))
			flagsex=1;
		if(strstr(buf, "STATUS"))
			statusex=1;
		if(strstr(buf, "121FIX"))
			novon=1;
		if(strstr(buf, "MITVON"))
			mitvon=1;
	}
	else if(!strncmpi(buf, "Gruppen:",8))
	{
		alle=0;
		get_gruppen(file);
	}
	else if(!strncmpi(buf, "Options:",8))
	{
		strupr(buf);
		if(strstr(buf, "DMQ"))
			dmq=1;
		if(strstr(buf, "PMS"))
			pms=1;
		if(strstr(buf, "ADR"))
			adrex=1;
		if(strstr(buf, "NOH"))
			headerex=0;
		if(strstr(buf, "UNIQUE"))
			unique=1;
		if(strstr(buf, "FAST"))
			fast=1;
		if(strstr(buf, "TASTE"))
			taste=1;
	}
	else if(!strncmpi(buf, "End:",4))
	{
		while(fgets2(buf,(int)sizeof(buf),file));
		return -1;
	}
	else
	{
		printf("Zeile ignoriert:");
		printf("'%s'\n", buf);
		return 0;
	}

	return 0;
}

int read_dat(void)
{
	FILE *datfile;
	int aktgruppe=0;

	betrlen=0;

	datfile=fopen(statdatname,"r");
	if(datfile==NULL)
	{
		printf("%s fehlt, nehme Default an\n", statdatname);
	}

	mindatum=000+1010000L;
	maxdatum=1501020000L;

	strcpy(database, ".\\");
/*	strcpy(ausgabe, "CAT_STAT.ERG");*/

	outfile=NULL;

	if(datfile!=NULL)
		while(!one_dat_par(datfile));

	if(mindatum<000+1010000L || maxdatum<000+1020000L
	 ||mindatum>1501010000L || maxdatum>1501020000L)
	{
		printf("Fehlerhaftes Datum\n");
		return -1;
	}

	if(alle)
	{
		if(datfile!=NULL)
			fclose(datfile);
		strcpy(buf, database);
		strcat(buf, "GRUPPEN.INF");
		datfile=fopen(buf, "r");
		if(!datfile)
		{
			printf("Gruppen.Inf nicht gefunden\n");
			return -1;
		}
		for(;aktgruppe+pms<MAXGRP; aktgruppe++)
		{
			if(!(long)fgets2(buf, (int)sizeof(buf),datfile))
				break;
			if(strchr(buf,'\n'))
				*strchr(buf,'\n')=0;
			if(!strncmpi(buf, ":GRUPPEN", 8))
				break;
			strncpy(gruppen[(long)aktgruppe], buf, MAXGRPCHARS);
		}
	}

	fclose(datfile);

	if(pms)
	{
		for(aktgruppe=0; *gruppen[(long)aktgruppe]; aktgruppe++);
		strncpy(gruppen[(long)aktgruppe++], "PMs", MAXGRPCHARS);
	}

	if(out)
	{
		if(!*outname)
		{
			strcpy(outname, database);
			strcpy(outname, "");
			strcat(outname, "OUTFILE.TXT");
		}

		outfile=seqopen(outname, SEQ_WRITE, &seqbufsiz);

		if(outfile==NULL)
		{
			printf("ôffnen des Outfiles ging schief.\n");
			return -1;
		}
	}
	else
		*outname=0;

	return 0;
}
#endif

#define UMLAUTE1 "ÑÑîîÅÅûûééôôöö"
#define UMLAUTE2 "AEOEUESSAEOEUE"

int chrvergl(char **p1, char **p2)
{
	static char u1[15]=UMLAUTE1;
	static char u2[15]=UMLAUTE2;

	char c1, c2;
	char *f;

	c1=toupper(**p1);
	c2=toupper(**p2);

	if(c1==c2)
	{
		(*p1)++;
		(*p2)++;
		return 0;
	}

	if((f=strchr(u1, c1))!=NULL)
		if(c2==u2[f-u1] && toupper(*(*p2+1))==u2[f-u1+1])
		{
			(*p1)++;
			(*p2)+=2;
			return 0;
		}

	if((f=strchr(u1, c2))!=NULL)
		if(c1==u2[f-u1] && toupper(*(*p1+1))==u2[f-u1+1])
		{
			(*p2)++;
			(*p1)+=2;
			return 0;
		}

	(*p1)++;
	(*p2)++;

	return strncmp(&c1, &c2, 1);
}

int ownstrncmp(char *s1, char *s2, size_t maxlen)
{
	char *p1, *p2, *n;
	int erg;

	if(fast)
		return strncmp(s1, s2, maxlen);

	p1=s1;
	p2=s2;

	for(n=p1+maxlen-1; n>=p1;)
	{
		if(!*p1 && !*p2)
			return 0;
		erg=chrvergl(&p1, &p2);
		if(erg)
			return erg;
	}
	return 0;
}

int main(int argv, char **argc)
{
	int i,doppelt=0, aktgruppe;
	long l;
	time_t secvon, secbis;
	
#if (CATVER==1)
 #ifdef GRUPSTAT
  #ifndef DEBUG
	struct stat statbuf;
  #endif
 #endif
 #ifndef DEBUG
	infodat1 id,id2;
	catbeg cb;
 #endif
#endif

	if(argv<2)
	{
		strcpy(statdatname, "cat_stat.dat");
	}
	else
	{
		strcpy(statdatname, argc[1]);
	}

#ifdef GRUPSTAT
	printf("\033qGrUpStAt\n");
#else
	printf("\033qCat-Stat %s\n",__DATE__);
	printf(VERSION);
	printf("\n");
#endif

	time(&secvon);

	memset(wtag, 0, sizeof(wtag));
	memset(gruppen, 0, sizeof(gruppen));

	time(&now);
	ztm=localtime(&now);

#ifdef GRUPSTAT
 #ifndef DEBUG
	if(ztm->tm_wday<3 || ztm->tm_wday>5)
		return 0;
	if(ztm->tm_wday==3 && ztm->tm_hour<15)
		return 0;
 #endif
#endif

	now-=(now%(24*60L*60)); 	/* Keine Stunden, Minuten oder Sekunden	*/

	monday=now-7*24*60L*60;	/* Ein Woche zurÅck						*/
	ztm=localtime(&monday);
	while(ztm->tm_wday!=1)	/* Wann war vorletzter Montag?			*/
	{
		monday-=24*60L*60;
		ztm=localtime(&monday);
	}

#ifdef GRUPSTAT
	mindatum=mausdatum(monday);

 #ifndef DEBUG
	if(!schonzeitp())
		return 0;
 #endif

 #ifdef GRUPSTAT
	strcpy(gruppen[0L], GRUPPE);
 #endif
	strcpy(database, MAUSPFAD"DATABASE\\");
	strcpy(ausgabe, "C:\\CLIPBRD\\SCRAP.TXT");
	gruppen[1L][0L]=0;
#else
	if(read_dat())
		return -1;
	aktgruppe=0;
#endif

#ifdef GRUPSTAT
	for(i=0;i<7;i++)
	{
		monday+=24*60L*60L; /* Naja, hierdrin ist es nicht immer Montag ;-) */
		wtag[i].maxdatum=mausdatum(monday);
	}

	maxdatum=wtag[6].maxdatum;
#endif

	l=stdatum(mindatum);
	ztm=localtime(&l);
	sprintf(mins,"%02d.%02d.%02d",
		ztm->tm_mday,
		ztm->tm_mon+1,
		ztm->tm_year);

	l=stdatum(maxdatum)-24*60L*60;
	ztm=localtime(&l);
	sprintf(maxs,"%02d.%02d.%02d",
		ztm->tm_mday,
		ztm->tm_mon+1,
		ztm->tm_year);

#ifdef GRUPSTAT
	printf("Statistik wird erstellt\n\n");
#endif

	init();

	monday=monday%(7*24*60L*60);

	if((long)Malloc(-1)>2200000L)
	{
		dat.buflen=(((long)Malloc(-1))/2-500000L)&~1;
		maxpers=16384;
	}
	else if((long)Malloc(-1)>800000L)
	{
		dat.buflen=(((long)Malloc(-1)-500000L)/2)&~1;
		maxpers=8192;
	}
	else if((long)Malloc(-1)>550000L)
	{
		dat.buflen=(((long)Malloc(-1)-300000L)/2)&~1;
		maxpers=4096;
	}
	else if((long)Malloc(-1)>280000L)
	{
		dat.buflen=100000L;
		maxpers=2048;
#ifndef GRUPSTAT
		seqbufsiz=5000L;
#endif
	}
	else
	{
		dat.buflen=2*WATERMARK;
		maxpers=512;
#ifndef GRUPSTAT
		seqbufsiz=1024L;
#endif
	}

	dat.buf=malloc(dat.buflen);
	if(dat.buf==NULL)
	{
		printf("Puffer fÅr Dat konnte nicht alloziert werden!\n");
		return -1;
	}
	printf("Puffer fÅr dat: %ld\n", dat.buflen);
	dat.highwater=WATERMARK;

#if (CATVER<1)
 	if(*outname)
	{
		par.buflen=(dat.buflen/10)&~1;
		par.buf=malloc(par.buflen);
		if(par.buf==NULL)
		{
			printf("Puffer fÅr Ref konnte nicht alloziert werden!\n");
			return -1;
		}
		printf("Puffer fÅr Ref: %ld\n", par.buflen);
		par.highwater=sizeof(par);
	}
#endif

#if (CATVER==1)
	linpar.buflen=(dat.buflen/10)&~1;
#else
	linpar.buflen=(dat.buflen/20)&~1;
#endif

	linpar.buf=malloc(linpar.buflen);
	if(linpar.buf==NULL)
	{
		printf("Puffer fÅr Par konnte nicht alloziert werden!\n");
		return -1;
	}
	printf("Puffer fÅr par: %ld\n", linpar.buflen);
	linpar.highwater=sizeof(parblk)*2;

	info=calloc(maxpers, sizeof(person));
	if(info==NULL)
	{
		printf("Array fÅr Personen konnte nicht alloziert werden!\n");
		return -1;
	}
	printf("Maximale Personen: %d\n", maxpers);
	printf("Freier Speicher: %ld\n\n", Malloc(-1));

	for(aktgruppe=0; gruppen[(long)aktgruppe][0L]; aktgruppe++)
	{
		strcpy(buf,database);
		strcat(buf,"GRUPPEN.INF");

		if(!strcmp(gruppen[(long)aktgruppe], "PMs"))
		{
			sprintf(buf,"%sPRIVATE.PAR",database);
			printf("Gruppe PMs\n");
		}
		else
		{
			FILE *file;

			file=fopen(buf,"r");
			if(file==NULL)
			{
				printf("ôffnen von Gruppen.Inf ging nicht!\n");
				return -1;
			}

			for(i=0;;i++)
			{
				if(fgets(buf,255,file)==NULL)
				{
					printf("Gruppe %s nicht in Gruppen.Inf\n", gruppen[(long)aktgruppe]);
					i=32766;
					break;
				}
				if(strchr(buf,'\n'))
					*strchr(buf,'\n')=0;
				strupr(buf);
				if(!ownstrncmp(buf, gruppen[(long)aktgruppe], strlen(buf)))
					break;
			}
			fclose(file);
			if(i==32766)
				continue;
			printf("Gruppe %s\n", gruppen[(long)aktgruppe]);
			if(i<100)
				sprintf(buf,"%sGRUPPE%2.2d.PAR",database,i);
			else
				sprintf(buf,"%sGRUPP%3.3d.PAR",database,i);
		}

#if (CATVER<1)
		par.handle=(int)Fopen(buf,FO_READ);
		if(par.handle<0)
		{
			printf("Open von %s fÅr %s ging nicht!\n", buf, gruppen[(long)aktgruppe]);
			return -1;
		}
#endif

		linpar.handle=(int)Fopen(buf,FO_READ);
		if(linpar.handle<0)
		{
			printf("Open von %s fÅr %s ging nicht!\n", buf, gruppen[(long)aktgruppe]);
			return -1;
		}

		strcpy(strrchr(buf, '.'), ".DAT");

		dat.handle=(int)Fopen(buf,FO_READ);
		if(dat.handle<0)
		{
			printf("Open von %s fÅr %s ging nicht\n", buf, gruppen[(long)aktgruppe]);
			return -1;
		}

		dat.lastseek=dat.seekpos=dat.len=dat.eof=0;
#if (CATVER<1)
		par.lastseek=par.seekpos=par.len=par.eof=0;
#endif
		linpar.lastseek=linpar.seekpos=linpar.len=linpar.eof=0;

	#if (CATVER==1)
		i=headerokP(&linpar);
		if(i)
		{
			printf("Falsche Cat-Version!\n");
			return i;
		}
	#endif

#if (CATVER==1)
		parpos=8;
#else
		parpos=0;
#endif
		while(!CACHELIN(parpos))
		{
#ifndef GRUPSTAT
			int betr;
#endif
			memcpy(&msg,DORTLIN(parpos), sizeof(parblk));
			if(msg.datum>=mindatum && msg.datum<maxdatum
#ifndef GRUPSTAT
				&& ((msg.bits&mussflags)==mussflags)
				&& (!(msg.bits&nichtflags))
#endif
				)
			{
#ifndef GRUPSTAT
				if(betrlen)
				{
					short eint;
					long dort;

					dort=msg.start+msg.idLength+((msg.idLength%2)?1:0);
					CACHEDAT(dort);
					DORTWDAT(eint, dort);
					betr=!strncmpi((char *)DORTDAT(dort+(eint+1)*2),betreff,betrlen);
				}
				else
					betr=1;

				if(betr)
				{
#endif
				char name[MAXNAMECHARS+MAXATCHARS+1L],
					 name2[MAXNAMECHARS+MAXATCHARS+1L], *n2, *atpos;
				int i;
				long msgstart;
				long day;

	#if (CATVER==1)
				short anzeint;
	#endif

				if(fast)
					n2=name;
				else
					n2=name2;

/*
	#ifdef GRUPSTAT
				i=-1;

				if(msg.datum<wtag[0].maxdatum)
					i=0;
				else if(msg.datum<wtag[1].maxdatum)
					i=1;
				else if(msg.datum<wtag[2].maxdatum)
					i=2;
				else if(msg.datum<wtag[3].maxdatum)
					i=3;
				else if(msg.datum<wtag[4].maxdatum)
					i=4;
				else if(msg.datum<wtag[5].maxdatum)
					i=5;
				else if(msg.datum<wtag[6].maxdatum)
					i=6;
	#else
*/
				day=stdatum(msg.datum)-monday;
				day=day%(7*24L*60*60);
				i=(int)(day/(24L*60*60));
/*	#endif*/
				if(i>=0)
				{
					wtag[i].anzahl++;
					wtag[i].um[(day/(60L*60L))%24]++;
				}

				if(msg.start<0)
				{
					printf("In .Par angegebene Position fÅr msgstart ungÅltig\n");
					return -1;
				}

 #if (CATVER==1)
				if(msg.items&1)
				{
					short crc, crc1;
	
					msgstart=msg.start;

					msgstart+=msg.idLength;
					if(msg.idLength&1)
						msgstart++;

					CACHEDAT(msgstart);
					DORTWDAT(anzeint,msgstart);

					if(anzeint>25)
					{
						printf("Nachricht %.50s hat %hd EintrÑge ...\n",
								DORTDAT(msgstart), anzeint);
						printf("Es sollte Position %ld sein\n", msgstart);
						abort();
					}
					msgstart+=(anzeint+1)*2;
 #else
				{
					short crc, crc1;

					msgstart=msg.start;
 #endif
					CACHEDAT(msgstart);

					i=(int)strlen((char *)DORTDAT(msgstart))+1;
					strncpy(name,(char *)DORTDAT(msgstart)+i, MAXNAMECHARS);

					atpos=strchr(name,'@');
	
/*					crc=(crc16(name)%(MAXPERS-1))+1;
					for(i=crc;info[(long)i].name[0]!=0 && i!=crc+MAXPERS-1;i++,i%=MAXPERS)
*/
					if(!fast)
					{
						size_t i;

						strcpy(name2, name);
						strupr(name2);
						for(i=0;name2[i] && name2[i]!='@';i++)
						{
							static char u1[15]=UMLAUTE1;
							static char u2[15]=UMLAUTE2;
							char *h;

							if((h=strchr(u1, name2[i]))!=NULL &&
								strlen(name2)<MAXNAMECHARS)
							{
								memmove(&name2[i+1], &name2[i],
										min(strlen(&name2[i]), MAXNAMECHARS-i-1));
								name2[i]=u2[h-u1];
								name2[i+1]=u2[h-u1+1];
							}
						}
					}

					crc=(crc16(n2)&(maxpers-1)); /* maxpers 2erpotenz! */
					crc1=(crc-1)&(maxpers-1);

					if(atpos)
					{
						for(i=crc;info[(long)i].name[0] && i!=crc1;
								i++,i&=maxpers-1)
							if(!ownstrncmp(name,info[(long)i].name,
										unique
											?min(strlen(name),MAXNAMECHARS)
											:min(atpos-name,MAXNAMECHARS)))
							{
								duplist **dupes;
								int j;
		
								if(strcmp(atpos, strchr(info[(long)i].name,'@')))
								{
									for(j=0, dupes=&info[(long)i].dupes;
										   *dupes!=NULL
										&& strncmp(atpos, (*dupes)->dupat, MAXATCHARS);
										j++, dupes=&(*dupes)->next);
		
									if(*dupes==NULL)
									{
										(*dupes)=malloc(sizeof(duplist));
										if(!*dupes)
										{
											printf("Malloc fÅr Dupe ging schief!\n");
											return -1;
										}
										(*dupes)->next=NULL;
										strncpy((*dupes)->dupat, atpos, MAXATCHARS);
										if(j==0)
											doppelt++;
									}
								}
								info[(long)i].bytes+=msg.Length;
								info[(long)i].msgs++;
	
								if(!unique && !fast && strncmp(name, info[(long)i].name,
											min(MAXNAMECHARS, atpos-name)
											   				   )
								  )
								{
									if(atpos-name<strcspn(info[(long)i].name, "@"))
									{
										strncpy(atpos, strchr(info[(long)i].name, '@'), MAXATCHARS);
										strncpy(info[(long)i].name, name, MAXNAMECHARS);
									}
								}
	
								break;
							}
		
						if(info[(long)i].name[0]==0)
						{
							strncpy(info[(long)i].name, name, MAXNAMECHARS);
							info[(long)i].bytes=msg.Length;
							info[(long)i].msgs=1;
							schreiber++;
						}
					}
					bytes+=msg.Length;
					msgs++;
				}
#if(CATVER==1)
				else
					*name=0;
#endif

#ifndef GRUPSTAT
				if(*outname)
				{
					char flags[50];
 #if (CATVER==1)
					long dort;
					short eint;
					size_t len;
					char an[MAXNAMECHARS+1];
 #endif
					char extinfobuf[3*MAXNAMECHARS+MAXBOXCHARS+
								2*MAXIDCHARS]="";

					msgstart=msg.start;
					CACHEDAT(msgstart);

					sprintf(bufseq, "#%s", DORTDAT(msgstart));
					seqputs(bufseq, outfile);
 #if (CATVER==1)
					msgstart+=msg.idLength;
					if(msg.idLength&1)
						msgstart++;

					CACHEDAT(msgstart);
					DORTWDAT(anzeint,msgstart);

					dort=msgstart+2;

					/* Jetzt den Status */
					if(msg.items & 0x8000) /* Sind Privat-Infos da? */
					{
						privblk pinfo;

						msgstart+=anzeint*2;
						DORTWDAT(eint, msgstart);
						msgstart=msg.start+eint;
						if(msg.items&16384) /* Unbekannte Informationen */
						{
							CACHEDAT(msgstart);
							while(*DORTDAT(msgstart) || *(DORTDAT(msgstart+1)))
							{
								printf("Zusatzinfo: %s\n", DORTDAT(msgstart));
								msgstart+=1+strlen((char *)DORTDAT(msgstart+1));
								CACHEDAT(msgstart);
							}
							msgstart++;
						}
						len=strlen((char *)DORTDAT(msgstart))+1;
						msgstart+=len;

						if((msgstart^msg.start)&1)
							msgstart++;

						CACHEDAT(msgstart);
						memcpy(&pinfo,(privblk *)DORTDAT(msgstart),
								sizeof(pinfo));
						sprintf(bufseq, "B%c%s", pinfo.status,
								lmausdatum(pinfo.lesedatum, buf));
						seqputs(bufseq, outfile);
					}
 #else
					msgstart+=(int)strlen((char *)DORTDAT(msgstart))+1;
 #endif
					if(flagsex)
					{
					/* Und jetzt die Flags */
						strcpy(flags, "");
	
						if(msg.bits&1)
							strcat(flags, "G");
						if(msg.bits&16)
							strcat(flags, "X");
						if(msg.bits&4)
							strcat(flags, "F");
						if(msg.bits&2)
							strcat(flags, "L");
						if(msg.bits&512)
							strcat(flags, "V");
						if(msg.bits&8)
							strcat(flags, "D");
						if(msg.bits&32)
							strcat(flags, "K");
						if(msg.bits&64)
							strcat(flags, "B");
						if(msg.bits&128)
							strcat(flags, "C");
						if(msg.bits&256)
							strcat(flags, "M");
						sprintf(bufseq, "s%s", flags);
						seqputs(bufseq, outfile);

					}

					if(strcmp(gruppen[(long)aktgruppe], "PMs"))
					{
						sprintf(bufseq, "G%s", gruppen[(long)aktgruppe]);
						seqputs(bufseq, outfile);
					}
 #if (CATVER==1)
					CACHEDAT(dort);
					DORTWDAT(eint, dort);
					sprintf(extinfobuf, "%s", DORTDAT(msg.start+eint));

					if(msg.items&1) /* Existiert Von-Feld? */
					{
						dort+=2;
						DORTWDAT(eint,dort);
						sprintf(bufseq, "V%s", DORTDAT(msg.start+eint));
						seqputs(bufseq, outfile);
					}
					else if(mitvon && !strcmp(gruppen[(long)aktgruppe], "PMs"))
					{
						sprintf(bufseq, "V%s", username);
						seqputs(bufseq, outfile);
					}

					if(msg.items&2) /* Existiert ein An-Feld? */
					{
						dort+=2;
						DORTWDAT(eint, dort);
						strcpy(an, (char *)DORTDAT(msg.start+eint));
						sprintf(bufseq, "A%.*s", MAXNAMECHARS, an);
						seqputs(bufseq, outfile);
					}

					sprintf(bufseq, "W%s", extinfobuf);
					seqputs(bufseq, outfile);
					sprintf(bufseq, "E%s", lmausdatum(msg.datum, buf));
					seqputs(bufseq, outfile);

					extinfobuf[0]=0;

					if(!(0x8000&msg.items) && statusex && strcmp(gruppen[(long)aktgruppe], "PMs"))
					{
						if(msg.bits&1)
						{
							sprintf(bufseq, "BG%s", lmausdatum(msg.datum, buf));
							seqputs(bufseq, outfile);
						}
						else if(msg.bits&2)
						{
							sprintf(bufseq, "BF%s", lmausdatum(msg.datum, buf));
							seqputs(bufseq, outfile);
						}
					}
 #else
					sprintf(bufseq, "V%s", name);
					seqputs(bufseq, outfile);
					sprintf(bufseq, "W%s", msg.wegen);
					seqputs(bufseq, outfile);
					sprintf(bufseq, "E%s", lmausdatum(msg.datum, buf));
					seqputs(bufseq, outfile);

					msgstart+=(int)strlen((char *)DORTDAT(msgstart))+1;
 #endif
					if(!strcmp(gruppen[(long)aktgruppe], "PMs"))
					{
						if(msg.bits&1)
						{
							sprintf(bufseq, "BG%s", lmausdatum(msg.datum, buf));
							seqputs(bufseq, outfile);
						}
						else if(msg.bits&2)
						{
							sprintf(bufseq, "BF%s", lmausdatum(msg.datum, buf));
							seqputs(bufseq, outfile);
						}
					}

#if (CATVER<1)

 #if (CATVER==1)
					if(msg.upMess<0xFFFE)
 #else
					if(!(msg.upMess&0x8000))
 #endif
					{
						parblk refmsg;
						size_t refpos;

 #if (CATVER==1)
						refpos=msg.upMess*sizeof(msg)+sizeof(catbeg);
 #else
						refpos=msg.upMess*sizeof(msg);
 #endif

						if(CACHEPAR(refpos)<0)
						{
							printf("Verkettung nicht auffindbar!\n");
							return -1;
						}
						memcpy(&refmsg, DORTPAR(refpos), sizeof(parblk));


						if(refmsg.start<0)
						{
			printf("In .Par angegebene Position fÅr refmsgstart ungÅltig\n");
							return -1;
						}

						CACHEDAT(refmsg.start);
						sprintf(bufseq, "-%s", DORTDAT(refmsg.start));
						seqputs(bufseq, outfile);
						
 #if (CATVER==1)
						CACHEDAT(dort);
 #endif
					}

#endif

 #if (CATVER==1)
					if(novon && (msg.items&1))
					{
						sprintf(extinfobuf+strlen(extinfobuf),
							"Von : %s (%s)\r\n\r\n", name, outfdatum(msg.datum, buf));
					}
/*
					if(msg.items&2)
					{
						sprintf(extinfobuf+strlen(extinfobuf),
							">An  : %s\r\n", an);
					}
*/
					if(msg.items&4) /* Existiert ein Fremd-ID-Feld? */
					{
						dort+=2;
						DORTWDAT(eint,dort);
						sprintf(bufseq, "I%s", DORTDAT(msg.start+eint));
						seqputs(bufseq, outfile);
/*						sprintf(extinfobuf+strlen(extinfobuf),
								">MId : <%s>\r\n", DORTDAT(msg.start+eint));
*/					}

					if(msg.items&8) /* Existiert eine Fremdverkettung? */
					{
						dort+=2;
						DORTWDAT(eint,dort);
						sprintf(bufseq, "R%s", DORTDAT(msg.start+eint));
						seqputs(bufseq, outfile);
/*						sprintf(extinfobuf+strlen(extinfobuf),
								">RId : <%s>\r\n", DORTDAT(msg.start+eint));
*/					}

					if(msg.items&16) /* Existiert eine Box? */
					{
						dort+=2;
						DORTWDAT(eint, dort);
						sprintf(bufseq, "O%s", DORTDAT(msg.start+eint));
						seqputs(bufseq, outfile);
/*						sprintf(extinfobuf+strlen(extinfobuf),
								">Box : %s\r\n", DORTDAT(msg.start+eint));
*/					}

					if(msg.items&32) /* Existiert ein Realname? */
					{
						dort+=2;
						DORTWDAT(eint, dort);
						sprintf(bufseq, "N%s", DORTDAT(msg.start+eint));
						seqputs(bufseq, outfile);
/*						sprintf(extinfobuf+strlen(extinfobuf),
								">Name: %s\r\n", DORTDAT(msg.start+eint));
*/
					}

					if(msg.items&64) /* Verkettung */
					{
						dort+=2;
						DORTWDAT(eint, dort);
						strcpy(an, (char *)DORTDAT(msg.start+eint));
						sprintf(bufseq, "-%.*s", MAXNAMECHARS, an);
						seqputs(bufseq, outfile);
					}

					if(msg.items&128) /* Distribution */
					{
						dort+=2;
						DORTWDAT(eint, dort);
						strcpy(an, (char *)DORTDAT(msg.start+eint));
						sprintf(bufseq, "D%.*s", MAXNAMECHARS, an);
						seqputs(bufseq, outfile);
					}

/*					sprintf(buf, extinfobuf);

					if(msg.items&1)
					{
						sprintf(bufseq, ">");
						seqputs(bufseq, outfile);
					}
*/

					msgstart=msg.start+msg.hLength;
 #else
					if(novon)
					{
						sprintf(extinfobuf,
							"Von : %s (%s)\n\n", name, outfdatum(msg.datum, buf));
					}
 #endif
					CACHEDAT(msgstart);

					if(novon)
						if(!strncmp(extinfobuf, (char *)DORTDAT(msgstart),
													strlen(extinfobuf)))
							msgstart+=strlen(extinfobuf);

 #if (CATVER==1)
					while(msgstart-msg.start-msg.hLength<msg.Length)
 #else
					while(msgstart-msg.start-msg.NameL-msg.idLength<msg.Length)
 #endif
					{
						char *zwi;

						zwi=strchr((char *)DORTDAT(msgstart), '\n');
						if(zwi)
						{
/*							if(zwi>(char *)DORTDAT(msgstart+msg.NameL+msg.idLength+msg.length))
								zwi=(char *)DORTDAT(msgstart+msg.NameL+msg.idLength+msg.length);
*/							*zwi=0;
							sprintf(bufseq, ":%s", DORTDAT(msgstart));
							seqputs(bufseq, outfile);
							msgstart=(long)(zwi+1-dat.buf+dat.lastseek);
						}
						else
							break;
					}
				}
#ifndef GRUPSTAT
				}
#endif
#endif
			}
			parpos+=sizeof(parblk);
		}
#if (CATVER<1)
		Fclose(par.handle);
#endif
		Fclose(linpar.handle);
		Fclose(dat.handle);
	}

	if(*ausgabe)
	{
		printf("\n");
		printf("Gruppenliste\n");
	}

#ifdef DEBUG
	if(*ausgabe)
	{
		file=fopen(ausgabe,"w");
		if(file==NULL)
		{
			printf("Ausgabefile %s lieû sich nicht îffnen\n", ausgabe);
			return -1;
		}
		setvbuf(file, NULL, _IOFBF, 60000L);
	}
	else
		file=stdout;
#else
	file=fopen(MAUSPFAD"MESSAGES\\MSG999.TXT","w");
#endif

#ifdef DEBUG
	if(headerex)
	{
		fprintf(file,"Highscore %s - %s Åber die Gruppen:\n",
				mausdat2str(mindatum, buf), mausdat2str(maxdatum, buf+200));
		for(aktgruppe=0; gruppen[(long)aktgruppe][0L]; aktgruppe++)
			fprintf(file, "%s\n", gruppen[(long)aktgruppe]);
		fprintf(file,"---\n");
	}
#endif

#ifdef GRUPSTAT
	if(!strcmp(GRUPPE, "KUNTERBUNT"))
	{
		fprintf(file,"Statistik (wer schrieb wieviel?) der letzten Woche in %s:\n\n", GRUPPE);
		fprintf(file,"Bitte, denkt daran:\n");
		fprintf(file,"Zuviele Msgs in Kubu kînnen zu Problemen fÅhren. In der\n");
		fprintf(file,"folgenden Liste weit vorne zu stehen, ist keine Ehre!\n\n");
	}
	else if(!strcmp(GRUPPE, "FFM.TALK"))
		fprintf(file,"Statistik (wer schrieb zuwenig?) der letzten Woche in %s:\n\n", GRUPPE);
	else if(!strcmp(GRUPPE, "OBERLEHRER"))
	{
		fprintf(file,"Statistik (wer schrieb wieviel?) der letzten Woche in %s:\n\n", GRUPPE);
		fprintf(file,"Bitte, denkt daran:\n");
		fprintf(file,"Zuviele Msgs in Oberlehrer zu schreiben ist unerwÅnscht. In der \n");
		fprintf(file,"folgenden Liste weit vorne zu stehen, ist keine Ehre!\n\n");
	}
	else
		fprintf(file,"Statistik der letzten Woche in %s:\n\n", GRUPPE);
#endif

/* Text abhÑngig von Anzahl Mitteilungen machen !**/

#ifndef GRUPSTAT
	if(outfile)
	{
		seqputs("#LOG", outfile);
		sprintf(bufseq,":!V%s",__DATE__);
		seqputs(bufseq, outfile);
		time(&now);
		sprintf(bufseq,":!I%s", lmausdatum(mausdatum(now), buf));
		seqputs(bufseq, outfile);
		sprintf(bufseq,":!Cat-Stat %s", VERSION);
		seqputs(bufseq, outfile);
		if(*username)
		{
			sprintf(bufseq,":!Database von %s", username);
			seqputs(bufseq, outfile);
		}
		seqputs(":#CMD", outfile);
		seqputs(":#", outfile);
		seqputs("#", outfile);
		seqclose(outfile);
	}
#endif

	if(*ausgabe)
		printf("Highscoreliste\n");

	if(headerex)
		fprintf(file,"Platz           Schreiberin/Schreiber           Msgs   %%  Byte/Msg  %%\n\n");
	else
		fprintf(file,"Platz        Schreiberin/Schreiber        Msgs   %%  Byte/Msg  %%\n\n");

	packliste();

	psort=malloc(schreiber*sizeof(person *));
	if(psort==NULL)
	{
		printf("Malloc fÅr Sortierung ging nicht\n");
	}
printf("1\n");

	for(l=0; l<schreiber; l++)
		psort[(long)l]=&info[(long)l];

	shellsort(psort, schreiber, vergleich);
printf("2\n");

/*	for(i=0;i<schreiber && info[(long)i].name[0];i++)*/
	for(i=0;i<schreiber;i++)
	{
printf("%d\r", i);
		if(i>0 && psort[(long)i]->msgs==psort[(long)i-1]->msgs)
			fputs("    ", file);
		else
			fprintf(file,"%3d.",i+1);

		if(headerex)
		{
			if(unatpos && !strncmp(psort[(long)i]->name, username, unatpos))
				fprintf(file,"   *%40s* %4ld  %4.1lf  %5.0lf  %4.1lf\n",
					psort[(long)i]->name, psort[(long)i]->msgs, 
					(psort[(long)i]->msgs*100L)/(double)msgs,
					psort[(long)i]->bytes/(double)psort[(long)i]->msgs,
					(psort[(long)i]->bytes*100L)/(double)bytes);
			else
				fprintf(file,"   %40s %4ld  %4.1lf  %5.0lf  %4.1lf\n",
					psort[(long)i]->name, psort[(long)i]->msgs, 
					(psort[(long)i]->msgs*100L)/(double)msgs,
					psort[(long)i]->bytes/(double)psort[(long)i]->msgs,
					(psort[(long)i]->bytes*100L)/(double)bytes);
		}
		else
		{
			fprintf(file,"   %34.34s %4ld  %4.1lf  %5.0lf  %4.1lf\n",
				psort[(long)i]->name, psort[(long)i]->msgs, 
				(psort[(long)i]->msgs*100L)/(double)msgs,
				psort[(long)i]->bytes/(double)psort[(long)i]->msgs,
				(psort[(long)i]->bytes*100L)/(double)bytes);
		}
	}
printf("3\n");

	if(headerex)
		fprintf(file,"------ ---------------------------------------- ---- ----- ------ -----\n");
	else
		fprintf(file,"------ ---------------------------------- ---- ----- ------ -----\n");

	if(headerex)
		fprintf(file,"%6ld                    %3ld                   %4ld  %4.1lf  %5.0lf  %4.1lf\n",
			(schreiber*(schreiber+1L))/2,schreiber,msgs,100L/(double)schreiber,
			bytes/(double)msgs,100L/(double)schreiber);
	else
		fprintf(file,"%6.6ld                 %3.3ld                %4.4ld  %4.1lf  %5.0lf  %4.1lf\n",
			(schreiber*(schreiber+1L))/2,schreiber,msgs,100L/(double)schreiber,
			bytes/(double)msgs,100L/(double)schreiber);

	fprintf(file, "---\n");

	if(*ausgabe)
		printf("Wochentage\n");

	fprintf(file,"Montag    : %3ld\n",wtag[0].anzahl);
	fprintf(file,"Dienstag  : %3ld\n",wtag[1].anzahl);
	fprintf(file,"Mittwoch  : %3ld\n",wtag[2].anzahl);
	fprintf(file,"Donnerstag: %3ld\n",wtag[3].anzahl);
	fprintf(file,"Freitag   : %3ld\n",wtag[4].anzahl);
	fprintf(file,"Samstag   : %3ld\n",wtag[5].anzahl);
	fprintf(file,"Sonntag   : %3ld\n\n",wtag[6].anzahl);

	/* Schnitt */

	for(l=0,i=0; i<7; l+=wtag[i++].anzahl);

	fprintf(file,"Im Schnitt kamen %1.1lf Msgs/Tag\n", 
		(double)l/(difftime(stdatum(maxdatum), stdatum(mindatum))/60/(double)60/24));

	fprintf(file, "---\n");

	l=(long)difftime(stdatum(maxdatum), stdatum(mindatum))/60/60L/24;
printf("4\n");

	if(dmq)
	{
		fprintf(file, "Der resultierende DMQ Åber %ld Tage ist %lf\n", l,
				(double)(1000L/aktgruppe * msgs/(double)l *(msgs /(double)bytes)));
		fprintf(file, "\n---\n");
	}

	/* 1000 *Messages/Tag / (BpM) */

	memset(summe,0,sizeof(summe));
	fprintf(file, "     0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23\n");
	for(i=0;i<7;i++)
	{
		int j;

		fprintf(file, "%2.2s:", &"MoDiMiDoFrSaSo"[2*i]);
		for(j=0;j<24;j++)
		{
			fprintf(file,"%3ld", wtag[i].um[j]);
			summe[j]+=wtag[i].um[j];
		}
		fprintf(file, "\n");
	}

	fprintf(file,"   ");
	for(i=0;i<24;i++)
		fprintf(file,"---");
	fprintf(file,"\n");
printf("5\n");

	fprintf(file,"  ");
	for(i=0;i<24;i+=2)
		fprintf(file,"%4ld  ", summe[i]);
	fprintf(file,"\n");

	fprintf(file,"   ");
	for(i=1;i<24;i+=2)
		fprintf(file,"  %4ld", summe[i]);
	fprintf(file,"\n");

#ifndef GRUPSTAT
	if(adrex)
	{
		if(*ausgabe)
			printf("Adr.Inf\n");

		strcpy(buf, database);
		strcat(buf, "ADR.INF");
		outfile=seqopen(buf, SEQ_WRITE, &seqbufsiz);
		shellsort(psort,schreiber,alphabetisch);
printf("6\n");
		for(l=0;l<schreiber;l++)
		{
			duplist *zwi;

			if((zwi=psort[(long)l]->dupes)!=NULL)
			{
				while(zwi->next)
					zwi=zwi->next;
				sprintf(bufseq,"%.*s%s" ,(int)strcspn(psort[(long)l]->name, "@"),
							psort[(long)l]->name, zwi->dupat);
				seqputs(bufseq, outfile);
			}
			else
			{
				sprintf(bufseq,"%s",psort[(long)l]->name);
				seqputs(bufseq, outfile);
			}
		}
		seqclose(outfile);
	}
#endif
printf("7\n");

	if(doppelt)
	{
		fprintf(file,"---\nVon zwei oder mehr Adressen schrieb%s %d Person%s:\n\n",
							doppelt==1?"":"en", doppelt, doppelt==1?"":"en");

		if(*ausgabe)
			printf("Gebe Dupes aus\n");
	
		for(l=0;l<schreiber;l++)
		{
			if(!info[(long)l].name[0])
				continue;
			if(info[(long)l].dupes!=NULL)
			{
				duplist *dupes, *dup2;
	
				fprintf(file,"%s",info[(long)l].name);
				for(dupes=info[(long)l].dupes;dupes!=NULL;dupes=dup2)
				{
					fprintf(file,",%s",dupes->dupat);
					dup2=dupes->next;
					free(dupes);
				}
				fprintf(file,"\n");
			}
		}
	}
printf("8\n");

#ifdef GRUPSTAT
	fprintf(file,"\nDiese Msg wird beim Booten automatisch erstellt, wenn es \n");
	fprintf(file,"Zeit dafÅr ist. Sollte sie ein zweites Mal kommen, so habe \n");
	fprintf(file,"ich Mist programmiert oder einen grîûeren Datenverlust \n");
	fprintf(file,"erlitten.\n\n");
	fprintf(file,"GT\n");
#endif
	fclose(file);

#if (CATVER==1)
 #ifndef DEBUG
	stat(MAUSPFAD"MESSAGES\\MSG199.TXT",&statbuf);
 #endif
#endif


#ifndef DEBUG
 #ifdef GRUPSTAT
	rename("grupstat.dat", "grupstat.da");
	grupfile=fopen("grupstat.dat","w");

	if(grupfile!=0)
	{
		FILE *olddat;
		char *fehler;
		int erfolg=0;

		olddat=fopen("grupstat.da", "r");

		while(olddat!=NULL)
		{
			fehler=fgets(buf,100,olddat);
			if(fehler==NULL)
			{
				break;
			}
			if(strncmpi(GRUPPE, buf, strlen(GRUPPE)))
				fputs(buf,grupfile);
			else
			{
				erfolg=1;
				fprintf(grupfile, "%s %lu\n", GRUPPE, mindatum);
			}
		}

		if(!erfolg)
			fprintf(grupfile, "%s %lu\n", GRUPPE, mindatum);

		fclose(grupfile);
		if(olddat!=NULL)
		{
			fclose(olddat);
			remove("grupstat.da");
		}
	}

   #if (CATVER==1)
	file=fopen(MAUSPFAD"MESSAGES\\MSGINFO.DAT","rb+");
	if(file==NULL)
	{
		printf("Wo ist denn Msginfo.Dat?\n");
		printf("Ach was, ich erzeuge es.\n");

		file=fopen(MAUSPFAD"MESSAGES\\MSGINFO.DAT","wb");
		fclose(file);
		file=fopen(MAUSPFAD"MESSAGES\\MSGINFO.DAT","rb+");
		strncpy(cb.text,"CATM",4);
		cb.magic=0x00020160L;
		if(fwrite(&cb,sizeof(cb),1,file)==0)
		{
			printf("Schreiben von Msginfo.Dat ging schief!\n");
		}
		fseek(file,0,SEEK_END);
	}
	else
	{
		if(!fread(&cb,sizeof(cb),1,file))
		{
			printf("Fehler beim Lesen fÅr Magic\n");
			fclose(file);
			return -1;
		}
		if(strncmp(cb.text,"CATM",4))
		{
			printf("CATM fehlt!\n");
			fclose(file);
			return -1;
		}
		if(cb.magic!=0x00020160L)
		{
			printf("Magic falsch!\n");
			fclose(file);
			return -1;
		}
	}

#ifdef AN_SEM
	id2.typ=3;
#else
	id2.typ=2;
#endif
	id2.msgnum=1;
	id2.unknown=0;
	id2.versenden=1;
	memset(id2.unknown2,0,6);
	memset(id2.unknown3,0,6);
	id2.msglen=(short)statbuf.st_size;
	while(fread(&id,sizeof(id),1,file))
	{
		if(id.msgnum>=id2.msgnum)
		{
			id2.msgnum=id.msgnum+1;

/*			printf("Msg 199 existiert schon!\n");
			printf("Ich korrigiere sie, in der Hoffnung, daû das richtig ist.\n");
			fseek(file,-sizeof(id),SEEK_CUR);
			fwrite(&id2,sizeof(id),1,file);
			fclose(file);
			return 0;
*/
		}
	}
	fseek(file,0,SEEK_END);
	fwrite(&id2,sizeof(id),1,file);
	fclose(file);

	sprintf(buf, MAUSPFAD"MESSAGES\\MSG%03d.HDR", id2.msgnum);

	file=fopen(buf,"wb");
	fprintf(file,"199%lu\n",maxdatum);
#ifdef AN_SEM
	fprintf(file,"Sem Hîlscher @ HB\n\n\n\n");
#else
	fprintf(file,"\n%s\n%s\n\n", GRUPPE, GRUPPE);
#endif
	fprintf(file,"Highscore %s - %s\n",mins,maxs);
	fclose(file);

	sprintf(buf, MAUSPFAD"MESSAGES\\MSG%03d.TXT", id2.msgnum);
	rename(MAUSPFAD"MESSAGES\\MSG999.TXT", buf);
  #elif (CATVER==0)
  #endif
 #endif
#endif

	free(dat.buf);
#if (CATVER<1)
	free(par.buf);
#endif
	free(info);
	free(psort);

	time(&secbis);

	printf("\nStatistik in %ld Sekunden generiert\n", secbis-secvon);

	if(taste)
		getchar();

	return 0;
}
