/******************************************
File	:sch.c  
Author	:ningxk
Date	:11/1/2006
******************************************/

//#include <memory.h>
#include <linux/string.h>
#include "sch.h"
//#include "Windows.h"



#define GET_UINT32(n,b,i)                       \
{                                               \
    (n) = ( (uint32) (b)[(i)    ] << 24 )       \
        | ( (uint32) (b)[(i) + 1] << 16 )       \
        | ( (uint32) (b)[(i) + 2] <<  8 )       \
        | ( (uint32) (b)[(i) + 3]       );      \
}

#define PUT_UINT32(n,b,i)                       \
{                                               \
    (b)[(i)    ] = (uint8) ( (n) >> 24 );       \
    (b)[(i) + 1] = (uint8) ( (n) >> 16 );       \
    (b)[(i) + 2] = (uint8) ( (n) >>  8 );       \
    (b)[(i) + 3] = (uint8) ( (n)       );       \
}

void sch_starts( sch_context *ctx )
{
    ctx->total[0] = 0;
    ctx->total[1] = 0;

    ctx->state[0] = 0x7380166F;
    ctx->state[1] = 0x4914B2B9;
    ctx->state[2] = 0x172442D7;
    ctx->state[3] = 0xDA8A0600;
    ctx->state[4] = 0xA96F30BC;
    ctx->state[5] = 0x163138AA;
    ctx->state[6] = 0xE38DEE4D;
    ctx->state[7] = 0xB0FB0E4E;
}

void sch_process( sch_context *ctx, uint8 data[64] )
{
    uint32 W[68],W1[64];
    uint32 A, B, C, D, E, F, G, H;
    uint32 SS1,SS2,TT1,TT2;
    uint32 j,T,FF,GG;

    GET_UINT32( W[0],  data,  0 );
    GET_UINT32( W[1],  data,  4 );
    GET_UINT32( W[2],  data,  8 );
    GET_UINT32( W[3],  data, 12 );
    GET_UINT32( W[4],  data, 16 );
    GET_UINT32( W[5],  data, 20 );
    GET_UINT32( W[6],  data, 24 );
    GET_UINT32( W[7],  data, 28 );
    GET_UINT32( W[8],  data, 32 );
    GET_UINT32( W[9],  data, 36 );
    GET_UINT32( W[10], data, 40 );
    GET_UINT32( W[11], data, 44 );
    GET_UINT32( W[12], data, 48 );
    GET_UINT32( W[13], data, 52 );
    GET_UINT32( W[14], data, 56 );
    GET_UINT32( W[15], data, 60 );

/* ROTATE_LEFT rotates x left n bits.*/
#define ROTL(x, n) (((x) << (n)) | ((x) >> (32-(n))))

#define P0(x) (x ^ ROTL(x, 9) ^ ROTL(x,17))
#define P1(x) (x ^ ROTL(x,15) ^ ROTL(x,23))

    for (j=16;j<68;j++)
    {
//		W[j] = P1( (W[j-16]^W[j-9]^ROTL(W[j-3],15)) ) ^ (ROTL(W[j-13],7)) ^ W[j-6];	
		//上面代码会在release版本出错 linyang 2006.12.11
		uint32 temp;
		temp = W[j-16]^W[j-9]^ROTL(W[j-3],15);
		W[j] = P1( temp ) ^ (ROTL(W[j-13],7)) ^ W[j-6];
    }
    for (j=0;j<64;j++)
    {
    	W1[j] = W[j] ^ W[j+4];
    }
    A = ctx->state[0];
    B = ctx->state[1];
    C = ctx->state[2];
    D = ctx->state[3];
    E = ctx->state[4];
    F = ctx->state[5];
    G = ctx->state[6];
    H = ctx->state[7];

    for (j=0;j<64;j++)
    {
	T = (j<16)?(0x79CC4519):(0x7A879D8A);
	SS1 =  ROTL( ROTL(A,12) + E + ROTL(T,j),7 );
    	SS2 = SS1 ^ ROTL(A,12);
	FF = (j<16)?(A^B^C):((A&B)|(A&C)|(B&C));
	GG = (j<16)?(E^F^G):((E&F)|((~E)&G));
    	TT1 = FF + D + SS2 + W1[j];
    	TT2 = GG + H + SS1 + W[j];
    	D = C;
    	C = ROTL(B,9);
    	B = A;
    	A = TT1;
    	H = G;
    	G = ROTL(F,19);
    	F = E;
    	E = P0(TT2);
    }

    ctx->state[0] ^= A;
    ctx->state[1] ^= B;
    ctx->state[2] ^= C;
    ctx->state[3] ^= D;
    ctx->state[4] ^= E;
    ctx->state[5] ^= F;
    ctx->state[6] ^= G;
    ctx->state[7] ^= H;

}

//#define TUNE_SCH_SPEED 1
#ifdef TUNE_SCH_SPEED

#define SCH_DELAY_TIME_IN_100_NANOSECOND		(-6L)
#define SCH_DELAY_BLOCK_SIZE					(4)

