#include <stdio.h>
#include <math.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

uint8_t palette[8*3] = {
	0, 0, 0,
	255, 255, 255,
	67, 138, 28,
	100, 64, 255,
	191, 0, 0,
	255, 243, 56,
	232, 126, 0,
	194 ,164 , 244 
};

int depalette( uint8_t * color )
{
	int p;
	int mindiff = 100000000;
	int bestc = 0;
	for( p = 0; p < sizeof(palette)/3; p++ )
	{
		int diffr = ((int)color[0]) - ((int)palette[p*3+0]);
		int diffg = ((int)color[1]) - ((int)palette[p*3+1]);
		int diffb = ((int)color[2]) - ((int)palette[p*3+2]);
		int diff = (diffr*diffr) + (diffg*diffg) + (diffb * diffb);
		if( diff < mindiff )
		{
			mindiff = diff;
			bestc = p;
		}
	}
	return bestc;
}


int main( int argc, char ** argv )
{
	if( argc != 3 )
	{
		fprintf( stderr, "Usage: converter [image file] [out file]\n" );
		return -1;
	}

	int x,y,n;
	unsigned char *data = stbi_load(argv[1], &x, &y, &n, 0);
	if( !data )
	{
		fprintf( stderr, "Error: Can't open image.\n" );
		return -6;
	}
	if( x != 600 )
	{
		fprintf( stderr, "Error: image dimensions must be 600 x ??.\n" );
		return -2;
	}
	if( y > 448 )
	{
		y = 448;
	}

	int i, j;
	FILE * fout = fopen( argv[2], "wb" );

	int margin = 448 - y;
	uint8_t line[600/2];
	if( y < 448 )
	{
		int k;
		int ke = margin / 2;
		//memset( line, 0x66, 600/2 );
		memset( line, 0x11, 600/2 );
		for( k = 0; k < ke; k++ )
		{
			fwrite( line, 600/2, 1, fout );
		}
		margin -= ke;
	}
	else 
	{
	}

	if( y > 448 ) y = 448;
	for( j = 0; j < y; j++ )
	{
		for( i = 0; i < x/2; i++ )
		{
			int c1 = depalette( data + n*(i*2 + x*j ) );
			int c2 = depalette( data + n*(i*2 + x*j + 1 ) );
			line[i] = c2 | (c1<<4);
		}
		fwrite( line, 600/2, 1, fout );
	}
	int k;
	//memset( line, 0x66, 600/2 );
	memset( line, 0x11, 600/2 );
	for( k = 0; k < margin; k++ )
	{
		fwrite( line, 600/2, 1, fout );
	}
	stbi_image_free(data);
}



