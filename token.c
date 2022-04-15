/******************************************************************************
 * Simple Tokenizer
 * 
 * Recognize words, small integers, and the period
 * 
 * File:    token.c
 * 
 * Usage:   token sourcefile
 *          -- sourcefile      name of source file to tokenize
 *
 *****************************************************************************/

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <sys/types.h>
#include <sys/timeb.h>

#define FORM_FEED_CHAR              '\f'
#define EOF_CHAR                    '\x7f'

#define MAX_FILE_NAME_LEN           32
#define MAX_SRC_LINE_LEN            256
#define MAX_PRINT_LINE_LEN          80
#define PAGE_MAX_LINES              50
#define DATE_STR_LEN                26
#define MAX_TOKEN_STR_LEN           MAX_SRC_LINE_LEN
#define MAX_CODE_BUFFER_SIZE        4096

#define MAX_INT                     23767 // 2^15 - 1
#define MAX_DIGIT_COUNT             20

#define CHAR_TABLE_SIZE             256


/*
==  Boolean
*/
typedef enum {
    FALSE,
    TRUE,
} BOOLEAN;

/*
==  Character codes
*/
typedef enum {
    LETTER,
    DIGIT,
    SPECIAL,
    EOF_CODE,
} CHAR_CODE;

/*
==  Token codes
*/
typedef enum {
    NO_TOKEN,
    WORD,
    NUMBER,
    PERIOD,
    END_OF_FILE,
    ERROR,
} TOKEN_CODE;

/*
==  Token name strings
*/
char* symbol_strings[] = {
    "<no token>",
    "<WORD>",
    "<NUMBER>",
    "<PERIOD>",
    "<END OF FILE>",
    "<ERROR>",
};

/*
==  Literal type
*/
typedef enum {
    INTEGER_LIT,
    STRING_LIT,
} LITERAL_TYPE;

/*
==  Literal structure
*/
typedef struct {
    LITERAL_TYPE type;
    union {
        int     integer;
        char    string[MAX_SRC_LINE_LEN];
    } value;
} LITERAL;


/*===========================================================================*/
/* Globals                                                                   */
/*===========================================================================*/

char            ch;                             // current input character
TOKEN_CODE      token;                          // code of the current token
LITERAL         literal;                        // value of literal
int             buffer_offset;                  // offset into source buffer
unsigned int    level       = 0;                // current nesting level
unsigned int    line_number = 0;                // current line number

char            src_buff[MAX_SRC_LINE_LEN];     // source file buffer
char            token_str[MAX_TOKEN_STR_LEN];   // token string

char*           bufferp     = src_buff;         // source buffer pointer
char*           tokenp      = token_str;        // token string pointer

unsigned short  digit_count;                    // no. of digits in number
BOOLEAN         count_error;                    // if too many digits in number

unsigned int    page_number = 0;
unsigned short  line_count  = PAGE_MAX_LINES;   // no. of lines on current page

char            src_name[MAX_FILE_NAME_LEN];    // name of the source file
char            date[DATE_STR_LEN];             // current date and time

FILE*           source_file;

CHAR_CODE       char_table[CHAR_TABLE_SIZE];


/*===========================================================================*/
/* char_code                Return the character code of give character.     */
/*===========================================================================*/

#define char_code(ch)       char_table[ch]


/*===========================================================================*/
/* main                     Loop to tokenize source file.                    */
/*===========================================================================*/

main(argc, argv)

    int     argc;
    char*   argv[];

{
    printf("scanner initialized\n");
    printf("%d %s\n", argc, argv[1]);
    /*
    -- Initialize the scanner
    */
    init_scanner(argv[1]);

    /*
    -- Repeatedly fetch token until a period or the end of file.
    */
    do {
        get_token();
        if (token == END_OF_FILE) {
            print_line("*** ERROR: Unexpected end of file.\n");
            break;
        }
        print_token();
    } while (token != PERIOD);

    quit_scanner();
}


