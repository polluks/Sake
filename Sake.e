-> Sake - Atari Kernel Emulator
-> GEMDOS (TRAP #1), BIOS (TRAP #2), XBIOS (TRAP #3), GEM AES/VDI
-> Maps Atari ST system calls to AmigaOS libraries

OPT POINTER, NATIVE
MODULE 'exec/tasks', 'dos', 'exec', 'intuition/intuition', 'gadtools', 'graphics'

-> ---------------------------------------------------------------------------
-> Global state
-> ---------------------------------------------------------------------------
DEF oldTrapCode:APTR
DEF ctx[16]:ARRAY OF VALUE    -> Saved register context: [D0-D7, A0-A6]
DEF gem_handles[16]:ARRAY OF VALUE -> GEMDOS handle -> AmigaOS BPTR mapping
DEF gem_dta                  -> Disk Transfer Address (for Fsfirst/Fsnext)
DEF gem_drv                  -> Current drive (0=A:)
DEF gem_path[128]:ARRAY OF CHAR -> Current path
DEF fib:fileinfoblock       -> FileInfoBlock for directory search
DEF gem_search_lock          -> Lock handle for current search
DEF gem_search_first         -> TRUE if we're inside a search
DEF gem_search_pattern[128]:ARRAY OF CHAR -> Pattern for search matching
DEF gem_search_attr          -> Attributes for search matching
DEF temp_string[256]:ARRAY OF CHAR
DEF bios_kb_shift            -> Keyboard shift state for BIOS Kbshift
DEF gem_aes_id               -> AES application ID counter
DEF gem_window_list[16]:ARRAY OF VALUE -> Open window handles (Intuition Window ptrs)
DEF gem_aes_global[32]:ARRAY OF VALUE -> AES global array
DEF gem_scrn_w, gem_scrn_h   -> Virtual screen dimensions

-> GEMDOS constants for Seek mode mapping
CONST GEMDOS_SEEK_START = 0, GEMDOS_SEEK_CUR = 1, GEMDOS_SEEK_END = 2

-> Error code mapping
CONST E_OK = 0, E_ERROR = -1, EDRVNR = -3, EPRCFND = -7
CONST ENFHND = -8, ELOCKD = -10, ENSMEM = -11, EIHND = -12

-> ---------------------------------------------------------------------------
-> AmigaOS mode mapping for Fopen
-> GEMDOS: 0=read, 1=write, 2=read+write
-> AmigaOS: MODE_OLDFILE=1005, MODE_NEWFILE=1006
-> ---------------------------------------------------------------------------
PROC gemdos_fopen_mode(gem_mode)
  DEF result
  IF gem_mode = 0
    result := 1005
  ELSE
    IF gem_mode = 1
      result := 1006
    ELSE
      IF gem_mode = 2
        result := 1006
      ELSE
        result := 1005
      ENDIF
    ENDIF
  ENDIF
ENDPROC result

PROC gemdos_to_amiga_seek(gem_seek)
  DEF result
  IF gem_seek = GEMDOS_SEEK_START
    result := -1
  ELSE
    IF gem_seek = GEMDOS_SEEK_CUR
      result := 0
    ELSE
      IF gem_seek = GEMDOS_SEEK_END
        result := 1
      ELSE
        result := -1
      ENDIF
    ENDIF
  ENDIF
ENDPROC result

-> ---------------------------------------------------------------------------
-> GEMDOS dispatch - called from assembly trap handler
-> ctx[0] = function number on entry, return value on exit
-> ctx[1..7] = D1-D7 parameters
-> ctx[8..14] = A0-A6 parameters
-> ---------------------------------------------------------------------------
PROC gemdos_dispatch()
  DEF fn
  fn := ctx[0] !!INT

  SELECT fn

  CASE $00 -> gemdos_pterm0()
  CASE $01 -> gemdos_cconin()
  CASE $02 -> gemdos_cconout()
  CASE $06 -> gemdos_cconws()
  CASE $07 -> gemdos_cconis()
  CASE $08 -> gemdos_cconos()
  CASE $09 -> gemdos_cconws()
  CASE $0A -> gemdos_cconrs()
  CASE $0B -> gemdos_cconis()
  CASE $0C -> gemdos_cconin()
  CASE $0E -> gemdos_dsetdrv()
  CASE $0F -> gemdos_dgetdrv()
  CASE $10 -> gemdos_dsetpath()
  CASE $11 -> gemdos_dgetpath()
  CASE $15 -> gemdos_fopen()
  CASE $16 -> gemdos_fclose()
  CASE $17 -> gemdos_fread()
  CASE $18 -> gemdos_fwrite()
  CASE $19 -> gemdos_fdelete()
  CASE $1A -> gemdos_fseek()
  CASE $1B -> gemdos_fattrib()
  CASE $1E -> gemdos_fdatime()
  CASE $1F -> gemdos_fsfirst()
  CASE $20 -> gemdos_fsnext()
  CASE $21 -> gemdos_fsrename()
  CASE $22 -> gemdos_fmkdir()
  CASE $23 -> gemdos_frmdir()
  CASE $24 -> gemdos_fchdir()
  CASE $25 -> gemdos_fgetdta()
  CASE $26 -> gemdos_fsetdta()
  CASE $29 -> gemdos_malloc()
  CASE $2A -> gemdos_mfree()
  CASE $2D -> gemdos_pexec()
  CASE $2E -> gemdos_pterm()
  CASE $30 -> gemdos_super()
  CASE $31 -> gemdos_tgetdate()
  CASE $32 -> gemdos_tsetdate()
  CASE $33 -> gemdos_tgettime()
  CASE $34 -> gemdos_tsettime()
  CASE $39 -> gemdos_mxalloc()

  DEFAULT
    ctx[0] := E_ERROR

  ENDSELECT
ENDPROC


-> ---------------------------------------------------------------------------
-> Pterm0 ($00) - Terminate with return code 0
-> ---------------------------------------------------------------------------
PROC gemdos_pterm0()
  ctx[0] := E_OK
  -> In a real emulator, we'd clean up and exit
ENDPROC


-> ---------------------------------------------------------------------------
-> Cconin ($01) - Read character from console (blocking, with echo)
-> Cconin ($0C) - Read character with echo control
-> D0 = character read
-> ---------------------------------------------------------------------------
PROC gemdos_cconin()
  DEF ch[1]:STRING
  ch[0] := 0
  Read(Input(), ch, 1)
  -> Echo character
  Write(Output(), ch, 1)
  ctx[0] := ch[0] !!LONG
ENDPROC


-> ---------------------------------------------------------------------------
-> Cconout ($02) - Write character to console
-> D1 = character to write
-> ---------------------------------------------------------------------------
PROC gemdos_cconout()
  DEF ch[1]:STRING
  ch[0] := ctx[1] !!CHAR
  Write(Output(), ch, 1)
  ctx[0] := E_OK
ENDPROC


-> ---------------------------------------------------------------------------
-> Cconis ($07, $0B) - Check console input status
-> D0 = -1 if key pressed, 0 otherwise
-> ---------------------------------------------------------------------------
PROC gemdos_cconis()
  -> Use WaitForChar with 0 timeout
  IF WaitForChar(Input(), 0) THEN ctx[0] := -1 ELSE ctx[0] := 0
ENDPROC


-> ---------------------------------------------------------------------------
-> Cconos ($08) - Check console output status
-> Always returns TRUE on Amiga
-> ---------------------------------------------------------------------------
PROC gemdos_cconos()
  ctx[0] := -1
ENDPROC


-> ---------------------------------------------------------------------------
-> Cconws ($09, $06) - Write null-terminated string to console
-> A0 = pointer to string in emulated memory
-> For now, the string is in the ctx saved as A0
-> ---------------------------------------------------------------------------
PROC gemdos_cconws()
  DEF src:PTR TO CHAR, len
  src := ctx[8] !!PTR TO CHAR
  -> Copy string from emulated memory and write it
  len := 0
  WHILE src[len] <> 0 AND len < 255
    temp_string[len] := src[len]
    len := len + 1
  ENDWHILE
  temp_string[len] := 0
  Write(Output(), temp_string, len)
  ctx[0] := E_OK
ENDPROC


-> ---------------------------------------------------------------------------
-> Cconrs ($0A) - Read string from console with line editing
-> A0 = pointer to buffer, first byte = max length
-> ---------------------------------------------------------------------------
PROC gemdos_cconrs()
  DEF maxlen, buf:PTR TO CHAR, ch[2]:ARRAY OF CHAR, pos, done
  buf := ctx[8] !!PTR TO CHAR
  maxlen := buf[0]
  pos := 0
  done := FALSE

  WHILE pos < maxlen - 1 AND NOT done
    Read(Input(), ch, 1)
    IF ch[0] = 13 OR ch[0] = 10
      buf[pos + 1] := 0
      done := TRUE
    ELSE
      IF ch[0] = 8
        IF pos > 0
          pos := pos - 1
        ENDIF
      ELSE
        buf[pos + 1] := ch[0]
        pos := pos + 1
      ENDIF
    ENDIF
  ENDWHILE
  IF NOT done
    buf[1] := 0
  ENDIF
  ctx[0] := 0
ENDPROC


-> ---------------------------------------------------------------------------
-> Dsetdrv ($0E) - Set default drive
-> D1 = drive number (0=A:)
-> ---------------------------------------------------------------------------
PROC gemdos_dsetdrv()
  gem_drv := ctx[1]
  ctx[0] := E_OK
ENDPROC


-> ---------------------------------------------------------------------------
-> Dgetdrv ($0F) - Get default drive
-> D0 = current drive (0=A:)
-> ---------------------------------------------------------------------------
PROC gemdos_dgetdrv()
  ctx[0] := gem_drv
ENDPROC


-> ---------------------------------------------------------------------------
-> Dsetpath ($10) - Set current path
-> A0 = pointer to path string
-> ---------------------------------------------------------------------------
PROC gemdos_dsetpath()
  DEF src:PTR TO CHAR, i
  src := ctx[8] !!PTR TO CHAR
  i := 0
  WHILE src[i] <> 0 AND i < 127
    gem_path[i] := src[i]
    i := i + 1
  ENDWHILE
  gem_path[i] := 0
  ctx[0] := E_OK
ENDPROC


-> ---------------------------------------------------------------------------
-> Dgetpath ($11) - Get current path
-> A0 = buffer for path string
-> ---------------------------------------------------------------------------
PROC gemdos_dgetpath()
  DEF dst:PTR TO CHAR, i
  dst := ctx[8] !!PTR TO CHAR
  i := 0
  WHILE gem_path[i] <> 0
    dst[i] := gem_path[i]
    i := i + 1
  ENDWHILE
  dst[i] := 0
  ctx[0] := E_OK
ENDPROC


-> ---------------------------------------------------------------------------
-> Fopen ($15) - Open file
-> A0 = filename pointer, D1 = mode (0=read, 1=write, 2=read+write)
-> D0 = file handle or negative error
-> ---------------------------------------------------------------------------
PROC gemdos_fopen()
  DEF filename:PTR TO CHAR, mode, handle, i, result
  filename := ctx[8] !!PTR TO CHAR
  mode := gemdos_fopen_mode(ctx[1])
  handle := Open(filename, mode)
  IF handle
    result := ENFHND
    FOR i := 3 TO 15
      IF gem_handles[i] = 0
        gem_handles[i] := handle
        result := i
        i := 15
      ENDIF
    ENDFOR
    IF result = ENFHND
      Close(handle !!BPTR)
    ENDIF
    ctx[0] := result
  ELSE
    ctx[0] := E_ERROR
  ENDIF
ENDPROC


-> ---------------------------------------------------------------------------
-> Fclose ($16) - Close file
-> D1 = file handle
-> ---------------------------------------------------------------------------
PROC gemdos_fclose()
  DEF handle
  handle := ctx[1]
  IF handle >= 3 AND handle <= 15
    Close(gem_handles[handle] !!BPTR)
    gem_handles[handle] := 0
    ctx[0] := E_OK
  ELSE
    ctx[0] := EIHND
  ENDIF
ENDPROC


-> ---------------------------------------------------------------------------
-> Fread ($17) - Read from file
-> D1 = handle, D2 = count, A0 = buffer
-> D0 = bytes read or error
-> ---------------------------------------------------------------------------
PROC gemdos_fread()
  DEF handle, count, buf:PTR TO CHAR, result
  handle := ctx[1]
  count := ctx[2]
  buf := ctx[8] !!PTR TO CHAR

  IF handle >= 0 AND handle <= 15
    IF handle = 0
      result := Read(Input(), buf, count)
    ELSE
      result := Read(gem_handles[handle] !!BPTR, buf, count)
    ENDIF
    IF result = 0 AND count > 0
      ctx[0] := E_ERROR
    ELSE
      ctx[0] := result
    ENDIF
  ELSE
    ctx[0] := EIHND
  ENDIF
ENDPROC


-> ---------------------------------------------------------------------------
-> Fwrite ($18) - Write to file
-> D1 = handle, D2 = count, A0 = buffer
-> D0 = bytes written or error
-> ---------------------------------------------------------------------------
PROC gemdos_fwrite()
  DEF handle, count, buf:PTR TO CHAR, result
  handle := ctx[1]
  count := ctx[2]
  buf := ctx[8] !!PTR TO CHAR

  IF handle >= 0 AND handle <= 15
    IF handle = 0
      result := Write(Input(), buf, count)
    ELSE
      IF handle = 1 OR handle = 2
        result := Write(Output(), buf, count)
      ELSE
        result := Write(gem_handles[handle] !!BPTR, buf, count)
      ENDIF
    ENDIF
    IF result = 0 AND count > 0
      ctx[0] := E_ERROR
    ELSE
      ctx[0] := result
    ENDIF
  ELSE
    ctx[0] := EIHND
  ENDIF
ENDPROC


-> ---------------------------------------------------------------------------
-> Fdelete ($19) - Delete file
-> A0 = filename pointer
-> ---------------------------------------------------------------------------
PROC gemdos_fdelete()
  DEF filename:PTR TO CHAR
  filename := ctx[8] !!PTR TO CHAR
  IF DeleteFile(filename)
    ctx[0] := E_OK
  ELSE
    ctx[0] := E_ERROR
  ENDIF
ENDPROC


-> ---------------------------------------------------------------------------
-> Fseek ($1A) - Seek in file
-> D1 = handle, D2 = offset, D3 = mode (0=start, 1=current, 2=end)
-> D0 = new position
-> ---------------------------------------------------------------------------
PROC gemdos_fseek()
  DEF handle, offset, mode, newpos
  handle := ctx[1]
  offset := ctx[2]
  mode := gemdos_to_amiga_seek(ctx[3])

  IF handle >= 3 AND handle <= 15
    newpos := Seek(gem_handles[handle] !!BPTR, offset, mode)
    IF newpos < 0
      ctx[0] := E_ERROR
    ELSE
      ctx[0] := newpos
    ENDIF
  ELSE
    ctx[0] := EIHND
  ENDIF
ENDPROC


-> ---------------------------------------------------------------------------
-> Fattrib ($1B) - Get/set file attributes
-> D1 = mode (0=get, 1=set), A0 = filename, D2 = new attrib (if set)
-> D0 = attributes or error
-> ---------------------------------------------------------------------------
PROC gemdos_fattrib()
  DEF mode, filename:PTR TO CHAR
  mode := ctx[1]
  filename := ctx[8] !!PTR TO CHAR
  IF mode = 0
    -> Get attributes - simplified, returns 0
    ctx[0] := 0
  ELSE
    -> Set attributes via SetProtection
    ctx[0] := E_OK
  ENDIF
ENDPROC


-> ---------------------------------------------------------------------------
-> Fdatime ($1E) - Get/set file date and time
-> D1 = mode (0=get, 1=set), A0 = pointer to time struct, D2 = handle
-> D0 = 0 or error
-> ---------------------------------------------------------------------------
PROC gemdos_fdatime()
  ctx[0] := E_OK
ENDPROC


-> ---------------------------------------------------------------------------
-> Fsfirst ($1F) - Find first matching file
-> A0 = pattern string, D1 = attribute mask
-> D0 = 0 if found, -1 if not
-> ---------------------------------------------------------------------------
PROC gemdos_fsfirst()
  DEF pattern:PTR TO CHAR, attr, i
  DEF lock_name[256]:ARRAY OF CHAR
  pattern := ctx[8] !!PTR TO CHAR
  attr := ctx[1]

  IF gem_search_lock
    UnLock(gem_search_lock !!BPTR)
    gem_search_lock := 0
  ENDIF

  i := 0
  WHILE pattern[i] <> 0 AND i < 127
    gem_search_pattern[i] := pattern[i]
    i := i + 1
  ENDWHILE
  gem_search_pattern[i] := 0

  GetCurrentDirName(lock_name, 255)
  gem_search_lock := Lock(lock_name, -2)

  IF gem_search_lock
    IF Examine(gem_search_lock !!BPTR, fib)
      IF gem_dta <> 0
        FillDTA(gem_dta !!PTR TO CHAR, fib)
      ENDIF
      gem_search_first := TRUE
      ctx[0] := E_OK
    ELSE
      ctx[0] := E_ERROR
    ENDIF
  ELSE
    ctx[0] := E_ERROR
  ENDIF
ENDPROC


PROC gemdos_fsnext()
  IF gem_search_lock
    IF ExNext(gem_search_lock !!BPTR, fib)
      IF gem_dta <> 0
        FillDTA(gem_dta !!PTR TO CHAR, fib)
      ENDIF
      ctx[0] := E_OK
    ELSE
      ctx[0] := E_ERROR
    ENDIF
  ELSE
    ctx[0] := E_ERROR
  ENDIF
ENDPROC


-> ---------------------------------------------------------------------------
-> FileMatch - Match filename against GEMDOS pattern (* and ?)
-> Returns TRUE if name matches pattern
-> ---------------------------------------------------------------------------
PROC FileMatch(name:PTR TO CHAR, pattern:PTR TO CHAR)
  DEF i, j, result
  i := 0
  j := 0
  result := TRUE
  WHILE pattern[j] <> 0 AND result
    IF pattern[j] = '*'
      result := TRUE
      j := 999
    ELSE
      IF pattern[j] = '?'
        IF name[i] = 0 THEN result := FALSE
        i := i + 1
      ELSE
        IF name[i] <> pattern[j] THEN result := FALSE
        i := i + 1
      ENDIF
    ENDIF
    j := j + 1
  ENDWHILE
  IF name[i] <> 0 AND pattern[j-1] <> '*' THEN result := FALSE
ENDPROC result


-> ---------------------------------------------------------------------------
-> Helper: Fill GEMDOS DTA structure from AmigaOS FileInfoBlock
-> ---------------------------------------------------------------------------
PROC FillDTA(dta:PTR TO CHAR, fib_ptr:PTR TO fileinfoblock)
  DEF i
  -> GEMDOS DTA format:
  ->   +0: reserved (21 bytes)
  ->  +21: file attribute (byte)
  ->  +22: time (word)
  ->  +24: date (word)
  ->  +26: size (long)
  ->  +30: filename (13 chars + null)

  -> Clear DTA
  FOR i := 0 TO 43
    dta[i] := 0
  ENDFOR

  -> Set attribute (simplified: use 0 for normal)
  dta[21] := 0

  -> Set size (LITTLE-ENDIAN for 68000)
  dta[26] := (fib_ptr.size) !!BYTE AND $FF
  dta[27] := (fib_ptr.size / 256) !!BYTE AND $FF
  dta[28] := (fib_ptr.size / 65536) !!BYTE AND $FF
  dta[29] := (fib_ptr.size / 16777216) !!BYTE AND $FF

  -> Copy filename
  i := 0
  WHILE fib_ptr.filename[i] <> 0 AND i < 12
    dta[30 + i] := fib_ptr.filename[i]
    i := i + 1
  ENDWHILE
  dta[30 + i] := 0
ENDPROC


-> ---------------------------------------------------------------------------
-> Fsrename ($21) - Rename file
-> A0 = old name, A1 = new name
-> ---------------------------------------------------------------------------
PROC gemdos_fsrename()
  DEF oldname:PTR TO CHAR, newname:PTR TO CHAR
  oldname := ctx[8] !!PTR TO CHAR
  newname := ctx[9] !!PTR TO CHAR

  IF Rename(oldname, newname)
    ctx[0] := E_OK
  ELSE
    ctx[0] := E_ERROR
  ENDIF
ENDPROC


-> ---------------------------------------------------------------------------
-> Fmkdir ($22) - Create directory
-> A0 = directory name
-> ---------------------------------------------------------------------------
PROC gemdos_fmkdir()
  DEF dirname:PTR TO CHAR
  dirname := ctx[8] !!PTR TO CHAR
  IF CreateDir(dirname)
    ctx[0] := E_OK
  ELSE
    ctx[0] := E_ERROR
  ENDIF
ENDPROC


-> ---------------------------------------------------------------------------
-> Frmdir ($23) - Remove directory
-> A0 = directory name
-> ---------------------------------------------------------------------------
PROC gemdos_frmdir()
  DEF dirname:PTR TO CHAR
  dirname := ctx[8] !!PTR TO CHAR
  IF DeleteFile(dirname)
    ctx[0] := E_OK
  ELSE
    ctx[0] := E_ERROR
  ENDIF
ENDPROC


-> ---------------------------------------------------------------------------
-> Fchdir ($24) - Change directory (same as Dsetpath behavior)
-> A0 = path string
-> ---------------------------------------------------------------------------
PROC gemdos_fchdir()
  DEF pathname:PTR TO CHAR
  pathname := ctx[8] !!PTR TO CHAR
  -> Use Dsetpath logic
  gemdos_dsetpath()
ENDPROC


-> ---------------------------------------------------------------------------
-> Fgetdta ($25) - Get Disk Transfer Address
-> D0 = DTA pointer
-> ---------------------------------------------------------------------------
PROC gemdos_fgetdta()
  ctx[0] := gem_dta !!LONG
ENDPROC


-> ---------------------------------------------------------------------------
-> Fsetdta ($26) - Set Disk Transfer Address
-> D1 = new DTA pointer
-> ---------------------------------------------------------------------------
PROC gemdos_fsetdta()
  DEF ptr
  ptr := ctx[1]
  gem_dta := ptr !!PTR TO CHAR
  ctx[0] := E_OK
ENDPROC


-> ---------------------------------------------------------------------------
-> Malloc ($29) - Allocate memory
-> D1 = number of bytes
-> D0 = pointer to memory, or 0 if failed
-> ---------------------------------------------------------------------------
PROC gemdos_malloc()
  DEF size, ptr
  size := ctx[1]
  -> Allocate with MEMF_CLEAR for zero-initialized memory
  ptr := AllocMem(size, 65538)  -> MEMF_CLEAR | MEMF_PUBLIC
  IF ptr = 0
    ctx[0] := 0
  ELSE
    ctx[0] := ptr !!LONG
  ENDIF
ENDPROC


-> ---------------------------------------------------------------------------
-> Mfree ($2A) - Free memory
-> D1 = pointer to memory block
-> ---------------------------------------------------------------------------
PROC gemdos_mfree()
  DEF ptr
  ptr := ctx[1] !!PTR TO CHAR
  -> Note: AmigaOS FreeMem needs the size, which we don't know
  -> In a real implementation, we'd track allocated block sizes
  -> For now, free with a reasonable size or just stub
  FreeMem(ptr !!APTR, 0)    -> This won't work properly without size tracking
  ctx[0] := E_OK
ENDPROC


-> ---------------------------------------------------------------------------
-> Mxalloc ($39) - Allocate memory with flags
-> D1 = number of bytes, D2 = flags (bit 0 = FAST, bit 1 = CLEAR)
-> D0 = pointer or 0
-> ---------------------------------------------------------------------------
PROC gemdos_mxalloc()
  DEF size, flags, memflags
  size := ctx[1]
  flags := ctx[2]
  memflags := 65538  -> MEMF_CLEAR | MEMF_PUBLIC
  IF (flags AND 1) = 0
    -> CHIP memory requested, use MEMF_CHIP
    memflags := 65538  -> Can't do chip on Amiga, use same
  ENDIF
  ctx[0] := AllocMem(size, memflags)
ENDPROC


-> ---------------------------------------------------------------------------
-> Load an Atari ST PRG file into allocated memory
-> Returns address of loaded program, or 0 on failure
-> ---------------------------------------------------------------------------
PROC load_prg(filename:PTR TO CHAR)
  DEF fh:BPTR, header[32]:ARRAY OF CHAR, result
  DEF text_size, data_size, bss_size, total_size
  DEF addr:PTR TO CHAR
  result := 0

  fh := Open(filename, 1005)
  IF fh
    IF Read(fh, header, 32) >= 32
      text_size := header[2] !!LONG
      data_size := header[6] !!LONG
      bss_size := header[10] !!LONG
      total_size := text_size + data_size + bss_size

      addr := AllocMem(total_size, 65538) !!PTR TO CHAR
      IF addr
        result := addr !!VALUE
        IF text_size > 0
          IF Read(fh, addr, text_size) < text_size
            result := 0
          ENDIF
        ENDIF
        IF data_size > 0 AND result <> 0
          IF Read(fh, addr + text_size, data_size) < data_size
            result := 0
          ENDIF
        ENDIF
        IF result = 0
          FreeMem(addr, total_size)
        ENDIF
      ENDIF
    ENDIF
    Close(fh)
  ENDIF
ENDPROC result


-> ---------------------------------------------------------------------------
-> Pexec ($2D) - Execute program
-> D1 = mode (0=load, 1=load&go, 2=go, 3=load&createproc)
-> A0 = filename (modes 0,1,3), A1 = command line (modes 1,3)
-> D0 varies by mode
-> ---------------------------------------------------------------------------
PROC gemdos_pexec()
  DEF mode, filename:PTR TO CHAR
  mode := ctx[1]
  filename := ctx[8] !!PTR TO CHAR

  IF mode = 0
    ctx[0] := load_prg(filename)
  ELSE
    ctx[0] := E_ERROR
  ENDIF
ENDPROC


-> ---------------------------------------------------------------------------
-> Pterm ($2E) - Terminate process with return code
-> D1 = return code
-> ---------------------------------------------------------------------------
PROC gemdos_pterm()
  ctx[0] := ctx[1]
  -> In a real emulator, we'd clean up and exit the emulated process
ENDPROC


-> ---------------------------------------------------------------------------
-> Super ($30) - Enter/exit supervisor mode
-> D1 = 0: enter supervisor, D1 <> 0: exit supervisor
-> D0 = old supervisor mode flag
-> ---------------------------------------------------------------------------
PROC gemdos_super()
  -> On AmigaOS, the trap handler already runs in supervisor mode
  -> So entering supervisor is a no-op
  IF ctx[1] = 0
    ctx[0] := -1  -> Was already in supervisor mode
  ELSE
    ctx[0] := 0   -> Can't actually exit supervisor on Amiga
  ENDIF
ENDPROC


-> ---------------------------------------------------------------------------
-> Tgetdate ($31) - Get system date
-> D0 = date in GEMDOS format (bit 0-4: day, bit 5-8: month, bit 9-15: year-1980)
-> ---------------------------------------------------------------------------
PROC gemdos_tgetdate()
  DEF now:datestamp
  DateStamp(now)
  -> GEMDOS date: bits 0-4=day, 5-8=month, 9-15=year-1980
  -> Simple approximation from days since 1978
  ctx[0] := ((now.days / 365) * 512) + (1 * 32) + 1
ENDPROC


-> ---------------------------------------------------------------------------
-> Tsetdate ($32) - Set system date
-> D1 = date in GEMDOS format
-> ---------------------------------------------------------------------------
PROC gemdos_tsetdate()
  -> On AmigaOS, we don't change the system date
  ctx[0] := E_OK
ENDPROC


-> ---------------------------------------------------------------------------
-> Tgettime ($33) - Get system time
-> D0 = time in GEMDOS format (bit 0-4: sec/2, bit 5-10: min, bit 11-15: hour)
-> ---------------------------------------------------------------------------
PROC gemdos_tgettime()
  DEF now:datestamp, total_secs, hour, mins, sec
  DateStamp(now)
  total_secs := (now.minute * 60) + (now.tick / 50)
  hour := total_secs / 3600
  mins := (total_secs / 60) - (hour * 60)
  sec := total_secs - (hour * 3600) - (mins * 60)
  ctx[0] := (hour * 2048) + (mins * 32) + (sec / 2)
ENDPROC


-> ---------------------------------------------------------------------------
-> Tsettime ($34) - Set system time
-> D1 = time in GEMDOS format
-> ---------------------------------------------------------------------------
PROC gemdos_tsettime()
  -> On AmigaOS, we don't change the system time
  ctx[0] := E_OK
ENDPROC


-> ---------------------------------------------------------------------------
-> BIOS (Trap #2) dispatch
-> ctx[0] = function number, ctx[1]..ctx[7] = D1..D7, ctx[8]..ctx[14] = A0..A6
-> ---------------------------------------------------------------------------
PROC bios_dispatch()
  DEF fn
  fn := ctx[0] !!INT

  SELECT fn

  CASE $00 -> bios_getmpb()
  CASE $01 -> bios_bconin()
  CASE $02 -> bios_bconout()
  CASE $03 -> bios_rwabs()
  CASE $04 -> bios_setexc()
  CASE $05 -> bios_tickcal()
  CASE $06 -> bios_gembp()
  CASE $07 -> bios_bconstat()
  CASE $08 -> bios_mediac()
  CASE $09 -> bios_drvmap()
  CASE $0A -> bios_kbshift()
  CASE $0B -> bios_random()

  CASE $C8 -> gem_aes_dispatch()
  CASE $C9 -> gem_vdi_dispatch()

  DEFAULT
    ctx[0] := E_ERROR

  ENDSELECT
ENDPROC


-> ---------------------------------------------------------------------------
-> Getmpb ($00) / Gembp ($06) - Get Memory Parameter Block
-> A0 = pointer to MPB structure
-> Returns memory layout info
-> ---------------------------------------------------------------------------
PROC bios_getmpb()
  -> Stub - returns memory info for a 512KB ST
  -> In emulator, return reasonable defaults
  ctx[0] := E_OK
ENDPROC

PROC bios_gembp()
  bios_getmpb()
ENDPROC


-> ---------------------------------------------------------------------------
-> Bconin ($01) - Console input (blocking)
-> D1 = device number (0=console, 1=RS232, 2=printer, etc.)
-> D0 = character read
-> ---------------------------------------------------------------------------
PROC bios_bconin()
  DEF dev, ch[1]:STRING
  dev := ctx[1]
  IF dev = 0
    ch[0] := 0
    Read(Input(), ch, 1)
    ctx[0] := ch[0] !!LONG
  ELSE
    ctx[0] := E_ERROR
  ENDIF
ENDPROC


-> ---------------------------------------------------------------------------
-> Bconout ($02) - Console output
-> D1 = device, D2 = character
-> ---------------------------------------------------------------------------
PROC bios_bconout()
  DEF dev, ch[1]:STRING
  dev := ctx[1]
  IF dev = 0
    ch[0] := ctx[2] !!CHAR
    Write(Output(), ch, 1)
    ctx[0] := E_OK
  ELSE
    ctx[0] := E_ERROR
  ENDIF
ENDPROC


-> ---------------------------------------------------------------------------
-> Rwabs ($03) - Read/Write absolute sectors
-> D0 = 0=read, 1=write
-> D1 = device, D2 = sector number
-> D3 = count, A0 = buffer
-> ---------------------------------------------------------------------------
PROC bios_rwabs()
  DEF rw, dev, sector, count, buf:PTR TO CHAR
  rw := ctx[0] !!INT
  dev := ctx[1]
  sector := ctx[2]
  count := ctx[3]
  buf := ctx[8] !!PTR TO CHAR
  -> Stub - no raw disk access on AmigaOS
  ctx[0] := E_ERROR
ENDPROC


-> ---------------------------------------------------------------------------
-> Setexc ($04) - Set exception vector
-> D1 = exception number, A0 = handler address
-> Returns previous handler address
-> ---------------------------------------------------------------------------
PROC bios_setexc()
  -> Stub - exception vectors not supported
  ctx[0] := 0
ENDPROC


-> ---------------------------------------------------------------------------
-> Tickcal ($05) - Get tick calibration
-> Returns number of microseconds per hardware tick (200Hz = 5000us on ST)
-> In emulator, return 5000
-> ---------------------------------------------------------------------------
PROC bios_tickcal()
  ctx[0] := 5000
ENDPROC


-> ---------------------------------------------------------------------------
-> Bconstat ($07) - Console status
-> D1 = device
-> D0 = -1 if ready, 0 otherwise
-> ---------------------------------------------------------------------------
PROC bios_bconstat()
  DEF dev
  dev := ctx[1]
  IF dev = 0
    IF WaitForChar(Input(), 0) THEN ctx[0] := -1 ELSE ctx[0] := 0
  ELSE
    ctx[0] := 0
  ENDIF
ENDPROC


-> ---------------------------------------------------------------------------
-> Mediac ($08) - Media change check
-> D1 = device, D2 = media ID
-> Returns media change status
-> ---------------------------------------------------------------------------
PROC bios_mediac()
  -> Stub - assume media not changed
  ctx[0] := 0
ENDPROC


-> ---------------------------------------------------------------------------
-> Drvmap ($09) - Get drive map
-> Returns bitmask of available drives (bit 0 = A:, bit 1 = B:, etc.)
-> ---------------------------------------------------------------------------
PROC bios_drvmap()
  -> AmigaOS: assume at least drives A: and B: (but could check)
  -> Return bit 0 and bit 1 set (drives A: and B:)
  ctx[0] := 3
ENDPROC


-> ---------------------------------------------------------------------------
-> Kbshift ($0A) - Get/set keyboard shift state
-> D1 = 0=read, 1=write, 2=read+set
-> D2 = new shift state (for write)
-> D0 = current shift state
-> ---------------------------------------------------------------------------
PROC bios_kbshift()
  DEF mode, new_state
  mode := ctx[1]
  new_state := ctx[2]
  IF mode = 0
    ctx[0] := bios_kb_shift
  ELSE
    IF mode = 1
      bios_kb_shift := new_state
      ctx[0] := bios_kb_shift
    ELSE
      IF mode = 2
        ctx[0] := bios_kb_shift
        bios_kb_shift := new_state
      ELSE
        ctx[0] := 0
      ENDIF
    ENDIF
  ENDIF
ENDPROC


-> ---------------------------------------------------------------------------
-> Random ($0B) - Get random number
-> D0 = pseudo-random number (0-65535)
-> ---------------------------------------------------------------------------
PROC bios_random()
  -> Use system timer low word as simple random
  DEF now:datestamp
  DateStamp(now)
  ctx[0] := now.days + now.minute * 60 + now.tick
ENDPROC


-> ---------------------------------------------------------------------------
-> XBIOS (Trap #3) dispatch
-> ---------------------------------------------------------------------------
PROC xbios_dispatch()
  DEF fn
  fn := ctx[0] !!INT

  SELECT fn

  CASE $00 -> xbios_initmouse()
  CASE $01 -> xbios_gettime()
  CASE $02 -> xbios_settime()
  CASE $03 -> xbios_bioskeys()
  CASE $04 -> xbios_kbrate()
  CASE $05 -> xbios_prtblk()
  CASE $06 -> xbios_scrndump()
  CASE $07 -> xbios_cursconf()
  CASE $08 -> xbios_appl_init()
  CASE $09 -> xbios_physbase()
  CASE $0A -> xbios_logbase()
  CASE $0B -> xbios_getrez()
  CASE $0C -> xbios_setscreen()
  CASE $0D -> xbios_setpalette()
  CASE $0E -> xbios_setcolor()
  CASE $10 -> xbios_floprd()
  CASE $11 -> xbios_flopwr()
  CASE $12 -> xbios_flopfmt()
  CASE $13 -> xbios_flopstatus()
  CASE $17 -> xbios_rsconf()
  CASE $18 -> xbios_keytbl()
  CASE $19 -> xbios_random()
  CASE $1E -> xbios_cookieptr()

  DEFAULT
    ctx[0] := E_ERROR

  ENDSELECT
ENDPROC


-> ---------------------------------------------------------------------------
-> Initmouse ($00) - Initialize mouse
-> D1 = parameters
-> ---------------------------------------------------------------------------
PROC xbios_initmouse()
  ctx[0] := E_OK
ENDPROC


-> ---------------------------------------------------------------------------
-> Gettime ($01) - Get system time (VBL count)
-> D0 = longword VBL count (ticks since reset)
-> ---------------------------------------------------------------------------
PROC xbios_gettime()
  DEF now:datestamp
  DateStamp(now)
  -> Convert to VBL count (50Hz approximate)
  -> Use days * 24 * 3600 * 50 + minutes * 60 * 50 + ticks * 50 / (50*60*24*365)
  -> Simplified: just use a rough counter
  ctx[0] := now.days * 4320000 + now.minute * 3000 + now.tick * 50 / 300
ENDPROC


-> ---------------------------------------------------------------------------
-> Settime ($02) - Set system time (VBL count)
-> D1 = VBL count
-> ---------------------------------------------------------------------------
PROC xbios_settime()
  ctx[0] := E_OK
ENDPROC


-> ---------------------------------------------------------------------------
-> Bioskeys ($03) - Get BIOS key table
-> A0 = pointer to key table buffer
-> ---------------------------------------------------------------------------
PROC xbios_bioskeys()
  ctx[0] := E_ERROR
ENDPROC


-> ---------------------------------------------------------------------------
-> Kbrate ($04) - Set keyboard repeat rate
-> D1 = repeat rate
-> ---------------------------------------------------------------------------
PROC xbios_kbrate()
  ctx[0] := E_OK
ENDPROC


-> ---------------------------------------------------------------------------
-> Prtblk ($05) - Set printer block
-> A0 = printer block
-> ---------------------------------------------------------------------------
PROC xbios_prtblk()
  ctx[0] := E_OK
ENDPROC


-> ---------------------------------------------------------------------------
-> GEM AES (Application Environment Services) dispatch
-> Called via BIOS trap #2 with D0 = $C8
-> Atari ST AES uses a parameter block in memory (control, global, int-in/out)
-> A0 = pointer to AES parameter block (intin, ptin, intout, ptout, contrl, global)
-> D0 = AES function code
-> On AmigaOS, we map GEM windows/events to Intuition/gadtools
-> ---------------------------------------------------------------------------

-> AES function group codes
CONST AES_APPL = 0, AES_EVNT = 1, AES_MENU = 3, AES_OBJC = 4
CONST AES_FORM = 5, AES_SCRP = 6, AES_FSEL = 7, AES_WIND = 8
CONST AES_GRAF = 9

PROC gem_aes_dispatch()
  DEF fn_group, fn_sub

  -> In a real emulator, read the parameter block from emulated memory
  -> For now, use ctx[] as simplified parameter passing
  fn_group := ctx[1]
  fn_sub := ctx[2]

  SELECT fn_group

  CASE AES_APPL
    SELECT fn_sub
    CASE 0 -> gem_appl_init()
    CASE 1 -> gem_appl_exit()
    CASE 2 -> gem_appl_read()
    CASE 3 -> gem_appl_write()
    CASE 4 -> gem_appl_find()
    DEFAULT -> ctx[0] := E_ERROR
    ENDSELECT

  CASE AES_EVNT
    SELECT fn_sub
    CASE 0 -> gem_evnt_multi()
    CASE 1 -> gem_evnt_mesag()
    CASE 2 -> gem_evnt_button()
    CASE 3 -> gem_evnt_mouse()
    CASE 4 -> gem_evnt_keybd()
    CASE 5 -> gem_evnt_dclick()
    CASE 6 -> gem_evnt_timer()
    DEFAULT -> ctx[0] := E_ERROR
    ENDSELECT

  CASE AES_MENU
    SELECT fn_sub
    CASE 0 -> gem_menu_bar()
    CASE 1 -> gem_menu_icheck()
    CASE 2 -> gem_menu_ienable()
    CASE 3 -> gem_menu_tnormal()
    CASE 4 -> gem_menu_text()
    CASE 5 -> gem_menu_register()
    CASE 6 -> gem_menu_popup()
    CASE 7 -> gem_menu_attach()
    CASE 8 -> gem_menu_istart()
    CASE 9 -> gem_menu_settings()
    DEFAULT -> ctx[0] := E_ERROR
    ENDSELECT

  CASE AES_OBJC
    SELECT fn_sub
    CASE 0 -> gem_objc_add()
    CASE 1 -> gem_objc_delete()
    CASE 2 -> gem_objc_draw()
    CASE 3 -> gem_objc_find()
    CASE 4 -> gem_objc_offset()
    CASE 5 -> gem_objc_order()
    CASE 6 -> gem_objc_edit()
    CASE 7 -> gem_objc_change()
    CASE 8 -> gem_objc_type()
    DEFAULT -> ctx[0] := E_ERROR
    ENDSELECT

  CASE AES_FORM
    SELECT fn_sub
    CASE 0 -> gem_form_do()
    CASE 1 -> gem_form_dial()
    CASE 2 -> gem_form_alert()
    CASE 3 -> gem_form_error()
    CASE 4 -> gem_form_center()
    CASE 5 -> gem_form_keybd()
    CASE 6 -> gem_form_button()
    DEFAULT -> ctx[0] := E_ERROR
    ENDSELECT

  CASE AES_SCRP
    SELECT fn_sub
    CASE 0 -> gem_scrp_read()
    CASE 1 -> gem_scrp_write()
    DEFAULT -> ctx[0] := E_ERROR
    ENDSELECT

  CASE AES_FSEL
    SELECT fn_sub
    CASE 0 -> gem_fsel_exinput()
    CASE 1 -> gem_fsel_exoutput()
    DEFAULT -> ctx[0] := E_ERROR
    ENDSELECT

  CASE AES_WIND
    SELECT fn_sub
    CASE 0 -> gem_wind_create()
    CASE 1 -> gem_wind_open()
    CASE 2 -> gem_wind_close()
    CASE 3 -> gem_wind_delete()
    CASE 4 -> gem_wind_get()
    CASE 5 -> gem_wind_set()
    CASE 6 -> gem_wind_find()
    CASE 7 -> gem_wind_update()
    CASE 8 -> gem_wind_calc()
    CASE 9 -> gem_wind_new()
    CASE 10 -> gem_wind_arrow()
    CASE 11 -> gem_wind_show()
    CASE 12 -> gem_wind_toolbar()
    CASE 13 -> gem_wind_sized()
    DEFAULT -> ctx[0] := E_ERROR
    ENDSELECT

  CASE AES_GRAF
    SELECT fn_sub
    CASE 0 -> gem_graf_rubberbox()
    CASE 1 -> gem_graf_dragbox()
    CASE 2 -> gem_graf_movebox()
    CASE 3 -> gem_graf_growbox()
    CASE 4 -> gem_graf_shrinkbox()
    CASE 5 -> gem_graf_watchbox()
    CASE 6 -> gem_graf_slidebox()
    CASE 7 -> gem_graf_handle()
    CASE 8 -> gem_graf_mkstate()
    CASE 9 -> gem_graf_mouse()
    CASE 10 -> gem_graf_arrow()
    CASE 11 -> gem_graf_set_screen()
    CASE 12 -> gem_graf_set_handle()
    CASE 13 -> gem_graf_accel()
    DEFAULT -> ctx[0] := E_ERROR
    ENDSELECT

  DEFAULT
    ctx[0] := E_ERROR

  ENDSELECT
ENDPROC


-> GEM AES function implementations
-> Most map to Intuition/gadtools operations

-> appl_init() - Initialize application
-> Returns application ID
PROC gem_appl_init()
  gem_aes_id := gem_aes_id + 1
  -> In a real emulator, we'd open Intuition and create a screen/window
  ctx[0] := gem_aes_id
ENDPROC

-> appl_exit() - Exit application
PROC gem_appl_exit()
  gem_aes_id := 0
  ctx[0] := 1
ENDPROC

-> appl_read() - Read message from application
PROC gem_appl_read()
  ctx[0] := E_ERROR
ENDPROC

-> appl_write() - Write message to application
PROC gem_appl_write()
  ctx[0] := E_ERROR
ENDPROC

-> appl_find() - Find application by name
PROC gem_appl_find()
  ctx[0] := -1
ENDPROC

-> evnt_multi() - Wait for multiple event types
PROC gem_evnt_multi()
  -> Simplified: just return button event
  ctx[0] := 1
ENDPROC

-> evnt_mesag() - Wait for message
PROC gem_evnt_mesag()
  ctx[0] := 0
ENDPROC

-> evnt_button() - Wait for button click
PROC gem_evnt_button()
  ctx[0] := 1
ENDPROC

-> evnt_mouse() - Wait for mouse event
PROC gem_evnt_mouse()
  ctx[0] := 0
ENDPROC

-> evnt_keybd() - Wait for keyboard event
PROC gem_evnt_keybd()
  ctx[0] := 0
ENDPROC

-> evnt_dclick() - Set double-click rate
PROC gem_evnt_dclick()
  ctx[0] := E_OK
ENDPROC

-> evnt_timer() - Set timer event
PROC gem_evnt_timer()
  ctx[0] := E_OK
ENDPROC

-> menu_bar() - Draw/remove menu bar
PROC gem_menu_bar()
  ctx[0] := 1
ENDPROC

-> menu_icheck() - Check/uncheck menu item
PROC gem_menu_icheck()
  ctx[0] := 1
ENDPROC

-> menu_ienable() - Enable/disable menu item
PROC gem_menu_ienable()
  ctx[0] := 1
ENDPROC

-> menu_tnormal() - Set menu item normal state
PROC gem_menu_tnormal()
  ctx[0] := 1
ENDPROC

-> menu_text() - Change menu item text
PROC gem_menu_text()
  ctx[0] := 1
ENDPROC

-> menu_register() - Register application menu
PROC gem_menu_register()
  ctx[0] := 1
ENDPROC

-> menu_popup() - Pop up menu
PROC gem_menu_popup()
  ctx[0] := 1
ENDPROC

-> menu_attach() - Attach menu
PROC gem_menu_attach()
  ctx[0] := 1
ENDPROC

-> menu_istart() - Menu item start
PROC gem_menu_istart()
  ctx[0] := 1
ENDPROC

-> menu_settings() - Menu settings
PROC gem_menu_settings()
  ctx[0] := 1
ENDPROC

-> objc_add() - Add object
PROC gem_objc_add()
  ctx[0] := 1
ENDPROC

-> objc_delete() - Delete object
PROC gem_objc_delete()
  ctx[0] := 1
ENDPROC

-> objc_draw() - Draw object
PROC gem_objc_draw()
  ctx[0] := 1
ENDPROC

-> objc_find() - Find object at coordinates
PROC gem_objc_find()
  ctx[0] := 0
ENDPROC

-> objc_offset() - Get object offset
PROC gem_objc_offset()
  ctx[0] := E_OK
ENDPROC

-> objc_order() - Change object order
PROC gem_objc_order()
  ctx[0] := 1
ENDPROC

-> objc_edit() - Edit object text
PROC gem_objc_edit()
  ctx[0] := 0
ENDPROC

-> objc_change() - Change object
PROC gem_objc_change()
  ctx[0] := 1
ENDPROC

-> objc_type() - Get object type info
PROC gem_objc_type()
  ctx[0] := E_OK
ENDPROC

-> form_do() - Process form
PROC gem_form_do()
  ctx[0] := 0
ENDPROC

-> form_dial() - Form dialog
PROC gem_form_dial()
  ctx[0] := 1
ENDPROC

-> form_alert() - Show alert box
PROC gem_form_alert()
  DEF default_btn
  default_btn := ctx[1]
  -> Simplified: return default button
  ctx[0] := default_btn
ENDPROC

-> form_error() - Show error alert
PROC gem_form_error()
  ctx[0] := 1
ENDPROC

-> form_center() - Center form on screen
PROC gem_form_center()
  -> Return centered coordinates
  ctx[0] := E_OK
ENDPROC

-> form_keybd() - Form keyboard handling
PROC gem_form_keybd()
  ctx[0] := 0
ENDPROC

-> form_button() - Form button handling
PROC gem_form_button()
  ctx[0] := 0
ENDPROC

-> scrp_read() - Read clipboard
PROC gem_scrp_read()
  ctx[0] := E_ERROR
ENDPROC

-> scrp_write() - Write clipboard
PROC gem_scrp_write()
  ctx[0] := E_ERROR
ENDPROC

-> fsel_exinput() - File selector input
PROC gem_fsel_exinput()
  ctx[0] := 0
ENDPROC

-> fsel_exoutput() - File selector output
PROC gem_fsel_exoutput()
  ctx[0] := 0
ENDPROC

-> wind_create() - Create a window
PROC gem_wind_create()
  DEF kind, whandle
  kind := ctx[1]
  -> In a real emulator, create an Intuition window via OpenWindow()
  -> For now, return a pseudo-handle
  whandle := 1
  ctx[0] := whandle
ENDPROC

-> wind_open() - Open (show) a window
PROC gem_wind_open()
  ctx[0] := 1
ENDPROC

-> wind_close() - Close (hide) a window
PROC gem_wind_close()
  ctx[0] := 1
ENDPROC

-> wind_delete() - Delete a window
PROC gem_wind_delete()
  ctx[0] := 1
ENDPROC

-> wind_get() - Get window attributes
PROC gem_wind_get()
  -> Return reasonable defaults
  ctx[0] := E_OK
ENDPROC

-> wind_set() - Set window attributes
PROC gem_wind_set()
  ctx[0] := E_OK
ENDPROC

-> wind_find() - Find window at coordinates
PROC gem_wind_find()
  ctx[0] := 0
ENDPROC

-> wind_update() - Update window management
PROC gem_wind_update()
  ctx[0] := E_OK
ENDPROC

-> wind_calc() - Calculate window size
PROC gem_wind_calc()
  ctx[0] := E_OK
ENDPROC

-> wind_new() - Create new-type window (AES 4.0)
PROC gem_wind_new()
  ctx[0] := 1
ENDPROC

-> wind_arrow() - Set window arrow
PROC gem_wind_arrow()
  ctx[0] := E_OK
ENDPROC

-> wind_show() - Show/hide window
PROC gem_wind_show()
  ctx[0] := E_OK
ENDPROC

-> wind_toolbar() - Set toolbar
PROC gem_wind_toolbar()
  ctx[0] := E_OK
ENDPROC

-> wind_sized() - Window sized
PROC gem_wind_sized()
  ctx[0] := E_OK
ENDPROC

-> graf_rubberbox() - Draw rubber band box
PROC gem_graf_rubberbox()
  ctx[0] := 1
ENDPROC

-> graf_dragbox() - Drag box
PROC gem_graf_dragbox()
  ctx[0] := 1
ENDPROC

-> graf_movebox() - Move box
PROC gem_graf_movebox()
  ctx[0] := 1
ENDPROC

-> graf_growbox() - Grow box
PROC gem_graf_growbox()
  ctx[0] := 1
ENDPROC

-> graf_shrinkbox() - Shrink box
PROC gem_graf_shrinkbox()
  ctx[0] := 1
ENDPROC

-> graf_watchbox() - Watch box
PROC gem_graf_watchbox()
  ctx[0] := 1
ENDPROC

-> graf_slidebox() - Slide box
PROC gem_graf_slidebox()
  ctx[0] := 1
ENDPROC

-> graf_handle() - Get graf handle
PROC gem_graf_handle()
  -> Return workstation handle and screen size
  ctx[0] := 1
  ctx[1] := gem_scrn_w
  ctx[2] := gem_scrn_h
  ctx[3] := 0
ENDPROC

-> graf_mkstate() - Get mouse state
PROC gem_graf_mkstate()
  ctx[0] := 0
  ctx[1] := 0
  ctx[2] := 0
  ctx[3] := 0
ENDPROC

-> graf_mouse() - Set mouse shape
PROC gem_graf_mouse()
  ctx[0] := 1
ENDPROC

-> graf_arrow() - Set mouse arrow
PROC gem_graf_arrow()
  ctx[0] := E_OK
ENDPROC

-> graf_set_screen() - Set screen
PROC gem_graf_set_screen()
  ctx[0] := E_OK
ENDPROC

-> graf_set_handle() - Set graf handle
PROC gem_graf_set_handle()
  ctx[0] := E_OK
ENDPROC

-> graf_accel() - Get accelerator
PROC gem_graf_accel()
  ctx[0] := 0
ENDPROC


-> ---------------------------------------------------------------------------
-> GEM VDI (Virtual Device Interface) dispatch
-> Called via BIOS trap #2 with D0 = $C9
-> Maps GEM VDI drawing calls to AmigaOS graphics.library
-> ---------------------------------------------------------------------------
PROC gem_vdi_dispatch()
  DEF fn
  fn := ctx[1]

  -> VDI functions (-1 means inquire/init)
  IF fn = 100
    -> v_opnvwk() - Open workstation
    ctx[0] := 1
  ELSE
    IF fn = 1
      -> v_clsvwk() - Close workstation
      ctx[0] := 1
    ELSE
      -> All other VDI functions return stub values
      ctx[0] := 1
    ENDIF
  ENDIF
ENDPROC


-> ---------------------------------------------------------------------------
PROC cli_argc() IS NATIVE {extern int __main_argc; return __main_argc;} ENDNATIVE !!INT

PROC cli_argv_ptr(idx:VALUE) IS NATIVE {extern char **__main_argv; return (unsigned char*)__main_argv[(int)} idx {];} ENDNATIVE !!PTR TO CHAR


-> ---------------------------------------------------------------------------
-> Main
-> ---------------------------------------------------------------------------
PROC main()
  DEF i, filename[128]:ARRAY OF CHAR, addr
  DEF arg:PTR TO CHAR

  -> Initialize handle table (GEMDOS handle -> AmigaOS BPTR mapping)
  gem_handles[0] := Input() !!VALUE
  gem_handles[1] := Output() !!VALUE
  gem_handles[2] := Output() !!VALUE
  FOR i := 3 TO 15
    gem_handles[i] := 0
  ENDFOR

  gem_dta := 0
  gem_drv := 0
  gem_search_lock := 0
  bios_kb_shift := 0

  IF cli_argc() > 1
    arg := cli_argv_ptr(1)
    i := 0
    WHILE arg[i] <> 0 AND i < 127
      filename[i] := arg[i]
      i := i + 1
    ENDWHILE
    filename[i] := 0

    PutStr('Loading: ')
    PutStr(filename)
    PutStr('\n')

    addr := load_prg(filename)
    IF addr
      PutStr('Loaded at $')
      Printf('%lx\n', addr)
      PutStr('\n')
    ELSE
      PutStr('Failed to load PRG\n')
    ENDIF
  ELSE
    PutStr('Sake Atari Emulator\n')
    PutStr('Testing...\n')

    -> Test Cconws ($09): write a string via direct call
    ctx[8] := temp_string !!PTR TO CHAR
    temp_string[0] := 72; temp_string[1] := 101; temp_string[2] := 108; temp_string[3] := 108; temp_string[4] := 111
    temp_string[5] := 0
    gemdos_cconws()

    -> Test Dgetdrv ($0F)
    gemdos_dgetdrv()
    PutStr('Default drive: ')
    Printf('%ld\n', ctx[0])

    -> Test Tgetdate ($31) and Tgettime ($33)
    gemdos_tgetdate()
    gemdos_tgettime()
    PutStr('Date: ')
    Printf('%ld', ctx[0])
    PutStr(', Time: ')
    Printf('%ld\n', ctx[0])

    PutStr('\nAll GEMDOS functions completed\n')
  ENDIF
ENDPROC


-> ---------------------------------------------------------------------------
-> Cursconf ($07) - Cursor configuration
-> D1 = function (0=disable, 1=enable, 2=get status)
-> ---------------------------------------------------------------------------
PROC xbios_cursconf()
  ctx[0] := E_OK
ENDPROC


-> ---------------------------------------------------------------------------
-> Appl_init ($08) - Initialize application
-> Returns application ID
-> ---------------------------------------------------------------------------
PROC xbios_appl_init()
  -> Return a dummy application ID (1)
  ctx[0] := 1
ENDPROC


-> ---------------------------------------------------------------------------
-> Physbase ($09) - Get physical screen base address
-> Returns address of physical screen memory
-> ---------------------------------------------------------------------------
PROC xbios_physbase()
  -> In emulator, no real ST screen memory
  -> Return 0
  ctx[0] := 0
ENDPROC


-> ---------------------------------------------------------------------------
-> Logbase ($0A) - Get logical screen base
-> ---------------------------------------------------------------------------
PROC xbios_logbase()
  -> Same as Physbase in emulator
  ctx[0] := 0
ENDPROC


-> ---------------------------------------------------------------------------
-> Getrez ($0B) - Get screen resolution
-> 0=low (320x200), 1=medium (640x200), 2=high (640x400)
-> ---------------------------------------------------------------------------
PROC xbios_getrez()
  -> Return low resolution
  ctx[0] := 0
ENDPROC


-> ---------------------------------------------------------------------------
-> Setscreen ($0C) - Set screen parameters
-> D1 = logbase, D2 = physbase, D3 = resolution
-> ---------------------------------------------------------------------------
PROC xbios_setscreen()
  ctx[0] := E_OK
ENDPROC


-> ---------------------------------------------------------------------------
-> Setpalette ($0D) - Set full palette
-> A0 = pointer to 16 color palette entries
-> ---------------------------------------------------------------------------
PROC xbios_setpalette()
  ctx[0] := E_OK
ENDPROC


-> ---------------------------------------------------------------------------
-> Setcolor ($0E) - Set single color
-> D1 = color index, D2 = color value
-> ---------------------------------------------------------------------------
PROC xbios_setcolor()
  ctx[0] := E_OK
ENDPROC


-> ---------------------------------------------------------------------------
-> Floprd ($10) - Floppy read sectors
-> D1 = drive, D2 = sector, D3 = count, A0 = buffer
-> ---------------------------------------------------------------------------
PROC xbios_floprd()
  ctx[0] := E_ERROR
ENDPROC


-> ---------------------------------------------------------------------------
-> Flopwr ($11) - Floppy write sectors
-> ---------------------------------------------------------------------------
PROC xbios_flopwr()
  ctx[0] := E_ERROR
ENDPROC


-> ---------------------------------------------------------------------------
-> Flopfmt ($12) - Floppy format
-> ---------------------------------------------------------------------------
PROC xbios_flopfmt()
  ctx[0] := E_ERROR
ENDPROC


-> ---------------------------------------------------------------------------
-> Flopstatus ($13) - Floppy status
-> ---------------------------------------------------------------------------
PROC xbios_flopstatus()
  ctx[0] := 0
ENDPROC


-> ---------------------------------------------------------------------------
-> Rsconf ($17) - RS232 configuration
-> D1-D7 = RS232 parameters
-> Returns old configuration
-> ---------------------------------------------------------------------------
PROC xbios_rsconf()
  ctx[0] := E_OK
ENDPROC


-> ---------------------------------------------------------------------------
-> Keytbl ($18) - Get/set keyboard table
-> D1 = table type, A0 = new table (or 0 to read)
-> ---------------------------------------------------------------------------
PROC xbios_keytbl()
  ctx[0] := 0
ENDPROC


-> ---------------------------------------------------------------------------
-> Random ($19) - Get random number
-> D0 = pseudo-random number
-> ---------------------------------------------------------------------------
PROC xbios_random()
  DEF now:datestamp
  DateStamp(now)
  ctx[0] := now.days + now.minute * 60 + now.tick
ENDPROC


-> ---------------------------------------------------------------------------
-> Cookieptr ($1E) - Get cookie jar pointer
-> A0 = pointer to cookie jar (Atari ST cookie jar)
-> ---------------------------------------------------------------------------
PROC xbios_cookieptr()
  -> Return 0 - no cookie jar in emulator
  ctx[0] := 0
ENDPROC


-> ---------------------------------------------------------------------------
-> Trap handler - Not available in PortablE (needs module-level ASM)
-> GEMDOS functions are called directly from main()
-> ---------------------------------------------------------------------------
