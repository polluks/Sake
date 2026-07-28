
/*** PE/CPP/base ***/
/* PortablE target module for C++ */ 



#define _CRT_SECURE_NO_DEPRECATE 1	//silence depreciated warnings of Visual C++
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
//#include <errno.h>
#include <limits.h>
#include <math.h>
#include <string.h>

#define NULLA NULL
#define NULLS NULL
#define NULLL NULL
#define EMPTY (void)0
#define TRUE -1
#define FALSE 0
#define QuadChara(a, b, c, d) ((a << 24) | (b << 16) | (c << 8) | d)
typedef signed char BOOLEAN;	//enum BOOLEAN {FALSE=0, TRUE=-1};
class eException {};

void* FastNew(long size, BOOLEAN noClear);
void* FastDispose(void* mem, long size);

const int ALL=(int) -1;
class object;
class class_base;
class function;
void* FastNew(long size, BOOLEAN noClear);
void* FastDispose(void* mem, long size);

/* non-native code */

//possible cast functions
//PROC Bool(value)  IS value <> FALSE
//PROC Byte(value)  IS (IF value>=0 THEN (value AND $7F) ELSE -(value AND $7F))!!BYTE
//PROC Char(value)  IS (value AND $FF)!!CHAR
//PROC Int(value)   IS (IF value>=0 THEN (value AND $7FFF) ELSE -(value AND $7FFF))!!INT
//PROC Long(value)  IS value!!LONG
//PROC Quad(value)  IS value!!QUAD
//PROC Float(value) IS value!!FLOAT
//PROC Ptr(value)   IS value!!PTR


class object {
public:
	void* operator new(size_t size) {
		return FastNew(size, FALSE);
	}
	void operator delete(void* mem) {
		FastDispose(mem, -999);
		return;
	}
};

class class_base: public object {
public:
	BOOLEAN notCalledDestructor;
public:
	class_base() {notCalledDestructor=1;}
	virtual ~class_base() {if(notCalledDestructor) {end_class(); notCalledDestructor=0;}}
	virtual void end_class() ;
	virtual long InfoClassType() ;
	virtual BOOLEAN IsSameClassTypeAs(long type) ;
	virtual BOOLEAN IsOfClassType(long parent) ;
};
extern const long TYPEOF_class_base;

class function: public class_base {
public:
	virtual ~function() {if(notCalledDestructor) {end_class(); notCalledDestructor=0;}}
	virtual void new_function() ;
	virtual BOOLEAN IsOfClassType(long parent) ;
	virtual long InfoClassType() ;
};
extern const long TYPEOF_function;
/* #### 
{
extern int    main_argc;
extern char** main_argv;
}
{
int    main_argc;
char** main_argv;
}
NATIVE {main_argc} DEF
NATIVE {main_argv} DEF
PROC PrivateGetMainArgc() IS NATIVE {main_argc} ENDNATIVE !!VALUE
PROC PrivateGetMainArgv() IS NATIVE {main_argv} ENDNATIVE !!ARRAY OF ARRAY OF CHAR
 #### */


extern class eException eException;
extern long main_argc;
extern char** main_argv;

extern char* pe_TargetLanguage;

/* system globals */

extern int exception;
extern char* exceptionInfo;

extern char quadStr[5];
extern float retFloat2; extern long ret2; extern float retFloat3; extern long ret3; extern float retFloat4; extern long ret4; extern float retFloat5; extern long ret5;
BOOLEAN OptMultiThreaded() ;
void PrintL(char* fmtString, long* args=NULLL) ;
short Inp(void* fileHandle) ;
long FileLength(char* path) ;
BOOLEAN StrCmp(char* first, char* second, long len=ALL, long firstOffset=0, long secondOffset=0) ;
BOOLEAN StrCmpNoCase(char* first, char* second, long len=ALL, long firstOffset=0, long secondOffset=0) ;
long Val(char* string, int* addrRead=NULLA, long startPos=0) ;
long InStr(char* haystack, char* needle, long startPos=0) ;
long InStrNoCase(char* haystack, char* needle, long startPos=0) ;
char* UpperStr(char* string);
char* LowerStr(char* string);
void AstrCopy(void* destination, char* source, long destSize);
char* QuadToStr(int value) ;
void CleanUp(long returnValue=0);
void* New(long size, BOOLEAN noClear=FALSE) ;
void* NewR(long size, BOOLEAN noClear=FALSE) ;
void* Dispose(void* mem) ;
void* FastNew(long size, BOOLEAN noClear=FALSE) ;
void* FastDispose(void* mem, long size) ;
BOOLEAN FastVerify(BOOLEAN quiet=FALSE) ;
long FastReport(BOOLEAN quiet=FALSE) ;
long Rnd(long max) ;
long Mod(long a, long b) ;
long Pow(long a, long b) ;
float RealVal(char* string) ;
void Throw(int a, char* b=NULLA);
void Raise(int a);
void PrintException();
void new_base() ;
const long TYPEOF_class_base = (long) "class_base";
const long TYPEOF_function = (long) "function";
/* #### 
{
extern int    main_argc;
extern char** main_argv;
}
{
int    main_argc;
char** main_argv;
}
NATIVE {main_argc} DEF
NATIVE {main_argv} DEF
PROC PrivateGetMainArgc() IS NATIVE {main_argc} ENDNATIVE !!VALUE
PROC PrivateGetMainArgv() IS NATIVE {main_argv} ENDNATIVE !!ARRAY OF ARRAY OF CHAR
 #### */


class eException eException;
long main_argc;
char** main_argv=(char**) NULL;

char* pe_TargetLanguage=NULL;

/* system globals */

int exception;
char* exceptionInfo=NULL;

char quadStr[5];
float retFloat2; long ret2; float retFloat3; long ret3; float retFloat4; long ret4; float retFloat5; long ret5;
short Inp(void* fileHandle)  {
	short char2;
	char2 = getc((FILE*)fileHandle );
	if( char2 == EOF  ) { char2 = -1;}
	return char2 ;
} 
long FileLength(char* path)  {
	long size;
	
	FILE* stream = fopen(path ,"rb");
	if( stream== NULL) {
		size = -1;
	} else {
		fseek(stream, 0, SEEK_END);
		size = ftell(stream);
		fclose(stream);
	}
	return size ;
} 

BOOLEAN StrCmp(char* first, char* second, long len, long firstOffset, long secondOffset)  {
	BOOLEAN match;
//IS 0 = (IF len=ALL THEN NATIVE {strcmp(} firstOffset*SIZEOF CHAR + first {,} secondOffset*SIZEOF CHAR + second {)} ENDNATIVE) ELSE (0 = NATIVE {strncmp(} firstOffset*SIZEOF CHAR + first {,} secondOffset*SIZEOF CHAR + second {,} len {)} ENDNATIVE))
	if( len == ALL) {
		match = - (strcmp((char*) (firstOffset*sizeof( char )+ (long) first ),(char*) (secondOffset*sizeof( char )+ (long) second ))== 0) 	;//!!BYTE
	} else {
		match = - (strncmp((char*) (firstOffset*sizeof( char )+ (long) first ),(char*) (secondOffset*sizeof( char )+ (long) second ),len )== 0) 	;//!!BYTE
	}
	return match ;
} 

BOOLEAN StrCmpNoCase(char* first, char* second, long len, long firstOffset, long secondOffset)  {
	BOOLEAN match;
//IS 0 = (IF len=ALL THEN NATIVE {strcasecmp(} firstOffset*SIZEOF CHAR + first {,} secondOffset*SIZEOF CHAR + second {)} ENDNATIVE) ELSE (0 = NATIVE {strncasecmp(} firstOffset*SIZEOF CHAR + first {,} secondOffset*SIZEOF CHAR + second {,} len {)} ENDNATIVE))
	if( len == ALL) {
		match = - (strcasecmp((char*) (firstOffset*sizeof( char )+ (long) first ),(char*) (secondOffset*sizeof( char )+ (long) second ))== 0) 	;//!!BYTE
	} else {
		match = - (strncasecmp((char*) (firstOffset*sizeof( char )+ (long) first ),(char*) (secondOffset*sizeof( char )+ (long) second ),len )== 0) 	;//!!BYTE
	}
	return match ;
} 

long Val(char* string, int* addrRead, long startPos)  {
	long value, read2;
	char* str=NULL; long i;
	char* final=NULL; signed char base, isNegative;
	int temp_QUAD;
try {
	temp_QUAD = exception ;
	exception = 0 ;
	
	//find start of number (skip any spaces & tabs)
	/*i := startPos
	WHILE (string[i]=" ") OR (string[i]="\t") DO i++
	*/
	i = strspn(string +startPos ," \t");
	str = (char*) ((long) string + (startPos+i )* sizeof( char));
	
	//determine sign & base of number (and skip their symbols)
	if( isNegative = - (str[0] == '-')  ) { str++;}
	
	if(      str[0] == '\000') { base =  0	;//string is empty
	} else if( str[0] == '%' ) { base =  2 ; str++;
	} else if( str[0] == '$' ) { base = 16 ; str++;
	} else {                  base = 10;
	}
	
	//interpret value
	if( base == 0) {
		value = 0;
		read2  = 0;
	} else {
		if( base == 10) {
			value = strtol(str ,&final ,base );
		} else {
			value = strtoul(str ,&final ,base );
		}
		if( final != str) {
			read2  = (int ) ((long) (char*) (final - (long) string)  / sizeof( char ))- startPos;
		} else {
			value = 0;
			read2  = 0;
		}
	}
	
	if( addrRead ) { addrRead[0] = (int) read2 ;}
	if( isNegative ) { value = -value;}
} catch(...) {}
	ret2 = read2 ;
	if (exception!=0) {throw eException ;} else {EMPTY;};
	exception = temp_QUAD ;
	return value ;
} 

long InStr(char* haystack, char* needle, long startPos)  {
	long matchPos;
	char* start=NULL; char* found=NULL;
	
	start = (char*) ((long) haystack + startPos * sizeof( char));
	
	found = strstr(start ,needle );
	if( found) {
		matchPos = (long) ((long) (char*) (found - (long) haystack)  / sizeof( char));
	} else {
		matchPos = -1;
	}
	return matchPos ;
} 

long InStrNoCase(char* haystack, char* needle, long startPos)  {
	long matchPos;
	long haystackPos, needlePos; char haystackChara, needleChara;
	
	haystackPos = startPos;
	  needlePos = 0;
	
	haystackChara = haystack[haystackPos];
	  needleChara =   needle[needlePos];
	
	matchPos = ( needleChara!=0 )? (long) -1 : haystackPos;
	while( - (haystackChara!=0)  & - (needleChara!=0)) {
		if( (( - (haystackChara >= 'A')  & - (haystackChara <= 'Z')  )? haystackChara + 'a' - 'A' : haystackChara)== (( - (needleChara >= 'A')  & - (needleChara <= 'Z')  )? needleChara + 'a' - 'A' : needleChara)) {
			//(matching charas)
			if( matchPos == -1 ) { matchPos = haystackPos;}
			needlePos++ ; needleChara = needle[needlePos];
		} else {
			//(mismatched charas) so restart search
			if( matchPos != -1) {
				haystackPos = matchPos;
				matchPos = -1;
			}
			needlePos = 0 ; needleChara = needle[needlePos];
		}
		
		haystackPos++ ; haystackChara = haystack[haystackPos];
	}
	
	if( needle[needlePos]!=0 ) { matchPos = -1;}
	return matchPos ;
} 
char* UpperStr(char* string) {
	long i; char chara;
	i = 0;
	while( chara = string[i]) {
		if( - (chara >= 'a')  & - (chara <= 'z')  ) { string[i] = chara + 'A' - 'a';}
		i++;
	}
	return string;
} 
char* LowerStr(char* string) {
	long i; char chara;
	i = 0;
	while( chara = string[i]) {
		if( - (chara >= 'A')  & - (chara <= 'Z')  ) { string[i] = chara + 'a' - 'A';}
		i++;
	}
	return string;
} 
void AstrCopy(void* destination, char* source, long destSize) {
	strncpy((char*) destination ,source ,destSize-1 );
	((char*) destination )[destSize - 1 ] = 0;
	return ;
}
//OstrCmp()
//OstrCmpNoCase()

char* QuadToStr(int value)  {
	char* string=NULL;
	quadStr[0] = (char) (( 32>(value >> 24 & 0xFF ))? (long) 32 : (long) (value >> 24 & 0xFF));
	quadStr[1] = (char) (( 32>(value >> 16 & 0xFF ))? (long) 32 : (long) (value >> 16 & 0xFF));
	quadStr[2] = (char) (( 32>(value >>  8 & 0xFF ))? (long) 32 : (long) (value >>  8 & 0xFF));
	quadStr[3] = (char) (( 32>(value >>  0 & 0xFF ))? (long) 32 : (long) (value >>  0 & 0xFF));
	quadStr[4] = 0;
	
	string = quadStr;
	return string ;
} 


void CleanUp(long returnValue) {
	Throw(-1, (char*) returnValue)	;//use reserved exception -1 for CleanUp()
	return ;
}
long Rnd(long max)  {
	long num;
	if( max >= 0) {
		num = ((rand() ) % (max ));
	} else {
		srand(labs(max ));
		num = 0;
	}
	return num ;
} 
long Mod(long a, long b)  {
	long c, d;
	int temp_QUAD;
try {
	temp_QUAD = exception ;
	exception = 0 ;
	d = a / b;
	c = a - d * b;
} catch(...) {}
	ret2 = d ;
	if (exception!=0) {throw eException ;} else {EMPTY;};
	exception = temp_QUAD ;
	return c ;
} 
/*->ldiv() simply does not work on too many compilers
PROC Mod(a, b) RETURNS c, d
	{ldiv_t temp = ldiv(} a {,} b {)}
	c := {temp.rem}!!VALUE
	d := {temp.quot}!!VALUE
ENDPROC*/
long Pow(long a, long b)  {
	long c;
	c = 1;
	while( b > 0) {
		c = c * a;
		b--;
	}
	return c ;
} 
float RealVal(char* string)  {
	float value; long read2;
	char* final=NULL;
	int temp_QUAD;
try {
	temp_QUAD = exception ;
	exception = 0 ;
	
	value = strtod(string ,&final );
	if( final != string) {
		read2  = (long) ((long) (char*) (final - (long) string)  / sizeof( char));
	} else {
		value = 0.0;
		read2  = 0;
	}
} catch(...) {}
	ret2 = read2 ;
	if (exception!=0) {throw eException ;} else {EMPTY;};
	exception = temp_QUAD ;
	return value ;
} 

//Does not work in StormC: IS (exception := a) BUT (exceptionInfo := b) BUT {throw new Exception()}
void Throw(int a, char* b) {
	exception     = a;
	exceptionInfo = b;
	throw  eException;
	return ;
}
void Raise(int a) {
	exception = a;
	throw  eException;
	return ;
}

void PrintException() {
	if( - (exception != 0)  & - (exception != -1)) {
		if( exceptionInfo) {
			printf("EXCEPTION: \"%s\"; %s.\n" ,(long) QuadToStr(exception) ,(long) exceptionInfo);
		} else {
			printf("EXCEPTION: \"%s\".\n" ,(long) QuadToStr(exception) );
		}
	}
	return ;
}
void new_base()  {
	pe_TargetLanguage = (char*) "CPP";
	return ;
}
void class_base::end_class()  {
	EMPTY;
}
long class_base::InfoClassType()  {
	return TYPEOF_class_base;
}
BOOLEAN class_base::IsSameClassTypeAs(long type)  {
	return - (type == this->InfoClassType());
}
BOOLEAN class_base::IsOfClassType(long parent)  {
	return - (parent == TYPEOF_class_base);
}
void function::new_function()  {
	EMPTY;
}
BOOLEAN function::IsOfClassType(long parent)  {
	return ( parent == TYPEOF_function)? TRUE : this ->class_base:: IsOfClassType(parent );
}
long function::InfoClassType()  {
	return TYPEOF_function;
}

/*** target/PE/base ***/
/* PortablE target module for C++ AmigaOS */ 

/* system constants */

const short OLDFILE=1005;
const short NEWFILE=1006;

extern char* pe_TargetOS;
void ReopenStdIn(char* fileName);
void ReopenStdOut(char* fileName);
void* SetStdIn(void* fileHandle) ;
void* SetStdOut(void* fileHandle) ;
void new_base2() ;

char* pe_TargetOS=NULL;

/* stdin & stdout */

void ReopenStdIn(char* fileName) {
	if( freopen(fileName , "r", stdin)== NULL ) { Throw(QuadChara('F','I','L','E'), (char*) "ReopenStdIn(); failed to re-open stdin on the given path");}
	return ;
}

void ReopenStdOut(char* fileName) {
	if( freopen(fileName , "w", stdout)== NULL ) { Throw(QuadChara('F','I','L','E'), (char*) "ReopenStdOut(); failed to re-open stdout on the given path");}
	return ;
}

void* SetStdIn(void* fileHandle)  {
	void* oldstdin=NULL;
	oldstdin = (void*) stdin;
	stdin = (FILE*) (unsigned long) fileHandle;
	return oldstdin ;
} 

void* SetStdOut(void* fileHandle)  {
	void* oldstdout=NULL;
	oldstdout = (void*) stdout;
	stdout = (FILE*) (unsigned long) fileHandle;
	return oldstdout ;
} 
void new_base2()  {
	pe_TargetOS = (char*) "AmigaOS3";
	return ;
}

/*** PE/EndianShared ***/
/* PE/EndianShared.e 27-04-2010
   Endian-swapping routines shared by all targets.
*/ 
long long SwapEndianBIGVALUE(long long in);
/*PROC SwapEndianLONG(in:LONG)
	DEF out:LONG
	DEF p1:LONG, p2:LONG, p3:LONG, p4:LONG
	p1 := in SHL 24 AND $FF000000
	p2 := in SHL  8 AND $00FF0000
	p3 := in SHR  8 AND $0000FF00
	p4 := in SHR 24 AND $000000FF
	out := p1 OR p2 OR p3 OR p4
ENDPROC out*/

long long SwapEndianBIGVALUE(long long in) {
	long long out;
	long long* bigValue=NULL; int buffer[2]; int temp;
	
	bigValue = (long long*) buffer ;
	bigValue[0] = in;
	
	temp      = (int) (buffer[0] << 24 & 0xFF000000  | buffer[0] <<  8 & 0x00FF0000  | buffer[0] >>  8 & 0x0000FF00  | buffer[0] >> 24 & 0x000000FF  );
	buffer[0] = (int) (buffer[1] << 24 & 0xFF000000  | buffer[1] <<  8 & 0x00FF0000  | buffer[1] >>  8 & 0x0000FF00  | buffer[1] >> 24 & 0x000000FF  );
	buffer[1] = temp;
	
	out = bigValue[0];
	return out;
} 

/*** PE/EString_partial ***/
/* PE/EString_partial.e 12-02-15
   A re-implementation of AmigaE's E-string functions.
   
   By Christopher S Handley:
   10-03-02 - Started coding it, to replace the existing AmigaE functions.
   19-03-02 - Mostly completed.
   14-07-06 - Ported to PortablE.
   27-07-06 - Updated to use the STRING type.
   27-10-07 - Fixed Next() & Link() bugs.
   19-04-09 - StrJoin() added earlier but not documented until now.
   09-10-09 - Added check to prevent source & destination being the same.  Problem reported by Matthias Rustler.
   03-05-10 - Declared the "missing" procedures as prototypes.  Added StringFL().
   13-01-11 - RealF() decimalPlaces is no-longer a BYTE.
   19-12-12 - Removed source<>destination restriction for StrCopy(), RightStr() & MidStr().  Fixed type of RightStr()'s eString2 parameter.
   12-02-15 - Removed source<>destination restriction for StrAdd().
*/ /* Emulated procedures:
NewString(maxLen) RETURNS eString:STRING
DisposeString(eString:STRING) RETURNS NILS
StrCopy( eString:STRING, string:ARRAY OF CHAR, len=ALL, pos=0) RETURNS eString:STRING
StrAdd(  eString:STRING, string:ARRAY OF CHAR, len=ALL, pos=0) RETURNS eString:STRING
StrJoin(s1=NILA:ARRAY OF CHAR, s2=NILA:ARRAY OF CHAR, s3=NILA:ARRAY OF CHAR, s4=NILA:ARRAY OF CHAR, s5=NILA:ARRAY OF CHAR, s6=NILA:ARRAY OF CHAR, s7=NILA:ARRAY OF CHAR, s8=NILA:ARRAY OF CHAR, s9=NILA:ARRAY OF CHAR, s10=NILA:ARRAY OF CHAR, s11=NILA:ARRAY OF CHAR) RETURNS newString:STRING
EstrLen( eString:STRING) RETURNS len:VALUE
StrMax(  eString:STRING) RETURNS max:VALUE
RightStr(eString:STRING, eString2:ARRAY OF CHAR, n) RETURNS eString:STRING
MidStr(  eString:STRING, string:ARRAY OF CHAR, pos, len=ALL) RETURNS eString:STRING
SetStr(  eString:STRING, newLen)
Link(    complex:STRING, tail:OWNS STRING) RETURNS complex:STRING
Next(    complex:STRING) RETURNS tail:STRING
Forward( complex:STRING, num) RETURNS tail:STRING

On-purposely missing procedures:
ReadStr(fileHandle:PTR, eString:STRING) RETURNS fail:BOOL
StringF( eString:STRING, fmtString:ARRAY OF CHAR, ...)             RETURNS eString:STRING, len
StringFL(eString:STRING, fmtString:ARRAY OF CHAR, args=NILL:ILIST) RETURNS eString:STRING, len
RealF(   eString:STRING, value:FLOAT, decimalPlaces=8) RETURNS eString:STRING
*/
class pEString;


class pEString: public object {
public:
	long length;                 	//length of actual string (excluding terminating zero)
	long size;                  	//max length of string    (including terminating zero)
	pEString* next;	//points to next string header, not the actual string
};
char* NewString(long maxLen);
char* DisposeString(char* eString);
char* StrCopy(char* eString, char* string, long len=ALL, long pos=0);
char* StrAdd(char* eString, char* string, long len=ALL, long pos=0);
char* StrJoin(char* s1=NULLA, char* s2=NULLA, char* s3=NULLA, char* s4=NULLA, char* s5=NULLA, char* s6=NULLA, char* s7=NULLA, char* s8=NULLA, char* s9=NULLA, char* s10=NULLA, char* s11=NULLA, char* s12=NULLA, char* s13=NULLA, char* s14=NULLA, char* s15=NULLA, char* s16=NULLA, char* s17=NULLA, char* s18=NULLA, char* s19=NULLA);
long EstrLen(char* eString);
long StrMax(char* eString);
char* RightStr(char* eString, char* eString2, long n);
char* MidStr(char* eString, char* string, long pos, long len=ALL);
void SetStr(char* eString, long newLen);
BOOLEAN ReadStr(void* fileHandle, char* eString) ;
char* StringF(char* eString, char* fmtString, long arg1=0, long arg2=0, long arg3=0, long arg4=0, long arg5=0, long arg6=0, long arg7=0, long arg8=0) ;
char* StringFL(char* eString, char* fmtString, long* args=NULLL) ;
char* RealF(char* eString, float value, long decimalPlaces=8) ;
char* Link(char* complex, char* tail);
char* Next(char* complex);
char* Forward(char* complex, long num);


char* NewString(long maxLen) {
	char* eString=NULL;
	pEString* pEString2=NULL;	
	long sizeOfEString;
	
	//use check
	if( maxLen < 0 ) { Throw(QuadChara(0,'E','P','U'), (char*) "EString; NewString(); maxLen<0");}
	
	//allocate eString
	sizeOfEString = (maxLen + 1 )* sizeof( char)  + sizeof( pEString);
	pEString2 = (pEString*) FastNew(sizeOfEString, TRUE);
	
	//init
	pEString2->length = 0;
	pEString2->size   = maxLen + 1;
	pEString2->next   = (pEString*) NULL;
	
	//retrieve string after header
	eString = (char*) ((long) pEString2 + sizeof( pEString ));
	
	//zero-terminate empty string
	eString[0] = '\000';
	return eString;
} 

char* DisposeString(char* eString) {
	pEString* pEString2=NULL;
	pEString* next=NULL;
	
	if( eString) {
		//retrieve string header
		pEString2 = (pEString*) ((long) eString - sizeof( pEString ));
		
		//loop through all strings in linked list
		do {
			//store any string tail
			next = pEString2->next;
			
			//dealloc string
			pEString2 = (pEString*) FastDispose(pEString2, pEString2->size * sizeof( char)  + sizeof( pEString));
			
			//move to tail
			pEString2 = next;
		} while(( (void*) pEString2 == NULL)==0);
	}
	return NULLS;
} 

char* StrCopy(char* eString, char* string, long len, long pos) {
	pEString* pEString2=NULL;
	long readIndex, maxReadIndex;
	long writeIndex, maxWriteIndex;
	
	//use check
	if( eString == NULLS ) { Throw(QuadChara(0,'E','P','U'), (char*) "EString; StrCopy(); eString=NILS");}
	if(  (void*) string == NULLA ) { Throw(QuadChara(0,'E','P','U'), (char*) "EString; StrCopy(); string=NILA");}
	if( - (len < 0)  & - (len != ALL)  ) { Throw(QuadChara(0,'E','P','U'), (char*) "EString; StrCopy(); len<0");}
	if( pos < 0) { Throw(QuadChara(0,'E','P','U'), (char*) "EString; StrCopy(); pos<0");}
	
	//retrieve string header
	pEString2 = (pEString*) ((long) eString - sizeof( pEString ));
	
	//calc end of string reading from & writing to (inc zero termination)
	maxReadIndex  = pos + (( len==ALL )? pEString2->size - 1 : len);
	maxWriteIndex = pEString2->size - 1;
	
	//copy all characters that will fit
	readIndex  = pos;
	writeIndex = 0;
	while( - (string[readIndex] != 0)  & - (writeIndex < maxWriteIndex)  & - (readIndex < maxReadIndex)) {
		eString[writeIndex] = string[readIndex];
		
		writeIndex++;
		readIndex++;
	}
	
	//update string's stored length
	pEString2->length = writeIndex;
	eString[writeIndex] = '\000';
	return eString;
} 

char* StrAdd(char* eString, char* string, long len, long pos) {
	pEString* pEString2=NULL;
	long readIndex, maxReadIndex;
	long writeIndex, maxWriteIndex;
	
	//use check
	if( eString == NULLS ) { Throw(QuadChara(0,'E','P','U'), (char*) "EString; StrAdd(); eString=NILS");}
	if(  (void*) string == NULLA ) { Throw(QuadChara(0,'E','P','U'), (char*) "EString; StrAdd(); string=NILA");}
	if( - (len < 0)  & - (len != ALL)  ) { Throw(QuadChara(0,'E','P','U'), (char*) "EString; StrAdd(); len<0");}
	if( pos < 0) { Throw(QuadChara(0,'E','P','U'), (char*) "EString; StrAdd(); pos<0");}
	
	//retrieve string header
	pEString2 = (pEString*) ((long) eString - sizeof( pEString ));
	
	//calc end of string reading from & writing to (inc zero termination)
	maxReadIndex  = pos + (( len==ALL )? pEString2->size - 1 : len);
	maxWriteIndex = pEString2->size - 1;
	
	//copy all characters that will fit
	readIndex  = pos;
	writeIndex = pEString2->length	;//start writing past end of string
	while( - (string[readIndex] != 0)  & - (writeIndex < maxWriteIndex)  & - (readIndex < maxReadIndex)) {
		eString[writeIndex] = string[readIndex];
		
		writeIndex++;
		readIndex++;
	}
	
	//update string's stored length
	pEString2->length = writeIndex;
	eString[writeIndex] = '\000';
	return eString;
} 

char* StrJoin(char* s1, char* s2, char* s3, char* s4, char* s5, char* s6, char* s7, char* s8, char* s9, char* s10, char* s11, char* s12, char* s13, char* s14, char* s15, char* s16, char* s17, char* s18, char* s19) {
	char* newString=NULL;
	long len;
	
	len = 0;
	if( s1 ) { len = len + strlen(s1 );}
	if( s2 ) { len = len + strlen(s2 );}
	if( s3 ) { len = len + strlen(s3 );}
	if( s4 ) { len = len + strlen(s4 );}
	if( s5 ) { len = len + strlen(s5 );}
	if( s6 ) { len = len + strlen(s6 );}
	if( s7 ) { len = len + strlen(s7 );}
	if( s8 ) { len = len + strlen(s8 );}
	if( s9 ) { len = len + strlen(s9 );}
	if( s10 ) { len = len + strlen(s10 );}
	if( s11 ) { len = len + strlen(s11 );}
	if( s12 ) { len = len + strlen(s12 );}
	if( s13 ) { len = len + strlen(s13 );}
	if( s14 ) { len = len + strlen(s14 );}
	if( s15 ) { len = len + strlen(s15 );}
	if( s16 ) { len = len + strlen(s16 );}
	if( s17 ) { len = len + strlen(s17 );}
	if( s18 ) { len = len + strlen(s18 );}
	if( s19 ) { len = len + strlen(s19 );}
	
	newString= NewString(len);
	if( s1 ) { StrAdd(newString, s1);}
	if( s2 ) { StrAdd(newString, s2);}
	if( s3 ) { StrAdd(newString, s3);}
	if( s4 ) { StrAdd(newString, s4);}
	if( s5 ) { StrAdd(newString, s5);}
	if( s6 ) { StrAdd(newString, s6);}
	if( s7 ) { StrAdd(newString, s7);}
	if( s8 ) { StrAdd(newString, s8);}
	if( s9 ) { StrAdd(newString, s9);}
	if( s10 ) { StrAdd(newString, s10);}
	if( s11 ) { StrAdd(newString, s11);}
	if( s12 ) { StrAdd(newString, s12);}
	if( s13 ) { StrAdd(newString, s13);}
	if( s14 ) { StrAdd(newString, s14);}
	if( s15 ) { StrAdd(newString, s15);}
	if( s16 ) { StrAdd(newString, s16);}
	if( s17 ) { StrAdd(newString, s17);}
	if( s18 ) { StrAdd(newString, s18);}
	if( s19 ) { StrAdd(newString, s19);}
	return newString;
} 

long EstrLen(char* eString) {
	long len;
	pEString* pEString2=NULL;
	
	//use check
	if( eString == NULLS ) { Throw(QuadChara(0,'E','P','U'), (char*) "EString; EstrLen(); eString=NILS");}
	
	//retrieve string header
	pEString2 = (pEString*) ((long) eString - sizeof( pEString ));
	len = pEString2->length;
	return len;
} 

long StrMax(char* eString) {
	long max;
	pEString* pEString2=NULL;
	
	//use check
	if( eString == NULLS ) { Throw(QuadChara(0,'E','P','U'), (char*) "EString; StrMax(); eString=NILS");}
	
	//retrieve string header
	pEString2 = (pEString*) ((long) eString - sizeof( pEString ));
	max = pEString2->size - 1;
	return max;
} 

char* RightStr(char* eString, char* eString2, long n) {
	pEString* pEString2=NULL;
	char* readString=NULL;
	
	//use check
	if( eString  == NULLS ) { Throw(QuadChara(0,'E','P','U'), (char*) "EString; RightStr(); eString=NILS");}
	if( eString2 == NULLS ) { Throw(QuadChara(0,'E','P','U'), (char*) "EString; RightStr(); eString2=NILS");}
	if( n < 0           ) { Throw(QuadChara(0,'E','P','U'), (char*) "EString; RightStr(); n<0");}
	
	//retrieve string header
	pEString2 = (pEString*) ((long) eString2 - sizeof( pEString ));
	
	//restrict n to sensible range
	if( n > pEString2->length ) { n = pEString2->length;}
	
	//move to start of n characters
	readString = (char*) ((long) eString2 + (pEString2->length - n) * sizeof( char)  );
	
	//use strCopy procedure
	StrCopy(eString, readString, n);
	return eString;
} 

char* MidStr(char* eString, char* string, long pos, long len) {
	long index; char* readString=NULL;
	
	//use check
	if( eString == NULLS ) { Throw(QuadChara(0,'E','P','U'), (char*) "EString; MidStr(); eString=NILS");}
	if(  (void*) string == NULLA ) { Throw(QuadChara(0,'E','P','U'), (char*) "EString; MidStr(); string=NILA");}
	if( pos < 0        ) { Throw(QuadChara(0,'E','P','U'), (char*) "EString; MidStr(); pos<0");}
	if( - (len < 0)  & - (len != ALL)  ) { Throw(QuadChara(0,'E','P','U'), (char*) "EString; MidStr(); len<0");}
	
	//find correct start position SAFELY (which is more than AmigaE does!)
	index = 0;
	while( - (string[index] != 0)  & - (pos > 0)) {
		index++;
		pos--;
	}
	
	readString = (char*) ((long) string + index * sizeof( char)  );
	
	//copy specified part of string
	StrCopy(eString, readString, len);
	return eString;
} 

void SetStr(char* eString, long newLen) {
	pEString* pEString2=NULL;
	
	//use check
	if( eString == NULLS ) { Throw(QuadChara(0,'E','P','U'), (char*) "EString; SetStr(); eString=NILS");}
	if( newLen < 0     ) { Throw(QuadChara(0,'E','P','U'), (char*) "EString; SetStr(); newLen<0");}
	
	//retrieve string header
	pEString2 = (pEString*) ((long) eString - sizeof( pEString ));
	
	//additional use check
	if( newLen >= pEString2->size ) { Throw(QuadChara(0,'E','P','U'), (char*) "EString; SetStr(); newLen exceeds string size");}
	
	//set length
	pEString2->length = newLen;
	eString[newLen] = '\000';
	return ;
}

char* Link(char* complex, char* tail) {
	pEString* pEString2=NULL;
	
	//use check
	if( complex == NULLS ) { Throw(QuadChara(0,'E','P','U'), (char*) "EString; Link(); complex=NILS");}
	
	//retrieve string header
	pEString2 = (pEString*) ((long) complex - sizeof( pEString ));
	
	//store tail's header
	pEString2->next = (pEString*) (( tail )? (void*) ((long) tail - sizeof( pEString )): NULL);
	return complex;
} 

char* Next(char* complex) {
	char* tail=NULL;
	pEString* pEString2=NULL;
	
	if( complex == NULLS ) {
		tail= NULLS;
		goto finally;
	}
	
	//retrieve string header
	pEString2 = (pEString*) ((long) complex - sizeof( pEString ));
	
	//return tail with hidden header
	pEString2 = pEString2->next;
	if( (void*) pEString2 == NULL ) {
		tail = NULLS ;
	} else { 
		tail = (char*) ((long) pEString2 + sizeof( pEString ));
	}
finally: 0; 
	return tail;
} 

char* Forward(char* complex, long num) {
	char* tail=NULL;
	pEString* pEString2=NULL;
	
	//use check
	if( num < 0        ) { Throw(QuadChara(0,'E','P','U'), (char*) "EString; Forward(); num<0");}
	
	if( complex == NULLS ) {
		tail= NULLS;
		goto finally;
	}
	
	//retrieve string header
	pEString2 = (pEString*) ((long) complex - sizeof( pEString ));
	
	while( - ((void*) pEString2 != NULL)  & - (num > 0)) {
		pEString2 = pEString2->next;
		num--;
	}
	
	//retrieve string after header
	if( (void*) pEString2 == NULL ) {
		tail = NULLS ;
	} else { 
		tail = (char*) ((long) pEString2 + sizeof( pEString ));
	}
finally: 0; 
	return tail;
} 

/*** PE/Mem_prototypes ***/
/* prototypes needed by PE/Mem */ 
void* baseNew(long size, BOOLEAN noClear=FALSE) ;
void baseDispose(void* mem) ;
void* baseNewSemaphore() ;
void* baseDisposeSemaphore(void* sem) ;
void baseSemLock(void* sem) ;
void baseSemUnlock(void* sem) ;

/*** PE/pSemaphores_prototypes ***/
/* prototypes of pSemaphores */ 
void* NewSemaphore() ;
void* DisposeSemaphore(void* sem) ;
void SemLock(void* sem) ;
void SemUnlock(void* sem) ;
BOOLEAN SemTryLock(void* sem) ;

/*** PE/OstrCmp ***/



//Replacement for AmigaE's OstrCmp(), which (possibly erratically) seems to incorrectly think these two characters are the same:
// "A" = $41 = 065 = %01000001
// "�" = $AB = 171 = %10101011

signed char OstrCmp(char* string1, char* string2, long max=ALL, long string1Offset=0, long string2Offset=0) ;
signed char OstrCmpNoCase(char* string1, char* string2, long max=ALL, long string1Offset=0, long string2Offset=0) ;

signed char OstrCmp(char* string1, char* string2, long max, long string1Offset, long string2Offset)  {
	signed char sign;
	short order; char char1; long index;
	signed char temp_BYTE;
	
	string1 = (char*) (string1Offset*sizeof( char )+ (long) string1);
	string2 = (char*) (string2Offset*sizeof( char )+ (long) string2);
	
	index = 0;
	if( - (index < max)  | - (max==ALL)) {
		do {
			char1 = string1[index];
			order = string2[index] - char1		;//sign indicates order
			
			index++;
		} while(( - (order!=0)  | - (char1==0)  | - (index >= max)  & - (max!=ALL) 	)==0);//char1=0 catches case where both strings are same length
	} else {
		order = 0;
	}
	
	sign = ( order==0 )? 0 : ( order<0 )? -1 : 1;
	return sign ;
} 

//This is like OstrCmp() but it does not care about letter case
signed char OstrCmpNoCase(char* string1, char* string2, long max, long string1Offset, long string2Offset)  {
	signed char sign;
	short order; char char1, char2; long index;
	signed char temp_BYTE;
	
	string1 = (char*) (string1Offset*sizeof( char )+ (long) string1);
	string2 = (char*) (string2Offset*sizeof( char )+ (long) string2);
	
	index = 0;
	if( - (index < max)  | - (max==ALL)) {
		do {
			char1=string1[index];
			char2=string2[index];
			
			if( - (char1>='a')  & - (char1<='z')  ) { char1 = char1 - 'a' + 'A';}
			if( - (char2>='a')  & - (char2<='z')  ) { char2 = char2 - 'a' + 'A';}
			
			order=char2 - char1	;//sign indicates order
			
			index++;
		} while(( - (order!=0)  | - (char1==0)  | - (index >= max)  & - (max!=ALL) 	)==0);//char1=0 catches case where both strings are same length
	} else {
		order = 0;
	}
	
	sign = ( order==0 )? 0 : ( order<0 )? -1 : 1;
	return sign ;
} 