/*===========================================================================*/
/* print_token              Print a line describing the current token.       */
/*===========================================================================*/

print_token()

{
    char    line[MAX_PRINT_LINE_LEN];
    char*   symbol_string = symbol_strings[token];

    switch (token) {

        case NUMBER:
            sprintf(line, "\t>> %-16s %-s\n",
                    symbol_string, literal.value.integer);
            break;

        default:
            sprintf(line, "\t>> %-16s %-s\n",
                    symbol_string, token_str);
            break;
    }
    print_line(line);
}


                    /***************************/
                    /*                         */
                    /*      Initialization     */
                    /*                         */
                    /***************************/

/*===========================================================================*/
/* init_scanner             Initialize the scanner globals and open the      */
/*                          source files.                                    */
/*===========================================================================*/

init_scanner(name)

    char*   name;           // name of the source file

{
    int ch;
    /*
    -- Initialize characters table.
    */
    for (ch = 0; ch < 256; ch++)        char_table[ch] = SPECIAL;
    for (ch = '0'; ch <= '9'; ch++)     char_table[ch] = DIGIT;
    for (ch = 'A'; ch <= 'Z'; ch++)     char_table[ch] = LETTER;
    for (ch = 'a'; ch <= 'z'; ch++)     char_table[ch] = LETTER;

    char_table[EOF_CHAR] = EOF_CODE;

    init_page_header(name);
    open_source_file(name);
}

/*===========================================================================*/
/* quit_scanner             Terminate the scanner.                           */
/*===========================================================================*/

quit_scanner()

{
    /*
    -- Close the source file
    */
    fclose(source_file);
}


                    /***************************/
                    /*                         */
                    /*    Character routines   */
                    /*                         */
                    /***************************/

/*===========================================================================*/
/* get_char                 Set ch to the next character from the source     */
/*                          buffer.                                          */
/*===========================================================================*/

get_char()

{
    BOOLEAN get_source_line();

    /*
    -- If at end of current source line, read another line.
    -- If at end of file, set ch to the EOF chracter and return.
    */
   if (*bufferp = '\0') {
       if (!get_source_line()) {
           ch = EOF_CHAR;
           return;
       }
       bufferp = src_buff;
       buffer_offset = 0;
   }

   ch = *bufferp++;             // next character in the buffer

   if ((ch == '\n') || (ch == '\t')) ch = ' ';
}

/*===========================================================================*/
/* skip_blanks              Skip past any blanks at the current location in  */
/*                          the source buffer. Set ch to the next nonblank   */
/*                          character.                                       */
/*===========================================================================*/

skip_blanks()

{
    while (ch == ' ') get_char();
}


                    /***************************/
                    /*                         */
                    /*      Token routines     */
                    /*                         */
                    /***************************/

                /*               Note:              */
                /* After a token is extracted, `ch` */
                /* is the first character after the */
                /* extracted token.                 */

/*===========================================================================*/
/* get_token                Extract the next token from the source buffer.   */
/*===========================================================================*/

get_token()

{
    skip_blanks();
    tokenp = token_str;

    switch (char_code(ch)) {
        case LETTER:        get_word();             break;
        case DIGIT:         get_number();           break;
        case EOF_CODE:      token = END_OF_FILE;    break;
        default:            get_special();          break;
    }
}

/*===========================================================================*/
/* get_word                 Extract a word token and set token to            */
/*                          be IDENTIFIER.                                   */
/*===========================================================================*/

get_word()

{
    BOOLEAN is_reversed_word();

    /*
    -- Extract the word
    */
    while ((char_code(ch) == LETTER) || (char_code(ch) == DIGIT)) {
        *tokenp++ = ch;
        get_char();
    }

    *tokenp = '\0';
    token   = WORD;
}

/*===========================================================================*/
/* get_number               Extract a number token and set literal to its    */
/*                          value. Set token to NUMBER.                      */
/*===========================================================================*/

