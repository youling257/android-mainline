/* opensslconf.h */

#if defined(HEADER_BN_H) && !defined(CONFIG_HEADER_BN_H)
#define CONFIG_HEADER_BN_H
#undef BN_LLONG

#undef SIXTY_FOUR_BIT_LONG
#define SIXTY_FOUR_BIT
#undef THIRTY_TWO_BIT
#undef SIXTEEN_BIT
#undef EIGHT_BIT
#endif
