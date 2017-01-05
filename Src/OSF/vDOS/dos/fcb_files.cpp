#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>

#include "vDos.h"
#include "bios.h"
#include "mem.h"
#include "regs.h"
#include "dos_inc.h"
#include "Shlwapi.h"

#define FCB_SUCCESS				0
#define FCB_READ_NODATA			1
#define FCB_READ_PARTIAL		3
#define FCB_ERR_NODATA			1
#define FCB_ERR_EOF				3
#define FCB_ERR_WRITE			1

// FCB drive:DOS 8.3 filename to string, padding spaces from name and extension removed
void FCBNameToStr(char *name)
	{
	int i, j;
	for (i = 9; i > 1; i--)
		if (name[i] != ' ')
			break;
	for (j = 13; j > 9; j--)
		if (name[j] != ' ')
			break;
	for (int k = 10; k <= j; k++)
		name[++i] = name[k];
	name[++i] = 0;
	}

static bool isvalid(const char in)
	{
	return (Bit8u(in) > 0x1F) && (!strchr(":.;,=+ \t/\"[]<>|", in));
	}

#define PARSE_SEP_STOP          0x01
#define PARSE_DFLT_DRIVE        0x02
#define PARSE_BLNK_FNAME        0x04
#define PARSE_BLNK_FEXT         0x08

#define PARSE_RET_NOWILD        0
#define PARSE_RET_WILD          1
#define PARSE_RET_BADDRIVE      0xff

Bit8u FCB_Parsename(Bit16u seg, Bit16u offset, Bit8u parser, char* string, Bit8u* change)
	{
	char* string_begin = string;
	Bit8u ret = 0;
	if (!(parser & PARSE_DFLT_DRIVE))												// Default drive forced, this intentionally invalidates an extended FCB
		Mem_Stosb(SegOff2Ptr(seg, offset), 0);
	DOS_FCB fcb(seg, offset, false);												// Always a non-extended FCB
	// First get the old data from the fcb
#pragma pack (1)
	union {
		struct {
			char drive[2];
			char name[9];
			char ext[4];
		} part;
		char full[DOS_FCBNAME];
	} fcb_name;
#pragma pack()
	fcb.GetName(fcb_name.full);														// Get the old information from the previous fcb
	fcb_name.part.drive[0] -= 'A'-1;
	fcb_name.part.drive[1] = 0;
	fcb_name.part.name[8] = 0;
	fcb_name.part.ext[3] = 0;
	if ((parser & PARSE_SEP_STOP) && *string)										// Ignore leading seperator
		if (strchr(":.;,=+", *string))
			string++;
	while ((*string == ' ') || (*string == '\t'))									// Strip leading spaces
		string++;
	bool hasdrive, hasname, hasext, finished;
	hasdrive = hasname = hasext = finished = false;
	if (string[1] == ':')															// Check for a drive
		{
		fcb_name.part.drive[0] = 0;
		hasdrive = true;
		if (isalpha(string[0]) && Drives[toupper(string[0])-'A'])
			fcb_name.part.drive[0] = (char)(toupper(string[0])-'A'+1);
		else
			ret = 0xff;
		string += 2;
		}
	if (string[0] == '.')															// Special checks for . and ..
		{
		string++;
		if (!string[0])
			{
			hasname = true;
			ret = PARSE_RET_NOWILD;
			strcpy(fcb_name.part.name, ".       ");
			goto savefcb;
			}
		if (string[1] == '.' && !string[1])
			{
			string++;
			hasname = true;
			ret = PARSE_RET_NOWILD;
			strcpy(fcb_name.part.name, "..      ");
			goto savefcb;
			}
		goto checkext;
		}
	hasname = true;																	// Copy the name
	finished = false;
	Bit8u fill = ' ';
	Bitu index = 0;
	while (index < 8)
		{
		if (!finished)
			{
			if (string[0] == '*')
				{
				fill = '?';
				fcb_name.part.name[index] = '?';
				if (!ret)
					ret = 1;
				finished = true;
				}
			else if (string[0] == '?')
				{
				fcb_name.part.name[index] = '?';
				if (!ret)
					ret = 1;
				}
			else if (isvalid(string[0]))
				fcb_name.part.name[index] = (char)(toupper(string[0]));
			else
				{
				finished = true;
				continue;
				}
			string++;
			}
		else
			fcb_name.part.name[index] = fill;
		index++;
		}
	if (!(string[0] == '.'))
		goto savefcb;
	string++;
checkext:
	hasext = true;																	// Copy the extension
	finished = false;
	fill = ' ';
	index = 0;
	while (index < 3)
		{
		if (!finished)
			{
			if (string[0] == '*')
				{
				fill = '?';
				fcb_name.part.ext[index] = '?';
				finished = true;
				}
			else if (string[0] == '?')
				{
				fcb_name.part.ext[index] = '?';
				if (!ret)
					ret = 1;
				}
			else if (isvalid(string[0]))
				fcb_name.part.ext[index] = (char)(toupper(string[0]));
			else
				{
				finished = true;
				continue;
				}
			string++;
			}
		else
			fcb_name.part.ext[index] = fill;
		index++;
		}
savefcb:
	if (!hasdrive & !(parser & PARSE_DFLT_DRIVE))
		fcb_name.part.drive[0] = 0;
	if (!hasname & !(parser & PARSE_BLNK_FNAME))
		strcpy(fcb_name.part.name, "        ");
	if (!hasext & !(parser & PARSE_BLNK_FEXT))
		strcpy(fcb_name.part.ext, "   ");
	fcb.SetName(fcb_name.part.drive[0], fcb_name.part.name, fcb_name.part.ext);
	*change = (Bit8u)(string-string_begin);
	return ret;
	}

