import re
from reaper_python import *
from sws_python import *

sel_safe = [] # for Selection_Save() and Selection_Restore()

# save selection to restore later
def Selection_Save() :
	
	# go through selected tracks
	for i in range(0, RPR_CountSelectedTracks(0)) :
		
		tk = RPR_GetSelectedTrack(0, i)

		# save
		sel_safe.append(tk)



# restore saved selection
def Selection_Restore() :

	# deselect all tracks
	tk = RPR_GetTrack(0, 0)
	RPR_SetOnlyTrackSelected(tk)
	RPR_SetTrackSelected(tk, False)

	# restore
	for tk in sel_safe :
		RPR_SetTrackSelected(tk, True)		



def Chunk_GetChunk(tk) :

	chunk = ""
	chunk = RPR_GetSetTrackState(tk, chunk, 100000)[2]

	return chunk



def Chunk_SafeChunk(tk, chunk) :

	RPR_GetSetTrackState(tk, chunk, 100000)



def Chunk_CountSections(sec_id, chunk) :

	sec_pat = re.compile(r'^<' + sec_id + '.*', re.MULTILINE)
	results = re.findall(sec_pat, chunk)

	return len(results)



def Chunk_GetSectionStart(sec_id, sec_num, chunk) :

	sec_pat = re.compile(r'^<' + sec_id + '.*', re.MULTILINE)

	out = []

	i = 0
	for match in sec_pat.finditer(chunk) :

		if i == sec_num :
			out.append(match.start())
			out.append(match.end() + 1)

		i += 1

	return out



def Chunk_CountLines(line_id, chunk) :

	line_pat = re.compile(r'^' + line_id + ' .*', re.MULTILINE)
	results = re.findall(line_pat, chunk)

	return len(results)



def Chunk_GetLine(line_id, line_num, chunk) :

	line_pat = re.compile(r'^' + line_id + ' .*', re.MULTILINE)

	out = []

	i = 0
	for match in line_pat.finditer(chunk) :

		if i == line_num :
			out.append(match.start())
			out.append(match.end() + 1)

		i += 1

	return out



def Chunk_GetValue(line_id, line_num, chunk) :

	line_pat = re.compile(r'^' + line_id + ' .*', re.MULTILINE)
	results = re.findall(line_pat, chunk)

	return results[line_num].replace(line_id + " ", "")



def Chunk_ChangeLine(line_id, line_num, new_val, chunk) :

	out = "Error: Chunk_ChangeLine(\"" + line_id + "\", " + str(line_num) + ", \"" + new_val + "\")"

	line_pos = Chunk_GetLine(line_id, line_num, chunk)	

	if (len(line_pos) > 0) :
		
		out = chunk[:line_pos[0]] + line_id + " " + new_val + "\n" + chunk[line_pos[1]:]

	return out



def Chunk_InsertLine(line, sec_id, sec_num, chunk) :

	out = "Error: Chunk_InsertLine(\"" + line + "\", \"" + sec_id + "\", " + str(sec_num) + ")"

	sec_count = Chunk_CountSections("TRACK", chunk)

	if (sec_count > sec_num) :
	
		sec_start = Chunk_GetSectionStart("TRACK", sec_num, chunk)

		if (len(sec_start) > 0) :
			
			out = chunk[:sec_start[1]] + line + "\n" + chunk[sec_start[1]:]		

	return out



def Chunk_RemoveLine(line_id, line_num, chunk) :

	out = "Error: Chunk_RemoveLine(\"" + line_id + "\", " + str(line_num) + ")"

	line_pos = Chunk_GetLine(line_id, line_num, chunk)	

	if (len(line_pos) > 0) :
		
		out = chunk[:line_pos[0]] + chunk[line_pos[1]:]

	return out
		


