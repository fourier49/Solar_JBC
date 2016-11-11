#define EnvStartAdd 0x1800
#define ContiRetCount	10

typedef struct ENVCONFIG
{
	uint8_t Mac[6];
	uint8_t SeriesNumber[8];


}ENVCONFIG;
////////////////////////GOBAL VALUE//////////////
ENVCONFIG G_ENVCONFIG;
/////////////////////////////////////////////////