
###############################################################################
##
## US2400 - Pan-Mode - FKey - 1
##
## Custom action for FKey + 1 on the US-2400 in Pan Mode
##
## You can change the code below to whatever action you wish,
## just DON'T CHANGE THE FILENAME, the binding to the button depends on it
##
###############################################################################


## MOVE SELECTED TRACKS LEFT


from reaper_python import *
from sws_python import *

# RPR_ShowConsoleMsg("Pan F 1: Move left")

if (RPR_CountSelectedTracks(0) > 0):

	RPR_Undo_BeginBlock()

	insert = -1

	# look for leftmost selected track
	for i in range(0, RPR_CountTracks(0)):
		tk = RPR_GetTrack(0, i)
		
		if ((RPR_IsTrackSelected(tk)) and (insert == -1)):
			insert = i

	# go one left if possible
	insert = insert - 2

	#cut with sends and receives
	cmd_id = RPR_NamedCommandLookup("_S&M_CUTSNDRCV1")
	RPR_Main_OnCommand(cmd_id, 0)

	if (insert < 0):
		target = RPR_GetMasterTrack(RPR_EnumProjects(-1, 0, 0))
	else: 
		target = RPR_GetTrack(0, insert)
	RPR_SetOnlyTrackSelected(target)

	cmd_id = RPR_NamedCommandLookup("_S&M_PASTSNDRCV1")
	RPR_Main_OnCommand(cmd_id, 0)

	RPR_Undo_EndBlock("Move selected tracks left", 0)