#include "beacon-lang/Scanner.h"
#include "beacon-lang/Memory.h"
#include "beacon-lang/Context.h"
#include "beacon-lang/ArrayList.h"
#include <stdbool.h>
#include <stddef.h>

static const char* tokenKindNames[] = {
#define TokenKindName(name) "BeaconToken" #name,
#include "beacon-lang/TokenKind.inc"
#undef TokenKindName
};

const char *beacon_TokenKind_toString(beacon_TokenKind_t kind)
{
    return tokenKindNames[kind];
}

bool scanner_isDigit(int character)
{
    return '0' <= character && character <= '9';
}

bool scanner_isIdentifierStart(int character)
{
    return
        ('A' <= character && character <= 'Z') ||
        ('a' <= character && character <= 'z') ||
        (character == '_')
        ;
}

bool scanner_isIdentifierMiddle(int character)
{
    return scanner_isIdentifierStart(character) || scanner_isDigit(character);
}

bool scanner_isOperatorCharacter(int character)
{
    const char *charset = "+-/\\*~<>=@,%|&?!";
    while(*charset != 0)
    {
        if(*charset == character)
            return true;
        ++charset;
    }
    return false;
}

typedef struct beacon_scannerState_s
{
    beacon_context_t *context;
    beacon_SourceCode_t *sourceCode;
    int32_t position;
    int32_t line;
    int32_t column;
    bool isPreviousCR;
}beacon_scannerState_t;

beacon_scannerState_t scannerState_newForSourceCode(beacon_context_t *context, beacon_SourceCode_t *sourceCode)
{
    beacon_scannerState_t state = {
        .context = context,
        .sourceCode = sourceCode,
        .position = 0, .line = 1, .column = 1,
        .isPreviousCR = false 
    };

    return state;
}

bool scannerState_atEnd(beacon_scannerState_t *state)
{
    return state->position >= beacon_decodeSmallInteger(state->sourceCode->textSize);
}

int scannerState_peek(beacon_scannerState_t *state, int peekOffset)
{
    intptr_t peekPosition = state->position + peekOffset;
    if(peekPosition < beacon_decodeSmallInteger(state->sourceCode->textSize))
        return state->sourceCode->text->data[peekPosition];
    else
        return -1;
}

void scannerState_advanceSinglePosition(beacon_scannerState_t *state)
{
    assert(!scannerState_atEnd(state));
    char c = state->sourceCode->text->data[state->position];
    ++state->position;
    switch(c)
    {
    case '\r':
        ++state->line;
        state->column = 1;
        state->isPreviousCR = true;
        break;
    case '\n':
        if (!state->isPreviousCR)
        {
            ++state->line;
            state->column = 1;
        }
        state->isPreviousCR = false;
        break;
    case '\t':
        state->column = (state->column + 4) % 4 * 4 + 1;
        state->isPreviousCR = false;
        break;
    default:
        ++state->column;
        state->isPreviousCR = false;
        break;
    }
}

void scannerState_advance(beacon_scannerState_t *state, int count)
{
    for(int i = 0; i < count; ++i)
    {
        scannerState_advanceSinglePosition(state);
    }
}

beacon_ScannerToken_t *scannerState_makeToken(beacon_scannerState_t *state, beacon_TokenKind_t kind)
{
    beacon_SourcePosition_t *sourcePosition = beacon_allocateObjectWithBehavior(state->context->heap, state->context->roots.sourcePositionClass, sizeof(beacon_SourcePosition_t), BeaconObjectKindPointers);
    sourcePosition->sourceCode  = state->sourceCode;
    sourcePosition->startIndex  = beacon_encodeSmallInteger(state->position);
    sourcePosition->startLine   = beacon_encodeSmallInteger(state->line);
    sourcePosition->startColumn = beacon_encodeSmallInteger(state->column);
    sourcePosition->endIndex    = beacon_encodeSmallInteger(state->position);
    sourcePosition->endLine     = beacon_encodeSmallInteger(state->line);
    sourcePosition->endColumn   = beacon_encodeSmallInteger(state->column);

    beacon_ScannerToken_t *token = beacon_allocateObjectWithBehavior(state->context->heap, state->context->roots.scannerTokenClass, sizeof(beacon_ScannerToken_t), BeaconObjectKindPointers);
    token->kind = beacon_encodeSmallInteger(kind);
    token->sourcePosition = sourcePosition;
    return token;
}

