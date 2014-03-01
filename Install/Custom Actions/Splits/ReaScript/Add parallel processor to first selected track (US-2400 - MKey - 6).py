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



# insert fx at beginning of chain, tk = Reaper Track, fx_name = name of plugin
def Track_InsertFX(tk, fx_name) :

	fx_id = 999999;

	while (fx_id > 0) :

		# insert or find fx
		fx_id = RPR_TrackFX_GetByName(tk, fx_name, True)

		# move up if necessary
		if (fx_id > 0) :
			SNM_MoveOrRemoveTrackFX(tk, fx_id, -1)
	


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



# Create a send between tracks, with options for ch, stereo and pre/post fader - 
# src_tk: sending Reaper Track, dest_tk: receiving Reaper Track, 
# src_ch: channels of sending track, dest_ch: receiving channels of receiving track
# stereo: if true, src_ch and dest_ch are made into pairs, e.g. '1' becomes '1/2'
# mode: -1 = user default, 0 = post-fader, 1 = pre-fx, 3 = pre-fader
def Track_CreateSend(src_tk, dest_tk, src_ch, dest_ch, stereo, mode) :

	src_num = int(RPR_GetMediaTrackInfo_Value(src_tk, "IP_TRACKNUMBER")) - 1

	if (stereo == False) :
		src_ch += 1024
		dest_ch += 1024
	
	chunk = Chunk_GetChunk(dest_tk)

	new_line = "AUXRECV "
	new_line = new_line + str(src_num) + " "
	new_line = new_line + str(mode) + " "
	new_line = new_line + "1.00000000000000 0.00000000000000 0 0 0 "
	new_line = new_line + str(src_ch) + " "
	new_line = new_line + str(dest_ch) + " "
	new_line = new_line + "-1.00000000000000 0 -1 ''"

	chunk = Chunk_InsertLine(new_line, "TRACK", 0, chunk)

	Chunk_SafeChunk(dest_tk, chunk)


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

if (RPR_CountSelectedTracks(0) > 0) :

	RPR_Undo_BeginBlock()

	Selection_Save()

	# get first selected track -> 'folder'
	folder = RPR_GetSelectedTrack(0, 0)
	RPR_SetOnlyTrackSelected(folder)

	# mute folder
	RPR_Main_OnCommand(40730, 0) # mute selected tracks

	# set track channel amount to 10
	Track_SetChannelAmount(folder, 10, False)

	# duplicate folder -> 'main'
	RPR_Main_OnCommand(40062, 0)
	main = RPR_GetSelectedTrack(0, 0)

	# remove all items from folder
	RPR_SetOnlyTrackSelected(folder)
	RPR_Main_OnCommand(RPR_NamedCommandLookup("_SWS_DELALLITEMS"), 0)

	# remove all fx from main
	Track_RemoveFX(main, "")

	# remove all routing from main
	RPR_SetOnlyTrackSelected(main)
	RPR_Main_OnCommand(RPR_NamedCommandLookup("_S&M_SENDS7"), 0)

	# remove master send from main
	RPR_SetMediaTrackInfo_Value(main, "B_MAINSEND", False)

	# create sends main to folder - 1/2 > 1/2 post, 9 > 9 pre, 10 > 10 post
	Track_CreateSend(main, folder, 9, 9, False, 0)
	Track_CreateSend(main, folder, 8, 8, False, 3)
	Track_CreateSend(main, folder, 0, 0, True, 0)

	# duplicate main -> add_tks ('pp'-tk)and remove items
	RPR_SetOnlyTrackSelected(main)
	RPR_Main_OnCommand(40062, 0)

	pp_tk = RPR_GetSelectedTrack(0, 0)

	RPR_SetOnlyTrackSelected(pp_tk)
	RPR_Main_OnCommand(RPR_NamedCommandLookup("_SWS_DELALLITEMS"), 0)

	# send main -> add_tks
	Track_CreateSend(main, pp_tk, 0, 0, True, 1)

	# insert autovol 
	Track_InsertFX(folder, "(Auto Vol - Add Receive)")
	Track_InsertFX(main, "(Auto Vol - Send)")
	Track_InsertFX(pp_tk, "(Auto Vol - Send)")

	# rename tracks
	Track_Rename(folder, " (PP)", 1)
	Track_Rename(main, "< Main", 0)
	Track_Rename(pp_tk, "< PP", 0)

	# select all tracks
	RPR_SetOnlyTrackSelected(folder)
	RPR_SetTrackSelected(main, True)
	RPR_SetTrackSelected(pp_tk, True)

	# unmute
	RPR_Main_OnCommand(40731, 0) # unmute selected tracks

	# create folder
	RPR_Main_OnCommand(RPR_NamedCommandLookup("_SWS_MAKEFOLDER"), 0)

	Selection_Restore()

	RPR_Undo_EndBlock("Add parallel processor to first selected track", 0)