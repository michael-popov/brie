/* Based on ANSI C grammar as in https://www.quut.com/c/ANSI-C-grammar-l.html */

%e  1019
%p  2807
%n  371
%k  284
%a  1213
%o  1117

O   [0-7]
D   [0-9]
NZ  [1-9]
L   [a-zA-Z_]
A   [a-zA-Z_0-9]
H   [a-fA-F0-9]
HP  (0[xX])
E   ([Ee][+-]?{D}+)
P   ([Pp][+-]?{D}+)
FS  (f|F|l|L)
IS  (((u|U)(l|L|ll|LL)?)|((l|L|ll|LL)(u|U)?))
CP  (u|U|L)
SP  (u8|u|U|L)
ES  (\\(['"\?\\abfnrtv]|[0-7]{1,3}|x[a-fA-F0-9]+))
WS  [ \t\v\n\f]

%{
#include <stdio.h>
#include "briebase.tab.h"
%}

%option noyywrap nounput

/* generate a scanner that's thread safe */
%option reentrant

%%

"u8"                        { return U8; }
"u16"                       { return U16; }
"u32"                       { return U32; }
"i16"                       { return I16; }
"i32"                       { return I32; }
"i64"                       { return I64; }
"f32"                       { return F32; }
"f64"                       { return F64; }
"str"                       { return STRING; }
"wstr"                      { return WSTRING; }
"void"                      { return VOID; }

\"([^\\\"]|\\.)*\"          { return STRING_LITERAL; }
\'([^\\\']|\\.)*\'          { return STRING_LITERAL; }

{L}{A}*                     { return IDENTIFIER; }
@{L}{A}*                    { return FUNC_CONSTANT; }

{HP}{H}+{IS}?				{ return I_CONSTANT; }
[-]?{NZ}{D}*{IS}?			{ return I_CONSTANT; }
"0"{O}*{IS}?				{ return I_CONSTANT; }
{CP}?"'"([^'\\\n]|{ES})+"'" { return I_CONSTANT; }

[-]?{D}+{E}{FS}?            { return F_CONSTANT; }
[-]?{D}*"."{D}+{E}?{FS}?    { return F_CONSTANT; }
[-]?{D}+"."{E}?{FS}?        { return F_CONSTANT; }

#{NZ}{D}*					{ return FIXED_LENGTH; }
\*{NZ}{D}*					{ return ARRAY_SIZE; }

"="                         { return '='; }
":"                         { return ':'; }
";"                         { return ';'; }
","                         { return ','; }
"<"                         { return '<'; }
">"                         { return '>'; }
"\n"                        { return '\n'; }

{WS}+                       { /* whitespace separates tokens */ }
.                           { /* discard bad characters */ }

%%

class ScanCleaner
{
public:
    ScanCleaner(yyscan_t& scanner, YY_BUFFER_STATE& buffer)
	 : m_scanner(scanner), m_buffer(buffer) {}
    ~ScanCleaner();
private:
    yyscan_t& m_scanner;
    YY_BUFFER_STATE& m_buffer;
};

int briebase_callback(void* context, char *s, void (*f)(void*, int num, const char *))
{
	/* in the reentrant mode, we maintain our own scanner and its associated state */
	int i;
	yyscan_t scanner;
	YY_BUFFER_STATE buf;

	if ((i = yylex_init(&scanner)) != 0) {
		return -1;
    }

	/* read from a string rather than a stream */
	buf = yy_scan_string(s, scanner);

	const ScanCleaner cleaner(scanner, buf);

	/* Each time yylex finds a word, it returns nonzero.
	   It resumes where it left off when we call it again */
	while ((i = yylex(scanner)) > 0)
	{
		/* call the user supplied function f with yytext of the match */
		if (i != STRING_LITERAL) {
		    f(context, i, yyget_text(scanner));
		} else {
			char* tmp = strdup(yyget_text(scanner));
			const DataCleaner dc(tmp);
			char* str = tmp + 1;
			int len = strlen(str);
			if (len > 0) str[len-1] = '\0';
			f(context, i, str);
		}
	}

	return 0;
}

ScanCleaner::~ScanCleaner()
{
	yy_delete_buffer(m_buffer, m_scanner);
	yylex_destroy(m_scanner);
}


