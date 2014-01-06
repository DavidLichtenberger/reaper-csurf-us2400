
###############################################################################
##
## US2400 - Pan-Mode - Shift - 6
##
## Custom action for Shift + 6 on the US-2400 in Pan Mode
##
## You can change the code below to whatever action you wish,
## just DON'T CHANGE THE FILENAME, the binding to the button depends on it
##
###############################################################################


## UNGROUP SELECTED TRACKS


from reaper_python import *
from sws_python import *

# RPR_ShowConsoleMsg("Pan Shift 6: Ungroup")

if (RPR_CountSelectedTracks(0) > 0):

	RPR_Undo_BeginBlock()

	cmd_id = RPR_NamedCommandLookup("_S&M_REMOVE_TR_GRP")
	RPR_Main_OnCommand(cmd_id, 0)

	RPR_Undo_EndBlock("Remove selected tracks from all groups they're in", 0)