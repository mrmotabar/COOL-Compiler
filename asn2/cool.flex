/*
 *  The scanner definition for COOL.
 */

/*
 *  Stuff enclosed in %{ %} in the first section is copied verbatim to the
 *  output, so headers and global definitions are placed here to be visible
 * to the code in the file.  Don't remove anything that was here initially
 */
%{
#include <cool-parse.h>
#include <stringtab.h>
#include <utilities.h>

/* The compiler assumes these identifiers. */
#define yylval cool_yylval
#define yylex  cool_yylex

/* Max size of string constants */
#define MAX_STR_CONST 1025
#define YY_NO_UNPUT   /* keep g++ happy */

extern FILE *fin; /* we read from this file */

/* define YY_INPUT so we read from the FILE fin:
 * This change makes it possible to use this scanner in
 * the Cool compiler.
 */
#undef YY_INPUT
#define YY_INPUT(buf,result,max_size) \
	if ( (result = fread( (char*)buf, sizeof(char), max_size, fin)) < 0) \
		YY_FATAL_ERROR( "read() in flex scanner failed");

char string_buf[MAX_STR_CONST]; /* to assemble string constants */
char *string_buf_ptr;

extern int curr_lineno;
extern int verbose_flag;

extern YYSTYPE cool_yylval;

/*
 *  Add Your own definitions here
 */

%}

/*
 * Define names for regular expressions here.
 */
/*key words*/
Class 		?i:class
Else 		?i:else
False 		f?i:alse
Fi 		?i:fi
If 		?i:if
In 		?i:in
Inherits 	?i:inherits
IsVoid 	?i:isvoid
Let 		?i:let
Loop 		?i:loop
Pool 		?i:pool
Then 		?i:then
While 		?i:while
Case 		?i:case
Esac 		?i:esac
New 		?i:new
Of 		?i:of
Not 		?i:not
True 		t?i:rue

/*symbols*/
DArrow 	=>
LE 		<=
Assignment 	<-
Symbol 	[+\-*/=<.,;:~()@{}]

/*types*/
Integer 	[0-9]+
TypeID 	[A-Z][A-Za-z0-9_]*
ObjectID 	[a-z][A-Za-z0-9_]*
StringSE \"
%x String

/*whitespaces*/
WhiteSpace 	[ \f|\r|\t|\v]+
NewLine 	\n 

/*errors*/
Invalid 	[^a-zA-Z0-9_ \f\r\t\v\n+\-*/=<.,;:~()@{}]

/*comments*/
CommentS 	"(*"
CommentE	"*)"
%x Comment
%x InlineComment

%%

 /*
  *  Nested comments
  */
{CommentS} {BEGIN(Comment);}
{CommentE} {
	yylval.error_msg = "Unmatched *)";
	return ERROR;
}
<Comment><<EOF>> {
	BEGIN(INITIAL);
	yylval.error_msg = "EOF in comment";
	return ERROR;
}
<Comment>. {}
<Comment>\n {curr_lineno++;}
<Comment>{CommentE} {BEGIN(INITIAL);}

 /*
  * Inline comments
  */
"--" {BEGIN(InlineComment);}
<InlineComment><<EOF>> {
	BEGIN(INITIAL);
	yylval.error_msg = "EOF in comment";
	return ERROR;
}
<InlineComment>. {}
<InlineComment>\n {
	BEGIN(INITIAL);
	curr_lineno++;
}

 /*
  *  The multiple-character operators.
  */

 /*
  * Keywords are case-insensitive except for the values true and false,
  * which must begin with a lower-case letter.
  */

{Class} 	{return CLASS;}
{Else} 	{return ELSE;}
{Fi} 		{return FI;}          
{If} 		{return IF;}            
{In} 		{return IN;}              
{Inherits} 	{return INHERITS;}      
{IsVoid} 	{return ISVOID;}        
{Let} 		{return LET;}            
{Loop} 	{return LOOP;}         
{Pool} 	{return POOL;}    
{Then} 	{return THEN;}           
{While} 	{return WHILE;}
{Case} 	{return CASE;}          
{Esac} 	{return ESAC;}           
{New} 		{return NEW;}        
{Of} 		{return OF;}              
{Not} 		{return NOT;}
{True} {
	yylval.boolean = true;
	return BOOL_CONST;
}
{False} {
	yylval.boolean = false;
	return BOOL_CONST;
}

{DArrow} {return DARROW;}
{LE} {return LE;}
{Assignment} {return ASSIGN;}
{Symbol} {return int(yytext[0]);}
{Integer} {
	yylval.symbol = inttable.add_string(yytext);
	return INT_CONST;
}
{TypeID} {
	yylval.symbol = idtable.add_string(yytext);
	return TYPEID;
}
{ObjectID} {
	yylval.symbol = idtable.add_string(yytext);
	return OBJECTID;
}
{WhiteSpace} {}
{NewLine} {curr_lineno++;}

 /*
  *  String constants (C syntax)
  *  Escape sequence \c is accepted for all characters c. Except for
  *  \n \t \b \f, the result is c.
  *
  */
{StringSE} {
	string_buf_ptr = string_buf;
	BEGIN(String);
}
<String><<EOF>> {
	BEGIN(INITIAL);
	yylval.error_msg = "EOF in string constant";
	return ERROR;
}
<String>\n {
	BEGIN(INITIAL);
	curr_lineno++;
	yylval.error_msg = "Unterminated string constant";
	return ERROR;
}
<String>\0 {
	BEGIN(INITIAL);
	yylval.error_msg = "String contains null character";
	return ERROR;
}
<String>\\[ntbf]  {
	if ((string_buf_ptr - 1) == &string_buf[MAX_STR_CONST - 1]) {
		BEGIN(INITIAL);
		yylval.error_msg = "String constant too long";
		return ERROR;
	}
	if (yytext[1] == 'n') {
		*string_buf_ptr++ = '\n';
	}
	else if (yytext[1] == 't') {
		*string_buf_ptr++ = '\t';
	}
	else if (yytext[1] == 'b') {
		*string_buf_ptr++ = '\b';
	}
	else {
		*string_buf_ptr++ = '\f';
	}
}

<String>\\.	{
	if ((string_buf_ptr - 1) == &string_buf[MAX_STR_CONST - 1]) {
		BEGIN(INITIAL);
		yylval.error_msg = "String constant too long";
		return ERROR;
	}
	*string_buf_ptr++ = yytext[1];
}
<String>\\\n	{
	if ((string_buf_ptr - 1) == &string_buf[MAX_STR_CONST - 1]) {
		BEGIN(INITIAL);
		yylval.error_msg = "String constant too long";
		return ERROR;
	}
	curr_lineno++;
	*string_buf_ptr++ = yytext[1];
}
<String>[^\\\n\"]+ {
	char *yptr = yytext;
	while ( *yptr ) {
		if ((string_buf_ptr - 1) == &string_buf[MAX_STR_CONST - 1]) {
			BEGIN(INITIAL);
			yylval.error_msg = "String constant too long";
			return ERROR;
		}
		*string_buf_ptr++ = *yptr++;
	}
}
<String>{StringSE}	{
	BEGIN(INITIAL);
	*string_buf_ptr = '\0';
	yylval.symbol = stringtable.add_string(string_buf);
	return STR_CONST;
}
{Invalid} {
  	yylval.error_msg = yytext;
	return ERROR;
}
%%
