#pragma once
#ifdef __cplusplus
extern "C" {
#endif

typedef struct ao_info_s
{
        /* driver name ("Matrox Millennium G200/G400" */
        const char *name;
        /* short name (for config strings) ("mga") */
        const char *short_name;
        /* author ("Aaron Holtzman <aholtzma@ess.engr.uvic.ca>") */
        const char *author;
        /* any additional comments */
        const char *comment;
} ao_info_t;


typedef struct ao_functions_s
{
				ao_info_t *info;
        int (*control)(int cmd,void *arg);
        int (*init)(int rate,int channels,int format,int flags);
        void (*uninit)();
        void (*reset)();
        int (*get_space)();
        int (*play)(void* data,int len,int flags);
        float (*get_delay)();
        void (*pause)();
        void (*resume)();
} ao_functions_t;


#define CONTROL_OK 1
#define CONTROL_TRUE 1
#define CONTROL_FALSE 0
#define CONTROL_UNKNOWN -1
#define CONTROL_ERROR -2
#define CONTROL_NA -3

#define AOCONTROL_SET_DEVICE 1
#define AOCONTROL_GET_DEVICE 2
#define AOCONTROL_QUERY_FORMAT 3 /* test for availabilty of a format */
#define AOCONTROL_GET_VOLUME 4
#define AOCONTROL_SET_VOLUME 5
#define AOCONTROL_SET_PLUGIN_DRIVER 6
#define AOCONTROL_SET_PLUGIN_LIST 7

typedef struct ao_control_vol_s {
	float left;
	float right;
} ao_control_vol_t;

/* standard, old OSS audio formats */
#ifndef AFMT_MU_LAW
#	define AFMT_MU_LAW		0x00000001
#	define AFMT_A_LAW		0x00000002
#	define AFMT_IMA_ADPCM		0x00000004
#	define AFMT_U8			0x00000008
#	define AFMT_S16_LE		0x00000010	/* Little endian signed 16*/
#	define AFMT_S16_BE		0x00000020	/* Big endian signed 16 */
#	define AFMT_S8			0x00000040
#	define AFMT_U16_LE		0x00000080	/* Little endian U16 */
#	define AFMT_U16_BE		0x00000100	/* Big endian U16 */
#endif

#ifndef AFMT_MPEG
#	define AFMT_MPEG		0x00000200	/* MPEG (2) audio */
#endif

#ifndef AFMT_AC3
#	define AFMT_AC3			0x00000400	/* Dolby Digital AC3 */
#endif

/* 32 bit formats (MSB aligned) formats */
#ifndef AFMT_S32_LE
# define AFMT_S32_LE              0x00001000
# define AFMT_S32_BE              0x00002000
#endif


/* native endian formats */
#ifndef	AFMT_S16_NE
# if WORDS_BIGENDIAN
#  define AFMT_S16_NE	AFMT_S16_BE
#  define AFMT_S32_NE	AFMT_S32_BE
# else
#  define AFMT_S16_NE	AFMT_S16_LE
#  define AFMT_S32_NE	AFMT_S32_LE
# endif
#endif

#ifndef AFMT_FLOAT
# define AFMT_FLOAT               0x00004000
#endif
/* global data used by mplayer and plugins */
typedef struct ao_data_s
{
  int samplerate;
  int channels;
  int format;
  int bps;
  int outburst; 	  
  int buffersize;         
  int pts;
} ao_data_t;

extern ao_functions_t audio_functions;

extern ao_data_t* GetAOData();
extern int audio_out_format_bits(int format);

#pragma once
#ifdef __cplusplus
}
#endif