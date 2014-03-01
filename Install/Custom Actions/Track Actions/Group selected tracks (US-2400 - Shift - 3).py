
###############################################################################
##
## US2400 - Pan-Mode - Shift - 3
##
## Custom action for Shift + 3 on the US-2400 in Pan Mode
##
## You can change the code below to whatever action you wish,
## just DON'T CHANGE THE FILENAME, the binding to the button depends on it
##
###############################################################################


## GROUP SELECTED TRACKS


from reaper_python import *
from sws_python import *

# RPR_ShowConsoleMsg("Pan Shift 3: Group")

if (RPR_CountSelectedTracks(0) > 0):

	RPR_Undo_BeginBlock()
	
	cmd_id = RPR_NamedCommandLookup("_S&M_SET_TRACK_UNUSEDGROUP")
	RPR_Main_OnCommand(cmd_id, 0)

	RPR_Undo_EndBlock("Group selected tracks with default parameters", 0)