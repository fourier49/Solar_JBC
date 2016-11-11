#include <stdio.h>
#include <string.h>
/* Application include files. */	
#include "configuration.h"
#include "Environment.h"
#include "Environment_global.h"
int Get_EvnString(void)
{
	char* ptr ;
	int stringCount=0;
	char StringBuf[10][50];
	char TempBuf[100];
	int tempbufindex=0;
	int retConut=0;
	char* parserStart;
	int i;

	ptr=(char*)EnvStartAdd;
	memset(StringBuf,0,sizeof(StringBuf));

	while(retConut<ContiRetCount && tempbufindex<40)
	{

		if(*ptr==0x0d && *(ptr+1)==0x0a)
		{
			TempBuf[tempbufindex]=0;
			strcpy(StringBuf[stringCount],TempBuf);
			stringCount+=1;
			tempbufindex=0;
			retConut=0;
			ptr++;
		}
		else
		{
			if(*ptr==0xff)
			{
				retConut++;
			}
			TempBuf[tempbufindex++]=*ptr;

		}

		ptr++;


	}

	for( i=0;i<stringCount;i++)
	{
		if((parserStart=strstr(StringBuf[i],"MAC=")))
		{
			SaveMac2Evn(parserStart+strlen("MAC="));
		}

		if((parserStart=strstr(StringBuf[i],"SN=")))
		{
			SaveSNEvn(parserStart+strlen("SN="));
		}

	}


	return stringCount;
}

int SaveMac2Evn(char*Start)
{
		uint8_t RetHex[1];


		G_ENVCONFIG.Mac[0]=*hex_decode(Start,2,RetHex);
		Start=strstr(Start,":")+1;
		G_ENVCONFIG.Mac[1]=*hex_decode(Start,2,RetHex);
		Start=strstr(Start,":")+1;
		G_ENVCONFIG.Mac[2]=*hex_decode(Start,2,RetHex);
		Start=strstr(Start,":")+1;
		G_ENVCONFIG.Mac[3]=*hex_decode(Start,2,RetHex);
		Start=strstr(Start,":")+1;
		G_ENVCONFIG.Mac[4]=*hex_decode(Start,2,RetHex);
		Start=strstr(Start,":")+1;
		G_ENVCONFIG.Mac[5]=*hex_decode(Start,2,RetHex);

		return 0;
}

int SaveSNEvn(char*Start)
{
		int i=0;
		G_ENVCONFIG.SeriesNumber[0]=*(Start+i++)-0x30;
		G_ENVCONFIG.SeriesNumber[1]=*(Start+i++)-0x30;
		G_ENVCONFIG.SeriesNumber[2]=*(Start+i++)-0x30;
		G_ENVCONFIG.SeriesNumber[3]=*(Start+i++)-0x30;
		G_ENVCONFIG.SeriesNumber[4]=*(Start+i++)-0x30;
		G_ENVCONFIG.SeriesNumber[5]=*(Start+i++)-0x30;
		G_ENVCONFIG.SeriesNumber[6]=*(Start+i++)-0x30;
		G_ENVCONFIG.SeriesNumber[7]=*(Start+i++)-0x30;


		return 0;
}

uint8_t* hex_decode(char *in, size_t len, uint8_t *out)
{
    unsigned int i, t, hn, ln;

    for (t = 0,i = 0; i < len; i+=2,++t) {

            hn = in[i] > '9' ? (in[i]|32) - 'a' + 10 : in[i] - '0';
            ln = in[i+1] > '9' ? (in[i+1]|32) - 'a' + 10 : in[i+1] - '0';

            out[t] = (hn << 4 ) | ln;

    }
    return out;

}
