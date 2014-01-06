
###############################################################################
##
## US2400 - Pan-Mode - Shift - 1
##
## Custom action for Shift + 1 on the US-2400 in Pan Mode
##
## You can change the code below to whatever action you wish,
## just DON'T CHANGE THE FILENAME, the binding to the button depends on it
##
###############################################################################


## WRAP SELECTED TRACKS IN FOLDER, 
## OR UNWRAP CONTENTS OF FOLDERS (IF ANY SELECTED)


from reaper_python import *
from sws_python import *

# RPR_ShowConsoleMsg("Pan Shift 1: Wrap/Unwrap")

if (RPR_CountSelectedTracks(0) > 0):

	RPR_Undo_BeginBlock()

	# Get info
	insert = -1
	folder = 0


	for i in range(0, RPR_CountTracks(0)):
		tk = RPR_GetTrack(0, i)
		
		if (RPR_IsTrackSelected(tk)):

			# is folder
			depth = int(RPR_GetMediaTrackInfo_Value(tk, 'I_FOLDERDEPTH'))
			if (depth == 1):
				folder = tk

			# look for leftmost selected track	
			if (insert == -1):
				insert = i


	# A wrap 

	if (folder == 0) :

		# 1 bundle tracks together

		insert = insert - 1

		#cut with sends and receives
		cmd_id = RPR_NamedCommandLookup("_S&M_CUTSNDRCV1")
		RPR_Main_OnCommand(cmd_id, 0)

		if (insert < 0):
			target = RPR_GetMasterTrack(RPR_EnumProjects(-1, 0, 0))
		else: 
			target = RPR_GetTrack(0, insert)

		RPR_SetOnlyTrackSelected(target)

		#paste with sends and receives
		cmd_id = RPR_NamedCommandLookup("_S&M_PASTSNDRCV1")
		RPR_Main_OnCommand(cmd_id, 0)

		# 2 save selection
		selection = []

		for i in range(RPR_CountSelectedTracks(0)):
			selection.append(RPR_GetSelectedTrack(0, i))

		# 3 insert folder track
		cmd_id = RPR_NamedCommandLookup("_SWS_INSRTTRKABOVE")
		RPR_Main_OnCommand(cmd_id, 0)

		# 4 restore selection
		for track in selection:
			RPR_SetTrackSelected(track, True)

		# 5 make folder
		cmd_id = RPR_NamedCommandLookup("_SWS_MAKEFOLDER")
		RPR_Main_OnCommand(cmd_id, 0)

		RPR_Undo_EndBlock("Wrap selected Tracks", 0)

	# B Unwrap
	else :

		# save selction withou folder
		selection = []

		for i in range(RPR_CountSelectedTracks(0)):
			tr = RPR_GetSelectedTrack(0, i)
			if (tr != folder):
				selection.append(tr)

		# select only folder
		RPR_SetOnlyTrackSelected(folder)

		# dismantle folder
		cmd_id = RPR_NamedCommandLookup("_XENAKIOS_SELTRACKTONOTFOLDER")
		RPR_Main_OnCommand(cmd_id, 0)

		# delete folder
		RPR_DeleteTrack(folder)

		#restore selection
		for track in selection:
			RPR_SetTrackSelected(track, True)

		RPR_Undo_EndBlock("Unwrap selected Tracks", 0)