/*
 * Sample runtime overlay manager.
 */

typedef enum { FALSE, TRUE } bool;

/* Entry Points: */

bool OverlayLoad (unsigned long ovlyno);
bool OverlayUnload (unsigned long ovlyno);