/*** PE/CPP/EString ***/
/* PortablE target module that completes EStrings */ /* missing E-string functions */
char* appendDecimal(char* eString, long value, long minWidth);

char* StringF(char* eString, char* fmtString, long arg1, long arg2, long arg3, long arg4, long arg5, long arg6, long arg7, long arg8)  {
	long max, len;
	int temp_QUAD;
try {
	temp_QUAD = exception ;
	exception = 0 ;
	
	max = StrMax(eString);
	len = snprintf(eString ,max+1 ,fmtString ,arg1 ,arg2 ,arg3 ,arg4 ,arg5 ,arg6 ,arg7 ,arg8 );
	len = ( len<max )? len : max;
	SetStr(eString, len);
} catch(...) {}
	ret2 = len;
	if (exception!=0) {throw eException ;} else {EMPTY;};
	exception = temp_QUAD ;
	return eString;
} 

BOOLEAN ReadStr(void* fileHandle, char* eString)  {
	BOOLEAN fail; 
	fail = - (fgets(eString ,StrMax(eString)+1 , (FILE*) fileHandle )== NULL);
	SetStr(eString, strlen(eString ));
	return fail ;
} 

/*->snprintf() simply does not work on too many compilers, for floating-point
PROC RealF(eString:STRING, value:FLOAT, decimalPlaces=8) REPLACEMENT
	DEF max, len
	
	max := StrMax(eString)
	len := NATIVE {snprintf(} eString {,} max+1 {, "%.*f",} decimalPlaces {,} value {)} ENDNATIVE !!VALUE
	SetStr(eString, Min(len, max))
ENDPROC eString
*/


char* RealF(char* eString, float value, long decimalPlaces)  {
	float integer; long nextDecimalPlaces;
	char* temp_ARRAY_OF_CHAR=NULL;
	
	if( value>0 ) {
		temp_ARRAY_OF_CHAR = (char*) "";
	} else { 
		temp_ARRAY_OF_CHAR = (char*) "-";
	}
	StrCopy(eString, temp_ARRAY_OF_CHAR );
	value = fabs(value );
	
	integer = floor(value );
	appendDecimal(eString, (long) integer, 0);
	
	if( decimalPlaces > 0) {
		StrAdd(eString, (char*) ".");
		
		value = value - integer;
		do {
			nextDecimalPlaces = ( decimalPlaces<9 )? decimalPlaces : (long) 9;
			decimalPlaces = decimalPlaces - nextDecimalPlaces;
			
			value = value * Pow(10, nextDecimalPlaces);
			integer = floor(value );
			appendDecimal(eString, (long) integer, nextDecimalPlaces);
			value = value - integer;
		} while(( decimalPlaces <= 0)==0);
	}
	return eString;
} 


char* appendDecimal(char* eString, long value, long minWidth) {
	BOOLEAN isNegative; char temp[12]; signed char pos; long digit;
	
	//use check
	if( (void*) eString == NULLA ) { Throw(QuadChara(0,'E','P','U'), (char*) "PE/CPP/EString; appendDecimal(); eString=NILA");}
	
	//force value to be positive
	if( value >= 0) {
		isNegative = FALSE;
	} else {
		isNegative = TRUE;
		value = 0 - value		;//make value positive
	}
	
	//end string with terminating zero to make it valid
	pos = 11;
	temp[pos] = '\000';
	
	//write string (from end of string) representing value as decimal digits
	if( value == 0) {
		pos--;
		temp[pos] = '0';
		minWidth--;
	} else {
		do {
			//extract right-most digit then remove from value
			digit= Mod(value, 10);
			value = ret2 ;
			
			//write digit as character
			pos--;
			temp[pos] = (char) ('0' + digit );
			minWidth--;
		} while(( value == 0)==0);
		
		//prepend minus sign if was negative
		if( isNegative) {
			pos--;
			temp[pos] = '-';
		}
	}
	
	//prepend required 0 digits
	while( minWidth > 0) {
		pos--;
		temp[pos] = '0';
		minWidth--;
	}
	
	//now append string representation to target E-string
	StrAdd(eString, temp, ALL, pos);
	return eString;
} 

/*** PE/Mem ***/
/* PE/Mem.e
   Implements New()/etc using any allocator, adding auto-deallocation upon program exit.
*/ 
class memNode;

const signed char ALIGN_SIZE=sizeof( long	);//must be a power of 2, and >= 4
const signed char ALIGN_SIZE_M1=ALIGN_SIZE - 1;


class memNode: public object {
public:
	memNode* next;	//circular
	memNode* prev;
};
extern memNode* memHead; extern memNode* memTail; extern void* memSem;
void new_Mem();
void end_Mem();
memNode* memHead=NULL; memNode* memTail=NULL; void* memSem=NULL;


void new_Mem() {
	//a dummy allocation so that never have to deal with head or tail being NIL (nor removal of tail node)
	memHead = (memNode*) baseNew(sizeof( memNode)  + ALIGN_SIZE_M1 & ~ ALIGN_SIZE_M1 		) ; if( (void*) memHead == NULL ) { Raise(QuadChara(0,'M','E','M'));}
	memHead->next = memHead;
	memHead->prev = memHead;
	memTail = memHead;
	
	memSem = baseNewSemaphore();
	return ;
}

void end_Mem() {
	memNode* node=NULL; memNode* next=NULL;
	
	//auto-deallocate anything remaining in the list
	next = memHead;
	do {
		node = next;
		next = node->next;
		
		baseDispose(node);
	} while(( next == memHead)==0);
	memHead = (memNode*) NULL;
	memTail = (memNode*) NULL;
	
	memSem = baseDisposeSemaphore(memSem);
	return ;
}


void* NewR(long size, BOOLEAN noClear)  {
	void* mem=NULL; 
	mem = New(size, noClear);
	if( mem == NULL ) { Raise(QuadChara(0,'M','E','M'));}
	return mem ;
} 

void* New(long size, BOOLEAN noClear)  {
	void* mem=NULL; 
	memNode* node=NULL;
	
	if( node = (memNode*) baseNew(size + (sizeof( memNode)  + ALIGN_SIZE_M1 & ~ ALIGN_SIZE_M1)		, noClear)) {
		//add mem to head of linked list
		baseSemLock(memSem);
		node->prev = memTail;
		node->next = memHead;
		memTail->next = node;
		memHead->prev = node;
		memHead = node;
		baseSemUnlock(memSem);
		
		//return mem without node header
		mem = (memNode*) ((long) node + (sizeof( memNode)  + ALIGN_SIZE_M1 & ~ ALIGN_SIZE_M1)		);
	}
	return mem ;
} 

void* Dispose(void* mem)  {
	memNode* node=NULL;
	
	if( mem) {
		//retrieve node
		node = (memNode*) ((long) mem - (sizeof( memNode)  + ALIGN_SIZE_M1 & ~ ALIGN_SIZE_M1)		);
		
		//remove mem from linked list
		baseSemLock(memSem);
		node->next->prev = node->prev;
		node->prev->next = node->next;
		if( memHead == node ) { memHead = node->next;}
		baseSemUnlock(memSem);
		
		//finally perform deallocation
		baseDispose(node);
	}
	return NULLA;
} 

/*** PE/pSemaphores_dummy ***/
/* dummy implementation of pSemaphores, for single-threaded programs */ 

void* NewSemaphore()  {
	void* sem=NULL; 
	sem = NULL;
	return sem ;
} 

void* DisposeSemaphore(void* sem)  {
	void* nil=NULL; 
	sem = NULL;
	nil = NULL;
	return nil ;
} 

void SemLock(void* sem)  {
	sem = NULL;
	return ;
}

void SemUnlock(void* sem)  {
	sem = NULL;
	return ;
}

BOOLEAN SemTryLock(void* sem)  {
	BOOLEAN success; 
	sem = NULL;
	success = TRUE;
	return success ;
} 

/*** PE/FastMem ***/
/* PE/FastMem.e 11-09-2020
	An incredibly fast memory allocator, with O(1) performance.


Copyright (c) 2009,2010,2011,2012,2016,2020 Christopher Steven Handley ( http://cshandley.co.uk/email )
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

* The source code must not be modified after it has been translated or converted
away from the PortablE programming language.  For clarification, the intention
is that all development of the source code must be done using the PortablE
programming language (as defined by Christopher Steven Handley).

* Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/ /*
This allocator is based upon the TLSF algorithm, but adapted to work on a
per-program basis (rather than the per-OS basis that it was intended for).  It
adds space (aka "pools") as needed, starting off at 64KB, and gradually
increasing up to 16MB.  Pools are removed when they become empty.

Unfortunately the TLSF algorithm has an overhead of 8 bytes per block, which is
unacceptable for small allocations, plus it's O(1) constant time allocation is
still quite large.  So on top of it there is a "Slab" allocator (loosly based
upon the real Slab allocator & also upon AmigaE's FastNew algorithm), which for
blocks <= 256 bytes will pre-allocate 16 blocks of identical size in a single
slab.  The downside is that blocks > 256 bytes now have an overhead of 12 ??
bytes per block.  But the upside is that blocks <= 256 bytes only have an
overhead of 2.875 ??  bytes per block, and their O(1) constant allocation time
is small.

(?? = this was calculated before SIZEOF_slab increase from 10 to 12 bytes.)
*/ 
class q;
class qq;
class qqp;
class qqq;
class qqpp;
class qqpq;
class qqqp;
class qqqq;
class qqppp;


class q: public object {
public:long q_q;qqpq* qq_q[11342 ^ 0x2C6E];};
class qq: public object {
public:long q_qq;q qq_qq[11342 ^ 0x2C6E]; qqp* qqp_qq;long qqq_qq;long qqpp_qq;qqp* qqpq_qq; };
class qqp: public object {
public:qqp* q_qqp; qqp* qq_qqp; long qqp_qqp;};
class qqq: public object {
public:long q_qqq;qqq* qq_qqq; };
class qqpp: public qqq {  };
class qqpq: public qqq {
public:qqpq* qqp_qqpq; qqpq* qqq_qqpq; };
class qqqp: public qq {
public:qqqq* qqqp_qqqp[(6788805 ^ 0x6797C1)/(281402 ^ 0x44B3E)+(27991862 ^ 0x1AB1F37)];};
class qqqq: public object {
public:qqqq* q_qqqq; qqqq* qq_qqqq; short qqp_qqqq;};
class qqppp: public object {
public:signed char q_qqppp; signed char qq_qqppp;};
extern qqqp* q_FastMem; extern void* qq_FastMem; extern char* qqp_FastMem; extern char* qqq_FastMem; extern char* qqpp_FastMem; extern char* qqpq_FastMem; extern char* qqqp_FastMem; extern char* qqqq_FastMem; extern char* qqppp_FastMem; extern char* qqppq; extern char* qqpqp; extern char* qqpqq; extern char* qqqpp; extern char* qqqpq; extern char* qqqqp; extern char* qqqqq; extern char* qqpppp; extern char* qqpppq; extern char* qqppqp; extern char* qqppqq; extern char* qqpqpp; extern char* qqpqpq; extern char* qqpqqp; extern char* qqpqqq; extern char* qqqppp; extern char* qqqppq; extern char* qqqpqp; extern char* qqqpqq; extern char* qqqqpp; extern char* qqqqpq; extern char* qqqqqp; extern char* qqqqqq;
void q_FastMem2(int qqppppp, char* qqppppq=NULLA);
signed char qqp_FastMem2(long qqppppp) ;
signed char qqq_FastMem2(long qqppppp) ;
qq* qqpp_FastMem2(long qqppppp) ;
void qqpq_FastMem2(qq* qqppppp);
long qqqp_FastMem2(qq* qqppppp) ;
long qqqq_FastMem2(long qqppppp, long* qqppppq=NULLA) ;
qqpq* qqppq_FastMem(qq* qqppppp, long qqppppq, long* qqpppqp=NULLA, long* qqpppqq=NULLA, q** qqppqpp=NULLA) ;
qqpp* qqpqp_FastMem(qq* qqppppp, qqpq* qqppppq, long qqpppqp, long qqpppqq, q* qqppqpp) ;
qqpp* qqpqq_FastMem(qq* qqppppp, qqpq* qqppppq) ;
void qqqpp_FastMem(qq* qqppppp, qqpp* qqppppq);
qqpp* qqqpq_FastMem(qqpp* qqppppp, long qqppppq, long qqpppqp) ;
qqpp* qqqqp_FastMem(qqpp* qqppppp, qqpp* qqppppq) ;
BOOLEAN qqqqq_FastMem(qq* qqppppp, long qqppppq) ;
void qqpppp_FastMem(qq* qqppppp, qqp* qqppppq);
void* qqpppq_FastMem(qq* qqppppp, long qqppppq, long* qqpppqp=NULLA) ;
void qqppqp_FastMem(qq* qqppppp, void* qqppppq);
void qqppqq_FastMem(qq* qqppppp);
qqqp* qqpqpp_FastMem() ;
void qqpqpq_FastMem(qqqp* qqppppp);
void* qqpqqp_FastMem(qqqp* qqppppp, long qqppppq, long* qqpppqp=NULLA, BOOLEAN qqpppqq=108601 ^ 0x1A839) ;
void qqpqqq_FastMem(qqqp* qqppppp, void* qqppppq);
void new_FastMem();
void end_FastMem();
qqqp* q_FastMem=NULL; void* qq_FastMem=NULL; char* qqp_FastMem=NULL; char* qqq_FastMem=NULL; char* qqpp_FastMem=NULL; char* qqpq_FastMem=NULL; char* qqqp_FastMem=NULL; char* qqqq_FastMem=NULL; char* qqppp_FastMem=NULL; char* qqppq=NULL; char* qqpqp=NULL; char* qqpqq=NULL; char* qqqpp=NULL; char* qqqpq=NULL; char* qqqqp=NULL; char* qqqqq=NULL; char* qqpppp=NULL; char* qqpppq=NULL; char* qqppqp=NULL; char* qqppqq=NULL; char* qqpqpp=NULL; char* qqpqpq=NULL; char* qqpqqp=NULL; char* qqpqqq=NULL; char* qqqppp=NULL; char* qqqppq=NULL; char* qqqpqp=NULL; char* qqqpqq=NULL; char* qqqqpp=NULL; char* qqqqpq=NULL; char* qqqqqp=NULL; char* qqqqqq=NULL;
void q_FastMem2(int qqppppp, char* qqppppq) { if( 9058 ^ 0x2362) { if( qqppppp ) { PrintException();} } Throw(qqppppp, qqppppq);
	return ;}
signed char qqp_FastMem2(long qqppppp)  {signed char qqppppq; if( qqppppp & (1632992 ^ 0xFFE7EAE0)) { if( qqppppp & (245542 ^ 0xFF03BF26)) { if( qqppppp & (7237714 ^ 0xF06E7052)) { if( qqppppp & (8186489 ^ 0xC07CEA79)) { if( qqppppp & (191961 ^ 0x8002EDD9)) { qqppppq = 368915 ^ 0x5A10C; } else {  qqppppq = 98783552 ^ 0x5E3515E; } } else {  if( qqppppp & (102130904 ^ 0x261664D8)) { qqppppq = 173737 ^ 0x2A6B4; } else {  qqppppq = 258639 ^ 0x3F253; } } } else {  if( qqppppp & (123613334 ^ 0xB5E3096)) { if( qqppppp & (971112 ^ 0x80ED168)) { qqppppq = 413342 ^ 0x64E85; } else {  qqppppq = 1280461676 ^ 0x4C524B76; } } else {  if( qqppppp & (6229814 ^ 0x25F0F36)) { qqppppq = 71745381 ^ 0x446BF7C; } else {  qqppppq = 93417 ^ 0x16CF1; } } } } else {  if( qqppppp & (7560259 ^ 0x835C43)) { if( qqppppp & (16505 ^ 0xC04079)) { if( qqppppp & (6321809 ^ 0xE07691)) { qqppppq = 382701 ^ 0x5D6FA; } else {  qqppppq = 1272054124 ^ 0x4BD2017A; } } else {  if( qqppppp & (2615205 ^ 0x7E7A5)) { qqppppq = 413420 ^ 0x64EF9; } else {  qqppppq = 158488 ^ 0x26B0C; } } } else {  if( qqppppp & (7979898 ^ 0x75C37A)) { if( qqppppp & (194786 ^ 0xAF8E2)) { qqppppq = 1973804 ^ 0x1E1E3F; } else {  qqppppq = 61202411 ^ 0x3A5DFF9; } } else {  if( qqppppp & (52673751 ^ 0x321BCD7)) { qqppppq = 75840383 ^ 0x4853B6E; } else {  qqppppq = 339515 ^ 0x52E2B; } } } } } else if( qqppppp & (1767862572 ^ 0x695F8ED3)) { if( qqppppp & (253594668 ^ 0xF1D732C)) { if( qqppppp & (1501439624 ^ 0x597ED688)) { if( qqppppp & (3098008 ^ 0x2F8598)) { if( qqppppp & (384634 ^ 0x55E7A)) { qqppppq = 803946 ^ 0xC4465; } else {  qqppppq = 2732006 ^ 0x29AFE8; } } else {  if( qqppppp & (101361453 ^ 0x60A872D)) { qqppppq = 6238034 ^ 0x5F2F5F; } else {  qqppppq = 253958 ^ 0x3E00A; } } } else {  if( qqppppp & (144873 ^ 0x239E9)) { if( qqppppp & (749249184 ^ 0x2CA8AAA0)) { qqppppq = 384186316 ^ 0x16E637C7; } else {  qqppppq = 1309956624 ^ 0x4E145A1A; } } else {  if( qqppppp & (289513 ^ 0x468E9)) { qqppppq = 771935632 ^ 0x2E02CD99; } else {  qqppppq = 324378 ^ 0x4F312; } } } } else {  if( qqppppp & (418582 ^ 0x663E6)) { if( qqppppp & (610248500 ^ 0x245FA7F4)) { if( qqppppp & (5426361 ^ 0x52CC39)) { qqppppq = 67602316 ^ 0x407878B; } else {  qqppppq = 1686717680 ^ 0x648944F6; } } else {  if( qqppppp & (11342 ^ 0x2C6E)) { qqppppq = 73008632 ^ 0x45A05FD; } else {  qqppppq = 46790608 ^ 0x2C9F7D4; } } } else {  if( qqppppp & (253958 ^ 0x3E00A)) { if( qqppppp & (2222035 ^ 0x21E7DB)) { qqppppq = 26587407 ^ 0x195B10C; } else {  qqppppq = 1977993 ^ 0x1E2E8B; } } else {  if( qqppppp & (1727723148 ^ 0x66FAF68E)) { qqppppq = 892908868 ^ 0x3538B545; } else {  qqppppq = 285809 ^ 0x45C71; } } } } } else {  qqppppq = -(460518 ^ 0x706E7); }
	return qqppppq ;} 
signed char qqq_FastMem2(long qqppppp)  {signed char qqppppq; if( qqppppp & (1767862572 ^ 0x695F8ED3)) { if( qqppppp & (43173449 ^ 0x292C6B6)) { if( qqppppp & (803946 ^ 0xC4465)) { if( qqppppp & (214308 ^ 0x34527)) { if( qqppppp & (114791197 ^ 0x6D7931C)) { qqppppq = 1659663280 ^ 0x62EC73B0; } else {  qqppppq = 1195807388 ^ 0x4746929D; } } else {  if( qqppppp & (404927 ^ 0x62DBB)) { qqppppq = 813155116 ^ 0x3077C32E; } else {  qqppppq = 881903040 ^ 0x3490C5C3; } } } else {  if( qqppppp & (720684976 ^ 0x2AF4C780)) { if( qqppppp & (339515 ^ 0x52E2B)) { qqppppq = 351838 ^ 0x55E5A; } else {  qqppppq = 6181091 ^ 0x5E50E6; } } else {  if( qqppppp & (809714 ^ 0xC5AB2)) { qqppppq = 129613036 ^ 0x7B9BCEA; } else {  qqppppq = 18758409 ^ 0x11E3B0E; } } } } else {  if( qqppppp & (36903212 ^ 0x233162C)) { if( qqppppp & (2642523 ^ 0x28515B)) { if( qqppppp & (686505848 ^ 0x28EB3E78)) { qqppppq = 1654446388 ^ 0x629CD93C; } else {  qqppppq = 161256 ^ 0x275E1; } } else {  if( qqppppp & (6769647 ^ 0x674FEF)) { qqppppq = 1309956624 ^ 0x4E145A1A; } else {  qqppppq = 384186316 ^ 0x16E637C7; } } } else {  if( qqppppp & (1469761100 ^ 0x579AF64C)) { if( qqppppp & (802738660 ^ 0x2FD8C1E4)) { qqppppq = 253958 ^ 0x3E00A; } else {  qqppppq = 6238034 ^ 0x5F2F5F; } } else {  if( qqppppp & (324011 ^ 0x4B1AB)) { qqppppq = 2732006 ^ 0x29AFE8; } else {  qqppppq = 803946 ^ 0xC4465; } } } } } else if( qqppppp & (1632992 ^ 0xFFE7EAE0)) { if( qqppppp & (402214 ^ 0xF92326)) { if( qqppppp & (53957259 ^ 0x338528B)) { if( qqppppp & (83410703 ^ 0x4FBBF0F)) { if( qqppppp & (5270768 ^ 0x516CF0)) { qqppppq = 339515 ^ 0x52E2B; } else {  qqppppq = 75840383 ^ 0x4853B6E; } } else {  if( qqppppp & (8206627 ^ 0x793923)) { qqppppq = 61202411 ^ 0x3A5DFF9; } else {  qqppppq = 1973804 ^ 0x1E1E3F; } } } else {  if( qqppppp & (2864738 ^ 0x1BB662)) { if( qqppppp & (426496 ^ 0x168200)) { qqppppq = 158488 ^ 0x26B0C; } else {  qqppppq = 413420 ^ 0x64EF9; } } else {  if( qqppppp & (1057089024 ^ 0x3F41E600)) { qqppppq = 1272054124 ^ 0x4BD2017A; } else {  qqppppq = 382701 ^ 0x5D6FA; } } } } else {  if( qqppppp & (7891988 ^ 0xF786C14)) { if( qqppppp & (2282894 ^ 0x322D58E)) { if( qqppppp & (1134815952 ^ 0x42A3EAD0)) { qqppppq = 93417 ^ 0x16CF1; } else {  qqppppq = 71745381 ^ 0x446BF7C; } } else {  if( qqppppp & (3729608 ^ 0x438E8C8)) { qqppppq = 1280461676 ^ 0x4C524B76; } else {  qqppppq = 413342 ^ 0x64E85; } } } else {  if( qqppppp & (1325843 ^ 0x30143B13)) { if( qqppppp & (37922237 ^ 0x1242A5BD)) { qqppppq = 258639 ^ 0x3F253; } else {  qqppppq = 173737 ^ 0x2A6B4; } } else {  if( qqppppp & (1733469336 ^ 0x2752A498)) { qqppppq = 98783552 ^ 0x5E3515E; } else {  qqppppq = 368915 ^ 0x5A10C; } } } } } else {  qqppppq = -(1409464396 ^ 0x5402B84D); }
	return qqppppq ;} 
qq* qqpp_FastMem2(long qqppppp)  {qq* qqppppq=NULL; qqppppq = (qq*) NewR(qqppppp) ; qqppppq->qqp_qq = (qqp*) NULL; qqppppq->qqq_qq = 24354903 ^ 0x173A057; qqppppq->qqpp_qq = 3653052 ^ 0x37BDBC; qqppppq->qqpq_qq = (qqp*) NULL;
	return qqppppq ;} 
void qqpq_FastMem2(qq* qqppppp) {qqp* qqppppq=NULL; qqp* qqpppqp=NULL; qqppppq = qqppppp->qqp_qq; while( qqppppq) { qqpppqp = qqppppq->q_qqp; Dispose((void*) qqppppq ); qqppppq = qqpppqp; } Dispose(qqppppp);
	return ;}
long qqqp_FastMem2(qq* qqppppp)  {long qqppppq, qqpppqp, qqpppqq, qqppqpp, qqppqpq; qqp* qqppqqp=NULL; qqq* qqppqqq=NULL; long qqpqppp; BOOLEAN qqpqppq;int temp_QUAD;
try {
	temp_QUAD = exception ;
	exception = 0 ; qqppppq = 60771 ^ 0xED63; qqpppqp = 6874816 ^ 0x68E6C0; qqpppqq = 98738809 ^ 0x5E2A279; qqppqpp = 49709254 ^ 0x2F680C6; qqppqpq = 5052592 ^ 0x4D18B0; qqppqqp = qqppppp->qqp_qq; while( qqppqqp) { qqppqqq = (qqq*) ((long) qqppqqp + (sizeof( qqp  )+ (359497 ^ 0x57C4A) & ~ (888927628 ^ 0x34FBF58F)) ); qqpqppp = (long) ((long) qqppqqp + qqppqqp->qqp_qqp); do { qqpqppq = - ((qqppqqq->q_qqq & (121363601 ^ 0x73BDC93) )!= (111945006 ^ 0x6AC252E)); if( qqpqppq == (4846322 ^ 0x49F2F2)) { qqpppqp= qqpppqp+(122660920 ^ 0x74FA839); qqppppq = qqppppq + (qqppqqq->q_qqq & (164243 ^ -0x28191)); } else { qqppqpp= qqppqpp+(133613035 ^ 0x7F6C5EA); qqpppqq = qqpppqq + (qqppqqq->q_qqq & (164243 ^ -0x28191)); } qqppqqq = (qqq*) ((long) qqppqqq + (qqppqqq->q_qqq & (164243 ^ -0x28191))); } while(( (long) qqppqqq >= qqpqppp)==0); qqppqpq= qqppqpq+(133613035 ^ 0x7F6C5EA); qqppqqp = qqppqqp->q_qqp; }
} catch(...) {}
	ret5 = qqppqpq ;
	ret4 = qqppqpp ;
	ret3 = qqpppqq ;
	ret2 = qqpppqp ;
	if (exception!=0) {throw eException ;} else {EMPTY;};
	exception = temp_QUAD ;
	return qqppppq ;} 
long qqqq_FastMem2(long qqppppp, long* qqppppq)  {long qqpppqp, qqpppqq; qqpppqp = qqp_FastMem2(qqppppp); qqpppqq = (( qqpppqp - (427285 ^ 0x68510) >= 0 )? qqppppp >> qqpppqp - (427285 ^ 0x68510) : qqppppp << labs(qqpppqp - (427285 ^ 0x68510) ))& ((70086787 ^ 0x42D7082) << (1049060808 ^ 0x3E8765CD)  )- (6367425 ^ 0x6128C0)  ; if( qqppppq ) { qqppppq[(5364256 ^ 0x51DA20)] = qqpppqq;}
	return qqpppqp ;} 
qqpq* qqppq_FastMem(qq* qqppppp, long qqppppq, long* qqpppqp, long* qqpppqq, q** qqppqpp)  {qqpq* qqppqpq=NULL; long qqppqqp, qqppqqq; q* qqpqppp=NULL; long qqpqppq, qqpqpqp, qqpqpqq;long temp_VALUE; int temp_QUAD;try {
	temp_QUAD = exception ;
	exception = 0 ; temp_VALUE = qqp_FastMem2(qqppppq) - (1630787924 ^ 0x6133D951); qqpqppq = qqqq_FastMem2(qqppppq + (( temp_VALUE >= 0 )? (373211 ^ 0x5B1DA )<< temp_VALUE : (373211 ^ 0x5B1DA )>> labs(temp_VALUE ))- (1722650936 ^ 0x66AD9139), &  qqpqpqp); qqpqppp = & qqppppp->qq_qq[qqpqppq]; qqpqpqq = qqpqppp->q_q & (318201 ^ 0xFFFB2506) << qqpqpqp; if( qqpqpqq != (88978724 ^ 0x54DB524)) { qqppqqq = qqq_FastMem2(qqpqpqq); qqppqqp = qqpqppq; } else { qqpqpqq = qqppppp->q_qq & (318201 ^ 0xFFFB2506) << qqpqppq + (88978724 ^ 0x54DB525); if( qqpqpqq == (218523 ^ 0x3559B) ) { qqppqqp=qqppqqq=-(79637997 ^ 0x4BF2DEC) ; qqpqppp=(q*) NULL ; qqppqpq = (qqpq*) NULL ; goto finally; }  qqppqqp = qqq_FastMem2(qqpqpqq); qqpqppp = & qqppppp->qq_qq[qqppqqp]; qqppqqq = qqq_FastMem2(qqpqppp->q_q); } qqppqpq = qqpqppp->qq_q[qqppqqq]; if( (long) qqppqpq == (long) (957377904 ^ 0x39106D70) ) { q_FastMem2(QuadChara(0,'B','U','G'), qqp_FastMem );}
finally: 0; } catch(...) {} if( qqpppqp ) { qqpppqp[(298826 ^ 0x48F4A)] = qqppqqp;} if( qqpppqq ) { qqpppqq[(1361132436 ^ 0x51213B94)] = qqppqqq;} if( qqppqpp ) { qqppqpp [(105705682 ^ 0x64CF0D2)] = qqpqppp;}
	if (exception!=0) {throw eException ;} else {EMPTY;};
	exception = temp_QUAD ;
	return qqppqpq ;} 
qqpp* qqpqp_FastMem(qq* qqppppp, qqpq* qqppppq, long qqpppqp, long qqpppqq, q* qqppqpp)  {qqpp* qqppqpq=NULL; long qqppqqp, qqppqqq; BOOLEAN qqpqppp; if( 25054 ^ 0x61DE) { qqppqqp = qqqq_FastMem2(qqppppq->q_qqq & (164243 ^ -0x28191), &  qqppqqq); if( - (qqpppqp != qqppqqp  )| - (qqpppqq != qqppqqq)) { printf(qqq_FastMem ,qqppppq->q_qqq ,qqppppq->q_qqq, qqpppqp, qqppqqp, qqpppqq, qqppqqq); } if( qqpppqp != qqppqqp ) { q_FastMem2(QuadChara(0,'B','U','G'), qqpp_FastMem );} if( qqpppqq != qqppqqq ) { q_FastMem2(QuadChara(0,'B','U','G'), qqpq_FastMem );} } if( 133613035 ^ 0x7F6C5EB ) { if( qqppqpp != & qqppppp->qq_qq[qqpppqp] ) { q_FastMem2(QuadChara(0,'B','U','G'), qqqp_FastMem );}} qqpqppp = 427285 ^ 0x68515; if( (long) qqppppq->qqp_qqpq != (long) (70086787 ^ 0x42D7083)) { qqppppq->qqp_qqpq->qqq_qqpq = qqppppq->qqq_qqpq; } if( (long) qqppppq->qqq_qqpq != (long) (1049060808 ^ 0x3E8765C8)) { qqppppq->qqq_qqpq->qqp_qqpq = qqppppq->qqp_qqpq; } else { if( 6367425 ^ 0x6128C1 ) { if( qqppqpp->qq_q[qqpppqq] != qqppppq ) { q_FastMem2(QuadChara(0,'B','U','G'), qqqq_FastMem ) ;}} qqppqpp->qq_q[qqpppqq] = qqppppq->qqp_qqpq; if( (long) qqppppq->qqp_qqpq == (long) (5364256 ^ 0x51DA20)) { qqpqppp = 373211 ^ -0x5B1DC; } else { qqppppq->qqp_qqpq->qqq_qqpq = (qqpq*) NULL; } } qqppppq->q_qqq = qqppppq->q_qqq & ~ (1630787924 ^ 0x6133D956); qqppqpq = (qqpp*) (qqq*) qqppppq ; if( qqpqppp) { qqppqpp->q_q = qqppqpp->q_q & ~ ((1722650936 ^ 0x66AD9139) << qqpppqq); if( qqppqpp->q_q == (399718 ^ 0x61966)) { qqppppp->q_qq = qqppppp->q_qq & ~ ((478810 ^ 0x74E5B) << qqpppqp); } }
	return qqppqpq ;} 
qqpp* qqpqq_FastMem(qq* qqppppp, qqpq* qqppppq)  {qqpp* qqpppqp=NULL; long qqpppqq, qqppqpp; q* qqppqpq=NULL; qqpppqq = qqqq_FastMem2(qqppppq->q_qqq & (164243 ^ -0x28191), &  qqppqpp); qqppqpq = & qqppppp->qq_qq[qqpppqq]; qqpppqp = qqpqp_FastMem(qqppppp, qqppppq, qqpppqq, qqppqpp, qqppqpq);
	return qqpppqp ;} 
void qqqpp_FastMem(qq* qqppppp, qqpp* qqppppq) {qqpq* qqpppqp=NULL; long qqpppqq, qqppqpp; q* qqppqpq=NULL; qqppppq->q_qqq = qqppppq->q_qqq | 133613035 ^ 0x7F6C5E9; qqpppqp = (qqpq*) (qqq*) qqppppq ; qqpppqq = qqqq_FastMem2(qqpppqp->q_qqq & (164243 ^ -0x28191), &  qqppqpp); qqppqpq = & qqppppp->qq_qq[qqpppqq]; qqpppqp->qqp_qqpq = qqppqpq->qq_q[qqppqpp]; qqpppqp->qqq_qqpq = (qqpq*) NULL; if( qqpppqp->qqp_qqpq ) { qqpppqp->qqp_qqpq->qqq_qqpq = qqpppqp;} qqppqpq->qq_q[qqppqpp] = qqpppqp; qqppqpq->q_q = qqppqpq->q_q | (133613035 ^ 0x7F6C5EA) << qqppqpp; qqppppp->q_qq = qqppppp->q_qq | (427285 ^ 0x68514) << qqpppqq;
	return ;}
qqpp* qqqpq_FastMem(qqpp* qqppppp, long qqppppq, long qqpppqp)  {qqpp* qqpppqq=NULL; qqq* qqppqpp=NULL;  if( qqppppq & (70086787 ^ 0x42D7080) ) { q_FastMem2(QuadChara(0,'B','U','G'), qqppp_FastMem );} if( 1049060808 ^ 0x3E8765C8 ) { if( qqpppqp != (qqppppp->q_qqq & (164243 ^ -0x28191)) ) { q_FastMem2(QuadChara(0,'B','U','G'), qqppq );}} if( 133613035 ^ 0x7F6C5EB ) { if( qqpppqp & (427285 ^ 0x68516) ) { q_FastMem2(QuadChara(0,'B','U','G'), qqpqp );}} qqpppqq = (qqpp*) ((long) qqppppp + qqppppq); qqpppqq->q_qqq = qqpppqp - qqppppq ; if( qqppppp->q_qqq & (70086787 ^ 0x42D7082) ) { qqpppqq->q_qqq = qqpppqq->q_qqq | 1049060808 ^ 0x3E8765C9;} qqpppqq->qq_qqq = qqppppp; qqppppp->q_qqq = qqppppq ; if( (qqpppqq->q_qqq & (6367425 ^ 0x6128C0) )== (5364256 ^ 0x51DA20)) { qqppqpp = (qqpp*) ((long) qqppppp + qqpppqp); qqppqpp->qq_qqq = qqpppqq; }
	return qqpppqq ;} 
qqpp* qqqqp_FastMem(qqpp* qqppppp, qqpp* qqppppq)  {qqpp* qqpppqp=NULL; qqq* qqpppqq=NULL; if( 373211 ^ 0x5B1DB ) { if( (qqq*) qqppppp != qqppppq->qq_qqq ) { q_FastMem2(QuadChara(0,'B','U','G'), qqpqq );}} qqppppp->q_qqq = (qqppppp->q_qqq & (164243 ^ -0x28191)  )+ (qqppppq->q_qqq & (164243 ^ -0x28191)) ; if( qqppppq->q_qqq & (133613035 ^ 0x7F6C5EA)) { qqppppp->q_qqq = qqppppp->q_qqq | 427285 ^ 0x68514; } else { qqpppqq = (qqpp*) ((long) qqppppq + (qqppppq->q_qqq & (164243 ^ -0x28191))); qqpppqq->qq_qqq = qqppppp; } qqpppqp = qqppppp;
	return qqpppqp ;} 
BOOLEAN qqqqq_FastMem(qq* qqppppp, long qqppppq)  {BOOLEAN qqpppqp; long qqpppqq; qqp* qqppqpp=NULL; qqpp* qqppqpq=NULL; qqpppqp = 133613035 ^ 0x7F6C5EB; qqppppq = qqppppq  + (427285 ^ 0x68516) & ~ (70086787 ^ 0x42D7080)  ; qqpppqq = (sizeof( qqp  )+ (1049060808 ^ 0x3E8765CB) & ~ (6367425 ^ 0x6128C2)   )+ qqppppq; qqppqpp = (qqp*) New(qqpppqq,  5364256 ^ -0x51DA21); if( (long) qqppqpp == (long) (373211 ^ 0x5B1DB) ) { goto finally;} qqppqpp->qqp_qqp = qqpppqq; qqppppp->qqq_qq = qqppppp->qqq_qq +  (qqppqpp->qqp_qqp - (sizeof( qqp  )+ (1630787924 ^ 0x6133D957) & ~ (1722650936 ^ 0x66AD913B)) ); qqppppp->qqpp_qq= qqppppp->qqpp_qq+(399718 ^ 0x61967); qqppqpp->q_qqp = qqppppp->qqp_qq; qqppqpp->qq_qqp = (qqp*) NULL; if( qqppppp->qqp_qq ) { qqppppp->qqp_qq->qq_qqp = qqppqpp;} qqppppp->qqp_qq = qqppqpp; qqppqpq = (qqpp*) ((long) qqppqpp + (sizeof( qqp  )+ (478810 ^ 0x74E59) & ~ (333828 ^ 0x51807)) ); qqppqpq->q_qqq = qqppppq | 131301172 ^ 0x7D37F35; qqppqpq->qq_qqq = (qqq*) NULL; qqqpp_FastMem(qqppppp, qqppqpq); qqpppqp = 274132 ^ -0x42ED5;
finally: 0; 
	return qqpppqp ;} 
