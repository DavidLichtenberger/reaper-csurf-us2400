
###############################################################################
##
## US2400 - Pan-Mode - FKey - 2
##
## Custom action for FKey + 2 on the US-2400 in Pan Mode
##
## You can change the code below to whatever action you wish,
## just DON'T CHANGE THE FILENAME, the binding to the button depends on it
##
###############################################################################


## MOVE SELECTED TRACKS RIGHT


from reaper_python import *
from sws_python import *

# RPR_ShowConsoleMsg("Pan F 2: Move right")

sel_len = RPR_CountSelectedTracks(0)

if (sel_len > 0):

	RPR_Undo_BeginBlock()
	
	insert = -1

	# look for rightmost selected track
	for i in range(0, RPR_CountTracks(0)):
		tk = RPR_GetTrack(0, i)
		
		if (RPR_IsTrackSelected(tk)):
			insert = i

	insert = insert - sel_len + 1

	#cut with sends and receives
	cmd_id = RPR_NamedCommandLookup("_S&M_CUTSNDRCV1")
	RPR_Main_OnCommand(cmd_id, 0)

	target = RPR_GetTrack(0, insert)
	RPR_SetOnlyTrackSelected(target)

	cmd_id = RPR_NamedCommandLookup("_S&M_PASTSNDRCV1")
	RPR_Main_OnCommand(cmd_id, 0)

	RPR_Undo_EndBlock("Move selected tracks right", 0)