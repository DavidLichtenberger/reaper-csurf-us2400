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

	sec_count = Chunk_CountSections(sec_id, chunk)

	if (sec_count > sec_num) :
		sec_start = Chunk_GetSectionStart(sec_id, sec_num, chunk)

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

	fx_id = 999999

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


def Track_MoveSends(idx_from, idx_to) :
	
	for t in range(0, RPR_CountTracks(0)) :

		tk = RPR_GetTrack(0, t)
		chunk = Chunk_GetChunk(tk)

		def change(matchobj) :
			if (matchobj.group(0) == ("\nAUXRECV " + str(idx_from) + " ")) : 
				return ("\nAUXRECV " + str(idx_to) + " ")
			else :
				return str(matchobj.group(0))

		chunk = re.sub(("\nAUXRECV " + str(idx_from) + " "), change, chunk)

		Chunk_SafeChunk(tk, chunk)


def Track_MoveAllExceptInputs(from_tk, to_tk) :

	# find positions of subchunks to insert / replace - 0: from, 1: to
	chunks = [Chunk_GetChunk(from_tk), Chunk_GetChunk(to_tk)]
	
	queries = []
	queries.append(re.compile(r"(^ISBUS)|(^>\Z)", re.MULTILINE)) # 0: folder state start
	queries.append(re.compile(r"(^SHOWINMIX)", re.MULTILINE)) # 1: folder state end
	queries.append(re.compile(r"(^AUXRECV)|(^>\Z)", re.MULTILINE)) # 2: receives start
	queries.append(re.compile(r"(^MIDIOUT)|(^MAINSEND)|(^\<VOLENV)|(^\<FXCHAIN)|(^\<ITEM)|(^>\Z)", re.MULTILINE)) # 3: receives end
	queries.append(re.compile(r"(^\<ITEM)|(^>\Z)", re.MULTILINE)) # 4: items start
	queries.append(re.compile(r"(^>\Z)", re.MULTILINE)) # 5: items end

	pos = []

	for c in range(0, len(chunks)) :

		pos.append([])

		for q in range(0, len(queries)) :

			result = re.search(queries[q], chunks[c])

			if (result == None) :
				pos[c].append(len(chunks[c]) - 1) # '>'
			else :
	 			pos[c].append(result.start(0))

	# compose and safe new chunk from 'from'- and 'to'-chunk
	new_chunk = chunks[0][:pos[0][0]] # everything of the 'from'-chunk until start of folder state
	new_chunk += chunks[1][pos[1][0]:pos[1][1]] # folder state of the 'to'-chunk
	
	new_chunk += chunks[0][pos[0][1]:pos[0][2]] # everything between folder state and receives of 'from'-chunk
	new_chunk += chunks[1][pos[1][2]:pos[1][3]] # receives of the 'to'-chunk
	
	new_chunk += chunks[0][pos[0][3]:pos[0][4]] # everything of the 'from'-chunk between receives and items
	new_chunk += chunks[1][pos[1][4]:pos[1][5]] # items of the 'to'-chunk

	Chunk_SafeChunk(to_tk, new_chunk)


def Track_ResetVolPan(tk) :

	chunk = Chunk_GetChunk(tk)

	chunk = Chunk_ChangeLine("VOLPAN", 0, "1.00000000000000 0.00000000000000 -1.00000000000000 -1.00000000000000 1.00000000000000", chunk)

	Chunk_SafeChunk(tk, chunk)



### MAIN