void qqpppp_FastMem(qq* qqppppp, qqp* qqppppq) {qqp* qqpppqp=NULL; qqp* qqpppqq=NULL; qqq* qqppqpp=NULL; qqppqpp = (qqq*) ((long) qqppppq + (sizeof( qqp  )+ (6568520 ^ 0x643A4B) & ~ (406216 ^ 0x632CB)) ); if( 108601 ^ 0x1A839) { if( (qqppqpp->q_qqq & (52994 ^ 0xCF03  | 277498 ^ 0x43BF8) )!= (5180577 ^ 0x4F0CA0  | 1431563048 ^ 0x5553EB2A) ) { q_FastMem2(QuadChara(0,'E','M','U'), qqqpp );} if( (long) qqppqpp->qq_qqq != (long) (1513891464 ^ 0x5A3C2688) ) { q_FastMem2(QuadChara(0,'B','U','G'), qqqpq );} } qqpqq_FastMem(qqppppp, (qqpq*) qqppqpp); qqpppqp = qqppppq->qq_qqp; qqpppqq = qqppppq->q_qqp; if( qqpppqp) { qqpppqp->q_qqp = qqpppqq; } else { if( 24905808 ^ 0x17C0850 ) { if( qqppppp->qqp_qq != qqppppq ) { q_FastMem2(QuadChara(0,'B','U','G'), qqqqp );}} qqppppp->qqp_qq = qqpppqq; } if( qqpppqq) { qqpppqq->qq_qqp = qqpppqp; } qqppppp->qqq_qq = qqppppp->qqq_qq - (qqppppq->qqp_qqp - (sizeof( qqp  )+ (57970439 ^ 0x3748F04) & ~ (279830 ^ 0x44515)) ); qqppppp->qqpp_qq= qqppppp->qqpp_qq-(6973431 ^ 0x6A67F6); Dispose((void*) qqppppq );
	return ;}
void* qqpppq_FastMem(qq* qqppppp, long qqppppq, long* qqpppqp)  {void* qqpppqq=NULL; long qqppqpp; qqpq* qqppqpq=NULL; long qqppqqp, qqppqqq; q* qqpqppp=NULL; qqpp* qqpqppq=NULL; long qqpqpqp; qqpp* qqpqpqq=NULL; long qqpqqpp, qqpqqpq, qqpqqqp;long temp_VALUE, temp_VALUE2, temp_VALUE3; int temp_QUAD;    try {
	temp_QUAD = exception ;
	exception = 0 ; qqppppq = (( (sizeof( qqpp)+ (36638 ^ 0x8F1D) & ~ (69042425 ^ 0x41D80FA)    )+ qqppppq>sizeof( qqpq) )? (sizeof( qqpp)+ (36638 ^ 0x8F1D) & ~ (69042425 ^ 0x41D80FA)    )+ qqppppq : (long) sizeof( qqpq))+ (4077217 ^ 0x3E36A2) & ~ (260038 ^ 0x3F7C5)  ; qqppqpq = qqppq_FastMem(qqppppp, qqppppq, &  qqppqqp, &  qqppqqq, &  qqpqppp); if( (long) qqppqpq == (long) (231481 ^ 0x38839)) { if( (1712532420 ^ 0x66132BC4) == (26575189 ^ 0x1958155)) { temp_VALUE = qqppppp->qqq_qq + qqppppp->qqq_qq / (( (95022438 ^ 0x5A9ED67)>qqppppp->qqpp_qq )? (long) (95022438 ^ 0x5A9ED67 ): qqppppp->qqpp_qq); qqpqqpp = ( qqppppq * (107856328 ^ 0x66DC1C0)>temp_VALUE )? qqppppq * (107856328 ^ 0x66DC1C0) : temp_VALUE ; if( qqpqqpp > (1134815952 ^ 0x42A3EAD0)) { temp_VALUE2 = qqp_FastMem2(qqppppq) - (57322041 ^ 0x36AAA3C); qqpqqpq = qqqq_FastMem2(qqppppq + (( temp_VALUE2 >= 0 )? (1149818616 ^ 0x4488D6F9 )<< temp_VALUE2 : (1149818616 ^ 0x4488D6F9 )>> labs(temp_VALUE2 ))- (1389268200 ^ 0x52CE8CE9), &  qqpqqqp); qqpqqpp = ((145742 ^ 0x2394F) << (1459930 ^ 0x1646DF) | qqpqqqp )<< qqpqqpq - (1652789336 ^ 0x6283905D); qqpqqpp = ( qqpqqpp>(1134815952 ^ 0x42A3EAD0 ))? qqpqqpp : (long) (1134815952 ^ 0x42A3EAD0); } else if( qqpqqpp < (5270768 ^ 0x516CF0)) { qqpqqpp = 5270768 ^ 0x516CF0; } if( qqqqq_FastMem(qqppppp, qqpqqpp) == (3559588 ^ 0x3650A4)) { qqppqq_FastMem(qqppppp); temp_VALUE3 = qqp_FastMem2(qqppppq) - (432876 ^ 0x69AE9); qqpqqpq = qqqq_FastMem2(qqppppq + (( temp_VALUE3 >= 0 )? (1935184144 ^ 0x73589111 )<< temp_VALUE3 : (1935184144 ^ 0x73589111 )>> labs(temp_VALUE3 ))- (14570047 ^ 0xDE523E), &  qqpqqqp); qqpqqpp = ((4765754 ^ 0x48B83B) << (7307120 ^ 0x6F7F75) | qqpqqqp )<< qqpqqpq - (50183394 ^ 0x2FDBCE7); qqqqq_FastMem(qqppppp, qqpqqpp); } qqppqpq = qqppq_FastMem(qqppppp, qqppppq, &  qqppqqp, &  qqppqqq, &  qqpqppp); } if( (long) qqppqpq == (long) (1021364376 ^ 0x3CE0C898) ) { qqppqpp=7950136 ^ 0x794F38 ; qqpppqq = NULLA ; goto finally; }   } qqpqppq = qqpqp_FastMem(qqppppp, qqppqpq, qqppqqp, qqppqqq, qqpqppp); qqpqpqp = qqpqppq->q_qqq & (164243 ^ -0x28191); if( qqpqpqp - qqppppq >= sizeof( qqpq)+ (133613035 ^ 0x7F6C5EA)) { qqpqpqq = qqqpq_FastMem(qqpqppq, qqppppq, qqpqpqp); qqqpp_FastMem(qqppppp, qqpqpqq); qqpqpqp = qqppppq; } qqpppqq = (void*) ((long) qqpqppq + (sizeof( qqpp  )+ (427285 ^ 0x68516) & ~ (70086787 ^ 0x42D7080))  ); qqppqpp = qqpqpqp - (sizeof( qqpp  )+ (1049060808 ^ 0x3E8765CB) & ~ (6367425 ^ 0x6128C2))  ; if( 5364256 ^ 0x51DA20 ) { if( qqpppqq != (void*) ((long) ((long) qqpppqq  + (373211 ^ 0x5B1D8) )& ~ (1630787924 ^ 0x6133D957))   ) { q_FastMem2(QuadChara(0,'E','M','U'), qqqqq );}}
finally: 0;     } catch(...) {} if( qqpppqp ) { qqpppqp[(1722650936 ^ 0x66AD9138)] = qqppqpp;}
	if (exception!=0) {throw eException ;} else {EMPTY;};
	exception = temp_QUAD ;
	return qqpppqq ;} 
void qqppqp_FastMem(qq* qqppppp, void* qqppppq) {qqpp* qqpppqp=NULL; qqq* qqpppqq=NULL; qqpp* qqppqpp=NULL; qqq* qqppqpq=NULL; qqpp* qqppqqp=NULL; qqp* qqppqqq=NULL; qqpppqp = (qqpp*) ((long) qqppppq - (sizeof( qqpp  )+ (399718 ^ 0x61965) & ~ (478810 ^ 0x74E59))  ); qqpppqq = qqpppqp->qq_qqq; qqppqpq = (qqpp*) ((long) qqpppqp + (qqpppqp->q_qqq & (164243 ^ -0x28191))); if( qqpppqq) { if( qqpppqq->q_qqq & (133613035 ^ 0x7F6C5E9)) { qqppqpp = qqpqq_FastMem(qqppppp, (qqpq*) qqpppqq); qqpppqp = qqqqp_FastMem(qqppqpp, qqpppqp); } } if( (qqpppqp->q_qqq & (427285 ^ 0x68514) )== (70086787 ^ 0x42D7083)) { if( qqppqpq->q_qqq & (1049060808 ^ 0x3E8765CA)) { qqppqqp = qqpqq_FastMem(qqppppp, (qqpq*) qqppqpq); qqpppqp = qqqqp_FastMem(qqpppqp, qqppqqp); } } qqqpp_FastMem(qqppppp, qqpppqp); if( (6367425 ^ 0x6128C1) == (5364256 ^ 0x51DA20)) { if( (long) qqpppqp->qq_qqq == (long) (373211 ^ 0x5B1DB)) { if( qqpppqp->q_qqq & (1630787924 ^ 0x6133D955)) { qqppqqq = (qqp*) ((long) qqpppqp - (sizeof( qqp  )+ (1722650936 ^ 0x66AD913B) & ~ (399718 ^ 0x61965)) ); if( qqppqqq != qqppppp->qqpq_qq) { qqppqq_FastMem(qqppppp); qqppppp->qqpq_qq = qqppqqq; } } } }
	return ;}
void qqppqq_FastMem(qq* qqppppp) {qqq* qqppppq=NULL; if( qqppppp->qqpq_qq) { qqppppq = (qqq*) ((long) qqppppp->qqpq_qq + (sizeof( qqp  )+ (478810 ^ 0x74E59) & ~ (333828 ^ 0x51807)) ); if( (qqppppq->q_qqq & (131301172 ^ 0x7D37F35  | 274132 ^ 0x42ED6) )== (6568520 ^ 0x643A49  | 406216 ^ 0x632CA)) { qqpppp_FastMem(qqppppp, qqppppp->qqpq_qq); qqppppp->qqpq_qq = (qqp*) NULL; } }
	return ;}
qqqp* qqpqpp_FastMem()  {qqqp* qqppppp=NULL; qqppppp = (qqqp*) qqpp_FastMem2(sizeof( qqqp));
	return qqppppp ;} 
void qqpqpq_FastMem(qqqp* qqppppp) { qqpq_FastMem2(qqppppp);
	return ;}
void* qqpqqp_FastMem(qqqp* qqppppp, long qqppppq, long* qqpppqp, BOOLEAN qqpppqq)  {void* qqppqpp=NULL; long qqppqpq; signed char qqppqqp; qqppp* qqppqqq=NULL; long qqpqppp; signed char qqpqppq; qqqq* qqpqpqp=NULL;int temp_QUAD;try {
	temp_QUAD = exception ;
	exception = 0 ; qqpqppp = (52994 ^ 0xCF00) + qqppppq  + (277498 ^ 0x43BF9) & ~ (5180577 ^ 0x4F0CA2)   ; if( - (qqpqppp > (6788805 ^ 0x6797C1)  )| 281402 ^ 0x44B3A  | qqpppqq) { qqppqpp = qqpppq_FastMem(qqppppp, ((27991862 ^ 0x1AB1F34)  + (9058 ^ 0x2361) & ~ (377210 ^ 0x5C179)   )+ qqppppq, &  qqppqpq); if( (long) qqppqpp == (long) (142726 ^ 0x22D86) ) { qqppqpq=16041963 ^ 0xF4C7EB ; qqppqpp = NULLA ; goto finally; }  qqppqpp = (void*) ((long) qqppqpp + ((2011023484 ^ 0x77DDC87E)  + (1025321 ^ 0xFA52A) & ~ (734765 ^ 0xB362E)) ); qqppqpq = qqppqpq - ((102663679 ^ 0x61E85FD)  + (501119 ^ 0x7A57C) & ~ (5342007 ^ 0x518334)) ; qqppqqq = (qqppp*) ((long) qqppqpp - (38271644 ^ 0x247FA9E)); qqppqqq->q_qqppp = -(96074069 ^ 0x5B9F954); } else { qqpqppq = (signed char) (qqpqppp / (6927480 ^ 0x69B47C) ); if( 2016722204 ^ 0x7834BD1C ) { if( - (qqpqppq < (131639 ^ 0x20237)  )| - (qqpqppq > (6788805 ^ 0x6797C1)/(281402 ^ 0x44B3E))  ) { q_FastMem2(QuadChara(0,'B','U','G'), qqpppp );}} qqpqpqp = qqppppp->qqqp_qqqp[qqpqppq]; if( (long) qqpqpqp == (long) (27991862 ^ 0x1AB1F36)) { qqpqpqp = (qqqq*) qqpppq_FastMem(qqppppp, (2732006 ^ 0x29AFE8) + (339515 ^ 0x52E2B) * qqpqppp) ; if( (long) qqpqpqp == (long) (351838 ^ 0x55E5E)) { qqppqpp = qqpqqp_FastMem(qqppppp, qqppppq, qqpppqp,  6181091 ^ -0x5E50E4); if( qqpppqp ) { qqppqpq = qqpppqp[(398509508 ^ 0x17C0C5C4)];} goto finally; } qqpqpqp->q_qqqq = (qqqq*) NULL; qqpqpqp->qq_qqqq = (qqqq*) NULL ; qqpqpqp->qqp_qqqq = -(92863 ^ 0x16ABE) ; qqppppp->qqqp_qqqp[qqpqppq] = qqpqpqp; qqppqqp = 3723938 ^ 0x38D2A2 ; } else { qqppqqp = qqp_FastMem2(qqpqpqp->qqp_qqqq & (1767862572 ^ 0x695F8ED3)); } if( 980704 ^ 0xEF6E0 ) { if( - (qqppqqp < (1917490104 ^ 0x724A93B8)  )| - (qqppqqp > (803946 ^ 0xC4465))  ) { q_FastMem2(QuadChara(0,'B','U','G'), qqpppq );}} qqppqqq = (qqppp*) ((long) ((long) qqpqpqp + (2732006 ^ 0x29AFE8) )+ qqppqqp * qqpqppp); qqppqpp = (void*) ((long) qqppqqq + (78883012 ^ 0x4B3A8C6)); qqppqpq = qqpqppp - (4752420 ^ 0x488426); qqppqqq-> q_qqppp = qqpqppq; qqppqqq->qq_qqppp = qqppqqp; qqpqpqp->qqp_qqqq = (short) (qqpqpqp->qqp_qqqq & ~ ((198438856 ^ 0xBD3EFC9) << qqppqqp)); if( qqpqpqp->qqp_qqqq == (977710868 ^ 0x3A46AF14)) { if( 2879046 ^ 0x2BEE46 ) { if( (long) qqpqpqp->q_qqqq != (long) (426561 ^ 0x68241) ) { q_FastMem2(QuadChara(0,'B','U','G'), qqppqp );}} if( 474047 ^ 0x73BBF ) { if( qqppppp->qqqp_qqqp[qqpqppq] != qqpqpqp ) { q_FastMem2(QuadChara(0,'B','U','G'), qqppqq );}} qqppppp->qqqp_qqqp[qqpqppq] = qqpqpqp->qq_qqqq; if( qqpqpqp->qq_qqqq ) { qqpqpqp->qq_qqqq->q_qqqq = (qqqq*) NULL;} } } if( 415460 ^ 0x656E4 ) { if( qqppqpp != (void*) ((long) ((long) qqppqpp  + (134754 ^ 0x20E61) )& ~ (110944398 ^ 0x69CE08D))   ) { q_FastMem2(QuadChara(0,'E','M','U'), qqpqpp );}}
finally: 0; } catch(...) {} if( qqpppqp ) { qqpppqp[(59681269 ^ 0x38EA9F5)] = qqppqpq;}
	if (exception!=0) {throw eException ;} else {EMPTY;};
	exception = temp_QUAD ;
	return qqppqpp ;} 
void qqpqqq_FastMem(qqqp* qqppppp, void* qqppppq) {signed char qqpppqp; qqppp* qqpppqq=NULL; long qqppqpp; signed char qqppqpq; qqqq* qqppqqp=NULL; qqpppqq = (qqppp*) ((long) qqppppq - (622942144 ^ 0x252157C2)); if( qqpppqq->q_qqppp == -(116387844 ^ 0x6EFF005)) { qqppppq = (void*) ((long) qqppppq - ((4995802 ^ 0x4C3AD8)  + (27321087 ^ 0x1A0E2FC) & ~ (489999756 ^ 0x1D34CD8F)) ); qqppqp_FastMem(qqppppp, qqppppq); } else { if( 426789944 ^ 0x19704C38 ) { q_FastMem2(QuadChara(0,'B','U','G'), qqpqpq );}  qqppqpq = qqpppqq-> q_qqppp; qqpppqp = qqpppqq->qq_qqppp; if( 4305156 ^ 0x41B104 ) { if( - (qqppqpq < (5070436 ^ 0x4D5E64)  )| - (qqppqpq > (6788805 ^ 0x6797C1)/(281402 ^ 0x44B3E))  ) { q_FastMem2(QuadChara(0,'B','U','G'), qqpqqp );}} if( 27991862 ^ 0x1AB1F36 ) { if( - (qqpppqp < (9058 ^ 0x2362)  )| - (qqpppqp > (803946 ^ 0xC4465))  ) { q_FastMem2(QuadChara(0,'B','U','G'), qqpqqq );}} qqppqpp = qqppqpq * (214308 ^ 0x34520); qqppqqp = (qqqq*) ((long) ((long) qqpppqq - (2732006 ^ 0x29AFE8) )- qqpppqp * qqppqpp); if( qqppqqp->qqp_qqqq == (78883012 ^ 0x4B3A8C4)) { qqppqqp->q_qqqq = (qqqq*) NULL; qqppqqp->qq_qqqq = qqppppp->qqqp_qqqp[qqppqpq]; qqppppp->qqqp_qqqp[qqppqpq] = qqppqqp; if( qqppqqp->qq_qqqq ) { qqppqqp->qq_qqqq->q_qqqq = qqppqqp;} } qqppqqp->qqp_qqqq = (short) (qqppqqp->qqp_qqqq | (4752420 ^ 0x488425) << qqpppqp); if( (qqppqqp->qqp_qqqq & (1767862572 ^ 0x695F8ED3) )== (1767862572 ^ 0x695F8ED3)) { if( (long) qqppqqp->q_qqqq == (long) (980704 ^ 0xEF6E0)) { if( 1917490104 ^ 0x724A93B8 ) { if( qqppppp->qqqp_qqqp[qqppqpq] != qqppqqp ) { q_FastMem2(QuadChara(0,'B','U','G'), qqqppp );}} qqppppp->qqqp_qqqp[qqppqpq] = qqppqqp->qq_qqqq; if( qqppqqp->qq_qqqq ) { qqppqqp->qq_qqqq->q_qqqq = (qqqq*) NULL;} } else { qqppqqp->q_qqqq->qq_qqqq = qqppqqp->qq_qqqq; if( qqppqqp->qq_qqqq ) { qqppqqp->qq_qqqq->q_qqqq = qqppqqp->q_qqqq;} } qqppqp_FastMem(qqppppp, qqppqqp); } }
	return ;}
void new_FastMem() { qqp_FastMem = (char*) "PE/FastMem; findFreeInArena(); freeBlock=NIL"; qqq_FastMem = (char*) "# freeBlock.size=%ld=$%lx, freeFI=%ld, freeSI=%ld, debugFI=%ld, debugSI=%ld\n"; qqpp_FastMem = (char*) "PE/FastMem; removeFreeFromArena(); incorrect freeFI - please use FastVerify() to find memory corruption"; qqpq_FastMem = (char*) "PE/FastMem; removeFreeFromArena(); incorrect freeSI - please use FastVerify() to find memory corruption"; qqqp_FastMem = (char*) "PE/FastMem; removeFreeFromArena(); incorrect sl"; qqqq_FastMem = (char*) "PE/FastMem; removeFreeFromArena(); freeBlock.prevFree=NIL"; qqppp_FastMem = (char*) "PE/FastMem; splitBlock(); sizeInBytes is not a multiple of 4"; qqppq = (char*) "PE/FastMem; splitBlock(); incorrect blockSize"; qqpqp = (char*) "PE/FastMem; splitBlock(); blockSize is not a multiple of 4"; qqpqq = (char*) "PE/FastMem; mergeBothBlocks(); blocks not adjacent"; qqqpp = (char*) "PE/FastMem; removePoolFromArena(); pool is not completely free"; qqqpq = (char*) "PE/FastMem; removePoolFromArena(); prevBlock<>NIL"; qqqqp = (char*) "PE/FastMem; removePoolFromArena(); pool was unexpectedly not head of list"; qqqqq = (char*) "PE/FastMem; allocFromArena(); block is not aligned"; qqpppp = (char*) "PE/FastMem; allocFromSlabArena(); slabIndex out of bounds"; qqpppq = (char*) "PE/FastMem; allocFromSlabArena(); blockIndex out of bounds"; qqppqp = (char*) "PE/FastMem; allocFromSlabArena(); slab not head (1)"; qqppqq = (char*) "PE/FastMem; allocFromSlabArena(); slab not head (2)"; qqpqpp = (char*) "PE/FastMem; allocFromSlabArena(); block is not aligned"; qqpqpq = (char*) "PE/FastMem; deallocFromSlabArena(); slabIndex<>-1"; qqpqqp = (char*) "PE/FastMem; deallocFromSlabArena(); slabIndex out of bounds"; qqpqqq = (char*) "PE/FastMem; deallocFromSlabArena(); blockIndex out of bounds"; qqqppp = (char*) "PE/FastMem; deallocFromSlabArena(); slab not head"; qqqppq = (char*) "WARNING: FastMem calls leaked %ld bytes (%ld MB) in %ld blocks.  There are %ld UNused bytes (%ld MB) still pre-allocated in %ld blocks by %ld pools.\n"; qqqpqp = (char*) "FastNew(); size<=0"; qqqpqq = (char*) "FastDispose(); size<=0"; qqqqpp = (char*) "SIZEOF_slab is too small"; qqqqpq = (char*) "SIZEOF_slabBlock is too small"; qqqqqp = (char*) "FastMem/FastVerify() FAILED due to: %s\n"; qqqqqq = (char*) "NOTE: FastMem calls allocated %ld bytes (%ld MB) in %ld blocks.  There are %ld UNused bytes (%ld MB) still pre-allocated in %ld blocks by %ld pools.\n"; qq_FastMem = NewSemaphore(); q_FastMem = qqpqpp_FastMem();
	return ;  }
void end_FastMem() {long qqppppp, qqppppq, qqpppqp, qqpppqq, qqppqpp; if( 49994422 ^ 0x2FADAB6) { qqppppp= qqqp_FastMem2(q_FastMem); qqppppq= ret2 ; qqpppqp= ret3 ; qqpppqq= ret4 ; qqppqpp = ret5 ; if( qqppppq != (218489 ^ 0x35579) ) { printf(qqqppq ,qqppppp ,qqppppp/(6769647 ^ 0x674FEF)/(6769647 ^ 0x674FEF), qqppppq, qqpppqp, qqpppqp/(6769647 ^ 0x674FEF)/(6769647 ^ 0x674FEF), qqpppqq, qqppqpp);} } qqpqpq_FastMem(q_FastMem) ; q_FastMem = (qqqp*) NULL; qq_FastMem = DisposeSemaphore(qq_FastMem);
	return ;}

void* FastNew(long qqppppp, BOOLEAN qqppppq)   {void* qqpppqp=NULL;  if( qqppppp <= (514354 ^ 0x7D932) ) { Throw(QuadChara(0,'M','E','M'), qqqpqp );} SemLock(qq_FastMem); qqpppqp = qqpqqp_FastMem(q_FastMem, qqppppp); SemUnlock(qq_FastMem); if( (long) qqpppqp == (long) (1488495812 ^ 0x58B8A4C4) ) { Raise(QuadChara(0,'M','E','M'));} if( qqppppq == (32688110 ^ 0x1F2C7EE) ) { memset(qqpppqp , 0,qqppppp );}
	return qqpppqp ;  } 
void* FastDispose(void* qqppppp, long qqppppq)   { if( qqppppp) { if( - (qqppppq <= (1152416 ^ 0x1195A0)  )& - (qqppppq != -(471566 ^ 0x731E9))  ) { q_FastMem2(QuadChara(0,'M','E','M'), qqqpqq );} SemLock(qq_FastMem); qqpqqq_FastMem(q_FastMem, qqppppp); SemUnlock(qq_FastMem); }
	return NULLA;} 
BOOLEAN FastVerify(BOOLEAN qqppppp)   {BOOLEAN qqppppq; int temp_QUAD;try {
	temp_QUAD = exception ;
	exception = 0 ; SemLock(qq_FastMem); if( (253958 ^ 0x3E00A) < sizeof( qqqq)) { if( qqppppp == (2222035 ^ 0x21E7D3) ) { Throw(QuadChara('F','A','I','L'), qqqqpp );} qqppppq = 26587407 ^ -0x195B110; } if( (1977993 ^ 0x1E2E8B) < sizeof( qqppp)) { if( qqppppp == (1727723148 ^ 0x66FAF68C) ) { Throw(QuadChara('F','A','I','L'), qqqqpq );} qqppppq = 892908868 ^ -0x3538B545; } qqppppq =  285809 ^ 0x45C71  ;} catch(...) {} SemUnlock(qq_FastMem); if( exception == QuadChara('F','A','I','L')) { exception = 460518 ^ 0x706E6; if( qqppppp == (1927095520 ^ 0x72DD24E0) ) { printf(qqqqqp ,(long) exceptionInfo );} qqppppq = 4980104 ^ -0x4BFD89; }
	if (exception!=0) {throw eException ;} else {EMPTY;};
	exception = temp_QUAD ;
	return qqppppq ;} 
long FastReport(BOOLEAN qqppppp)   {long qqppppq, qqpppqp, qqpppqq, qqppqpp, qqppqpq;int temp_QUAD;
try {
	temp_QUAD = exception ;
	exception = 0 ; SemLock(qq_FastMem); qqppppq= qqqp_FastMem2(q_FastMem); qqpppqp= ret2 ; qqpppqq= ret3 ; qqppqpp= ret4 ; qqppqpq = ret5 ; SemUnlock(qq_FastMem); if( qqppppp == (305609 ^ 0x4A9C9) ) { printf(qqqqqq ,qqppppq ,qqppppq/(6769647 ^ 0x674FEF)/(6769647 ^ 0x674FEF), qqpppqp, qqpppqq, qqpppqq/(6769647 ^ 0x674FEF)/(6769647 ^ 0x674FEF), qqppqpp, qqppqpq);}
} catch(...) {}
	ret5 = qqppqpq ;
	ret4 = qqppqpp ;
	ret3 = qqpppqq ;
	ret2 = qqpppqp ;
	if (exception!=0) {throw eException ;} else {EMPTY;};
	exception = temp_QUAD ;
	return qqppppq ;} 

/*** PE/CPP/EList ***/
/* PE/CPP/EList.e 14-10-11
   A re-implementation of AmigaE's E-list functions.
   
   By Christopher S Handley:
   27-07-06 - Started coding it for PortablE.
   30-07-06 - Completed.
   14-08-06 - Updated to use the ILIST type.
   07-01-08 - Fixed DisposeList() bug.
   18-08-09 - Rewrote to store everything in it's array, rather than using an object as a header.
   06-02-11 - Added PrintL() & StringFL() functions (limited to lists of up to 8 items).
   25-05-11 - Improved PrintL() & StringFL() to handle lists of up to 20 items.
   14-10-11 - Fixed a potential threading issue.
*/ /* Emulated procedures:
NewList(maxLen) RETURNS list:LIST
DisposeList(list:LIST) RETURNS NILL
ListCopy(list: LIST, other:ILIST, len=ALL) RETURNS list:LIST
ListAdd( list: LIST, other:ILIST, len=ALL) RETURNS list:LIST
ListCmp( list:ILIST, other:ILIST, len=ALL) RETURNS match:BOOL
ListMax( list: LIST) RETURNS max:VALUE
ListLen( list:ILIST) RETURNS len:VALUE
ListItem(list:ILIST, index) RETURNS value
SetList( list: LIST, newLen)

PrintL(fmtString:ARRAY OF CHAR, args=NILL:ILIST)
*/



const BOOLEAN INDEX_LENGTH=-1;
const signed char INDEX_SIZE=-2;

const signed char ITEM_SIZE=sizeof( long);
const signed char HEADER_SIZE=2			;//PortablE is hard-coded to use a value of 2 for static ILISTs
const signed char HEADER_OFFSET=HEADER_SIZE * ITEM_SIZE;
long* InitList(long* array, long maxLen) ;
long* NewList(long maxLen) ;
long* DisposeList(long* list);
long* ListCopy(long* list, long* other, long len=ALL);
long* ListAdd(long* list, long* other, long len=ALL);
BOOLEAN ListCmp(long* list, long* other, long len=ALL) ;
long ListMax(long* list) ;
long ListLen(long* list) ;
long ListItem(long* list, long index) ;
void SetList(long* list, long newLen);
char* StringF2(char* eString, char* fmtString, long arg1=0, long arg2=0, long arg3=0, long arg4=0, long arg5=0, long arg6=0, long arg7=0, long arg8=0, long arg9=0, long arg10=0, long arg11=0, long arg12=0, long arg13=0, long arg14=0, long arg15=0, long arg16=0, long arg17=0, long arg18=0, long arg19=0, long arg20=0, long* returnLen=NULLA);



long* InitList(long* array, long maxLen)  {
	long* list=NULL;
	list = (long*) ((long) array + HEADER_OFFSET );
	list[INDEX_LENGTH] = 0;
	list[INDEX_SIZE]   = maxLen;
	return list ;
} 

long* NewList(long maxLen)  {
	long* list=NULL;
	//use check
	if( maxLen < 0 ) { Throw(QuadChara(0,'E','P','U'), (char*) "EList; NewList(); maxLen<0");}
	
	//allocate e-list
	list = (long*) ((long) FastNew((maxLen + HEADER_SIZE )* ITEM_SIZE, TRUE) + HEADER_OFFSET );
	
	//init
	list[INDEX_LENGTH] = 0;
	list[INDEX_SIZE]   = maxLen;
	return list ;
} 

long* DisposeList(long* list) {
	if( list ) { FastDispose((long*) ((long) list - HEADER_OFFSET), -999)	;}//could use "list[INDEX_SIZE] + HEADER_SIZE * ITEM_SIZE" instead of -999
	return NULLL;
} 

long* ListCopy(long* list, long* other, long len) {
	//use check
	if(  list == NULLL ) { Throw(QuadChara(0,'E','P','U'), (char*) "EList; ListCopy(); list=NILL");}
	if( other == NULLL ) { Throw(QuadChara(0,'E','P','U'), (char*) "EList; ListCopy(); other=NILL");}
	if( - (len < 0)  & - (len != ALL)  ) { Throw(QuadChara(0,'E','P','U'), (char*) "EList; ListCopy(); len<0");}
	
	//empty e-list before appending to it
	list[INDEX_LENGTH] = 0;
	ListAdd(list, other, len);
	return list;
} 

long* ListAdd(long* list, long* other, long len) {
	long readIndex, maxReadIndex;
	long writeIndex, maxWriteIndex;
	
	//use check
	if(  list == NULLL ) { Throw(QuadChara(0,'E','P','U'), (char*) "EList; ListAdd(); list=NILL");}
	if( other == NULLL ) { Throw(QuadChara(0,'E','P','U'), (char*) "EList; ListAdd(); other=NILL");}
	if( - (len < 0)  & - (len != ALL)  ) { Throw(QuadChara(0,'E','P','U'), (char*) "EList; ListAdd(); len<0");}
	
	//calc end of list reading from & writing to
	maxReadIndex  = ( len==ALL )? other[INDEX_SIZE] : ( len<other[INDEX_SIZE] )? len : other[INDEX_SIZE];
	maxWriteIndex = list[INDEX_SIZE];
	
	//copy all characters that will fit
	readIndex  = 0;
	writeIndex = list[INDEX_LENGTH]		;//start writing past end of list
	while( - (writeIndex < maxWriteIndex)  & - (readIndex < maxReadIndex)) {
		list[writeIndex] = other[readIndex];
		
		writeIndex++;
		readIndex++;
	}
	
	//update list's stored length
	list[INDEX_LENGTH] = writeIndex;
	return list;
} 

BOOLEAN ListCmp(long* list, long* other, long len)  {
	BOOLEAN match;
	long index, maxIndex;
	
	//use check
	if(  list == NULLL ) { Throw(QuadChara(0,'E','P','U'), (char*) "EList; ListCmp(); list=NILL");}
	if( other == NULLL ) { Throw(QuadChara(0,'E','P','U'), (char*) "EList; ListCmp(); other=NILL");}
	if( - (len < 0)  & - (len != ALL)  ) { Throw(QuadChara(0,'E','P','U'), (char*) "EList; ListCmp(); len<0");}
	
	if( list[INDEX_LENGTH] != other[INDEX_LENGTH]) {
		match = FALSE;
	} else {
		//calc where should stop comparison
		maxIndex = ( list[INDEX_SIZE]<other[INDEX_SIZE] )? list[INDEX_SIZE] : other[INDEX_SIZE];
		if( len != ALL ) { maxIndex = ( len<maxIndex )? len : maxIndex;}
		
		//compare all characters
		match = TRUE;
		index = 0;
		while( index < maxIndex) {
			if( list[index] != other[index] ) { match = FALSE;}
			
			index++;
		if( match == FALSE) break;
		} 
	}
	return match ;
} 

long ListMax(long* list)  {
	long max;
	//use check
	if( list == NULLL ) { Throw(QuadChara(0,'E','P','U'), (char*) "EList; ListMax(); list=NILL");}
	
	max = list[INDEX_SIZE];
	return max ;
} 

long ListLen(long* list)  {
	long len;
	//use check
	if( list == NULLL ) { Throw(QuadChara(0,'E','P','U'), (char*) "EList; ListLen(); list=NILL");}
	
	len = list[INDEX_LENGTH];
	return len ;
} 

long ListItem(long* list, long index)  {
	long value;
	//use check
	if( list == NULLL ) { Throw(QuadChara(0,'E','P','U'), (char*) "EList; ListLen(); list=NILL");}
	
	//additional use check
	if( - (index < 0)  | - (index >= list[INDEX_LENGTH])  ) { Throw(QuadChara(0,'E','P','U'), (char*) "EList; ListLen(); index exceeds list bounds");}
	
	value = list[index];
	return value ;
} 

void SetList(long* list, long newLen) {
	//use check
	if( list == NULLL ) { Throw(QuadChara(0,'E','P','U'), (char*) "EList; SetList(); list=NILL");}
	if( newLen < 0  ) { Throw(QuadChara(0,'E','P','U'), (char*) "EList; SetList(); newLen<0");}
	
	//additional use check
	if( newLen > list[INDEX_SIZE] ) { Throw(QuadChara(0,'E','P','U'), (char*) "EList; SetList(); newLen exceeds list size");}
	
	//set length
	list[INDEX_LENGTH] = newLen;
	return ;
}


void PrintL(char* fmtString, long* args)  {
	long alen;
	alen = ( args )? ListLen(args) : (long) 0;
	if( alen > 20 ) { Throw(QuadChara(0,'E','P','U'), (char*) "PrintL(); args has too many items");}
	printf(fmtString ,( alen >= 1 )? args[0] : (long) 0 ,( alen >= 2 )? args[1] : (long) 0 ,( alen >= 3 )? args[2] : (long) 0 ,( alen >= 4 )? args[3] : (long) 0 ,( alen >= 5 )? args[4] : (long) 0 ,( alen >= 6 )? args[5] : (long) 0 ,( alen >= 7 )? args[6] : (long) 0 ,( alen >= 8 )? args[7] : (long) 0 ,( alen >= 9 )? args[8] : (long) 0 ,( alen >= 10 )? args[9] : (long) 0 ,( alen >= 11 )? args[10] : (long) 0 ,( alen >= 12 )? args[11] : (long) 0 ,( alen >= 13 )? args[12] : (long) 0 ,( alen >= 14 )? args[13] : (long) 0 ,( alen >= 15 )? args[14] : (long) 0 ,( alen >= 16 )? args[15] : (long) 0 ,( alen >= 17 )? args[16] : (long) 0 ,( alen >= 18 )? args[17] : (long) 0 ,( alen >= 19 )? args[18] : (long) 0 ,( alen >= 20 )? args[19] : (long) 0 );
	return ;
}

char* StringFL(char* eString, char* fmtString, long* args)  {
	long alen, len;
	int temp_QUAD;
try {
	temp_QUAD = exception ;
	exception = 0 ;
	alen = ( args )? ListLen(args) : (long) 0;
	if( alen > 20 ) { Throw(QuadChara(0,'E','P','U'), (char*) "StringFL(); args has too many items");}
	eString = StringF2(eString, fmtString, ( alen >= 1 )? args[0] : (long) 0, ( alen >= 2 )? args[1] : (long) 0, ( alen >= 3 )? args[2] : (long) 0, ( alen >= 4 )? args[3] : (long) 0, ( alen >= 5 )? args[4] : (long) 0, ( alen >= 6 )? args[5] : (long) 0, ( alen >= 7 )? args[6] : (long) 0, ( alen >= 8 )? args[7] : (long) 0, ( alen >= 9 )? args[8] : (long) 0, ( alen >= 10 )? args[9] : (long) 0, ( alen >= 11 )? args[10] : (long) 0, ( alen >= 12 )? args[11] : (long) 0, ( alen >= 13 )? args[12] : (long) 0, ( alen >= 14 )? args[13] : (long) 0, ( alen >= 15 )? args[14] : (long) 0, ( alen >= 16 )? args[15] : (long) 0, ( alen >= 17 )? args[16] : (long) 0, ( alen >= 18 )? args[17] : (long) 0, ( alen >= 19 )? args[18] : (long) 0, ( alen >= 20 )? args[19] : (long) 0, & len);
} catch(...) {}
	ret2 = len;
	if (exception!=0) {throw eException ;} else {EMPTY;};
	exception = temp_QUAD ;
	return eString;
} 