static void DTAExtendName(char* const name, char* const filename, char* const ext)
	{
	strcpy(filename, "        ");
	strcpy(ext, "   ");
	char * pos = name;
	int i = 0;
	bool in_ext = false;
	for (char * pos = name; *pos; pos++)
		{
		if (!in_ext)
			if (*pos == '.')
				{
				in_ext = true;
				i = 0;
				}
			else if (i < 8)
				filename[i++] = *pos;
		else if (i < 3)
			ext[i++] = *pos;
		}
	}

// static bool fcbIsExt = false;
bool FCB_CreateFile(Bit16u seg, Bit16u offset)
	{
//	if (!fcbIsExt)
//		{
//		DOS_PSP psp(dos.psp());
//		psp.SetNumFiles(255);
//		}
	DOS_FCB fcb(seg, offset);
	char shortname[DOS_FCBNAME];
	Bit16u handle;
	fcb.GetName(shortname);
	FCBNameToStr(shortname);
	if (!DOS_CreateFile(shortname, DOS_ATTR_ARCHIVE, &handle))
		return false;
	fcb.FileOpen((Bit8u)handle);
	return true;
	}

bool FCB_OpenFile(Bit16u seg, Bit16u offset)
	{
	DOS_FCB fcb(seg, offset);
	char shortname[DOS_FCBNAME];
	Bit16u handle;

	fcb.GetName(shortname);
	FCBNameToStr(shortname);
	Bit8u drive;
	char fullname[DOS_PATHLENGTH];
	if (!DOS_MakeName(shortname, fullname, &drive))									// First check if the name is correct
		return false;
	if (!DOS_OpenFile(shortname, OPEN_READWRITE, &handle))
		return false;
	fcb.FileOpen((Bit8u)handle);
	return true;
	}

bool FCB_CloseFile(Bit16u seg, Bit16u offset)
	{
	DOS_FCB fcb(seg, offset);
	if (!fcb.Valid())
		return false;
	Bit8u fhandle;
	fcb.FileClose(fhandle);															// Sets fhandle
	DOS_CloseFile(fhandle);
	return true;
	}

