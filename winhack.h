/* The gdidefs.h header badly conflicts with windows.h. Microsoft's printer
 * drivers in the Windows DDK solve that by manually defining a large subset
 * of windows.h. We solve that by including the normal windows.h but hiding
 * the conflicting definitions.
 */
#define NOGDI
#define PPOINT  n_PPOINT
#define LPPOINT n_LPPOINT
#define RECT    n_RECT
#define PRECT   n_PRECT
#define LPRECT  n_LPRECT
#include <windows.h>
#undef  PPOINT
#undef  LPPOINT
#undef  RECT
#undef  PRECT
#undef  LPRECT

