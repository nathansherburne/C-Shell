/*             getword.c
 *
 * Nathan Sherburne
 * Carroll
 * CS570
 * masc0093
 * Program1
 * Due Date: 9/21/16
 *
 * The getword() function gets one word from the input stream.
 * Words and metacharacters are defined specifically in the getword.h file.
 * It returns -1 if it encounters end-of-file;
 * otherwise, it returns the number of characters in the word(or the negative).
 *
 * INPUT: a pointer to the beginning of a character string.
 * OUTPUT: the number of characters in the word (or the negative of that number).
 * SIDE EFFECTS: the character string that the input pointer
 *    w points to will be overwritten with the word from the
 *    input stream.
 */

/* Include Files */
#include <stdio.h>

#define BUFFER 254
#define BACKSLASH 0x5C
#define NEWLINE 0xA
#define TAB 0x9

int getword( char * w )
{
    int iochar;                                           /* iochar holds the current characters int value */
    int counter = 0;					  /* counter counts the number of characters in each word */
    int specialCase = 0;
    int returnNegative = 0;
    extern int IGNORE_THIS_METACHARACTER;

    /* Stores one character at a time in memory pointed to
    by w, ending in null-terminator once a space, tab,
    new-line, or certain metacharacters are encountered. */

    while ( ( iochar = getchar() ) != EOF )
    {   
	if ( counter == BUFFER )			/* Quit if buffer overrun */
	{
	    ungetc(iochar, stdin);
	    break;
	}
	if ( specialCase != 0 )				/* Special case for metacharacters following a backslash */
	{
	    specialCase = 0;
	    /* Only treat backslash if it is followed by a metacharacter. */
	    if ( iochar == BACKSLASH || iochar == '<' || iochar == '>' || iochar == '|' || iochar == '$' || iochar == '&' || iochar == ' ')
	    {
	    	IGNORE_THIS_METACHARACTER = 1;		/* This metacharacter was preceeded by a '\' */
	        *w++ = iochar;
	        counter++;
	        continue;
	    }
	}
	if ( iochar == BACKSLASH )
	{
	    specialCase = 1;
	    continue;
	}
	if ( iochar == '$' && counter == 0 )		/* If $ is first char of word, return negative value */ 
	{
		returnNegative = 1;				/* if lone '$', this variable doesn't do anything, but -1 will still be returned */
	}
	if ( iochar == '<' || iochar == '>' || iochar == '|' || iochar == '&' )
	{
	    if ( counter > 1 ) 		/* 'Peek' metacharacter */
	    {				/* Deal with word before metacharacter */
		ungetc(iochar,stdin);	/* Put metacharacter back on input stream */
		*w = '\0';
		if ( returnNegative == 1 )
		{
		    return counter - 2*counter;
		}
		return counter;
	    }
	    if ( counter == 0 )		/* Deal with the metacharacter itself */
	    {
		*w++ = iochar;
		*w = '\0';
		return 1;
	    }
	}
	else if ( iochar == ' ' || iochar == TAB || iochar == NEWLINE )
	{
	    if ( counter != 0 )
	    {
	        *w = '\0';
			ungetc(iochar,stdin);		/* In case of new-line, put it back into in stream. */
			if ( returnNegative == 1 )
			{
				return counter - 2*counter;
			}
	        return counter;
	    }
	    else if ( iochar == NEWLINE )
	    {
		*w = '\0';
		return 0;
	    }
	}
	else	/* iochar is not a space, backslash, or metacharacter(it might be a '$' provided that the '$' was not 1st in word */
	{
		if(returnNegative == 1 && counter == 1)	/* last char was a '$', but not a lone '$', so overwrite it */
		{
			w--;
		}
        *w++ = iochar;
		counter++;
	}
    }
    *w = '\0';		/* EOF reached(or buffer overrun), now null terminate and return the word we were working on. */
    if ( counter == 0 )	/* If there is no word we were working on, just return -1 for EOF */
    {
	return -1;
    }
    return counter;
}