beacon_ScannerToken_t *scannerState_makeTokenStartingFrom(beacon_scannerState_t *state, beacon_TokenKind_t kind, beacon_scannerState_t *initialState)
{
    beacon_SourcePosition_t *sourcePosition = beacon_allocateObjectWithBehavior(state->context->heap, state->context->roots.sourcePositionClass, sizeof(beacon_SourcePosition_t), BeaconObjectKindPointers);
    sourcePosition->sourceCode  = state->sourceCode,
    sourcePosition->startIndex  = beacon_encodeSmallInteger(initialState->position);
    sourcePosition->startLine   = beacon_encodeSmallInteger(initialState->line);
    sourcePosition->startColumn = beacon_encodeSmallInteger(initialState->column);
    sourcePosition->endIndex    = beacon_encodeSmallInteger(state->position);
    sourcePosition->endLine     = beacon_encodeSmallInteger(state->line);
    sourcePosition->endColumn   = beacon_encodeSmallInteger(state->column);

    beacon_ScannerToken_t *token = beacon_allocateObjectWithBehavior(state->context->heap, state->context->roots.scannerTokenClass, sizeof(beacon_ScannerToken_t), BeaconObjectKindPointers);
    token->kind = beacon_encodeSmallInteger(kind),
    token->sourcePosition = sourcePosition;
    token->textPosition = beacon_encodeSmallInteger(initialState->position);
    token->textSize = beacon_encodeSmallInteger(state->position - initialState->position);
    return token;
}

beacon_ScannerToken_t *scannerState_makeErrorTokenStartingFrom(beacon_scannerState_t *state, const char *errorMessage, const beacon_scannerState_t *initialState)
{
    beacon_SourcePosition_t *sourcePosition = beacon_allocateObjectWithBehavior(state->context->heap, state->context->roots.sourcePositionClass, sizeof(beacon_SourcePosition_t), BeaconObjectKindPointers);
    sourcePosition->sourceCode = state->sourceCode,
    sourcePosition->startIndex  = beacon_encodeSmallInteger(initialState->position);
    sourcePosition->startLine   = beacon_encodeSmallInteger(initialState->line);
    sourcePosition->startColumn = beacon_encodeSmallInteger(initialState->column);
    sourcePosition->endIndex    = beacon_encodeSmallInteger(state->position);
    sourcePosition->endLine     = beacon_encodeSmallInteger(state->line);
    sourcePosition->endColumn   = beacon_encodeSmallInteger(state->column);

    beacon_ScannerToken_t *token = beacon_allocateObjectWithBehavior(state->context->heap, state->context->roots.scannerTokenClass, sizeof(beacon_ScannerToken_t), BeaconObjectKindPointers);
    token->kind = beacon_encodeSmallInteger(BeaconTokenError);
    token->sourcePosition = sourcePosition;
    token->errorMessage = beacon_importCString(state->context, errorMessage);
    
    return token;
}

beacon_ScannerToken_t *beacon_scanner_skipWhite(beacon_scannerState_t *state)
{
    bool hasSeenComment = false;
    
    do
    {
        hasSeenComment = false;
        while (!scannerState_atEnd(state) && scannerState_peek(state, 0) <= ' ')
            scannerState_advance(state, 1);

        // Multiline comment
        if(scannerState_peek(state, 0) == '"')
        {
            beacon_scannerState_t commentInitialState = *state;
            scannerState_advance(state, 2);
            bool hasCommentEnd = false;
            while (!scannerState_atEnd(state))
            {
                hasCommentEnd = scannerState_peek(state, 0) == '"';
                scannerState_advance(state, 1);
                if (hasCommentEnd)
                    break;
            }
            
            if (!hasCommentEnd)
            {
                return scannerState_makeErrorTokenStartingFrom(state, "Incomplete multiline comment.", &commentInitialState);
            }
            hasSeenComment = true;

        }
    } while (hasSeenComment);
    
    return NULL;
}

bool scanAdvanceKeyword(beacon_scannerState_t *state)
{
    if(!scanner_isIdentifierStart(scannerState_peek(state, 0)))
        return false;

    beacon_scannerState_t initialState = *state;
    while (scanner_isIdentifierMiddle(scannerState_peek(state, 0)))
        scannerState_advance(state, 1);

    if(scannerState_peek(state, 0) == ':')
    {
        scannerState_advance(state, 1);
    }
    else
    {
        *state = initialState;
        return false;
    }
    
    return true;
}

