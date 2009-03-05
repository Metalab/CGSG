#include "portmidi.h"
#include "porttime.h"
#include "pmutil.h"

typedef void midiCallbackFuncType(void *userdata, PmEvent *buffer);
int startMidi(midiCallbackFuncType *CBFptr, void *userdata);
//int startMidi(void(*CBptr)( void *userdata));

void finalize();