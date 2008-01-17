/********************************************************************
 * cecbpath.h -Cassette BASIC path definitions header file
 *
 * $Id$
 ********************************************************************/

#ifndef	_CECBPATH_H
#define	_CECBPATH_H

#ifdef __cplusplus
extern "C" {
#endif

#define CAS_FILE_EXTENSION ".cas"
#define WAV_FILE_EXTENSION ".wav"

#include "util.h"

/* File descriptor */
/* Casette BASIC doesn't have a file descriptor per se, but we use this structure as one. */

typedef struct _cecb_dir_entry
{
//	u_char	magic1;
//		/* 0x55
//		*/
//	u_char	magic2;
//		/* 0x3c
//		*/
//	u_char block_type;
//		/* 0x00 for directory entries
//		*/
//	u_char block_length;
//		/* Number of bytes that follow
//		*/
	u_char	filename[8];
		/* Left justified, space filled
		*/
	u_char	file_type;
		/* 00 - BASIC
		   01 - BASIC Data
		   02 - Machine Language program
		   03 - Text Editor Source
		*/
	u_char	ascii_flag;
		/* 00 - Binary file
		   FF - ASCII file
		*/
	u_char	gap_flag;
		/* 00 - No gap
		   FF - gap
		*/
	u_char	ml_load_address1;
	u_char	ml_load_address2;
		/* Machine language load address
		*/
	u_char	ml_exec_address1;
	u_char	ml_exec_address2;
		/* Machine language execution address
		*/
//	u_char	checksum;
//		/* 8 bit checksum from block_type thru ml_exec_address, inclusive
//		*/
//	u_char	magic3;
//		/* 0x55
//		*/
} cecb_dir_entry;

typedef enum { NONE=0, CAS, WAV } _tape_type;
typedef enum { AUTO=0, ODD, EVEN } _wave_parity;

#define WAV_SAMPLE_MUL (path->wav_bits_per_sample == 8 ? 1 : 2)

typedef struct _cecb_path_id
{
	int				mode;					/* access mode */
	_tape_type		tape_type;				/* true for WAV files, false for CAS files */
	char			imgfile[512];			/* pointer to image file */
	char			filename[8];			/* Filename requested */
	cecb_dir_entry  dir_entry;
	unsigned int	filepos;				/* file position */
	int				israw;					/* No file I/O possible, just get/set blocks */
	long			play_at;				/* Sample or bit to begin reading */
	unsigned char	data[256];				/* Current blocks data */
	unsigned char	block_type;				/* The block type of held data */
	int				current_pointer;		/* Current location in current block */
	unsigned char	length;					/* Length of data in above block */
	int				eof_flag;				/* End of file flag. Set when last block read */
	long			cas_start_byte;			/* Byte where file starts */
	unsigned char	cas_start_bit;			/* Bit where file starts.  Fist bit of block type. */
	long			cas_current_byte;		/* byte position in CAS file */
	unsigned char   cas_current_bit;		/* bit position in byte of CAS file */
	unsigned char	cas_byte;				/* current byte read from file */
	unsigned int	wav_riff_size;
	long			wav_data_start;			/* File position of start of data chunk */
	long			wav_data_length;		/* Length of data chunk */
	long			wav_total_samples;		/* Tot number of samples in data */
	unsigned int	wav_sample_rate;		/* Sample rate of WAV file */
	unsigned short	wav_bits_per_sample;	/* Bits per sample of WAV file */
	double			wav_threshold;			/* Remove noise below this threshold */
	double			wav_frequency_limit;	/* Bit Deliniation frequency limit */
	long			wav_start_sample;		/* Sample where file starts. Fist bit of block type. */
	long			wav_current_sample;		/* Current sample position in WAV file */
	_wave_parity	wav_parity;				/* Even or Odd wav type */
	short			wav_ss1, wav_ss2;		/* Wave Phase timing */
	FILE			*fd;					/* file path pointer */
} *cecb_path_id;

error_code _cecb_create(cecb_path_id *path, char *pathlist, int mode, int file_type, int data_type, int gap, int ml_load_address, int ml_exec_address);
error_code _cecb_open(cecb_path_id *path, char *pathlist, int mode );
error_code _cecb_close(cecb_path_id path);
error_code _cecb_parse_cas( cecb_path_id path );
error_code _cecb_parse_riff( cecb_path_id path );
error_code _cecb_read( cecb_path_id path, void *buffer, u_int *size );
error_code _cecb_readln(cecb_path_id path, void *buffer, u_int *size);
error_code _cecb_gs_eof(cecb_path_id path);
error_code _cecb_gs_pos(cecb_path_id path, u_int *pos);
error_code _cecb_read_next_dir_entry( cecb_path_id path, cecb_dir_entry *dir_entry );
error_code _cecb_read_next_block( cecb_path_id path, unsigned char *block_type, unsigned char *block_length, unsigned char *data  );
error_code _cecb_read_bits( cecb_path_id path, int count, unsigned char *result );
error_code _cecb_read_bits_wav( cecb_path_id path, int count, unsigned char *result );
error_code _cecb_read_bits_cas( cecb_path_id path, int count, unsigned char *result );

/* WAV and CAS global settings copied by _cecb_open and _cecb_create */
extern double cecb_threshold;
extern double cecb_frequency;
extern _wave_parity cecb_wave_parity;
extern long cecb_start_sample;

#include <cocopath.h>

#ifdef __cplusplus
}
#endif

#endif	/* _CECBPATH_H */
