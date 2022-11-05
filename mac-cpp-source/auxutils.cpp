#include "Types.h"
#include "Gestalt.h"
#include "Dialogs.h"
#include "SegLoad.h"

#include "auxutils.h"

void auxinit(int alertID) {
	long auxversion;
	OSErr err;
	
	err = Gestalt('a/ux', &auxversion);
	
	if (err != noErr || auxversion < 2) {
		StopAlert(alertID, nil);
		
		ExitToShell();
	}
}