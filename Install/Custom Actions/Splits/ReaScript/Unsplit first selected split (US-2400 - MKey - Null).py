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


def Track_Unwrap(tk) :

	tk_idx = int(RPR_GetMediaTrackInfo_Value(tk, "IP_TRACKNUMBER")) - 1
	tk_count = RPR_CountTracks(0)

	depth = 1

	while tk_idx < tk_count :
		tk_idx += 1
		child = RPR_GetTrack(0, tk_idx)
		folderstate = int(RPR_GetMediaTrackInfo_Value(child, "I_FOLDERDEPTH"))
		depth += folderstate

		if (depth == 0):
			RPR_SetMediaTrackInfo_Value(child, "I_FOLDERDEPTH", folderstate + 1)
			break

	chunk = Chunk_GetChunk(tk)
	chunk = Chunk_ChangeLine("ISBUS", 0, "0 0", chunk)
	Chunk_SafeChunk(tk, chunk)


### MAIN

if (RPR_CountSelectedTracks(0) > 0) :

	# 0. PREPARE

	RPR_Undo_BeginBlock()
	Selection_Save()


	# 1. FIND TRACKS

	folder = 0
	main = 0
	main_name = ""

	for f in range(0, len(sel_safe)) :

		# only continue if you haven't found 'folder' and 'main'
		if ((folder == 0) and (main == 0)) : 

			# is folder?
			if int(RPR_GetMediaTrackInfo_Value(sel_safe[f], "I_FOLDERDEPTH")) == 1:
				
				folder = sel_safe[f]

				# go through children of folder
				RPR_SetOnlyTrackSelected(folder)
				RPR_Main_OnCommand(RPR_NamedCommandLookup("_SWS_SELCHILDREN2"), 0)

				for m in range (0, RPR_CountSelectedTracks(0)) :

					# check name
					m_tk = RPR_GetSelectedTrack(0, m)
					name = ""
					name = RPR_GetSetMediaTrackInfo_String(m_tk, "P_NAME", name, False)[3]

					# find either the first track called '< Main' or, 
					# if the original track is a folder, any track called '<< ...'
					if (re.search(r"(\< )", name) != None) and (main == 0) :
						main = m_tk

					if (re.search(r"(\<\< )", name) != None) :
						main = m_tk

	# only continue if folder and main have been found					
	if (main != 0) and (folder != 0) :


		# 2. PREPARE AND MERGE 'MAIN' AND 'FOLDER'

		# positions and states		
		main_idx = int(RPR_GetMediaTrackInfo_Value(main, "IP_TRACKNUMBER")) - 1
		folder_idx = int(RPR_GetMediaTrackInfo_Value(folder, "IP_TRACKNUMBER")) - 1
		before = RPR_GetTrack(0, folder_idx - 1)
		is_muted = bool(RPR_GetMediaTrackInfo_Value(folder, "B_MUTE"))

		# mute 'folder' and 'main'
		RPR_SetOnlyTrackSelected(folder)
		RPR_SetTrackSelected(main, True)

		RPR_Main_OnCommand(40730, 0) # mute selected tracks
	
		# remove Auto Vol from 'folder'
		Track_RemoveFX(folder, "(Auto Vol - Split Receive)")
		Track_RemoveFX(folder, "(Auto Vol - Add Receive)")

		# set 'folder' track channel amount back to 2
		Track_SetChannelAmount(folder, 2, True)

		# move 'folder' settings to 'main' but keep items and receives
		Track_MoveAllExceptInputs(folder, main)

		# remove suffix from 'main' name
		Track_Rename(main, " (PP)", -1)
		Track_Rename(main, " (Split)", -1)

		# move sends from 'folder' to 'main'
		Track_MoveSends(folder_idx, main_idx)

		
		# 3. UNWRAP AND REMOVE SPLIT

		# find children of main
		RPR_SetOnlyTrackSelected(main)
		RPR_Main_OnCommand(RPR_NamedCommandLookup("_SWS_SELCHILDREN"), 0) # select only children
		main_children = []
		for t in range(0, RPR_CountSelectedTracks(0)) :
			main_children.append(RPR_GetSelectedTrack(0, t))

		RPR_SetOnlyTrackSelected(folder)
		RPR_Main_OnCommand(RPR_NamedCommandLookup("_SWS_SELCHILDREN2"), 0) # select folder & children
		
		# deselect all that stays
		RPR_SetTrackSelected(main, False)
		for child in main_children :
			RPR_SetTrackSelected(child, False)

		Track_Unwrap(folder)

		RPR_Main_OnCommand(40005, 0) # remove tracks
		
		orig = RPR_GetTrack(0, folder_idx)

		# 4. CLEAN-UP

		# unmute 'orig'
		if (is_muted != True) :
			RPR_SetOnlyTrackSelected(orig)
			RPR_Main_OnCommand(40731, 0) # unmute selected tracks

		# restoring selection on success is pointless - some are gone, so reaper won't find anything.
		# we'll just leave the 'orig' selected

	else :

		Selection_Restore()	
	
	RPR_Undo_EndBlock('Un-split first selected split', 0)