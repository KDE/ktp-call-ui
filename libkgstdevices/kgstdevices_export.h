#ifndef KGSTDEVICES_EXPORT_H
#define KGSTDEVICES_EXPORT_H

#include <kdemacros.h>

#ifndef KGSTDEVICES_EXPORT
# if defined(MAKE_KGSTDEVICES_LIB)
#  define KGSTDEVICES_EXPORT KDE_EXPORT
# else
#  define KGSTDEVICES_EXPORT KDE_IMPORT
# endif
#endif

#endif
