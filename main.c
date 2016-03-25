#include <sys/mman.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <err.h>
#include <string.h>

const unsigned char bitrev[256] =
{
  0x00,0x80,0x40,0xc0,0x20,0xa0,0x60,0xe0,0x10,0x90,0x50,0xd0,0x30,0xb0,0x70,
  0xf0,0x08,0x88,0x48,0xc8,0x28,0xa8,0x68,0xe8,0x18,0x98,0x58,0xd8,0x38,0xb8,
  0x78,0xf8,0x04,0x84,0x44,0xc4,0x24,0xa4,0x64,0xe4,0x14,0x94,0x54,0xd4,0x34,
  0xb4,0x74,0xf4,0x0c,0x8c,0x4c,0xcc,0x2c,0xac,0x6c,0xec,0x1c,0x9c,0x5c,0xdc,
  0x3c,0xbc,0x7c,0xfc,0x02,0x82,0x42,0xc2,0x22,0xa2,0x62,0xe2,0x12,0x92,0x52,
  0xd2,0x32,0xb2,0x72,0xf2,0x0a,0x8a,0x4a,0xca,0x2a,0xaa,0x6a,0xea,0x1a,0x9a,
  0x5a,0xda,0x3a,0xba,0x7a,0xfa,0x06,0x86,0x46,0xc6,0x26,0xa6,0x66,0xe6,0x16,
  0x96,0x56,0xd6,0x36,0xb6,0x76,0xf6,0x0e,0x8e,0x4e,0xce,0x2e,0xae,0x6e,0xee,
  0x1e,0x9e,0x5e,0xde,0x3e,0xbe,0x7e,0xfe,0x01,0x81,0x41,0xc1,0x21,0xa1,0x61,
  0xe1,0x11,0x91,0x51,0xd1,0x31,0xb1,0x71,0xf1,0x09,0x89,0x49,0xc9,0x29,0xa9,
  0x69,0xe9,0x19,0x99,0x59,0xd9,0x39,0xb9,0x79,0xf9,0x05,0x85,0x45,0xc5,0x25,
  0xa5,0x65,0xe5,0x15,0x95,0x55,0xd5,0x35,0xb5,0x75,0xf5,0x0d,0x8d,0x4d,0xcd,
  0x2d,0xad,0x6d,0xed,0x1d,0x9d,0x5d,0xdd,0x3d,0xbd,0x7d,0xfd,0x03,0x83,0x43,
  0xc3,0x23,0xa3,0x63,0xe3,0x13,0x93,0x53,0xd3,0x33,0xb3,0x73,0xf3,0x0b,0x8b,
  0x4b,0xcb,0x2b,0xab,0x6b,0xeb,0x1b,0x9b,0x5b,0xdb,0x3b,0xbb,0x7b,0xfb,0x07,
  0x87,0x47,0xc7,0x27,0xa7,0x67,0xe7,0x17,0x97,0x57,0xd7,0x37,0xb7,0x77,0xf7,
  0x0f,0x8f,0x4f,0xcf,0x2f,0xaf,0x6f,0xef,0x1f,0x9f,0x5f,0xdf,0x3f,0xbf,0x7f,
  0xff
};

