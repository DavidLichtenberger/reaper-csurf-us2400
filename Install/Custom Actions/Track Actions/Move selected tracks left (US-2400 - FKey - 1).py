
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

from time import *
from reaper_python import *
from sws_python import *

RPR_ShowConsoleMsg("")

sel_tks = RPR_CountSelectedTracks(0)

if (sel_tks > 0):

	RPR_Undo_BeginBlock()

	sel = []
	all_tks = RPR_CountTracks(0)
	
	# find leftmost in selection
	leftmost_tk = RPR_GetSelectedTrack(0, 0)
	
	# find target track
	target_id = int(RPR_GetMediaTrackInfo_Value(leftmost_tk, "IP_TRACKNUMBER")) - 3
	target_tk = leftmost_tk
	
	# if not already left
	if target_id > -2:
	
		if target_id == -1:
			target_tk = RPR_GetMasterTrack(0)

		else: target_tk = RPR_GetTrack(0, target_id)


		# find first visible target if not master
		while (bool(RPR_GetMediaTrackInfo_Value(target_tk, "B_SHOWINMIXER")) != True) and (target_id >= 0):

			target_id -= 1
			target_tk = RPR_GetTrack(0, target_id)
		

	# target found?
	if (target_tk != leftmost_tk) :

		RPR_Main_OnCommand(40337, 0) # cut

		RPR_SetOnlyTrackSelected(target_tk)
		RPR_Main_OnCommand(40914, 0) #last touched to sel

		RPR_Main_OnCommand(40058, 0) # paste

	RPR_Undo_EndBlock("Move selected tracks right", 0)