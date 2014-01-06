
###############################################################################
##
## US2400 - Pan-Mode - FKey - 6
##
## Custom action for FKey + 6 on the US-2400 in Pan Mode
##
## You can change the code below to whatever action you wish,
## just DON'T CHANGE THE FILENAME, the binding to the button depends on it
##
###############################################################################


## REMOVE ENVELOPES FROM SELECTED TRACKS


from reaper_python import *
from sws_python import *

# RPR_ShowConsoleMsg("Pan F 6: Remove Envelopes")

if (RPR_CountSelectedTracks(0) > 0):

	RPR_Undo_BeginBlock()
	
	RPR_Main_OnCommand(RPR_NamedCommandLookup("_S&M_REMOVE_ALLENVS"), 0)

	RPR_Undo_EndBlock("Remove envelopes from selected tracks", 0)
