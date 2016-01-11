#ifdef _WIN32

/* Not sure why this is defined, but it adds the dllexport/import crap to
 * function definitions, which we don't want.
 */
#undef _DLL

/* Both MikMod and MinGW picked the _mm_ prefix for these. */
#undef _mm_free
#undef _mm_malloc

#endif