char* StringF2(char* eString, char* fmtString, long arg1, long arg2, long arg3, long arg4, long arg5, long arg6, long arg7, long arg8, long arg9, long arg10, long arg11, long arg12, long arg13, long arg14, long arg15, long arg16, long arg17, long arg18, long arg19, long arg20, long* returnLen) {
//REPLACEMENT
	long max, len;
	int temp_QUAD;
try {
	temp_QUAD = exception ;
	exception = 0 ;
	
	max = StrMax(eString);
	len = snprintf(eString ,max+1 ,fmtString ,arg1 ,arg2 ,arg3 ,arg4 ,arg5 ,arg6 ,arg7 ,arg8 ,arg9 ,arg10 ,arg11 ,arg12 ,arg13 ,arg14 ,arg15 ,arg16 ,arg17 ,arg18 ,arg19 ,arg20 );
	len = ( len<max )? len : max;
	SetStr(eString, len);
} catch(...) {}
	if( returnLen ) { returnLen[0] = len;}
	if (exception!=0) {throw eException ;} else {EMPTY;};
	exception = temp_QUAD ;
	return eString ;//, len
} 

/*** PE/pMultiThreaded_disabled ***/
/* alias module - The module used when NOT using OPT MULTITHREADED */ //unneeded at the moment: PUBLIC MODULE 'PE/pThreadNode_dummy'



BOOLEAN OptMultiThreaded()  {
	BOOLEAN multiThreaded; 
	multiThreaded = FALSE;
	goto finally;
finally: 0; 
	return multiThreaded ;
} 

/*** target/graphics/rpattr ***/
/* $VER: rpattr.h 39.2 (31.5.1993) */ 

#include <graphics/rpattr.h>

/*** target/graphics/display ***/
/* $VER: display.h 39.0 (21.8.1991) */ 

#include <graphics/display.h>

/*** target/graphics/collide ***/
/* $VER: collide.h 37.0 (7.1.1991) */ 

#include <graphics/collide.h>

/*** target/graphics/coerce ***/
/* $VER: coerce.h 39.3 (15.2.1993) */ 

#include <graphics/coerce.h>

/*** target/exec/strings ***/


const signed char NL=10;
      const signed char DEL=0x7F;
      const signed char BS=8;
      const signed char BELL=7;
      const signed char CR=13;
      const signed char LF=10;
      const BOOLEAN EOS=0;

/*** target/exec/initializers ***/
/* $VER: initializers.h 39.0 (15.10.1991) */ 

#include <exec/initializers.h>

/*** target/exec/errors ***/
/* $VER: errors.h 39.0 (15.10.1991) */ 

#include <exec/errors.h>

const BOOLEAN ERR_OPENDEVICE=-1	;//IOERR_OPENFAIL

/*** target/exec/alerts ***/
/* $VER: alerts.h 39.3 (12.5.1992) */ 

#include <exec/alerts.h>

/*** target/dos/dos_lib ***/


const signed char RESERVE=4;
      const signed char VSIZE=6;

/*** target/dos/doshunks ***/
/* $VER: doshunks.h 36.9 (2.6.1992) */ 

#include <dos/doshunks.h>

const signed char EXT_COMMONDEF=4;

/*** target/exec/types ***/
/* $Id: types.h,v 45.2 2001/03/12 17:51:53 heinz Exp $ */ 

#include <exec/types.h>

/*** target/hardware/dmabits ***/
/* placeholder module */ 

#include <hardware/dmabits.h>

/*** target/hardware/custom ***/
/* placeholder module */ 

#include <hardware/custom.h>

/*** target/hardware/blit ***/
/* placeholder module */ 

#include <hardware/blit.h>

/*** target/graphics/sprite ***/
/* $VER: sprite.h 39.6 (16.6.1992) */ 

#include <graphics/sprite.h>

/*** target/graphics/gels ***/
/* $VER: gels.h 39.0 (21.8.1991) */ 

#include <graphics/gels.h>

/*** target/exec/avl ***/
/* $VER: avl.h 45.4 (27.2.2001) */ 

#include <exec/avl.h>

/*** target/exec/resident ***/
/* $VER: resident.h 39.0 (15.10.1991) */ 

#include <exec/resident.h>

/*** target/exec/nodes ***/
/* $VER: nodes.h 39.0 (15.10.1991) */ 

#include <exec/nodes.h>

/*** target/utility/tagitem ***/
/* $VER: tagitem.h 40.1 (19.7.1993) */ 

#include <utility/tagitem.h>

/*** target/graphics/gfx ***/
/* $VER: gfx.h 39.5 (19.3.1992) */ 

#include <graphics/gfx.h>

/*** target/dos/datetime ***/
/* $VER: datetime.h 45.1 (17.12.2001) */ 

#include <dos/datetime.h>

/*** target/dos/dos ***/
/* $VER: dos.h 36.27 (5.4.1992) */ 

#include <dos/dos.h>
extern char* dosname;
void new_dos() ;
char* dosname=NULL;
void new_dos()  {
	dosname = (char*) "dos.library";
	return ;
}

/*** target/utility/hooks ***/
/* $VER: hooks.h 39.2 (16.6.1993) */ 

#include <utility/hooks.h>

/*** target/graphics/graphint ***/
/* $VER: graphint.h 39.0 (23.9.1991) */ 

#include <graphics/graphint.h>

/*** target/exec/libraries ***/
/* $VER: libraries.h 39.2 (10.4.1992) */ 

#include <exec/libraries.h>

const signed char LIBF_EXP0CNT=0x10;

/*** target/exec/memory ***/
/* $VER: memory.h 39.3 (21.5.1992) */ 

#include <exec/memory.h>

/*** target/dos/var ***/
/* $VER: var.h 36.11 (2.6.1992) */ 

#include <dos/var.h>

/*** target/dos/rdargs ***/
/* $VER: rdargs.h 36.6 (12.7.1990) */ 

#include <dos/rdargs.h>

/*** target/exec/lists ***/
/* $VER: lists.h 39.0 (15.10.1991) */ 

#include <exec/lists.h>

/*** target/graphics/videocontrol ***/
/* $VER: videocontrol.h 39.8 (31.5.1993) */ 

#include <graphics/videocontrol.h>

/*** target/dos/dostags ***/
/* $VER: dostags.h 36.11 (29.4.1991) */ 

#include <dos/dostags.h>

/*** target/graphics/scale ***/
/* $VER: scale.h 39.0 (21.8.1991) */ 

#include <graphics/scale.h>

/*** target/graphics/regions ***/
/* $VER: regions.h 39.0 (21.8.1991) */ 

#include <graphics/regions.h>

/*** target/dos/record ***/
/* $VER: record.h 36.5 (12.7.1990) */ 

#include <dos/record.h>

/*** target/dos/exall ***/
/* $VER: exall.h 36.6 (5.4.1992) */ 

#include <dos/exall.h>

/*** target/devices/keymap ***/
/* $VER: keymap.h 36.3 (13.4.1990) */ 

#include <devices/keymap.h>

/*** target/exec/interrupts ***/
/* $VER: interrupts.h 39.1 (18.9.1992) */ 

#include <exec/interrupts.h>

const int SF_SAR=0x8000;
const signed char SIH_QUEUES=5;
const short SF_SINT=0x2000;
const short SF_TQE=0x4000;

/*** target/dos/dosasl ***/
/* $VER: dosasl.h 36.16 (2.5.1991) */ 

#include <dos/dosasl.h>

/*** target/exec/ports ***/
/* $VER: ports.h 39.0 (15.10.1991) */ 

#include <exec/ports.h>

const signed char MP_SOFTINT=16;

/*** target/exec/devices ***/
/* $VER: devices.h 39.0 (15.10.1991) */ 

#include <exec/devices.h>

/*** target/exec/tasks ***/
/* $VER: tasks.h 39.3 (18.9.1992) */ 

#include <exec/tasks.h>
class etask;

const signed char CHILD_NOTNEW=1;
const signed char CHILD_NOTFOUND=2;
const signed char CHILD_EXITED=3;
const signed char CHILD_ACTIVE=4;

const int SYS_TRAPALLOC=0x8000;
const int SYS_SIGALLOC=0xFFFF;

//no such object!
class etask: public object {
public:
	struct Message mn;
	struct Task* parent;
	long uniqueid;
	struct MinList children;
	short trapalloc;
	short trapable;
	long result1;
	long result2;
	struct MsgPort taskmsgport;
};

/*** target/exec/io ***/
/* $VER: io.h 39.0 (15.10.1991) */ 

#include <exec/io.h>

/*** target/exec/execbase ***/
/* $VER: execbase.h 39.6 (18.1.1993) */ 

#include <exec/execbase.h>

/*** target/dos/notify ***/
/* $VER: notify.h 36.8 (29.8.1990) */ 

#include <dos/notify.h>

/*** target/exec/semaphores ***/
/* $VER: semaphores.h 39.1 (7.2.1992) */ 

#include <exec/semaphores.h>

const signed char SM_LOCKMSG=16;

/*** target/devices/timer ***/
/* $VER: timer.h 36.16 (25.1.1991) */ 

#include <devices/timer.h>
extern char* timername;
void new_timer() ;
char* timername=NULL;
void new_timer()  {
	timername = (char*) "timer.device";
	return ;
}

/*** target/exec ***/
/* C++ module, for $VER: exec_protos.h 45.2 (6.6.1998) */ 


#include <proto/exec.h>


struct ExecBase* SysBase = NULL;


/*** target/intuition/preferences ***/
/* $VER: preferences.h 38.2 (16.9.1992) */ 

#include <intuition/preferences.h>

/*** target/dos/dosextens ***/
/* $VER: dosextens.h 36.41 (14.5.1992) */ 

#include <dos/dosextens.h>

/*** target/graphics/gfxnodes ***/
/* $VER: gfxnodes.h 39.0 (21.8.1991) */ 

#include <graphics/gfxnodes.h>

/*** target/graphics/copper ***/
/* $VER: copper.h 39.10 (31.5.1993) */ 

#include <graphics/copper.h>

/*** target/graphics/text ***/
/* $VER: text.h 39.0 (21.8.1991) */ 

#include <graphics/text.h>

/*** target/graphics/layers ***/
/* $VER: layers.h 39.4 (14.4.1992) */ 

#include <graphics/layers.h>

/*** target/graphics/rastport ***/
/* $VER: rastport.h 39.0 (21.8.1991) */ 

#include <graphics/rastport.h>

/*** target/graphics/clip ***/
/* $VER: clip.h 39.0 (2.12.1991) */ 

#include <graphics/clip.h>

/*** targetShared/CPP/Amiga/pStack ***/




long FreeStack() ;
long StackSize() ;

long FreeStack()  {
	long bytes;
	struct Task* task=NULL; long size;
	task = FindTask((char*) NULLA );
	size = (long) ((char*) (void*) task->tc_SPUpper - (long) task->tc_SPLower);
	
	bytes = (long) ((char*) (void* ) & bytes - (long) task->tc_SPLower);
	if( - (bytes < 0)  | - (bytes > size)) {
		bytes = (long) ((char*) (void*) task->tc_SPReg - (long) task->tc_SPLower);
		if( - (bytes < 0)  | - (bytes > size)) {
			bytes = size;
		}
	}
	return bytes ;
} 

long StackSize()  {
	long bytes;
	struct Task* task=NULL;
	task = FindTask((char*) NULLA );
	bytes = (long) ((char*) (void*) task->tc_SPUpper - (long) task->tc_SPLower);
	return bytes ;
} 

/*** targetShared/Amiga/exec ***/




void* NewM(long size, long flags);

void* NewM(long size, long flags) {
	void* mem=NULL;
	
	if( flags & (MEMF_CHIP | MEMF_FAST | MEMF_PUBLIC | MEMF_LOCAL | MEMF_24BITDMA)) {
		printf("NewM() emulation was passed an unsupported flag\n" ,0 );
		Raise(QuadChara(0,'M','E','M'));
	}
	
	mem = New(size);
	if( mem == NULL ) { Raise(QuadChara(0,'M','E','M'));}
	return mem;
} 

/*** PE/Amiga/Mem ***/
/* AmigaOS implementation needed by PE/Mem */ 
class memSemaphore;



class memSemaphore: public object {
public:
	struct SignalSemaphore ss;
};

void* baseNew(long size, BOOLEAN noClear)  {
	void* mem=NULL; 
	
		mem = (void*) AllocVec((ULONG) size ,(ULONG) (MEMF_PUBLIC | (( noClear )? (int) 0 : MEMF_CLEAR)) );
	return mem ;
	
} 

void baseDispose(void* mem)  {
	FreeVec((APTR) (unsigned long) mem );
	return ;
}


void* baseNewSemaphore()  {
	void* sem=NULL; 
	sem = baseNew(sizeof( memSemaphore));
	if( sem == NULL ) { Throw(QuadChara(0,'M','E','M'), (char*) "baseNewSemaphore(); allocation failed");}
	InitSemaphore(& ((memSemaphore*) sem)->ss );
	return sem ;
} 

void* baseDisposeSemaphore(void* sem)  {
	void* nil=NULL; 
	baseDispose(sem);
	nil = NULL;
	return nil ;
} 

void baseSemLock(void* sem)  {
	ObtainSemaphore(& ((memSemaphore*) sem)->ss );
	return ;
}

void baseSemUnlock(void* sem)  {
	ReleaseSemaphore(& ((memSemaphore*) sem)->ss );
	return ;
}

/*** target/dos ***/
/* C++ module, for $VER: dos_protos.h 40.2 (6.6.1998) */ 


#include <proto/dos.h>

void new_dos2();
void end_dos();

struct DosLibrary* DOSBase = NULL;


//automatic opening of dos library
void new_dos2() {
	DOSBase = (struct DosLibrary*) (unsigned long) OpenLibrary("dos.library" ,(ULONG) 39 );
	if( (void*) DOSBase==NULL ) { CleanUp(RETURN_ERROR);}
	return ;
}

//automatic closing of dos library
void end_dos() {
	CloseLibrary((struct Library*) DOSBase );
	return ;
}

/*** target/graphics/monitor ***/
/* $VER: monitor.h 39.7 (9.6.1992) */ 

#include <graphics/monitor.h>
extern char* default_monitor_name;
extern char* ntsc_monitor_name;
extern char* pal_monitor_name;
extern char* vga_monitor_name;
extern char* vga70_monitor_name;
void new_monitor() ;
char* default_monitor_name=NULL;
char* ntsc_monitor_name=NULL;
char* pal_monitor_name=NULL;
char* vga_monitor_name=NULL;
char* vga70_monitor_name=NULL;
void new_monitor()  {
	default_monitor_name = (char*) "default.monitor";
	ntsc_monitor_name = (char*) "ntsc.monitor";
	pal_monitor_name = (char*) "pal.monitor";
	vga_monitor_name = (char*) "vga.monitor";
	vga70_monitor_name = (char*) "vga70.monitor";
	return ;
}

/*** targetShared/CPP/Amiga/dos ***/






extern char* arg;


extern char* argString;
void new_dos3();
void end_dos2();
struct Process* CreateNewProc_dos2(struct TagItem* tags) ;

char* arg=NULL;


char* argString=NULLS;


void new_dos3() {
	char* args=NULL; long len;
	
	if( args = GetArgStr()) {
		len = strlen(args );
		argString= NewString(len);
		
		StrCopy(argString, args);
		if( len > 0) {
			if( argString[len-1] == '\n' ) { SetStr(argString, len-1);}
		}
	}
	
	arg = argString;
	return ;
}

void end_dos2() {
	argString= DisposeString(argString);
	return ;
}

struct Process* CreateNewProc_dos2(struct TagItem* tags)  {
	struct Process* proc=NULL; 
	struct TagItem temp_ARRAY_OF_tagitem[1 ];
	temp_ARRAY_OF_tagitem [0].ti_Tag = (Tag) (( tags )? TAG_MORE : TAG_END);
	temp_ARRAY_OF_tagitem [0].ti_Data = (ULONG) (unsigned long) tags;
	proc = CreateNewProc(temp_ARRAY_OF_tagitem  );
	goto finally;
finally: 0; 
	return proc ;
} 

/*** target/graphics/displayinfo ***/
/* $VER: displayinfo.h 39.13 (31.5.1993) */ 

#include <graphics/displayinfo.h>

/*** target/graphics/modeid ***/
/* $VER: modeid.h 39.9 (27.5.1993) */ 

#include <graphics/modeid.h>

/*** target/graphics/view ***/
/* $VER: view.h 39.34 (31.5.1993) */ 

#include <graphics/view.h>

/*** target/graphics/gfxbase ***/
/* $VER: gfxbase.h 39.21 (21.4.1993) */ 

#include <graphics/gfxbase.h>
extern char* graphicsname;
void new_gfxbase() ;
char* graphicsname=NULL;
void new_gfxbase()  {
	graphicsname = (char*) "graphics.library";
	return ;
}

/*** target/graphics ***/
/* $VER: graphics_protos.h 40.2 (6.6.1998) */ 


#include <proto/graphics.h>

void new_graphics();
void end_graphics();
long BltBitMap_graphics( struct BitMap* srcBitMap, long xSrc, long ySrc, struct BitMap* destBitMap, long xDest, long yDest, long xSize, long ySize, ULONG minterm, ULONG mask, PLANEPTR tempA ) ;

struct GfxBase* GfxBase = NULL;


//automatic opening of gfx library
void new_graphics() {
	GfxBase = (struct GfxBase*) (unsigned long) OpenLibrary("graphics.library" ,(ULONG) 39 );
	if( (void*) GfxBase==NULL ) { CleanUp(RETURN_ERROR);}
	return ;
}

//automatic closing of gfx library
void end_graphics() {
	CloseLibrary((struct Library*) GfxBase );
	return ;
}
//Could not get to compile: PROC BltBitMap( srcBitMap:PTR TO bitmap, xSrc:VALUE, ySrc:VALUE, destBitMap:PTR TO bitmap, xDest:VALUE, yDest:VALUE, xSize:VALUE, ySize:VALUE, minterm:ULONG, mask:ULONG, tempA:PLANEPTR ) IS NATIVE {BltBitMap(} srcBitMap {,} xSrc {,} ySrc {,} destBitMap {,} xDest {,} yDest {,} xSize {,} ySize {,} minterm {,} mask {,} tempA {)} ENDNATIVE !!VALUE
long BltBitMap_graphics( struct BitMap* srcBitMap, long xSrc, long ySrc, struct BitMap* destBitMap, long xDest, long yDest, long xSize, long ySize, ULONG minterm, ULONG mask, PLANEPTR tempA )  {
	long planecnt;
	struct RastPort destRP;
	long temp_VALUE, temp_VALUE2;
	InitRastPort(& destRP );
	destRP.BitMap = destBitMap;
	BltBitMapRastPort(srcBitMap ,xSrc ,ySrc ,& destRP ,xDest ,yDest ,xSize ,ySize ,minterm );
	temp_VALUE = (long) GetBitMapAttr(srcBitMap ,(ULONG) BMA_DEPTH );
	temp_VALUE2 = (long) GetBitMapAttr(destBitMap ,(ULONG) BMA_DEPTH );
	planecnt = ( temp_VALUE <temp_VALUE2 )? temp_VALUE : temp_VALUE2 ;
	mask = (ULONG) 0 ; tempA = (PLANEPTR) (unsigned long) NULLA	;//dummy
	return planecnt ;
} 

/*** target/intuition/screens ***/
/* $VER: screens.h 38.25 (15.2.1993) */ 

#include <intuition/screens.h>

/*** targetShared/CPP/Amiga/graphics ***/





extern struct RastPort* stdrast;
void Plot(long x, long y, long colour=1);
void Line(long x1, long y1, long x2, long y2, long colour=1);
void Box(long x1, long y1, long x2, long y2, long colour=1);
void Colour(long foreground, long background=0);
long TextF(long x, long y, char* fmtString, long arg1=0, long arg2=0, long arg3=0, long arg4=0, long arg5=0, long arg6=0, long arg7=0, long arg8=0);
struct RastPort* SetStdRast(struct RastPort* rast);
void SetTopaz(short size=8);

struct RastPort* stdrast=NULL;

void Plot(long x, long y, long colour) {
	if( stdrast) {
		SetAPen(stdrast ,(ULONG) colour );
		WritePixel(stdrast ,x ,y );
	}
	return ;
}

void Line(long x1, long y1, long x2, long y2, long colour) {
	if( stdrast) {
		SetAPen(stdrast ,(ULONG) colour );
		Move(stdrast ,(short) x1  ,(short) y1  );
		Draw(stdrast ,x2 ,y2 );
	}
	return ;
}

void Box(long x1, long y1, long x2, long y2, long colour) {
	long xmin, ymin, xmax, ymax;
	if( stdrast) {
		SetAPen(stdrast ,(ULONG) colour );
		xmin = ( x1<x2 )? x1 : x2;
		xmax = ( x1>x2 )? x1 : x2;
		ymin = ( y1<y2 )? y1 : y2;
		ymax = ( y1>y2 )? y1 : y2;
		RectFill(stdrast ,xmin ,ymin ,xmax ,ymax );
	}
	return ;
}

void Colour(long foreground, long background) {
	if( stdrast) {
		SetAPen(stdrast ,(ULONG) foreground );
		SetBPen(stdrast ,(ULONG) background );
	}
	return ;
}

long TextF(long x, long y, char* fmtString, long arg1, long arg2, long arg3, long arg4, long arg5, long arg6, long arg7, long arg8) {
	long length;
	char* string=NULL;
	
	if( stdrast) {
		string= NewString(strlen(fmtString )*2 + 100 );
		StringF(string, fmtString, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);
		
		length = EstrLen(string);
		Move(stdrast ,(short) x  ,(short) y  );
		Text(stdrast ,string ,(ULONG) length );
		
		string= DisposeString(string);
	}
	return length;
} 

struct RastPort* SetStdRast(struct RastPort* rast) {
	struct RastPort* oldstdrast=NULL;
	oldstdrast = stdrast;
	stdrast = rast;
	return oldstdrast;
} 

void SetTopaz(short size) {
	struct TextFont* font=NULL;
	struct TextAttr temp_ARRAY_OF_textattr[1 ];
	temp_ARRAY_OF_textattr [0].ta_Name = (char*) "topaz.font";
	temp_ARRAY_OF_textattr [0].ta_Style = (UBYTE) FS_NORMAL;
	temp_ARRAY_OF_textattr [0].ta_Flags = (UBYTE) (FPF_PROPORTIONAL | FPF_DESIGNED);
	
	if( stdrast) {
		temp_ARRAY_OF_textattr [0].ta_YSize = (UWORD) size;
		font = OpenFont((struct TextAttr*) temp_ARRAY_OF_textattr  );
		SetFont(stdrast ,font );
		//CloseFont(font)
	}
	return ;
}

/*** target/graphics/gfxmacros ***/
/* $VER: gfxmacros.h 39.3 (31.5.1993) */ 

#include <graphics/gfxmacros.h>

/*** target/devices/inputevent ***/
/* $VER: inputevent.h 36.10 (26.6.1992) */ 

#include <devices/inputevent.h>

/*** target/intuition/intuition ***/
/* $VER: intuition.h 38.26 (15.2.1993) */ 

#include <intuition/intuition.h>

/*** :/root/g/Sake/Sake ***/
// Sake - Atari Kernel Emulator
// GEMDOS (TRAP #1), BIOS (TRAP #2), XBIOS (TRAP #3), GEM AES/VDI
// Maps Atari ST system calls to AmigaOS libraries




// ---------------------------------------------------------------------------
// Global state
// ---------------------------------------------------------------------------


// GEMDOS constants for Seek mode mapping
const BOOLEAN GEMDOS_SEEK_START=0; const signed char GEMDOS_SEEK_CUR=1; const signed char GEMDOS_SEEK_END=2;

// Error code mapping
const BOOLEAN E_OK=0; const BOOLEAN E_ERROR=-1; const signed char EDRVNR=-3; const signed char EPRCFND=-7;
const signed char ENFHND=-8; const signed char ELOCKD=-10; const signed char ENSMEM=-11; const signed char EIHND=-12;


// ---------------------------------------------------------------------------
// GEM AES (Application Environment Services) dispatch
// Called via BIOS trap #2 with D0 = $C8
// Atari ST AES uses a parameter block in memory (control, global, int-in/out)
// A0 = pointer to AES parameter block (intin, ptin, intout, ptout, contrl, global)
// D0 = AES function code
// On AmigaOS, we map GEM windows/events to Intuition/gadtools
// ---------------------------------------------------------------------------

// AES function group codes
// ---------------------------------------------------------------------------
// GEM AES (Application Environment Services) dispatch and implementation
// Maps Atari GEM AES calls to AmigaOS Intuition
// ---------------------------------------------------------------------------

const BOOLEAN AES_APPL=0; const signed char AES_EVNT=1; const signed char AES_MENU=3; const signed char AES_OBJC=4;
const signed char AES_FORM=5; const signed char AES_SCRP=6; const signed char AES_FSEL=7; const signed char AES_WIND=8;
const signed char AES_GRAF=9;

// Window kinds (wind_create parameter)
const signed char WIN_NAME=1; const signed char WIN_CLOSER=2; const signed char WIN_FULLER=4; const signed char WIN_MOVER=8;
const signed char WIN_SIZER=16; const signed char WIN_UPARROW=32; const signed char WIN_DNARROW=64;
const short WIN_VSLIDE=128; const short WIN_LFARROW=256; const short WIN_RTARROW=512;
const short WIN_HSLIDE=1024; const short WIN_SMALLER=2048; const short WIN_INFO=4096;

// Window states
const BOOLEAN WS_CLOSED=0; const signed char WS_OPEN=1; const signed char WS_ICONIFIED=2;

// Window messages (for message queue)
const signed char WM_REDRAW=10; const signed char WM_TOPPED=11; const signed char WM_CLOSED=12;
const signed char WM_FULLED=13; const signed char WM_ARROWED=14; const signed char WM_HSLID=15;
const signed char WM_VSLID=16; const signed char WM_SIZED=17; const signed char WM_MOVED=18;
const signed char WM_UNTOPPED=19; const signed char WM_ONTOP=20;

// Max tracked resources
const signed char MAX_WINDOWS=16; const signed char MAX_MENUS=8; const short MAX_OBJECTS=256;
const signed char MAX_MESSAGES=64; const signed char MAX_APPS=8;


// ============================
// GRAF - Graphics services
// ============================

// Mouse shape constants
const BOOLEAN M_ON=0; const signed char M_OFF=1; const signed char M_ARROW=2; const signed char M_BUSY=3; const signed char M_IBEAM=4; const signed char M_POINT=5;
const signed char M_USERDEF=6; const signed char M_SPECIAL=7;

// VDI attribute constants
const signed char VDI_REPLACE=1; const signed char VDI_TRANSPARENT=2; const signed char VDI_XOR=3; const signed char VDI_REVERSE=4;

extern APTR oldTrapCode;
extern long ctx[32];    // Saved register context + extended params
extern long gem_handles[16]; // GEMDOS handle -> AmigaOS BPTR mapping
extern long gem_dta;                  // Disk Transfer Address (for Fsfirst/Fsnext)
extern long gem_drv;                  // Current drive (0=A:)
extern char gem_path[128]; // Current path
extern struct FileInfoBlock fib;       // FileInfoBlock for directory search
extern long gem_search_lock;          // Lock handle for current search
extern long gem_search_first;         // TRUE if we're inside a search
extern char gem_search_pattern[128]; // Pattern for search matching
extern long gem_search_attr;          // Attributes for search matching
extern char temp_string[256];
extern long bios_kb_shift;            // Keyboard shift state for BIOS Kbshift
extern long gem_aes_id;               // AES application ID counter
extern long gem_window_list[16]; // Open window handles (Intuition Window ptrs)
extern long gem_aes_global[32]; // AES global array
extern long gem_scrn_w; extern long gem_scrn_h;   // Virtual screen dimensions
extern long gadtools_base;            // gadtools.library base (opened at runtime)

// Window info structure
extern long gem_wind_kind[MAX_WINDOWS];
extern long gem_wind_state[MAX_WINDOWS];
extern long gem_wind_x[MAX_WINDOWS];
extern long gem_wind_y[MAX_WINDOWS];
extern long gem_wind_w[MAX_WINDOWS];
extern long gem_wind_h[MAX_WINDOWS];
extern long gem_wind_work_x[MAX_WINDOWS];
extern long gem_wind_work_y[MAX_WINDOWS];
extern long gem_wind_work_w[MAX_WINDOWS];
extern long gem_wind_work_h[MAX_WINDOWS];
extern long gem_wind_full_x[MAX_WINDOWS];
extern long gem_wind_full_y[MAX_WINDOWS];
extern long gem_wind_full_w[MAX_WINDOWS];
extern long gem_wind_full_h[MAX_WINDOWS];
extern long gem_wind_parent[MAX_WINDOWS];
extern long gem_wind_handle[MAX_WINDOWS];

// Message queue for inter-application communication
extern long gem_msg_queue[MAX_MESSAGES];  // encoded messages (16-word packets)
extern long gem_msg_src[MAX_MESSAGES];    // source app ID
extern long gem_msg_len[MAX_MESSAGES];    // message length
extern long gem_msg_head; extern long gem_msg_tail;

// Application tracking
extern long gem_app_list[MAX_APPS]; // app IDs
extern long gem_app_count;

// Menu tracking
extern long gem_menu_tree[8]; // menu tree pointers
extern long gem_menu_owner[8]; // owning app ID
extern long gem_menu_count;
extern long gem_menu_bar_visible; // 0=none, 1=shown
extern long gem_menu_active_app; // app ID whose menu bar is showing
// Per-menu item state (max 64 items per menu tree, max 8 menus)
extern long gem_menu_item_count[8]; // item count per tree
extern long gem_menu_item_checked[8]; // bitmask of checked items
extern long gem_menu_item_enabled[8]; // bitmask of enabled items
extern long gem_menu_item_normal[8]; // bitmask of normal (non-inverted) items
extern long gem_menu_item_text[8]; // pointer array to text strings (simulated)
extern char gem_menu_item_text_buf[512]; // text storage buffer
extern long gem_menu_pending_action; // 0=none, 1=item_selected
extern long gem_menu_pending_item; // selected item index
extern long gem_menu_pending_tree; // selected menu tree
extern long gem_menu_pending_app; // app to notify

// Object tree tracking
extern long gem_obj_tree[8]; // object tree pointers

// Form state
extern long gem_form_active; // handle of active form dialog (-1 = none)

// Radio button gadget tracking
extern long gem_radio_gad[32]; // gadtools gadget pointers (up to 32 radio buttons)
extern long gem_radio_obj[32]; // GEM object index for each gadget
extern long gem_radio_count; // number of active radio gadgets
extern long gem_radio_tree; // tree pointer for current radio group
extern long gem_radio_win; // window gadgets are attached to

// Clipboard state
extern char gem_scrap_buffer[1024];
extern long gem_scrap_len;

// Graf (graphics) state
extern long gem_mouse_x; extern long gem_mouse_y;           // Current mouse position
extern long gem_mouse_buttons;                   // Button state (bit0=left, bit1=right)
extern long gem_mouse_kstate;                    // Keyboard shift state
extern long gem_mouse_shape;                     // Current cursor shape (M_ON..M_POINT)
extern long gem_mouse_visible;                   // 0=hidden, 1=visible
extern long gem_graf_wk_handle;                  // Workstation handle
extern long gem_graf_char_w; extern long gem_graf_char_h;    // Character cell size
extern long gem_graf_arrow_mode;                 // 0=menu nav, 1=normal arrows
extern long gem_graf_slidex[8];   // Slider positions
extern long gem_graf_slidey[8];
extern long gem_graf_accel_key;                  // Last accelerator key
// Mouse pointer data store (33 words: 16 mask + 16 data + 1 resolution/hotspot)
extern long gem_mouse_user_data[33];
extern long gem_mouse_user_hotx; extern long gem_mouse_user_hoty;
extern long gem_mouse_user_active;
// Predefined pointer shape bitmaps (16x16, 16 data words per shape)
extern long gem_mouse_arrow_data[16];
extern long gem_mouse_arrow_mask[16];
extern long gem_mouse_busy_data[16];
extern long gem_mouse_busy_mask[16];
extern long gem_mouse_ibeam_data[16];
extern long gem_mouse_ibeam_mask[16];
extern long gem_mouse_point_data[16];
extern long gem_mouse_point_mask[16];

// AES global arrays (as per GEM AES parameter block spec)
extern long gem_control[12]; // control array
extern long gem_intin[128];
extern long gem_intout[128];
extern long gem_ptrin[16];
extern long gem_pttout[16];


// ============================
// VDI - Virtual Device Interface
// ============================

extern long vdi_handle; // current workstation handle (-1 = none)
extern long vdi_work_w; extern long vdi_work_h; // workstation pixel dimensions
extern long vdi_dev_w; extern long vdi_dev_h; // device pixel dimensions
extern long vdi_n_planes; // bits per pixel
extern long vdi_line_type; extern long vdi_line_width; extern long vdi_line_color;
extern long vdi_fill_type; extern long vdi_fill_index; extern long vdi_fill_color;
extern long vdi_marker_type; extern long vdi_marker_height; extern long vdi_marker_color;
extern long vdi_text_font; extern long vdi_text_color; extern long vdi_text_rotation;
extern long vdi_wr_mode; // writing mode (1=replace, 2=transparent, 3=XOR, 4=reverseTransparent)
extern long vdi_clip_x; extern long vdi_clip_y; extern long vdi_clip_w; extern long vdi_clip_h;
extern long vdi_cur_x; extern long vdi_cur_y; // graphics cursor position

// VDI colour lookup (Atari ST standard 16-colour palette indexed by colour index)
extern char vdi_rgb[48]; // 16 colours x 3 bytes (R,G,B)

// VDI line type patterns (dash/dot definitions)
extern long vdi_line_pats[6];

// VDI pen colour to Amiga pen mapping (first 16 standard VDI colours)
extern long vdi_pen_map[16];
void vdi_init_pen_map();
void gem_SetVDIAttrs_RP(struct RastPort *rp);
long gem_GetVDIRastPort();


// ---------------------------------------------------------------------------
// Atari ST System Font Registration
// Registers the Atari ST character set as AmigaOS fonts via AddFont()
// 8x16 font for high-resolution (640x400), 8x8 font for low/medium
// Uses the Atari ST character encoding:
//   0x00-0x1F: special graphic chars (arrows, icons, music notes)
//   0x20-0x7E: standard ASCII (matching ISO-8859-1 in printable range)
//   0x7F:     solid block / alternate DEL
//   0x80-0xFF: extended chars (accented Latin, Greek, math, Hebrew)
// ---------------------------------------------------------------------------

// Font registration status
extern long gem_font_8x16_registered; extern long gem_font_8x8_registered;

// Atari ST 8x16 font bitmap data (256 chars × 16 bytes = 4096 bytes)
// Generated from Atari ST ROM glyph definitions
extern char gem_font_data_8x16[4096];

// Atari ST 8x8 font bitmap data (256 chars × 8 bytes = 2048 bytes)
extern char gem_font_data_8x8[2048];

// Character width tables (all 8 for fixed-width)
extern char gem_font_width_8x16[256];
extern char gem_font_width_8x8[256];

// Character location tables (WORD offsets into bitmap data)
extern char gem_font_loc_8x16[512];
extern char gem_font_loc_8x8[256];

