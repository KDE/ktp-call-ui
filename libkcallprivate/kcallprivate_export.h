#ifndef KCALLPRIVATE_EXPORT_H
#define KCALLPRIVATE_EXPORT_H

#include <kdemacros.h>

#ifndef KCALLPRIVATE_EXPORT
# if defined(MAKE_KCALLPRIVATE_LIB)
#  define KCALLPRIVATE_EXPORT KDE_EXPORT
# else
#  define KCALLPRIVATE_EXPORT KDE_IMPORT
# endif
#endif

#endif
