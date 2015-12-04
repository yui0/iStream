//---------------------------------------------------------
//	Motion JPEG
//
//		©2013 Yuichiro Nakada
//---------------------------------------------------------

#define FOUR_ZERO_ZERO	0
#define FOUR_TWO_ZERO	1
#define FOUR_TWO_TWO	2
#define FOUR_FOUR_FOUR	3
#define RGB		4
#define RGB32		5

#define BLOCK_SIZE	64

unsigned char *encode_image(unsigned char *, unsigned char *, unsigned int, unsigned int, unsigned int, unsigned int);
