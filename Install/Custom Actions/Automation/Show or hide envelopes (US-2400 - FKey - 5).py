
###############################################################################
##
## US2400 - Pan-Mode - FKey - 5
##
## Custom action for FKey + 5 on the US-2400 in Pan Mode
##
## You can change the code below to whatever action you wish,
## just DON'T CHANGE THE FILENAME, the binding to the button depends on it
##
###############################################################################


## SHOW HIDE ENVELOPES FOR SELECTED TRACKS / ALL TRACKS


from reaper_python import *
from sws_python import *

# RPR_ShowConsoleMsg("Pan F 5: Show/Hide Envelopes")

RPR_Undo_BeginBlock()

no_sel = False

if (RPR_CountSelectedTracks(0) == 0):
	
	no_sel = True

	# none selected: select all
	for i in range(0, RPR_CountTracks(0)):
		RPR_SetTrackSelected(RPR_GetTrack(0, i), True)

# Toggle show all envelopes for tracks
RPR_Main_OnCommand(41151, 0)

# if no selection, reset to no selection
if (no_sel):
	for i in range(0, RPR_CountTracks(0)):
		RPR_SetTrackSelected(RPR_GetTrack(0, i), False)

RPR_Undo_EndBlock("Show / hide envelopes on selected / all tracks", 0)