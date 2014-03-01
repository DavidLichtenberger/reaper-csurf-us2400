
###############################################################################
##
## US2400 - Pan-Mode - Shift - 4
##
## Custom action for Shift + 4 on the US-2400 in Pan Mode
##
## You can change the code below to whatever action you wish,
## just DON'T CHANGE THE FILENAME, the binding to the button depends on it
##
###############################################################################


## INSERT NEW TRACK BEFORE FIRST SELECTED TRACK


from reaper_python import *
from sws_python import *

# RPR_ShowConsoleMsg("Pan Shift 4: Insert")

RPR_Undo_BeginBlock()

if (RPR_CountSelectedTracks(0) == 0):

	# basic insert track (somewhere, probably last touched or something)
	RPR_Main_OnCommand(40001, 0)

	RPR_Undo_EndBlock("Insert track", 0)

else:

	# insert track before selected
	cmd_id = RPR_NamedCommandLookup("_SWS_INSRTTRKABOVE")
	RPR_Main_OnCommand(cmd_id, 0)

	RPR_Undo_EndBlock("Insert track before selected tracks", 0)