typedef unsigned long bgra_pixel;

#if defined (__cplusplus)
extern "C"
#endif
int mmx_available (void);

#if defined (__cplusplus)
extern "C"
#endif
void mmx_alpha_blend (bgra_pixel *src, bgra_pixel *dest, int ga, int w, int l, int t, int r, int b);
