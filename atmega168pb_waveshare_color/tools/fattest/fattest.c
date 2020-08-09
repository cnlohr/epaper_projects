#include "basicfat.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

int sd_image;

void _delay_ms( uint32_t ms )
{
	usleep( ms * 1000 );
}

void sendchr( char chr )
{
	printf( "%c", chr );
}

void sendhex4( uint16_t hex )
{
	printf( "%04x", hex );
}
void sendhex2( uint8_t hex )
{
	printf( "%02x", hex );
}

void dumpSDDAT( unsigned short num )
{
	lseek( sd_image, num, SEEK_CUR ) < 0;
}

uint16_t opsleftSD = 0;

unsigned char startSDread( uint32_t sector )
{
	opsleftSD = 512;
	return lseek( sd_image, sector * 512, SEEK_SET ) < 0;
}

unsigned char popSDread()
{
	unsigned char ret[1];
	int r = read( sd_image, ret, 1 );
	return ret[0];
}

unsigned char endSDread()
{
	return 0; //returns nonzero if the CRC failed.
}

int main()
{
	int r;
	sd_image = open( "/dev/sda", O_RDONLY );
	if( sd_image <= 0 )
	{
		fprintf( stderr, "Could not open image.\n" );
		return -5;
	}

	r = openFAT();
	printf( "openFAT() = %d\n", r );

	int filelen;
	char fname[MAX_LONG_FILENAME];
	//memcpy( fname, "001.dat", 8 );
	r = FindClusterFileInDir( fname, ROOT_CLUSTER, 2, &filelen );
	printf( "FindClusterFileInDir: %d %d\n", r, filelen );
	if( r )
	{
		//ClearEpaper( EPD_5IN65F_YELLOW );
		struct FileInfo f;
		InitFileStructure( &f, r );
		StartReadFAT(&f);

//		SetupEPaperForData();
		uint32_t i;
		int j;
		for( i = 0; i < filelen; i+=512 )
		{
			char dat[1];
			for( j = 0; j < 512; j++ )
			{
				dat[0] = read8FAT();
				//dat[0] = EPD_5IN65F_YELLOW | (EPD_5IN65F_YELLOW<<4);
				//SendEPaperData( dat, 1 );
				printf( "%d\n", dat[0] );
			}
		}
		//SetupEpaperDone();
		EndReadFAT();
	}

	printf( "RSF: %s %d %d\n", fname, r, filelen );

}

