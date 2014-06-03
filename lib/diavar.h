#ifndef DIAVAR_H
#define DIAVAR_H

#include <glib.h>
#include <stdio.h>

//#ifdef G_OS_WIN32
//#  ifdef LIBDIA_COMPILATION
//#    define DIAVAR __declspec(dllexport)
//#  else  /* !LIBDIA_COMPILATION */
//#    define DIAVAR extern __declspec(dllimport)
//#  endif /* !LIBDIA_COMPILATION */
//#else  /* !G_OS_WIN32 */
//#  define DIAVAR extern
//#endif
#define DIAVAR extern __declspec(dllexport)
/*! bounding box debug helper : set to !0 to see the caclulated bounding boxes */
DIAVAR int render_bounding_boxes;
DIAVAR FILE *logfd ;

#endif