# set track channel amount - tk = Reaper Track, amount = amount, multi = multitrack metering activated
def Track_SetChannelAmount(tk, ch_amount, make_multi) :

	chunk = Chunk_GetChunk(tk)

	# change channel amount
	chunk = Chunk_ChangeLine("NCHAN", 0, str(ch_amount), chunk)

	# has multi?
	is_multi = True
	if (Chunk_CountLines("VU", chunk)) == 0 :
		is_multi = False

	# change if necessary
	if (is_multi != make_multi) :

		if (is_multi) :
			chunk = Chunk_RemoveLine('VU', 0, chunk)
		else :
			chunk = Chunk_InsertLine('VU 2', "TRACK", 0, chunk)
	
	# write chunk
	Chunk_SafeChunk(tk, chunk)
	


# Remove FX from Track - tk = Reaper Track, name = "" -> all fx
def Track_RemoveFX(tk, fx_name) :

	# delete all
	if (fx_name == "") :

		fx_amount = RPR_TrackFX_GetCount(tk)

		for i in range(0, fx_amount) :
			SNM_MoveOrRemoveTrackFX(tk, fx_amount - i - 1, 0)

	# delete 'fx_name'
	else :
		fx_id = RPR_TrackFX_GetByName(tk, fx_name, False)
	
		# if exists
		if (fx_id >= 0) :
			SNM_MoveOrRemoveTrackFX(tk, fx_id, 0)



# mode: -1 = remove, 0 = replace, 1 = append
def Track_Rename(tk, change, mode) :

	chunk = Chunk_GetChunk(tk)

	# replace
	if (mode == 0) :
		new_name = change

	else:
		old_name = Chunk_GetValue("NAME", 0, chunk).replace("\"", "")
		
		# append
		if (mode == 1) :
			new_name = old_name + change

		# remove
		if (mode == -1) :
			new_name = old_name.replace(change, "")

	new_name = "\"" + new_name + "\""

	chunk = Chunk_ChangeLine("NAME", 0, new_name, chunk)

	Chunk_SafeChunk(tk, chunk)



### MAIN

# find folder in selection
found = False
for i in range(RPR_CountSelectedTracks(0)) :
	
	tk = RPR_GetSelectedTrack(0, i)

	name = ""
	name = RPR_GetSetMediaTrackInfo_String(tk, "P_NAME", name, False)[3]

	isfolder = int(RPR_GetMediaTrackInfo_Value(tk, "I_FOLDERDEPTH"))

	if ((found == False) and (isfolder == 1) and ((" (Split)" in name) or (" (PP)" in name))) :

		folder = tk
		found = True

if (found == True) :

	RPR_Undo_BeginBlock()

	Selection_Save()

	RPR_SetOnlyTrackSelected(folder)

	# mute folder
	RPR_Main_OnCommand(40730, 0) # mute selected tracks

	# first child -> main
	RPR_Main_OnCommand(RPR_NamedCommandLookup("_SWS_SELCHILDREN"), 0)
	main = RPR_GetSelectedTrack(0, 0)

	# move items from main to folder
	RPR_SetOnlyTrackSelected(main)
	RPR_Main_OnCommand(40421, 0) # select all items in track
	RPR_Main_OnCommand(40117, 0) # move items one track up

	# delete children
	RPR_SetOnlyTrackSelected(folder)
	RPR_Main_OnCommand(RPR_NamedCommandLookup("_SWS_SELCHILDREN"), 0)
	RPR_Main_OnCommand(40005, 0) #remove tracks

	# remove Auto Vol receive
	Track_RemoveFX(folder, "(Auto Vol - Split Receive)")
	Track_RemoveFX(folder, "(Auto Vol - Add Receive)")

	# remove suffix from folder name
	Track_Rename(folder, " (Split)", -1)
	Track_Rename(folder, " (PP)", -1)

	# set track channel amount back to 2
	Track_SetChannelAmount(folder, 2, True)

	# select only folder
	RPR_SetOnlyTrackSelected(folder)

	# unmute
	RPR_Main_OnCommand(40731, 0) # unmute selected tracks

	# dismantle folder
	RPR_Main_OnCommand(RPR_NamedCommandLookup("_XENAKIOS_SELTRACKTONOTFOLDER"), 0)

	# restore selection
	Selection_Restore()

RPR_Undo_EndBlock("Unsplit first selected split", 0)