int main(int ac, char **av)
{
  struct stat sb;
  char ch_name[128];
  char str[128];
  char *data;
  int max_enc;
  int ch_oenc;
  int ch_enc;
  int maj;
  int min;
  int fdi;
  int fdo;
  int i;
  int j;
  int k;
  int x;
  int y;

  // we need three args
  if (ac != 4)
    errx(EXIT_FAILURE, "usage: mbdf2h <infile.bdf> <outfile.h> <max enc>");
  max_enc = atoi(av[3]);
  
  // open input file
  fdi = open(av[1], O_RDONLY);
  if (fdi == -1)
    err(EXIT_FAILURE, "can't open infile");

  // get input file size
  if (fstat(fdi, &sb) == -1)
    err(EXIT_FAILURE, "can't stat infile");

  // mmap input file
  data = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fdi, 0);
  if (data == MAP_FAILED)
    err(EXIT_FAILURE, "can't mmap infile");

  // open/create output file
  fdo = open(av[2], O_RDWR | O_TRUNC | O_CREAT, 0644);
  if (fdo == -1)
    err(EXIT_FAILURE, "can't open outfile");

  // verify first header line
  if (sscanf(data, "STARTFONT %d.%d\n", &maj, &min) != 2)
    errx(EXIT_FAILURE, "bad font header");
  if (maj != 2)
    errx(EXIT_FAILURE, "bad font header");

  // search for bounding box information
  for (i = 0; i < sb.st_size; i++)
    if (sscanf(data + i, "FONTBOUNDINGBOX %d %d %*d %*d\n",
	       &x, &y) == 2)
      break;
  if (i == sb.st_size)
    errx(EXIT_FAILURE, "can't find bounding box information in header");
  if ((x < 1) || (x > 8))
    errx(EXIT_FAILURE, "this program only handles 1 <= font width <= 8");
  
  // output start of header
  min = snprintf(str, 128, "const unsigned char font[%d][%d] =\n{\n",
		 max_enc, y);
  write(fdo, str, min);
  
  // search for characters
  ch_oenc = -1;
  for (i = 0; i < sb.st_size; i++)
    // if we found a character, handle it
    if (sscanf(data + i, "STARTCHAR %127s\n", ch_name) == 1)
    {
      // search for encoding
      i += (11 + strlen(ch_name));
      if (sscanf(data + i, "ENCODING %d\n", &ch_enc) != 1)
	errx(EXIT_FAILURE, "malformed char '%s' missing encoding", ch_name);

      // end if current encoding > encoding arg, but fill first
      if (ch_enc >= max_enc)
	ch_enc = max_enc;
      
      // fill for missing characters with empty ones
      if (ch_enc - ch_oenc > 1)
      {
	for (j = ch_oenc + 1; j < ch_enc; j++)
	{
	  write(fdo, "  {", 3);
	  for (k = 0; k < y; k++)
	    write(fdo, "0x00,", 5);
	  write(fdo, "},\n", 3);
	}
      }
      ch_oenc = ch_enc;

      // end if arg encoding reached
      if (ch_enc == max_enc)
	break;
      
      // search for character bounding box
      for (i += 11; i < sb.st_size; i++)
      {
	// check that the bounding box is the same as the general one
	if (sscanf(data + i, "BBX %d %d %*d %*d\n", &j, &k) == 2)
	{
	  if ((j != x) || (k != y))
	    errx(EXIT_FAILURE, "this program only handles monospace font");
	  break;
	}
	if (!strncmp(data + i, "ENDCHAR\n", 8))
	  errx(EXIT_FAILURE, "can't find bounding box for char '%s'", ch_name);
      }

      // search for character data
      for (i += 12; i < sb.st_size; i++)
      {
	if (!strncmp(data + i, "BITMAP\n", 7))
	  break;
	if (!strncmp(data + i, "ENDCHAR\n", 8))
	  errx(EXIT_FAILURE, "can't find data for char '%s'", ch_name);
      }

      // start new character output
      write(fdo, "  {", 3);
      
      // read y line of hex numbers
      for (i += 7, j = 0; j < y; j++)
      {
	if (sscanf(data + i, "%x\n%n", &k, &min) != 1)
	  errx(EXIT_FAILURE, "malformed data for char '%s'", ch_name);
	if ((k < 0) || (k > 255))
	  errx(EXIT_FAILURE, "malformed data for char '%s'", ch_name);
	i += min;
	// write data
	min = snprintf(str, 128, "0x%02X,", bitrev[k]);
	write(fdo, str, min);
      }

      // end character
      write(fdo, "},\n", 3);
    }

  // end file
  write(fdo, "};\n", 3);
  
  return EXIT_SUCCESS;
}
