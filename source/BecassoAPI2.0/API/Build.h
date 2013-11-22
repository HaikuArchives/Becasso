#ifndef _BUILD_H
#define _BUILD_H

#if defined (BUILDING_BECASSO)
#   define IMPEXP __declspec(dllexport)
#else
#   define IMPEXP __declspec(dllimport)
#endif

#endif