int SchDelay()
{
	HANDLE hTimer = NULL;
	LARGE_INTEGER liDueTime;


	// Create an unnamed waitable timer.
	hTimer = CreateWaitableTimer(NULL, TRUE, NULL);
	if (NULL == hTimer)
	{
		return 1;
	}


	// Set a timer to wait for 10 seconds.
	liDueTime.QuadPart = SCH_DELAY_TIME_IN_100_NANOSECOND; 

	if (!SetWaitableTimer(hTimer, &liDueTime, 0, NULL, NULL, 0))
	{
		return 2;
	}

	// Wait for the timer.
	if (WaitForSingleObject(hTimer, INFINITE) != WAIT_OBJECT_0)
	{
		return 3;
	}

 
	return 0;
}

#endif


void sch_update( sch_context *ctx, uint8 *input, uint32 length )
{
    uint32 left, fill;
	uint32 blockCounter = 0;

    if( ! length ) return;

    left = ctx->total[0] & 0x3F;
    fill = 64 - left;

    ctx->total[0] += length;
    ctx->total[0] &= 0xFFFFFFFF;

    if( ctx->total[0] < length )
        ctx->total[1]++;

    if( left && length >= fill )
    {
        memcpy( (void *) (ctx->buffer + left),
                (void *) input, fill );
        sch_process( ctx, ctx->buffer );
        length -= fill;
        input  += fill;
        left = 0;
    }

    while( length >= 64 )
    {
        sch_process( ctx, input );
        length -= 64;
        input  += 64;

#ifdef TUNE_SCH_SPEED
		blockCounter++;
		if(0 == (blockCounter%SCH_DELAY_BLOCK_SIZE))
		{
			SchDelay();
		}
#endif
    }

    if( length )
    {
        memcpy( (void *) (ctx->buffer + left),
                (void *) input, length );
    }
}

static uint8 sch_padding[64] =
{
 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

void sch_finish( sch_context *ctx, uint8 diglen, uint8 digest[32]  )
{
    uint32 last, padn;
    uint32 high, low;
    uint8 msglen[8];
    uint32 y1[6],y2[5]; 

    high = ( ctx->total[0] >> 29 )
         | ( ctx->total[1] <<  3 );
    low  = ( ctx->total[0] <<  3 );

    PUT_UINT32( high, msglen, 0 );
    PUT_UINT32( low,  msglen, 4 );

    last = ctx->total[0] & 0x3F;
    padn = ( last < 56 ) ? ( 56 - last ) : ( 120 - last );

    sch_update( ctx, sch_padding, padn );
    sch_update( ctx, msglen, 8 );

//    if( diglen==256)
    if( diglen==32)
    {
    	PUT_UINT32( ctx->state[0], digest,  0 );
    	PUT_UINT32( ctx->state[1], digest,  4 );
    	PUT_UINT32( ctx->state[2], digest,  8 );
    	PUT_UINT32( ctx->state[3], digest, 12 );
    	PUT_UINT32( ctx->state[4], digest, 16 );
    	PUT_UINT32( ctx->state[5], digest, 20 );
    	PUT_UINT32( ctx->state[6], digest, 24 );
    	PUT_UINT32( ctx->state[7], digest, 28 );
    }
//   else if( diglen==192)
    else if( diglen==24)
    {
    	y1[0] = ctx->state[0] ^ ctx->state[1] ^ ctx->state[4];
    	y1[1] = ctx->state[1] ^ ctx->state[5];
    	y1[2] = ctx->state[2] ^ ctx->state[6];
    	y1[3] = ctx->state[3] ^ ctx->state[7];
    	y1[4] = ctx->state[5] ^ ctx->state[2];
    	y1[5] = ctx->state[3] ^ ctx->state[6];
    	PUT_UINT32( y1[0], digest,  0 );
    	PUT_UINT32( y1[1], digest,  4 );
    	PUT_UINT32( y1[2], digest,  8 );
    	PUT_UINT32( y1[3], digest, 12 );
    	PUT_UINT32( y1[4], digest, 16 );
    	PUT_UINT32( y1[5], digest, 20 );
    }
//    else if( diglen==160)
    else if( diglen==20)
    {
    	y2[0] = ctx->state[0] ^ ctx->state[1] ^ ctx->state[4];
    	y2[1] = ctx->state[1] ^ ctx->state[5] ^ ctx->state[2];
    	y2[2] = ctx->state[2] ^ ctx->state[6];
    	y2[3] = ctx->state[3] ^ ctx->state[7];
    	y2[4] = ctx->state[3] ^ ctx->state[6];
    	PUT_UINT32( y2[0], digest,  0 );
    	PUT_UINT32( y2[1], digest,  4 );
    	PUT_UINT32( y2[2], digest,  8 );
    	PUT_UINT32( y2[3], digest, 12 );
    	PUT_UINT32( y2[4], digest, 16 );
    }
    
}