static void SaveFindResult(DOS_FCB & find_fcb)
	{
	DOS_DTA find_dta(dos.tables.tempdta);
	char name[DOS_NAMELENGTH_ASCII];
	Bit32u size;
	Bit16u date;
	Bit16u time;
	Bit8u attr;
	Bit8u drive;
	char file_name[9];
	char ext[4];
	find_dta.GetResult(name, size, date, time, attr);
	drive = find_fcb.GetDrive()+1;
	
	DTAExtendName(name, file_name, ext);											// Create a correct file and extention
	DOS_FCB fcb(RealSeg(dos.dta()), RealOff(dos.dta()));							// TODO
	fcb.Create(find_fcb.Extended());
	fcb.SetName(drive, file_name, ext);
	fcb.SetAttr(attr);																// Only adds attribute if fcb is extended
	fcb.SetSizeDateTime(size, date, time);
	}

bool FCB_FindFirst(Bit16u seg, Bit16u offset)
	{
	DOS_FCB fcb(seg, offset);
	RealPt old_dta = dos.dta();
	dos.dta(dos.tables.tempdta);
	char name[DOS_FCBNAME];
	fcb.GetName(name);
	FCBNameToStr(name);
	Bit8u attr = DOS_ATTR_ARCHIVE;
	fcb.GetAttr(attr);																// Gets search attributes if extended
	bool ret = DOS_FindFirst(name, attr);
	dos.dta(old_dta);
	if (ret)
		SaveFindResult(fcb);
	return ret;
	}

bool FCB_FindNext(Bit16u seg, Bit16u offset)
	{
	DOS_FCB fcb(seg, offset);
	RealPt old_dta = dos.dta();
	dos.dta(dos.tables.tempdta);
	bool ret = DoFindNext();
	dos.dta(old_dta);
	if (ret)
		SaveFindResult(fcb);
	return ret;
	}

Bit8u FCB_ReadFile(Bit16u seg, Bit16u offset, Bit16u recno)
	{
	DOS_FCB fcb(seg, offset);
	Bit8u fhandle, cur_rec;
	Bit16u cur_block, rec_size;

	fcb.GetSeqData(fhandle, rec_size);
	fcb.GetRecord(cur_block, cur_rec);
	Bit32u pos = ((cur_block*128)+cur_rec)*rec_size;
	if (!DOS_SeekFile(fhandle, &pos, DOS_SEEK_SET))
		return FCB_READ_NODATA; 
	Bit16u toread = rec_size;
	if (!DOS_ReadFile(fhandle, dos_copybuf, &toread) || toread == 0)
		return FCB_READ_NODATA;
	if (toread < rec_size)															// Zero pad copybuffer to rec_size
		memset(dos_copybuf+toread, 0, rec_size-toread);
	Mem_CopyTo(dWord2Ptr(dos.dta())+recno*rec_size, dos_copybuf, rec_size);
	if (++cur_rec > 127)
		{
		cur_block++;
		cur_rec = 0;
		}
	fcb.SetRecord(cur_block, cur_rec);
	if (toread == rec_size)
		return FCB_SUCCESS;
	return FCB_READ_PARTIAL;
	}

Bit8u FCB_WriteFile(Bit16u seg, Bit16u offset, Bit16u recno)
	{
	DOS_FCB fcb(seg, offset);
	Bit8u fhandle, cur_rec;
	Bit16u cur_block, rec_size;

	fcb.GetSeqData(fhandle, rec_size);
	fcb.GetRecord(cur_block, cur_rec);
	Bit32u pos=((cur_block*128)+cur_rec)*rec_size;
	if (!DOS_SeekFile(fhandle, &pos, DOS_SEEK_SET))
		return FCB_ERR_WRITE; 
	Mem_CopyFrom(dWord2Ptr(dos.dta())+recno*rec_size, dos_copybuf, rec_size);
	Bit16u towrite = rec_size;
	if (!DOS_WriteFile(fhandle, dos_copybuf, &towrite))
		return FCB_ERR_WRITE;
	Bit32u size;
	Bit16u date, time;
	fcb.GetSizeDateTime(size, date, time);
	if (pos+towrite > size)
		size = pos+towrite;
	
	date = DOS_PackDate(dos.date.year, dos.date.month, dos.date.day);				// Ttime doesn't keep track of endofday
	Bit32u ticks = Mem_Lodsd(BIOS_TIMER);
	Bit32u seconds = (ticks*10)/182;
	Bit16u hour = (Bit16u)(seconds/3600);
	Bit16u min = (Bit16u)((seconds % 3600)/60);
	Bit16u sec = (Bit16u)(seconds % 60);
	time = DOS_PackTime(hour, min, sec);
	Bit8u temp = RealHandle(fhandle);
	Files[temp]->time = time;
	Files[temp]->date = date;
	fcb.SetSizeDateTime(size, date, time);
	if (++cur_rec > 127)
		{
		cur_block++;
		cur_rec = 0;
		}	
	fcb.SetRecord(cur_block, cur_rec);
	return FCB_SUCCESS;
	}

