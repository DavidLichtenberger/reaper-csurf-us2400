
###############################################################################
##
## US2400 - Pan-Mode - FKey - 3
##
## Custom action for FKey + 3 on the US-2400 in Pan Mode
##
## You can change the code below to whatever action you wish,
## just DON'T CHANGE THE FILENAME, the binding to the button depends on it
##
###############################################################################


## DUPLICATE SELECTED TRACKS


from reaper_python import *
from sws_python import *

# RPR_ShowConsoleMsg("Pan F 3: Duplicate")

if (RPR_CountSelectedTracks(0) > 0):

	RPR_Undo_BeginBlock()

	# Duplicate tracks
	RPR_Main_OnCommand(40062, 0)

	RPR_Undo_EndBlock("Duplicate selected Tracks", 0)