/*
 * wclex.cpp -- lexical analyser for the Labyrinth world compiler.
 * Reconstructed from LABYRINTH.EXE 0x413590-0x413de0.
 *
 * Tokens: keywords (case-insensitive), identifiers, integers, decimals
 * (an optional leading '-', no exponents), "strings" and single characters.
 * Comments run from '#' to end of line, or are C-style.
 */
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "wc.h"

extern "C" char *LabStrdup(const char *s);

/* wclval (4a6b70) is defined by the generated parser */

static FILE *lexFile;                                   /* 4a6520 */
static int lexChar;                                     /* 4a6118: lookahead */
static int lexLine;                                     /* 4a6114 */
static int lexColumn;                                   /* 4a6110 */
static char lexBuf[1024];                               /* 4a6120 */
extern int wcInWorld;                                   /* 4b8254 */

static struct {
    const char *name;
    int token;
} keywords[] = {                                        /* 4b7d20 */
    { "Begin", WC_BEGIN },
    { "Camera", WC_CAMERA },
    { "Close", WC_CLOSE },
    { "Color", WC_COLOR },
    { "Element", WC_ELEMENT },
    { "End", WC_END },
    { "Field", WC_FIELD },
    { "Light", WC_LIGHT },
    { "Material", WC_MATERIAL },
    { "Normal", WC_NORMAL },
    { "Object", WC_OBJECT },
    { "Open", WC_OPEN },
    { "Orientation", WC_ORIENTATION },
    { "Poly", WC_POLY },
    { "Position", WC_POSITION },
    { "Scene", WC_SCENE },
    { "Shapefile", WC_SHAPEFILE },
    { "Texture", WC_TEXTURE },
    { "URL", WC_URL },
    { "Vertex", WC_VERTEX },
    { "World", WC_WORLD },
};
static int nKeywords = sizeof(keywords) / sizeof(keywords[0]);  /* 4b7dc8 */

/*
 * A canned token stream that bypasses the file (4b7dd0).  It was a debugging
 * aid: the flag that enables it (4b7d18) is never set in the shipped
 * program, and the stream is not even a complete world.
 */
#ifdef WC_CANNED_TOKENS
static int cannedEnabled = 1;
#else
static int cannedEnabled = 0;                           /* 4b7d18 */
#endif
static struct {
    int token;
    int ival;
    double dval;
    const char *sval;
} canned[] = {
    { WC_OBJECT }, { WC_STRING, 0, 0, "Box" }, { WC_OPEN },
    { WC_POLY }, { '{' },
    { WC_VERTEX }, { WC_DOUBLE, 0, 1.5 }, { WC_DOUBLE, 0, 50.0 }, { WC_DOUBLE, 0, -5.0 },
    { WC_VERTEX }, { WC_DOUBLE, 0, 0.5 }, { WC_DOUBLE, 0, 5.0 }, { WC_DOUBLE, 0, -0.5 },
    { WC_VERTEX }, { WC_DOUBLE, 0, 1.0 }, { WC_DOUBLE, 0, 10.0 }, { WC_DOUBLE, 0, -10.0 },
    { '}' },
    { WC_COLOR }, { WC_DOUBLE, 0, 0.5 }, { WC_DOUBLE, 0, 0.7 }, { WC_DOUBLE, 0, 0.9 },
    { WC_CLOSE },
    { WC_STRING, 0, 0, "Car Bomb" }, { WC_OPEN },
    { WC_OBJECT }, { WC_STRING, 0, 0, "Box" },
    { WC_URL }, { WC_STRING, 0, 0, "http://fool.netzero.com/carbomb.html" },
    { WC_CLOSE },
    { WC_SCENE }, { WC_STRING, 0, 0, "Magic" }, { WC_OPEN }, { WC_CLOSE },
};
static int cannedCount = sizeof(canned) / sizeof(canned[0]);   /* 4b8100 */
static int cannedIndex = -1;                            /* 4b7dcc */

void WcLexInit(FILE *fp)                                /* 413590 */
{
    lexFile = fp;
    lexLine = 0;
    lexColumn = 0;
    lexBuf[0] = '\0';
    wcInWorld = 0;
    lexChar = '\n';
}