static Bit8u DOS_FCBIncreaseSize(Bit16u seg, Bit16u offset)
	{
	DOS_FCB fcb(seg,offset);
	Bit8u fhandle, cur_rec;
	Bit16u cur_block, rec_size;
	fcb.GetSeqData(fhandle, rec_size);
	fcb.GetRecord(cur_block, cur_rec);
	Bit32u pos = ((cur_block*128)+cur_rec)*rec_size;
	if (!DOS_SeekFile(fhandle, &pos, DOS_SEEK_SET))
		return FCB_ERR_WRITE; 
	Bit16u towrite = 0;
	if (!DOS_WriteFile(fhandle, dos_copybuf, &towrite))
		return FCB_ERR_WRITE;
	Bit32u size;
	Bit16u date, time;
	fcb.GetSizeDateTime(size, date, time);
	if (pos+towrite > size)
		size = pos+towrite;
	
	date = DOS_PackDate(dos.date.year, dos.date.month, dos.date.day);				// Time doesn't keep track of endofday
	Bit32u ticks = Mem_Lodsd(BIOS_TIMER);
	Bit32u seconds = (ticks*10)/182;
	Bit16u hour = (Bit16u)(seconds/3600);
	Bit16u min = (Bit16u)((seconds % 3600)/60);
	Bit16u sec = (Bit16u)(seconds % 60);
	time = DOS_PackTime(hour, min, sec);
	Bit8u temp = RealHandle(fhandle);
	Files[temp]->time = time;
	Files[temp]->date = date;
	fcb.SetSizeDateTime(size, date, time);
	fcb.SetRecord(cur_block, cur_rec);
	return FCB_SUCCESS;
	}

Bit8u FCB_RandomRead(Bit16u seg, Bit16u offset, Bit16u *numRec, bool restore)
	{
/* if restore is true :random read else random blok read. 
 * random read updates old block and old record to reflect the random data
 * before the read! and the random data is not updated! (user must do this)
 * Random block read updates these fields to reflect the state after the read!
 */
	DOS_FCB fcb(seg, offset);
	Bit32u random;
	Bit16u old_block = 0;
	Bit8u old_rec = 0;
	Bit8u error = 0;
	Bit16u count;

	fcb.GetRandom(random);															// Set the correct record from the random data
	fcb.SetRecord((Bit16u)(random/128), (Bit8u)(random&127));
	if (restore)
		fcb.GetRecord(old_block, old_rec);											// Store this for after the read.
	for (count = 0; count < *numRec; count++)										// Read records
		{
		error = FCB_ReadFile(seg, offset, count);
		if (error != FCB_SUCCESS)
			break;
		}
	if (error == FCB_READ_PARTIAL)
		count++;																	// Partial read counts
	*numRec = count;
	Bit16u new_block;
	Bit8u new_rec;
	fcb.GetRecord(new_block, new_rec);
	if (restore)
		fcb.SetRecord(old_block,old_rec);
	else																			// Update the random record pointer with new position only when restore is false
		fcb.SetRandom(new_block*128+new_rec); 
	return error;
	}