// TextFont structures allocated on the heap
extern struct TextFont* gem_font_8x16;
extern struct TextFont* gem_font_8x8;
long gemdos_fopen_mode(long gem_mode);
long gemdos_to_amiga_seek(long gem_seek);
void gemdos_dispatch();
void gemdos_pterm0();
void gemdos_cconin();
void gemdos_cconout();
void gemdos_cconis();
void gemdos_cconos();
void gemdos_cconws();
void gemdos_cconrs();
void gemdos_dsetdrv();
void gemdos_dgetdrv();
void gemdos_dsetpath();
void gemdos_dgetpath();
void gemdos_fopen();
void gemdos_fclose();
void gemdos_fread();
void gemdos_fwrite();
void gemdos_fdelete();
void gemdos_fseek();
void gemdos_fattrib();
void gemdos_fdatime();
void gemdos_fsfirst();
void gemdos_fsnext();
long FileMatch(char* name, char* pattern);
void FillDTA(char* dta, struct FileInfoBlock* fib_ptr);
void gemdos_fsrename();
void gemdos_fmkdir();
void gemdos_frmdir();
void gemdos_fchdir();
void gemdos_fgetdta();
void gemdos_fsetdta();
void gemdos_malloc();
void gemdos_mfree();
void gemdos_mxalloc();
long load_prg(char* filename);
void gemdos_pexec();
void gemdos_pterm();
void gemdos_super();
void gemdos_tgetdate();
void gemdos_tsetdate();
void gemdos_tgettime();
void gemdos_tsettime();
void bios_dispatch();
void bios_getmpb();
void bios_gembp();
void bios_bconin();
void bios_bconout();
void bios_rwabs();
void bios_setexc();
void bios_tickcal();
void bios_bconstat();
void bios_mediac();
void bios_drvmap();
void bios_kbshift();
void bios_random();
void xbios_dispatch();
void xbios_initmouse();
void xbios_gettime();
void xbios_settime();
void xbios_bioskeys();
void xbios_kbrate();
void xbios_prtblk();
BOOLEAN gem_init_gadtools();
void gem_wind_alloc();
long gem_wind_find_handle(long h);
void gem_msg_push(long msg, long src, long len);
long gem_bit(long n);
void gem_msg_pop();
void gem_aes_dispatch();
void gem_appl_init();
void gem_appl_exit();
void gem_appl_read();
void gem_appl_write();
void gem_appl_find();
void gem_evnt_multi();
void gem_evnt_mesag();
void gem_evnt_button();
void gem_evnt_mouse();
void gem_evnt_keybd();
void gem_evnt_dclick();
void gem_evnt_timer();
void gem_menu_bar();
void gem_menu_icheck();
void gem_menu_ienable();
void gem_menu_tnormal();
void gem_menu_text();
void gem_menu_register();
void gem_menu_popup();
void gem_menu_attach();
void gem_menu_istart();
void gem_menu_settings();
void gem_objc_add();
void gem_objc_delete();
void gem_objc_draw();
void gem_objc_find();
void gem_objc_offset();
void gem_objc_order();
void gem_objc_edit();
void gem_objc_change();
void gem_objc_type();
void gem_form_do();
void gem_form_dial();
void gem_form_alert();
void gem_form_error();
void gem_form_center();
void gem_form_keybd();
void gem_form_button();
void gem_scrp_read();
void gem_scrp_write();
void gem_fsel_exinput();
void gem_fsel_exoutput();
void gem_wind_create();
void gem_wind_open();
void gem_wind_close();
void gem_wind_delete();
void gem_wind_get();
void gem_wind_set();
void gem_wind_find();
void gem_wind_update();
void gem_wind_calc();
void gem_wind_new();
void gem_wind_arrow();
void gem_wind_show();
void gem_wind_toolbar();
void gem_wind_sized();
void gem_mouse_init();
void gem_graf_rubberbox();
void gem_graf_dragbox();
void gem_graf_movebox();
void gem_graf_growbox();
void gem_graf_shrinkbox();
void gem_graf_watchbox();
void gem_graf_slidebox();
void gem_graf_handle();
void gem_graf_mkstate();
void gem_graf_mouse();
void gem_graf_arrow();
void gem_graf_set_screen();
void gem_graf_set_handle();
void gem_graf_accel();
void vdi_init_rgb();
void vdi_init_line_pats();
void vdi_opnvwk();
void vdi_clsvwk();
void vdi_clrwk();
void vdi_updwk();
void vdi_pline();
void vdi_pmarker();
void vdi_gtext();
void vdi_fillarea();
void vdi_bar();
void vdi_circle();
void vdi_ellipse();
void vdi_ellarc();
void vdi_ellpie();
void vdi_arc();
void vdi_pieslice();
void vdi_rbox();
void vdi_rfbox();
void vdi_justified();
void vdi_cellarray();
void vdi_bezier();
void vdi_qcolor();
void vdi_qcurpos();
void vdi_qcontxt();
void vdi_qextnd();
void vdi_qcellht();
void vdi_qcellwd();
void vdi_qchcells();
void vdi_qvgd();
void vdi_qkey_s();
void vdi_opnwk();
void vdi_set_line_type();
void vdi_set_line_width();
void vdi_set_line_color();
void vdi_set_fill_type();
void vdi_set_fill_index();
void vdi_set_fill_color();
void vdi_set_marker_type();
void vdi_set_marker_height();
void vdi_set_marker_color();
void vdi_set_text_font();
void vdi_set_text_color();
void vdi_set_text_rotation();
void vdi_set_writing_mode();
void vdi_set_clip_rect();
void vdi_set_clip_state();
void vdi_set_curpos();
void vdi_inq_fill_pats();
void gem_vdi_dispatch();
void gem_init_font_8x16();
void gem_init_font_8x8();
void gem_init_fonts();
int main(int argc, char** argv);
void xbios_cursconf();
void xbios_appl_init();
void xbios_physbase();
void xbios_logbase();
void xbios_getrez();
void xbios_setscreen();
void xbios_setpalette();
void xbios_setcolor();
void xbios_floprd();
void xbios_flopwr();
void xbios_flopfmt();
void xbios_flopstatus();
void xbios_rsconf();
void xbios_keytbl();
void xbios_random();
void xbios_cookieptr();

APTR oldTrapCode=(APTR) NULL;
long ctx[32];    // Saved register context + extended params
long gem_handles[16]; // GEMDOS handle -> AmigaOS BPTR mapping
long gem_dta;                  // Disk Transfer Address (for Fsfirst/Fsnext)
long gem_drv;                  // Current drive (0=A:)
char gem_path[128]; // Current path
struct FileInfoBlock fib;       // FileInfoBlock for directory search
long gem_search_lock;          // Lock handle for current search
long gem_search_first;         // TRUE if we're inside a search
char gem_search_pattern[128]; // Pattern for search matching
long gem_search_attr;          // Attributes for search matching
char temp_string[256];
long bios_kb_shift;            // Keyboard shift state for BIOS Kbshift
long gem_aes_id;               // AES application ID counter
long gem_window_list[16]; // Open window handles (Intuition Window ptrs)
long gem_aes_global[32]; // AES global array
long gem_scrn_w; long gem_scrn_h;   // Virtual screen dimensions
long gadtools_base;            // gadtools.library base (opened at runtime)

// Window info structure
long gem_wind_kind[MAX_WINDOWS];
long gem_wind_state[MAX_WINDOWS];
long gem_wind_x[MAX_WINDOWS];
long gem_wind_y[MAX_WINDOWS];
long gem_wind_w[MAX_WINDOWS];
long gem_wind_h[MAX_WINDOWS];
long gem_wind_work_x[MAX_WINDOWS];
long gem_wind_work_y[MAX_WINDOWS];
long gem_wind_work_w[MAX_WINDOWS];
long gem_wind_work_h[MAX_WINDOWS];
long gem_wind_full_x[MAX_WINDOWS];
long gem_wind_full_y[MAX_WINDOWS];
long gem_wind_full_w[MAX_WINDOWS];
long gem_wind_full_h[MAX_WINDOWS];
long gem_wind_parent[MAX_WINDOWS];
long gem_wind_handle[MAX_WINDOWS];

// Message queue for inter-application communication
long gem_msg_queue[MAX_MESSAGES];  // encoded messages (16-word packets)
long gem_msg_src[MAX_MESSAGES];    // source app ID
long gem_msg_len[MAX_MESSAGES];    // message length
long gem_msg_head; long gem_msg_tail;

// Application tracking
long gem_app_list[MAX_APPS]; // app IDs
long gem_app_count;

// Menu tracking
long gem_menu_tree[8]; // menu tree pointers
long gem_menu_owner[8]; // owning app ID
long gem_menu_count;
long gem_menu_bar_visible; // 0=none, 1=shown
long gem_menu_active_app; // app ID whose menu bar is showing
// Per-menu item state (max 64 items per menu tree, max 8 menus)
long gem_menu_item_count[8]; // item count per tree
long gem_menu_item_checked[8]; // bitmask of checked items
long gem_menu_item_enabled[8]; // bitmask of enabled items
long gem_menu_item_normal[8]; // bitmask of normal (non-inverted) items
long gem_menu_item_text[8]; // pointer array to text strings (simulated)
char gem_menu_item_text_buf[512]; // text storage buffer
long gem_menu_pending_action; // 0=none, 1=item_selected
long gem_menu_pending_item; // selected item index
long gem_menu_pending_tree; // selected menu tree
long gem_menu_pending_app; // app to notify

// Object tree tracking
long gem_obj_tree[8]; // object tree pointers

// Form state
long gem_form_active; // handle of active form dialog (-1 = none)

// Radio button gadget tracking
long gem_radio_gad[32]; // gadtools gadget pointers (up to 32 radio buttons)
long gem_radio_obj[32]; // GEM object index for each gadget
long gem_radio_count; // number of active radio gadgets
long gem_radio_tree; // tree pointer for current radio group
long gem_radio_win; // window gadgets are attached to

// Clipboard state
char gem_scrap_buffer[1024];
long gem_scrap_len;

// Graf (graphics) state
long gem_mouse_x; long gem_mouse_y;           // Current mouse position
long gem_mouse_buttons;                   // Button state (bit0=left, bit1=right)
long gem_mouse_kstate;                    // Keyboard shift state
long gem_mouse_shape;                     // Current cursor shape (M_ON..M_POINT)
long gem_mouse_visible;                   // 0=hidden, 1=visible
long gem_graf_wk_handle;                  // Workstation handle
long gem_graf_char_w; long gem_graf_char_h;    // Character cell size
long gem_graf_arrow_mode;                 // 0=menu nav, 1=normal arrows
long gem_graf_slidex[8];   // Slider positions
long gem_graf_slidey[8];
long gem_graf_accel_key;                  // Last accelerator key
// Mouse pointer data store (33 words: 16 mask + 16 data + 1 resolution/hotspot)
long gem_mouse_user_data[33];
long gem_mouse_user_hotx; long gem_mouse_user_hoty;
long gem_mouse_user_active;
// Predefined pointer shape bitmaps (16x16, 16 data words per shape)
long gem_mouse_arrow_data[16];
long gem_mouse_arrow_mask[16];
long gem_mouse_busy_data[16];
long gem_mouse_busy_mask[16];
long gem_mouse_ibeam_data[16];
long gem_mouse_ibeam_mask[16];
long gem_mouse_point_data[16];
long gem_mouse_point_mask[16];

// AES global arrays (as per GEM AES parameter block spec)
long gem_control[12]; // control array
long gem_intin[128];
long gem_intout[128];
long gem_ptrin[16];
long gem_pttout[16];


// ============================
// VDI - Virtual Device Interface
// ============================

long vdi_handle; // current workstation handle (-1 = none)
long vdi_work_w; long vdi_work_h; // workstation pixel dimensions
long vdi_dev_w; long vdi_dev_h; // device pixel dimensions
long vdi_n_planes; // bits per pixel
long vdi_line_type; long vdi_line_width; long vdi_line_color;
long vdi_fill_type; long vdi_fill_index; long vdi_fill_color;
long vdi_marker_type; long vdi_marker_height; long vdi_marker_color;
long vdi_text_font; long vdi_text_color; long vdi_text_rotation;
long vdi_wr_mode; // writing mode (1=replace, 2=transparent, 3=XOR, 4=reverseTransparent)
long vdi_clip_x; long vdi_clip_y; long vdi_clip_w; long vdi_clip_h;
long vdi_cur_x; long vdi_cur_y; // graphics cursor position

// VDI colour lookup (Atari ST standard 16-colour palette indexed by colour index)
char vdi_rgb[48]; // 16 colours x 3 bytes (R,G,B)

// VDI line type patterns (dash/dot definitions)
long vdi_line_pats[6];

// VDI pen colour to Amiga pen mapping (first 16 standard VDI colours)
long vdi_pen_map[16];
void vdi_init_pen_map() {
  vdi_pen_map[0] = 0; vdi_pen_map[1] = 1; vdi_pen_map[2] = 2; vdi_pen_map[3] = 3;
  vdi_pen_map[4] = 4; vdi_pen_map[5] = 5; vdi_pen_map[6] = 6; vdi_pen_map[7] = 7;
  vdi_pen_map[8] = 8; vdi_pen_map[9] = 9; vdi_pen_map[10] = 10; vdi_pen_map[11] = 11;
  vdi_pen_map[12] = 12; vdi_pen_map[13] = 13; vdi_pen_map[14] = 14; vdi_pen_map[15] = 15;
}
long gem_GetVDIRastPort() {
  extern long gem_window_list[16];
  struct Window *win;
  int i;
  for (i = 0; i < 16; i++) {
    win = (struct Window *)gem_window_list[i];
    if (win) return (long)win->RPort;
  }
  return 0;
}


// ---------------------------------------------------------------------------
// Atari ST System Font Registration
// Registers the Atari ST character set as AmigaOS fonts via AddFont()
// 8x16 font for high-resolution (640x400), 8x8 font for low/medium
// Uses the Atari ST character encoding:
//   0x00-0x1F: special graphic chars (arrows, icons, music notes)
//   0x20-0x7E: standard ASCII (matching ISO-8859-1 in printable range)
//   0x7F:     solid block / alternate DEL
//   0x80-0xFF: extended chars (accented Latin, Greek, math, Hebrew)
// ---------------------------------------------------------------------------

// Font registration status
long gem_font_8x16_registered; long gem_font_8x8_registered;

// Atari ST 8x16 font bitmap data (256 chars × 16 bytes = 4096 bytes)
// Generated from Atari ST ROM glyph definitions
char gem_font_data_8x16[4096];

// Atari ST 8x8 font bitmap data (256 chars × 8 bytes = 2048 bytes)
char gem_font_data_8x8[2048];

// Character width tables (all 8 for fixed-width)
char gem_font_width_8x16[256];
char gem_font_width_8x8[256];

// Character location tables (WORD offsets into bitmap data)
char gem_font_loc_8x16[512];
char gem_font_loc_8x8[256];

// TextFont structures allocated on the heap
struct TextFont* gem_font_8x16=NULL;
struct TextFont* gem_font_8x8=NULL;

// ---------------------------------------------------------------------------
// AmigaOS mode mapping for Fopen
// GEMDOS: 0=read, 1=write, 2=read+write
// AmigaOS: MODE_OLDFILE=1005, MODE_NEWFILE=1006
// ---------------------------------------------------------------------------
long gemdos_fopen_mode(long gem_mode) {
  long result;
  if( gem_mode == 0) {
    result = 1005;
  } else {
    if( gem_mode == 1) {
      result = 1006;
    } else {
      if( gem_mode == 2) {
        result = 1006;
      } else {
        result = 1005;
      }
    }
  }
	return result;
} 

long gemdos_to_amiga_seek(long gem_seek) {
  long result;
  if( gem_seek == GEMDOS_SEEK_START) {
    result = -1;
  } else {
    if( gem_seek == GEMDOS_SEEK_CUR) {
      result = 0;
    } else {
      if( gem_seek == GEMDOS_SEEK_END) {
        result = 1;
      } else {
        result = -1;
      }
    }
  }
	return result;
} 

// ---------------------------------------------------------------------------
// GEMDOS dispatch - called from assembly trap handler
// ctx[0] = function number on entry, return value on exit
// ctx[1..7] = D1-D7 parameters
// ctx[8..14] = A0-A6 parameters
// ---------------------------------------------------------------------------
void gemdos_dispatch() {
  long fn;
  fn =  return (int)v; ;

  switch( fn) {

  case 0x00 :// gemdos_pterm0()

  	break;
  case 0x01 :// gemdos_cconin()
  	break;
  case 0x02 :// gemdos_cconout()
  	break;
  case 0x06 :// gemdos_cconws()
  	break;
  case 0x07 :// gemdos_cconis()
  	break;
  case 0x08 :// gemdos_cconos()
  	break;
  case 0x09 :// gemdos_cconws()
  	break;
  case 0x0A :// gemdos_cconrs()
  	break;
  case 0x0B :// gemdos_cconis()
  	break;
  case 0x0C :// gemdos_cconin()
  	break;
  case 0x0E :// gemdos_dsetdrv()
  	break;
  case 0x0F :// gemdos_dgetdrv()
  	break;
  case 0x10 :// gemdos_dsetpath()
  	break;
  case 0x11 :// gemdos_dgetpath()
  	break;
  case 0x15 :// gemdos_fopen()
  	break;
  case 0x16 :// gemdos_fclose()
  	break;
  case 0x17 :// gemdos_fread()
  	break;
  case 0x18 :// gemdos_fwrite()
  	break;
  case 0x19 :// gemdos_fdelete()
  	break;
  case 0x1A :// gemdos_fseek()
  	break;
  case 0x1B :// gemdos_fattrib()
  	break;
  case 0x1E :// gemdos_fdatime()
  	break;
  case 0x1F :// gemdos_fsfirst()
  	break;
  case 0x20 :// gemdos_fsnext()
  	break;
  case 0x21 :// gemdos_fsrename()
  	break;
  case 0x22 :// gemdos_fmkdir()
  	break;
  case 0x23 :// gemdos_frmdir()
  	break;
  case 0x24 :// gemdos_fchdir()
  	break;
  case 0x25 :// gemdos_fgetdta()
  	break;
  case 0x26 :// gemdos_fsetdta()
  	break;
  case 0x29 :// gemdos_malloc()
  	break;
  case 0x2A :// gemdos_mfree()
  	break;
  case 0x2D :// gemdos_pexec()
  	break;
  case 0x2E :// gemdos_pterm()
  	break;
  case 0x30 :// gemdos_super()
  	break;
  case 0x31 :// gemdos_tgetdate()
  	break;
  case 0x32 :// gemdos_tsetdate()
  	break;
  case 0x33 :// gemdos_tgettime()
  	break;
  case 0x34 :// gemdos_tsettime()
  	break;
  case 0x39 :// gemdos_mxalloc()
  	break;

  default:
    ctx[0] = E_ERROR;

  	break;

  }
	return ;
}


// ---------------------------------------------------------------------------
// Pterm0 ($00) - Terminate with return code 0
// ---------------------------------------------------------------------------
void gemdos_pterm0() {
  ctx[0] = E_OK;
	return ;
  // In a real emulator, we'd clean up and exit
}


// ---------------------------------------------------------------------------
// Cconin ($01) - Read character from console (blocking, with echo)
// Cconin ($0C) - Read character with echo control
// D0 = character read
// ---------------------------------------------------------------------------
void gemdos_cconin() {
  char* ch=NULL;
  int temp_QUAD;
try {
	ch = NewString(1);
	temp_QUAD = exception ;
	exception = 0 ;
  ch[0] = 0;
  Read(Input() ,(APTR) (unsigned long) ch ,1 );
  // Echo character
  Write(Output() ,(APTR) (unsigned long) ch ,1 );
  ctx[0] =  return (long)v; ;
} catch(...) {}
	DisposeString(ch );
	if (exception!=0) {throw eException ;} else {EMPTY;};
	exception = temp_QUAD ;
	return ;
}


// ---------------------------------------------------------------------------
// Cconout ($02) - Write character to console
// D1 = character to write
// ---------------------------------------------------------------------------
void gemdos_cconout() {
  char* ch=NULL;
  int temp_QUAD;
try {
	ch = NewString(1);
	temp_QUAD = exception ;
	exception = 0 ;
  ch[0] =  return (unsigned char)v; ;
  Write(Output() ,(APTR) (unsigned long) ch ,1 );
  ctx[0] = E_OK;
} catch(...) {}
	DisposeString(ch );
	if (exception!=0) {throw eException ;} else {EMPTY;};
	exception = temp_QUAD ;
	return ;
}


// ---------------------------------------------------------------------------
// Cconis ($07, $0B) - Check console input status
// D0 = -1 if key pressed, 0 otherwise
// ---------------------------------------------------------------------------
void gemdos_cconis() {
  // Use WaitForChar with 0 timeout
  if( -(BOOLEAN)(0!=WaitForChar(Input() ,0 ))) { ctx[0] = -1 ;} else { ctx[0] = 0;}
	return ;
}


// ---------------------------------------------------------------------------
// Cconos ($08) - Check console output status
// Always returns TRUE on Amiga
// ---------------------------------------------------------------------------
void gemdos_cconos() {
  ctx[0] = -1;
	return ;
}


// ---------------------------------------------------------------------------
// Cconws ($09, $06) - Write null-terminated string to console
// A0 = pointer to string in emulated memory
// For now, the string is in the ctx saved as A0
// ---------------------------------------------------------------------------
void gemdos_cconws() {
  char* src=NULL; long len;
  src =  return (unsigned char*)v; ;
  // Copy string from emulated memory and write it
  len = 0;
  while( (- (src[len] != 0 )& len )< 255) {
    temp_string[len] = src[len];
    len = len + 1;
  }
  temp_string[len] = 0;
  Write(Output() ,(APTR) (unsigned long) temp_string ,len );
  ctx[0] = E_OK;
	return ;
}


// ---------------------------------------------------------------------------
// Cconrs ($0A) - Read string from console with line editing
// A0 = pointer to buffer, first byte = max length
// ---------------------------------------------------------------------------
void gemdos_cconrs() {
  long maxlen; char* buf=NULL; char ch[2]; long pos, done;
  buf =  return (unsigned char*)v; ;
  maxlen = buf[0];
  pos = 0;
  done = FALSE;

  while( - (pos < maxlen )- 1 & ~ done) {
    Read(Input() ,(APTR) (unsigned long) ch ,1 );
    if( ((short) - (ch[0] == 13 )| ch[0] )== 10) {
      buf[pos + 1] = 0;
      done = TRUE;
    } else {
      if( ch[0] == 8) {
        if( pos > 0) {
          pos = pos - 1;
        }
      } else {
        buf[pos + 1] = ch[0];
        pos = pos + 1;
      }
    }
  }
  if( ~ done) {
    buf[1] = 0;
  }
  ctx[0] = 0;
	return ;
}


// ---------------------------------------------------------------------------
// Dsetdrv ($0E) - Set default drive
// D1 = drive number (0=A:)
// ---------------------------------------------------------------------------
void gemdos_dsetdrv() {
  gem_drv = ctx[1];
  ctx[0] = E_OK;
	return ;
}


// ---------------------------------------------------------------------------
// Dgetdrv ($0F) - Get default drive
// D0 = current drive (0=A:)
// ---------------------------------------------------------------------------
void gemdos_dgetdrv() {
  ctx[0] = gem_drv;
	return ;
}


// ---------------------------------------------------------------------------
// Dsetpath ($10) - Set current path
// A0 = pointer to path string
// ---------------------------------------------------------------------------
void gemdos_dsetpath() {
  char* src=NULL; long i;
  src =  return (unsigned char*)v; ;
  i = 0;
  while( (- (src[i] != 0 )& i )< 127) {
    gem_path[i] = src[i];
    i = i + 1;
  }
  gem_path[i] = 0;
  ctx[0] = E_OK;
	return ;
}


// ---------------------------------------------------------------------------
// Dgetpath ($11) - Get current path
// A0 = buffer for path string
// ---------------------------------------------------------------------------
void gemdos_dgetpath() {
  char* dst=NULL; long i;
  dst =  return (unsigned char*)v; ;
  i = 0;
  while( gem_path[i] != 0) {
    dst[i] = gem_path[i];
    i = i + 1;
  }
  dst[i] = 0;
  ctx[0] = E_OK;
	return ;
}


// ---------------------------------------------------------------------------
// Fopen ($15) - Open file
// A0 = filename pointer, D1 = mode (0=read, 1=write, 2=read+write)
// D0 = file handle or negative error
// ---------------------------------------------------------------------------
void gemdos_fopen() {
  char* filename=NULL; long mode, handle, i, result;
  filename =  return (unsigned char*)v; ;
  mode = gemdos_fopen_mode(ctx[1]);
  handle = (long) Open(filename ,mode );
  if( handle) {
    result = ENFHND;
    for ( i = 3 ; i <= 15; i = i + 1) {
      if( gem_handles[i] == 0) {
        gem_handles[i] = handle;
        result = i;
        i = 15;
      }
    }
    if( result == ENFHND) {
      -(BOOLEAN)(0!=Close( return (BPTR)v;  ));
    }
    ctx[0] = result;
  } else {
    ctx[0] = E_ERROR;
  }
	return ;
}


// ---------------------------------------------------------------------------
// Fclose ($16) - Close file
// D1 = file handle
// ---------------------------------------------------------------------------
void gemdos_fclose() {
  long handle;
  handle = ctx[1];
  if( (- (handle >= 3 )& handle )<= 15) {
    -(BOOLEAN)(0!=Close( return (BPTR)v;  ));
    gem_handles[handle] = 0;
    ctx[0] = E_OK;
  } else {
    ctx[0] = EIHND;
  }
	return ;
}


// ---------------------------------------------------------------------------
// Fread ($17) - Read from file
// D1 = handle, D2 = count, A0 = buffer
// D0 = bytes read or error
// ---------------------------------------------------------------------------
void gemdos_fread() {
  long handle, count; char* buf=NULL; long result;
  handle = ctx[1];
  count = ctx[2];
  buf =  return (unsigned char*)v; ;

  if( (- (handle >= 0 )& handle )<= 15) {
    if( handle == 0) {
      result = Read(Input() ,(APTR) (unsigned long) buf ,count );
    } else {
      result = Read( return (BPTR)v;  ,(APTR) (unsigned long) buf ,count );
    }
    if( (- (result == 0 )& count )> 0) {
      ctx[0] = E_ERROR;
    } else {
      ctx[0] = result;
    }
  } else {
    ctx[0] = EIHND;
  }
	return ;
}


// ---------------------------------------------------------------------------
// Fwrite ($18) - Write to file
// D1 = handle, D2 = count, A0 = buffer
// D0 = bytes written or error
// ---------------------------------------------------------------------------
void gemdos_fwrite() {
  long handle, count; char* buf=NULL; long result;
  handle = ctx[1];
  count = ctx[2];
  buf =  return (unsigned char*)v; ;

  if( (- (handle >= 0 )& handle )<= 15) {
    if( handle == 0) {
      result = Write(Input() ,(APTR) (unsigned long) buf ,count );
    } else {
      if( (- (handle == 1 )| handle )== 2) {
        result = Write(Output() ,(APTR) (unsigned long) buf ,count );
      } else {
        result = Write( return (BPTR)v;  ,(APTR) (unsigned long) buf ,count );
      }
    }
    if( (- (result == 0 )& count )> 0) {
      ctx[0] = E_ERROR;
    } else {
      ctx[0] = result;
    }
  } else {
    ctx[0] = EIHND;
  }
	return ;
}


// ---------------------------------------------------------------------------
// Fdelete ($19) - Delete file
// A0 = filename pointer
// ---------------------------------------------------------------------------
void gemdos_fdelete() {
  char* filename=NULL;
  filename =  return (unsigned char*)v; ;
  if( -(BOOLEAN)(0!=DeleteFile(filename ))) {
    ctx[0] = E_OK;
  } else {
    ctx[0] = E_ERROR;
  }
	return ;
}


// ---------------------------------------------------------------------------
// Fseek ($1A) - Seek in file
// D1 = handle, D2 = offset, D3 = mode (0=start, 1=current, 2=end)
// D0 = new position
// ---------------------------------------------------------------------------
void gemdos_fseek() {
  long handle, offset, mode, newpos;
  handle = ctx[1];
  offset = ctx[2];
  mode = gemdos_to_amiga_seek(ctx[3]);

  if( (- (handle >= 3 )& handle )<= 15) {
    newpos = Seek( return (BPTR)v;  ,offset ,mode );
    if( newpos < 0) {
      ctx[0] = E_ERROR;
    } else {
      ctx[0] = newpos;
    }
  } else {
    ctx[0] = EIHND;
  }
	return ;
}


// ---------------------------------------------------------------------------
// Fattrib ($1B) - Get/set file attributes
// D1 = mode (0=get, 1=set), A0 = filename, D2 = new attrib (if set)
// D0 = attributes or error
// ---------------------------------------------------------------------------
void gemdos_fattrib() {
  long mode; char* filename=NULL;
  mode = ctx[1];
  filename =  return (unsigned char*)v; ;
  if( mode == 0) {
    // Get attributes - simplified, returns 0
    ctx[0] = 0;
  } else {
    // Set attributes via SetProtection
    ctx[0] = E_OK;
  }
	return ;
}


// ---------------------------------------------------------------------------
// Fdatime ($1E) - Get/set file date and time
// D1 = mode (0=get, 1=set), A0 = pointer to time struct, D2 = handle
// D0 = 0 or error
// ---------------------------------------------------------------------------
void gemdos_fdatime() {
  ctx[0] = E_OK;
	return ;
}


// ---------------------------------------------------------------------------
// Fsfirst ($1F) - Find first matching file
// A0 = pattern string, D1 = attribute mask
// D0 = 0 if found, -1 if not
// ---------------------------------------------------------------------------
void gemdos_fsfirst() {
  char* pattern=NULL; long attr, i;
  char lock_name[256];
  pattern =  return (unsigned char*)v; ;
  attr = ctx[1];

  if( gem_search_lock) {
    UnLock( return (BPTR)v;  );
    gem_search_lock = 0;
  }

  i = 0;
  while( (- (pattern[i] != 0 )& i )< 127) {
    gem_search_pattern[i] = pattern[i];
    i = i + 1;
  }
  gem_search_pattern[i] = 0;

  -(BOOLEAN)(0!=GetCurrentDirName(lock_name ,255 ));
  gem_search_lock = (long) Lock(lock_name ,-2 );

  if( gem_search_lock) {
    if( -(BOOLEAN)(0!=Examine( return (BPTR)v;  ,& fib ))) {
      if( gem_dta != 0) {
        FillDTA( return (unsigned char*)v; , & fib);
      }
      gem_search_first = TRUE;
      ctx[0] = E_OK;
    } else {
      ctx[0] = E_ERROR;
    }
  } else {
    ctx[0] = E_ERROR;
  }
	return ;
}


void gemdos_fsnext() {
  if( gem_search_lock) {
    if( -(BOOLEAN)(0!=ExNext( return (BPTR)v;  ,& fib ))) {
      if( gem_dta != 0) {
        FillDTA( return (unsigned char*)v; , & fib);
      }
      ctx[0] = E_OK;
    } else {
      ctx[0] = E_ERROR;
    }
  } else {
    ctx[0] = E_ERROR;
  }
	return ;
}


// ---------------------------------------------------------------------------
// FileMatch - Match filename against GEMDOS pattern (* and ?)
// Returns TRUE if name matches pattern
// ---------------------------------------------------------------------------
long FileMatch(char* name, char* pattern) {
  long i, j, result;
  i = 0;
  j = 0;
  result = TRUE;
  while( - (pattern[j] != 0 )& result) {
    if( (long) pattern[j] == (long) "*") {
      result = TRUE;
      j = 999;
    } else {
      if( (long) pattern[j] == (long) "?") {
        if( name[i] == 0 ) { result = FALSE;}
        i = i + 1;
      } else {
        if( name[i] != pattern[j] ) { result = FALSE;}
        i = i + 1;
      }
    }
    j = j + 1;
  }
  if( (long) ((short) - (name[i] != 0 )& pattern[j-1] )!= (long) "*") { result = FALSE;}
	return result;
} 


// ---------------------------------------------------------------------------
// Helper: Fill GEMDOS DTA structure from AmigaOS FileInfoBlock
// ---------------------------------------------------------------------------
void FillDTA(char* dta, struct FileInfoBlock* fib_ptr) {
  long i;
  // GEMDOS DTA format:
  //   +0: reserved (21 bytes)
  //  +21: file attribute (byte)
  //  +22: time (word)
  //  +24: date (word)
  //  +26: size (long)
  //  +30: filename (13 chars + null)

  // Clear DTA
  for ( i = 0 ; i <= 43; i = i + 1) {
    dta[i] = 0;
  }

  // Set attribute (simplified: use 0 for normal)
  dta[21] = 0;

  // Set size (LITTLE-ENDIAN for 68000)
  dta[26] =  return (unsigned char)v; ;
  dta[27] =  return (unsigned char)v; ;
  dta[28] =  return (unsigned char)v; ;
  dta[29] =  return (unsigned char)v; ;

  // Copy filename
  i = 0;
  while( (- (fib_ptr->fib_FileName[i] != 0 )& i )< 12) {
    dta[30 + i] = fib_ptr->fib_FileName[i];
    i = i + 1;
  }
  dta[30 + i] = 0;
	return ;
}


// ---------------------------------------------------------------------------
// Fsrename ($21) - Rename file
// A0 = old name, A1 = new name
// ---------------------------------------------------------------------------
void gemdos_fsrename() {
  char* oldname=NULL; char* newname=NULL;
  oldname =  return (unsigned char*)v; ;
  newname =  return (unsigned char*)v; ;

  if( -(BOOLEAN)(0!=Rename(oldname ,newname ))) {
    ctx[0] = E_OK;
  } else {
    ctx[0] = E_ERROR;
  }
	return ;
}


// ---------------------------------------------------------------------------
// Fmkdir ($22) - Create directory
// A0 = directory name
// ---------------------------------------------------------------------------
void gemdos_fmkdir() {
  char* dirname=NULL;
  dirname =  return (unsigned char*)v; ;
  if( CreateDir(dirname )) {
    ctx[0] = E_OK;
  } else {
    ctx[0] = E_ERROR;
  }
	return ;
}


// ---------------------------------------------------------------------------
// Frmdir ($23) - Remove directory
// A0 = directory name
// ---------------------------------------------------------------------------
void gemdos_frmdir() {
  char* dirname=NULL;
  dirname =  return (unsigned char*)v; ;
  if( -(BOOLEAN)(0!=DeleteFile(dirname ))) {
    ctx[0] = E_OK;
  } else {
    ctx[0] = E_ERROR;
  }
	return ;
}


// ---------------------------------------------------------------------------
// Fchdir ($24) - Change directory (same as Dsetpath behavior)
// A0 = path string
// ---------------------------------------------------------------------------
void gemdos_fchdir() {
  char* pathname=NULL;
  pathname =  return (unsigned char*)v; ;
  // Use Dsetpath logic
  gemdos_dsetpath();
	return ;
}


// ---------------------------------------------------------------------------
// Fgetdta ($25) - Get Disk Transfer Address
// D0 = DTA pointer
// ---------------------------------------------------------------------------
void gemdos_fgetdta() {
  ctx[0] =  return (long)v; ;
	return ;
}


// ---------------------------------------------------------------------------
// Fsetdta ($26) - Set Disk Transfer Address
// D1 = new DTA pointer
// ---------------------------------------------------------------------------
void gemdos_fsetdta() {
  long ptr;
  ptr = ctx[1];
  gem_dta = (long)  return (unsigned char*)v; ;
  ctx[0] = E_OK;
	return ;
}


// ---------------------------------------------------------------------------
// Malloc ($29) - Allocate memory
// D1 = number of bytes
// D0 = pointer to memory, or 0 if failed
// ---------------------------------------------------------------------------
void gemdos_malloc() {
  long size, ptr;
  size = ctx[1];
  // Allocate with MEMF_CLEAR for zero-initialized memory
  ptr = (long) AllocMem((ULONG) size ,(ULONG) 65538 );
  if( ptr == 0) {
    ctx[0] = 0;
  } else {
    ctx[0] =  return (long)v; ;
  }
	return ;
}


// ---------------------------------------------------------------------------
// Mfree ($2A) - Free memory
// D1 = pointer to memory block
// ---------------------------------------------------------------------------
void gemdos_mfree() {
  long ptr;
  ptr = (long)  return (unsigned char*)v; ;
  // Note: AmigaOS FreeMem needs the size, which we don't know
  // In a real implementation, we'd track allocated block sizes
  // For now, free with a reasonable size or just stub
  FreeMem( return (APTR)v;  ,(ULONG) 0 );
  ctx[0] = E_OK;
	return ;
}


// ---------------------------------------------------------------------------
// Mxalloc ($39) - Allocate memory with flags
// D1 = number of bytes, D2 = flags (bit 0 = FAST, bit 1 = CLEAR)
// D0 = pointer or 0
// ---------------------------------------------------------------------------
void gemdos_mxalloc() {
  long size, flags, memflags;
  size = ctx[1];
  flags = ctx[2];
  memflags = 65538  ;// MEMF_CLEAR | MEMF_PUBLIC
  if( (flags & 1) == 0) {
    // CHIP memory requested, use MEMF_CHIP
    memflags = 65538  ;// Can't do chip on Amiga, use same
  }
  ctx[0] = (long) AllocMem((ULONG) size ,(ULONG) memflags );
	return ;
}


// ---------------------------------------------------------------------------
// Load an Atari ST PRG file into allocated memory
// Returns address of loaded program, or 0 on failure
// ---------------------------------------------------------------------------
long load_prg(char* filename) {
  BPTR fh=(BPTR) NULL; char header[32]; long result;
  long text_size, data_size, bss_size, total_size;
  char* addr=NULL;
  result = 0;

  fh = Open(filename ,1005 );
  if( fh) {
    if( Read(fh ,(APTR) (unsigned long) header ,32 )>= 32) {
      text_size =  return (long)v; ;
      data_size =  return (long)v; ;
      bss_size =  return (long)v; ;
      total_size = text_size + data_size + bss_size;

      addr =  return (unsigned char*)v; ;
      if( addr) {
        result = (long) addr;
        if( text_size > 0) {
          if( Read(fh ,(APTR) (unsigned long) addr ,text_size )< text_size) {
            result = 0;
          }
        }
        if( (- (data_size > 0 )& result )!= 0) {
          if( Read(fh ,(APTR) (unsigned long) ((long) addr + text_size) ,data_size )< data_size) {
            result = 0;
          }
        }
        if( result == 0) {
          FreeMem((APTR) (unsigned long) addr ,(ULONG) total_size );
        }
      }
    }
    -(BOOLEAN)(0!=Close(fh ));
  }
	return result;
} 


// ---------------------------------------------------------------------------
// Pexec ($2D) - Execute program
// D1 = mode (0=load, 1=load&go, 2=go, 3=load&createproc)
// A0 = filename (modes 0,1,3), A1 = command line (modes 1,3)
// D0 varies by mode
// ---------------------------------------------------------------------------
void gemdos_pexec() {
  long mode; char* filename=NULL;
  mode = ctx[1];
  filename =  return (unsigned char*)v; ;

  if( mode == 0) {
    ctx[0] = load_prg(filename);
  } else {
    ctx[0] = E_ERROR;
  }
	return ;
}


// ---------------------------------------------------------------------------
// Pterm ($2E) - Terminate process with return code
// D1 = return code
// ---------------------------------------------------------------------------
void gemdos_pterm() {
  ctx[0] = ctx[1];
	return ;
  // In a real emulator, we'd clean up and exit the emulated process
}


// ---------------------------------------------------------------------------
// Super ($30) - Enter/exit supervisor mode
// D1 = 0: enter supervisor, D1 <> 0: exit supervisor
// D0 = old supervisor mode flag
// ---------------------------------------------------------------------------
void gemdos_super() {
  // On AmigaOS, the trap handler already runs in supervisor mode
  // So entering supervisor is a no-op
  if( ctx[1] == 0) {
    ctx[0] = -1  ;// Was already in supervisor mode
  } else {
    ctx[0] = 0   ;// Can't actually exit supervisor on Amiga
  }
	return ;
}


// ---------------------------------------------------------------------------
// Tgetdate ($31) - Get system date
// D0 = date in GEMDOS format (bit 0-4: day, bit 5-8: month, bit 9-15: year-1980)
// ---------------------------------------------------------------------------
void gemdos_tgetdate() {
  struct DateStamp now;
  DateStamp(& now );
  // GEMDOS date: bits 0-4=day, 5-8=month, 9-15=year-1980
  // Simple approximation from days since 1978
  ctx[0] = now.ds_Days / 365  * 512  + 1 * 32  + 1;
	return ;
}


// ---------------------------------------------------------------------------
// Tsetdate ($32) - Set system date
// D1 = date in GEMDOS format
// ---------------------------------------------------------------------------
void gemdos_tsetdate() {
  // On AmigaOS, we don't change the system date
  ctx[0] = E_OK;
	return ;
}