static void advance(void)
{
    lexChar = getc(lexFile);
}

static int keyword(const char *word)                    /* 413d00 */
{
    for (int i = 0; i < nKeywords; i++)
        if (_stricmp(keywords[i].name, word) == 0)
            return keywords[i].token;
    return WC_IDENTIFIER;
}

static int cannedToken(void)                            /* 413d50 */
{
    if (++cannedIndex >= cannedCount)
        return 0;
    int tok = canned[cannedIndex].token;
    switch (tok) {
    case WC_IDENTIFIER:
    case WC_STRING:
        wclval.sval = (char *)canned[cannedIndex].sval;
        break;
    case WC_INTEGER:
        wclval.ival = canned[cannedIndex].ival;
        break;
    case WC_DOUBLE:
        wclval.dval = canned[cannedIndex].dval;
        break;
    default:
        wclval.ival = tok;
        break;
    }
    return tok;
}

int WcLex(void)                                         /* 4135d0 */
{
    if (cannedEnabled)
        return cannedToken();

    for (;;) {
        if (lexChar == EOF)
            return 0;
        while (isspace(lexChar)) {
            if (lexChar == '\n') {
                lexLine++;
                lexColumn = -1;
            }
            advance();
            lexColumn++;
        }
        if (lexChar == EOF)
            return 0;
        if (lexChar == '#') {                   /* comment to end of line */
            do {
                advance();
                if (lexChar == EOF)
                    return 0;
            } while (lexChar != '\n');
            continue;
        }
        if (lexChar != '/')
            break;
        advance();
        if (lexChar != '*')
            return '/';
        for (;;) {                              /* C comment */
            do {
                advance();
                if (lexChar == EOF) {
                    WcError("Unexpected end of file within comment");
                    /* the original kept reading EOF forever here */
                    return 0;
                }
            } while (lexChar != '*');
            advance();
            if (lexChar == '/')
                break;
        }
        advance();
    }

    if (lexChar == '"') {                       /* string */
        int n = 0;
        for (;;) {
            advance();
            if (lexChar == '\\') {
                /* an escaped character always becomes a double quote */
                advance();
                lexBuf[n++] = '"';
            } else if (lexChar == '"') {
                advance();
                break;
            } else if (lexChar == EOF) {        /* (not checked originally) */
                break;
            } else {
                lexBuf[n++] = (char)lexChar;
            }
            if (n >= (int)sizeof(lexBuf) - 1)
                break;
        }
        lexBuf[n] = '\0';
        wclval.sval = LabStrdup(lexBuf);
        return WC_STRING;
    }

    if (isdigit(lexChar) || lexChar == '.' || lexChar == '-') {   /* number */
        int token = WC_INTEGER;
        int whole = 0;
        int negative = 0;
        double fraction = 0.0;

        if (lexChar == '-') {
            negative = 1;
            advance();
        }
        while (isdigit(lexChar)) {
            whole = whole * 10 + (lexChar - '0');
            advance();
        }
        if (lexChar == '.') {
            int div = 10;
            token = WC_DOUBLE;
            advance();
            while (isdigit(lexChar)) {
                fraction += (double)(lexChar - '0') / (double)div;
                div *= 10;
                advance();
            }
        }
        if (token == WC_INTEGER) {
            wclval.ival = negative ? -whole : whole;
        } else {
            wclval.dval = (double)whole + fraction;
            if (negative)
                wclval.dval = -wclval.dval;
        }
        return token;
    }

    if (isalpha(lexChar)) {                     /* identifier or keyword */
        int n = 0;
        while (isalnum(lexChar) && n < (int)sizeof(lexBuf) - 1) {
            lexBuf[n++] = (char)lexChar;
            advance();
        }
        lexBuf[n] = '\0';
        wclval.sval = LabStrdup(lexBuf);
        int tok = keyword(lexBuf);
        if (tok != WC_IDENTIFIER) {
            wclval.ival = tok;
            return tok;
        }
        return WC_IDENTIFIER;
    }

    int c = lexChar;                            /* single character */
    wclval.ival = c;
    advance();
    return c;
}

int WcLineNumber(void)                                  /* 413de0 */
{
    return lexLine;
}