get_number()

{
    int     nvalue      = 0;        // value of number
    int     digit_count = 0;        // total no. of digits in number
    BOOLEAN count_error = 0;        // true if too many digits in number

    do {
        *tokenp++ = ch;

        if (++digit_count <= MAX_DIGIT_COUNT) {
            nvalue = 10 * nvalue + (ch - '0');
        }
        else {
            count_error = TRUE;
        }

        get_char();
    } while (char_code(ch) == DIGIT);

    if (count_error) {
        token = ERROR;
        return;
    }

    literal.type            = INTEGER_LIT;
    literal.value.integer   = nvalue;
    *tokenp                 = '\0';
    token                   = NUMBER;
}

/*===========================================================================*/
/* get_special              Extract a special token. The only special token  */
/*                          we have in this simple version of tokenizer is   */
/*                          PERIOD. All others will be considered ERROR.     */
/*===========================================================================*/

get_special()

{
    *tokenp++ = ch;
    token = (ch == '.') ? PERIOD : ERROR;
    get_char();
    *tokenp = '\0';
}


                    /***************************/
                    /*                         */
                    /*   Source file routines  */
                    /*                         */
                    /***************************/


/*===========================================================================*/
/* open_source_file         Open the source file and fetch its first         */
/*                          character.                                       */
/*===========================================================================*/

open_source_file(name)

    char*   name;           // name of the source file

{
    printf("Open source file\n");
    if (name == NULL || (source_file = fopen(name, "r")) == NULL) {
        printf("*** Error: No such file.\n");
        exit(1);
    }
    /*
    -- Fetch the first character
    */
   bufferp = "";
   getchar();
}

/*===========================================================================*/
/* get_source_file          Read the next line from the source file. If      */
/*                          there exists one, print it and return TRUE.      */
/*                          Else, return FALSE for the end of file.          */
/*===========================================================================*/

BOOLEAN get_source_line()

{
    char print_buffer[MAX_SRC_LINE_LEN + 9];

    if (fgets(src_buff, MAX_SRC_LINE_LEN, source_file) != NULL) {
        ++line_number;
        sprintf(print_buffer, "%4d %d: %s", line_number, level, src_buff);
        print_line(print_buffer);

        return TRUE;
    }
    return FALSE;
}


                    /***************************/
                    /*                         */
                    /*    Printing routines    */
                    /*                         */
                    /***************************/

/*===========================================================================*/
/* print_line               Print out a line. Start a new page if the        */
/*                          current page is full.                            */
/*===========================================================================*/

print_line(line)

    char    line[];         // the line to be printed

{
    char    save_ch;
    char*   save_ch_ptr = NULL;

    if (++line_count > PAGE_MAX_LINES) {
        print_page_header();
        line_count = 1;
    }
    if (strlen(line) > MAX_PRINT_LINE_LEN) {
        save_ch_ptr = &line[MAX_PRINT_LINE_LEN];
    }
    if (save_ch_ptr) {
        save_ch         = *save_ch_ptr;
        *save_ch_ptr    = '\0';
    }
    
    printf("%s", line);

    if (save_ch_ptr) {
        *save_ch_ptr    = save_ch;
    }
}

/*===========================================================================*/
/* init_page_header         Initialize the fields of the page header.        */
/*===========================================================================*/

init_page_header(name)

    char*   name;           // name of the source file

{
    time_t timer;

    strncpy(src_name, name, MAX_FILE_NAME_LEN - 1);

    /*
    -- Set the current date and time in the date string.
    */
   time(&timer);
   strcpy(date, asctime(localtime(&timer)));
}

/*===========================================================================*/
/* print_page_header        Print the page header at the top of the next     */
/*                          page.                                            */
/*===========================================================================*/

print_page_header()

{
    putchar(FORM_FEED_CHAR);
    printf("Page %d     %s     %s\n\n", ++page_number, src_name, date);
}