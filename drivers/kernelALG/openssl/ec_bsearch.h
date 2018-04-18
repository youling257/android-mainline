/* bsearch.h */

void *wapi_Bsearch(const void *key, const void *base, size_t num, size_t width, int (__cdecl *compare)(const void *, const void *));