beacon_ScannerToken_t *beacon_scanSingleToken(beacon_scannerState_t *state)
{
    beacon_ScannerToken_t *whiteToken = beacon_scanner_skipWhite(state);
    if(whiteToken)
        return whiteToken;

    if(scannerState_atEnd(state))
        return scannerState_makeToken(state, BeaconTokenEndOfSource);

    beacon_scannerState_t initialState = *state;
    int c = scannerState_peek(state, 0);

    // Identifiers, keywords and multi-keywords
    if(scanner_isIdentifierStart(c))
    {
        scannerState_advance(state, 1);
        while (scanner_isIdentifierMiddle(scannerState_peek(state, 0)))
            scannerState_advance(state, 1);

        if(scannerState_peek(state, 0) == ':')
        {
            scannerState_advance(state, 1);
            bool isMultiKeyword = false;
            bool hasAdvanced = true;
            while(hasAdvanced)
            {
                hasAdvanced = scanAdvanceKeyword(state);
                isMultiKeyword = isMultiKeyword || hasAdvanced;
            }

            if(isMultiKeyword)
                return scannerState_makeTokenStartingFrom(state, BeaconTokenMultiKeyword, &initialState);
            else
                return scannerState_makeTokenStartingFrom(state, BeaconTokenKeyword, &initialState);
        }

        return scannerState_makeTokenStartingFrom(state, BeaconTokenIdentifier, &initialState);
    }

    // Numbers
    if(scanner_isDigit(c) || (c == '-' && scanner_isDigit(scannerState_peek(state, 1))))
    {
        scannerState_advance(state, 1);
        while(scanner_isDigit(scannerState_peek(state, 0)))
            scannerState_advance(state, 1);

        // Parse the radix
        if(scannerState_peek(state, 0) == 'r')
        {
            scannerState_advance(state, 1);
            while(scanner_isIdentifierMiddle(scannerState_peek(state, 0)))
                scannerState_advance(state, 1);
            return scannerState_makeTokenStartingFrom(state, BeaconTokenInteger, &initialState);
        }

        // Parse the decimal point
        if(scannerState_peek(state, 0) == '.' && scanner_isDigit(scannerState_peek(state, 1)))
        {
            scannerState_advance(state, 2);
            while(scanner_isDigit(scannerState_peek(state, 0)))
                scannerState_advance(state, 1);

            // Parse the exponent
            if(scannerState_peek(state, 0) == 'e' || scannerState_peek(state, 0) == 'E')
            {
                if(scanner_isDigit(scannerState_peek(state, 1)) ||
                ((scannerState_peek(state, 1) == '+' || scannerState_peek(state, 1) == '-') && scanner_isDigit(scannerState_peek(state, 2) )))
                {
                    scannerState_advance(state, 2);
                    while(scanner_isDigit(scannerState_peek(state, 0)))
                        scannerState_advance(state, 1);
                }
            }

            return scannerState_makeTokenStartingFrom(state, BeaconTokenFloat, &initialState);
        }

        return scannerState_makeTokenStartingFrom(state, BeaconTokenInteger, &initialState);
    }

    // Symbols
    if(c == '#')
    {
        int c1 = scannerState_peek(state, 1);
        if(scanner_isIdentifierStart(c1))
        {
            scannerState_advance(state, 2);
            while (scanner_isIdentifierMiddle(scannerState_peek(state, 0)))
                scannerState_advance(state, 1);


            if (scannerState_peek(state, 0) == ':')
            {
                scannerState_advance(state, 1);
                bool hasAdvanced = true;
                while(hasAdvanced)
                {
                    hasAdvanced = scanAdvanceKeyword(state);
                }

                return scannerState_makeTokenStartingFrom(state, BeaconTokenSymbol, &initialState); 
            }
            return scannerState_makeTokenStartingFrom(state, BeaconTokenSymbol, &initialState); 
        }
        else if(scanner_isOperatorCharacter(c1))
        {
            scannerState_advance(state, 2);
            while(scanner_isOperatorCharacter(scannerState_peek(state, 0)))
                scannerState_advance(state, 1);
            return scannerState_makeTokenStartingFrom(state, BeaconTokenSymbol, &initialState);
        }
        else if(c1 == '\'')
        {
            scannerState_advance(state, 2);
            while (!scannerState_atEnd(state))
            {
                if(scannerState_peek(state, 0) == '\'' && scannerState_peek(state, 1) == '\'')
                {
                    scannerState_advance(state, 2);
                }
                if(scannerState_peek(state, 0) == '\'')
                {
                    break;
                }
                else
                {   
                    scannerState_advance(state, 1);
                }
            }

            if (scannerState_peek(state, 0) != '\'')
                return scannerState_makeErrorTokenStartingFrom(state, "Incomplete symbol string literal.", &initialState);
            
            scannerState_advance(state, 1);
            return scannerState_makeTokenStartingFrom(state, BeaconTokenSymbol, &initialState);
        }
        else if (c1 == '[')
        {
            scannerState_advance(state, 2);
            return scannerState_makeTokenStartingFrom(state, BeaconTokenByteArrayStart, &initialState);
        }
        else if (c1 == '(')
        {
            scannerState_advance(state, 2);
            return scannerState_makeTokenStartingFrom(state, BeaconTokenLiteralArrayStart, &initialState);
        }
    }

    // Strings
    if(c == '\'')
    {
        scannerState_advance(state, 1);
        while (!scannerState_atEnd(state))
        {
            if(scannerState_peek(state, 0) == '\'' && scannerState_peek(state, 1) == '\'')
            {
                scannerState_advance(state, 2);
            }
            if(scannerState_peek(state, 0) == '\'')
            {
                break;
            }
            else
            {   
                scannerState_advance(state, 1);
            }
        }

        if (scannerState_peek(state, 0) != '\'')
            return scannerState_makeErrorTokenStartingFrom(state, "Incomplete string literal.", &initialState);
        
        scannerState_advance(state, 1);
        return scannerState_makeTokenStartingFrom(state, BeaconTokenString, &initialState);
    }

    // Characters
    if(c == '$')
    {
        scannerState_advance(state, 1);
        if(scannerState_atEnd(state))
            return scannerState_makeErrorTokenStartingFrom(state, "Incomplete character literal.", &initialState);
        
        scannerState_advance(state, 1);
        return scannerState_makeTokenStartingFrom(state, BeaconTokenCharacter, &initialState);
    }

    switch(c)
    {
    case '(':
        scannerState_advance(state, 1);
        return scannerState_makeTokenStartingFrom(state, BeaconTokenLeftParent, &initialState);
    case ')':
        scannerState_advance(state, 1);
        return scannerState_makeTokenStartingFrom(state, BeaconTokenRightParent, &initialState);
    case '[':
        scannerState_advance(state, 1);
        return scannerState_makeTokenStartingFrom(state, BeaconTokenLeftBracket, &initialState);
    case ']':
        scannerState_advance(state, 1);
        return scannerState_makeTokenStartingFrom(state, BeaconTokenRightBracket, &initialState);
    case '{':
        scannerState_advance(state, 1);
        return scannerState_makeTokenStartingFrom(state, BeaconTokenLeftCurlyBracket, &initialState);
    case '}':
        scannerState_advance(state, 1);
        return scannerState_makeTokenStartingFrom(state, BeaconTokenRightCurlyBracket, &initialState);
    case ';':
        scannerState_advance(state, 1);
        return scannerState_makeTokenStartingFrom(state, BeaconTokenSemicolon, &initialState);
    case '.':
        scannerState_advance(state, 1);
        return scannerState_makeTokenStartingFrom(state, BeaconTokenDot, &initialState);
    case '|':
        scannerState_advance(state, 1);
        if (scanner_isOperatorCharacter(scannerState_peek(state, 0)))
        {
            while(scanner_isOperatorCharacter(scannerState_peek(state, 0)))
                scannerState_advance(state, 1);
            return scannerState_makeTokenStartingFrom(state, BeaconTokenOperator, &initialState);
        }

        return scannerState_makeTokenStartingFrom(state, BeaconTokenBar, &initialState);
    default:
        break;
    }

    if(scanner_isOperatorCharacter(c))
    {
        scannerState_advance(state, 1);
        if(!scanner_isOperatorCharacter(scannerState_peek(state, 0)))
        {
            switch(c)
            {
            case '<':
                return scannerState_makeTokenStartingFrom(state, BeaconTokenLessThan, &initialState);
            case '>':
                return scannerState_makeTokenStartingFrom(state, BeaconTokenGreaterThan, &initialState);
            default:
                // Generic character, do nothing.
                break;
            }
        }

        while(scanner_isOperatorCharacter(scannerState_peek(state, 0)))
            scannerState_advance(state, 1);

        return scannerState_makeTokenStartingFrom(state, BeaconTokenOperator, &initialState);
    }

    scannerState_advance(state, 1);
    return scannerState_makeErrorTokenStartingFrom(state, "Unknown character", &initialState);
}

beacon_ArrayList_t *beacon_scanSourceCode(beacon_context_t *context, beacon_SourceCode_t *sourceCode)
{
    beacon_ArrayList_t *arrayList = beacon_ArrayList_new(context);
    beacon_scannerState_t currentState = scannerState_newForSourceCode(context, sourceCode);
    beacon_ScannerToken_t *scannedToken = NULL;
    do
    {
        scannedToken = beacon_scanSingleToken(&currentState);
        if(beacon_decodeSmallInteger(scannedToken->kind) != BeaconTokenNullToken)
        {
            beacon_ArrayList_add(context, arrayList, (beacon_oop_t)scannedToken);
            if (beacon_decodeSmallInteger(scannedToken->kind) == BeaconTokenEndOfSource)
                break;
        }
    } while (beacon_decodeSmallInteger(scannedToken->kind) != BeaconTokenEndOfSource);

    return arrayList;
}