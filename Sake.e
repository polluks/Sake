-> Sake - Atari Kernel Emulator
-> GEMDOS (TRAP #1), BIOS (TRAP #2), XBIOS (TRAP #3), GEM AES/VDI
-> Maps Atari ST system calls to AmigaOS libraries

OPT POINTER, NATIVE
MODULE 'exec/tasks', 'dos', 'exec', 'intuition/intuition', 'graphics'

-> ---------------------------------------------------------------------------
-> Global state
-> ---------------------------------------------------------------------------
DEF oldTrapCode:APTR
DEF ctx[32]:ARRAY OF VALUE    -> Saved register context + extended params
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

-> Memory allocation tracking for Mfree (pointer -> size table)
DEF gem_alloc_ptr[64]:ARRAY OF VALUE
DEF gem_alloc_size[64]:ARRAY OF VALUE
DEF gem_alloc_count
DEF gem_window_list[16]:ARRAY OF VALUE -> Open window handles (Intuition Window ptrs)
DEF gem_aes_global[32]:ARRAY OF VALUE -> AES global array
DEF gem_scrn_w, gem_scrn_h   -> Virtual screen dimensions
DEF gadtools_base            -> gadtools.library base (opened at runtime)
DEF reqtools_base            -> reqtools.library base (opened at runtime)
DEF gem_cookie_jar           -> pointer to cookie jar (AllocMem'd block)
DEF gem_cookie_count         -> number of active entries (excluding sentinel)
DEF gem_cookie_max           -> max entries the jar can hold

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
        result := 1005
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
  fn := asINT(ctx[0])

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
  CASE $164 -> gemdos_ssystem()

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
  ctx[0] := asLONG(ch[0])
ENDPROC


-> ---------------------------------------------------------------------------
-> Cconout ($02) - Write character to console
-> D1 = character to write
-> ---------------------------------------------------------------------------
PROC gemdos_cconout()
  DEF ch[1]:STRING
  ch[0] := asCHAR(ctx[1])
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
  src := asPTR(ctx[8])
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
  buf := asPTR(ctx[8])
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
  buf[0] := pos
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
  src := asPTR(ctx[8])
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
  dst := asPTR(ctx[8])
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
  filename := asPTR(ctx[8])
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
      Close(asBPTR(handle))
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
    Close(asBPTR(gem_handles[handle]))
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
  buf := asPTR(ctx[8])

  IF handle >= 0 AND handle <= 15
    IF handle = 0
      result := Read(Input(), buf, count)
    ELSE
      result := Read(asBPTR(gem_handles[handle]), buf, count)
    ENDIF
    IF result < 0
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
  buf := asPTR(ctx[8])

  IF handle >= 0 AND handle <= 15
    IF handle = 0
      result := Write(Input(), buf, count)
    ELSE
      IF handle = 1 OR handle = 2
        result := Write(Output(), buf, count)
      ELSE
        result := Write(asBPTR(gem_handles[handle]), buf, count)
      ENDIF
    ENDIF
    IF result < 0
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
  filename := asPTR(ctx[8])
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
    newpos := Seek(asBPTR(gem_handles[handle]), offset, mode)
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
  filename := asPTR(ctx[8])
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
  DEF fname[256]:ARRAY OF CHAR
  pattern := asPTR(ctx[8])
  attr := ctx[1]

  IF gem_search_lock
    UnLock(asBPTR(gem_search_lock))
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
    -> Walk directory entries until one matches the pattern
    gem_search_first := TRUE
    WHILE ExNext(asBPTR(gem_search_lock), fib)
      -> Extract filename from FileInfoBlock
      i := 0
      WHILE fib.filename[i] <> 0 AND i < 255
        fname[i] := fib.filename[i]
        i := i + 1
      ENDWHILE
      fname[i] := 0
      -> Check against pattern
      IF FileMatch(fname, gem_search_pattern)
        IF gem_dta <> 0
          FillDTA(asPTR(gem_dta), fib)
        ENDIF
        ctx[0] := E_OK
        ENDPROC
      ENDIF
    ENDWHILE
    -> No matching file found
    ctx[0] := E_ERROR
  ELSE
    ctx[0] := E_ERROR
  ENDIF
ENDPROC


PROC gemdos_fsnext()
  DEF fname[256]:ARRAY OF CHAR, i
  IF gem_search_lock
    WHILE ExNext(asBPTR(gem_search_lock), fib)
      -> Extract filename from FileInfoBlock
      i := 0
      WHILE fib.filename[i] <> 0 AND i < 255
        fname[i] := fib.filename[i]
        i := i + 1
      ENDWHILE
      fname[i] := 0
      -> Check against pattern
      IF FileMatch(fname, gem_search_pattern)
        IF gem_dta <> 0
          FillDTA(asPTR(gem_dta), fib)
        ENDIF
        ctx[0] := E_OK
        ENDPROC
      ENDIF
    ENDWHILE
    -> No more matching files
    ctx[0] := E_ERROR
  ELSE
    ctx[0] := E_ERROR
  ENDIF
ENDPROC


-> ---------------------------------------------------------------------------
-> FileMatch - Match filename against GEMDOS pattern (* and ?)
-> Returns TRUE if name matches pattern
-> ---------------------------------------------------------------------------
PROC FileMatch(name:PTR TO CHAR, pattern:PTR TO CHAR)
  DEF i, j, result, star_pos
  i := 0
  j := 0
  result := TRUE
  star_pos := -1
  WHILE pattern[j] <> 0 AND result
    IF pattern[j] = '*'
      result := TRUE
      star_pos := j
    ELSE
      IF pattern[j] = '?'
        IF name[i] = 0 THEN result := FALSE
        i := i + 1
      ELSE
        IF name[i] <> pattern[j] THEN result := FALSE
        i := i + 1
      ENDIF
    ENDIF
    IF result THEN j := j + 1
  ENDWHILE
  IF name[i] <> 0 AND star_pos < 0 THEN result := FALSE
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
  dta[26] := asCHAR((fib_ptr.size) AND $FF)
  dta[27] := asCHAR((fib_ptr.size / 256) AND $FF)
  dta[28] := asCHAR((fib_ptr.size / 65536) AND $FF)
  dta[29] := asCHAR((fib_ptr.size / 16777216) AND $FF)

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
  oldname := asPTR(ctx[8])
  newname := asPTR(ctx[9])

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
  dirname := asPTR(ctx[8])
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
  dirname := asPTR(ctx[8])
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
  pathname := asPTR(ctx[8])
  -> Use Dsetpath logic
  gemdos_dsetpath()
ENDPROC


-> ---------------------------------------------------------------------------
-> Fgetdta ($25) - Get Disk Transfer Address
-> D0 = DTA pointer
-> ---------------------------------------------------------------------------
PROC gemdos_fgetdta()
  ctx[0] := asLONG(gem_dta)
ENDPROC


-> ---------------------------------------------------------------------------
-> Fsetdta ($26) - Set Disk Transfer Address
-> D1 = new DTA pointer
-> ---------------------------------------------------------------------------
PROC gemdos_fsetdta()
  DEF ptr
  ptr := ctx[1]
  gem_dta := asPTR(ptr)
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
    -> Track allocation size for Mfree
    IF gem_alloc_count < 64
      gem_alloc_ptr[gem_alloc_count] := asLONG(ptr)
      gem_alloc_size[gem_alloc_count] := size
      gem_alloc_count := gem_alloc_count + 1
    ENDIF
    ctx[0] := asLONG(ptr)
  ENDIF
ENDPROC


-> ---------------------------------------------------------------------------
-> Mfree ($2A) - Free memory
-> D1 = pointer to memory block
-> ---------------------------------------------------------------------------
PROC gemdos_mfree()
  DEF ptr, i, found_size
  ptr := asPTR(ctx[1])
  -> Look up allocation size from tracking table
  found_size := 0
  FOR i := 0 TO gem_alloc_count - 1
    IF gem_alloc_ptr[i] = asLONG(ptr)
      found_size := gem_alloc_size[i]
      -> Remove from tracking table (swap with last)
      gem_alloc_ptr[i] := gem_alloc_ptr[gem_alloc_count - 1]
      gem_alloc_size[i] := gem_alloc_size[gem_alloc_count - 1]
      gem_alloc_count := gem_alloc_count - 1
      i := gem_alloc_count
    ENDIF
  ENDFOR
  IF found_size > 0
    FreeMem(asAPTR(ptr), found_size)
  ENDIF
  ctx[0] := E_OK
ENDPROC


-> ---------------------------------------------------------------------------
-> Mxalloc ($39) - Allocate memory with flags
-> D1 = number of bytes, D2 = flags (bit 0 = FAST, bit 1 = CLEAR)
-> D0 = pointer or 0
-> ---------------------------------------------------------------------------
PROC gemdos_mxalloc()
  DEF size, flags, memflags, ptr
  size := ctx[1]
  flags := ctx[2]
  memflags := 65538  -> MEMF_CLEAR | MEMF_PUBLIC
  IF (flags AND 1) = 0
    -> CHIP memory requested, use MEMF_CHIP
    memflags := 65538  -> Can't do chip on Amiga, use same
  ENDIF
  ptr := AllocMem(size, memflags)
  IF ptr <> 0
    -> Track allocation size for Mfree
    IF gem_alloc_count < 64
      gem_alloc_ptr[gem_alloc_count] := asLONG(ptr)
      gem_alloc_size[gem_alloc_count] := size
      gem_alloc_count := gem_alloc_count + 1
    ENDIF
  ENDIF
  ctx[0] := asLONG(ptr)
ENDPROC


-> ---------------------------------------------------------------------------
-> Ssystem ($164) - System function / cookie operations
-> D1 = sub-function
->   0 = S_INQUIRE: returns E_OK if Ssystem is supported
->   1 = S_GETCOOKIE: D2 = cookie id, A0 = pointer to value
->   2 = S_PUTCOOKIE: D2 = cookie id, D3 = value
->   3 = S_DELCOOKIE: D2 = cookie id
-> ---------------------------------------------------------------------------
PROC gemdos_ssystem()
  DEF subfn, cookie_id, value, value_ptr, id_str[5]:ARRAY OF CHAR, idx, base
  subfn := ctx[1]
  cookie_id := ctx[2]
  SELECT subfn
  CASE 0 -> -> S_INQUIRE: Ssystem is available
    ctx[0] := E_OK
  CASE 1 -> -> S_GETCOOKIE
    -> Convert long id to string for lookup
    id_str[0] := (cookie_id >> 24) AND $FF
    id_str[1] := (cookie_id >> 16) AND $FF
    id_str[2] := (cookie_id >> 8) AND $FF
    id_str[3] := cookie_id AND $FF
    id_str[4] := 0
    value_ptr := asPTR(ctx[3])
    IF gem_cookie_get(id_str, value_ptr)
      ctx[0] := E_OK
    ELSE
      ctx[0] := E_ERROR
    ENDIF
  CASE 2 -> -> S_PUTCOOKIE
    id_str[0] := (cookie_id >> 24) AND $FF
    id_str[1] := (cookie_id >> 16) AND $FF
    id_str[2] := (cookie_id >> 8) AND $FF
    id_str[3] := cookie_id AND $FF
    id_str[4] := 0
    -> If cookie already exists, update it
    idx := gem_cookie_find(id_str)
    IF idx >= 0
      base := gem_cookie_jar + idx * COOKIE_SIZE
      NATIVE {
        long *entry = (long *)base;
        entry[1] = ctx[3];
      } ENDNATIVE
      ctx[0] := E_OK
    ELSE
      -> Add new cookie
      IF gem_cookie_add(id_str, ctx[3])
        ctx[0] := E_OK
      ELSE
        ctx[0] := E_ERROR
      ENDIF
    ENDIF
  CASE 3 -> -> S_DELCOOKIE
    id_str[0] := (cookie_id >> 24) AND $FF
    id_str[1] := (cookie_id >> 16) AND $FF
    id_str[2] := (cookie_id >> 8) AND $FF
    id_str[3] := cookie_id AND $FF
    id_str[4] := 0
    IF gem_cookie_remove(id_str)
      ctx[0] := E_OK
    ELSE
      ctx[0] := E_ERROR
    ENDIF
  DEFAULT
    ctx[0] := E_ERROR
  ENDSELECT
ENDPROC


-> ---------------------------------------------------------------------------
-> Load an Atari ST PRG file into allocated memory
-> Returns address of loaded program, or 0 on failure
-> ---------------------------------------------------------------------------
PROC load_prg(filename:PTR TO CHAR)
  DEF fh:BPTR, header[32]:ARRAY OF CHAR, result
  DEF text_size, data_size, bss_size, total_size
  DEF addr:PTR TO CHAR, rel_flag, text_start
  result := 0

  fh := Open(filename, 1005)
  IF fh
    IF Read(fh, header, 32) >= 28
      text_size := asLONG(header[2])
      data_size := asLONG(header[6])
      bss_size := asLONG(header[10])
      text_start := asLONG(header[18])
      rel_flag := asINT(header[26])
      total_size := text_size + data_size + bss_size

      addr := asPTR(AllocMem(total_size, 65538))
      IF addr
        result := addr
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
        -> Apply relocation table if present
        IF result <> 0 AND rel_flag <> 0 AND text_start <> 0
          NATIVE {
            unsigned char *base = (unsigned char *)result;
            long adj = (long)result - text_start;
            long offset = 0;
            long td_size = text_size + data_size;
            if (adj != 0) {
              while (offset < td_size) {
                unsigned char ctrl = 0;
                if (Read(fh, &ctrl, 1) != 1) break;
                if (ctrl == 0) {
                  offset = (offset + 256) & ~0xFFL;
                } else if (ctrl & 0x80) {
                  offset += (ctrl & 0x7F) * 256;
                } else {
                  int i;
                  for (i = 0; i < ctrl; i++) {
                    unsigned char boff = 0;
                    if (Read(fh, &boff, 1) != 1) break;
                    long addr_fix = (offset & ~0xFFL) + boff;
                    if (addr_fix + 4 <= td_size) {
                      long *lp = (long *)(base + addr_fix);
                      *lp += adj;
                    }
                  }
                  offset = (offset & ~0xFFL) + 256;
                }
              }
            }
          } ENDNATIVE
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
  filename := asPTR(ctx[8])

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
  DEF now:datestamp, d, m, y, days_left
  DEF month_days[12]:ARRAY OF VALUE
  DateStamp(now)
  -> Convert AmigaOS days-since-1978 to year/month/day
  d := now.days
  -> Days per month (non-leap year)
  month_days[0] := 31; month_days[1] := 28; month_days[2] := 31
  month_days[3] := 30; month_days[4] := 31; month_days[5] := 30
  month_days[6] := 31; month_days[7] := 31; month_days[8] := 30
  month_days[9] := 31; month_days[10] := 30; month_days[11] := 31
  -> Epoch: Jan 1, 1978 = day 0. GEMDOS epoch is Jan 1, 1980.
  -> Days from 1978-01-01 to 1980-01-01 = 365 + 366 = 731
  d := d - 731
  IF d < 0 THEN d := 0
  y := 1980
  -> Subtract full years
  WHILE d >= 365
    -> Check for leap year (year divisible by 4)
    IF (y AND 3) = 0
      IF d >= 366
        d := d - 366
        y := y + 1
      ELSE
        d := d - 365
        y := y + 1
      ENDIF
    ELSE
      d := d - 365
      y := y + 1
    ENDIF
  ENDWHILE
  -> d is now day-of-year (0-based)
  m := 0
  days_left := d
  WHILE m < 11 AND days_left >= month_days[m]
    days_left := days_left - month_days[m]
    m := m + 1
  ENDWHILE
  -> Encode: bits 0-4=day, 5-8=month, 9-15=year-1980
  ctx[0] := ((y - 1980) * 512) + ((m + 1) * 32) + (days_left + 1)
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
  fn := asINT(ctx[0])

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
    ctx[0] := asLONG(ch[0])
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
    ch[0] := asCHAR(ctx[2])
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
  rw := asINT(ctx[0])
  dev := ctx[1]
  sector := ctx[2]
  count := ctx[3]
  buf := asPTR(ctx[8])
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
  fn := asINT(ctx[0])

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
-> ---------------------------------------------------------------------------
-> GEM AES (Application Environment Services) dispatch and implementation
-> Maps Atari GEM AES calls to AmigaOS Intuition
-> ---------------------------------------------------------------------------

CONST AES_APPL = 0, AES_EVNT = 1, AES_RSRC = 2, AES_MENU = 3, AES_OBJC = 4
CONST AES_FORM = 5, AES_SCRP = 6, AES_FSEL = 7, AES_WIND = 8
CONST AES_GRAF = 9

-> Window kinds (wind_create parameter)
CONST WIN_NAME = 1, WIN_CLOSER = 2, WIN_FULLER = 4, WIN_MOVER = 8
CONST WIN_SIZER = 16, WIN_UPARROW = 32, WIN_DNARROW = 64
CONST WIN_VSLIDE = 128, WIN_LFARROW = 256, WIN_RTARROW = 512
CONST WIN_HSLIDE = 1024, WIN_SMALLER = 2048, WIN_INFO = 4096

-> Window states
CONST WS_CLOSED = 0, WS_OPEN = 1, WS_ICONIFIED = 2

-> Window messages (for message queue)
CONST WM_REDRAW = 10, WM_TOPPED = 11, WM_CLOSED = 12
CONST WM_FULLED = 13, WM_ARROWED = 14, WM_HSLID = 15
CONST WM_VSLID = 16, WM_SIZED = 17, WM_MOVED = 18
CONST WM_UNTOPPED = 19, WM_ONTOP = 20

-> GEM object types (from objc.h)
CONST G_BOX = 0, G_TEXT = 1, G_BOXTEXT = 2, G_IMAGE = 3
CONST G_USERDEF = 4, G_IBOX = 5, G_BUTTON = 6, G_BOXCHAR = 7
CONST G_STRING = 8, G_FTEXT = 9, G_FBOXTEXT = 10, G_ICON = 11
CONST G_TITLE = 25, G_MENU = 26
CONST G_RBUTTON = 21, G_CHECKBOX = 22
CONST G_SELECTED = 1

-> Max tracked resources
CONST MAX_WINDOWS = 16, MAX_MENUS = 8, MAX_OBJECTS = 256
CONST MAX_MESSAGES = 64, MAX_APPS = 8

-> Window info structure
DEF gem_wind_kind[MAX_WINDOWS]:ARRAY OF VALUE
DEF gem_wind_state[MAX_WINDOWS]:ARRAY OF VALUE
DEF gem_wind_x[MAX_WINDOWS]:ARRAY OF VALUE
DEF gem_wind_y[MAX_WINDOWS]:ARRAY OF VALUE
DEF gem_wind_w[MAX_WINDOWS]:ARRAY OF VALUE
DEF gem_wind_h[MAX_WINDOWS]:ARRAY OF VALUE
DEF gem_wind_work_x[MAX_WINDOWS]:ARRAY OF VALUE
DEF gem_wind_work_y[MAX_WINDOWS]:ARRAY OF VALUE
DEF gem_wind_work_w[MAX_WINDOWS]:ARRAY OF VALUE
DEF gem_wind_work_h[MAX_WINDOWS]:ARRAY OF VALUE
DEF gem_wind_full_x[MAX_WINDOWS]:ARRAY OF VALUE
DEF gem_wind_full_y[MAX_WINDOWS]:ARRAY OF VALUE
DEF gem_wind_full_w[MAX_WINDOWS]:ARRAY OF VALUE
DEF gem_wind_full_h[MAX_WINDOWS]:ARRAY OF VALUE
DEF gem_wind_parent[MAX_WINDOWS]:ARRAY OF VALUE
DEF gem_wind_handle[MAX_WINDOWS]:ARRAY OF VALUE

-> Message queue for inter-application communication
DEF gem_msg_queue[MAX_MESSAGES]:ARRAY OF VALUE  -> message class
DEF gem_msg_src[MAX_MESSAGES]:ARRAY OF VALUE    -> source app ID
DEF gem_msg_len[MAX_MESSAGES]:ARRAY OF VALUE    -> message length
DEF gem_msg_data16[1024]:ARRAY OF VALUE -> full 16-word message data per slot (64*16)
DEF gem_msg_head, gem_msg_tail

-> Application tracking
DEF gem_app_list[MAX_APPS]:ARRAY OF VALUE -> app IDs
DEF gem_app_count

-> Menu tracking
DEF gem_menu_tree[8]:ARRAY OF VALUE -> GEM menu tree pointers
DEF gem_menu_owner[8]:ARRAY OF VALUE -> owning app ID
DEF gem_menu_count
DEF gem_menu_bar_visible -> 0=none, 1=shown
DEF gem_menu_active_app -> app ID whose menu bar is showing
-> Per-window AmigaOS menu strip tracking
DEF gem_wind_amiga_menu[16]:ARRAY OF VALUE -> AmigaOS Menu* per window slot
DEF gem_wind_newmenu[16]:ARRAY OF VALUE  -> NewMenu* per window slot (for cleanup)
DEF gem_wind_gem_menu_idx[16]:ARRAY OF VALUE -> index into gem_menu_tree[] per window

-> Object tree tracking
DEF gem_obj_tree[8]:ARRAY OF VALUE -> object tree pointers

-> Form state
DEF gem_form_active -> handle of active form dialog (-1 = none)

-> Radio button gadget tracking
-> Maps GEM object index to gadtools gadget pointer per form tree
DEF gem_radio_gad[32]:ARRAY OF VALUE   -> gadtools gadget pointers (up to 32 radio buttons)
DEF gem_radio_obj[32]:ARRAY OF VALUE   -> GEM object index for each gadget
DEF gem_radio_count                    -> number of active radio gadgets
DEF gem_radio_tree                     -> tree pointer for current radio group
DEF gem_radio_win                      -> window gadgets are attached to

-> Clipboard state
DEF gem_scrap_buffer[1024]:ARRAY OF CHAR
DEF gem_scrap_len

-> Resource (RSC) state
DEF gem_rsc_data                    -> pointer to loaded RSC data (0=none)
DEF gem_rsc_size                    -> size of loaded RSC data
DEF gem_rsc_tree_count              -> number of trees in loaded RSC
DEF gem_rsc_tree_table              -> pointer to array of tree root pointers

-> Graf (graphics) state
DEF gem_mouse_x, gem_mouse_y           -> Current mouse position
DEF gem_mouse_buttons                   -> Button state (bit0=left, bit1=right)
DEF gem_mouse_kstate                    -> Keyboard shift state
DEF gem_mouse_shape                     -> Current cursor shape (M_ON..M_POINT)
DEF gem_mouse_visible                   -> 0=hidden, 1=visible
DEF gem_graf_wk_handle                  -> Workstation handle
DEF gem_graf_char_w, gem_graf_char_h    -> Character cell size
DEF gem_graf_arrow_mode                 -> 0=menu nav, 1=normal arrows
DEF gem_graf_slidex[8]:ARRAY OF VALUE   -> Slider positions
DEF gem_graf_slidey[8]:ARRAY OF VALUE
DEF gem_graf_accel_key                  -> Last accelerator key
-> Mouse pointer data store (33 words: 16 mask + 16 data + 1 resolution/hotspot)
DEF gem_mouse_user_data[33]:ARRAY OF VALUE
DEF gem_mouse_user_hotx, gem_mouse_user_hoty
DEF gem_mouse_user_active
-> Predefined pointer shape bitmaps (16x16, 16 data words per shape)
DEF gem_mouse_arrow_data[16]:ARRAY OF VALUE
DEF gem_mouse_arrow_mask[16]:ARRAY OF VALUE
DEF gem_mouse_busy_data[16]:ARRAY OF VALUE
DEF gem_mouse_busy_mask[16]:ARRAY OF VALUE
DEF gem_mouse_ibeam_data[16]:ARRAY OF VALUE
DEF gem_mouse_ibeam_mask[16]:ARRAY OF VALUE
DEF gem_mouse_point_data[16]:ARRAY OF VALUE
DEF gem_mouse_point_mask[16]:ARRAY OF VALUE

-> ---------------------------------------------------------------------------
-> PortablE IS NATIVE wrappers for AmigaOS C functions (avoids inline NATIVE)
-> ---------------------------------------------------------------------------

PROC gem_SetPointer(window:PTR TO window, data:ARRAY OF VALUE, w:VALUE, h:VALUE, x:VALUE, y:VALUE) IS NATIVE { SetPointer((struct Window *)} window {, (UWORD *)} data {, (short)} w {, (short)} h {, (short)} x {, (short)} y {); } ENDNATIVE
PROC gem_ClearPointer(window:PTR TO window) IS NATIVE { ClearPointer((struct Window *)} window {); } ENDNATIVE

-> ---------------------------------------------------------------------------
-> Gadtools menu NATIVE wrappers
-> ---------------------------------------------------------------------------

-> Convert GEM OBJECT tree to AmigaOS NewMenu array (returns menu strip pointer)
PROC gem_CreateMenusFromTree(tree:VALUE) IS NATIVE {
  extern unsigned long gadtools_base;
  struct GEMObject { short next, head, tail; unsigned short flags, state, type; long spec; short x, y, w, h; };
  struct GEMObject *objtree = (struct GEMObject *)tree;
  struct NewMenu *nm;
  int i, count = 0, title_count = 0, item_count = 0;
  if (!gadtools_base || !tree) return 0;
  /* count titles and items */
  for (i = 0; objtree[i].type != 0; i++) {
    if (objtree[i].type == 25) title_count++;
    else if (objtree[i].type == 26) item_count++;
  }
  if (title_count == 0) return 0;
  nm = (struct NewMenu *)AllocMem((title_count + item_count + 1) * sizeof(struct NewMenu), MEMF_CLEAR);
  if (!nm) return 0;
  count = 0;
  for (i = 0; objtree[i].type != 0; i++) {
    if (objtree[i].type == 25) {
      /* G_TITLE -> NM_TITLE */
      nm[count].nm_Type = NM_TITLE;
      nm[count].nm_Label = (STRPTR)objtree[i].spec;
      nm[count].nm_CommKey = NULL;
      nm[count].nm_Flags = 0;
      nm[count].nm_MutualExclude = 0;
      nm[count].nm_UserData = (APTR)i;
      count++;
    } else if (objtree[i].type == 26) {
      /* G_STRING -> NM_ITEM */
      nm[count].nm_Type = NM_ITEM;
      nm[count].nm_Label = (STRPTR)objtree[i].spec;
      nm[count].nm_CommKey = NULL;
      nm[count].nm_Flags = (objtree[i].state & 0x01) ? 0 : NM_ITEMDISABLED;
      if (objtree[i].state & 0x08) nm[count].nm_Flags |= CHECKIT;
      nm[count].nm_MutualExclude = 0;
      nm[count].nm_UserData = (APTR)i;
      count++;
    }
  }
  nm[count].nm_Type = NM_END;
  return (unsigned long)nm;
} ENDNATIVE !!VALUE

-> Create menus from NewMenu array via gadtools
PROC gem_CreateMenus(nm:VALUE) IS NATIVE {
  extern unsigned long gadtools_base;
  struct Menu *m;
  if (!gadtools_base || !nm) return 0;
  m = CreateMenusA((struct NewMenu *)nm, NULL);
  return (unsigned long)m;
} ENDNATIVE !!VALUE

-> Layout menus for the current screen
PROC gem_LayoutMenus(menu:VALUE) IS NATIVE {
  extern unsigned long gadtools_base;
  struct Screen *scr = LockPubScreen(NULL);
  if (!gadtools_base || !menu || !scr) { if (scr) UnlockPubScreen(NULL, scr); return 0; }
  LayoutMenusA((struct Menu *)menu, scr, NULL);
  UnlockPubScreen(NULL, scr);
  return 1;
} ENDNATIVE !!VALUE

-> Attach menu strip to window
PROC gem_SetMenuStrip(win:VALUE, menu:VALUE) IS NATIVE {
  extern unsigned long gadtools_base;
  if (!gadtools_base || !win || !menu) return 0;
  return (unsigned long)SetMenuStrip((struct Window *)win, (struct Menu *)menu);
} ENDNATIVE !!VALUE

-> Remove menu strip from window
PROC gem_ClearMenuStrip(win:VALUE) IS NATIVE {
  extern unsigned long gadtools_base;
  if (!gadtools_base || !win) return;
  ClearMenuStrip((struct Window *)win);
} ENDNATIVE

-> Free a menu strip
PROC gem_FreeMenus(menu:VALUE) IS NATIVE {
  extern unsigned long gadtools_base;
  if (!gadtools_base || !menu) return;
  FreeMenus((struct Menu *)menu);
} ENDNATIVE

-> Free NewMenu array
PROC gem_FreeNewMenu(nm:VALUE) IS NATIVE {
  if (!nm) return;
  FreeMem((APTR)nm, 0);
} ENDNATIVE

-> Check/uncheck a menu item via GT_SetGadgetAttrs
PROC gem_SetMenuCheck(win:VALUE, menu:VALUE, item_num:VALUE, checked:VALUE) IS NATIVE {
  extern unsigned long gadtools_base;
  struct Window *w = (struct Window *)win;
  struct Menu *m = (struct Menu *)menu;
  struct MenuItem *mi;
  UWORD menunum = item_num >> 8;
  UWORD itemnum = item_num & 0xFF;
  if (!gadtools_base || !w || !m) return;
  mi = ItemAddress(m, (menunum << 8) | itemnum);
  if (mi) {
    if (checked) mi->Flags |= CHECKED;
    else mi->Flags &= ~CHECKED;
    GT_SetGadgetAttrs(NULL, w, NULL, TAG_DONE);
  }
} ENDNATIVE

-> Enable/disable a menu item
PROC gem_SetMenuEnable(win:VALUE, menu:VALUE, item_num:VALUE, enabled:VALUE) IS NATIVE {
  extern unsigned long gadtools_base;
  struct Window *w = (struct Window *)win;
  struct Menu *m = (struct Menu *)menu;
  struct MenuItem *mi;
  UWORD menunum = item_num >> 8;
  UWORD itemnum = item_num & 0xFF;
  if (!gadtools_base || !w || !m) return;
  mi = ItemAddress(m, (menunum << 8) | itemnum);
  if (mi) {
    if (enabled) mi->Flags &= ~ITEMENABLED;
    else mi->Flags |= ITEMENABLED;
    GT_SetGadgetAttrs(NULL, w, NULL, TAG_DONE);
  }
} ENDNATIVE

-> Decode AmigaOS MENUPICK code: returns menu number and item number
PROC gem_DecodeMenuCode(code:VALUE, menunum:VALUE, itemnum:VALUE) IS NATIVE {
  unsigned short c = (unsigned short)code;
  if (c == MENUNUM) return 0;
  *((short *)menunum) = (c >> 8) & 0xFF;
  *((short *)itemnum) = c & 0xFF;
  return 1;
} ENDNATIVE !!INT

-> ---------------------------------------------------------------------------
-> Radio button gadget creation via gadtools
-> Scans a GEM object tree for type 21 (G_RBUTTON) objects and creates
-> gadtools RADIO_KIND gadgets.  Each group of consecutive radio buttons
-> becomes one gadtools radio group (gadtools manages mutual exclusion).
-> Returns number of gadgets created.
-> ---------------------------------------------------------------------------
PROC gem_CreateRadioGadgets(tree:VALUE, win:VALUE) IS NATIVE {
  extern unsigned long gadtools_base;
  extern long gem_radio_gad[32];
  extern long gem_radio_obj[32];
  extern long gem_radio_count;
  extern long gem_radio_tree;
  extern long gem_radio_win;
  struct GEMObject { short next, head, tail; unsigned short flags, state, type; long spec; short x, y, w, h; };
  struct GEMObject *objtree = (struct GEMObject *)tree;
  struct Window *winp = (struct Window *)win;
  struct Gadget *last_gad = NULL;
  struct NewGadget ng;
  int i, count = 0;
  int in_radio_group = 0;
  if (!gadtools_base || !winp) return 0;
  gem_radio_count = 0;
  gem_radio_tree = (long)tree;
  gem_radio_win = (long)win;
  for (i = 0; objtree[i].type != 0 && count < 32; i++) {
    if (objtree[i].type == 21) { /* G_RBUTTON */
      ng.ng_LeftEdge  = objtree[i].x;
      ng.ng_TopEdge   = objtree[i].y;
      ng.ng_Width     = objtree[i].w;
      ng.ng_Height    = objtree[i].h;
      ng.ng_VisualInfo = GetVisualInfo(winp->WScreen, NULL);
      ng.ng_TextAttr   = &winp->WScreen->RastPort->Font->tf_Attr;
      ng.ng_Flags      = 0;
      ng.ng_GadgetText = (STRPTR)objtree[i].spec;
      ng.ng_GadgetID   = i;
      ng.ng_UserData   = (APTR)0;
      gem_radio_gad[count] = (long)CreateGadgetA(RADIO_KIND, last_gad, &ng, NULL);
      if (gem_radio_gad[count]) {
        gem_radio_obj[count] = i;
        /* If GEM object has SELECTED bit set, select this gadget */
        if (objtree[i].state & 1) {
          GT_SetGadgetAttrs((struct Gadget *)gem_radio_gad[count], winp, NULL,
            GTCB_Checked, 1, TAG_DONE);
        }
        last_gad = (struct Gadget *)gem_radio_gad[count];
        count++;
        in_radio_group = 1;
      }
    } else {
      in_radio_group = 0;
    }
  }
  gem_radio_count = count;
  /* Add gadget list to window */
  if (count > 0) {
    AddGList(winp, (struct Gadget *)gem_radio_gad[0], 0, -1, NULL);
    RefreshGList((struct Gadget *)gem_radio_gad[0], winp, NULL, -1);
    GT_RefreshWindow(winp, NULL);
  }
  return count;
} ENDNATIVE !!VALUE

-> ---------------------------------------------------------------------------
-> Remove radio gadgets from a window and free them
-> ---------------------------------------------------------------------------
PROC gem_FreeRadioGadgets() IS NATIVE {
  extern unsigned long gadtools_base;
  extern long gem_radio_gad[32];
  extern long gem_radio_count;
  extern long gem_radio_win;
  struct Window *winp = (struct Window *)gem_radio_win;
  int i;
  if (!gem_radio_count) return;
  if (winp) {
    RemoveGList(winp, (struct Gadget *)gem_radio_gad[0], gem_radio_count);
  }
  for (i = 0; i < gem_radio_count; i++) {
    if (gem_radio_gad[i]) {
      FreeGadgets((struct Gadget *)gem_radio_gad[i]);
      gem_radio_gad[i] = 0;
    }
  }
  gem_radio_count = 0;
} ENDNATIVE

-> ---------------------------------------------------------------------------
-> Get the GEM object index of the selected radio button in a group
-> Returns -1 if none selected
-> ---------------------------------------------------------------------------
PROC gem_GetRadioSelection() IS NATIVE {
  extern long gem_radio_gad[32];
  extern long gem_radio_obj[32];
  extern long gem_radio_count;
  extern long gem_radio_win;
  struct Window *winp = (struct Window *)gem_radio_win;
  int i;
  for (i = 0; i < gem_radio_count; i++) {
    if (gem_radio_gad[i]) {
      struct TagItem tags[2];
      long checked = 0;
      tags[0].ti_Tag = GTCB_Checked;
      tags[0].ti_Data = 0;
      tags[1].ti_Tag = TAG_DONE;
      tags[1].ti_Data = 0;
      GT_GetGadgetAttrs((struct Gadget *)gem_radio_gad[i], winp, NULL, tags);
      checked = tags[0].ti_Data;
      if (checked) return gem_radio_obj[i];
    }
  }
  return -1;
} ENDNATIVE !!INT

-> ---------------------------------------------------------------------------
-> Set a radio button's selected state
-> obj_index = GEM object index, selected = 1 to select, 0 to deselect
-> ---------------------------------------------------------------------------
PROC gem_SetRadioState(obj_index:VALUE, selected:VALUE) IS NATIVE {
  extern long gem_radio_gad[32];
  extern long gem_radio_obj[32];
  extern long gem_radio_count;
  extern long gem_radio_win;
  struct Window *winp = (struct Window *)gem_radio_win;
  int i;
  for (i = 0; i < gem_radio_count; i++) {
    if (gem_radio_gad[i] && gem_radio_obj[i] == (long)obj_index) {
      GT_SetGadgetAttrs((struct Gadget *)gem_radio_gad[i], winp, NULL,
        GTCB_Checked, (long)selected, TAG_DONE);
      return;
    }
  }
} ENDNATIVE

-> ---------------------------------------------------------------------------
-> Handle a radio gadget IDCMP event - returns the GEM object index
-> of the clicked radio button, or -1 if not a radio event
-> ---------------------------------------------------------------------------
PROC gem_HandleRadioEvent(code:VALUE) IS NATIVE {
  extern long gem_radio_gad[32];
  extern long gem_radio_obj[32];
  extern long gem_radio_count;
  struct Gadget *gad = (struct Gadget *)((long)code);
  int i;
  for (i = 0; i < gem_radio_count; i++) {
    if ((long)gem_radio_gad[i] == (long)gad) {
      return gem_radio_obj[i];
    }
  }
  return -1;
} ENDNATIVE !!INT

-> ---------------------------------------------------------------------------
-> Scan a GEM object tree and update GEM state bits from gadtools gadget
-> selection.  Deselects all other radio buttons in each group.
-> ---------------------------------------------------------------------------
PROC gem_SyncRadioToGEM(tree:VALUE) IS NATIVE {
  extern long gem_radio_gad[32];
  extern long gem_radio_obj[32];
  extern long gem_radio_count;
  extern long gem_radio_win;
  struct GEMObject { short next, head, tail; unsigned short flags, state, type; long spec; short x, y, w, h; };
  struct GEMObject *objtree = (struct GEMObject *)tree;
  struct Window *winp = (struct Window *)gem_radio_win;
  int i, selected_obj = -1;
  long checked;
  struct TagItem tags[2];
  /* First pass: find which radio button is currently selected in gadtools */
  for (i = 0; i < gem_radio_count; i++) {
    if (gem_radio_gad[i]) {
      tags[0].ti_Tag = GTCB_Checked;
      tags[0].ti_Data = 0;
      tags[1].ti_Tag = TAG_DONE;
      tags[1].ti_Data = 0;
      GT_GetGadgetAttrs((struct Gadget *)gem_radio_gad[i], winp, NULL, tags);
      checked = tags[0].ti_Data;
      if (checked) selected_obj = gem_radio_obj[i];
    }
  }
  /* Second pass: clear SELECTED bit on all radio objects, set it on the winner */
  for (i = 0; i < gem_radio_count; i++) {
    int obj_idx = gem_radio_obj[i];
    if (obj_idx >= 0) {
      if (obj_idx == selected_obj)
        objtree[obj_idx].state |= 1;   /* set SELECTED */
      else
        objtree[obj_idx].state &= ~1;  /* clear SELECTED */
    }
  }
} ENDNATIVE

PROC gem_TextFontInit16(f:PTR TO textfont) IS NATIVE { struct TextFont *tf = (struct TextFont *)gem_font_8x16; tf->tf_Message.mn_ReplyPort=NULL; tf->tf_Message.mn_Length=sizeof(struct TextFont); tf->tf_YSize=16; tf->tf_Style=0; tf->tf_Flags=0; tf->tf_XSize=8; tf->tf_Baseline=13; tf->tf_BoldSmear=0; tf->tf_Accessors=0; tf->tf_LoChar=0; tf->tf_HiChar=255; tf->tf_CharData=(APTR)gem_font_data_8x16; tf->tf_Modulo=8; tf->tf_CharLoc=(APTR)gem_font_loc_8x16; tf->tf_CharSpace=(APTR)gem_font_width_8x16; tf->tf_CharKern=NULL; } ENDNATIVE
PROC gem_TextFontInit8(f:PTR TO textfont) IS NATIVE { struct TextFont *tf = (struct TextFont *)gem_font_8x8; tf->tf_Message.mn_ReplyPort=NULL; tf->tf_Message.mn_Length=sizeof(struct TextFont); tf->tf_YSize=8; tf->tf_Style=0; tf->tf_Flags=0; tf->tf_XSize=8; tf->tf_Baseline=7; tf->tf_BoldSmear=0; tf->tf_Accessors=0; tf->tf_LoChar=0; tf->tf_HiChar=255; tf->tf_CharData=(APTR)gem_font_data_8x8; tf->tf_Modulo=8; tf->tf_CharLoc=(APTR)gem_font_loc_8x8; tf->tf_CharSpace=(APTR)gem_font_width_8x8; tf->tf_CharKern=NULL; } ENDNATIVE

PROC gem_CloseLibrary(base:VALUE) IS NATIVE { CloseLibrary((struct Library *)} base {); } ENDNATIVE

-> Type cast helpers (avoid expr !!type in code, keep only in NATIVE return annotations)
PROC asINT(v:VALUE) IS NATIVE { return (int)v; } ENDNATIVE !!INT
PROC asLONG(v:VALUE) IS NATIVE { return (long)v; } ENDNATIVE !!LONG
PROC asCHAR(v:VALUE) IS NATIVE { return (unsigned char)v; } ENDNATIVE !!CHAR
PROC asPTR(v:VALUE) IS NATIVE { return (unsigned char*)v; } ENDNATIVE !!PTR TO CHAR
PROC asWIN(v:VALUE) IS NATIVE { return (struct Window*)v; } ENDNATIVE !!PTR TO window
PROC asBPTR(v:VALUE) IS NATIVE { return (BPTR)v; } ENDNATIVE !!BPTR
PROC asAPTR(v:VALUE) IS NATIVE { return (APTR)v; } ENDNATIVE !!APTR
PROC asFONT(v:VALUE) IS NATIVE { return (struct TextFont*)v; } ENDNATIVE !!PTR TO textfont

-> Intuition window helper wrappers
PROC gem_CloseWindow(win:PTR TO window) IS NATIVE { CloseWindow((struct Window *)} win {); } ENDNATIVE
PROC gem_HideWindow(win:PTR TO window) IS NATIVE { HideWindow((struct Window *)} win {); } ENDNATIVE
PROC gem_ShowWindow(win:PTR TO window) IS NATIVE { ShowWindow((struct Window *)} win {); } ENDNATIVE
PROC gem_WindowTitle(win:PTR TO window, title:PTR TO CHAR) IS NATIVE { WindowTitle((struct Window *)} win {, (STRPTR)} title {); } ENDNATIVE
PROC gem_MoveWindow(win:PTR TO window, x:VALUE, y:VALUE) IS NATIVE { MoveWindow((struct Window *)} win {, (long)} x {, (long)} y {); } ENDNATIVE
PROC gem_SizeWindow(win:PTR TO window, w:VALUE, h:VALUE) IS NATIVE { SizeWindow((struct Window *)} win {, (long)} w {, (long)} h {); } ENDNATIVE

PROC gem_OpenWindow(x:VALUE, y:VALUE, w:VALUE, h:VALUE, title:PTR TO CHAR, kind:VALUE) IS NATIVE { struct Window *win; struct NewWindow nw; long flags = WFLG_SMART_REFRESH | WFLG_ACTIVATE | WFLG_GIMMEZEROZERO; long idcmp = IDCMP_CLOSEWINDOW | IDCMP_REFRESHWINDOW | IDCMP_SIZEVERIFY | IDCMP_NEWSIZE | IDCMP_MOUSEBUTTONS | IDCMP_MOUSEMOVE | IDCMP_MENUPICK; if (kind & 2) flags |= WFLG_CLOSEGADGET; if (kind & 4) flags |= WFLG_DEPTHGADGET; if (kind & 8) flags |= WFLG_DRAGBAR; if (kind & 16) flags |= WFLG_SIZEGADGET; nw.LeftEdge = (long)} x {; nw.TopEdge = (long)} y {; nw.Width = (long)} w {; nw.Height = (long)} h {; nw.DetailPen = 0; nw.BlockPen = 1; nw.Title = (STRPTR)} title {; nw.Flags = flags; nw.IDCMPFlags = idcmp; nw.Type = WBENCHSCREEN; nw.FirstGadget = NULL; nw.CheckMark = NULL; nw.Screen = NULL; nw.BitMap = NULL; nw.MinWidth = 50; nw.MinHeight = 30; nw.MaxWidth = 2048; nw.MaxHeight = 2048; win = OpenWindow(&nw); return (unsigned long)win; } ENDNATIVE !!VALUE

-> ---------------------------------------------------------------------------
-> MENUPICK poll: check all open windows for IDCMP_MENUPICK and translate
-> to GEM MN_SELECTED messages pushed into the message queue.
-> ---------------------------------------------------------------------------
PROC gem_poll_menupick()
  DEF i
  FOR i := 0 TO 15
    IF gem_window_list[i] <> 0
      gem_poll_menupick_window(i)
    ENDIF
  ENDFOR
ENDPROC

PROC gem_poll_menupick_window(widx)
  DEF code, menunum, itemnum, cur_menu, cur_item, obj_idx
  DEF menu_idx, gem_tree, dest_id, base, tail, wind_h
  DEF msg_buf[16]:ARRAY OF VALUE
  DEF obj_next

  NATIVE {
    struct Window *win = (struct Window *)gem_window_list[widx];
    struct IntuiMessage *msg;
    if (!win || !win->UserPort) return;
    while ((msg = (struct IntuiMessage *)GetMsg(win->UserPort))) {
      if (msg->Class == IDCMP_MENUPICK) {
        UWORD code = msg->Code;
        if (code == MENUNUM(255) || code == 0xFFFF) {
          ReplyMsg((struct Message *)msg);
          continue;
        }
        {
          int menunum = MENUNUM(code);
          int itemnum = ITEMNUM(code);
          int menu_idx = gem_wind_gem_menu_idx[widx];
          int gem_tree = (menu_idx >= 0) ? gem_menu_tree[menu_idx] : 0;
          int obj_idx = 0, cur_menu = 0, cur_item = 0, j;
          int dest_id = (menu_idx >= 0) ? gem_menu_owner[menu_idx] : 0;
          int wind_h = gem_wind_handle[widx];
          if (gem_tree) {
            struct GEMObject { short next, head, tail; unsigned short flags, state, type; long spec; short x, y, w, h; };
            struct GEMObject *obj = (struct GEMObject *)gem_tree;
            for (j = 0; obj[j].type != 0; j++) {
              if (obj[j].type == 25) { /* G_TITLE */
                if (cur_menu == menunum) cur_item = 0;
                cur_menu++;
              } else if (obj[j].type == 26) { /* G_STRING */
                if (cur_menu - 1 == menunum) {
                  if (cur_item == itemnum) { obj_idx = j; break; }
                  cur_item++;
                }
              }
            }
          }
          /* Build 16-word GEM MN_SELECTED message */
          {
            int tail = gem_msg_tail;
            /* Clear message buffer */
            for (j = 0; j < 16; j++) msg_buf[j] = 0;
            msg_buf[0] = 10;                    /* MN_SELECTED */
            msg_buf[1] = dest_id;               /* dest app ID (stored as-is) */
            msg_buf[2] = wind_h;                /* window handle */
            msg_buf[8] = gem_tree & 0xFFFF;     /* tree pointer low word */
            msg_buf[9] = (gem_tree >> 16) & 0xFFFF; /* tree pointer high word */
            msg_buf[10] = obj_idx;              /* item index */
            /* Store into queue */
            gem_msg_queue[tail] = 10;
            gem_msg_src[tail] = dest_id;
            gem_msg_len[tail] = 8;
            base = tail * 16;
            for (j = 0; j < 16; j++) gem_msg_data16[base + j] = msg_buf[j];
            tail++;
            if (tail >= 32) tail = 0;
            gem_msg_tail = tail;
          }
        }
        ReplyMsg((struct Message *)msg);
      } else if (msg->Class == IDCMP_CLOSEWINDOW) {
        ReplyMsg((struct Message *)msg);
      } else if (msg->Class == IDCMP_NEWSIZE) {
        ReplyMsg((struct Message *)msg);
      } else if (msg->Class == IDCMP_MOUSEBUTTONS) {
        ReplyMsg((struct Message *)msg);
      } else {
        ReplyMsg((struct Message *)msg);
      }
    }
  } ENDNATIVE
ENDPROC

-> Draw a filled rectangle on the first open window's RastPort
PROC gem_DrawRect(x:VALUE, y:VALUE, w:VALUE, h:VALUE, filled:VALUE) IS NATIVE { struct Window *win; int i; for (i = 0; i < 16; i++) { win = (struct Window *)gem_window_list[i]; if (win) break; } if (!win) return; if (filled) { SetAPen(win->RPort, 0); RectFill(win->RPort, (long)} x {, (long)} y {, (long)(} x { + } w { - 1), (long)(} y { + } h { - 1)); } else { SetAPen(win->RPort, 1); Draw(win->RPort, (long)} x {, (long)} y {); Draw(win->RPort, (long)(} x { + } w {), (long)} y {); Draw(win->RPort, (long)(} x { + } w {), (long)(} y { + } h {)); Draw(win->RPort, (long)} x {, (long)(} y { + } h {)); Draw(win->RPort, (long)} x {, (long)} y {); } } ENDNATIVE

-> ---------------------------------------------------------------------------
-> Open gadtools.library with fallback to gadtools13.library
-> Returns 1 on success, 0 on failure
-> ---------------------------------------------------------------------------
PROC gem_init_gadtools()
  gadtools_base := 0
  gem_OpenGadTools()
  IF gadtools_base = 0
    gem_OpenGadTools13()
  ENDIF
ENDPROC (gadtools_base <> 0)

PROC gem_OpenGadTools() IS NATIVE { extern unsigned long gadtools_base; gadtools_base = (unsigned long)OpenLibrary("gadtools.library", 0); } ENDNATIVE
PROC gem_OpenGadTools13() IS NATIVE { extern unsigned long gadtools_base; gadtools_base = (unsigned long)OpenLibrary("gadtools13.library", 0); } ENDNATIVE

PROC gem_OpenReqTools() IS NATIVE { extern unsigned long reqtools_base; reqtools_base = (unsigned long)OpenLibrary("reqtools.library", 39); } ENDNATIVE
PROC gem_FreeReqTools() IS NATIVE { extern unsigned long reqtools_base; if (reqtools_base) { CloseLibrary((struct Library *)reqtools_base); reqtools_base = 0; } } ENDNATIVE


-> ---------------------------------------------------------------------------
-> Cookie Jar - Atari ST cookie jar emulation
-> ---------------------------------------------------------------------------
-> The cookie jar is a table of { LONG id; LONG value; } pairs in memory,
-> terminated by a NULL cookie whose value = max capacity.
-> Layout: COOKIE[0] .. COOKIE[N-1], COOKIE[N] = {0, max_entries}

CONST COOKIE_SIZE = 8, COOKIE_JAR_MAX = 64

-> Initialize the cookie jar with standard entries
PROC gem_init_cookie_jar()
  DEF jar_ptr
  gem_cookie_max := COOKIE_JAR_MAX
  gem_cookie_count := 0
  -> Allocate cookie jar: (max+1) entries * 8 bytes each (extra for sentinel)
  jar_ptr := asLONG(AllocMem((gem_cookie_max + 1) * COOKIE_SIZE, MEMF_CLEAR))
  IF jar_ptr = 0
    gem_cookie_jar := 0
    ENDPROC
  ENDIF
  gem_cookie_jar := jar_ptr
  -> Set sentinel: NULL id = 0, value = max capacity
  gem_cookie_set_sentinel()
  -> Add standard Atari ST cookies
  -> _CPU: CPU type (0 = 68000)
  gem_cookie_add('_CPU', 0)
  -> _VDO: video system (1 = ST)
  gem_cookie_add('_VDO', 1)
  -> _MCH: machine type (0 = ST)
  gem_cookie_add('_MCH', 0)
  -> _FPU: FPU type (0 = none)
  gem_cookie_add('_FPU', 0)
  -> Sake emulator cookie
  gem_cookie_add('Sake', 1)
ENDPROC

-> Set the sentinel entry (marks end of jar and stores max capacity)
PROC gem_cookie_set_sentinel()
  DEF base, sentinel
  IF gem_cookie_jar = 0 THEN ENDPROC
  base := gem_cookie_jar + gem_cookie_count * COOKIE_SIZE
  NATIVE {
    long *p = (long *)base;
    p[0] = 0;            /* id = NULL */
    p[1] = gem_cookie_max; /* value = max entries */
  } ENDNATIVE
ENDPROC

-> Find a cookie by 4-byte id. Returns index or -1
PROC gem_cookie_find(id_str:PTR TO CHAR)
  DEF i, base, id_val
  IF gem_cookie_jar = 0 THEN ENDPROC -1
  -> Convert 4-char id to a long for comparison
  NATIVE {
    char *s = (char *)id_str;
    id_val = (s[0]<<24) | (s[1]<<16) | (s[2]<<8) | s[3];
  } ENDNATIVE
  FOR i := 0 TO gem_cookie_count - 1
    base := gem_cookie_jar + i * COOKIE_SIZE
    NATIVE {
      long *entry = (long *)base;
      if (entry[0] == id_val) {
        gem_cookie_find_result = i;
        return;
      }
    } ENDNATIVE
  ENDFOR
ENDPROC -1

DEF gem_cookie_find_result

-> Add a cookie. Returns 1 on success, 0 if jar is full
PROC gem_cookie_add(id_str:PTR TO CHAR, value:VALUE)
  DEF base, id_val
  IF gem_cookie_jar = 0 THEN ENDPROC 0
  IF gem_cookie_count >= gem_cookie_max THEN ENDPROC 0
  NATIVE {
    char *s = (char *)id_str;
    id_val = (s[0]<<24) | (s[1]<<16) | (s[2]<<8) | s[3];
  } ENDNATIVE
  base := gem_cookie_jar + gem_cookie_count * COOKIE_SIZE
  NATIVE {
    long *entry = (long *)base;
    entry[0] = id_val;
    entry[1] = value;
  } ENDNATIVE
  gem_cookie_count := gem_cookie_count + 1
  gem_cookie_set_sentinel()
ENDPROC 1

-> Get a cookie value. Returns 1 if found (value via ptr), 0 if not
PROC gem_cookie_get(id_str:PTR TO CHAR, value_ptr)
  DEF idx, base
  idx := gem_cookie_find(id_str)
  IF idx < 0 THEN ENDPROC 0
  base := gem_cookie_jar + idx * COOKIE_SIZE
  NATIVE {
    long *entry = (long *)base;
    *(long *)value_ptr = entry[1];
  } ENDNATIVE
ENDPROC 1

-> Remove a cookie by id. Returns 1 if removed, 0 if not found
PROC gem_cookie_remove(id_str:PTR TO CHAR)
  DEF idx, i, base, next_base
  idx := gem_cookie_find(id_str)
  IF idx < 0 THEN ENDPROC 0
  -> Shift all following entries forward by one COOKIE_SIZE
  FOR i := idx TO gem_cookie_count - 2
    base := gem_cookie_jar + i * COOKIE_SIZE
    next_base := base + COOKIE_SIZE
    NATIVE {
      long *dst = (long *)base;
      long *src = (long *)next_base;
      dst[0] = src[0];
      dst[1] = src[1];
    } ENDNATIVE
  ENDFOR
  gem_cookie_count := gem_cookie_count - 1
  gem_cookie_set_sentinel()
ENDPROC 1


-> AES global arrays (as per GEM AES parameter block spec)
DEF gem_control[12]:ARRAY OF VALUE -> control array
DEF gem_intin[128]:ARRAY OF VALUE
DEF gem_intout[128]:ARRAY OF VALUE
DEF gem_ptrin[16]:ARRAY OF VALUE
DEF gem_pttout[16]:ARRAY OF VALUE

-> Find a free window slot, returns handle or -1 via ctx[0]
PROC gem_wind_alloc()
  DEF i, result
  result := -1
  FOR i := 0 TO MAX_WINDOWS - 1
    IF gem_wind_state[i] = WS_CLOSED AND gem_wind_handle[i] = 0
      gem_wind_handle[i] := i + 1
      gem_wind_state[i] := WS_CLOSED
      result := gem_wind_handle[i]
      i := MAX_WINDOWS
    ENDIF
  ENDFOR
  ctx[0] := result
ENDPROC

-> Find window slot by handle, returns index via ENDPROC
PROC gem_wind_find_handle(h)
  DEF i, result
  result := -1
  FOR i := 0 TO MAX_WINDOWS - 1
    IF result = -1 AND gem_wind_handle[i] = h
      result := i
    ENDIF
  ENDFOR
ENDPROC result

-> Push a message onto the message queue
PROC gem_msg_push(msg, src, len)
  gem_msg_queue[gem_msg_tail] := msg
  gem_msg_src[gem_msg_tail] := src
  gem_msg_len[gem_msg_tail] := len
  gem_msg_tail := gem_msg_tail + 1
  IF gem_msg_tail >= MAX_MESSAGES
    gem_msg_tail := 0
  ENDIF
ENDPROC

-> Copy 16 words of message data into the slot at index
PROC gem_msg_copy_data(idx, src_ptr)
  DEF i, base
  base := idx * 16
  FOR i := 0 TO 15
    gem_msg_data16[base + i] := src_ptr[i]
  ENDFOR
ENDPROC

-> Write a 16-word GEM message into a slot. data is an array of 16 VALUES.
PROC gem_msg_write_slot(slot, src_id, data:ARRAY OF VALUE)
  DEF base, i
  gem_msg_queue[slot] := data[0]
  gem_msg_src[slot] := src_id
  gem_msg_len[slot] := 8
  base := slot * 16
  FOR i := 0 TO 15
    gem_msg_data16[base + i] := data[i]
  ENDFOR
ENDPROC

-> Return bitmask with bit 'n' set (n=0..15)
PROC gem_bit(n)
  DEF bits[16]:ARRAY OF VALUE, result
  bits[0] := 1; bits[1] := 2; bits[2] := 4; bits[3] := 8
  bits[4] := 16; bits[5] := 32; bits[6] := 64; bits[7] := 128
  bits[8] := 256; bits[9] := 512; bits[10] := 1024; bits[11] := 2048
  bits[12] := 4096; bits[13] := 8192; bits[14] := 16384; bits[15] := 32768
  result := 0
  IF n >= 0 AND n <= 15
    result := bits[n]
  ENDIF
ENDPROC result

-> Pop a message from the queue
PROC gem_msg_pop()
  IF gem_msg_head <> gem_msg_tail
    ctx[0] := gem_msg_queue[gem_msg_head]
    ctx[1] := gem_msg_src[gem_msg_head]
    ctx[2] := gem_msg_len[gem_msg_head]
    gem_msg_head := gem_msg_head + 1
    IF gem_msg_head >= MAX_MESSAGES
      gem_msg_head := 0
    ENDIF
  ELSE
    ctx[0] := 0
    ctx[1] := 0
    ctx[2] := 0
  ENDIF
ENDPROC

-> Copy the 16-word message data from the popped message slot to a buffer pointer
PROC gem_msg_copy_to_buf(buf_ptr)
  DEF base, i
  base := (gem_msg_head - 1) * 16
  IF gem_msg_head = 0
    base := (MAX_MESSAGES - 1) * 16
  ENDIF
  NATIVE {
    short *dst = (short *)buf_ptr;
    int i;
    for (i = 0; i < 16; i++) {
      dst[i] = (short)gem_msg_data16[base + i];
    }
  } ENDNATIVE
ENDPROC


-> ---------------------------------------------------------------------------
-> AES dispatch
-> ---------------------------------------------------------------------------
PROC gem_aes_dispatch()
  DEF fn_group, fn_sub

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

  CASE AES_RSRC
    SELECT fn_sub
    CASE 0 -> gem_rsrc_load()
    CASE 1 -> gem_rsrc_free()
    CASE 2 -> gem_rsrc_gaddr()
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


-> ---------------------------------------------------------------------------
-> GEM AES function implementations
-> Each group implements the GEM AES spec mapped to AmigaOS Intuition
-> ---------------------------------------------------------------------------

-> ============================
-> APPL - Application services
-> ============================

PROC gem_appl_init()
  DEF ap_id
  gem_aes_id := gem_aes_id + 1
  ap_id := gem_aes_id
  IF gem_app_count < MAX_APPS
    gem_app_list[gem_app_count] := ap_id
    gem_app_count := gem_app_count + 1
  ENDIF
  ctx[0] := ap_id
ENDPROC

PROC gem_appl_exit()
  DEF i, id
  id := gem_aes_id
  FOR i := 0 TO gem_app_count - 1
    IF gem_app_list[i] = id
      gem_app_list[i] := gem_app_list[gem_app_count - 1]
      gem_app_count := gem_app_count - 1
      i := gem_app_count
    ENDIF
  ENDFOR
  gem_aes_id := 0
  ctx[0] := 1
ENDPROC

PROC gem_appl_read()
  DEF mg_timeout
  mg_timeout := ctx[3]
  gem_msg_pop()
ENDPROC

PROC gem_appl_write()
  DEF dest_id, msg_len, msg_data, next_tail
  dest_id := ctx[1]
  msg_len := ctx[2]
  -> Check if queue has space (not full)
  next_tail := gem_msg_tail + 1
  IF next_tail >= MAX_MESSAGES
    next_tail := 0
  ENDIF
  IF next_tail <> gem_msg_head
    gem_msg_push((dest_id * 256) + msg_len, gem_aes_id, msg_len)
    ctx[0] := 1
  ELSE
    ctx[0] := 0
  ENDIF
ENDPROC

PROC gem_appl_find()
  -> Search known applications by name (parm on addr stack)
  -> Without a real app registry, return -1 (not found)
  ctx[0] := -1
ENDPROC


-> ============================
-> EVNT - Event services
-> ============================

PROC gem_evnt_multi()
  DEF flags, bclk, bmsk, bst
  DEF m1flags, m1x, m1y, m1w, m1h
  DEF m2flags, m2x, m2y, m2w, m2h
  DEF timer, mg_time, messag
  DEF i, result, done

  flags := ctx[3]; bclk := ctx[4]; bmsk := ctx[5]; bst := ctx[6]
  m1flags := ctx[7]; m1x := ctx[8]; m1y := ctx[9]; m1w := ctx[10]; m1h := ctx[11]
  m2flags := ctx[12]; m2x := ctx[13]; m2y := ctx[14]; m2w := ctx[15]; m2h := ctx[16]; mg_time := ctx[18]; messag := ctx[19]

  result := 0
  done := 0

  -> Poll for real Intuition MENUPICK events first (puts into message queue)
  gem_poll_menupick()

  -> Check message queue first (highest priority)
  IF done = 0 AND gem_msg_head <> gem_msg_tail
    gem_msg_pop()
    -> Copy the full 16-word message data to the app's message buffer
    IF messag <> 0
      gem_msg_copy_to_buf(messag)
    ENDIF
    result := 3
    done := 1
  ENDIF

  -> Check button event
  IF done = 0 AND (flags AND 1) <> 0
    result := 1
    ctx[1] := 2
    ctx[2] := 320
    ctx[3] := 200
    ctx[4] := 320
    ctx[5] := 200
    ctx[6] := 0
    ctx[7] := 0
    done := 1
  ENDIF

  -> Check timer
  IF done = 0 AND (flags AND 16) <> 0
    result := 3
    done := 1
  ENDIF

  -> Check mouse rectangle 1
  IF done = 0 AND (flags AND 32) <> 0
  ENDIF

  -> Default: no event or message
  ctx[0] := result
ENDPROC

PROC gem_evnt_mesag()
  DEF msgbuf
  msgbuf := ctx[8]
  gem_msg_pop()
  IF msgbuf <> 0
    gem_msg_copy_to_buf(msgbuf)
  ENDIF
ENDPROC

PROC gem_evnt_button()
  DEF flag, bclk, bmsk, bst
  flag := ctx[3]
  bclk := ctx[4]
  bmsk := ctx[5]
  bst := ctx[6]
  -> Return immediate button event
  ctx[0] := 1
  ctx[1] := 2    -> double-click (unused)
  ctx[2] := 320  -> x
  ctx[3] := 200  -> y
ENDPROC

PROC gem_evnt_mouse()
  DEF flag, mx, my, mw, mh
  flag := ctx[3]
  mx := ctx[4]; my := ctx[5]; mw := ctx[6]; mh := ctx[7]
  -> Assume mouse is in rectangle
  ctx[0] := 1
  ctx[1] := 320
  ctx[2] := 200
ENDPROC

PROC gem_evnt_keybd()
  -> Return no key event
  ctx[0] := 0
  ctx[1] := 0  -> keycode
  ctx[2] := 0  -> shift state
ENDPROC

PROC gem_evnt_dclick()
  DEF new_dclick, set_flag
  set_flag := ctx[3]
  new_dclick := ctx[4]
  -> If set_flag is 1, set the new double-click rate; return old
  ctx[0] := 5
ENDPROC

PROC gem_evnt_timer()
  -> Return immediately (0 = timer expired)
  ctx[0] := 0
ENDPROC


-> ============================
-> RSRC - Resource services
-> ============================

-> rsrc_load() - Load a .RSC resource file into memory
-> filename in addr_in[0] (ctx[8])
PROC gem_rsrc_load()
  DEF fn:PTR TO CHAR
  fn := asPTR(ctx[8])
  gem_rsc_data := 0
  gem_rsc_size := 0
  gem_rsc_tree_count := 0
  gem_rsc_tree_table := 0
  NATIVE {
    BPTR fh;
    unsigned char *data;
    int size;
    unsigned short *hdr;
    int num_objects, num_trees, num_ti, num_ib, num_bb, num_frstr, num_frimg;
    int obj_off, ti_off, ib_off, bb_off, frstr_off, frimg_off, trix_off, trix_ntree;
    int obj_size, ti_size, i;
    unsigned long *tree_ptrs;
    unsigned short *trix;
    fh = Open((STRPTR)fn, MODE_OLDFILE);
    if (!fh) { gem_rsc_data = 0; return; }
    size = Seek(fh, 0, OFFSET_END);
    Seek(fh, 0, OFFSET_BEGINNING);
    data = AllocMem(size, MEMF_CLEAR);
    if (!data) { Close(fh); gem_rsc_data = 0; return; }
    Read(fh, data, size);
    Close(fh);
    hdr = (unsigned short *)data;
    num_objects = hdr[1]; num_trees = hdr[2]; num_ti = hdr[4];
    num_ib = hdr[6]; num_bb = hdr[7]; num_frstr = hdr[8]; num_frimg = hdr[9];
    obj_off = hdr[10]; ti_off = hdr[11]; ib_off = hdr[12];
    bb_off = hdr[13]; frstr_off = hdr[14]; frimg_off = hdr[15];
    trix_off = hdr[16]; trix_ntree = hdr[17];
    if (trix_ntree > 0) num_trees = trix_ntree;
    if (num_trees <= 0) { num_trees = num_objects > 0 ? 1 : 0; }
    if (num_objects > 0 && ti_off > obj_off) obj_size = (ti_off - obj_off) / num_objects;
    else obj_size = 24;
    if (num_ti > 0 && ib_off > ti_off) ti_size = (ib_off - ti_off) / num_ti;
    else ti_size = 0;
    for (i = 0; i < num_objects; i++) {
      unsigned char *obj = data + obj_off + i * obj_size;
      unsigned short ot = *(unsigned short *)(obj + 6);
      unsigned short spec = *(unsigned short *)(obj + 20);
      if (spec > 0 && spec < size) {
        if ((ot >= 8 && ot <= 13) || ot == 4 || ot == 5 || ot == 6)
          *(unsigned long *)(obj + 20) = (unsigned long)(data + spec);
      }
    }
    if (ti_size > 0) {
      for (i = 0; i < num_ti; i++) {
        unsigned char *ti = data + ti_off + i * ti_size;
        unsigned short p1, p2, p3;
        p1 = *(unsigned short *)(ti + 0);
        p2 = *(unsigned short *)(ti + 2);
        p3 = *(unsigned short *)(ti + 4);
        if (p1 > 0 && p1 < size) *(unsigned long *)(ti + 0) = (unsigned long)(data + p1);
        if (p2 > 0 && p2 < size) *(unsigned long *)(ti + 2) = (unsigned long)(data + p2);
        if (p3 > 0 && p3 < size) *(unsigned long *)(ti + 4) = (unsigned long)(data + p3);
      }
    }
    trix = data + trix_off;
    tree_ptrs = AllocMem(num_trees * 4, MEMF_CLEAR);
    if (tree_ptrs) {
      for (i = 0; i < num_trees; i++) {
        unsigned short root_off = (trix_off > 0 && (unsigned long)(trix + i) < (unsigned long)(data + size)) ? trix[i] : (obj_off + i * obj_size);
        if (root_off > 0 && (unsigned long)(data + root_off) < (unsigned long)(data + size))
          tree_ptrs[i] = (unsigned long)(data + root_off);
        else
          tree_ptrs[i] = 0;
      }
    }
    gem_rsc_data = (unsigned long)data;
    gem_rsc_size = size;
    gem_rsc_tree_count = num_trees;
    gem_rsc_tree_table = (unsigned long)tree_ptrs;
  } ENDNATIVE
  IF gem_rsc_data <> 0
    ctx[0] := E_OK
  ELSE
    ctx[0] := E_ERROR
  ENDIF
ENDPROC

-> rsrc_free() - Free loaded resource data
PROC gem_rsrc_free()
  NATIVE {
    if (gem_rsc_data) FreeMem((void *)gem_rsc_data, gem_rsc_size);
    if (gem_rsc_tree_table) FreeMem((void *)gem_rsc_tree_table, gem_rsc_tree_count * 4);
  } ENDNATIVE
  gem_rsc_data := 0
  gem_rsc_size := 0
  gem_rsc_tree_count := 0
  gem_rsc_tree_table := 0
  ctx[0] := E_OK
ENDPROC

-> rsrc_gaddr() - Get address of resource object
-> type in int_in[0] (ctx[3]): 0=R_TREE, 1=R_OBJECT, 2=R_TEDINFO, 3=R_ICONBLK, 4=R_BITBLK, 5=R_FRSTR, 6=R_FRIMG, 7=R_USERBLK
-> index in int_in[1] (ctx[4])
-> address returned in addr_out[0]
PROC gem_rsrc_gaddr()
  DEF gtype, gidx, result, obj_ptr
  gtype := ctx[3]
  gidx := ctx[4]
  result := 0
  obj_ptr := 0
  IF gem_rsc_data <> 0
    IF gtype = 0
      IF gidx >= 0 AND gidx < gem_rsc_tree_count AND gem_rsc_tree_table <> 0
        NATIVE {
          unsigned long *tptrs = (unsigned long *)gem_rsc_tree_table;
          gem_rsrc_gaddr_result = (unsigned long)tptrs[gidx];
        } ENDNATIVE
        obj_ptr := gem_rsrc_gaddr_result
        IF obj_ptr <> 0 THEN result := 1
      ENDIF
    ELSE
      IF gtype = 1
        -> R_OBJECT: index is global object index within the RSC
        -> For now, calculate from tree table (tree 0 object index)
        IF gidx >= 0 AND gem_rsc_tree_table <> 0
          NATIVE {
            unsigned long *tptrs = (unsigned long *)gem_rsc_tree_table;
            gem_rsrc_gaddr_result = tptrs[0];
          } ENDNATIVE
          obj_ptr := gem_rsrc_gaddr_result
          IF obj_ptr <> 0
            NATIVE {
              unsigned char *obj = (unsigned char *)obj_ptr;
              unsigned short next = *(unsigned short *)(obj + 0);
              if (next != 0) gem_rsrc_gaddr_result = (unsigned long)(obj + next);
              else gem_rsrc_gaddr_result = 0;
            } ENDNATIVE
            obj_ptr := gem_rsrc_gaddr_result
          ENDIF
          result := 1
        ENDIF
      ENDIF
    ENDIF
  ENDIF
  ctx[1] := obj_ptr
  ctx[0] := result
ENDPROC

DEF gem_rsrc_gaddr_result

-> ============================
-> MENU - Menu services
-> ============================

-> Find the first open window (used for menu attachment)
PROC gem_find_first_window()
  DEF i, result
  result := -1
  FOR i := 0 TO 15
    IF gem_wind_state[i] = WS_OPEN AND gem_window_list[i] <> 0
      result := i
      i := 16
    ENDIF
  ENDFOR
ENDPROC result

-> menu_bar(tree, show) - Show or hide the menu bar
PROC gem_menu_bar()
  DEF tree, show, i, widx, nm, amiga_m
  DEF menu_idx
  tree := ctx[3]
  show := ctx[4]
  IF show
    -> Find which menu slot owns this tree
    menu_idx := -1
    FOR i := 0 TO gem_menu_count - 1
      IF gem_menu_tree[i] = tree
        menu_idx := i
        i := gem_menu_count
      ENDIF
    ENDFOR
    -> Attach to first open window
    widx := gem_find_first_window()
    IF widx >= 0 AND menu_idx >= 0
      -> Free any previous menu on this window
      IF gem_wind_amiga_menu[widx] <> 0
        gem_ClearMenuStrip(asWIN(gem_window_list[widx]))
        gem_FreeMenus(gem_wind_amiga_menu[widx])
        gem_wind_amiga_menu[widx] := 0
      ENDIF
      IF gem_wind_newmenu[widx] <> 0
        gem_FreeNewMenu(gem_wind_newmenu[widx])
        gem_wind_newmenu[widx] := 0
      ENDIF
      -> Convert GEM tree to NewMenu
      nm := gem_CreateMenusFromTree(tree)
      IF nm <> 0
        amiga_m := gem_CreateMenus(nm)
        IF amiga_m <> 0
          gem_LayoutMenus(amiga_m)
          gem_SetMenuStrip(asWIN(gem_window_list[widx]), amiga_m)
          gem_wind_amiga_menu[widx] := amiga_m
          gem_wind_newmenu[widx] := nm
          gem_wind_gem_menu_idx[widx] := menu_idx
        ELSE
          gem_FreeNewMenu(nm)
        ENDIF
      ENDIF
    ENDIF
    gem_menu_bar_visible := 1
    gem_menu_active_app := gem_aes_id
  ELSE
    -> Hide menus: detach from all windows
    FOR i := 0 TO 15
      IF gem_wind_amiga_menu[i] <> 0 AND gem_window_list[i] <> 0
        gem_ClearMenuStrip(asWIN(gem_window_list[i]))
      ENDIF
      IF gem_wind_amiga_menu[i] <> 0
        gem_FreeMenus(gem_wind_amiga_menu[i])
        gem_wind_amiga_menu[i] := 0
      ENDIF
      IF gem_wind_newmenu[i] <> 0
        gem_FreeNewMenu(gem_wind_newmenu[i])
        gem_wind_newmenu[i] := 0
      ENDIF
      gem_wind_gem_menu_idx[i] := -1
    ENDFOR
    gem_menu_bar_visible := 0
    gem_menu_active_app := 0
  ENDIF
  ctx[0] := 1
ENDPROC

-> menu_icheck(tree, item, check) - Check or uncheck a menu item
PROC gem_menu_icheck()
  DEF tree, item, check, i, widx, item_num
  tree := ctx[3]
  item := ctx[4]
  check := ctx[5]
  -> Find window with this menu attached
  widx := -1
  FOR i := 0 TO 15
    IF gem_wind_gem_menu_idx[i] >= 0
      DEF midx
      midx := gem_wind_gem_menu_idx[i]
      IF gem_menu_tree[midx] = tree
        widx := i
        i := 16
      ENDIF
    ENDIF
  ENDFOR
  IF widx >= 0 AND gem_wind_amiga_menu[widx] <> 0 AND gem_window_list[widx] <> 0
    -> item is the object index; we need to find its menu/item position
    -> For now, encode as (menu_num << 8) | item_num based on tree walk
    item_num := gem_tree_item_to_menu_pos(tree, item)
    gem_SetMenuCheck(asWIN(gem_window_list[widx]), gem_wind_amiga_menu[widx], item_num, check)
  ENDIF
  ctx[0] := 1
ENDPROC

-> menu_ienable(tree, item, enable) - Enable or disable a menu item
PROC gem_menu_ienable()
  DEF tree, item, enable, i, widx, item_num
  tree := ctx[3]
  item := ctx[4]
  enable := ctx[5]
  widx := -1
  FOR i := 0 TO 15
    IF gem_wind_gem_menu_idx[i] >= 0
      DEF midx
      midx := gem_wind_gem_menu_idx[i]
      IF gem_menu_tree[midx] = tree
        widx := i
        i := 16
      ENDIF
    ENDIF
  ENDFOR
  IF widx >= 0 AND gem_wind_amiga_menu[widx] <> 0 AND gem_window_list[widx] <> 0
    item_num := gem_tree_item_to_menu_pos(tree, item)
    gem_SetMenuEnable(asWIN(gem_window_list[widx]), gem_wind_amiga_menu[widx], item_num, enable)
  ENDIF
  ctx[0] := 1
ENDPROC

-> menu_tnormal(tree, item, normal) - Set normal/inverted state of menu item
PROC gem_menu_tnormal()
  DEF tree, item, normal
  tree := ctx[3]
  item := ctx[4]
  normal := ctx[5]
  -> On AmigaOS, menu item highlighting is managed by Intuition
  -> after selection; this is typically a no-op in the emulator
  ctx[0] := 1
ENDPROC

-> menu_text(tree, item, text) - Change text of a menu item
PROC gem_menu_text()
  DEF tree, item, text_ptr
  tree := ctx[3]
  item := ctx[4]
  text_ptr := ctx[5]
  -> Text changes require rebuilding the NewMenu; stub for now
  ctx[0] := 1
ENDPROC

-> menu_register(pid, tree) - Register a menu tree for an application
PROC gem_menu_register()
  DEF pid, tree, i
  pid := ctx[3]
  tree := ctx[4]
  -> Check if already registered
  FOR i := 0 TO gem_menu_count - 1
    IF gem_menu_tree[i] = tree
      ctx[0] := gem_menu_count
      ENDPROC
    ENDIF
  ENDFOR
  IF gem_menu_count < 8
    gem_menu_tree[gem_menu_count] := tree
    gem_menu_owner[gem_menu_count] := pid
    gem_menu_count := gem_menu_count + 1
  ENDIF
  ctx[0] := gem_menu_count
ENDPROC

-> menu_popup() - Pop up a menu at a position
PROC gem_menu_popup()
  DEF menu_id, x, y
  menu_id := ctx[3]
  x := ctx[4]
  y := ctx[5]
  ctx[1] := 0
  ctx[0] := 0
ENDPROC

PROC gem_menu_attach()
  DEF pid, tree, item, child_tree
  pid := ctx[3]
  tree := ctx[4]
  item := ctx[5]
  child_tree := ctx[6]
  ctx[0] := 1
ENDPROC

PROC gem_menu_istart()
  DEF pid, tree, item
  pid := ctx[3]
  tree := ctx[4]
  item := ctx[5]
  ctx[0] := 1
ENDPROC

PROC gem_menu_settings()
  DEF pid, tree, item, settings
  pid := ctx[3]
  tree := ctx[4]
  item := ctx[5]
  settings := ctx[6]
  ctx[0] := 1
ENDPROC


-> ---------------------------------------------------------------------------
-> gem_tree_item_to_menu_pos - Convert GEM object index to AmigaOS menu position
-> Walks the GEM OBJECT tree to find the item's menu# and item# within that menu
-> Returns (menunum << 8) | itemnum
-> ---------------------------------------------------------------------------
PROC gem_tree_item_to_menu_pos(tree:VALUE, obj_index:VALUE)
  DEF result
  NATIVE {
    extern unsigned long gem_tree_item_to_menu_pos_result;
    struct GEMObject { short next, head, tail; unsigned short flags, state, type; long spec; short x, y, w, h; };
    struct GEMObject *objtree = (struct GEMObject *)tree;
    int idx = (int)obj_index;
    int menu_num = -1, item_num = 0, cur_item = 0, i;
    /* walk tree: titles are menu_num increments, items count within each menu */
    for (i = 0; objtree[i].type != 0; i++) {
      if (objtree[i].type == 25) { /* G_TITLE */
        menu_num++;
        cur_item = 0;
      } else if (objtree[i].type == 26) { /* G_STRING */
        if (i == idx) {
          item_num = cur_item;
          break;
        }
        cur_item++;
      }
    }
    if (menu_num < 0) menu_num = 0;
    gem_tree_item_to_menu_pos_result = (menu_num << 8) | item_num;
  } ENDNATIVE
ENDPROC gem_tree_item_to_menu_pos_result

DEF gem_tree_item_to_menu_pos_result


-> ============================
-> OBJC - Object services
-> ============================

PROC gem_objc_add()
  DEF tree, parent, child
  tree := ctx[3]
  parent := ctx[4]
  child := ctx[5]
  ctx[0] := 1
ENDPROC

PROC gem_objc_delete()
  ctx[0] := 1
ENDPROC

PROC gem_objc_draw()
  DEF tree, obj, depth
  DEF xc, yc, wc, hc
  tree := ctx[3]
  obj := ctx[4]
  depth := ctx[5]
  xc := ctx[6]; yc := ctx[7]; wc := ctx[8]; hc := ctx[9]
  -> Draw the object tree (or sub-tree) on the first open window
  IF tree <> 0
    gem_draw_tree(tree, obj, depth)
  ENDIF
  ctx[0] := 1
ENDPROC

PROC gem_objc_find()
  DEF tree, obj, depth, mx, my
  tree := ctx[3]; obj := ctx[4]; depth := ctx[5]
  mx := ctx[6]; my := ctx[7]
  -> Walk the tree to find the top-most object containing (mx, my)
  IF tree <> 0
    NATIVE {
      struct GEMObject { short next, head, tail; unsigned short flags, state, type; long spec; short x, y, w, h; };
      struct GEMObject *objtree = (struct GEMObject *)tree;
      int start = (int)obj;
      int found = 0, i;
      /* Scan from start to end for clickable objects containing the point */
      for (i = start; objtree[i].type != 0; i++) {
        if (objtree[i].type == 21 || objtree[i].type == 22 || objtree[i].type == 6) {
          if (mx >= objtree[i].x && mx < objtree[i].x + objtree[i].w &&
              my >= objtree[i].y && my < objtree[i].y + objtree[i].h) {
            found = i;
          }
        }
      }
      gem_objc_find_result = found;
    } ENDNATIVE
    ctx[0] := gem_objc_find_result
  ELSE
    ctx[0] := 0
  ENDIF
ENDPROC

DEF gem_objc_find_result

-> ---------------------------------------------------------------------------
-> Draw a GEM object tree on the first open window
-> Walks the tree and draws boxes, radio buttons, checkboxes, and strings
-> ---------------------------------------------------------------------------
PROC gem_draw_tree(tree:VALUE, root_obj:VALUE, max_depth:VALUE)
  DEF i
  DEF win_ptr
  win_ptr := 0
  FOR i := 0 TO MAX_WINDOWS - 1
    IF win_ptr = 0 AND gem_window_list[i] <> 0 AND gem_wind_state[i] = WS_OPEN
      win_ptr := gem_window_list[i]
    ENDIF
  ENDFOR
  IF win_ptr = 0 THEN RETURN
  NATIVE {
    struct GEMObject { short next, head, tail; unsigned short flags, state, type; long spec; short x, y, w, h; };
    struct GEMObject *objtree = (struct GEMObject *)tree;
    struct Window *win = (struct Window *)win_ptr;
    struct RastPort *rp;
    int start = (int)root_obj;
    int i;
    if (!win) return;
    rp = win->RPort;
    SetAPen(rp, 1);
    SetDrMd(rp, JAM1);
    for (i = start; objtree[i].type != 0; i++) {
      short ox = objtree[i].x;
      short oy = objtree[i].y;
      short ow = objtree[i].w;
      short oh = objtree[i].h;
      switch (objtree[i].type) {
        case 0: /* G_BOX */
        case 5: /* G_IBOX */
          SetAPen(rp, 0);
          RectFill(rp, ox, oy, ox + ow - 1, oy + oh - 1);
          SetAPen(rp, 1);
          Move(rp, ox, oy);
          Draw(rp, ox + ow - 1, oy);
          Draw(rp, ox + ow - 1, oy + oh - 1);
          Draw(rp, ox, oy + oh - 1);
          Draw(rp, ox, oy);
          break;
        case 21: /* G_RBUTTON */
          {
            short cx = ox + 6;
            short cy = oy + oh / 2;
            short r = 5;
            int sel = (objtree[i].state & 1);
            /* Draw circle outline */
            SetAPen(rp, 1);
            Move(rp, cx + r, cy);
            {
              int angle;
              for (angle = 0; angle <= 360; angle += 15) {
                double rad = angle * 3.14159265 / 180.0;
                short px = cx + (int)(r * cos(rad));
                short py = cy + (int)(r * sin(rad));
                if (angle == 0) Move(rp, px, py);
                else Draw(rp, px, py);
              }
            }
            /* Fill if selected */
            if (sel) {
              SetAPen(rp, 1);
              RectFill(rp, cx - r/2, cy - r/2, cx + r/2, cy + r/2);
            }
            /* Draw label text */
            if (objtree[i].spec) {
              SetAPen(rp, 1);
              Move(rp, ox + 16, cy + 4);
              Text(rp, (STRPTR)objtree[i].spec, strlen((STRPTR)objtree[i].spec));
            }
          }
          break;
        case 22: /* G_CHECKBOX */
          {
            short bx = ox;
            short by = oy + (oh - 10) / 2;
            short bsz = 10;
            int sel = (objtree[i].state & 1);
            /* Draw checkbox square */
            SetAPen(rp, 1);
            Move(rp, bx, by);
            Draw(rp, bx + bsz, by);
            Draw(rp, bx + bsz, by + bsz);
            Draw(rp, bx, by + bsz);
            Draw(rp, bx, by);
            /* If selected, draw checkmark */
            if (sel) {
              Move(rp, bx + 2, by + 5);
              Draw(rp, bx + 4, by + 7);
              Draw(rp, bx + 8, by + 2);
            }
            /* Draw label text */
            if (objtree[i].spec) {
              SetAPen(rp, 1);
              Move(rp, ox + 14, by + 8);
              Text(rp, (STRPTR)objtree[i].spec, strlen((STRPTR)objtree[i].spec));
            }
          }
          break;
        case 6: /* G_BUTTON */
          {
            int sel = (objtree[i].state & 1);
            /* Draw button rectangle */
            SetAPen(rp, sel ? 1 : 0);
            RectFill(rp, ox, oy, ox + ow - 1, oy + oh - 1);
            SetAPen(rp, 1);
            Move(rp, ox, oy);
            Draw(rp, ox + ow - 1, oy);
            Draw(rp, ox + ow - 1, oy + oh - 1);
            Draw(rp, ox, oy + oh - 1);
            Draw(rp, ox, oy);
            /* Draw label centered */
            if (objtree[i].spec) {
              int len = strlen((STRPTR)objtree[i].spec);
              int tx = ox + (ow - len * 8) / 2;
              int ty = oy + (oh + 8) / 2;
              SetAPen(rp, sel ? 0 : 1);
              Move(rp, tx, ty);
              Text(rp, (STRPTR)objtree[i].spec, len);
            }
          }
          break;
        case 8: /* G_STRING */
          if (objtree[i].spec) {
            SetAPen(rp, 1);
            Move(rp, ox + 2, oy + oh - 3);
            Text(rp, (STRPTR)objtree[i].spec, strlen((STRPTR)objtree[i].spec));
          }
          break;
        default:
          break;
      }
    }
  } ENDNATIVE
ENDPROC

PROC gem_objc_offset()
  DEF tree, obj
  tree := ctx[3]; obj := ctx[4]
  -> Return offset of object within tree
  ctx[1] := 0
  ctx[2] := 0
  ctx[0] := E_OK
ENDPROC

PROC gem_objc_order()
  ctx[0] := 1
ENDPROC

PROC gem_objc_edit()
  -> Returns keystroke or 0
  ctx[0] := 0
ENDPROC

PROC gem_objc_change()
  DEF tree, obj, depth, new_state
  tree := ctx[3]; obj := ctx[4]; depth := ctx[5]; new_state := ctx[9]
  -> Update the state field of the object
  IF tree <> 0 AND obj >= 0
    NATIVE {
      struct GEMObject { short next, head, tail; unsigned short flags, state, type; long spec; short x, y, w, h; };
      struct GEMObject *objtree = (struct GEMObject *)tree;
      int idx = (int)obj;
      objtree[idx].state = (unsigned short)((int)new_state & 0xFFFF);
    } ENDNATIVE
  ENDIF
  ctx[0] := 1
ENDPROC

PROC gem_objc_type()
  ctx[0] := E_OK
ENDPROC


-> ============================
-> FORM - Form services
-> ============================

PROC gem_form_do()
  DEF tree, startobj, obj_ptr, key
  DEF form_x, form_y, form_w, form_h
  DEF win_idx, win_h
  DEF done, event_obj, i
  tree := ctx[3]; startobj := ctx[4]
  gem_form_active := tree
  -> If no tree, return immediately
  IF tree = 0
    gem_form_active := -1
    ctx[0] := startobj
    RETURN
  ENDIF
  -> Read form dimensions from root object (type G_BOX = 0)
  obj_ptr := tree
  form_x := 0; form_y := 0; form_w := 200; form_h := 100
  NATIVE {
    unsigned char *obj = (unsigned char *)obj_ptr;
    short ot = *(short *)(obj + 6);
    if (ot >= 0 && ot <= 3) {
      form_x = *(short *)(obj + 12);
      form_y = *(short *)(obj + 14);
      form_w = *(short *)(obj + 16);
      form_h = *(short *)(obj + 18);
    }
  } ENDNATIVE
  -> Find or create a window for the form
  -> Look for an existing open window, or use a temporary one
  win_h := 0
  FOR i := 0 TO MAX_WINDOWS - 1
    IF win_h = 0 AND gem_window_list[i] <> 0 AND gem_wind_state[i] = WS_OPEN
      win_h := gem_wind_handle[i]
    ENDIF
  ENDFOR
  -> If no window available, create a temporary one for the form
  IF win_h = 0
    gem_wind_alloc()
    win_idx := gem_wind_find_handle(ctx[0])
    IF win_idx >= 0
      gem_wind_kind[win_idx] := WIN_MOVER OR WIN_CLOSER
      gem_wind_x[win_idx] := form_x
      gem_wind_y[win_idx] := form_y
      gem_wind_w[win_idx] := form_w
      gem_wind_h[win_idx] := form_h
      gem_wind_work_x[win_idx] := form_x + 4
      gem_wind_work_y[win_idx] := form_y + 30
      gem_wind_work_w[win_idx] := form_w - 8
      gem_wind_work_h[win_idx] := form_h - 34
      gem_wind_full_x[win_idx] := form_x
      gem_wind_full_y[win_idx] := form_y
      gem_wind_full_w[win_idx] := form_w
      gem_wind_full_h[win_idx] := form_h
      NATIVE {
        struct Window *w;
        struct NewWindow nw;
        long flags = WFLG_SMART_REFRESH | WFLG_ACTIVATE | WFLG_GIMMEZEROZERO | WFLG_CLOSEGADGET | WFLG_DRAGBAR;
        long idcmp = IDCMP_CLOSEWINDOW | IDCMP_REFRESHWINDOW | IDCMP_MOUSEBUTTONS | IDCMP_MOUSEMOVE | IDCMP_GADGETUP | IDCMP_GADGETDOWN;
        nw.LeftEdge = form_x; nw.TopEdge = form_y;
        nw.Width = form_w; nw.Height = form_h;
        nw.DetailPen = 0; nw.BlockPen = 1;
        nw.Title = "Form";
        nw.Flags = flags; nw.IDCMPFlags = idcmp;
        nw.Type = WBENCHSCREEN; nw.FirstGadget = NULL;
        nw.CheckMark = NULL; nw.Screen = NULL; nw.BitMap = NULL;
        nw.MinWidth = 50; nw.MinHeight = 30;
        nw.MaxWidth = 2048; nw.MaxHeight = 2048;
        w = OpenWindow(&nw);
        gem_window_list[win_idx] = (long)w;
        if (w) {
          gem_wind_state[win_idx] = 1;
          win_h = gem_wind_handle[win_idx];
        }
      } ENDNATIVE
    ENDIF
  ENDIF
  -> Create radio button gadgets from the tree
  gem_radio_count := 0
  IF win_h <> 0
    win_idx := gem_wind_find_handle(win_h)
    IF win_idx >= 0
      gem_CreateRadioGadgets(tree, gem_window_list[win_idx])
    ENDIF
  ENDIF
  -> Event loop: wait for a button click or key press
  done := 0
  event_obj := startobj
  WHILE done = 0
    -> Check for IDCMP events if we have a window
    IF win_h <> 0
      win_idx := gem_wind_find_handle(win_h)
      IF win_idx >= 0 AND gem_window_list[win_idx] <> 0
        NATIVE {
          struct Window *win = (struct Window *)gem_window_list[win_idx];
          struct IntuiMessage *msg;
          if (win && win->UserPort) {
            while ((msg = (struct IntuiMessage *)GetMsg(win->UserPort))) {
              if (msg->Class == IDCMP_GADGETUP || msg->Class == IDCMP_GADGETDOWN) {
                struct Gadget *gad = (struct Gadget *)msg->IAddress;
                /* Find which radio button was clicked */
                { int ri;
                  for (ri = 0; ri < gem_radio_count; ri++) {
                    if ((long)gem_radio_gad[ri] == (long)gad) {
                      event_obj = gem_radio_obj[ri];
                      done = 1;
                      break;
                    }
                  }
                }
                /* Sync GEM state: clear all, set selected */
                { int si; int sel_obj = event_obj;
                  struct GEMObject { short next, head, tail; unsigned short flags, state, type; long spec; short x, y, w, h; };
                  struct GEMObject *objtree = (struct GEMObject *)tree;
                  for (si = 0; si < gem_radio_count; si++) {
                    int oi = gem_radio_obj[si];
                    if (oi >= 0) {
                      if (oi == sel_obj) objtree[oi].state |= 1;
                      else objtree[oi].state &= ~1;
                    }
                  }
                }
              } else if (msg->Class == IDCMP_CLOSEWINDOW) {
                event_obj = startobj;
                done = 1;
              }
              ReplyMsg((struct Message *)msg);
            }
          }
        } ENDNATIVE
      ENDIF
    ENDIF
    -> Also check console for Enter key (fallback)
    IF done = 0 AND WaitForChar(Input(), 0)
      key := 0
      Read(Input(), key, 1)
      IF key = 10 OR key = 13
        -> Enter: accept the currently selected radio or default button
        event_obj := gem_GetRadioSelection()
        IF event_obj < 0
          event_obj := startobj
        ENDIF
        done := 1
      ENDIF
    ENDIF
    IF done = 0
      -> Small delay to avoid busy-waiting
      Delay(1)
    ENDIF
  ENDWHILE
  -> Remove radio gadgets and free the window if we created it
  IF gem_radio_count > 0
    gem_FreeRadioGadgets()
  ENDIF
  -> Clean up temporary window if we created one
  IF win_h <> 0
    win_idx := gem_wind_find_handle(win_h)
    IF win_idx >= 0 AND gem_window_list[win_idx] <> 0
      -> Only close if this was a temporary form window
      -> (check if the window was not pre-existing by seeing if we set it up)
      NATIVE {
        struct Window *w = (struct Window *)gem_window_list[win_idx];
        if (w) {
          /* Remove any remaining gadgets first */
          RemoveGList(w, w->FirstGadget, -1);
          CloseWindow(w);
          gem_window_list[win_idx] = 0;
        }
      } ENDNATIVE
      gem_wind_state[win_idx] := WS_CLOSED
    ENDIF
  ENDIF
  gem_form_active := -1
  ctx[0] := event_obj
ENDPROC

PROC gem_form_dial()
  DEF dtype, ix, iy, iw, ih, x, y, w, h
  dtype := ctx[3]
  ix := ctx[4]; iy := ctx[5]; iw := ctx[6]; ih := ctx[7]
  x := ctx[8]; y := ctx[9]; w := ctx[10]; h := ctx[11]
  -> Animated dialog box transition (0=init, 1=start, 2=draw, 3=exit)
  -> For now, just return OK (no real animation drawn to screen)
  ctx[0] := 1
ENDPROC

DEF gem_form_alert_title[128]:ARRAY OF CHAR
DEF gem_form_alert_body[256]:ARRAY OF CHAR
DEF gem_form_alert_gad[128]:ARRAY OF CHAR
DEF gem_form_alert_result

PROC gem_form_alert()
  DEF default_btn, alert_str:PTR TO CHAR, i, bracket
  default_btn := ctx[3]
  alert_str := asPTR(ctx[4])
  -> Parse GEM alert format: "[default][message][button1|button2|button3]"
  gem_form_alert_title[0] := 0
  gem_form_alert_body[0] := 0
  gem_form_alert_gad[0] := 0
  NATIVE {
    char *src = (char *)alert_str;
    char *dst;
    int section = 0, ch;
    while ((ch = *src++) != 0) {
      if (ch == '[') {
        section++;
        dst = section == 1 ? gem_form_alert_title : (section == 2 ? gem_form_alert_body : gem_form_alert_gad);
        *dst = 0;
      } else if (ch == ']') {
        if (dst) *dst = 0;
      } else if (section == 1) {
        *dst++ = ch; *dst = 0;
      } else if (section == 2) {
        *dst++ = ch; *dst = 0;
      } else if (section == 3) {
        *dst++ = ch; *dst = 0;
      }
    }
    if (!gem_form_alert_gad[0]) CopyMem(" OK ", gem_form_alert_gad, 4);
  } ENDNATIVE
  -> Show EasyRequest
  NATIVE {
    struct EasyStruct es;
    char *win_title = gem_form_alert_title[0] ? gem_form_alert_title : "Alert";
    es.es_StructSize = sizeof(struct EasyStruct);
    es.es_Flags = 0;
    es.es_Title = win_title;
    es.es_TextFormat = (STRPTR)gem_form_alert_body;
    es.es_GadgetFormat = (STRPTR)gem_form_alert_gad;
    gem_form_alert_result = EasyRequest(NULL, &es, NULL);
  } ENDNATIVE
  ctx[0] := gem_form_alert_result + 1
ENDPROC

PROC gem_form_error()
  ctx[0] := 1
ENDPROC

PROC gem_form_center()
  DEF tree, fw, fh
  tree := ctx[3]
  -> Try to read form dimensions from root object
  fw := 200
  fh := 100
  IF tree <> 0
    NATIVE {
      unsigned char *obj = (unsigned char *)tree;
      short w = *(short *)(obj + 16);
      short h = *(short *)(obj + 18);
      if (w > 0 && h > 0) { gem_form_center_w = w; gem_form_center_h = h; }
    } ENDNATIVE
    fw := gem_form_center_w
    fh := gem_form_center_h
  ENDIF
  ctx[1] := (gem_scrn_w - fw) / 2
  ctx[2] := (gem_scrn_h - fh) / 2
  ctx[3] := fw
  ctx[4] := fh
  ctx[0] := E_OK
ENDPROC

DEF gem_form_center_w, gem_form_center_h

PROC gem_form_keybd()
  -> Return next object (0 = none)
  ctx[0] := 0
ENDPROC

PROC gem_form_button()
  DEF tree, obj, depth, next_obj
  tree := ctx[3]; obj := ctx[4]; depth := ctx[5]
  next_obj := 0
  -> If this is a radio button (type 21), handle mutual exclusion
  IF tree <> 0 AND obj >= 0
    NATIVE {
      struct GEMObject { short next, head, tail; unsigned short flags, state, type; long spec; short x, y, w, h; };
      struct GEMObject *objtree = (struct GEMObject *)tree;
      int idx = (int)obj;
      if (objtree[idx].type == 21) { /* G_RBUTTON */
        int i;
        /* Clear SELECTED bit on all radio buttons in this group.
         * Radio group detection: walk from current object following 'next'
         * until we find a non-radio button or wrap back. */
        for (i = 0; objtree[i].type != 0; i++) {
          if (objtree[i].type == 21) {
            objtree[i].state &= ~1; /* clear SELECTED */
          }
        }
        /* Set SELECTED on the clicked one */
        objtree[idx].state |= 1;
        next_obj = idx;
      } else if (objtree[idx].type == 22) { /* G_CHECKBOX: toggle */
        objtree[idx].state ^= 1;
        next_obj = idx;
      } else {
        next_obj = idx;
      }
    } ENDNATIVE
  ENDIF
  -> Return the object that was clicked (or next object to process)
  ctx[0] := next_obj
ENDPROC


-> ============================
-> SCRP - Scrap (clipboard)
-> ============================

PROC gem_scrp_read()
  -> Set pointer to scrap buffer, return length
  ctx[1] := gem_scrap_len
  ctx[0] := E_OK
ENDPROC

PROC gem_scrp_write()
  -> Write scrap from pointer
  gem_scrap_len := ctx[1]
  ctx[0] := E_OK
ENDPROC


-> ============================
-> FSEL - File selector
-> ============================

-> fsel_exinput() - File selector (load mode)
-> addr_in[0] = path (128 bytes), addr_in[1] = filename (13 bytes)
-> intout[0] = 0=cancelled, 1=selected
-> The path and filename are updated in place
PROC gem_fsel_exinput()
  DEF path_ptr, file_ptr
  path_ptr := ctx[8]
  file_ptr := ctx[9]
  IF path_ptr = 0 OR file_ptr = 0
    ctx[0] := 0
    ctx[1] := 0
    ENDPROC
  ENDIF
  NATIVE {
    extern unsigned long reqtools_base;
    APTR (*rtAllocRequestA)(ULONG, APTR, struct TagItem *);
    APTR (*rtFileRequestA)(APTR, APTR, APTR, struct TagItem *);
    void (*rtFreeRequest)(APTR);
    struct TagItem tags[2];
    APTR filereq;
    char *path = (char *)path_ptr;
    char *file = (char *)file_ptr;
    long ok = 0;

    if (!reqtools_base) { ctx[1] = 0; return; }

    /* LVO offsets from library base:
     * Library base points to first function (Open).
     * User functions start at offset 24 (after Open/Close/Expunge/ExtFunc).
     * rtAllocRequestA: LVO -132 -> offset 108
     * rtFileRequestA:  LVO -204 -> offset 180
     * rtFreeRequest:   LVO -138 -> offset 114
     */
    rtAllocRequestA = (APTR)((char *)reqtools_base + 108);
    rtFreeRequest   = (APTR)((char *)reqtools_base + 114);
    rtFileRequestA  = (APTR)((char *)reqtools_base + 180);

    /* Allocate a file requester (type 0 = RTREQTYPE_FILE) */
    filereq = rtAllocRequestA(0, 0, 0);
    if (filereq) {
      tags[0].ti_Tag  = 0; /* TAG_DONE */
      tags[0].ti_Data = 0;

      ok = rtFileRequestA(filereq, file, "Select File", tags);

      if (ok) {
        /* Directory string is at offset 100 in struct rtFileRequester */
        char *dir = *((char **)((char *)filereq + 100));
        int len = 0;
        if (dir) {
          while (dir[len] && len < 126) { path[len] = dir[len]; len++; }
          /* Ensure trailing slash for AmigaOS directory paths */
          if (len > 0 && path[len-1] != ':' && path[len-1] != '/') {
            path[len] = '/';
            len++;
          }
        }
        path[len] = 0;
      }
      rtFreeRequest(filereq);
    }
    ctx[1] = ok;
  } ENDNATIVE
ENDPROC

-> fsel_exoutput() - File selector (save mode)
-> Same as fsel_exinput but titled "Save File"
PROC gem_fsel_exoutput()
  DEF path_ptr, file_ptr
  path_ptr := ctx[8]
  file_ptr := ctx[9]
  IF path_ptr = 0 OR file_ptr = 0
    ctx[0] := 0
    ctx[1] := 0
    ENDPROC
  ENDIF
  NATIVE {
    extern unsigned long reqtools_base;
    APTR (*rtAllocRequestA)(ULONG, APTR, struct TagItem *);
    APTR (*rtFileRequestA)(APTR, APTR, APTR, struct TagItem *);
    void (*rtFreeRequest)(APTR);
    struct TagItem tags[2];
    APTR filereq;
    char *path = (char *)path_ptr;
    char *file = (char *)file_ptr;
    long ok = 0;

    if (!reqtools_base) { ctx[1] = 0; return; }

    rtAllocRequestA = (APTR)((char *)reqtools_base + 108);
    rtFreeRequest   = (APTR)((char *)reqtools_base + 114);
    rtFileRequestA  = (APTR)((char *)reqtools_base + 180);

    filereq = rtAllocRequestA(0, 0, 0);
    if (filereq) {
      tags[0].ti_Tag  = 0; /* TAG_DONE */
      tags[0].ti_Data = 0;

      ok = rtFileRequestA(filereq, file, "Save File", tags);

      if (ok) {
        char *dir = *((char **)((char *)filereq + 100));
        int len = 0;
        if (dir) {
          while (dir[len] && len < 126) { path[len] = dir[len]; len++; }
          if (len > 0 && path[len-1] != ':' && path[len-1] != '/') {
            path[len] = '/';
            len++;
          }
        }
        path[len] = 0;
      }
      rtFreeRequest(filereq);
    }
    ctx[1] = ok;
  } ENDNATIVE
ENDPROC


-> ============================
-> WIND - Window services
-> ============================

PROC gem_wind_create()
  DEF kind
  kind := ctx[3]
  gem_wind_alloc()
ENDPROC

PROC gem_wind_open()
  DEF handle, idx, title[128]:ARRAY OF CHAR
  handle := ctx[3]
  idx := gem_wind_find_handle(handle)
  IF idx >= 0
    gem_wind_state[idx] := WS_OPEN
    gem_wind_x[idx] := ctx[4]
    gem_wind_y[idx] := ctx[5]
    gem_wind_w[idx] := ctx[6]
    gem_wind_h[idx] := ctx[7]
    gem_wind_work_x[idx] := ctx[4] + 4
    gem_wind_work_y[idx] := ctx[5] + 30
    gem_wind_work_w[idx] := ctx[6] - 8
    gem_wind_work_h[idx] := ctx[7] - 34
    gem_wind_full_x[idx] := ctx[4]
    gem_wind_full_y[idx] := ctx[5]
    gem_wind_full_w[idx] := ctx[6]
    gem_wind_full_h[idx] := ctx[7]
    title[0] := 0
    gem_window_list[idx] := gem_OpenWindow(gem_wind_x[idx], gem_wind_y[idx], gem_wind_w[idx], gem_wind_h[idx], title, gem_wind_kind[idx])
    IF gem_window_list[idx] <> 0
      ctx[0] := 1
    ELSE
      ctx[0] := 0
    ENDIF
  ELSE
    ctx[0] := 0
  ENDIF
ENDPROC

PROC gem_wind_close()
  DEF handle, idx
  handle := ctx[3]
  idx := gem_wind_find_handle(handle)
  IF idx >= 0
    IF gem_window_list[idx] <> 0
      gem_HideWindow(asWIN(gem_window_list[idx]))
    ENDIF
    gem_wind_state[idx] := WS_CLOSED
    ctx[0] := 1
  ELSE
    ctx[0] := 0
  ENDIF
ENDPROC

PROC gem_wind_delete()
  DEF handle, idx
  handle := ctx[3]
  idx := gem_wind_find_handle(handle)
  IF idx >= 0
    IF gem_window_list[idx] <> 0
      -> Detach and free menus if this window had them
      IF gem_wind_amiga_menu[idx] <> 0
        gem_ClearMenuStrip(asWIN(gem_window_list[idx]))
        gem_FreeMenus(gem_wind_amiga_menu[idx])
        gem_wind_amiga_menu[idx] := 0
      ENDIF
      IF gem_wind_newmenu[idx] <> 0
        gem_FreeNewMenu(gem_wind_newmenu[idx])
        gem_wind_newmenu[idx] := 0
      ENDIF
      gem_wind_gem_menu_idx[idx] := -1
      gem_CloseWindow(asWIN(gem_window_list[idx]))
      gem_window_list[idx] := 0
    ENDIF
    gem_wind_state[idx] := WS_CLOSED
    gem_wind_handle[idx] := 0
    ctx[0] := 1
  ELSE
    ctx[0] := 0
  ENDIF
ENDPROC

PROC gem_wind_get()
  DEF handle, field, idx
  handle := ctx[3]
  field := ctx[4]
  idx := gem_wind_find_handle(handle)
  IF idx >= 0
    SELECT field
    CASE 0 -> -> WF_WORKXYWH: work area
      ctx[1] := gem_wind_work_x[idx]
      ctx[2] := gem_wind_work_y[idx]
      ctx[3] := gem_wind_work_w[idx]
      ctx[4] := gem_wind_work_h[idx]
    CASE 1 -> -> WF_CURRXYWH: current dimensions
      ctx[1] := gem_wind_x[idx]
      ctx[2] := gem_wind_y[idx]
      ctx[3] := gem_wind_w[idx]
      ctx[4] := gem_wind_h[idx]
    CASE 2 -> -> WF_PREVXYWH: previous dimensions
      ctx[1] := gem_wind_full_x[idx]
      ctx[2] := gem_wind_full_y[idx]
      ctx[3] := gem_wind_full_w[idx]
      ctx[4] := gem_wind_full_h[idx]
    CASE 5 -> -> WF_TOP: top window handle
      ctx[1] := handle
    CASE 6 -> -> WF_FIRSTXYWH: first rectangle
      ctx[1] := gem_wind_work_x[idx]
      ctx[2] := gem_wind_work_y[idx]
      ctx[3] := gem_wind_work_w[idx]
      ctx[4] := gem_wind_work_h[idx]
    CASE 7 -> -> WF_OWNER: owner PID
      ctx[1] := gem_wind_parent[idx]
    DEFAULT ->
      ctx[1] := gem_wind_x[idx]
      ctx[2] := gem_wind_y[idx]
      ctx[3] := gem_wind_w[idx]
      ctx[4] := gem_wind_h[idx]
    ENDSELECT
    ctx[0] := E_OK
  ELSE
    ctx[0] := E_ERROR
  ENDIF
ENDPROC

PROC gem_wind_set()
  DEF handle, field, idx
  handle := ctx[3]
  field := ctx[4]
  idx := gem_wind_find_handle(handle)
  IF idx >= 0
    SELECT field
    CASE 0 -> WF_WORKXYWH
      gem_wind_work_x[idx] := ctx[5]; gem_wind_work_y[idx] := ctx[6]
      gem_wind_work_w[idx] := ctx[7]; gem_wind_work_h[idx] := ctx[8]
    CASE 1 -> WF_CURRXYWH
      gem_wind_x[idx] := ctx[5]; gem_wind_y[idx] := ctx[6]
      gem_wind_w[idx] := ctx[7]; gem_wind_h[idx] := ctx[8]
      IF gem_window_list[idx] <> 0
        gem_MoveWindow(asWIN(gem_window_list[idx]), gem_wind_x[idx], gem_wind_y[idx])
        gem_SizeWindow(asWIN(gem_window_list[idx]), gem_wind_w[idx], gem_wind_h[idx])
      ENDIF
    CASE 3 -> WF_NEWSIZE
      gem_wind_w[idx] := ctx[5]; gem_wind_h[idx] := ctx[6]
      IF gem_window_list[idx] <> 0
        gem_SizeWindow(asWIN(gem_window_list[idx]), gem_wind_w[idx], gem_wind_h[idx])
      ENDIF
    CASE 4 -> WF_ICONIFY
      gem_wind_state[idx] := WS_ICONIFIED
      IF gem_window_list[idx] <> 0
        gem_HideWindow(asWIN(gem_window_list[idx]))
      ENDIF
    CASE 5 -> WF_TOP
    CASE 10 -> WF_NAME
      IF gem_window_list[idx] <> 0
        gem_WindowTitle(asWIN(gem_window_list[idx]), asPTR(ctx[5]))
      ENDIF
    ENDSELECT
    ctx[0] := E_OK
  ELSE
    ctx[0] := E_ERROR
  ENDIF
ENDPROC

PROC gem_wind_find()
  DEF mx, my, i, result
  mx := ctx[3]; my := ctx[4]
  result := 0
  -> Find top-most open window at (mx, my)
  FOR i := MAX_WINDOWS - 1 TO 0 STEP -1
    IF result = 0 AND gem_wind_state[i] = WS_OPEN
      IF mx >= gem_wind_x[i] AND mx < gem_wind_x[i] + gem_wind_w[i] AND my >= gem_wind_y[i] AND my < gem_wind_y[i] + gem_wind_h[i]
        result := gem_wind_handle[i]
      ENDIF
    ENDIF
  ENDFOR
  ctx[0] := result
ENDPROC

PROC gem_wind_update()
  DEF beg_end
  beg_end := ctx[3]
  -> 0=begin update, 1=end update
  ctx[0] := E_OK
ENDPROC

PROC gem_wind_calc()
  DEF calc_type, kind, x, y, w, h
  DEF title_h, frame_w
  calc_type := ctx[3]
  kind := ctx[4]
  x := ctx[5]; y := ctx[6]; w := ctx[7]; h := ctx[8]
  title_h := 30
  frame_w := 4
  -> calc_type 0: full→work area, 1: work→full area
  IF calc_type = 0
    ctx[1] := x + frame_w
    ctx[2] := y + title_h
    ctx[3] := w - frame_w * 2
    ctx[4] := h - title_h - frame_w
  ELSE
    ctx[1] := x - frame_w
    ctx[2] := y - title_h
    ctx[3] := w + frame_w * 2
    ctx[4] := h + title_h + frame_w
  ENDIF
  ctx[0] := E_OK
ENDPROC

PROC gem_wind_new()
  ctx[0] := 1
ENDPROC

PROC gem_wind_arrow()
  ctx[0] := E_OK
ENDPROC

PROC gem_wind_show()
  DEF handle, flag
  handle := ctx[3]; flag := ctx[4]
  ctx[0] := E_OK
ENDPROC

PROC gem_wind_toolbar()
  ctx[0] := E_OK
ENDPROC

PROC gem_wind_sized()
  ctx[0] := E_OK
ENDPROC


-> ============================
-> GRAF - Graphics services
-> ============================

-> Mouse shape constants
CONST M_ON = 0, M_OFF = 1, M_ARROW = 2, M_BUSY = 3, M_IBEAM = 4, M_POINT = 5
CONST M_USERDEF = 6, M_SPECIAL = 7

-> Initialize predefined mouse pointer shapes
PROC gem_mouse_init()
  DEF i
  -> Arrow sprite data (16x16)
  gem_mouse_arrow_data[0] := $0000;  gem_mouse_arrow_mask[0] := $0000
  gem_mouse_arrow_data[1] := $7C00;  gem_mouse_arrow_mask[1] := $FC00
  gem_mouse_arrow_data[2] := $7200;  gem_mouse_arrow_mask[2] := $F600
  gem_mouse_arrow_data[3] := $7100;  gem_mouse_arrow_mask[3] := $F300
  gem_mouse_arrow_data[4] := $7080;  gem_mouse_arrow_mask[4] := $F180
  gem_mouse_arrow_data[5] := $7040;  gem_mouse_arrow_mask[5] := $F0C0
  gem_mouse_arrow_data[6] := $7020;  gem_mouse_arrow_mask[6] := $F060
  gem_mouse_arrow_data[7] := $7C10;  gem_mouse_arrow_mask[7] := $FC30
  gem_mouse_arrow_data[8] := $4608;  gem_mouse_arrow_mask[8] := $EF18
  gem_mouse_arrow_data[9] := $4304;  gem_mouse_arrow_mask[9] := $C78C
  gem_mouse_arrow_data[10] := $4182; gem_mouse_arrow_mask[10] := $C3C6
  gem_mouse_arrow_data[11] := $40E1; gem_mouse_arrow_mask[11] := $C1E3
  gem_mouse_arrow_data[12] := $4070; gem_mouse_arrow_mask[12] := $C0F0
  gem_mouse_arrow_data[13] := $4038; gem_mouse_arrow_mask[13] := $C078
  gem_mouse_arrow_data[14] := $401C; gem_mouse_arrow_mask[14] := $C03C
  gem_mouse_arrow_data[15] := $4000; gem_mouse_arrow_mask[15] := $C000

  -> Busy/hourglass sprite data (16x16)
  gem_mouse_busy_data[0] := $0000;  gem_mouse_busy_mask[0] := $0000
  gem_mouse_busy_data[1] := $7FFE;  gem_mouse_busy_mask[1] := $FFFF
  gem_mouse_busy_data[2] := $6006;  gem_mouse_busy_mask[2] := $F00F
  gem_mouse_busy_data[3] := $300C;  gem_mouse_busy_mask[3] := $781E
  gem_mouse_busy_data[4] := $1818;  gem_mouse_busy_mask[4] := $3C3C
  gem_mouse_busy_data[5] := $0C30;  gem_mouse_busy_mask[5] := $1E78
  gem_mouse_busy_data[6] := $0660;  gem_mouse_busy_mask[6] := $0FF0
  gem_mouse_busy_data[7] := $03C0;  gem_mouse_busy_mask[7] := $07E0
  gem_mouse_busy_data[8] := $0660;  gem_mouse_busy_mask[8] := $0FF0
  gem_mouse_busy_data[9] := $0C30;  gem_mouse_busy_mask[9] := $1E78
  gem_mouse_busy_data[10] := $1818; gem_mouse_busy_mask[10] := $3C3C
  gem_mouse_busy_data[11] := $300C; gem_mouse_busy_mask[11] := $781E
  gem_mouse_busy_data[12] := $6006; gem_mouse_busy_mask[12] := $F00F
  gem_mouse_busy_data[13] := $7FFE; gem_mouse_busy_mask[13] := $FFFF
  gem_mouse_busy_data[14] := $0000; gem_mouse_busy_mask[14] := $0000
  gem_mouse_busy_data[15] := $0000; gem_mouse_busy_mask[15] := $0000

  -> I-beam/vertical bar sprite data (16x16)
  FOR i := 0 TO 15
    gem_mouse_ibeam_data[i] := $0180
    gem_mouse_ibeam_mask[i] := $03C0
  ENDFOR
  gem_mouse_ibeam_data[0] := $0000; gem_mouse_ibeam_mask[0] := $0000
  gem_mouse_ibeam_data[15] := $0000; gem_mouse_ibeam_mask[15] := $0000

  -> Pointing finger sprite data (16x16)
  gem_mouse_point_data[0] := $0000;  gem_mouse_point_mask[0] := $0000
  gem_mouse_point_data[1] := $0E00;  gem_mouse_point_mask[1] := $1F00
  gem_mouse_point_data[2] := $0A00;  gem_mouse_point_mask[2] := $1F00
  gem_mouse_point_data[3] := $0A00;  gem_mouse_point_mask[3] := $1F00
  gem_mouse_point_data[4] := $0A00;  gem_mouse_point_mask[4] := $1F00
  gem_mouse_point_data[5] := $0A00;  gem_mouse_point_mask[5] := $1F00
  gem_mouse_point_data[6] := $0A00;  gem_mouse_point_mask[6] := $1F00
  gem_mouse_point_data[7] := $0A00;  gem_mouse_point_mask[7] := $1F00
  gem_mouse_point_data[8] := $0A00;  gem_mouse_point_mask[8] := $1F00
  gem_mouse_point_data[9] := $0A00;  gem_mouse_point_mask[9] := $1F00
  gem_mouse_point_data[10] := $0E00; gem_mouse_point_mask[10] := $1F00
  gem_mouse_point_data[11] := $1C00; gem_mouse_point_mask[11] := $3E00
  gem_mouse_point_data[12] := $3800; gem_mouse_point_mask[12] := $7C00
  gem_mouse_point_data[13] := $7000; gem_mouse_point_mask[13] := $F800
  gem_mouse_point_data[14] := $2000; gem_mouse_point_mask[14] := $7000
  gem_mouse_point_data[15] := $0000; gem_mouse_point_mask[15] := $0000
ENDPROC

-> graf_rubberbox() - Interactive rubber-band rectangle
-> Draws a XOR outline box from (x,y) to (x+minw, y+minh) at minimum
-> GEM waits for a button click and returns final width/height
PROC gem_graf_rubberbox()
  DEF x, y, minw, minh, finalw, finalh
  x := ctx[3]; y := ctx[4]; minw := ctx[5]; minh := ctx[6]
  -> Simulate a rubber-band box. In a real system, this tracks the mouse.
  -> We return a reasonable default size.
  finalw := minw + 40
  finalh := minh + 30
  ctx[1] := finalw
  ctx[2] := finalh
  ctx[0] := 1
ENDPROC

-> graf_dragbox() - Drag a box (interactive move feedback)
-> Draws a XOR outline box of size (w x h), starts at (sx,sy)
-> Constrained to area defined by (cx,cy) and bound flag:
->   bound=0: no constraint, bound=1: constrain to 1st rect, bound=2: to 2nd rect
-> Returns final (x,y) position
PROC gem_graf_dragbox()
  DEF w, h, sx, sy, cx, cy, bound, finalx, finaly
  w := ctx[3]; h := ctx[4]
  sx := ctx[5]; sy := ctx[6]
  cx := ctx[7]; cy := ctx[8]
  bound := ctx[9]
  finalx := sx
  finaly := sy
  IF bound = 1
    IF finalx < 0 THEN finalx := 0
    IF finalx + w > gem_scrn_w THEN finalx := gem_scrn_w - w
    IF finaly < 0 THEN finaly := 0
    IF finaly + h > gem_scrn_h THEN finaly := gem_scrn_h - h
  ENDIF
  ctx[1] := finalx
  ctx[2] := finaly
  ctx[0] := 1
ENDPROC

-> graf_movebox() - Animate moving a box from one position to another
-> Draws outline at (sx,sy) then at (dx,dy) to show movement
PROC gem_graf_movebox()
  DEF w, h, sx, sy, dx, dy
  w := ctx[3]; h := ctx[4]; sx := ctx[5]; sy := ctx[6]
  dx := ctx[7]; dy := ctx[8]
  ctx[0] := 1
ENDPROC

-> graf_growbox() - Animate growing a box (expand effect)
-> Grows from small rect (px,py,pw,ph) to big rect (sx,sy,sw,sh)
PROC gem_graf_growbox()
  DEF px, py, pw, ph, sx, sy, sw, sh
  px := ctx[3]; py := ctx[4]; pw := ctx[5]; ph := ctx[6]
  sx := ctx[7]; sy := ctx[8]; sw := ctx[9]; sh := ctx[10]
  ctx[0] := 1
ENDPROC

-> graf_shrinkbox() - Animate shrinking a box (collapse effect)
-> Shrinks from big rect (sx,sy,sw,sh) to small rect (px,py,pw,ph)
PROC gem_graf_shrinkbox()
  DEF sx, sy, sw, sh, px, py, pw, ph
  sx := ctx[3]; sy := ctx[4]; sw := ctx[5]; sh := ctx[6]
  px := ctx[7]; py := ctx[8]; pw := ctx[9]; ph := ctx[10]
  ctx[0] := 1
ENDPROC

-> graf_watchbox() - Watch an object for state change
-> Waits until the user clicks on the specified object
-> tree = object tree, obj = object index
-> instate = color when entered, outstate = color when exited
-> Returns outstate (the final state of the object)
PROC gem_graf_watchbox()
  DEF tree, obj, instate, outstate
  tree := ctx[3]; obj := ctx[4]; instate := ctx[5]; outstate := ctx[6]
  -> Simulate: return the object index to simulate selection
  ctx[1] := obj
  ctx[0] := outstate
ENDPROC

-> graf_slidebox() - Handle slider box movement
-> tree = object tree, parent = parent object, obj = slider object
-> is_horiz = 1 for horizontal, 0 for vertical
-> Returns new slider position (offset in pixels)
PROC gem_graf_slidebox()
  DEF tree, parent, obj, is_horiz, idx, newpos
  tree := ctx[3]; parent := ctx[4]; obj := ctx[5]
  is_horiz := ctx[6]
  -> Find slider index from object
  idx := obj AND 7
  IF idx > 7 THEN idx := 0
  IF idx < 0 THEN idx := 0
  -> Return stored slider position (simulated)
  IF is_horiz
    newpos := gem_graf_slidex[idx]
  ELSE
    newpos := gem_graf_slidey[idx]
  ENDIF
  ctx[1] := newpos
  ctx[0] := 1
ENDPROC

-> graf_handle() - Get graphics handle and character cell size
-> Returns: workstation handle, char width, char height, cell width (in pixels)
-> Also sets the workstation cell dimensions for text operations
PROC gem_graf_handle()
  -> Workstation handle: unique ID for the virtual device
  -> Char cell: 8x16 for high-resolution Atari ST mode
  -> Last value is typically 0 (box width/height unused)
  ctx[0] := gem_graf_wk_handle
  ctx[1] := gem_graf_char_w
  ctx[2] := gem_graf_char_h
  ctx[3] := 0
ENDPROC

-> graf_mkstate() - Get mouse state
-> Returns: button state, mouse X, mouse Y, keyboard state
PROC gem_graf_mkstate()
  ctx[0] := gem_mouse_buttons
  ctx[1] := gem_mouse_x
  ctx[2] := gem_mouse_y
  ctx[3] := gem_mouse_kstate
ENDPROC

-> graf_mouse() - Set mouse pointer shape
-> shape: M_ON=0 (show), M_OFF=1 (hide), M_ARROW=2 (default arrow),
->        M_BUSY=3 (hourglass), M_IBEAM=4 (text), M_POINT=5 (finger),
->        M_USERDEF=6 (custom 2-plane), M_SPECIAL=7 (custom 4-plane)
-> For M_USERDEF: ctx[4]=hotx, ctx[5]=hoty, ctx[6]=data_ptr
-> For M_SPECIAL: same + ctx[7]=color_ptr, ctx[8]=words
-> Returns previous mouse shape
PROC gem_graf_mouse()
  DEF shape, old_shape, hotx, hoty, data_ptr, win:PTR TO window
  shape := ctx[3]
  old_shape := gem_mouse_shape

  IF shape = M_OFF
    gem_mouse_visible := 0
  ELSE
    IF shape = M_ON
      gem_mouse_visible := 1
      gem_mouse_shape := M_ARROW
    ELSE
      IF shape = M_USERDEF OR shape = M_SPECIAL
        hotx := ctx[4]
        hoty := ctx[5]
        data_ptr := ctx[6]
        gem_mouse_user_hotx := hotx
        gem_mouse_user_hoty := hoty
        gem_mouse_user_active := 1
        -> Store pointer data from emulated memory (33 words)
        NATIVE {
          unsigned short *dst = (unsigned short *)gem_mouse_user_data;
          unsigned short *src = (unsigned short *)data_ptr;
          int n;
          for (n = 0; n < 33; n++) dst[n] = src[n];
        } ENDNATIVE
        gem_mouse_shape := shape
      ELSE
        IF shape >= M_ARROW AND shape <= M_POINT
          gem_mouse_shape := shape
          gem_mouse_user_active := 0
        ENDIF
      ENDIF
      gem_mouse_visible := 1
    ENDIF
  ENDIF

  -> Attempt to update the pointer via AmigaOS Intuition if a window is open
  win := asWIN(gem_window_list[0])
  IF gem_mouse_visible AND win <> 0
    IF gem_mouse_user_active
      gem_SetPointer(win, gem_mouse_user_data, 16, 16, gem_mouse_user_hotx, gem_mouse_user_hoty)
    ELSE
      -> Use predefined shape
      IF shape = M_ARROW
        gem_SetPointer(win, gem_mouse_arrow_data, 16, 16, 0, 0)
      ELSE
        IF shape = M_BUSY
          gem_SetPointer(win, gem_mouse_busy_data, 16, 16, 7, 7)
        ELSE
          IF shape = M_IBEAM
            gem_SetPointer(win, gem_mouse_ibeam_data, 16, 16, 7, 7)
          ELSE
            IF shape = M_POINT
              gem_SetPointer(win, gem_mouse_point_data, 16, 16, 0, 0)
            ENDIF
          ENDIF
        ENDIF
      ENDIF
    ENDIF
  ELSE
    IF NOT gem_mouse_visible AND win <> 0
      gem_ClearPointer(win)
    ENDIF
  ENDIF

  ctx[0] := old_shape
ENDPROC

-> graf_arrow() - Set arrow key mode
-> flag=0: arrow keys used for menu navigation (mouse-emulated)
-> flag=1: arrow keys used for normal cursor movement
PROC gem_graf_arrow()
  DEF flag
  flag := ctx[3]
  IF flag = 0 OR flag = 1
    gem_graf_arrow_mode := flag
  ENDIF
  ctx[0] := E_OK
ENDPROC

-> graf_set_screen() - Set screen parameters (reserved, rarely used)
-> Typically called during initialisation to pass screen info
PROC gem_graf_set_screen()
  DEF handle, ws_w, ws_h, ws_bits
  handle := ctx[3]
  ws_w := ctx[4]
  ws_h := ctx[5]
  ws_bits := ctx[6]
  gem_graf_wk_handle := handle
  ctx[0] := E_OK
ENDPROC

-> graf_set_handle() - Set workstation handle explicitly
PROC gem_graf_set_handle()
  DEF handle, char_w, char_h
  handle := ctx[3]
  char_w := ctx[4]
  char_h := ctx[5]
  gem_graf_wk_handle := handle
  IF char_w > 0 THEN gem_graf_char_w := char_w
  IF char_h > 0 THEN gem_graf_char_h := char_h
  ctx[0] := E_OK
ENDPROC

-> graf_accel() - Register/unregister accelerator key
-> pid = application ID, tree = object tree
-> Returns accelerator key code or 0
PROC gem_graf_accel()
  DEF pid, tree, key
  pid := ctx[3]
  tree := ctx[4]
  key := ctx[5]
  gem_graf_accel_key := key
  ctx[0] := gem_graf_accel_key
ENDPROC


-> ============================
-> VDI - Virtual Device Interface
-> ============================

DEF vdi_handle -> current workstation handle (-1 = none)
DEF vdi_work_w, vdi_work_h -> workstation pixel dimensions
DEF vdi_dev_w, vdi_dev_h -> device pixel dimensions
DEF vdi_n_planes -> bits per pixel
DEF vdi_line_type, vdi_line_width, vdi_line_color
DEF vdi_fill_type, vdi_fill_index, vdi_fill_color
DEF vdi_marker_type, vdi_marker_height, vdi_marker_color
DEF vdi_text_font, vdi_text_color, vdi_text_rotation
DEF vdi_wr_mode -> writing mode (1=replace, 2=transparent, 3=XOR, 4=reverseTransparent)
DEF vdi_clip_x, vdi_clip_y, vdi_clip_w, vdi_clip_h
DEF vdi_cur_x, vdi_cur_y -> graphics cursor position

-> VDI attribute constants
CONST VDI_REPLACE = 1, VDI_TRANSPARENT = 2, VDI_XOR = 3, VDI_REVERSE = 4

-> VDI colour lookup (Atari ST standard 16-colour palette indexed by colour index)
DEF vdi_rgb[48]:ARRAY OF CHAR -> 16 colours x 3 bytes (R,G,B)
PROC vdi_init_rgb()
  DEF v
  v := 0; vdi_rgb[0] := asCHAR(v); vdi_rgb[1] := asCHAR(v); vdi_rgb[2] := asCHAR(v)
  v := 0; vdi_rgb[3] := asCHAR(v); vdi_rgb[4] := asCHAR(v); v := 200; vdi_rgb[5] := asCHAR(v)
  v := 0; vdi_rgb[6] := asCHAR(v); v := 200; vdi_rgb[7] := asCHAR(v); v := 0; vdi_rgb[8] := asCHAR(v)
  v := 0; vdi_rgb[9] := asCHAR(v); v := 200; vdi_rgb[10] := asCHAR(v); v := 200; vdi_rgb[11] := asCHAR(v)
  v := 200; vdi_rgb[12] := asCHAR(v); v := 0; vdi_rgb[13] := asCHAR(v); v := 0; vdi_rgb[14] := asCHAR(v)
  v := 200; vdi_rgb[15] := asCHAR(v); v := 0; vdi_rgb[16] := asCHAR(v); v := 200; vdi_rgb[17] := asCHAR(v)
  v := 200; vdi_rgb[18] := asCHAR(v); v := 200; vdi_rgb[19] := asCHAR(v); v := 0; vdi_rgb[20] := asCHAR(v)
  v := 200; vdi_rgb[21] := asCHAR(v); v := 200; vdi_rgb[22] := asCHAR(v); v := 200; vdi_rgb[23] := asCHAR(v)
  v := 100; vdi_rgb[24] := asCHAR(v); v := 100; vdi_rgb[25] := asCHAR(v); v := 100; vdi_rgb[26] := asCHAR(v)
  v := 0; vdi_rgb[27] := asCHAR(v); v := 0; vdi_rgb[28] := asCHAR(v); v := 100; vdi_rgb[29] := asCHAR(v)
  v := 0; vdi_rgb[30] := asCHAR(v); v := 100; vdi_rgb[31] := asCHAR(v); v := 0; vdi_rgb[32] := asCHAR(v)
  v := 0; vdi_rgb[33] := asCHAR(v); v := 100; vdi_rgb[34] := asCHAR(v); v := 100; vdi_rgb[35] := asCHAR(v)
  v := 100; vdi_rgb[36] := asCHAR(v); v := 0; vdi_rgb[37] := asCHAR(v); v := 0; vdi_rgb[38] := asCHAR(v)
  v := 100; vdi_rgb[39] := asCHAR(v); v := 0; vdi_rgb[40] := asCHAR(v); v := 100; vdi_rgb[41] := asCHAR(v)
  v := 100; vdi_rgb[42] := asCHAR(v); v := 100; vdi_rgb[43] := asCHAR(v); v := 0; vdi_rgb[44] := asCHAR(v)
  v := 255; vdi_rgb[45] := asCHAR(v); v := 255; vdi_rgb[46] := asCHAR(v); v := 255; vdi_rgb[47] := asCHAR(v)
ENDPROC

-> VDI line type patterns (dash/dot definitions)
DEF vdi_line_pats[6]:ARRAY OF VALUE
PROC vdi_init_line_pats()
  vdi_line_pats[0] := $FFFF -> solid (user-defined, default = solid)
  vdi_line_pats[1] := $FFFF -> solid
  vdi_line_pats[2] := $FF88 -> long dash
  vdi_line_pats[3] := $FF88 -> short dash (same as long dash on ST)
  vdi_line_pats[4] := $CCCC -> dash-dot
  vdi_line_pats[5] := $CCCC -> dash-dot (alternate)
ENDPROC

-> VDI pen colour to Amiga pen mapping (first 16 standard VDI colours)
DEF vdi_pen_map[16]:ARRAY OF VALUE
PROC vdi_init_pen_map()
  vdi_pen_map[0] := 0; vdi_pen_map[1] := 1; vdi_pen_map[2] := 2; vdi_pen_map[3] := 3
  vdi_pen_map[4] := 4; vdi_pen_map[5] := 5; vdi_pen_map[6] := 6; vdi_pen_map[7] := 7
  vdi_pen_map[8] := 8; vdi_pen_map[9] := 9; vdi_pen_map[10] := 10; vdi_pen_map[11] := 11
  vdi_pen_map[12] := 12; vdi_pen_map[13] := 13; vdi_pen_map[14] := 14; vdi_pen_map[15] := 15
ENDPROC

-> Set VDI drawing attributes on a RastPort (pen, writing mode, line pattern)
PROC gem_SetVDIAttrs_RP(rp:PTR TO rastport) IS NATIVE {
  extern long vdi_line_color;
  extern long vdi_wr_mode;
  extern long vdi_line_type;
  extern long vdi_line_pats[6];
  extern long vdi_pen_map[16];
  struct RastPort *r = (struct RastPort *)rp;
  int pen = (int)vdi_line_color;
  if (pen < 0) pen = 0;
  if (pen > 15) pen = 15;
  SetAPen(r, vdi_pen_map[pen]);
  switch ((int)vdi_wr_mode) {
    case 1: SetDrMd(r, JAM1); break;
    case 2: SetDrMd(r, JAM2); break;
    case 3: SetDrMd(r, INVERS_XOR); break;
    default: SetDrMd(r, JAM1); break;
  }
  { int lt = (int)vdi_line_type;
    if (lt >= 0 && lt < 6) SetDrPt(r, vdi_line_pats[lt]);
  }
} ENDNATIVE

-> Get RastPort from first open window (0 if none)
PROC gem_GetVDIRastPort() IS NATIVE {
  extern long gem_window_list[16];
  struct Window *win;
  int i;
  for (i = 0; i < 16; i++) {
    win = (struct Window *)gem_window_list[i];
    if (win) return (long)win->RPort;
  }
  return 0;
} ENDNATIVE !!LONG

-> Open a virtual workstation with the given work-in array
-> ctx[3] = work_in ptr (in emulated memory) — 45 words of device-independent attributes
-> Returns handle via ctx[0]
PROC vdi_opnvwk()
  vdi_init_pen_map()
  vdi_handle := 1
  vdi_work_w := 640; vdi_work_h := 400
  vdi_dev_w := 640; vdi_dev_h := 400
  vdi_n_planes := 4
  vdi_line_type := 1; vdi_line_width := 1; vdi_line_color := 1
  vdi_fill_type := 1; vdi_fill_index := 1; vdi_fill_color := 1
  vdi_marker_type := 3; vdi_marker_height := 10; vdi_marker_color := 1
  vdi_text_font := 1; vdi_text_color := 1; vdi_text_rotation := 0
  vdi_wr_mode := VDI_REPLACE
  vdi_clip_x := 0; vdi_clip_y := 0; vdi_clip_w := vdi_work_w; vdi_clip_h := vdi_work_h
  vdi_cur_x := 0; vdi_cur_y := 0
  -> Fill work_out array in emulated memory via intout
  gem_intout[0] := 0            -> device id
  gem_intout[1] := 1            -> line type count
  gem_intout[2] := 1            -> line width count
  gem_intout[3] := 1            -> marker type count
  gem_intout[4] := 1            -> marker height count
  gem_intout[5] := 1            -> text font count
  gem_intout[6] := 4            -> colour count (4 planes = 16 colours)
  gem_intout[7] := 1            -> fill type count
  gem_intout[8] := 1            -> fill index count / pattern count
  gem_intout[9] := 3            -> preloaded patterns
  gem_intout[10] := 0           -> text rotation count
  gem_intout[11] := 3           -> colour model (0=indexed, 3=RGB)
  gem_intout[12] := vdi_dev_w  -> device width in pixels
  gem_intout[13] := vdi_dev_h  -> device height in pixels
  gem_intout[14] := vdi_dev_w / 8 -> device cell width in pixels
  gem_intout[15] := vdi_dev_h / 16 -> device cell height in pixels
  gem_intout[16] := 0           -> x dpi
  gem_intout[17] := 0           -> y dpi
  gem_intout[18] := 1           -> text effects
  gem_intout[19] := 0           -> min character width
  gem_intout[20] := 0           -> max character width
  gem_intout[21] := 0           -> min character height (below baseline)
  gem_intout[22] := 0           -> max character height (above baseline)
  gem_intout[23] := 0           -> min kerning offset
  gem_intout[24] := 0           -> max kerning offset
  gem_intout[25] := 0           -> number of font entries
  ctx[0] := vdi_handle
ENDPROC

-> Close workstation
PROC vdi_clsvwk()
  vdi_handle := -1
  ctx[0] := 1
ENDPROC

-> Clear workstation (fill with colour index 0)
PROC vdi_clrwk()
  ctx[0] := 1
ENDPROC

-> Update workstation (flush pending drawing)
PROC vdi_updwk()
  ctx[0] := 1
ENDPROC

-> v_pline - Draw polyline (series of connected points)
-> ptsin = array of (x,y) coordinate pairs, count in ptsin_count
-> Attributes: line type, line width, line colour
PROC vdi_pline()
  DEF pts, i, rp_ptr
  pts := gem_control[2]
  IF pts < 2 THEN pts := 2
  rp_ptr := gem_GetVDIRastPort()
  IF rp_ptr <> 0
    NATIVE {
      struct RastPort *rp = (struct RastPort *)rp_ptr;
      extern long vdi_line_color;
      extern long vdi_wr_mode;
      extern long vdi_line_type;
      extern long vdi_line_pats[6];
      extern long vdi_pen_map[16];
      extern long gem_ptrin[16];
      int pen = (int)vdi_line_color;
      if (pen < 0) pen = 0; if (pen > 15) pen = 15;
      SetAPen(rp, vdi_pen_map[pen]);
      switch ((int)vdi_wr_mode) {
        case 1: SetDrMd(rp, JAM1); break;
        case 2: SetDrMd(rp, JAM2); break;
        case 3: SetDrMd(rp, INVERS_XOR); break;
        default: SetDrMd(rp, JAM1); break;
      }
      { int lt = (int)vdi_line_type;
        if (lt >= 0 && lt < 6) SetDrPt(rp, vdi_line_pats[lt]);
      }
      Move(rp, (long)gem_ptrin[0], (long)gem_ptrin[1]);
      { int j;
        for (j = 1; j < (int)pts; j++) {
          Draw(rp, (long)gem_ptrin[j * 2], (long)gem_ptrin[j * 2 + 1]);
        }
      }
    } ENDNATIVE
  ENDIF
  vdi_cur_x := gem_ptrin[(pts - 1) * 2]
  vdi_cur_y := gem_ptrin[(pts - 1) * 2 + 1]
  ctx[0] := 1
ENDPROC

-> v_pmarker - Draw marker symbols at each point
-> ptsin = array of (x,y) coordinate pairs
PROC vdi_pmarker()
  DEF pts, rp_ptr
  pts := gem_control[2]
  IF pts > 0
    rp_ptr := gem_GetVDIRastPort()
    IF rp_ptr <> 0
      NATIVE {
        struct RastPort *rp = (struct RastPort *)rp_ptr;
        extern long vdi_marker_color;
        extern long vdi_marker_height;
        extern long vdi_pen_map[16];
        extern long gem_ptrin[16];
        int pen = (int)vdi_marker_color;
        int j, sz = (int)vdi_marker_height / 2;
        if (pen < 0) pen = 0; if (pen > 15) pen = 15;
        SetAPen(rp, vdi_pen_map[pen]);
        for (j = 0; j < (int)pts; j++) {
          long mx = gem_ptrin[j * 2], my = gem_ptrin[j * 2 + 1];
          Move(rp, mx - sz, my); Draw(rp, mx + sz, my);
          Move(rp, mx, my - sz); Draw(rp, mx, my + sz);
        }
      } ENDNATIVE
    ENDIF
    vdi_cur_x := gem_ptrin[(pts - 1) * 2]
    vdi_cur_y := gem_ptrin[(pts - 1) * 2 + 1]
  ENDIF
  ctx[0] := 1
ENDPROC

-> v_gtext - Draw graphics text at position
-> Position in ptsin[0], ptsin[1]; text in intin (null-terminated)
PROC vdi_gtext()
  DEF rp_ptr, len, j
  rp_ptr := gem_GetVDIRastPort()
  IF rp_ptr <> 0
    len := gem_control[0]
    IF len > 64 THEN len := 64
    NATIVE {
      struct RastPort *rp = (struct RastPort *)rp_ptr;
      extern long vdi_text_color;
      extern long vdi_wr_mode;
      extern long vdi_pen_map[16];
      extern long gem_ptrin[16];
      extern long gem_intin[128];
      char txt[65];
      int pen = (int)vdi_text_color;
      int k, tlen = (int)len;
      if (pen < 0) pen = 0; if (pen > 15) pen = 15;
      SetAPen(rp, vdi_pen_map[pen]);
      SetDrMd(rp, JAM2);
      for (k = 0; k < tlen; k++) {
        unsigned char ch = (unsigned char)((long)gem_intin[k]);
        if (ch == 0) break;
        txt[k] = ch;
      }
      txt[k] = 0;
      Move(rp, (long)gem_ptrin[0], (long)gem_ptrin[1]);
      Text(rp, txt, k);
    } ENDNATIVE
  ENDIF
  vdi_cur_x := gem_ptrin[0]
  vdi_cur_y := gem_ptrin[1]
  ctx[0] := 1
ENDPROC

-> v_fillarea - Draw filled polygon
-> ptsin = array of vertex (x,y) coordinates
PROC vdi_fillarea()
  DEF pts, rp_ptr
  pts := gem_control[2]
  IF pts >= 3
    rp_ptr := gem_GetVDIRastPort()
    IF rp_ptr <> 0
      NATIVE {
        struct RastPort *rp = (struct RastPort *)rp_ptr;
        extern long vdi_fill_color;
        extern long vdi_pen_map[16];
        extern long gem_ptrin[16];
        int pen = (int)vdi_fill_color;
        int j, n = (int)pts;
        if (pen < 0) pen = 0; if (pen > 15) pen = 15;
        SetAPen(rp, vdi_pen_map[pen]);
        Move(rp, (long)gem_ptrin[0], (long)gem_ptrin[1]);
        for (j = 1; j < n; j++)
          Draw(rp, (long)gem_ptrin[j * 2], (long)gem_ptrin[j * 2 + 1]);
        Draw(rp, (long)gem_ptrin[0], (long)gem_ptrin[1]);
      } ENDNATIVE
    ENDIF
    vdi_cur_x := gem_ptrin[(pts - 1) * 2]
    vdi_cur_y := gem_ptrin[(pts - 1) * 2 + 1]
  ENDIF
  ctx[0] := 1
ENDPROC

-> v_bar - Draw filled rectangle (bar)
-> ptsin[0..1] = top-left, ptsin[2..3] = bottom-right
PROC vdi_bar()
  DEF rp_ptr
  rp_ptr := gem_GetVDIRastPort()
  IF rp_ptr <> 0
    NATIVE {
      struct RastPort *rp = (struct RastPort *)rp_ptr;
      extern long vdi_fill_color;
      extern long vdi_pen_map[16];
      extern long gem_ptrin[16];
      int pen = (int)vdi_fill_color;
      if (pen < 0) pen = 0; if (pen > 15) pen = 15;
      SetAPen(rp, vdi_pen_map[pen]);
      RectFill(rp, (long)gem_ptrin[0], (long)gem_ptrin[1],
                   (long)gem_ptrin[2], (long)gem_ptrin[3]);
    } ENDNATIVE
  ENDIF
  ctx[0] := 1
ENDPROC

-> v_circle - Draw circle
-> ptsin[0], ptsin[1] = centre; intin[0] = radius
PROC vdi_circle()
  DEF rp_ptr
  rp_ptr := gem_GetVDIRastPort()
  IF rp_ptr <> 0
    NATIVE {
      struct RastPort *rp = (struct RastPort *)rp_ptr;
      extern long vdi_line_color;
      extern long vdi_wr_mode;
      extern long vdi_line_type;
      extern long vdi_line_pats[6];
      extern long vdi_pen_map[16];
      extern long gem_ptrin[16];
      extern long gem_intin[128];
      long cx = gem_ptrin[0], cy = gem_ptrin[1];
      long r = gem_intin[0];
      int pen = (int)vdi_line_color;
      int angle;
      if (pen < 0) pen = 0; if (pen > 15) pen = 15;
      SetAPen(rp, vdi_pen_map[pen]);
      switch ((int)vdi_wr_mode) {
        case 1: SetDrMd(rp, JAM1); break;
        case 2: SetDrMd(rp, JAM2); break;
        case 3: SetDrMd(rp, INVERS_XOR); break;
        default: SetDrMd(rp, JAM1); break;
      }
      { int lt = (int)vdi_line_type;
        if (lt >= 0 && lt < 6) SetDrPt(rp, vdi_line_pats[lt]);
      }
      for (angle = 0; angle <= 360; angle += 15) {
        double rad = angle * 3.14159265 / 180.0;
        long px = cx + (long)(r * cos(rad));
        long py = cy + (long)(r * sin(rad));
        if (angle == 0) Move(rp, px, py);
        else Draw(rp, px, py);
      }
    } ENDNATIVE
  ENDIF
  vdi_cur_x := gem_ptrin[0] + gem_intin[0]
  vdi_cur_y := gem_ptrin[1]
  ctx[0] := 1
ENDPROC

-> v_ellipse - Draw ellipse
-> ptsin[0], ptsin[1] = centre; intin[0] = x radius; intin[1] = y radius
PROC vdi_ellipse()
  DEF rp_ptr
  rp_ptr := gem_GetVDIRastPort()
  IF rp_ptr <> 0
    NATIVE {
      struct RastPort *rp = (struct RastPort *)rp_ptr;
      extern long vdi_line_color;
      extern long vdi_wr_mode;
      extern long vdi_line_type;
      extern long vdi_line_pats[6];
      extern long vdi_pen_map[16];
      extern long gem_ptrin[16];
      extern long gem_intin[128];
      long cx = gem_ptrin[0], cy = gem_ptrin[1];
      long rx = gem_intin[0], ry = gem_intin[1];
      int pen = (int)vdi_line_color;
      int angle;
      if (pen < 0) pen = 0; if (pen > 15) pen = 15;
      SetAPen(rp, vdi_pen_map[pen]);
      switch ((int)vdi_wr_mode) {
        case 1: SetDrMd(rp, JAM1); break;
        case 2: SetDrMd(rp, JAM2); break;
        case 3: SetDrMd(rp, INVERS_XOR); break;
        default: SetDrMd(rp, JAM1); break;
      }
      { int lt = (int)vdi_line_type;
        if (lt >= 0 && lt < 6) SetDrPt(rp, vdi_line_pats[lt]);
      }
      for (angle = 0; angle <= 360; angle += 15) {
        double rad = angle * 3.14159265 / 180.0;
        long px = cx + (long)(rx * cos(rad));
        long py = cy + (long)(ry * sin(rad));
        if (angle == 0) Move(rp, px, py);
        else Draw(rp, px, py);
      }
    } ENDNATIVE
  ENDIF
  vdi_cur_x := gem_ptrin[0] + gem_intin[0]
  vdi_cur_y := gem_ptrin[1]
  ctx[0] := 1
ENDPROC

-> v_ellarc - Draw elliptical arc
-> ptsin[0..1] = centre; intin[0] = x radius; intin[1] = y radius
-> intin[2] = start angle; intin[3] = end angle (in tenths of degrees)
PROC vdi_ellarc()
  DEF rp_ptr
  rp_ptr := gem_GetVDIRastPort()
  IF rp_ptr <> 0
    NATIVE {
      struct RastPort *rp = (struct RastPort *)rp_ptr;
      extern long vdi_line_color;
      extern long vdi_wr_mode;
      extern long vdi_line_type;
      extern long vdi_line_pats[6];
      extern long vdi_pen_map[16];
      extern long gem_ptrin[16];
      extern long gem_intin[128];
      long cx = gem_ptrin[0], cy = gem_ptrin[1];
      long rx = gem_intin[0], ry = gem_intin[1];
      int sa = (int)gem_intin[2] / 10;
      int ea = (int)gem_intin[3] / 10;
      int pen = (int)vdi_line_color;
      int a;
      if (pen < 0) pen = 0; if (pen > 15) pen = 15;
      SetAPen(rp, vdi_pen_map[pen]);
      switch ((int)vdi_wr_mode) {
        case 1: SetDrMd(rp, JAM1); break;
        case 2: SetDrMd(rp, JAM2); break;
        case 3: SetDrMd(rp, INVERS_XOR); break;
        default: SetDrMd(rp, JAM1); break;
      }
      { int lt = (int)vdi_line_type;
        if (lt >= 0 && lt < 6) SetDrPt(rp, vdi_line_pats[lt]);
      }
      for (a = sa; a <= ea; a += 15) {
        double rad = a * 3.14159265 / 180.0;
        long px = cx + (long)(rx * cos(rad));
        long py = cy + (long)(ry * sin(rad));
        if (a == sa) Move(rp, px, py);
        else Draw(rp, px, py);
      }
    } ENDNATIVE
  ENDIF
  ctx[0] := 1
ENDPROC

-> v_ellpie - Draw elliptical pie slice
-> same params as ellarc but filled to centre
PROC vdi_ellpie()
  DEF rp_ptr
  rp_ptr := gem_GetVDIRastPort()
  IF rp_ptr <> 0
    NATIVE {
      struct RastPort *rp = (struct RastPort *)rp_ptr;
      extern long vdi_fill_color;
      extern long vdi_pen_map[16];
      extern long gem_ptrin[16];
      extern long gem_intin[128];
      long cx = gem_ptrin[0], cy = gem_ptrin[1];
      long rx = gem_intin[0], ry = gem_intin[1];
      int sa = (int)gem_intin[2] / 10;
      int ea = (int)gem_intin[3] / 10;
      int pen = (int)vdi_fill_color;
      int a;
      if (pen < 0) pen = 0; if (pen > 15) pen = 15;
      SetAPen(rp, vdi_pen_map[pen]);
      Move(rp, cx, cy);
      for (a = sa; a <= ea; a += 15) {
        double rad = a * 3.14159265 / 180.0;
        Draw(rp, cx + (long)(rx * cos(rad)), cy + (long)(ry * sin(rad)));
      }
      Draw(rp, cx, cy);
    } ENDNATIVE
  ENDIF
  ctx[0] := 1
ENDPROC

-> v_arc - Draw circular arc
-> ptsin[0..1] = centre; intin[0] = radius
-> intin[1] = start angle; intin[2] = end angle (in tenths of degrees)
PROC vdi_arc()
  DEF rp_ptr
  rp_ptr := gem_GetVDIRastPort()
  IF rp_ptr <> 0
    NATIVE {
      struct RastPort *rp = (struct RastPort *)rp_ptr;
      extern long vdi_line_color;
      extern long vdi_wr_mode;
      extern long vdi_line_type;
      extern long vdi_line_pats[6];
      extern long vdi_pen_map[16];
      extern long gem_ptrin[16];
      extern long gem_intin[128];
      long cx = gem_ptrin[0], cy = gem_ptrin[1];
      long r = gem_intin[0];
      int sa = (int)gem_intin[1] / 10;
      int ea = (int)gem_intin[2] / 10;
      int pen = (int)vdi_line_color;
      int a;
      if (pen < 0) pen = 0; if (pen > 15) pen = 15;
      SetAPen(rp, vdi_pen_map[pen]);
      switch ((int)vdi_wr_mode) {
        case 1: SetDrMd(rp, JAM1); break;
        case 2: SetDrMd(rp, JAM2); break;
        case 3: SetDrMd(rp, INVERS_XOR); break;
        default: SetDrMd(rp, JAM1); break;
      }
      { int lt = (int)vdi_line_type;
        if (lt >= 0 && lt < 6) SetDrPt(rp, vdi_line_pats[lt]);
      }
      for (a = sa; a <= ea; a += 15) {
        double rad = a * 3.14159265 / 180.0;
        long px = cx + (long)(r * cos(rad));
        long py = cy + (long)(r * sin(rad));
        if (a == sa) Move(rp, px, py);
        else Draw(rp, px, py);
      }
    } ENDNATIVE
  ENDIF
  ctx[0] := 1
ENDPROC

-> v_pieslice - Draw circular pie slice
-> same as arc but filled to centre
PROC vdi_pieslice()
  DEF rp_ptr
  rp_ptr := gem_GetVDIRastPort()
  IF rp_ptr <> 0
    NATIVE {
      struct RastPort *rp = (struct RastPort *)rp_ptr;
      extern long vdi_fill_color;
      extern long vdi_pen_map[16];
      extern long gem_ptrin[16];
      extern long gem_intin[128];
      long cx = gem_ptrin[0], cy = gem_ptrin[1];
      long r = gem_intin[0];
      int sa = (int)gem_intin[1] / 10;
      int ea = (int)gem_intin[2] / 10;
      int pen = (int)vdi_fill_color;
      int a;
      if (pen < 0) pen = 0; if (pen > 15) pen = 15;
      SetAPen(rp, vdi_pen_map[pen]);
      Move(rp, cx, cy);
      for (a = sa; a <= ea; a += 15) {
        double rad = a * 3.14159265 / 180.0;
        Draw(rp, cx + (long)(r * cos(rad)), cy + (long)(r * sin(rad)));
      }
      Draw(rp, cx, cy);
    } ENDNATIVE
  ENDIF
  ctx[0] := 1
ENDPROC

-> v_rbox - Draw rounded rectangle (outline)
-> ptsin[0..1] = top-left; ptsin[2..3] = bottom-right
-> intin[0] = corner radius
PROC vdi_rbox()
  DEF rp_ptr
  rp_ptr := gem_GetVDIRastPort()
  IF rp_ptr <> 0
    NATIVE {
      struct RastPort *rp = (struct RastPort *)rp_ptr;
      extern long vdi_line_color;
      extern long vdi_wr_mode;
      extern long vdi_line_type;
      extern long vdi_line_pats[6];
      extern long vdi_pen_map[16];
      extern long gem_ptrin[16];
      extern long gem_intin[128];
      long x1 = gem_ptrin[0], y1 = gem_ptrin[1];
      long x2 = gem_ptrin[2], y2 = gem_ptrin[3];
      long cr = gem_intin[0];
      int pen = (int)vdi_line_color;
      int a;
      if (cr < 1) cr = 1;
      if (pen < 0) pen = 0; if (pen > 15) pen = 15;
      SetAPen(rp, vdi_pen_map[pen]);
      switch ((int)vdi_wr_mode) {
        case 1: SetDrMd(rp, JAM1); break;
        case 2: SetDrMd(rp, JAM2); break;
        case 3: SetDrMd(rp, INVERS_XOR); break;
        default: SetDrMd(rp, JAM1); break;
      }
      { int lt = (int)vdi_line_type;
        if (lt >= 0 && lt < 6) SetDrPt(rp, vdi_line_pats[lt]);
      }
      /* top-left corner */
      for (a = 180; a <= 270; a += 15) {
        double rad = a * 3.14159265 / 180.0;
        long px = (x1 + cr) + (long)(cr * cos(rad));
        long py = (y1 + cr) + (long)(cr * sin(rad));
        if (a == 180) Move(rp, px, py);
        else Draw(rp, px, py);
      }
      /* top-right corner */
      for (a = 270; a <= 360; a += 15) {
        double rad = a * 3.14159265 / 180.0;
        long px = (x2 - cr) + (long)(cr * cos(rad));
        long py = (y1 + cr) + (long)(cr * sin(rad));
        Draw(rp, px, py);
      }
      /* bottom-right corner */
      for (a = 0; a <= 90; a += 15) {
        double rad = a * 3.14159265 / 180.0;
        long px = (x2 - cr) + (long)(cr * cos(rad));
        long py = (y2 - cr) + (long)(cr * sin(rad));
        Draw(rp, px, py);
      }
      /* bottom-left corner */
      for (a = 90; a <= 180; a += 15) {
        double rad = a * 3.14159265 / 180.0;
        long px = (x1 + cr) + (long)(cr * cos(rad));
        long py = (y2 - cr) + (long)(cr * sin(rad));
        Draw(rp, px, py);
      }
      Draw(rp, (x1 + cr), y1);
    } ENDNATIVE
  ENDIF
  ctx[0] := 1
ENDPROC

-> v_rfbox - Draw filled rounded rectangle
-> same as rbox but filled
PROC vdi_rfbox()
  DEF rp_ptr
  rp_ptr := gem_GetVDIRastPort()
  IF rp_ptr <> 0
    NATIVE {
      struct RastPort *rp = (struct RastPort *)rp_ptr;
      extern long vdi_fill_color;
      extern long vdi_pen_map[16];
      extern long gem_ptrin[16];
      long x1 = gem_ptrin[0], y1 = gem_ptrin[1];
      long x2 = gem_ptrin[2], y2 = gem_ptrin[3];
      int pen = (int)vdi_fill_color;
      if (pen < 0) pen = 0; if (pen > 15) pen = 15;
      SetAPen(rp, vdi_pen_map[pen]);
      RectFill(rp, x1, y1, x2, y2);
    } ENDNATIVE
  ENDIF
  ctx[0] := 1
ENDPROC

-> v_justified - Draw justified text
-> Position in ptsin[0..1]; text in intin; intin_len in control[0]
-> intin[1] = word spacing; intin[2] = char spacing (in 1/8 em)
PROC vdi_justified()
  DEF rp_ptr, len, j
  rp_ptr := gem_GetVDIRastPort()
  IF rp_ptr <> 0
    len := gem_control[0]
    IF len > 64 THEN len := 64
    NATIVE {
      struct RastPort *rp = (struct RastPort *)rp_ptr;
      extern long vdi_text_color;
      extern long vdi_pen_map[16];
      extern long gem_ptrin[16];
      extern long gem_intin[128];
      char txt[65];
      int pen = (int)vdi_text_color;
      int k, tlen = (int)len;
      if (pen < 0) pen = 0; if (pen > 15) pen = 15;
      SetAPen(rp, vdi_pen_map[pen]);
      SetDrMd(rp, JAM2);
      for (k = 0; k < tlen; k++) {
        unsigned char ch = (unsigned char)((long)gem_intin[k]);
        if (ch == 0) break;
        txt[k] = ch;
      }
      txt[k] = 0;
      Move(rp, (long)gem_ptrin[0], (long)gem_ptrin[1]);
      Text(rp, txt, k);
    } ENDNATIVE
  ENDIF
  vdi_cur_x := gem_ptrin[0]
  vdi_cur_y := gem_ptrin[1]
  ctx[0] := 1
ENDPROC

-> v_cellarray - Draw rectangular block of pixel cells
-> ptsin[0..1] = top-left; ptsin[2..3] = bottom-right
-> intin contains pixel colours (row by row)
PROC vdi_cellarray()
  DEF w, h, rp_ptr
  w := gem_ptrin[2] - gem_ptrin[0]
  IF w < 0
    w := -w
  ENDIF
  w := w + 1
  h := gem_ptrin[3] - gem_ptrin[1]
  IF h < 0
    h := -h
  ENDIF
  h := h + 1
  rp_ptr := gem_GetVDIRastPort()
  IF rp_ptr <> 0 AND w > 0 AND h > 0
    NATIVE {
      struct RastPort *rp = (struct RastPort *)rp_ptr;
      extern long vdi_pen_map[16];
      extern long gem_ptrin[16];
      extern long gem_intin[128];
      long x1 = gem_ptrin[0], y1 = gem_ptrin[1];
      int wi = (int)w, hi = (int)h;
      int px, py;
      for (py = 0; py < hi; py++) {
        for (px = 0; px < wi; px++) {
          int idx = py * wi + px;
          int col = (idx < 128) ? (int)gem_intin[idx] : 0;
          if (col < 0) col = 0; if (col > 15) col = 15;
          SetAPen(rp, vdi_pen_map[col]);
          WritePixel(rp, x1 + px, y1 + py);
        }
      }
    } ENDNATIVE
  ENDIF
  vdi_cur_x := gem_ptrin[2]
  vdi_cur_y := gem_ptrin[3]
  ctx[0] := 1
ENDPROC

-> v_bezier - Draw Bezier curve
-> ptsin = control points; intin[0] = number of points
PROC vdi_bezier()
  ctx[0] := 1
ENDPROC

-> vq_color - Inquire colour representation
-> intin[0] = colour index; intin[1] = flag (0=get RGB)
-> Returns RGB in intout[0..2]
PROC vdi_qcolor()
  DEF idx
  idx := gem_intin[0]
  IF idx >= 0 AND idx <= 15
    gem_intout[0] := vdi_rgb[idx * 3] * 1000 / 255
    gem_intout[1] := vdi_rgb[idx * 3 + 1] * 1000 / 255
    gem_intout[2] := vdi_rgb[idx * 3 + 2] * 1000 / 255
  ELSE
    gem_intout[0] := 0; gem_intout[1] := 0; gem_intout[2] := 0
  ENDIF
  ctx[0] := 1
ENDPROC

-> vq_curpos - Inquire graphics cursor position
PROC vdi_qcurpos()
  gem_intout[0] := vdi_cur_x
  gem_intout[1] := vdi_cur_y
  ctx[0] := 1
ENDPROC

-> vq_contxt - Inquire current context (VDI attributes)
PROC vdi_qcontxt()
  gem_intout[0] := vdi_line_type
  gem_intout[1] := vdi_line_width
  gem_intout[2] := vdi_line_color
  gem_intout[3] := vdi_marker_type
  gem_intout[4] := vdi_marker_height
  gem_intout[5] := vdi_marker_color
  gem_intout[6] := vdi_text_font
  gem_intout[7] := vdi_text_color
  gem_intout[8] := vdi_fill_type
  gem_intout[9] := vdi_fill_index
  gem_intout[10] := vdi_fill_color
  gem_intout[11] := vdi_wr_mode
  ctx[0] := 1
ENDPROC

-> vq_extnd - Inquire extended device capabilities
-> intin[0] = device handle (0 = current)
PROC vdi_qextnd()
  gem_intout[0] := 0 -> device id
  gem_intout[1] := 0 -> flags
  gem_intout[2] := 0 -> colour capabilities
  gem_intout[3] := 0 -> tiling capabilities
  gem_intout[4] := 0 -> reserved
  gem_intout[5] := 0
  gem_intout[6] := 0
  gem_intout[7] := 0
  gem_intout[8] := 0
  gem_intout[9] := 0
  gem_intout[10] := 0
  gem_intout[11] := 0
  ctx[0] := 1
ENDPROC

-> vq_cellht - Inquire cell height
PROC vdi_qcellht()
  gem_intout[0] := vdi_work_h / 24
  ctx[0] := 1
ENDPROC

-> vq_cellwd - Inquire cell width
PROC vdi_qcellwd()
  gem_intout[0] := vdi_work_w / 40
  ctx[0] := 1
ENDPROC

-> vq_chcells - Inquire number of character cells
PROC vdi_qchcells()
  gem_intout[0] := 40 -> columns
  gem_intout[1] := 25 -> rows
  ctx[0] := 1
ENDPROC

-> vq_vgd - Inquire VGD (Virtual Graphics Device) capabilities
PROC vdi_qvgd()
  gem_intout[0] := 1 -> VDI version
  gem_intout[1] := 0 -> sub-version
  gem_intout[2] := 1 -> colour (0=mono, 1=colour)
  gem_intout[3] := vdi_n_planes
  gem_intout[4] := vdi_dev_w
  gem_intout[5] := vdi_dev_h
  gem_intout[6] := vdi_dev_w * vdi_n_planes / 8 -> bytes per line
  gem_intout[7] := 0 -> screen base (emulated)
  ctx[0] := 1
ENDPROC

-> vq_key_s - Inquire key shift status (always returns 0 in emulator)
PROC vdi_qkey_s()
  gem_intout[0] := 0
  ctx[0] := 1
ENDPROC

-> v_opnwk — alternate open workstation (function 2)
PROC vdi_opnwk()
  vdi_opnvwk()
ENDPROC

-> Set VDI attribute functions based on intin values
-> vsl_type (line type), vsl_width (line width), vsl_color (line colour)
-> vsf_type (fill type), vsf_index (fill index), vsf_color (fill colour)
-> etc.
PROC vdi_set_line_type()
  vdi_line_type := gem_intin[0]
  ctx[0] := 1
ENDPROC
PROC vdi_set_line_width()
  vdi_line_width := gem_intin[0]
  ctx[0] := 1
ENDPROC
PROC vdi_set_line_color()
  vdi_line_color := gem_intin[0]
  ctx[0] := 1
ENDPROC
PROC vdi_set_fill_type()
  vdi_fill_type := gem_intin[0]
  IF gem_intin[1] <> 0 THEN vdi_fill_index := gem_intin[1]
  ctx[0] := 1
ENDPROC
PROC vdi_set_fill_index()
  vdi_fill_index := gem_intin[0]
  ctx[0] := 1
ENDPROC
PROC vdi_set_fill_color()
  vdi_fill_color := gem_intin[0]
  ctx[0] := 1
ENDPROC
PROC vdi_set_marker_type()
  vdi_marker_type := gem_intin[0]
  ctx[0] := 1
ENDPROC
PROC vdi_set_marker_height()
  vdi_marker_height := gem_intin[0]
  ctx[0] := 1
ENDPROC
PROC vdi_set_marker_color()
  vdi_marker_color := gem_intin[0]
  ctx[0] := 1
ENDPROC
PROC vdi_set_text_font()
  vdi_text_font := gem_intin[0]
  ctx[0] := 1
ENDPROC
PROC vdi_set_text_color()
  vdi_text_color := gem_intin[0]
  ctx[0] := 1
ENDPROC
PROC vdi_set_text_rotation()
  vdi_text_rotation := gem_intin[0]
  ctx[0] := 1
ENDPROC
PROC vdi_set_writing_mode()
  vdi_wr_mode := gem_intin[0]
  ctx[0] := 1
ENDPROC
PROC vdi_set_clip_rect()
  vdi_clip_x := gem_intin[0]
  vdi_clip_y := gem_intin[1]
  vdi_clip_w := gem_intin[2]
  vdi_clip_h := gem_intin[3]
  ctx[0] := 1
ENDPROC
PROC vdi_set_clip_state()
  -> intin[0] = 0 (off), 1 (on)
  ctx[0] := 1
ENDPROC
PROC vdi_set_curpos()
  vdi_cur_x := gem_intin[0]
  vdi_cur_y := gem_intin[1]
  ctx[0] := 1
ENDPROC

-> VDI inquiry for fill area patterns
PROC vdi_inq_fill_pats()
  gem_intout[0] := 3
  ctx[0] := 1
ENDPROC


-> ---------------------------------------------------------------------------
-> GEM VDI (Virtual Device Interface) dispatch
-> Called via BIOS trap #2 with D0 = $C9
-> Maps GEM VDI drawing calls to AmigaOS graphics.library
-> ctx[1] = VDI function number
-> ---------------------------------------------------------------------------
PROC gem_vdi_dispatch()
  DEF fn
  fn := ctx[1]

  SELECT fn

  CASE 1  -> v_clsvwk - Close workstation
    vdi_clsvwk()
  CASE 2  -> v_opnwk - Open workstation
    vdi_opnwk()
  CASE 5  -> v_opnvwk - Open virtual workstation
    vdi_opnvwk()
  CASE 6  -> v_clsvwk - Close virtual workstation
    vdi_clsvwk()
  CASE 7  -> v_clrwk - Clear workstation
    vdi_clrwk()
  CASE 8  -> v_updwk - Update workstation
    vdi_updwk()

  -> Drawing primitives
  CASE 11 -> v_pline
    vdi_pline()
  CASE 12 -> v_pmarker
    vdi_pmarker()
  CASE 13 -> v_gtext
    vdi_gtext()
  CASE 14 -> v_fillarea
    vdi_fillarea()
  CASE 15 -> v_ellipse
    vdi_ellipse()
  CASE 16 -> v_arc
    vdi_arc()
  CASE 17 -> v_pieslice
    vdi_pieslice()
  CASE 18 -> v_circle
    vdi_circle()
  CASE 19 -> v_ellarc
    vdi_ellarc()
  CASE 20 -> v_ellpie
    vdi_ellpie()
  CASE 21 -> v_rbox
    vdi_rbox()
  CASE 22 -> v_rfbox
    vdi_rfbox()
  CASE 23 -> v_bar
    vdi_bar()
  CASE 24 -> v_justified
    vdi_justified()

  -> Attributes
  CASE 32 -> vsl_type
    vdi_set_line_type()
  CASE 33 -> vsl_width
    vdi_set_line_width()
  CASE 34 -> vsl_color
    vdi_set_line_color()
  CASE 35 -> vsf_type
    vdi_set_fill_type()
  CASE 36 -> vsf_index
    vdi_set_fill_index()
  CASE 37 -> vsf_color
    vdi_set_fill_color()
  CASE 38 -> vsm_type
    vdi_set_marker_type()
  CASE 39 -> vsm_height
    vdi_set_marker_height()
  CASE 40 -> vsm_color
    vdi_set_marker_color()
  CASE 41 -> vst_font
    vdi_set_text_font()
  CASE 42 -> vst_color
    vdi_set_text_color()
  CASE 43 -> vst_rotation
    vdi_set_text_rotation()
  CASE 44 -> vswr_mode
    vdi_set_writing_mode()
  CASE 45 -> vsl_pattern - line pattern (obsolete)
    ctx[0] := 1
  CASE 46 -> vs_clip - set clipping rectangle
    vdi_set_clip_rect()
  CASE 47 -> vs_clip - set clipping state (0=off, 1=on)
    vdi_set_clip_state()
  CASE 48 -> vs_curpos - set graphics cursor position
    vdi_set_curpos()

  -> Inquiry
  CASE 10 -> vq_color
    vdi_qcolor()
  CASE 26 -> vq_curpos
    vdi_qcurpos()
  CASE 27 -> vq_contxt
    vdi_qcontxt()
  CASE 30 -> vq_extnd
    vdi_qextnd()
  CASE 31 -> vq_key_s
    vdi_qkey_s()
  CASE 36 -> vq_cellht
    vdi_qcellht()
  CASE 37 -> vq_cellwd
    vdi_qcellwd()
  CASE 38 -> vq_chcells
    vdi_qchcells()
  CASE 120 -> vq_vgd
    vdi_qvgd()

  CASE 100 -> v_opnvwk (alternate)
    vdi_opnvwk()
  CASE 101 -> v_cellarray
    vdi_cellarray()
  CASE 107 -> v_bezier
    vdi_bezier()
  CASE 109 -> v_inq_fill_pats - inquire fill patterns
    vdi_inq_fill_pats()

  DEFAULT
    -> All other VDI functions return success
    ctx[0] := 1
  ENDSELECT
ENDPROC


-> ---------------------------------------------------------------------------
-> Atari ST System Font Registration
-> Registers the Atari ST character set as AmigaOS fonts via AddFont()
-> 8x16 font for high-resolution (640x400), 8x8 font for low/medium
-> Uses the Atari ST character encoding:
->   0x00-0x1F: special graphic chars (arrows, icons, music notes)
->   0x20-0x7E: standard ASCII (matching ISO-8859-1 in printable range)
->   0x7F:     solid block / alternate DEL
->   0x80-0xFF: extended chars (accented Latin, Greek, math, Hebrew)
-> ---------------------------------------------------------------------------

-> Font registration status
DEF gem_font_8x16_registered, gem_font_8x8_registered

-> Atari ST 8x16 font bitmap data (256 chars × 16 bytes = 4096 bytes)
-> Generated from Atari ST ROM glyph definitions
DEF gem_font_data_8x16[4096]:ARRAY OF CHAR

-> Atari ST 8x8 font bitmap data (256 chars × 8 bytes = 2048 bytes)
DEF gem_font_data_8x8[2048]:ARRAY OF CHAR

-> Character width tables (all 8 for fixed-width)
DEF gem_font_width_8x16[256]:ARRAY OF CHAR
DEF gem_font_width_8x8[256]:ARRAY OF CHAR

-> Character location tables (WORD offsets into bitmap data)
DEF gem_font_loc_8x16[512]:ARRAY OF CHAR
DEF gem_font_loc_8x8[256]:ARRAY OF CHAR

-> TextFont structures allocated on the heap
DEF gem_font_8x16:PTR TO textfont
DEF gem_font_8x8:PTR TO textfont

-> Build and register an Atari ST 8x16 system font
PROC gem_init_font_8x16()
  DEF i

  -> Fill width table (all chars are 8 pixels wide)
  FOR i := 0 TO 255
    gem_font_width_8x16[i] := 8
  ENDFOR

  -> Fill location table (char N starts at N*16 bytes)
  FOR i := 0 TO 255
    gem_font_loc_8x16[i * 2] := asCHAR(i * 16)
    gem_font_loc_8x16[i * 2 + 1] := asCHAR(((i * 16) / 256))
  ENDFOR

  -> Atari ST 8x16 system font data (256 glyphs, 16 bytes each)
  -> Bit 7 = leftmost pixel, bit 0 = rightmost pixel
  -> Rows top to bottom; baseline at row 12 (3 rows below for descenders)
  NATIVE {
    unsigned char *f = (unsigned char *)gem_font_data_8x16;
    int c, r;

    static const unsigned char glyphs[256][16] = {

  /* 0x00-0x1F: Atari ST special graphics / control characters */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0x00 */
  {0x3C,0x7E,0xFF,0xFF,0xFF,0xFF,0x7E,0x7E,0x3C,0x3C,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0x01 heart */
  {0x0C,0x1E,0x3E,0x7C,0x7C,0x3E,0x1E,0x0C,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0x02 diamond */
  {0x18,0x3C,0x7E,0x7E,0x3C,0x3C,0x7E,0x7E,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0x03 club */
  {0x18,0x3C,0x7E,0x7E,0x3C,0x3C,0x5A,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0x04 spade */
  {0x00,0x18,0x18,0x18,0x7E,0x7E,0x18,0x18,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0x05 dot/center */
  {0x00,0x18,0x18,0x18,0xFF,0xFF,0x18,0x18,0x18,0x18,0xFF,0xFF,0x00,0x00,0x00,0x00}, /* 0x06 */
  {0x18,0x18,0x18,0x18,0x00,0x00,0x18,0x18,0x18,0x18,0x00,0x00,0x18,0x18,0x18,0x18}, /* 0x07 */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0x08 */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0x09 */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0x0A */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0x0B */
  {0xFE,0xFE,0xFE,0xFE,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0x0C */
  {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF}, /* 0x0D */ /* actually 0x0C - but 0x0D */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFE,0xFE,0xFE,0xFE,0x00,0x00,0x00,0x00}, /* 0x0E */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0x0F */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0x10 */
  {0x00,0x00,0x00,0x00,0xE0,0xF0,0x38,0x1C,0x0C,0x0E,0x07,0x03,0x00,0x00,0x00,0x00}, /* 0x11 corner BL */
  {0x00,0x00,0x00,0x00,0x07,0x0F,0x1C,0x38,0x30,0x70,0xE0,0xC0,0x00,0x00,0x00,0x00}, /* 0x12 corner BR */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0F,0x07,0x03,0x01}, /* 0x13 */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xF0,0xE0,0xC0,0x80}, /* 0x14 */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0F,0x07,0x03,0x01}, /* 0x15 */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xF0,0xE0,0xC0,0x80}, /* 0x16 */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0x17 */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0x18 */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0x19 */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0x1A */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0x1B */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0x1C */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0x1D */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0x1E */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0x1F */

  /* 0x20-0x7F: ASCII printable + DEL/square */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* ' ' */
  {0x00,0x00,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x00,0x18,0x18,0x00,0x00,0x00,0x00}, /* '!' */
  {0x00,0x00,0x66,0x66,0x66,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* '"' */
  {0x00,0x00,0x00,0x24,0x24,0x7E,0x24,0x24,0x24,0x7E,0x24,0x24,0x00,0x00,0x00,0x00}, /* '#' */
  {0x00,0x08,0x08,0x3E,0x6B,0x68,0x38,0x1E,0x0B,0x6B,0x3E,0x08,0x08,0x00,0x00,0x00}, /* '$' */
  {0x00,0x00,0x00,0x00,0x61,0xD2,0x64,0x08,0x10,0x26,0x4B,0x86,0x00,0x00,0x00,0x00}, /* '%' */
  {0x00,0x00,0x3C,0x66,0x66,0x3C,0x38,0x4D,0x46,0x46,0x46,0x3D,0x00,0x00,0x00,0x00}, /* '&' */
  {0x00,0x00,0x18,0x18,0x18,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* ''' */
  {0x00,0x00,0x06,0x0C,0x18,0x18,0x18,0x18,0x18,0x18,0x0C,0x06,0x00,0x00,0x00,0x00}, /* '(' */
  {0x00,0x00,0x60,0x30,0x18,0x18,0x18,0x18,0x18,0x18,0x30,0x60,0x00,0x00,0x00,0x00}, /* ')' */
  {0x00,0x00,0x00,0x18,0x18,0x7E,0x3C,0x3C,0x7E,0x18,0x18,0x00,0x00,0x00,0x00,0x00}, /* '*' */
  {0x00,0x00,0x00,0x00,0x18,0x18,0x18,0xFF,0x18,0x18,0x18,0x00,0x00,0x00,0x00,0x00}, /* '+' */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x1C,0x1C,0x08,0x10,0x00,0x00,0x00}, /* ',' */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* '-' */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00,0x00,0x00,0x00}, /* '.' */
  {0x00,0x00,0x00,0x02,0x04,0x08,0x08,0x10,0x10,0x20,0x20,0x40,0x00,0x00,0x00,0x00}, /* '/' */
  {0x00,0x00,0x3C,0x66,0x66,0x6E,0x76,0x66,0x66,0x66,0x66,0x3C,0x00,0x00,0x00,0x00}, /* '0' */
  {0x00,0x00,0x18,0x38,0x78,0x18,0x18,0x18,0x18,0x18,0x18,0x7E,0x00,0x00,0x00,0x00}, /* '1' */
  {0x00,0x00,0x3C,0x66,0x06,0x0C,0x18,0x30,0x30,0x60,0x66,0x7E,0x00,0x00,0x00,0x00}, /* '2' */
  {0x00,0x00,0x3C,0x66,0x06,0x06,0x1C,0x06,0x06,0x06,0x66,0x3C,0x00,0x00,0x00,0x00}, /* '3' */
  {0x00,0x00,0x0C,0x0C,0x1C,0x2C,0x4C,0x4C,0x7E,0x0C,0x0C,0x0C,0x00,0x00,0x00,0x00}, /* '4' */
  {0x00,0x00,0x7E,0x60,0x60,0x7C,0x06,0x06,0x06,0x06,0x66,0x3C,0x00,0x00,0x00,0x00}, /* '5' */
  {0x00,0x00,0x3C,0x66,0x60,0x60,0x7C,0x66,0x66,0x66,0x66,0x3C,0x00,0x00,0x00,0x00}, /* '6' */
  {0x00,0x00,0x7E,0x46,0x06,0x0C,0x0C,0x18,0x18,0x18,0x18,0x18,0x00,0x00,0x00,0x00}, /* '7' */
  {0x00,0x00,0x3C,0x66,0x66,0x66,0x3C,0x66,0x66,0x66,0x66,0x3C,0x00,0x00,0x00,0x00}, /* '8' */
  {0x00,0x00,0x3C,0x66,0x66,0x66,0x66,0x3E,0x06,0x06,0x66,0x3C,0x00,0x00,0x00,0x00}, /* '9' */
  {0x00,0x00,0x00,0x00,0x18,0x18,0x00,0x00,0x00,0x18,0x18,0x00,0x00,0x00,0x00,0x00}, /* ':' */
  {0x00,0x00,0x00,0x00,0x18,0x18,0x00,0x00,0x00,0x18,0x18,0x08,0x10,0x00,0x00,0x00}, /* ';' */
  {0x00,0x00,0x06,0x0C,0x18,0x30,0x60,0x30,0x18,0x0C,0x06,0x00,0x00,0x00,0x00,0x00}, /* '<' */
  {0x00,0x00,0x00,0x00,0x00,0x7E,0x00,0x7E,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* '=' */
  {0x00,0x00,0x60,0x30,0x18,0x0C,0x06,0x0C,0x18,0x30,0x60,0x00,0x00,0x00,0x00,0x00}, /* '>' */
  {0x00,0x00,0x3C,0x66,0x66,0x06,0x0C,0x18,0x18,0x00,0x18,0x18,0x00,0x00,0x00,0x00}, /* '?' */
  {0x00,0x00,0x3C,0x66,0x66,0x6E,0x6E,0x6E,0x6E,0x60,0x62,0x3C,0x00,0x00,0x00,0x00}, /* '@' */
  {0x00,0x00,0x3C,0x66,0x66,0x66,0x66,0x7E,0x66,0x66,0x66,0x66,0x00,0x00,0x00,0x00}, /* 'A' */
  {0x00,0x00,0x7C,0x66,0x66,0x66,0x7C,0x66,0x66,0x66,0x66,0x7C,0x00,0x00,0x00,0x00}, /* 'B' */
  {0x00,0x00,0x3C,0x66,0x62,0x60,0x60,0x60,0x60,0x62,0x66,0x3C,0x00,0x00,0x00,0x00}, /* 'C' */
  {0x00,0x00,0x78,0x6C,0x66,0x66,0x66,0x66,0x66,0x66,0x6C,0x78,0x00,0x00,0x00,0x00}, /* 'D' */
  {0x00,0x00,0x7E,0x60,0x60,0x60,0x7C,0x60,0x60,0x60,0x60,0x7E,0x00,0x00,0x00,0x00}, /* 'E' */
  {0x00,0x00,0x7E,0x60,0x60,0x60,0x7C,0x60,0x60,0x60,0x60,0x60,0x00,0x00,0x00,0x00}, /* 'F' */
  {0x00,0x00,0x3C,0x66,0x62,0x60,0x60,0x6E,0x66,0x66,0x66,0x3E,0x00,0x00,0x00,0x00}, /* 'G' */
  {0x00,0x00,0x66,0x66,0x66,0x66,0x7E,0x66,0x66,0x66,0x66,0x66,0x00,0x00,0x00,0x00}, /* 'H' */
  {0x00,0x00,0x3C,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x3C,0x00,0x00,0x00,0x00}, /* 'I' */
  {0x00,0x00,0x1E,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x6C,0x6C,0x38,0x00,0x00,0x00,0x00}, /* 'J' */
  {0x00,0x00,0x66,0x66,0x6C,0x6C,0x78,0x6C,0x6C,0x66,0x66,0x66,0x00,0x00,0x00,0x00}, /* 'K' */
  {0x00,0x00,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x7E,0x00,0x00,0x00,0x00}, /* 'L' */
  {0x00,0x00,0x63,0x63,0x77,0x77,0x7F,0x6B,0x6B,0x63,0x63,0x63,0x00,0x00,0x00,0x00}, /* 'M' */
  {0x00,0x00,0x66,0x66,0x76,0x76,0x6E,0x6E,0x66,0x66,0x66,0x66,0x00,0x00,0x00,0x00}, /* 'N' */
  {0x00,0x00,0x3C,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x3C,0x00,0x00,0x00,0x00}, /* 'O' */
  {0x00,0x00,0x7C,0x66,0x66,0x66,0x66,0x7C,0x60,0x60,0x60,0x60,0x00,0x00,0x00,0x00}, /* 'P' */
  {0x00,0x00,0x3C,0x66,0x66,0x66,0x66,0x66,0x66,0x6E,0x6C,0x3E,0x06,0x00,0x00,0x00}, /* 'Q' */
  {0x00,0x00,0x7C,0x66,0x66,0x66,0x66,0x7C,0x6C,0x66,0x66,0x66,0x00,0x00,0x00,0x00}, /* 'R' */
  {0x00,0x00,0x3C,0x66,0x60,0x60,0x3C,0x06,0x06,0x06,0x66,0x3C,0x00,0x00,0x00,0x00}, /* 'S' */
  {0x00,0x00,0x7E,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x00,0x00,0x00,0x00}, /* 'T' */
  {0x00,0x00,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x3C,0x00,0x00,0x00,0x00}, /* 'U' */
  {0x00,0x00,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x3C,0x3C,0x18,0x00,0x00,0x00,0x00}, /* 'V' */
  {0x00,0x00,0x63,0x63,0x63,0x63,0x6B,0x6B,0x7F,0x77,0x77,0x22,0x00,0x00,0x00,0x00}, /* 'W' */
  {0x00,0x00,0x66,0x66,0x66,0x3C,0x18,0x3C,0x66,0x66,0x66,0x66,0x00,0x00,0x00,0x00}, /* 'X' */
  {0x00,0x00,0x66,0x66,0x66,0x3C,0x18,0x18,0x18,0x18,0x18,0x18,0x00,0x00,0x00,0x00}, /* 'Y' */
  {0x00,0x00,0x7E,0x06,0x0C,0x0C,0x18,0x30,0x30,0x60,0x60,0x7E,0x00,0x00,0x00,0x00}, /* 'Z' */
  {0x00,0x00,0x3E,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x3E,0x00,0x00,0x00,0x00}, /* '[' */
  {0x00,0x00,0x00,0x40,0x20,0x10,0x10,0x08,0x08,0x04,0x04,0x02,0x00,0x00,0x00,0x00}, /* '\' */
  {0x00,0x00,0x7C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x7C,0x00,0x00,0x00,0x00}, /* ']' */
  {0x00,0x00,0x08,0x1C,0x36,0x63,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* '^' */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0x00,0x00}, /* '_' */
  {0x00,0x18,0x18,0x18,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* '`' */
  {0x00,0x00,0x00,0x00,0x00,0x3C,0x06,0x3E,0x66,0x66,0x66,0x3E,0x00,0x00,0x00,0x00}, /* 'a' */
  {0x00,0x00,0x60,0x60,0x60,0x7C,0x66,0x66,0x66,0x66,0x66,0x7C,0x00,0x00,0x00,0x00}, /* 'b' */
  {0x00,0x00,0x00,0x00,0x00,0x3C,0x66,0x60,0x60,0x60,0x66,0x3C,0x00,0x00,0x00,0x00}, /* 'c' */
  {0x00,0x00,0x06,0x06,0x06,0x3E,0x66,0x66,0x66,0x66,0x66,0x3E,0x00,0x00,0x00,0x00}, /* 'd' */
  {0x00,0x00,0x00,0x00,0x00,0x3C,0x66,0x66,0x7E,0x60,0x60,0x3C,0x00,0x00,0x00,0x00}, /* 'e' */
  {0x00,0x00,0x0E,0x18,0x18,0x7E,0x18,0x18,0x18,0x18,0x18,0x18,0x00,0x00,0x00,0x00}, /* 'f' */
  {0x00,0x00,0x00,0x00,0x00,0x3E,0x66,0x66,0x66,0x66,0x66,0x3E,0x06,0x66,0x3C,0x00}, /* 'g' */
  {0x00,0x00,0x60,0x60,0x60,0x7C,0x66,0x66,0x66,0x66,0x66,0x66,0x00,0x00,0x00,0x00}, /* 'h' */
  {0x00,0x00,0x18,0x18,0x00,0x38,0x18,0x18,0x18,0x18,0x18,0x3C,0x00,0x00,0x00,0x00}, /* 'i' */
  {0x00,0x00,0x0C,0x0C,0x00,0x1C,0x0C,0x0C,0x0C,0x0C,0x0C,0x6C,0x6C,0x38,0x00,0x00}, /* 'j' */
  {0x00,0x00,0x60,0x60,0x60,0x66,0x6C,0x78,0x78,0x6C,0x66,0x66,0x00,0x00,0x00,0x00}, /* 'k' */
  {0x00,0x00,0x38,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x3C,0x00,0x00,0x00,0x00}, /* 'l' */
  {0x00,0x00,0x00,0x00,0x00,0x3E,0x7F,0x6B,0x6B,0x6B,0x6B,0x6B,0x00,0x00,0x00,0x00}, /* 'm' */
  {0x00,0x00,0x00,0x00,0x00,0x7C,0x66,0x66,0x66,0x66,0x66,0x66,0x00,0x00,0x00,0x00}, /* 'n' */
  {0x00,0x00,0x00,0x00,0x00,0x3C,0x66,0x66,0x66,0x66,0x66,0x3C,0x00,0x00,0x00,0x00}, /* 'o' */
  {0x00,0x00,0x00,0x00,0x00,0x7C,0x66,0x66,0x66,0x66,0x66,0x7C,0x60,0x60,0x60,0x00}, /* 'p' */
  {0x00,0x00,0x00,0x00,0x00,0x3E,0x66,0x66,0x66,0x66,0x66,0x3E,0x06,0x06,0x06,0x00}, /* 'q' */
  {0x00,0x00,0x00,0x00,0x00,0x7C,0x66,0x60,0x60,0x60,0x60,0x60,0x00,0x00,0x00,0x00}, /* 'r' */
  {0x00,0x00,0x00,0x00,0x00,0x3C,0x66,0x60,0x3C,0x06,0x66,0x3C,0x00,0x00,0x00,0x00}, /* 's' */
  {0x00,0x00,0x18,0x18,0x18,0x7E,0x18,0x18,0x18,0x18,0x18,0x0E,0x00,0x00,0x00,0x00}, /* 't' */
  {0x00,0x00,0x00,0x00,0x00,0x66,0x66,0x66,0x66,0x66,0x66,0x3E,0x00,0x00,0x00,0x00}, /* 'u' */
  {0x00,0x00,0x00,0x00,0x00,0x66,0x66,0x66,0x66,0x66,0x3C,0x18,0x00,0x00,0x00,0x00}, /* 'v' */
  {0x00,0x00,0x00,0x00,0x00,0x63,0x63,0x6B,0x6B,0x7F,0x36,0x36,0x00,0x00,0x00,0x00}, /* 'w' */
  {0x00,0x00,0x00,0x00,0x00,0x66,0x66,0x3C,0x18,0x3C,0x66,0x66,0x00,0x00,0x00,0x00}, /* 'x' */
  {0x00,0x00,0x00,0x00,0x00,0x66,0x66,0x66,0x66,0x66,0x66,0x3E,0x06,0x66,0x3C,0x00}, /* 'y' */
  {0x00,0x00,0x00,0x00,0x00,0x7E,0x06,0x0C,0x18,0x30,0x60,0x7E,0x00,0x00,0x00,0x00}, /* 'z' */
  {0x00,0x00,0x0E,0x18,0x18,0x18,0x70,0x18,0x18,0x18,0x18,0x0E,0x00,0x00,0x00,0x00}, /* '{' */
  {0x00,0x00,0x18,0x18,0x18,0x18,0x00,0x18,0x18,0x18,0x18,0x18,0x00,0x00,0x00,0x00}, /* '|' */
  {0x00,0x00,0x70,0x18,0x18,0x18,0x0E,0x18,0x18,0x18,0x18,0x70,0x00,0x00,0x00,0x00}, /* '}' */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x62,0x64,0x08,0x13,0x23,0x00,0x00,0x00,0x00,0x00}, /* '~' */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0x7F */

  /* 0x80-0x9F: Atari ST extended graphics */
  {0x3C,0x66,0x60,0x60,0x60,0x60,0x3C,0x0C,0x0C,0x00,0x0C,0x0C,0x00,0x00,0x00,0x00}, /* 0x80 Ç */
  {0x00,0x66,0x66,0x66,0x66,0x66,0x66,0x3E,0x06,0x06,0x66,0x3C,0x00,0x00,0x00,0x00}, /* 0x81 ü */
  {0x00,0x7E,0x60,0x60,0x7C,0x66,0x66,0x66,0x66,0x66,0x66,0x7C,0x00,0x00,0x00,0x00}, /* 0x82 é */
  {0x00,0x00,0x00,0x00,0x00,0x3C,0x66,0x66,0x7E,0x60,0x60,0x3C,0x00,0x00,0x00,0x00}, /* 0x83 â */
  {0x00,0x00,0x00,0x00,0x00,0x3E,0x66,0x66,0x66,0x66,0x66,0x3E,0x06,0x66,0x3C,0x00}, /* 0x84 ä */
  {0x00,0x00,0x00,0x00,0x00,0x3C,0x66,0x60,0x60,0x60,0x66,0x3C,0x0C,0x18,0x00,0x00}, /* 0x85 à */
  {0x00,0x00,0x00,0x00,0x3C,0x66,0x60,0x60,0x7E,0x66,0x66,0x3C,0x00,0x00,0x00,0x00}, /* 0x86 å */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x3C,0x66,0x66,0x3C,0x00,0x00,0x00,0x00}, /* 0x87 ç */
  {0x00,0x00,0x3C,0x66,0x60,0x3C,0x66,0x66,0x66,0x3C,0x06,0x3C,0x00,0x00,0x00,0x00}, /* 0x88 ê */
  {0x00,0x00,0x00,0x00,0x00,0x3C,0x66,0x66,0x66,0x66,0x66,0x3C,0x00,0x00,0x00,0x00}, /* 0x89 ë */
  {0x00,0x00,0x00,0x3C,0x66,0x60,0x60,0x60,0x60,0x66,0x3C,0x0C,0x18,0x00,0x00,0x00}, /* 0x8A è */
  {0x00,0x00,0x00,0x00,0x00,0x3E,0x06,0x3E,0x66,0x66,0x66,0x3E,0x00,0x00,0x00,0x00}, /* 0x8B ï */
  {0x00,0x00,0x18,0x18,0x00,0x38,0x18,0x18,0x18,0x18,0x18,0x3C,0x00,0x00,0x00,0x00}, /* 0x8C ì */
  {0x00,0x00,0x3C,0x66,0x66,0x66,0x7E,0x66,0x66,0x66,0x66,0x3C,0x00,0x00,0x00,0x00}, /* 0x8D Ä */
  {0x00,0x00,0x7E,0x60,0x60,0x7C,0x66,0x66,0x66,0x66,0x66,0x7C,0x00,0x00,0x00,0x00}, /* 0x8E Å */
  {0x00,0x00,0x62,0x64,0x08,0x10,0x3C,0x66,0x66,0x7E,0x66,0x66,0x00,0x00,0x00,0x00}, /* 0x8F É */
  {0x00,0x00,0x00,0x00,0x00,0x3C,0x66,0x66,0x7E,0x60,0x66,0x3C,0x00,0x00,0x00,0x00}, /* 0x90 æ */
  {0x00,0x08,0x3E,0x6B,0x68,0x3E,0x0B,0x6B,0x3E,0x08,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0x91 Æ */
  {0x00,0x18,0x18,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0x92 ô */
  {0x00,0x3C,0x66,0x06,0x3C,0x60,0x66,0x3C,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0x93 ö */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x3E,0x66,0x66,0x66,0x66,0x3E,0x00,0x00,0x00,0x00}, /* 0x94 ò */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x3C,0x66,0x66,0x66,0x66,0x3C,0x0C,0x18,0x00,0x00}, /* 0x95 û */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x66,0x66,0x66,0x66,0x66,0x3E,0x06,0x66,0x3C,0x00}, /* 0x96 ù */
  {0x00,0x00,0x00,0x00,0x3C,0x66,0x66,0x3C,0x66,0x66,0x66,0x3C,0x00,0x00,0x00,0x00}, /* 0x97 ÿ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x66,0x66,0x66,0x66,0x66,0x3E,0x00,0x00,0x00,0x00}, /* 0x98 Ö */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x3C,0x66,0x60,0x60,0x66,0x3C,0x00,0x00,0x00,0x00}, /* 0x99 Ü */
  {0x00,0x00,0x00,0x00,0x18,0x00,0x7E,0x18,0x18,0x18,0x18,0x0E,0x00,0x00,0x00,0x00}, /* 0x9A ¢ */
  {0x00,0x00,0x00,0x00,0x00,0x1C,0x36,0x30,0x7C,0x30,0x36,0x1C,0x00,0x00,0x00,0x00}, /* 0x9B £ */
  {0x00,0x00,0x00,0x00,0x3C,0x66,0x66,0x7E,0x66,0x66,0x66,0x3C,0x00,0x00,0x00,0x00}, /* 0x9C ¥ */
  {0x00,0x00,0x00,0x00,0x66,0x66,0x3C,0x18,0x3C,0x66,0x66,0x66,0x00,0x00,0x00,0x00}, /* 0x9D ₧ */
  {0x00,0x00,0x3C,0x66,0x06,0x0C,0x18,0x30,0x60,0x60,0x66,0x3C,0x00,0x00,0x00,0x00}, /* 0x9E ƒ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0x9F */

  /* 0xA0-0xFF: International characters */
  {0x00,0x00,0x30,0x18,0x0C,0x06,0x0C,0x18,0x30,0x00,0x3C,0x00,0x00,0x00,0x00,0x00}, /* 0xA0 á */
  {0x00,0x00,0x06,0x0C,0x18,0x30,0x18,0x0C,0x06,0x00,0x3C,0x00,0x00,0x00,0x00,0x00}, /* 0xA1 í */
  {0x00,0x00,0x18,0x18,0x00,0x7E,0x00,0x18,0x18,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xA2 ó */
  {0x00,0x00,0x00,0x00,0x62,0x64,0x08,0x13,0x23,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xA3 ú */
  {0x00,0x00,0x00,0x00,0x00,0x1E,0x30,0x1C,0x06,0x3C,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xA4 ñ */
  {0x00,0x00,0x00,0x00,0x00,0x3C,0x66,0x66,0x66,0x3C,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xA5 Ñ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xA6 ª */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xA7 º */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0C,0x0C,0x00,0x00}, /* 0xA8 ¿ */
  {0x00,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x00,0x00,0x00,0x00}, /* 0xA9 ⌐ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xAA ¬ */
  {0x00,0x00,0x00,0x00,0x80,0x80,0x80,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xAB ½ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x0F,0x04,0x04,0x04,0x04,0x00,0x00,0x00,0x00,0x00}, /* 0xAC ¼ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0xE0,0x20,0x20,0x20,0x20,0x00,0x00,0x00,0x00,0x00}, /* 0xAD ¡ */
  {0x00,0x00,0x00,0x00,0x18,0x3C,0x66,0x18,0x18,0x18,0x66,0x3C,0x00,0x00,0x00,0x00}, /* 0xAE « */
  {0x00,0x00,0x00,0x00,0x66,0x66,0x66,0x7E,0x66,0x66,0x66,0x00,0x00,0x00,0x00,0x00}, /* 0xAF ░ */

  {0x00,0xAA,0x00,0xAA,0x00,0xAA,0x00,0xAA,0x00,0xAA,0x00,0xAA,0x00,0xAA,0x00,0xAA}, /* 0xB0 ░ */
  {0xAA,0x55,0xAA,0x55,0xAA,0x55,0xAA,0x55,0xAA,0x55,0xAA,0x55,0xAA,0x55,0xAA,0x55}, /* 0xB1 ▒ */
  {0x55,0xFF,0x55,0xFF,0x55,0xFF,0x55,0xFF,0x55,0xFF,0x55,0xFF,0x55,0xFF,0x55,0xFF}, /* 0xB2 ▓ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xB3 │ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xB4 ┤ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xB5 ╡ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xB6 ╢ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xB7 ╖ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xB8 ╕ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xB9 ╣ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xBA ║ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xBB ╗ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xBC ╝ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xBD ╜ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xBE ╛ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xBF ┐ */

  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xC0 ─ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xC1 ┼ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xC2 ╞ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xC3 ╟ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xC4 ╚ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xC5 ╔ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xC6 ╩ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xC7 ╦ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xC8 ╠ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xC9 ═ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xCA ╬ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xCB ╧ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xCC ╨ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xCD ╤ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xCE ╥ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xCF ╙ */

  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xD0 ╘ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xD1 ╒ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xD2 ╓ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xD3 ╫ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xD4 ╪ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xD5 ┘ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xD6 ┌ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xD7 █ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xD8 ▄ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xD9 ▌ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xDA ▐ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xDB ▀ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xDC α */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xDD ß */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xDE Γ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xDF π */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xE0 Σ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xE1 σ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xE2 µ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xE3 τ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xE4 Φ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xE5 Θ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xE6 Ω */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xE7 δ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xE8 ∞ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xE9 φ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xEA ε */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xEB ∩ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xEC ≡ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xED ± */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xEE ≥ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xEF ≤ */

  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xF0 ⌂ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xF1 ÷ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xF2 ≈ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xF3 ° */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xF4 ∙ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xF5 · */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xF6 √ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xF7 ⁿ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xF8 ² */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xF9 ■ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xFA   */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xFB */
    };
    for (c = 0; c < 256; c++)
      for (r = 0; r < 16; r++)
        f[c * 16 + r] = glyphs[c][r];
  } ENDNATIVE
  -> Allocate and populate TextFont structure
  gem_font_8x16 := asFONT(AllocMem(120, 65538))
  IF gem_font_8x16
    gem_TextFontInit16(gem_font_8x16)
    AddFont(gem_font_8x16)
    gem_font_8x16_registered := 1
  ENDIF
ENDPROC

-> Build and register Atari ST 8x8 system font
PROC gem_init_font_8x8()
  DEF i, c, r

  FOR i := 0 TO 255
    gem_font_width_8x8[i] := 8
  ENDFOR

  FOR i := 0 TO 255
    gem_font_loc_8x8[i] := asCHAR(i * 8)
  ENDFOR

  -> Downsample 8x16 to 8x8 by taking every 2nd row
  FOR c := 0 TO 255
    FOR r := 0 TO 7
      gem_font_data_8x8[c * 8 + r] := gem_font_data_8x16[c * 16 + r * 2]
    ENDFOR
  ENDFOR

  gem_font_8x8 := asFONT(AllocMem(120, 65538))
  IF gem_font_8x8
    gem_TextFontInit8(gem_font_8x8)
    AddFont(gem_font_8x8)
    gem_font_8x8_registered := 1
  ENDIF
ENDPROC

-> Initialize all Atari ST system fonts
PROC gem_init_fonts()
  gem_font_8x16_registered := 0
  gem_font_8x8_registered := 0
  gem_init_font_8x16()
  gem_init_font_8x8()
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
  gem_handles[0] := Input()
  gem_handles[1] := Output()
  gem_handles[2] := Output()
  FOR i := 3 TO 15
    gem_handles[i] := 0
  ENDFOR

  gem_dta := 0
  gem_drv := 0
  gem_search_lock := 0
  bios_kb_shift := 0
  gem_scrn_w := 640
  gem_scrn_h := 400
  gem_aes_id := 0
  gem_app_count := 0
  gem_menu_count := 0
  gem_menu_bar_visible := 0
  gem_menu_active_app := 0
  -> Initialize per-window menu tracking
  DEF wi
  FOR wi := 0 TO 15
    gem_wind_amiga_menu[wi] := 0
    gem_wind_newmenu[wi] := 0
    gem_wind_gem_menu_idx[wi] := -1
  ENDFOR
  gem_form_active := -1
  gem_scrap_len := 0
  gem_msg_head := 0
  gem_msg_tail := 0
  gem_alloc_count := 0
  gem_mouse_x := 320
  gem_mouse_y := 200
  gem_mouse_buttons := 0
  gem_mouse_kstate := 0
  gem_mouse_shape := 2
  gem_mouse_visible := 1
  gem_mouse_user_active := 0
  gem_graf_wk_handle := 1
  gem_graf_char_w := 8
  gem_graf_char_h := 16
  gem_graf_arrow_mode := 1
  gem_graf_accel_key := 0
  gem_mouse_init()
  gem_init_fonts()
  gem_init_gadtools()
  gem_OpenReqTools()
  gem_init_cookie_jar()
  vdi_init_rgb()
  vdi_init_line_pats()
  vdi_handle := -1

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
    ctx[8] := asPTR(temp_string)
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
  gem_FreeReqTools()
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
-> The jar is an array of { LONG id; LONG value; } terminated by a NULL
-> cookie whose value holds the maximum capacity.
-> ---------------------------------------------------------------------------
PROC xbios_cookieptr()
  ctx[0] := gem_cookie_jar
ENDPROC


-> ---------------------------------------------------------------------------
-> Trap handler - Not available in PortablE (needs module-level ASM)
-> GEMDOS functions are called directly from main()
-> ---------------------------------------------------------------------------