// ---------------------------------------------------------------------------
// Tgettime ($33) - Get system time
// D0 = time in GEMDOS format (bit 0-4: sec/2, bit 5-10: min, bit 11-15: hour)
// ---------------------------------------------------------------------------
void gemdos_tgettime() {
  struct DateStamp now; long total_secs, hour, mins, sec;
  DateStamp(& now );
  total_secs = now.ds_Minute * 60  + now.ds_Tick / 50;
  hour = total_secs / 3600;
  mins = total_secs / 60  - hour * 60;
  sec = total_secs - hour * 3600  - mins * 60;
  ctx[0] = hour * 2048  + mins * 32  + sec / 2;
	return ;
}


// ---------------------------------------------------------------------------
// Tsettime ($34) - Set system time
// D1 = time in GEMDOS format
// ---------------------------------------------------------------------------
void gemdos_tsettime() {
  // On AmigaOS, we don't change the system time
  ctx[0] = E_OK;
	return ;
}


// ---------------------------------------------------------------------------
// BIOS (Trap #2) dispatch
// ctx[0] = function number, ctx[1]..ctx[7] = D1..D7, ctx[8]..ctx[14] = A0..A6
// ---------------------------------------------------------------------------
void bios_dispatch() {
  long fn;
  fn =  return (int)v; ;

  switch( fn) {

  case 0x00 :// bios_getmpb()

  	break;
  case 0x01 :// bios_bconin()
  	break;
  case 0x02 :// bios_bconout()
  	break;
  case 0x03 :// bios_rwabs()
  	break;
  case 0x04 :// bios_setexc()
  	break;
  case 0x05 :// bios_tickcal()
  	break;
  case 0x06 :// bios_gembp()
  	break;
  case 0x07 :// bios_bconstat()
  	break;
  case 0x08 :// bios_mediac()
  	break;
  case 0x09 :// bios_drvmap()
  	break;
  case 0x0A :// bios_kbshift()
  	break;
  case 0x0B :// bios_random()
  	break;

  case 0xC8 :// gem_aes_dispatch()

  	break;
  case 0xC9 :// gem_vdi_dispatch()
  	break;

  default:
    ctx[0] = E_ERROR;

  	break;

  }
	return ;
}


// ---------------------------------------------------------------------------
// Getmpb ($00) / Gembp ($06) - Get Memory Parameter Block
// A0 = pointer to MPB structure
// Returns memory layout info
// ---------------------------------------------------------------------------
void bios_getmpb() {
  // Stub - returns memory info for a 512KB ST
  // In emulator, return reasonable defaults
  ctx[0] = E_OK;
	return ;
}

void bios_gembp() {
  bios_getmpb();
	return ;
}


// ---------------------------------------------------------------------------
// Bconin ($01) - Console input (blocking)
// D1 = device number (0=console, 1=RS232, 2=printer, etc.)
// D0 = character read
// ---------------------------------------------------------------------------
void bios_bconin() {
  long dev; char* ch=NULL;
  int temp_QUAD;
try {
	ch = NewString(1);
	temp_QUAD = exception ;
	exception = 0 ;
  dev = ctx[1];
  if( dev == 0) {
    ch[0] = 0;
    Read(Input() ,(APTR) (unsigned long) ch ,1 );
    ctx[0] =  return (long)v; ;
  } else {
    ctx[0] = E_ERROR;
  }
} catch(...) {}
	DisposeString(ch );
	if (exception!=0) {throw eException ;} else {EMPTY;};
	exception = temp_QUAD ;
	return ;
}


// ---------------------------------------------------------------------------
// Bconout ($02) - Console output
// D1 = device, D2 = character
// ---------------------------------------------------------------------------
void bios_bconout() {
  long dev; char* ch=NULL;
  int temp_QUAD;
try {
	ch = NewString(1);
	temp_QUAD = exception ;
	exception = 0 ;
  dev = ctx[1];
  if( dev == 0) {
    ch[0] =  return (unsigned char)v; ;
    Write(Output() ,(APTR) (unsigned long) ch ,1 );
    ctx[0] = E_OK;
  } else {
    ctx[0] = E_ERROR;
  }
} catch(...) {}
	DisposeString(ch );
	if (exception!=0) {throw eException ;} else {EMPTY;};
	exception = temp_QUAD ;
	return ;
}


// ---------------------------------------------------------------------------
// Rwabs ($03) - Read/Write absolute sectors
// D0 = 0=read, 1=write
// D1 = device, D2 = sector number
// D3 = count, A0 = buffer
// ---------------------------------------------------------------------------
void bios_rwabs() {
  long rw, dev, sector, count; char* buf=NULL;
  rw =  return (int)v; ;
  dev = ctx[1];
  sector = ctx[2];
  count = ctx[3];
  buf =  return (unsigned char*)v; ;
  // Stub - no raw disk access on AmigaOS
  ctx[0] = E_ERROR;
	return ;
}


// ---------------------------------------------------------------------------
// Setexc ($04) - Set exception vector
// D1 = exception number, A0 = handler address
// Returns previous handler address
// ---------------------------------------------------------------------------
void bios_setexc() {
  // Stub - exception vectors not supported
  ctx[0] = 0;
	return ;
}


// ---------------------------------------------------------------------------
// Tickcal ($05) - Get tick calibration
// Returns number of microseconds per hardware tick (200Hz = 5000us on ST)
// In emulator, return 5000
// ---------------------------------------------------------------------------
void bios_tickcal() {
  ctx[0] = 5000;
	return ;
}


// ---------------------------------------------------------------------------
// Bconstat ($07) - Console status
// D1 = device
// D0 = -1 if ready, 0 otherwise
// ---------------------------------------------------------------------------
void bios_bconstat() {
  long dev;
  dev = ctx[1];
  if( dev == 0) {
    if( -(BOOLEAN)(0!=WaitForChar(Input() ,0 ))) { ctx[0] = -1 ;} else { ctx[0] = 0;}
  } else {
    ctx[0] = 0;
  }
	return ;
}


// ---------------------------------------------------------------------------
// Mediac ($08) - Media change check
// D1 = device, D2 = media ID
// Returns media change status
// ---------------------------------------------------------------------------
void bios_mediac() {
  // Stub - assume media not changed
  ctx[0] = 0;
	return ;
}


// ---------------------------------------------------------------------------
// Drvmap ($09) - Get drive map
// Returns bitmask of available drives (bit 0 = A:, bit 1 = B:, etc.)
// ---------------------------------------------------------------------------
void bios_drvmap() {
  // AmigaOS: assume at least drives A: and B: (but could check)
  // Return bit 0 and bit 1 set (drives A: and B:)
  ctx[0] = 3;
	return ;
}


// ---------------------------------------------------------------------------
// Kbshift ($0A) - Get/set keyboard shift state
// D1 = 0=read, 1=write, 2=read+set
// D2 = new shift state (for write)
// D0 = current shift state
// ---------------------------------------------------------------------------
void bios_kbshift() {
  long mode, new_state;
  mode = ctx[1];
  new_state = ctx[2];
  if( mode == 0) {
    ctx[0] = bios_kb_shift;
  } else {
    if( mode == 1) {
      bios_kb_shift = new_state;
      ctx[0] = bios_kb_shift;
    } else {
      if( mode == 2) {
        ctx[0] = bios_kb_shift;
        bios_kb_shift = new_state;
      } else {
        ctx[0] = 0;
      }
    }
  }
	return ;
}


// ---------------------------------------------------------------------------
// Random ($0B) - Get random number
// D0 = pseudo-random number (0-65535)
// ---------------------------------------------------------------------------
void bios_random() {
  // Use system timer low word as simple random
  struct DateStamp now;
  DateStamp(& now );
  ctx[0] = (now.ds_Days + now.ds_Minute )* 60 + now.ds_Tick;
	return ;
}


// ---------------------------------------------------------------------------
// XBIOS (Trap #3) dispatch
// ---------------------------------------------------------------------------
void xbios_dispatch() {
  long fn;
  fn =  return (int)v; ;

  switch( fn) {

  case 0x00 :// xbios_initmouse()

  	break;
  case 0x01 :// xbios_gettime()
  	break;
  case 0x02 :// xbios_settime()
  	break;
  case 0x03 :// xbios_bioskeys()
  	break;
  case 0x04 :// xbios_kbrate()
  	break;
  case 0x05 :// xbios_prtblk()
  	break;
  case 0x06 :// xbios_scrndump()
  	break;
  case 0x07 :// xbios_cursconf()
  	break;
  case 0x08 :// xbios_appl_init()
  	break;
  case 0x09 :// xbios_physbase()
  	break;
  case 0x0A :// xbios_logbase()
  	break;
  case 0x0B :// xbios_getrez()
  	break;
  case 0x0C :// xbios_setscreen()
  	break;
  case 0x0D :// xbios_setpalette()
  	break;
  case 0x0E :// xbios_setcolor()
  	break;
  case 0x10 :// xbios_floprd()
  	break;
  case 0x11 :// xbios_flopwr()
  	break;
  case 0x12 :// xbios_flopfmt()
  	break;
  case 0x13 :// xbios_flopstatus()
  	break;
  case 0x17 :// xbios_rsconf()
  	break;
  case 0x18 :// xbios_keytbl()
  	break;
  case 0x19 :// xbios_random()
  	break;
  case 0x1E :// xbios_cookieptr()
  	break;

  default:
    ctx[0] = E_ERROR;

  	break;

  }
	return ;
}


// ---------------------------------------------------------------------------
// Initmouse ($00) - Initialize mouse
// D1 = parameters
// ---------------------------------------------------------------------------
void xbios_initmouse() {
  ctx[0] = E_OK;
	return ;
}


// ---------------------------------------------------------------------------
// Gettime ($01) - Get system time (VBL count)
// D0 = longword VBL count (ticks since reset)
// ---------------------------------------------------------------------------
void xbios_gettime() {
  struct DateStamp now;
  DateStamp(& now );
  // Convert to VBL count (50Hz approximate)
  // Use days * 24 * 3600 * 50 + minutes * 60 * 50 + ticks * 50 / (50*60*24*365)
  // Simplified: just use a rough counter
  ctx[0] = ((now.ds_Days * 4320000 + now.ds_Minute )* 3000 + now.ds_Tick )* 50 / 300;
	return ;
}


// ---------------------------------------------------------------------------
// Settime ($02) - Set system time (VBL count)
// D1 = VBL count
// ---------------------------------------------------------------------------
void xbios_settime() {
  ctx[0] = E_OK;
	return ;
}


// ---------------------------------------------------------------------------
// Bioskeys ($03) - Get BIOS key table
// A0 = pointer to key table buffer
// ---------------------------------------------------------------------------
void xbios_bioskeys() {
  ctx[0] = E_ERROR;
	return ;
}


// ---------------------------------------------------------------------------
// Kbrate ($04) - Set keyboard repeat rate
// D1 = repeat rate
// ---------------------------------------------------------------------------
void xbios_kbrate() {
  ctx[0] = E_OK;
	return ;
}


// ---------------------------------------------------------------------------
// Prtblk ($05) - Set printer block
// A0 = printer block
// ---------------------------------------------------------------------------
void xbios_prtblk() {
  ctx[0] = E_OK;
	return ;
}

// ---------------------------------------------------------------------------
// Open gadtools.library with fallback to gadtools13.library
// Returns 1 on success, 0 on failure
// ---------------------------------------------------------------------------
BOOLEAN gem_init_gadtools() {
  gadtools_base = 0;
   extern unsigned long gadtools_base; gadtools_base = (unsigned long)OpenLibrary("gadtools.library", 0); ;
  if( gadtools_base == 0) {
     extern unsigned long gadtools_base; gadtools_base = (unsigned long)OpenLibrary("gadtools13.library", 0); ;
  }
	return - (gadtools_base != 0);
} 

// Find a free window slot, returns handle or -1 via ctx[0]
void gem_wind_alloc() {
  long i, result;
  result = -1;
  for ( i = 0 ; i <= MAX_WINDOWS - 1; i = i + 1) {
    if( (- (gem_wind_state[i] == WS_CLOSED )& gem_wind_handle[i] )== 0) {
      gem_wind_handle[i] = i + 1;
      gem_wind_state[i] = WS_CLOSED;
      result = gem_wind_handle[i];
      i = MAX_WINDOWS;
    }
  }
  ctx[0] = result;
	return ;
}

// Find window slot by handle, returns index via ENDPROC
long gem_wind_find_handle(long h) {
  long i, result;
  result = -1;
  for ( i = 0 ; i <= MAX_WINDOWS - 1; i = i + 1) {
    if( (- (result == -1 )& gem_wind_handle[i] )== h) {
      result = i;
    }
  }
	return result;
} 

// Push a message onto the message queue
void gem_msg_push(long msg, long src, long len) {
  gem_msg_queue[gem_msg_tail] = msg;
  gem_msg_src[gem_msg_tail] = src;
  gem_msg_len[gem_msg_tail] = len;
  gem_msg_tail = gem_msg_tail + 1;
  if( gem_msg_tail >= MAX_MESSAGES) {
    gem_msg_tail = 0;
  }
	return ;
}

// Return bitmask with bit 'n' set (n=0..15)
long gem_bit(long n) {
  long bits[16]; long result;
  bits[0] = 1; bits[1] = 2; bits[2] = 4; bits[3] = 8;
  bits[4] = 16; bits[5] = 32; bits[6] = 64; bits[7] = 128;
  bits[8] = 256; bits[9] = 512; bits[10] = 1024; bits[11] = 2048;
  bits[12] = 4096; bits[13] = 8192; bits[14] = 16384; bits[15] = 32768;
  result = 0;
  if( (- (n >= 0 )& n )<= 15) {
    result = bits[n];
  }
	return result;
} 

// Pop a message from the queue
void gem_msg_pop() {
  if( gem_msg_head != gem_msg_tail) {
    ctx[0] = gem_msg_queue[gem_msg_head];
    ctx[1] = gem_msg_src[gem_msg_head];
    ctx[2] = gem_msg_len[gem_msg_head];
    gem_msg_head = gem_msg_head + 1;
    if( gem_msg_head >= MAX_MESSAGES) {
      gem_msg_head = 0;
    }
  } else {
    ctx[0] = 0;
  }
	return ;
}


// ---------------------------------------------------------------------------
// AES dispatch
// ---------------------------------------------------------------------------
void gem_aes_dispatch() {
  long fn_group, fn_sub;

  fn_group = ctx[1];
  fn_sub = ctx[2];

  switch( fn_group) {

  case AES_APPL:
    switch( fn_sub) {
    case 0 :// gem_appl_init()
    	break;
    case 1 :// gem_appl_exit()
    	break;
    case 2 :// gem_appl_read()
    	break;
    case 3 :// gem_appl_write()
    	break;
    case 4 :// gem_appl_find()
    	break;
    default: // ctx[0] := E_ERROR
    	break;
    }

  	break;

  case AES_EVNT:
    switch( fn_sub) {
    case 0 :// gem_evnt_multi()
    	break;
    case 1 :// gem_evnt_mesag()
    	break;
    case 2 :// gem_evnt_button()
    	break;
    case 3 :// gem_evnt_mouse()
    	break;
    case 4 :// gem_evnt_keybd()
    	break;
    case 5 :// gem_evnt_dclick()
    	break;
    case 6 :// gem_evnt_timer()
    	break;
    default: // ctx[0] := E_ERROR
    	break;
    }

  	break;

  case AES_MENU:
    switch( fn_sub) {
    case 0 :// gem_menu_bar()
    	break;
    case 1 :// gem_menu_icheck()
    	break;
    case 2 :// gem_menu_ienable()
    	break;
    case 3 :// gem_menu_tnormal()
    	break;
    case 4 :// gem_menu_text()
    	break;
    case 5 :// gem_menu_register()
    	break;
    case 6 :// gem_menu_popup()
    	break;
    case 7 :// gem_menu_attach()
    	break;
    case 8 :// gem_menu_istart()
    	break;
    case 9 :// gem_menu_settings()
    	break;
    default: // ctx[0] := E_ERROR
    	break;
    }

  	break;

  case AES_OBJC:
    switch( fn_sub) {
    case 0 :// gem_objc_add()
    	break;
    case 1 :// gem_objc_delete()
    	break;
    case 2 :// gem_objc_draw()
    	break;
    case 3 :// gem_objc_find()
    	break;
    case 4 :// gem_objc_offset()
    	break;
    case 5 :// gem_objc_order()
    	break;
    case 6 :// gem_objc_edit()
    	break;
    case 7 :// gem_objc_change()
    	break;
    case 8 :// gem_objc_type()
    	break;
    default: // ctx[0] := E_ERROR
    	break;
    }

  	break;

  case AES_FORM:
    switch( fn_sub) {
    case 0 :// gem_form_do()
    	break;
    case 1 :// gem_form_dial()
    	break;
    case 2 :// gem_form_alert()
    	break;
    case 3 :// gem_form_error()
    	break;
    case 4 :// gem_form_center()
    	break;
    case 5 :// gem_form_keybd()
    	break;
    case 6 :// gem_form_button()
    	break;
    default: // ctx[0] := E_ERROR
    	break;
    }

  	break;

  case AES_SCRP:
    switch( fn_sub) {
    case 0 :// gem_scrp_read()
    	break;
    case 1 :// gem_scrp_write()
    	break;
    default: // ctx[0] := E_ERROR
    	break;
    }

  	break;

  case AES_FSEL:
    switch( fn_sub) {
    case 0 :// gem_fsel_exinput()
    	break;
    case 1 :// gem_fsel_exoutput()
    	break;
    default: // ctx[0] := E_ERROR
    	break;
    }

  	break;

  case AES_WIND:
    switch( fn_sub) {
    case 0 :// gem_wind_create()
    	break;
    case 1 :// gem_wind_open()
    	break;
    case 2 :// gem_wind_close()
    	break;
    case 3 :// gem_wind_delete()
    	break;
    case 4 :// gem_wind_get()
    	break;
    case 5 :// gem_wind_set()
    	break;
    case 6 :// gem_wind_find()
    	break;
    case 7 :// gem_wind_update()
    	break;
    case 8 :// gem_wind_calc()
    	break;
    case 9 :// gem_wind_new()
    	break;
    case 10 :// gem_wind_arrow()
    	break;
    case 11 :// gem_wind_show()
    	break;
    case 12 :// gem_wind_toolbar()
    	break;
    case 13 :// gem_wind_sized()
    	break;
    default: // ctx[0] := E_ERROR
    	break;
    }

  	break;

  case AES_GRAF:
    switch( fn_sub) {
    case 0 :// gem_graf_rubberbox()
    	break;
    case 1 :// gem_graf_dragbox()
    	break;
    case 2 :// gem_graf_movebox()
    	break;
    case 3 :// gem_graf_growbox()
    	break;
    case 4 :// gem_graf_shrinkbox()
    	break;
    case 5 :// gem_graf_watchbox()
    	break;
    case 6 :// gem_graf_slidebox()
    	break;
    case 7 :// gem_graf_handle()
    	break;
    case 8 :// gem_graf_mkstate()
    	break;
    case 9 :// gem_graf_mouse()
    	break;
    case 10 :// gem_graf_arrow()
    	break;
    case 11 :// gem_graf_set_screen()
    	break;
    case 12 :// gem_graf_set_handle()
    	break;
    case 13 :// gem_graf_accel()
    	break;
    default: // ctx[0] := E_ERROR
    	break;
    }

  	break;

  default:
    ctx[0] = E_ERROR;

  	break;

  }
	return ;
}


// ---------------------------------------------------------------------------
// GEM AES function implementations
// Each group implements the GEM AES spec mapped to AmigaOS Intuition
// ---------------------------------------------------------------------------

// ============================
// APPL - Application services
// ============================

void gem_appl_init() {
  long ap_id;
  gem_aes_id = gem_aes_id + 1;
  ap_id = gem_aes_id;
  if( gem_app_count < MAX_APPS) {
    gem_app_list[gem_app_count] = ap_id;
    gem_app_count = gem_app_count + 1;
  }
  ctx[0] = ap_id;
	return ;
}

void gem_appl_exit() {
  long i, id;
  id = gem_aes_id;
  for ( i = 0 ; i <= gem_app_count - 1; i = i + 1) {
    if( gem_app_list[i] == id) {
      gem_app_list[i] = gem_app_list[gem_app_count - 1];
      gem_app_count = gem_app_count - 1;
      i = gem_app_count;
    }
  }
  gem_aes_id = 0;
  ctx[0] = 1;
	return ;
}

void gem_appl_read() {
  long mg_timeout;
  mg_timeout = ctx[3];
  gem_msg_pop();
	return ;
}

void gem_appl_write() {
  long dest_id, msg_len, msg_data;
  dest_id = ctx[1];
  msg_len = ctx[2];
  if( gem_msg_tail != gem_msg_head) {
    gem_msg_push(dest_id * 256  + msg_len, gem_aes_id, msg_len);
    ctx[0] = 1;
  } else {
    ctx[0] = 0;
  }
	return ;
}

void gem_appl_find() {
  // Search known applications by name (parm on addr stack)
  // Without a real app registry, return -1 (not found)
  ctx[0] = -1;
	return ;
}


// ============================
// EVNT - Event services
// ============================

void gem_evnt_multi() {
  long flags, bclk, bmsk, bst;
  long m1flags, m1x, m1y, m1w, m1h;
  long m2flags, m2x, m2y, m2w, m2h;
  long timer, mg_time, messag;
  long i, result, done;

  flags = ctx[3]; bclk = ctx[4]; bmsk = ctx[5]; bst = ctx[6];
  m1flags = ctx[7]; m1x = ctx[8]; m1y = ctx[9]; m1w = ctx[10]; m1h = ctx[11];
  m2flags = ctx[12]; m2x = ctx[13]; m2y = ctx[14]; m2w = ctx[15]; m2h = ctx[16]; mg_time = ctx[18]; messag = ctx[19];

  result = 0;
  done = 0;

  // Check message queue first (highest priority)
  if( (- (done == 0 )& gem_msg_head )!= gem_msg_tail) {
    gem_msg_pop();
    result = 3;
    done = 1;
  }

  // Check button event
  if( (- (done == 0 )& (flags & 1) )!= 0) {
    result = 1;
    ctx[1] = 2;
    ctx[2] = 320;
    ctx[3] = 200;
    ctx[4] = 320;
    ctx[5] = 200;
    ctx[6] = 0;
    ctx[7] = 0;
    done = 1;
  }

  // Check timer
  if( (- (done == 0 )& (flags & 16) )!= 0) {
    result = 3;
    done = 1;
  }

  // Check mouse rectangle 1
  if( (- (done == 0 )& (flags & 32) )!= 0) {
  }

  // Default: no event or message
  ctx[0] = result;
	return ;
}

void gem_evnt_mesag() {
  gem_msg_pop();
	return ;
}

void gem_evnt_button() {
  long flag, bclk, bmsk, bst;
  flag = ctx[3];
  bclk = ctx[4];
  bmsk = ctx[5];
  bst = ctx[6];
  // Return immediate button event
  ctx[0] = 1;
  ctx[1] = 2    ;// double-click (unused)
  ctx[2] = 320  ;// x
  ctx[3] = 200  ;// y
	return ;
}

void gem_evnt_mouse() {
  long flag, mx, my, mw, mh;
  flag = ctx[3];
  mx = ctx[4]; my = ctx[5]; mw = ctx[6]; mh = ctx[7];
  // Assume mouse is in rectangle
  ctx[0] = 1;
  ctx[1] = 320;
  ctx[2] = 200;
	return ;
}

void gem_evnt_keybd() {
  // Return no key event
  ctx[0] = 0;
  ctx[1] = 0  ;// keycode
  ctx[2] = 0  ;// shift state
	return ;
}

void gem_evnt_dclick() {
  long new_dclick, set_flag;
  set_flag = ctx[3];
  new_dclick = ctx[4];
  // If set_flag is 1, set the new double-click rate; return old
  ctx[0] = 5;
	return ;
}

void gem_evnt_timer() {
  // Return immediately (0 = timer expired)
  ctx[0] = 0;
	return ;
}


// ============================
// MENU - Menu services
// ============================

void gem_menu_bar() {
  long tree, show, i;
  tree = ctx[3];
  show = ctx[4];
  if( show) {
    gem_menu_bar_visible = 1;
    gem_menu_active_app = gem_aes_id;
    // Find the menu tree owned by this app
    for ( i = 0 ; i <= gem_menu_count - 1; i = i + 1) {
      if( gem_menu_owner[i] == gem_aes_id) {
        gem_menu_pending_tree = gem_menu_tree[i];
      }
    }
  } else {
    gem_menu_bar_visible = 0;
    gem_menu_active_app = 0;
  }
  ctx[0] = 1;
	return ;
}

void gem_menu_icheck() {
  long tree, item, check, i, idx;
  tree = ctx[3];
  item = ctx[4];
  check = ctx[5];
  idx = -1;
  for ( i = 0 ; i <= gem_menu_count - 1; i = i + 1) {
    if( gem_menu_tree[i] == tree) {
      idx = i;
      i = gem_menu_count;
    }
  }
  if( (- ((- (idx >= 0 )& item )>= 0 )& item )< 64) {
    if( check) {
      gem_menu_item_checked[idx] = gem_menu_item_checked[idx] | gem_bit(item);
    } else {
      gem_menu_item_checked[idx] = gem_menu_item_checked[idx] & 65535 - gem_bit(item);
    }
  }
  ctx[0] = 1;
	return ;
}

void gem_menu_ienable() {
  long tree, item, enable, i, idx;
  tree = ctx[3];
  item = ctx[4];
  enable = ctx[5];
  idx = -1;
  for ( i = 0 ; i <= gem_menu_count - 1; i = i + 1) {
    if( gem_menu_tree[i] == tree) {
      idx = i;
      i = gem_menu_count;
    }
  }
  if( (- ((- (idx >= 0 )& item )>= 0 )& item )< 64) {
    if( enable) {
      gem_menu_item_enabled[idx] = gem_menu_item_enabled[idx] | gem_bit(item);
    } else {
      gem_menu_item_enabled[idx] = gem_menu_item_enabled[idx] & 65535 - gem_bit(item);
    }
  }
  ctx[0] = 1;
	return ;
}

void gem_menu_tnormal() {
  long tree, item, normal, i, idx;
  tree = ctx[3];
  item = ctx[4];
  normal = ctx[5];
  idx = -1;
  for ( i = 0 ; i <= gem_menu_count - 1; i = i + 1) {
    if( gem_menu_tree[i] == tree) {
      idx = i;
      i = gem_menu_count;
    }
  }
  if( (- ((- (idx >= 0 )& item )>= 0 )& item )< 64) {
    if( normal == 0) {
      gem_menu_item_normal[idx] = gem_menu_item_normal[idx] | gem_bit(item);
    } else {
      gem_menu_item_normal[idx] = gem_menu_item_normal[idx] & 65535 - gem_bit(item);
    }
  }
  ctx[0] = 1;
	return ;
}

void gem_menu_text() {
  long tree, item, text_ptr, i, idx;
  tree = ctx[3];
  item = ctx[4];
  text_ptr = ctx[5];
  idx = -1;
  for ( i = 0 ; i <= gem_menu_count - 1; i = i + 1) {
    if( gem_menu_tree[i] == tree) {
      idx = i;
      i = gem_menu_count;
    }
  }
  if( (- ((- (idx >= 0 )& item )>= 0 )& item )< 64) {
    gem_menu_item_text[idx] = text_ptr;
  }
  ctx[0] = 1;
	return ;
}

void gem_menu_register() {
  long pid, tree;
  pid = ctx[3];
  tree = ctx[4];
  if( gem_menu_count < 8) {
    gem_menu_tree[gem_menu_count] = tree;
    gem_menu_owner[gem_menu_count] = pid;
    gem_menu_item_count[gem_menu_count] = 0;
    gem_menu_item_checked[gem_menu_count] = 0;
    gem_menu_item_enabled[gem_menu_count] = 0;
    gem_menu_item_normal[gem_menu_count] = 0;
    gem_menu_item_text[gem_menu_count] = 0;
    gem_menu_count = gem_menu_count + 1;
  }
  ctx[0] = gem_menu_count;
	return ;
}

void gem_menu_popup() {
  long menu_id, x, y;
  menu_id = ctx[3];
  x = ctx[4];
  y = ctx[5];
  // If there's a pending menu action, return it
  if( (- (gem_menu_pending_action == 1 )& gem_menu_pending_app )== gem_aes_id) {
    ctx[1] = gem_menu_pending_item;
    gem_menu_pending_action = 0;
    ctx[0] = 1;
  } else {
    // No selection made - return 0 (cancelled)
    ctx[1] = 0;
    ctx[0] = 0;
  }
	return ;
}

void gem_menu_attach() {
  long pid, tree, item, child_tree;
  pid = ctx[3];
  tree = ctx[4];
  item = ctx[5];
  child_tree = ctx[6];
  ctx[0] = 1;
	return ;
}

void gem_menu_istart() {
  long pid, tree, item;
  pid = ctx[3];
  tree = ctx[4];
  item = ctx[5];
  // Mark that user is interacting with menu item
  ctx[0] = 1;
	return ;
}

void gem_menu_settings() {
  long pid, tree, item, settings;
  pid = ctx[3];
  tree = ctx[4];
  item = ctx[5];
  settings = ctx[6];
  // Store menu settings flags
  ctx[0] = 1;
	return ;
}


// ============================
// OBJC - Object services
// ============================

void gem_objc_add() {
  long tree, parent, child;
  tree = ctx[3];
  parent = ctx[4];
  child = ctx[5];
  ctx[0] = 1;
	return ;
}

void gem_objc_delete() {
  ctx[0] = 1;
	return ;
}

void gem_objc_draw() {
  long tree, obj, depth;
  long xc, yc, wc, hc;
  tree = ctx[3];
  obj = ctx[4];
  depth = ctx[5];
  xc = ctx[6]; yc = ctx[7]; wc = ctx[8]; hc = ctx[9];
  // Typically triggers object redraw
  ctx[0] = 1;
	return ;
}

void gem_objc_find() {
  long tree, obj, depth, mx, my;
  long found = 0;
  tree = ctx[3]; obj = ctx[4]; depth = ctx[5];
  mx = ctx[6]; my = ctx[7];
  if (tree != 0) {
    struct GEMObject { short next, head, tail; unsigned short flags, state, type; long spec; short x, y, w, h; };
    struct GEMObject *objtree = (struct GEMObject *)tree;
    int start = (int)obj;
    int i;
    for (i = start; objtree[i].type != 0; i++) {
      if (objtree[i].type == 21 || objtree[i].type == 22 || objtree[i].type == 6) {
        if (mx >= objtree[i].x && mx < objtree[i].x + objtree[i].w &&
            my >= objtree[i].y && my < objtree[i].y + objtree[i].h) {
          found = i;
        }
      }
    }
  }
  ctx[0] = found;
	return ;
}

void gem_objc_offset() {
  long tree, obj;
  tree = ctx[3]; obj = ctx[4];
  // Return offset of object within tree
  ctx[1] = 0;
  ctx[2] = 0;
  ctx[0] = E_OK;
	return ;
}

void gem_objc_order() {
  ctx[0] = 1;
	return ;
}

void gem_objc_edit() {
  // Returns keystroke or 0
  ctx[0] = 0;
	return ;
}

void gem_objc_change() {
  long tree, obj, depth, new_state;
  tree = ctx[3]; obj = ctx[4]; depth = ctx[5]; new_state = ctx[9];
  if (tree != 0 && obj >= 0) {
    struct GEMObject { short next, head, tail; unsigned short flags, state, type; long spec; short x, y, w, h; };
    struct GEMObject *objtree = (struct GEMObject *)tree;
    int idx = (int)obj;
    objtree[idx].state = (unsigned short)((int)new_state & 0xFFFF);
  }
  ctx[0] = 1;
	return ;
}

void gem_objc_type() {
  ctx[0] = E_OK;
	return ;
}


// ============================
// FORM - Form services
// ============================

