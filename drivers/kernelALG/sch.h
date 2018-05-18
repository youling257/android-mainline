/* sch.h */

#ifndef HEADER_SCH_H
#define HEADER_SCH_H

#ifdef  __cplusplus
extern "C" {
#endif

#ifndef uint8
#define uint8  unsigned char
#endif

#ifndef uint32
//#define uint32 unsigned long int
#define uint32 unsigned int
#endif

typedef struct
{
    uint32 total[2];
    uint32 state[8];
    uint8 buffer[64];
}
sch_context;


void sch_starts( sch_context *ctx );
void sch_update( sch_context *ctx, uint8 *input, uint32 length );
void sch_finish( sch_context *ctx, uint8 diglen, uint8 digest[32] );


#ifdef  __cplusplus
}
#endif

#endif
