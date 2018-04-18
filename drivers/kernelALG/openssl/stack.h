/* stack.h */

#ifndef HEADER_STACK_H
#define HEADER_STACK_H

#ifdef  __cplusplus
extern "C" {
#endif

typedef struct stack_st
	{
	int num;
	char **data;
	int sorted;

	int num_alloc;
	int (*comp)(const char * const *, const char * const *);
	} STACK;

#define M_sk_num(sk)		((sk) ? (sk)->num:-1)
#define M_sk_value(sk,n)	((sk) ? (sk)->data[n] : NULL)

int sk_num(const STACK *);
char *sk_value(const STACK *, int);

char *sk_set(STACK *, int, char *);

STACK *sk_new(int (*cmp)(const char * const *, const char * const *));
STACK *sk_new_null(void);
int sk_insert(STACK *sk,char *data,int where);
int sk_find(STACK *st,char *data);
int sk_push(STACK *st,char *data);
void sk_sort(STACK *st);

#ifdef  __cplusplus
}
#endif

#endif