Bit8u FCB_RandomWrite(Bit16u seg, Bit16u offset, Bit16u *numRec, bool restore)
    {
	DOS_FCB fcb(seg,offset);
	Bit32u random;
	Bit16u old_block = 0;
	Bit8u old_rec = 0;
	Bit8u error = 0;
	Bit16u count;

	fcb.GetRandom(random);															// Set the correct record from the random data
	fcb.SetRecord((Bit16u)(random/128), (Bit8u)(random&127));
	if (restore)
		fcb.GetRecord(old_block, old_rec);
	if (*numRec > 0)
		{
		for (count = 0; count < *numRec; count++)									// Write records
			{
			error = FCB_WriteFile(seg, offset, count);								// DOS_FCBwrite return 0 false when true...
			if (error!=FCB_SUCCESS)
				break;
            }
		*numRec = count;
		}
    else
		DOS_FCBIncreaseSize(seg, offset);
	Bit16u new_block;
	Bit8u new_rec;
	fcb.GetRecord(new_block, new_rec);
	if (restore)
		fcb.SetRecord(old_block, old_rec);
	else																			// Update the random record pointer with new position only when restore is false
		fcb.SetRandom(new_block*128+new_rec); 
	return error;
	}

bool FCB_GetFileSize(Bit16u seg, Bit16u offset)
	{
	char shortname[DOS_PATHLENGTH];
	Bit16u entry;
	Bit8u handle;
	Bit16u rec_size;
	DOS_FCB fcb(seg, offset);
	fcb.GetName(shortname);
	FCBNameToStr(shortname);
	if (!DOS_OpenFile(shortname, OPEN_READ, &entry))
		return false;
	handle = RealHandle(entry);
	Bit32u size = 0;
	Files[handle]->Seek(&size, DOS_SEEK_END);
	DOS_CloseFile(entry);
	fcb.GetSeqData(handle, rec_size);
	Bit32u random = (size/rec_size);
	if (size % rec_size)
		random++;
	fcb.SetRandom(random);
	return true;
	}

bool FCB_DeleteFile(Bit16u seg, Bit16u offset)
	{
	/* FCB DELETE honours wildcards. it will return true if one or more
	 * files get deleted. 
	 * To get this: the dta is set to temporary dta in which found files are
	 * stored. This can not be the tempdta as that one is used by fcbfindfirst
	 */
	RealPt old_dta = dos.dta();
	dos.dta(dos.tables.tempdta_fcbdelete);
	DOS_FCB fcb(RealSeg(dos.dta()), RealOff(dos.dta()));
	DOS_FCB fcb1(seg, offset);
	bool return_value = false;
	bool nextfile = FCB_FindFirst(seg, offset);
	while (nextfile)
		{
		char shortname[DOS_FCBNAME] = { 0 };
		fcb1.GetName(shortname);													// Fix to get the correct FCB
		FCBNameToStr(shortname);
		bool res = DOS_DeleteFile(shortname);
		if (!return_value && res)
			return_value = true;													// At least one file deleted
		nextfile = FCB_FindNext(seg, offset);
		}
	dos.dta(old_dta);																// Restore dta
	return return_value;
	}

bool FCB_RenameFile(Bit16u seg, Bit16u offset)
	{
	DOS_FCB fcbold(seg, offset);
	DOS_FCB fcbnew(seg, offset+16);
	if (!fcbold.Valid())
		return false;
	char oldname[DOS_FCBNAME];
	char newname[DOS_FCBNAME];
	fcbold.GetName(oldname);
	FCBNameToStr(oldname);
	fcbnew.GetName(newname);
	FCBNameToStr(newname);
	return DOS_Rename(oldname, newname);											// Rename the file
	}

void FCB_SetRandomRecord(Bit16u seg, Bit16u offset)
	{
	DOS_FCB fcb(seg, offset);
	Bit16u block;
	Bit8u rec;
	fcb.GetRecord(block, rec);
	fcb.SetRandom(block*128+rec);
	}