void gem_form_do() {
  long tree, startobj, obj_ptr, key;
  long form_x, form_y, form_w, form_h;
  long win_idx, win_h;
  long done, event_obj, i;
  tree = ctx[3]; startobj = ctx[4];
  gem_form_active = tree;
  if (tree == 0) {
    gem_form_active = -1;
    ctx[0] = startobj;
    return;
  }
  obj_ptr = tree;
  form_x = 0; form_y = 0; form_w = 200; form_h = 100;
  {
    unsigned char *obj = (unsigned char *)obj_ptr;
    short ot = *(short *)(obj + 6);
    if (ot >= 0 && ot <= 3) {
      form_x = *(short *)(obj + 12);
      form_y = *(short *)(obj + 14);
      form_w = *(short *)(obj + 16);
      form_h = *(short *)(obj + 18);
    }
  }
  win_h = 0;
  for (i = 0; i < 16; i++) {
    if (win_h == 0 && gem_window_list[i] != 0 && gem_wind_state[i] == 1)
      win_h = gem_wind_handle[i];
  }
  gem_radio_count = 0;
  if (win_h != 0) {
    win_idx = gem_wind_find_handle(win_h);
    if (win_idx >= 0) {
      // Create radio button gadgets from the tree
      {
        extern unsigned long gadtools_base;
        struct GEMObject { short next, head, tail; unsigned short flags, state, type; long spec; short x, y, w, h; };
        struct GEMObject *objtree = (struct GEMObject *)tree;
        struct Window *winp = (struct Window *)gem_window_list[win_idx];
        struct Gadget *last_gad = NULL;
        struct NewGadget ng;
        int gi, count = 0;
        if (gadtools_base && winp) {
          for (gi = 0; objtree[gi].type != 0 && count < 32; gi++) {
            if (objtree[gi].type == 21) { /* G_RBUTTON */
              ng.ng_LeftEdge  = objtree[gi].x;
              ng.ng_TopEdge   = objtree[gi].y;
              ng.ng_Width     = objtree[gi].w;
              ng.ng_Height    = objtree[gi].h;
              ng.ng_VisualInfo = GetVisualInfo(winp->WScreen, NULL);
              ng.ng_TextAttr   = &winp->WScreen->RastPort->Font->tf_Attr;
              ng.ng_Flags      = 0;
              ng.ng_GadgetText = (STRPTR)objtree[gi].spec;
              ng.ng_GadgetID   = gi;
              ng.ng_UserData   = (APTR)0;
              gem_radio_gad[count] = (long)CreateGadgetA(RADIO_KIND, last_gad, &ng, NULL);
              if (gem_radio_gad[count]) {
                gem_radio_obj[count] = gi;
                if (objtree[gi].state & 1) {
                  GT_SetGadgetAttrs((struct Gadget *)gem_radio_gad[count], winp, NULL,
                    GTCB_Checked, 1, TAG_DONE);
                }
                last_gad = (struct Gadget *)gem_radio_gad[count];
                count++;
              }
            }
          }
          gem_radio_count = count;
          if (count > 0) {
            AddGList(winp, (struct Gadget *)gem_radio_gad[0], 0, -1, NULL);
            RefreshGList((struct Gadget *)gem_radio_gad[0], winp, NULL, -1);
            GT_RefreshWindow(winp, NULL);
          }
        }
      }
    }
  }
  done = 0;
  event_obj = startobj;
  while (done == 0) {
    if (win_h != 0) {
      win_idx = gem_wind_find_handle(win_h);
      if (win_idx >= 0 && gem_window_list[win_idx] != 0) {
        struct Window *win = (struct Window *)gem_window_list[win_idx];
        struct IntuiMessage *msg;
        if (win && win->UserPort) {
          while ((msg = (struct IntuiMessage *)GetMsg(win->UserPort))) {
            if (msg->Class == IDCMP_GADGETUP || msg->Class == IDCMP_GADGETDOWN) {
              struct Gadget *gad = (struct Gadget *)msg->IAddress;
              { int ri;
                for (ri = 0; ri < gem_radio_count; ri++) {
                  if ((long)gem_radio_gad[ri] == (long)gad) {
                    event_obj = gem_radio_obj[ri];
                    done = 1;
                    break;
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
      }
    }
    if (done == 0 && WaitForChar(Input(), 0)) {
      key = 0;
      Read(Input(), &key, 1);
      if (key == 10 || key == 13) {
        event_obj = startobj;
        done = 1;
      }
    }
    if (done == 0) Delay(1);
  }
  // Remove radio gadgets
  if (gem_radio_count > 0) {
    win_idx = gem_wind_find_handle(win_h);
    if (win_idx >= 0) {
      struct Window *w = (struct Window *)gem_window_list[win_idx];
      if (w) RemoveGList(w, (struct Gadget *)gem_radio_gad[0], gem_radio_count);
    }
    for (i = 0; i < gem_radio_count; i++) {
      if (gem_radio_gad[i]) {
        FreeGadgets((struct Gadget *)gem_radio_gad[i]);
        gem_radio_gad[i] = 0;
      }
    }
    gem_radio_count = 0;
  }
  gem_form_active = -1;
  ctx[0] = event_obj;
	return ;
}

void gem_form_dial() {
  long dtype, ix, iy, iw, ih, x, y, w, h;
  dtype = ctx[3];
  ix = ctx[4]; iy = ctx[5]; iw = ctx[6]; ih = ctx[7];
  x = ctx[8]; y = ctx[9]; w = ctx[10]; h = ctx[11];
  // Animated dialog box transition (0=init, 1=start, 2=draw, 3=exit)
  ctx[0] = 1;
	return ;
}

void gem_form_alert() {
  long default_btn;
  default_btn = ctx[3];
  // Show a simple alert; return default button
  ctx[0] = default_btn;
	return ;
}

void gem_form_error() {
  ctx[0] = 1;
	return ;
}

void gem_form_center() {
  long tree;
  tree = ctx[3];
  // Center form on screen
  ctx[1] = (gem_scrn_w - 200) / 2;
  ctx[2] = (gem_scrn_h - 100) / 2;
  ctx[3] = 200;
  ctx[4] = 100;
  ctx[0] = E_OK;
	return ;
}

void gem_form_keybd() {
  // Return next object (0 = none)
  ctx[0] = 0;
	return ;
}

void gem_form_button() {
  long tree, obj, depth, next_obj;
  tree = ctx[3]; obj = ctx[4]; depth = ctx[5];
  next_obj = 0;
  // If this is a radio button (type 21), handle mutual exclusion
  if (tree != 0 && obj >= 0) {
    struct GEMObject { short next, head, tail; unsigned short flags, state, type; long spec; short x, y, w, h; };
    struct GEMObject *objtree = (struct GEMObject *)tree;
    int idx = (int)obj;
    if (objtree[idx].type == 21) { /* G_RBUTTON */
      int i;
      for (i = 0; objtree[i].type != 0; i++) {
        if (objtree[i].type == 21) {
          objtree[i].state &= ~1; /* clear SELECTED */
        }
      }
      objtree[idx].state |= 1;
      next_obj = idx;
    } else if (objtree[idx].type == 22) { /* G_CHECKBOX: toggle */
      objtree[idx].state ^= 1;
      next_obj = idx;
    } else {
      next_obj = idx;
    }
  }
  ctx[0] = next_obj;
	return ;
}


// ============================
// SCRP - Scrap (clipboard)
// ============================

void gem_scrp_read() {
  // Set pointer to scrap buffer, return length
  ctx[1] = gem_scrap_len;
  ctx[0] = E_OK;
	return ;
}

void gem_scrp_write() {
  // Write scrap from pointer
  gem_scrap_len = ctx[1];
  ctx[0] = E_OK;
	return ;
}


// ============================
// FSEL - File selector
// ============================

void gem_fsel_exinput() {
  // Return path and filename
  ctx[1] = 0   ;// path ptr
  ctx[2] = 0   ;// selection ptr
  ctx[0] = 0   ;// 0=cancelled
	return ;
}

void gem_fsel_exoutput() {
  ctx[0] = 0;
	return ;
}


// ============================
// WIND - Window services
// ============================

void gem_wind_create() {
  long kind;
  kind = ctx[3];
  gem_wind_alloc();
	return ;
}

void gem_wind_open() {
  long handle, idx; char title[128];
  handle = ctx[3];
  idx = gem_wind_find_handle(handle);
  if( idx >= 0) {
    gem_wind_state[idx] = WS_OPEN;
    gem_wind_x[idx] = ctx[4];
    gem_wind_y[idx] = ctx[5];
    gem_wind_w[idx] = ctx[6];
    gem_wind_h[idx] = ctx[7];
    gem_wind_work_x[idx] = ctx[4] + 4;
    gem_wind_work_y[idx] = ctx[5] + 30;
    gem_wind_work_w[idx] = ctx[6] - 8;
    gem_wind_work_h[idx] = ctx[7] - 34;
    gem_wind_full_x[idx] = ctx[4];
    gem_wind_full_y[idx] = ctx[5];
    gem_wind_full_w[idx] = ctx[6];
    gem_wind_full_h[idx] = ctx[7];
    title[0] = 0;
    gem_window_list[idx] =  struct Window *win; struct NewWindow nw; long flags = WFLG_SMART_REFRESH | WFLG_ACTIVATE | WFLG_GIMMEZEROZERO; long idcmp = IDCMP_CLOSEWINDOW | IDCMP_REFRESHWINDOW | IDCMP_SIZEVERIFY | IDCMP_NEWSIZE | IDCMP_MOUSEBUTTONS | IDCMP_MOUSEMOVE; if (kind & 2) flags |= WFLG_CLOSEGADGET; if (kind & 4) flags |= WFLG_DEPTHGADGET; if (kind & 8) flags |= WFLG_DRAGBAR; if (kind & 16) flags |= WFLG_SIZEGADGET; nw.LeftEdge = (long)gem_wind_x[idx] ; nw.TopEdge = (long)gem_wind_y[idx] ; nw.Width = (long)gem_wind_w[idx] ; nw.Height = (long)gem_wind_h[idx] ; nw.DetailPen = 0; nw.BlockPen = 1; nw.Title = (STRPTR)title ; nw.Flags = flags; nw.IDCMPFlags = idcmp; nw.Type = WBENCHSCREEN; nw.FirstGadget = NULL; nw.CheckMark = NULL; nw.Screen = NULL; nw.BitMap = NULL; nw.MinWidth = 50; nw.MinHeight = 30; nw.MaxWidth = 2048; nw.MaxHeight = 2048; win = OpenWindow(&nw); return (unsigned long)win; ;
    if( gem_window_list[idx] != 0) {
      ctx[0] = 1;
    } else {
      ctx[0] = 0;
    }
  } else {
    ctx[0] = 0;
  }
	return ;
}

void gem_wind_close() {
  long handle, idx;
  handle = ctx[3];
  idx = gem_wind_find_handle(handle);
  if( idx >= 0) {
    if( gem_window_list[idx] != 0) {
       HideWindow((struct Window *) return (struct Window*)v;  ); ;
    }
    gem_wind_state[idx] = WS_CLOSED;
    ctx[0] = 1;
  } else {
    ctx[0] = 0;
  }
	return ;
}

void gem_wind_delete() {
  long handle, idx;
  handle = ctx[3];
  idx = gem_wind_find_handle(handle);
  if( idx >= 0) {
    if( gem_window_list[idx] != 0) {
       CloseWindow((struct Window *) return (struct Window*)v;  ); ;
      gem_window_list[idx] = 0;
    }
    gem_wind_state[idx] = WS_CLOSED;
    gem_wind_handle[idx] = 0;
    ctx[0] = 1;
  } else {
    ctx[0] = 0;
  }
	return ;
}

void gem_wind_get() {
  long handle, field, idx;
  handle = ctx[3];
  field = ctx[4];
  idx = gem_wind_find_handle(handle);
  if( idx >= 0) {
    switch( field) {
    case 0 :// -> WF_WORKXYWH: work area
      ctx[1] = gem_wind_work_x[idx];
      ctx[2] = gem_wind_work_y[idx];
      ctx[3] = gem_wind_work_w[idx];
      ctx[4] = gem_wind_work_h[idx];
    	break;
    case 1 :// -> WF_CURRXYWH: current dimensions
      ctx[1] = gem_wind_x[idx];
      ctx[2] = gem_wind_y[idx];
      ctx[3] = gem_wind_w[idx];
      ctx[4] = gem_wind_h[idx];
    	break;
    case 2 :// -> WF_PREVXYWH: previous dimensions
      ctx[1] = gem_wind_full_x[idx];
      ctx[2] = gem_wind_full_y[idx];
      ctx[3] = gem_wind_full_w[idx];
      ctx[4] = gem_wind_full_h[idx];
    	break;
    case 5 :// -> WF_TOP: top window handle
      ctx[1] = handle;
    	break;
    case 6 :// -> WF_FIRSTXYWH: first rectangle
      ctx[1] = gem_wind_work_x[idx];
      ctx[2] = gem_wind_work_y[idx];
      ctx[3] = gem_wind_work_w[idx];
      ctx[4] = gem_wind_work_h[idx];
    	break;
    case 7 :// -> WF_OWNER: owner PID
      ctx[1] = gem_wind_parent[idx];
    	break;
    default: //
      ctx[1] = gem_wind_x[idx];
      ctx[2] = gem_wind_y[idx];
      ctx[3] = gem_wind_w[idx];
      ctx[4] = gem_wind_h[idx];
    	break;
    }
    ctx[0] = E_OK;
  } else {
    ctx[0] = E_ERROR;
  }
	return ;
}

void gem_wind_set() {
  long handle, field, idx;
  handle = ctx[3];
  field = ctx[4];
  idx = gem_wind_find_handle(handle);
  if( idx >= 0) {
    switch( field) {
    case 0 :// WF_WORKXYWH
      gem_wind_work_x[idx] = ctx[5]; gem_wind_work_y[idx] = ctx[6];
      gem_wind_work_w[idx] = ctx[7]; gem_wind_work_h[idx] = ctx[8];
    	break;
    case 1 :// WF_CURRXYWH
      gem_wind_x[idx] = ctx[5]; gem_wind_y[idx] = ctx[6];
      gem_wind_w[idx] = ctx[7]; gem_wind_h[idx] = ctx[8];
      if( gem_window_list[idx] != 0) {
         MoveWindow((struct Window *) return (struct Window*)v;  , (long)gem_wind_x[idx] , (long)gem_wind_y[idx] ); ;
         SizeWindow((struct Window *) return (struct Window*)v;  , (long)gem_wind_w[idx] , (long)gem_wind_h[idx] ); ;
      }
    	break;
    case 3 :// WF_NEWSIZE
      gem_wind_w[idx] = ctx[5]; gem_wind_h[idx] = ctx[6];
      if( gem_window_list[idx] != 0) {
         SizeWindow((struct Window *) return (struct Window*)v;  , (long)gem_wind_w[idx] , (long)gem_wind_h[idx] ); ;
      }
    	break;
    case 4 :// WF_ICONIFY
      gem_wind_state[idx] = WS_ICONIFIED;
      if( gem_window_list[idx] != 0) {
         HideWindow((struct Window *) return (struct Window*)v;  ); ;
      }
    	break;
    case 5 :// WF_TOP
    	break;
    case 10 :// WF_NAME
      if( gem_window_list[idx] != 0) {
         WindowTitle((struct Window *) return (struct Window*)v;  , (STRPTR) return (unsigned char*)v;  ); ;
      }
    	break;
    }
    ctx[0] = E_OK;
  } else {
    ctx[0] = E_ERROR;
  }
	return ;
}

void gem_wind_find() {
  long mx, my, i, result;
  mx = ctx[3]; my = ctx[4];
  result = 0;
  // Find top-most open window at (mx, my)
  for ( i = MAX_WINDOWS - 1 ; i >= 0 ; i = i + -1) {
    if( (- (result == 0 )& gem_wind_state[i] )== WS_OPEN) {
      if( - ((- ((- ((- (mx >= gem_wind_x[i] )& mx )< gem_wind_x[i] )+ gem_wind_w[i] & my )>= gem_wind_y[i] )& my )< gem_wind_y[i] )+ gem_wind_h[i]) {
        result = gem_wind_handle[i];
      }
    }
  }
  ctx[0] = result;
	return ;
}

void gem_wind_update() {
  long beg_end;
  beg_end = ctx[3];
  // 0=begin update, 1=end update
  ctx[0] = E_OK;
	return ;
}

void gem_wind_calc() {
  long calc_type, kind, x, y, w, h;
  long title_h, frame_w;
  calc_type = ctx[3];
  kind = ctx[4];
  x = ctx[5]; y = ctx[6]; w = ctx[7]; h = ctx[8];
  title_h = 30;
  frame_w = 4;
  // calc_type 0: full→work area, 1: work→full area
  if( calc_type == 0) {
    ctx[1] = x + frame_w;
    ctx[2] = y + title_h;
    ctx[3] = (w - frame_w )* 2;
    ctx[4] = h - title_h - frame_w;
  } else {
    ctx[1] = x - frame_w;
    ctx[2] = y - title_h;
    ctx[3] = (w + frame_w )* 2;
    ctx[4] = h + title_h + frame_w;
  }
  ctx[0] = E_OK;
	return ;
}

void gem_wind_new() {
  ctx[0] = 1;
	return ;
}

void gem_wind_arrow() {
  ctx[0] = E_OK;
	return ;
}

void gem_wind_show() {
  long handle, flag;
  handle = ctx[3]; flag = ctx[4];
  ctx[0] = E_OK;
	return ;
}

void gem_wind_toolbar() {
  ctx[0] = E_OK;
	return ;
}

void gem_wind_sized() {
  ctx[0] = E_OK;
	return ;
}

// Initialize predefined mouse pointer shapes
void gem_mouse_init() {
  long i;
  // Arrow sprite data (16x16)
  gem_mouse_arrow_data[0] = 0x0000;  gem_mouse_arrow_mask[0] = 0x0000;
  gem_mouse_arrow_data[1] = 0x7C00;  gem_mouse_arrow_mask[1] = 0xFC00;
  gem_mouse_arrow_data[2] = 0x7200;  gem_mouse_arrow_mask[2] = 0xF600;
  gem_mouse_arrow_data[3] = 0x7100;  gem_mouse_arrow_mask[3] = 0xF300;
  gem_mouse_arrow_data[4] = 0x7080;  gem_mouse_arrow_mask[4] = 0xF180;
  gem_mouse_arrow_data[5] = 0x7040;  gem_mouse_arrow_mask[5] = 0xF0C0;
  gem_mouse_arrow_data[6] = 0x7020;  gem_mouse_arrow_mask[6] = 0xF060;
  gem_mouse_arrow_data[7] = 0x7C10;  gem_mouse_arrow_mask[7] = 0xFC30;
  gem_mouse_arrow_data[8] = 0x4608;  gem_mouse_arrow_mask[8] = 0xEF18;
  gem_mouse_arrow_data[9] = 0x4304;  gem_mouse_arrow_mask[9] = 0xC78C;
  gem_mouse_arrow_data[10] = 0x4182; gem_mouse_arrow_mask[10] = 0xC3C6;
  gem_mouse_arrow_data[11] = 0x40E1; gem_mouse_arrow_mask[11] = 0xC1E3;
  gem_mouse_arrow_data[12] = 0x4070; gem_mouse_arrow_mask[12] = 0xC0F0;
  gem_mouse_arrow_data[13] = 0x4038; gem_mouse_arrow_mask[13] = 0xC078;
  gem_mouse_arrow_data[14] = 0x401C; gem_mouse_arrow_mask[14] = 0xC03C;
  gem_mouse_arrow_data[15] = 0x4000; gem_mouse_arrow_mask[15] = 0xC000;

  // Busy/hourglass sprite data (16x16)
  gem_mouse_busy_data[0] = 0x0000;  gem_mouse_busy_mask[0] = 0x0000;
  gem_mouse_busy_data[1] = 0x7FFE;  gem_mouse_busy_mask[1] = 0xFFFF;
  gem_mouse_busy_data[2] = 0x6006;  gem_mouse_busy_mask[2] = 0xF00F;
  gem_mouse_busy_data[3] = 0x300C;  gem_mouse_busy_mask[3] = 0x781E;
  gem_mouse_busy_data[4] = 0x1818;  gem_mouse_busy_mask[4] = 0x3C3C;
  gem_mouse_busy_data[5] = 0x0C30;  gem_mouse_busy_mask[5] = 0x1E78;
  gem_mouse_busy_data[6] = 0x0660;  gem_mouse_busy_mask[6] = 0x0FF0;
  gem_mouse_busy_data[7] = 0x03C0;  gem_mouse_busy_mask[7] = 0x07E0;
  gem_mouse_busy_data[8] = 0x0660;  gem_mouse_busy_mask[8] = 0x0FF0;
  gem_mouse_busy_data[9] = 0x0C30;  gem_mouse_busy_mask[9] = 0x1E78;
  gem_mouse_busy_data[10] = 0x1818; gem_mouse_busy_mask[10] = 0x3C3C;
  gem_mouse_busy_data[11] = 0x300C; gem_mouse_busy_mask[11] = 0x781E;
  gem_mouse_busy_data[12] = 0x6006; gem_mouse_busy_mask[12] = 0xF00F;
  gem_mouse_busy_data[13] = 0x7FFE; gem_mouse_busy_mask[13] = 0xFFFF;
  gem_mouse_busy_data[14] = 0x0000; gem_mouse_busy_mask[14] = 0x0000;
  gem_mouse_busy_data[15] = 0x0000; gem_mouse_busy_mask[15] = 0x0000;

  // I-beam/vertical bar sprite data (16x16)
  for ( i = 0 ; i <= 15; i = i + 1) {
    gem_mouse_ibeam_data[i] = 0x0180;
    gem_mouse_ibeam_mask[i] = 0x03C0;
  }
  gem_mouse_ibeam_data[0] = 0x0000; gem_mouse_ibeam_mask[0] = 0x0000;
  gem_mouse_ibeam_data[15] = 0x0000; gem_mouse_ibeam_mask[15] = 0x0000;

  // Pointing finger sprite data (16x16)
  gem_mouse_point_data[0] = 0x0000;  gem_mouse_point_mask[0] = 0x0000;
  gem_mouse_point_data[1] = 0x0E00;  gem_mouse_point_mask[1] = 0x1F00;
  gem_mouse_point_data[2] = 0x0A00;  gem_mouse_point_mask[2] = 0x1F00;
  gem_mouse_point_data[3] = 0x0A00;  gem_mouse_point_mask[3] = 0x1F00;
  gem_mouse_point_data[4] = 0x0A00;  gem_mouse_point_mask[4] = 0x1F00;
  gem_mouse_point_data[5] = 0x0A00;  gem_mouse_point_mask[5] = 0x1F00;
  gem_mouse_point_data[6] = 0x0A00;  gem_mouse_point_mask[6] = 0x1F00;
  gem_mouse_point_data[7] = 0x0A00;  gem_mouse_point_mask[7] = 0x1F00;
  gem_mouse_point_data[8] = 0x0A00;  gem_mouse_point_mask[8] = 0x1F00;
  gem_mouse_point_data[9] = 0x0A00;  gem_mouse_point_mask[9] = 0x1F00;
  gem_mouse_point_data[10] = 0x0E00; gem_mouse_point_mask[10] = 0x1F00;
  gem_mouse_point_data[11] = 0x1C00; gem_mouse_point_mask[11] = 0x3E00;
  gem_mouse_point_data[12] = 0x3800; gem_mouse_point_mask[12] = 0x7C00;
  gem_mouse_point_data[13] = 0x7000; gem_mouse_point_mask[13] = 0xF800;
  gem_mouse_point_data[14] = 0x2000; gem_mouse_point_mask[14] = 0x7000;
  gem_mouse_point_data[15] = 0x0000; gem_mouse_point_mask[15] = 0x0000;
	return ;
}

// graf_rubberbox() - Interactive rubber-band rectangle
// Draws a XOR outline box from (x,y) to (x+minw, y+minh) at minimum
// GEM waits for a button click and returns final width/height
void gem_graf_rubberbox() {
  long x, y, minw, minh, finalw, finalh;
  x = ctx[3]; y = ctx[4]; minw = ctx[5]; minh = ctx[6];
  // Simulate a rubber-band box. In a real system, this tracks the mouse.
  // We return a reasonable default size.
  finalw = minw + 40;
  finalh = minh + 30;
  ctx[1] = finalw;
  ctx[2] = finalh;
  ctx[0] = 1;
	return ;
}

// graf_dragbox() - Drag a box (interactive move feedback)
// Draws a XOR outline box of size (w x h), starts at (sx,sy)
// Constrained to area defined by (cx,cy) and bound flag:
//   bound=0: no constraint, bound=1: constrain to 1st rect, bound=2: to 2nd rect
// Returns final (x,y) position
void gem_graf_dragbox() {
  long w, h, sx, sy, cx, cy, bound, finalx, finaly;
  w = ctx[3]; h = ctx[4];
  sx = ctx[5]; sy = ctx[6];
  cx = ctx[7]; cy = ctx[8];
  bound = ctx[9];
  finalx = sx;
  finaly = sy;
  if( bound == 1) {
    if( finalx < 0 ) { finalx = 0;}
    if( finalx + w > gem_scrn_w ) { finalx = gem_scrn_w - w;}
    if( finaly < 0 ) { finaly = 0;}
    if( finaly + h > gem_scrn_h ) { finaly = gem_scrn_h - h;}
  }
  ctx[1] = finalx;
  ctx[2] = finaly;
  ctx[0] = 1;
	return ;
}

// graf_movebox() - Animate moving a box from one position to another
// Draws outline at (sx,sy) then at (dx,dy) to show movement
void gem_graf_movebox() {
  long w, h, sx, sy, dx, dy;
  w = ctx[3]; h = ctx[4]; sx = ctx[5]; sy = ctx[6];
  dx = ctx[7]; dy = ctx[8];
  ctx[0] = 1;
	return ;
}

// graf_growbox() - Animate growing a box (expand effect)
// Grows from small rect (px,py,pw,ph) to big rect (sx,sy,sw,sh)
void gem_graf_growbox() {
  long px, py, pw, ph, sx, sy, sw, sh;
  px = ctx[3]; py = ctx[4]; pw = ctx[5]; ph = ctx[6];
  sx = ctx[7]; sy = ctx[8]; sw = ctx[9]; sh = ctx[10];
  ctx[0] = 1;
	return ;
}

// graf_shrinkbox() - Animate shrinking a box (collapse effect)
// Shrinks from big rect (sx,sy,sw,sh) to small rect (px,py,pw,ph)
void gem_graf_shrinkbox() {
  long sx, sy, sw, sh, px, py, pw, ph;
  sx = ctx[3]; sy = ctx[4]; sw = ctx[5]; sh = ctx[6];
  px = ctx[7]; py = ctx[8]; pw = ctx[9]; ph = ctx[10];
  ctx[0] = 1;
	return ;
}

// graf_watchbox() - Watch an object for state change
// Waits until the user clicks on the specified object
// tree = object tree, obj = object index
// instate = color when entered, outstate = color when exited
// Returns outstate (the final state of the object)
void gem_graf_watchbox() {
  long tree, obj, instate, outstate;
  tree = ctx[3]; obj = ctx[4]; instate = ctx[5]; outstate = ctx[6];
  // Simulate: if there's a pending menu/button action, act on it
  if( (- (gem_menu_pending_action == 1 )& gem_menu_pending_app )== gem_aes_id) {
    // A menu item was selected, notify the form
    gem_menu_pending_action = 0;
    ctx[1] = gem_menu_pending_item;
    ctx[0] = outstate  ;// Object changed to outstate (was selected)
  } else {
    // No pending action, return the object index to simulate selection
    ctx[1] = obj;
    ctx[0] = outstate;
  }
	return ;
}

// graf_slidebox() - Handle slider box movement
// tree = object tree, parent = parent object, obj = slider object
// is_horiz = 1 for horizontal, 0 for vertical
// Returns new slider position (offset in pixels)
void gem_graf_slidebox() {
  long tree, parent, obj, is_horiz, idx, newpos;
  tree = ctx[3]; parent = ctx[4]; obj = ctx[5];
  is_horiz = ctx[6];
  // Find slider index from object
  idx = obj & 7;
  if( idx > 7 ) { idx = 0;}
  if( idx < 0 ) { idx = 0;}
  // Return stored slider position (simulated)
  if( is_horiz) {
    newpos = gem_graf_slidex[idx];
  } else {
    newpos = gem_graf_slidey[idx];
  }
  ctx[1] = newpos;
  ctx[0] = 1;
	return ;
}

// graf_handle() - Get graphics handle and character cell size
// Returns: workstation handle, char width, char height, cell width (in pixels)
// Also sets the workstation cell dimensions for text operations
void gem_graf_handle() {
  // Workstation handle: unique ID for the virtual device
  // Char cell: 8x16 for high-resolution Atari ST mode
  // Last value is typically 0 (box width/height unused)
  ctx[0] = gem_graf_wk_handle;
  ctx[1] = gem_graf_char_w;
  ctx[2] = gem_graf_char_h;
  ctx[3] = 0;
	return ;
}

// graf_mkstate() - Get mouse state
// Returns: button state, mouse X, mouse Y, keyboard state
void gem_graf_mkstate() {
  ctx[0] = gem_mouse_buttons;
  ctx[1] = gem_mouse_x;
  ctx[2] = gem_mouse_y;
  ctx[3] = gem_mouse_kstate;
	return ;
}

// graf_mouse() - Set mouse pointer shape
// shape: M_ON=0 (show), M_OFF=1 (hide), M_ARROW=2 (default arrow),
//        M_BUSY=3 (hourglass), M_IBEAM=4 (text), M_POINT=5 (finger),
//        M_USERDEF=6 (custom 2-plane), M_SPECIAL=7 (custom 4-plane)
// For M_USERDEF: ctx[4]=hotx, ctx[5]=hoty, ctx[6]=data_ptr
// For M_SPECIAL: same + ctx[7]=color_ptr, ctx[8]=words
// Returns previous mouse shape
void gem_graf_mouse() {
  long shape, old_shape, hotx, hoty, data_ptr; struct Window* win=NULL;
  shape = ctx[3];
  old_shape = gem_mouse_shape;

  if( shape == M_OFF) {
    gem_mouse_visible = 0;
  } else {
    if( shape == M_ON) {
      gem_mouse_visible = 1;
      gem_mouse_shape = M_ARROW;
    } else {
      if( (- (shape == M_USERDEF )| shape )== M_SPECIAL) {
        hotx = ctx[4];
        hoty = ctx[5];
        data_ptr = ctx[6];
        gem_mouse_user_hotx = hotx;
        gem_mouse_user_hoty = hoty;
        gem_mouse_user_active = 1;
        // Store pointer data from emulated memory (33 words)
        
          unsigned short *dst = (unsigned short *)gem_mouse_user_data;
          unsigned short *src = (unsigned short *)data_ptr;
          int n;
          for (n = 0; n < 33; n++) dst[n] = src[n];
        ;
        gem_mouse_shape = shape;
      } else {
        if( (- (shape >= M_ARROW )& shape )<= M_POINT) {
          gem_mouse_shape = shape;
          gem_mouse_user_active = 0;
        }
      }
      gem_mouse_visible = 1;
    }
  }

  // Attempt to update the pointer via AmigaOS Intuition if a window is open
  win =  return (struct Window*)v; ;
  if( (long) (gem_mouse_visible & (long) win )!= (long) 0) {
    if( gem_mouse_user_active) {
       SetPointer((struct Window *)win , (UWORD *)gem_mouse_user_data , (short)16 , (short)16 , (short)gem_mouse_user_hotx , (short)gem_mouse_user_hoty ); ;
    } else {
      // Use predefined shape
      if( shape == M_ARROW) {
         SetPointer((struct Window *)win , (UWORD *)gem_mouse_arrow_data , (short)16 , (short)16 , (short)0 , (short)0 ); ;
      } else {
        if( shape == M_BUSY) {
           SetPointer((struct Window *)win , (UWORD *)gem_mouse_busy_data , (short)16 , (short)16 , (short)7 , (short)7 ); ;
        } else {
          if( shape == M_IBEAM) {
             SetPointer((struct Window *)win , (UWORD *)gem_mouse_ibeam_data , (short)16 , (short)16 , (short)7 , (short)7 ); ;
          } else {
            if( shape == M_POINT) {
               SetPointer((struct Window *)win , (UWORD *)gem_mouse_point_data , (short)16 , (short)16 , (short)0 , (short)0 ); ;
            }
          }
        }
      }
    }
  } else {
    if( (long) (~ gem_mouse_visible & (long) win )!= (long) 0) {
       ClearPointer((struct Window *)win ); ;
    }
  }

  ctx[0] = old_shape;
	return ;
}

// graf_arrow() - Set arrow key mode
// flag=0: arrow keys used for menu navigation (mouse-emulated)
// flag=1: arrow keys used for normal cursor movement
void gem_graf_arrow() {
  long flag;
  flag = ctx[3];
  if( (- (flag == 0 )| flag )== 1) {
    gem_graf_arrow_mode = flag;
  }
  ctx[0] = E_OK;
	return ;
}

// graf_set_screen() - Set screen parameters (reserved, rarely used)
// Typically called during initialisation to pass screen info
void gem_graf_set_screen() {
  long handle, ws_w, ws_h, ws_bits;
  handle = ctx[3];
  ws_w = ctx[4];
  ws_h = ctx[5];
  ws_bits = ctx[6];
  gem_graf_wk_handle = handle;
  ctx[0] = E_OK;
	return ;
}

// graf_set_handle() - Set workstation handle explicitly
void gem_graf_set_handle() {
  long handle, char_w, char_h;
  handle = ctx[3];
  char_w = ctx[4];
  char_h = ctx[5];
  gem_graf_wk_handle = handle;
  if( char_w > 0 ) { gem_graf_char_w = char_w;}
  if( char_h > 0 ) { gem_graf_char_h = char_h;}
  ctx[0] = E_OK;
	return ;
}

// graf_accel() - Register/unregister accelerator key
// pid = application ID, tree = object tree
// Returns accelerator key code or 0
void gem_graf_accel() {
  long pid, tree, key;
  pid = ctx[3];
  tree = ctx[4];
  key = ctx[5];
  gem_graf_accel_key = key;
  ctx[0] = gem_graf_accel_key;
	return ;
}
void vdi_init_rgb() {
  long v;
  char vdi_rgb2[48]; // 16 colours x 3 bytes (R,G,B)
  v = 0; vdi_rgb2[0] =  return (unsigned char)v; ; vdi_rgb2[1] =  return (unsigned char)v; ; vdi_rgb2[2] =  return (unsigned char)v; ;
  v = 0; vdi_rgb2[3] =  return (unsigned char)v; ; vdi_rgb2[4] =  return (unsigned char)v; ; v = 200; vdi_rgb2[5] =  return (unsigned char)v; ;
  v = 0; vdi_rgb2[6] =  return (unsigned char)v; ; v = 200; vdi_rgb2[7] =  return (unsigned char)v; ; v = 0; vdi_rgb2[8] =  return (unsigned char)v; ;
  v = 0; vdi_rgb2[9] =  return (unsigned char)v; ; v = 200; vdi_rgb2[10] =  return (unsigned char)v; ; v = 200; vdi_rgb2[11] =  return (unsigned char)v; ;
  v = 200; vdi_rgb2[12] =  return (unsigned char)v; ; v = 0; vdi_rgb2[13] =  return (unsigned char)v; ; v = 0; vdi_rgb2[14] =  return (unsigned char)v; ;
  v = 200; vdi_rgb2[15] =  return (unsigned char)v; ; v = 0; vdi_rgb2[16] =  return (unsigned char)v; ; v = 200; vdi_rgb2[17] =  return (unsigned char)v; ;
  v = 200; vdi_rgb2[18] =  return (unsigned char)v; ; v = 200; vdi_rgb2[19] =  return (unsigned char)v; ; v = 0; vdi_rgb2[20] =  return (unsigned char)v; ;
  v = 200; vdi_rgb2[21] =  return (unsigned char)v; ; v = 200; vdi_rgb2[22] =  return (unsigned char)v; ; v = 200; vdi_rgb2[23] =  return (unsigned char)v; ;
  v = 100; vdi_rgb2[24] =  return (unsigned char)v; ; v = 100; vdi_rgb2[25] =  return (unsigned char)v; ; v = 100; vdi_rgb2[26] =  return (unsigned char)v; ;
  v = 0; vdi_rgb2[27] =  return (unsigned char)v; ; v = 0; vdi_rgb2[28] =  return (unsigned char)v; ; v = 100; vdi_rgb2[29] =  return (unsigned char)v; ;
  v = 0; vdi_rgb2[30] =  return (unsigned char)v; ; v = 100; vdi_rgb2[31] =  return (unsigned char)v; ; v = 0; vdi_rgb2[32] =  return (unsigned char)v; ;
  v = 0; vdi_rgb2[33] =  return (unsigned char)v; ; v = 100; vdi_rgb2[34] =  return (unsigned char)v; ; v = 100; vdi_rgb2[35] =  return (unsigned char)v; ;
  v = 100; vdi_rgb2[36] =  return (unsigned char)v; ; v = 0; vdi_rgb2[37] =  return (unsigned char)v; ; v = 0; vdi_rgb2[38] =  return (unsigned char)v; ;
  v = 100; vdi_rgb2[39] =  return (unsigned char)v; ; v = 0; vdi_rgb2[40] =  return (unsigned char)v; ; v = 100; vdi_rgb2[41] =  return (unsigned char)v; ;
  v = 100; vdi_rgb2[42] =  return (unsigned char)v; ; v = 100; vdi_rgb2[43] =  return (unsigned char)v; ; v = 0; vdi_rgb2[44] =  return (unsigned char)v; ;
  v = 255; vdi_rgb2[45] =  return (unsigned char)v; ; v = 255; vdi_rgb2[46] =  return (unsigned char)v; ; v = 255; vdi_rgb2[47] =  return (unsigned char)v; ;
	return ;
}
void vdi_init_line_pats() {
  vdi_line_pats[0] = 0xFFFF ;// solid (user-defined, default = solid)
  vdi_line_pats[1] = 0xFFFF ;// solid
  vdi_line_pats[2] = 0xFF88 ;// long dash
  vdi_line_pats[3] = 0xFF88 ;// short dash (same as long dash on ST)
  vdi_line_pats[4] = 0xCCCC ;// dash-dot
  vdi_line_pats[5] = 0xCCCC ;// dash-dot (alternate)
	return ;
}

// Open a virtual workstation with the given work-in array
// ctx[3] = work_in ptr (in emulated memory) — 45 words of device-independent attributes
// Returns handle via ctx[0]
void vdi_opnvwk() {
  vdi_init_pen_map();
  vdi_handle = 1;
  vdi_work_w = 640; vdi_work_h = 400;
  vdi_dev_w = 640; vdi_dev_h = 400;
  vdi_n_planes = 4;
  vdi_line_type = 1; vdi_line_width = 1; vdi_line_color = 1;
  vdi_fill_type = 1; vdi_fill_index = 1; vdi_fill_color = 1;
  vdi_marker_type = 3; vdi_marker_height = 10; vdi_marker_color = 1;
  vdi_text_font = 1; vdi_text_color = 1; vdi_text_rotation = 0;
  vdi_wr_mode = VDI_REPLACE;
  vdi_clip_x = 0; vdi_clip_y = 0; vdi_clip_w = vdi_work_w; vdi_clip_h = vdi_work_h;
  vdi_cur_x = 0; vdi_cur_y = 0;
  // Fill work_out array in emulated memory via intout
  gem_intout[0] = 0            ;// device id
  gem_intout[1] = 1            ;// line type count
  gem_intout[2] = 1            ;// line width count
  gem_intout[3] = 1            ;// marker type count
  gem_intout[4] = 1            ;// marker height count
  gem_intout[5] = 1            ;// text font count
  gem_intout[6] = 4            ;// colour count (4 planes = 16 colours)
  gem_intout[7] = 1            ;// fill type count
  gem_intout[8] = 1            ;// fill index count / pattern count
  gem_intout[9] = 3            ;// preloaded patterns
  gem_intout[10] = 0           ;// text rotation count
  gem_intout[11] = 3           ;// colour model (0=indexed, 3=RGB)
  gem_intout[12] = vdi_dev_w  ;// device width in pixels
  gem_intout[13] = vdi_dev_h  ;// device height in pixels
  gem_intout[14] = vdi_dev_w / 8 ;// device cell width in pixels
  gem_intout[15] = vdi_dev_h / 16 ;// device cell height in pixels
  gem_intout[16] = 0           ;// x dpi
  gem_intout[17] = 0           ;// y dpi
  gem_intout[18] = 1           ;// text effects
  gem_intout[19] = 0           ;// min character width
  gem_intout[20] = 0           ;// max character width
  gem_intout[21] = 0           ;// min character height (below baseline)
  gem_intout[22] = 0           ;// max character height (above baseline)
  gem_intout[23] = 0           ;// min kerning offset
  gem_intout[24] = 0           ;// max kerning offset
  gem_intout[25] = 0           ;// number of font entries
  ctx[0] = vdi_handle;
	return ;
}

// Close workstation
void vdi_clsvwk() {
  vdi_handle = -1;
  ctx[0] = 1;
	return ;
}

// Clear workstation (fill with colour index 0)
void vdi_clrwk() {
  ctx[0] = 1;
	return ;
}

// Update workstation (flush pending drawing)
void vdi_updwk() {
  ctx[0] = 1;
	return ;
}

// v_pline - Draw polyline (series of connected points)
// ptsin = array of (x,y) coordinate pairs, count in ptsin_count
// Attributes: line type, line width, line colour
void vdi_pline() {
  long pts, rp_ptr;
  pts = gem_control[2];
  if( pts < 2 ) { pts = 2;}
  rp_ptr = gem_GetVDIRastPort();
  if( rp_ptr != 0) {
    struct RastPort *rp = (struct RastPort *)rp_ptr;
    extern long vdi_line_color;
    extern long vdi_wr_mode;
    extern long vdi_line_type;
    extern long vdi_line_pats[6];
    extern long vdi_pen_map[16];
    extern long gem_ptrin[16];
    int pen = (int)vdi_line_color;
    int j;
    if( pen < 0) { pen = 0;} if( pen > 15) { pen = 15;}
    SetAPen(rp, vdi_pen_map[pen]);
    switch ((int)vdi_wr_mode) {
      case 1: SetDrMd(rp, JAM1); break;
      case 2: SetDrMd(rp, JAM2); break;
      case 3: SetDrMd(rp, INVERS_XOR); break;
      default: SetDrMd(rp, JAM1); break;
    }
    { int lt = (int)vdi_line_type;
      if( lt >= 0 && lt < 6) { SetDrPt(rp, vdi_line_pats[lt]);}
    }
    Move(rp, (long)gem_ptrin[0], (long)gem_ptrin[1]);
    for ( j = 1; j < (int)pts; j = j + 1) {
      Draw(rp, (long)gem_ptrin[j * 2], (long)gem_ptrin[j * 2 + 1]);
    }
  }
  vdi_cur_x = gem_ptrin[(pts - 1) * 2];
  vdi_cur_y = gem_ptrin[(pts - 1) * 2 + 1];
  ctx[0] = 1;
	return ;
}

// v_pmarker - Draw marker symbols at each point
// ptsin = array of (x,y) coordinate pairs
void vdi_pmarker() {
  long pts, rp_ptr;
  pts = gem_control[2];
  if( pts > 0) {
    rp_ptr = gem_GetVDIRastPort();
    if( rp_ptr != 0) {
      struct RastPort *rp = (struct RastPort *)rp_ptr;
      extern long vdi_marker_color;
      extern long vdi_marker_height;
      extern long vdi_pen_map[16];
      extern long gem_ptrin[16];
      int pen = (int)vdi_marker_color;
      int j, sz = (int)vdi_marker_height / 2;
      if( pen < 0) { pen = 0;} if( pen > 15) { pen = 15;}
      SetAPen(rp, vdi_pen_map[pen]);
      for ( j = 0; j < (int)pts; j = j + 1) {
        long mx = gem_ptrin[j * 2], my = gem_ptrin[j * 2 + 1];
        Move(rp, mx - sz, my); Draw(rp, mx + sz, my);
        Move(rp, mx, my - sz); Draw(rp, mx, my + sz);
      }
    }
    vdi_cur_x = gem_ptrin[(pts - 1) * 2];
    vdi_cur_y = gem_ptrin[(pts - 1) * 2 + 1];
  }
  ctx[0] = 1;
	return ;
}

// v_gtext - Draw graphics text at position
// Position in ptsin[0], ptsin[1]; text in intin (null-terminated)
void vdi_gtext() {
  long rp_ptr, len;
  rp_ptr = gem_GetVDIRastPort();
  len = gem_control[0];
  if( len > 64 ) { len = 64;}
  if( rp_ptr != 0) {
    struct RastPort *rp = (struct RastPort *)rp_ptr;
    extern long vdi_text_color;
    extern long vdi_pen_map[16];
    extern long gem_ptrin[16];
    extern long gem_intin[128];
    char txt[65];
    int pen = (int)vdi_text_color;
    int k, tlen = (int)len;
    if( pen < 0) { pen = 0;} if( pen > 15) { pen = 15;}
    SetAPen(rp, vdi_pen_map[pen]);
    SetDrMd(rp, JAM2);
    for ( k = 0; k < tlen; k = k + 1) {
      unsigned char ch = (unsigned char)((long)gem_intin[k]);
      if( ch == 0) { break;}
      txt[k] = ch;
    }
    txt[k] = 0;
    Move(rp, (long)gem_ptrin[0], (long)gem_ptrin[1]);
    Text(rp, txt, k);
  }
  vdi_cur_x = gem_ptrin[0];
  vdi_cur_y = gem_ptrin[1];
  ctx[0] = 1;
	return ;
}

// v_fillarea - Draw filled polygon
// ptsin = array of vertex (x,y) coordinates
void vdi_fillarea() {
  long pts, rp_ptr;
  pts = gem_control[2];
  if( pts >= 3) {
    rp_ptr = gem_GetVDIRastPort();
    if( rp_ptr != 0) {
      struct RastPort *rp = (struct RastPort *)rp_ptr;
      extern long vdi_fill_color;
      extern long vdi_pen_map[16];
      extern long gem_ptrin[16];
      int pen = (int)vdi_fill_color;
      int j, n = (int)pts;
      if( pen < 0) { pen = 0;} if( pen > 15) { pen = 15;}
      SetAPen(rp, vdi_pen_map[pen]);
      Move(rp, (long)gem_ptrin[0], (long)gem_ptrin[1]);
      for ( j = 1; j < n; j = j + 1) {
        Draw(rp, (long)gem_ptrin[j * 2], (long)gem_ptrin[j * 2 + 1]);
      }
      Draw(rp, (long)gem_ptrin[0], (long)gem_ptrin[1]);
    }
    vdi_cur_x = gem_ptrin[(pts - 1) * 2];
    vdi_cur_y = gem_ptrin[(pts - 1) * 2 + 1];
  }
  ctx[0] = 1;
	return ;
}

// v_bar - Draw filled rectangle (bar)
// ptsin[0..1] = top-left, ptsin[2..3] = bottom-right
void vdi_bar() {
  long rp_ptr;
  rp_ptr = gem_GetVDIRastPort();
  if( rp_ptr != 0) {
    struct RastPort *rp = (struct RastPort *)rp_ptr;
    extern long vdi_fill_color;
    extern long vdi_pen_map[16];
    extern long gem_ptrin[16];
    int pen = (int)vdi_fill_color;
    if( pen < 0) { pen = 0;} if( pen > 15) { pen = 15;}
    SetAPen(rp, vdi_pen_map[pen]);
    RectFill(rp, (long)gem_ptrin[0], (long)gem_ptrin[1],
                 (long)gem_ptrin[2], (long)gem_ptrin[3]);
  }
  ctx[0] = 1;
	return ;
}

// v_circle - Draw circle
// ptsin[0], ptsin[1] = centre; intin[0] = radius
void vdi_circle() {
  long rp_ptr;
  rp_ptr = gem_GetVDIRastPort();
  if( rp_ptr != 0) {
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
    if( pen < 0) { pen = 0;} if( pen > 15) { pen = 15;}
    SetAPen(rp, vdi_pen_map[pen]);
    switch ((int)vdi_wr_mode) {
      case 1: SetDrMd(rp, JAM1); break;
      case 2: SetDrMd(rp, JAM2); break;
      case 3: SetDrMd(rp, INVERS_XOR); break;
      default: SetDrMd(rp, JAM1); break;
    }
    { int lt = (int)vdi_line_type;
      if( lt >= 0 && lt < 6) { SetDrPt(rp, vdi_line_pats[lt]);}
    }
    for ( angle = 0; angle <= 360; angle = angle + 15) {
      double rad = angle * 3.14159265 / 180.0;
      long px = cx + (long)(r * cos(rad));
      long py = cy + (long)(r * sin(rad));
      if( angle == 0) { Move(rp, px, py);}
      else { Draw(rp, px, py);}
    }
  }
  vdi_cur_x = gem_ptrin[0] + gem_intin[0];
  vdi_cur_y = gem_ptrin[1];
  ctx[0] = 1;
	return ;
}

// v_ellipse - Draw ellipse
// ptsin[0], ptsin[1] = centre; intin[0] = x radius; intin[1] = y radius
void vdi_ellipse() {
  long rp_ptr;
  rp_ptr = gem_GetVDIRastPort();
  if( rp_ptr != 0) {
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
    if( pen < 0) { pen = 0;} if( pen > 15) { pen = 15;}
    SetAPen(rp, vdi_pen_map[pen]);
    switch ((int)vdi_wr_mode) {
      case 1: SetDrMd(rp, JAM1); break;
      case 2: SetDrMd(rp, JAM2); break;
      case 3: SetDrMd(rp, INVERS_XOR); break;
      default: SetDrMd(rp, JAM1); break;
    }
    { int lt = (int)vdi_line_type;
      if( lt >= 0 && lt < 6) { SetDrPt(rp, vdi_line_pats[lt]);}
    }
    for ( angle = 0; angle <= 360; angle = angle + 15) {
      double rad = angle * 3.14159265 / 180.0;
      long px = cx + (long)(rx * cos(rad));
      long py = cy + (long)(ry * sin(rad));
      if( angle == 0) { Move(rp, px, py);}
      else { Draw(rp, px, py);}
    }
  }
  vdi_cur_x = gem_ptrin[0] + gem_intin[0];
  vdi_cur_y = gem_ptrin[1];
  ctx[0] = 1;
	return ;
}

// v_ellarc - Draw elliptical arc
// ptsin[0..1] = centre; intin[0] = x radius; intin[1] = y radius
// intin[2] = start angle; intin[3] = end angle (in tenths of degrees)
void vdi_ellarc() {
  long rp_ptr;
  rp_ptr = gem_GetVDIRastPort();
  if( rp_ptr != 0) {
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
    if( pen < 0) { pen = 0;} if( pen > 15) { pen = 15;}
    SetAPen(rp, vdi_pen_map[pen]);
    switch ((int)vdi_wr_mode) {
      case 1: SetDrMd(rp, JAM1); break;
      case 2: SetDrMd(rp, JAM2); break;
      case 3: SetDrMd(rp, INVERS_XOR); break;
      default: SetDrMd(rp, JAM1); break;
    }
    { int lt = (int)vdi_line_type;
      if( lt >= 0 && lt < 6) { SetDrPt(rp, vdi_line_pats[lt]);}
    }
    for ( a = sa; a <= ea; a = a + 15) {
      double rad = a * 3.14159265 / 180.0;
      long px = cx + (long)(rx * cos(rad));
      long py = cy + (long)(ry * sin(rad));
      if( a == sa) { Move(rp, px, py);}
      else { Draw(rp, px, py);}
    }
  }
  ctx[0] = 1;
	return ;
}

// v_ellpie - Draw elliptical pie slice
// same params as ellarc but filled to centre
void vdi_ellpie() {
  long rp_ptr;
  rp_ptr = gem_GetVDIRastPort();
  if( rp_ptr != 0) {
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
    if( pen < 0) { pen = 0;} if( pen > 15) { pen = 15;}
    SetAPen(rp, vdi_pen_map[pen]);
    Move(rp, cx, cy);
    for ( a = sa; a <= ea; a = a + 15) {
      double rad = a * 3.14159265 / 180.0;
      Draw(rp, cx + (long)(rx * cos(rad)), cy + (long)(ry * sin(rad)));
    }
    Draw(rp, cx, cy);
  }
  ctx[0] = 1;
	return ;
}

// v_arc - Draw circular arc
// ptsin[0..1] = centre; intin[0] = radius
// intin[1] = start angle; intin[2] = end angle (in tenths of degrees)
void vdi_arc() {
  long rp_ptr;
  rp_ptr = gem_GetVDIRastPort();
  if( rp_ptr != 0) {
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
    if( pen < 0) { pen = 0;} if( pen > 15) { pen = 15;}
    SetAPen(rp, vdi_pen_map[pen]);
    switch ((int)vdi_wr_mode) {
      case 1: SetDrMd(rp, JAM1); break;
      case 2: SetDrMd(rp, JAM2); break;
      case 3: SetDrMd(rp, INVERS_XOR); break;
      default: SetDrMd(rp, JAM1); break;
    }
    { int lt = (int)vdi_line_type;
      if( lt >= 0 && lt < 6) { SetDrPt(rp, vdi_line_pats[lt]);}
    }
    for ( a = sa; a <= ea; a = a + 15) {
      double rad = a * 3.14159265 / 180.0;
      long px = cx + (long)(r * cos(rad));
      long py = cy + (long)(r * sin(rad));
      if( a == sa) { Move(rp, px, py);}
      else { Draw(rp, px, py);}
    }
  }
  ctx[0] = 1;
	return ;
}

// v_pieslice - Draw circular pie slice
// same as arc but filled to centre
void vdi_pieslice() {
  long rp_ptr;
  rp_ptr = gem_GetVDIRastPort();
  if( rp_ptr != 0) {
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
    if( pen < 0) { pen = 0;} if( pen > 15) { pen = 15;}
    SetAPen(rp, vdi_pen_map[pen]);
    Move(rp, cx, cy);
    for ( a = sa; a <= ea; a = a + 15) {
      double rad = a * 3.14159265 / 180.0;
      Draw(rp, cx + (long)(r * cos(rad)), cy + (long)(r * sin(rad)));
    }
    Draw(rp, cx, cy);
  }
  ctx[0] = 1;
	return ;
}

// v_rbox - Draw rounded rectangle (outline)
// ptsin[0..1] = top-left; ptsin[2..3] = bottom-right
// intin[0] = corner radius
void vdi_rbox() {
  long rp_ptr;
  rp_ptr = gem_GetVDIRastPort();
  if( rp_ptr != 0) {
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
    if( cr < 1) { cr = 1;}
    if( pen < 0) { pen = 0;} if( pen > 15) { pen = 15;}
    SetAPen(rp, vdi_pen_map[pen]);
    switch ((int)vdi_wr_mode) {
      case 1: SetDrMd(rp, JAM1); break;
      case 2: SetDrMd(rp, JAM2); break;
      case 3: SetDrMd(rp, INVERS_XOR); break;
      default: SetDrMd(rp, JAM1); break;
    }
    { int lt = (int)vdi_line_type;
      if( lt >= 0 && lt < 6) { SetDrPt(rp, vdi_line_pats[lt]);}
    }
    for ( a = 180; a <= 270; a = a + 15) {
      double rad = a * 3.14159265 / 180.0;
      long px = (x1 + cr) + (long)(cr * cos(rad));
      long py = (y1 + cr) + (long)(cr * sin(rad));
      if( a == 180) { Move(rp, px, py);}
      else { Draw(rp, px, py);}
    }
    for ( a = 270; a <= 360; a = a + 15) {
      double rad = a * 3.14159265 / 180.0;
      long px = (x2 - cr) + (long)(cr * cos(rad));
      long py = (y1 + cr) + (long)(cr * sin(rad));
      Draw(rp, px, py);
    }
    for ( a = 0; a <= 90; a = a + 15) {
      double rad = a * 3.14159265 / 180.0;
      long px = (x2 - cr) + (long)(cr * cos(rad));
      long py = (y2 - cr) + (long)(cr * sin(rad));
      Draw(rp, px, py);
    }
    for ( a = 90; a <= 180; a = a + 15) {
      double rad = a * 3.14159265 / 180.0;
      long px = (x1 + cr) + (long)(cr * cos(rad));
      long py = (y2 - cr) + (long)(cr * sin(rad));
      Draw(rp, px, py);
    }
    Draw(rp, (x1 + cr), y1);
  }
  ctx[0] = 1;
	return ;
}

// v_rfbox - Draw filled rounded rectangle
// same as rbox but filled
void vdi_rfbox() {
  long rp_ptr;
  rp_ptr = gem_GetVDIRastPort();
  if( rp_ptr != 0) {
    struct RastPort *rp = (struct RastPort *)rp_ptr;
    extern long vdi_fill_color;
    extern long vdi_pen_map[16];
    extern long gem_ptrin[16];
    long x1 = gem_ptrin[0], y1 = gem_ptrin[1];
    long x2 = gem_ptrin[2], y2 = gem_ptrin[3];
    int pen = (int)vdi_fill_color;
    if( pen < 0) { pen = 0;} if( pen > 15) { pen = 15;}
    SetAPen(rp, vdi_pen_map[pen]);
    RectFill(rp, x1, y1, x2, y2);
  }
  ctx[0] = 1;
	return ;
}

// v_justified - Draw justified text
// Position in ptsin[0..1]; text in intin; intin_len in control[0]
// intin[1] = word spacing; intin[2] = char spacing (in 1/8 em)
void vdi_justified() {
  long rp_ptr, len;
  rp_ptr = gem_GetVDIRastPort();
  len = gem_control[0];
  if( len > 64 ) { len = 64;}
  if( rp_ptr != 0) {
    struct RastPort *rp = (struct RastPort *)rp_ptr;
    extern long vdi_text_color;
    extern long vdi_pen_map[16];
    extern long gem_ptrin[16];
    extern long gem_intin[128];
    char txt[65];
    int pen = (int)vdi_text_color;
    int k, tlen = (int)len;
    if( pen < 0) { pen = 0;} if( pen > 15) { pen = 15;}
    SetAPen(rp, vdi_pen_map[pen]);
    SetDrMd(rp, JAM2);
    for ( k = 0; k < tlen; k = k + 1) {
      unsigned char ch = (unsigned char)((long)gem_intin[k]);
      if( ch == 0) { break;}
      txt[k] = ch;
    }
    txt[k] = 0;
    Move(rp, (long)gem_ptrin[0], (long)gem_ptrin[1]);
    Text(rp, txt, k);
  }
  vdi_cur_x = gem_ptrin[0];
  vdi_cur_y = gem_ptrin[1];
  ctx[0] = 1;
	return ;
}

// v_cellarray - Draw rectangular block of pixel cells
// ptsin[0..1] = top-left; ptsin[2..3] = bottom-right
// intin contains pixel colours
void vdi_cellarray() {
  long w, h;
  w = gem_ptrin[2] - gem_ptrin[0];
  if( w < 0) {
    w = -w;
  }
  w = w + 1;
  h = gem_ptrin[3] - gem_ptrin[1];
  if( h < 0) {
    h = -h;
  }
  h = h + 1;
  vdi_cur_x = gem_ptrin[2];
  vdi_cur_y = gem_ptrin[3];
  ctx[0] = 1;
	return ;
}

// v_bezier - Draw Bezier curve
// ptsin = control points; intin[0] = number of points
void vdi_bezier() {
  ctx[0] = 1;
	return ;
}

// vq_color - Inquire colour representation
// intin[0] = colour index; intin[1] = flag (0=get RGB)
// Returns RGB in intout[0..2]
void vdi_qcolor() {
  long idx;
  idx = gem_intin[0];
  if( (- (idx >= 0 )& idx )<= 15) {
    gem_intout[0] = vdi_rgb[idx * 3] * (int) 1000 / 255;
    gem_intout[1] = vdi_rgb[idx * 3 + 1] * (int) 1000 / 255;
    gem_intout[2] = vdi_rgb[idx * 3 + 2] * (int) 1000 / 255;
  } else {
    gem_intout[0] = 0; gem_intout[1] = 0; gem_intout[2] = 0;
  }
  ctx[0] = 1;
	return ;
}

// vq_curpos - Inquire graphics cursor position
void vdi_qcurpos() {
  gem_intout[0] = vdi_cur_x;
  gem_intout[1] = vdi_cur_y;
  ctx[0] = 1;
	return ;
}

// vq_contxt - Inquire current context (VDI attributes)
void vdi_qcontxt() {
  gem_intout[0] = vdi_line_type;
  gem_intout[1] = vdi_line_width;
  gem_intout[2] = vdi_line_color;
  gem_intout[3] = vdi_marker_type;
  gem_intout[4] = vdi_marker_height;
  gem_intout[5] = vdi_marker_color;
  gem_intout[6] = vdi_text_font;
  gem_intout[7] = vdi_text_color;
  gem_intout[8] = vdi_fill_type;
  gem_intout[9] = vdi_fill_index;
  gem_intout[10] = vdi_fill_color;
  gem_intout[11] = vdi_wr_mode;
  ctx[0] = 1;
	return ;
}

// vq_extnd - Inquire extended device capabilities
// intin[0] = device handle (0 = current)
void vdi_qextnd() {
  gem_intout[0] = 0 ;// device id
  gem_intout[1] = 0 ;// flags
  gem_intout[2] = 0 ;// colour capabilities
  gem_intout[3] = 0 ;// tiling capabilities
  gem_intout[4] = 0 ;// reserved
  gem_intout[5] = 0;
  gem_intout[6] = 0;
  gem_intout[7] = 0;
  gem_intout[8] = 0;
  gem_intout[9] = 0;
  gem_intout[10] = 0;
  gem_intout[11] = 0;
  ctx[0] = 1;
	return ;
}

// vq_cellht - Inquire cell height
void vdi_qcellht() {
  gem_intout[0] = vdi_work_h / 24;
  ctx[0] = 1;
	return ;
}

// vq_cellwd - Inquire cell width
void vdi_qcellwd() {
  gem_intout[0] = vdi_work_w / 40;
  ctx[0] = 1;
	return ;
}

// vq_chcells - Inquire number of character cells
void vdi_qchcells() {
  gem_intout[0] = 40 ;// columns
  gem_intout[1] = 25 ;// rows
  ctx[0] = 1;
	return ;
}

// vq_vgd - Inquire VGD (Virtual Graphics Device) capabilities
void vdi_qvgd() {
  gem_intout[0] = 1 ;// VDI version
  gem_intout[1] = 0 ;// sub-version
  gem_intout[2] = 1 ;// colour (0=mono, 1=colour)
  gem_intout[3] = vdi_n_planes;
  gem_intout[4] = vdi_dev_w;
  gem_intout[5] = vdi_dev_h;
  gem_intout[6] = vdi_dev_w * vdi_n_planes / 8 ;// bytes per line
  gem_intout[7] = 0 ;// screen base (emulated)
  ctx[0] = 1;
	return ;
}

// vq_key_s - Inquire key shift status (always returns 0 in emulator)
void vdi_qkey_s() {
  gem_intout[0] = 0;
  ctx[0] = 1;
	return ;
}

// v_opnwk — alternate open workstation (function 2)
void vdi_opnwk() {
  vdi_opnvwk();
	return ;
}

// Set VDI attribute functions based on intin values
// vsl_type (line type), vsl_width (line width), vsl_color (line colour)
// vsf_type (fill type), vsf_index (fill index), vsf_color (fill colour)
// etc.
void vdi_set_line_type() {
  vdi_line_type = gem_intin[0];
  ctx[0] = 1;
	return ;
}
void vdi_set_line_width() {
  vdi_line_width = gem_intin[0];
  ctx[0] = 1;
	return ;
}
void vdi_set_line_color() {
  vdi_line_color = gem_intin[0];
  ctx[0] = 1;
	return ;
}
void vdi_set_fill_type() {
  vdi_fill_type = gem_intin[0];
  if( gem_intin[1] != 0 ) { vdi_fill_index = gem_intin[1];}
  ctx[0] = 1;
	return ;
}
void vdi_set_fill_index() {
  vdi_fill_index = gem_intin[0];
  ctx[0] = 1;
	return ;
}
void vdi_set_fill_color() {
  vdi_fill_color = gem_intin[0];
  ctx[0] = 1;
	return ;
}
void vdi_set_marker_type() {
  vdi_marker_type = gem_intin[0];
  ctx[0] = 1;
	return ;
}
void vdi_set_marker_height() {
  vdi_marker_height = gem_intin[0];
  ctx[0] = 1;
	return ;
}
void vdi_set_marker_color() {
  vdi_marker_color = gem_intin[0];
  ctx[0] = 1;
	return ;
}
void vdi_set_text_font() {
  vdi_text_font = gem_intin[0];
  ctx[0] = 1;
	return ;
}
void vdi_set_text_color() {
  vdi_text_color = gem_intin[0];
  ctx[0] = 1;
	return ;
}
void vdi_set_text_rotation() {
  vdi_text_rotation = gem_intin[0];
  ctx[0] = 1;
	return ;
}
void vdi_set_writing_mode() {
  vdi_wr_mode = gem_intin[0];
  ctx[0] = 1;
	return ;
}
void vdi_set_clip_rect() {
  vdi_clip_x = gem_intin[0];
  vdi_clip_y = gem_intin[1];
  vdi_clip_w = gem_intin[2];
  vdi_clip_h = gem_intin[3];
  ctx[0] = 1;
	return ;
}
void vdi_set_clip_state() {
  // intin[0] = 0 (off), 1 (on)
  ctx[0] = 1;
	return ;
}
void vdi_set_curpos() {
  vdi_cur_x = gem_intin[0];
  vdi_cur_y = gem_intin[1];
  ctx[0] = 1;
	return ;
}

// VDI inquiry for fill area patterns
void vdi_inq_fill_pats() {
  gem_intout[0] = 3;
  ctx[0] = 1;
	return ;
}


// ---------------------------------------------------------------------------
// GEM VDI (Virtual Device Interface) dispatch
// Called via BIOS trap #2 with D0 = $C9
// Maps GEM VDI drawing calls to AmigaOS graphics.library
// ctx[1] = VDI function number
// ---------------------------------------------------------------------------
void gem_vdi_dispatch() {
  long fn;
  fn = ctx[1];

  switch( fn) {

  case 1  :// v_clsvwk - Close workstation
    vdi_clsvwk();

  	break;
  case 2  :// v_opnwk - Open workstation
    vdi_opnwk();
  	break;
  case 5  :// v_opnvwk - Open virtual workstation
    vdi_opnvwk();
  	break;
  case 6  :// v_clsvwk - Close virtual workstation
    vdi_clsvwk();
  	break;
  case 7  :// v_clrwk - Clear workstation
    vdi_clrwk();
  	break;
  case 8  :// v_updwk - Update workstation
    vdi_updwk();

  // Drawing primitives
  	break;
  case 11 :// v_pline
    vdi_pline();
  	break;
  case 12 :// v_pmarker
    vdi_pmarker();
  	break;
  case 13 :// v_gtext
    vdi_gtext();
  	break;
  case 14 :// v_fillarea
    vdi_fillarea();
  	break;
  case 15 :// v_ellipse
    vdi_ellipse();
  	break;
  case 16 :// v_arc
    vdi_arc();
  	break;
  case 17 :// v_pieslice
    vdi_pieslice();
  	break;
  case 18 :// v_circle
    vdi_circle();
  	break;
  case 19 :// v_ellarc
    vdi_ellarc();
  	break;
  case 20 :// v_ellpie
    vdi_ellpie();
  	break;
  case 21 :// v_rbox
    vdi_rbox();
  	break;
  case 22 :// v_rfbox
    vdi_rfbox();
  	break;
  case 23 :// v_bar
    vdi_bar();
  	break;
  case 24 :// v_justified
    vdi_justified();

  // Attributes
  	break;
  case 32 :// vsl_type
    vdi_set_line_type();
  	break;
  case 33 :// vsl_width
    vdi_set_line_width();
  	break;
  case 34 :// vsl_color
    vdi_set_line_color();
  	break;
  case 35 :// vsf_type
    vdi_set_fill_type();
  	break;
  case 36 :// vsf_index
    vdi_set_fill_index();
  	break;
  case 37 :// vsf_color
    vdi_set_fill_color();
  	break;
  case 38 :// vsm_type
    vdi_set_marker_type();
  	break;
  case 39 :// vsm_height
    vdi_set_marker_height();
  	break;
  case 40 :// vsm_color
    vdi_set_marker_color();
  	break;
  case 41 :// vst_font
    vdi_set_text_font();
  	break;
  case 42 :// vst_color
    vdi_set_text_color();
  	break;
  case 43 :// vst_rotation
    vdi_set_text_rotation();
  	break;
  case 44 :// vswr_mode
    vdi_set_writing_mode();
  	break;
  case 45 :// vsl_pattern - line pattern (obsolete)
    ctx[0] = 1;
  	break;
  case 46 :// vs_clip - set clipping rectangle
    vdi_set_clip_rect();
  	break;
  case 47 :// vs_clip - set clipping state (0=off, 1=on)
    vdi_set_clip_state();
  	break;
  case 48 :// vs_curpos - set graphics cursor position
    vdi_set_curpos();

  // Inquiry
  	break;
  case 10 :// vq_color
    vdi_qcolor();
  	break;
  case 26 :// vq_curpos
    vdi_qcurpos();
  	break;
  case 27 :// vq_contxt
    vdi_qcontxt();
  	break;
  case 30 :// vq_extnd
    vdi_qextnd();
  	break;
  case 31 :// vq_key_s
    vdi_qkey_s();
  	break;
  case 36 :// vq_cellht
    vdi_qcellht();
  	break;
  case 37 :// vq_cellwd
    vdi_qcellwd();
  	break;
  case 38 :// vq_chcells
    vdi_qchcells();
  	break;
  case 120 :// vq_vgd
    vdi_qvgd();
  	break;

  case 100 :// v_opnvwk (alternate)
    vdi_opnvwk();

  	break;
  case 101 :// v_cellarray
    vdi_cellarray();
  	break;
  case 107 :// v_bezier
    vdi_bezier();
  	break;
  case 109 :// v_inq_fill_pats - inquire fill patterns
    vdi_inq_fill_pats();
  	break;

  default:
    // All other VDI functions return success
    ctx[0] = 1;
  	break;
  }
	return ;
}

// Build and register an Atari ST 8x16 system font
void gem_init_font_8x16() {
  long i;

  // Fill width table (all chars are 8 pixels wide)
  for ( i = 0 ; i <= 255; i = i + 1) {
    gem_font_width_8x16[i] = 8;
  }

  // Fill location table (char N starts at N*16 bytes)
  for ( i = 0 ; i <= 255; i = i + 1) {
    gem_font_loc_8x16[i * 2] =  return (unsigned char)v; ;
    gem_font_loc_8x16[i * 2 + 1] =  return (unsigned char)v; ;
  }

  // Atari ST 8x16 system font data (256 glyphs, 16 bytes each)
  // Bit 7 = leftmost pixel, bit 0 = rightmost pixel
  // Rows top to bottom; baseline at row 12 (3 rows below for descenders)
  
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
  {0x00,0x00,0x00,0x40,0x20,0x10,0x10,0x08,0x08,0x04,0x04,0x02,0x00,0x00,0x00,0x00}, /* '' */
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
  ;
  // Allocate and populate TextFont structure
  gem_font_8x16 =  return (struct TextFont*)v; ;
  if( gem_font_8x16) {
     struct TextFont *tf = (struct TextFont *)gem_font_8x16; tf->tf_Message.mn_ReplyPort=NULL; tf->tf_Message.mn_Length=sizeof(struct TextFont); tf->tf_YSize=16; tf->tf_Style=0; tf->tf_Flags=0; tf->tf_XSize=8; tf->tf_Baseline=13; tf->tf_BoldSmear=0; tf->tf_Accessors=0; tf->tf_LoChar=0; tf->tf_HiChar=255; tf->tf_CharData=(APTR)gem_font_data_8x16; tf->tf_Modulo=8; tf->tf_CharLoc=(APTR)gem_font_loc_8x16; tf->tf_CharSpace=(APTR)gem_font_width_8x16; tf->tf_CharKern=NULL; ;
    AddFont(gem_font_8x16 );
    gem_font_8x16_registered = 1;
  }
	return ;
}

// Build and register Atari ST 8x8 system font
void gem_init_font_8x8() {
  long i, c, r;

  for ( i = 0 ; i <= 255; i = i + 1) {
    gem_font_width_8x8[i] = 8;
  }

  for ( i = 0 ; i <= 255; i = i + 1) {
    gem_font_loc_8x8[i] =  return (unsigned char)v; ;
  }

  // Downsample 8x16 to 8x8 by taking every 2nd row
  for ( c = 0 ; c <= 255; c = c + 1) {
    for ( r = 0 ; r <= 7; r = r + 1) {
      gem_font_data_8x8[c * 8 + r] = gem_font_data_8x16[(c * 16 + r )* 2];
    }
  }

  gem_font_8x8 =  return (struct TextFont*)v; ;
  if( gem_font_8x8) {
     struct TextFont *tf = (struct TextFont *)gem_font_8x8; tf->tf_Message.mn_ReplyPort=NULL; tf->tf_Message.mn_Length=sizeof(struct TextFont); tf->tf_YSize=8; tf->tf_Style=0; tf->tf_Flags=0; tf->tf_XSize=8; tf->tf_Baseline=7; tf->tf_BoldSmear=0; tf->tf_Accessors=0; tf->tf_LoChar=0; tf->tf_HiChar=255; tf->tf_CharData=(APTR)gem_font_data_8x8; tf->tf_Modulo=8; tf->tf_CharLoc=(APTR)gem_font_loc_8x8; tf->tf_CharSpace=(APTR)gem_font_width_8x8; tf->tf_CharKern=NULL; ;
    AddFont(gem_font_8x8 );
    gem_font_8x8_registered = 1;
  }
	return ;
}

// Initialize all Atari ST system fonts
void gem_init_fonts() {
  gem_font_8x16_registered = 0;
  gem_font_8x8_registered = 0;
  gem_init_font_8x16();
  gem_init_font_8x8();
	return ;
}


// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------
int main(int argc, char** argv) {
  long i; char filename[128]; long addr;
  char* arg2=NULL;
main_argc = argc;
main_argv = argv;
try {
	new_base();
	new_base2();
	new_Mem();
	new_FastMem();
	new_dos();
	new_timer();
	new_dos2();
	new_monitor();
	new_dos3();
	new_gfxbase();
	new_graphics();

  // Initialize handle table (GEMDOS handle -> AmigaOS BPTR mapping)
  gem_handles[0] = (long) Input();
  gem_handles[1] = (long) Output();
  gem_handles[2] = (long) Output();
  for ( i = 3 ; i <= 15; i = i + 1) {
    gem_handles[i] = 0;
  }

  gem_dta = 0;
  gem_drv = 0;
  gem_search_lock = 0;
  bios_kb_shift = 0;
  gem_scrn_w = 640;
  gem_scrn_h = 400;
  gem_aes_id = 0;
  gem_app_count = 0;
  gem_menu_count = 0;
  gem_menu_bar_visible = 0;
  gem_menu_active_app = 0;
  gem_menu_pending_action = 0;
  gem_menu_pending_item = 0;
  gem_menu_pending_tree = 0;
  gem_menu_pending_app = 0;
  gem_form_active = -1;
  gem_scrap_len = 0;
  gem_msg_head = 0;
  gem_msg_tail = 0;
  gem_mouse_x = 320;
  gem_mouse_y = 200;
  gem_mouse_buttons = 0;
  gem_mouse_kstate = 0;
  gem_mouse_shape = 2;
  gem_mouse_visible = 1;
  gem_mouse_user_active = 0;
  gem_graf_wk_handle = 1;
  gem_graf_char_w = 8;
  gem_graf_char_h = 16;
  gem_graf_arrow_mode = 1;
  gem_graf_accel_key = 0;
  gem_mouse_init();
  gem_init_fonts();
  gem_init_gadtools();
  vdi_init_rgb();
  vdi_init_line_pats();
  vdi_handle = -1;

  if( extern int __main_argc; return __main_argc;> 1) {
    arg2 = extern char **__main_argv; return (unsigned char*)__main_argv[(int)1 ];;
    i = 0;
    while( (- (arg2[i] != 0 )& i )< 127) {
      filename[i] = arg2[i];
      i = i + 1;
    }
    filename[i] = 0;

    PutStr("Loading: " );
    PutStr(filename );
    PutStr("\n" );

    addr = load_prg(filename);
    if( addr) {
      PutStr("Loaded at $" );
      Printf("%%lx\n" ,(ULONG) addr );
      PutStr("\n" );
    } else {
      PutStr("Failed to load PRG\n" );
    }
  } else {
    PutStr("Sake Atari Emulator\n" );
    PutStr("Testing...\n" );

    // Test Cconws ($09): write a string via direct call
    ctx[8] = (long)  return (unsigned char*)v; ;
    temp_string[0] = 72; temp_string[1] = 101; temp_string[2] = 108; temp_string[3] = 108; temp_string[4] = 111;
    temp_string[5] = 0;
    gemdos_cconws();

    // Test Dgetdrv ($0F)
    gemdos_dgetdrv();
    PutStr("Default drive: " );
    Printf("%%ld\n" ,(ULONG) ctx[0] );

    // Test Tgetdate ($31) and Tgettime ($33)
    gemdos_tgetdate();
    gemdos_tgettime();
    PutStr("Date: " );
    Printf("%%ld" ,(ULONG) ctx[0] );
    PutStr(", Time: " );
    Printf("%%ld\n" ,(ULONG) ctx[0] );

    PutStr("\nAll GEMDOS functions completed\n" );
  }
} catch(...) {}
try {
	end_graphics();
	end_dos2();
	end_dos();
	end_FastMem();
	end_Mem();
} catch(...) {}
	if (exception==-1) return (int) (long) exceptionInfo; /*finish the CleanUp() call*/
	return 0;
}


// ---------------------------------------------------------------------------
// Cursconf ($07) - Cursor configuration
// D1 = function (0=disable, 1=enable, 2=get status)
// ---------------------------------------------------------------------------
void xbios_cursconf() {
  ctx[0] = E_OK;
	return ;
}


// ---------------------------------------------------------------------------
// Appl_init ($08) - Initialize application
// Returns application ID
// ---------------------------------------------------------------------------
void xbios_appl_init() {
  // Return a dummy application ID (1)
  ctx[0] = 1;
	return ;
}


// ---------------------------------------------------------------------------
// Physbase ($09) - Get physical screen base address
// Returns address of physical screen memory
// ---------------------------------------------------------------------------
void xbios_physbase() {
  // In emulator, no real ST screen memory
  // Return 0
  ctx[0] = 0;
	return ;
}


// ---------------------------------------------------------------------------
// Logbase ($0A) - Get logical screen base
// ---------------------------------------------------------------------------
void xbios_logbase() {
  // Same as Physbase in emulator
  ctx[0] = 0;
	return ;
}


// ---------------------------------------------------------------------------
// Getrez ($0B) - Get screen resolution
// 0=low (320x200), 1=medium (640x200), 2=high (640x400)
// ---------------------------------------------------------------------------
void xbios_getrez() {
  // Return low resolution
  ctx[0] = 0;
	return ;
}


// ---------------------------------------------------------------------------
// Setscreen ($0C) - Set screen parameters
// D1 = logbase, D2 = physbase, D3 = resolution
// ---------------------------------------------------------------------------
void xbios_setscreen() {
  ctx[0] = E_OK;
	return ;
}


// ---------------------------------------------------------------------------
// Setpalette ($0D) - Set full palette
// A0 = pointer to 16 color palette entries
// ---------------------------------------------------------------------------
void xbios_setpalette() {
  ctx[0] = E_OK;
	return ;
}


// ---------------------------------------------------------------------------
// Setcolor ($0E) - Set single color
// D1 = color index, D2 = color value
// ---------------------------------------------------------------------------
void xbios_setcolor() {
  ctx[0] = E_OK;
	return ;
}


// ---------------------------------------------------------------------------
// Floprd ($10) - Floppy read sectors
// D1 = drive, D2 = sector, D3 = count, A0 = buffer
// ---------------------------------------------------------------------------
void xbios_floprd() {
  ctx[0] = E_ERROR;
	return ;
}


// ---------------------------------------------------------------------------
// Flopwr ($11) - Floppy write sectors
// ---------------------------------------------------------------------------
void xbios_flopwr() {
  ctx[0] = E_ERROR;
	return ;
}


// ---------------------------------------------------------------------------
// Flopfmt ($12) - Floppy format
// ---------------------------------------------------------------------------
void xbios_flopfmt() {
  ctx[0] = E_ERROR;
	return ;
}


// ---------------------------------------------------------------------------
// Flopstatus ($13) - Floppy status
// ---------------------------------------------------------------------------
void xbios_flopstatus() {
  ctx[0] = 0;
	return ;
}


// ---------------------------------------------------------------------------
// Rsconf ($17) - RS232 configuration
// D1-D7 = RS232 parameters
// Returns old configuration
// ---------------------------------------------------------------------------
void xbios_rsconf() {
  ctx[0] = E_OK;
	return ;
}


// ---------------------------------------------------------------------------
// Keytbl ($18) - Get/set keyboard table
// D1 = table type, A0 = new table (or 0 to read)
// ---------------------------------------------------------------------------
void xbios_keytbl() {
  ctx[0] = 0;
	return ;
}


// ---------------------------------------------------------------------------
// Random ($19) - Get random number
// D0 = pseudo-random number
// ---------------------------------------------------------------------------
void xbios_random() {
  struct DateStamp now;
  DateStamp(& now );
  ctx[0] = (now.ds_Days + now.ds_Minute )* 60 + now.ds_Tick;
	return ;
}


// ---------------------------------------------------------------------------
// Cookieptr ($1E) - Get cookie jar pointer
// A0 = pointer to cookie jar (Atari ST cookie jar)
// ---------------------------------------------------------------------------
void xbios_cookieptr() {
  // Return 0 - no cookie jar in emulator
  ctx[0] = 0;
	return ;
}