if (RPR_CountSelectedTracks(0) > 0) :


	# 0. PREPARE

	RPR_Undo_BeginBlock()
	Selection_Save()
	inner = 0

	# 1. WRAP ORIGINAL TRACK ('MAIN')

	# get first selected track -> 'main'
	main = RPR_GetSelectedTrack(0, 0)
	main_idx = int(RPR_GetMediaTrackInfo_Value(main, "IP_TRACKNUMBER")) - 1
	is_muted = bool(RPR_GetMediaTrackInfo_Value(main, "B_MUTE"))
	
	# get name
	main_name = ""
	main_name = RPR_GetSetMediaTrackInfo_String(main, "P_NAME", main_name, False)[3]

	# insert track before -> 'folder'
	RPR_InsertTrackAtIndex(main_idx, True) 
	RPR_TrackList_AdjustWindows(False)
	folder_idx = main_idx
	main_idx += 1

	folder = RPR_GetTrack(0, folder_idx)
	main = RPR_GetTrack(0, main_idx)

	# select 'main' 
	RPR_SetOnlyTrackSelected(main)

	# mute 'main'
	RPR_Main_OnCommand(40730, 0) # mute selected tracks

	# move all sends from 'main' to 'folder'
	Track_MoveSends(main_idx, folder_idx)

	# duplicate track without items and receives
	Track_MoveAllExceptInputs(main, folder)

	# reset main
	Track_ResetVolPan(main)

	# remove fx from 'main'
	RPR_Main_OnCommand(RPR_NamedCommandLookup("_S&M_COPYFXCHAIN6"), 0)
	
	# if 'main' is a folder itself we need a duplicate 'main' to insert the split-tracks after
	if int(RPR_GetMediaTrackInfo_Value(main, "I_FOLDERDEPTH")) == 1:
		
		# insert track before main -> 'dupl'
		RPR_InsertTrackAtIndex(main_idx, True) 
		RPR_TrackList_AdjustWindows(False)
		dupl_idx = main_idx
		main_idx += 1

		main = RPR_GetTrack(0, main_idx)
		dupl = RPR_GetTrack(0, dupl_idx)

		# create send 'main' to 'dupl' - 1/2 > 1/2 post
		Track_CreateSend(main, dupl, 0, 0, True, 0)

		# remove master send from 'main'
		RPR_SetMediaTrackInfo_Value(main, "B_MAINSEND", False)
		RPR_TrackList_AdjustWindows(True)
	
		# rename old main to indicate state
		Track_Rename(main, "<< " + main_name, 0)

		# make the duplicate our new 'main'
		inner = main
		inner_idx = main_idx
		main_idx = dupl_idx
		main = dupl

	# rename tracks
	Track_Rename(folder, main_name + " (Split)", 0)
	Track_Rename(main, "< L", 0)

	# set track channel amount to 10
	Track_SetChannelAmount(folder, 10, False)
	Track_SetChannelAmount(main, 10, False)

	# create sends main to folder - 1/2 > 1/2 post (audio) , 9 > 9 pre, 10 > 10 post (autovol)
	Track_CreateSend(main, folder, 0, 0, True, 0)
	Track_CreateSend(main, folder, 9, 9, False, 0)
	Track_CreateSend(main, folder, 8, 8, False, 3)

	
	# 2. ADD SPLIT TKS & ROUTING

	# insert split tks
	rtk_idx = main_idx + 1
	RPR_InsertTrackAtIndex(rtk_idx, True) 
	RPR_TrackList_AdjustWindows(False)
	rtk = RPR_GetTrack(0, rtk_idx)

	# name split tks
	Track_Rename(rtk, "< R", 0)

	# set track channel amount to 10
	Track_SetChannelAmount(rtk, 10, False)

	# send main -> split tks
	Track_CreateSend(main, rtk, 0, 0, True, 1)

	# create sends split tks to folder - 1/2 > 1/2 post (audio) , 9 > 9 pre, 10 > 10 post (autovol)
	Track_CreateSend(rtk, folder, 0, 0, True, 0)
	Track_CreateSend(rtk, folder, 9, 9, False, 0)
	Track_CreateSend(rtk, folder, 8, 8, False, 3)

	# insert split
	Track_InsertFX(main, "(LR-Split)")

	# insert autovol
	Track_InsertFX(folder, "(Auto Vol - Add Receive)")
	Track_InsertFX(main, "(Auto Vol - Send)")
	Track_InsertFX(rtk, "(Auto Vol - Send)")


	# 3. WRAP IT ALL IN A NEW FOLDER

	# select 'folder', split-tracks, 'main' & 'inner', if exists
	RPR_SetOnlyTrackSelected(folder)
	RPR_SetTrackSelected(rtk, True)
	RPR_SetTrackSelected(main, True)
	if (inner != 0) :
		RPR_SetTrackSelected(inner, True)

	# add existing children of 'main' to sel
	RPR_Main_OnCommand(RPR_NamedCommandLookup("_SWS_SELCHILDREN2"), 0)

	# make 'folder' a real folder, containing split-tracks, 'main' & its children
	RPR_Main_OnCommand(RPR_NamedCommandLookup("_SWS_MAKEFOLDER"), 0)
	

	# 4. CLEAN-UP

	# unmute tracks
	RPR_SetOnlyTrackSelected(main)
	
	if (inner != 0) :
		RPR_SetTrackSelected(inner, True)

	if (is_muted != True) :
		RPR_SetTrackSelected(folder, True)

	RPR_Main_OnCommand(40731, 0) # unmute selected tracks
	
	# restore original sel, but select new split folder instead of main
	Selection_Restore()
	RPR_SetTrackSelected(main, False)
	if (inner != 0) :
		RPR_SetTrackSelected(inner, False)

	RPR_SetTrackSelected(folder, True)

	RPR_Undo_EndBlock('Add Parallel Processor to first selected track', 0)
	