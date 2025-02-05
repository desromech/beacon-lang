#include "beacon-lang/Scanner.h"
#include "beacon-lang/Memory.h"
#include "beacon-lang/Context.h"
#include "beacon-lang/OrderedCollection.h"
#include <stdbool.h>
#include <stddef.h>

static const char* tokenKindNames[] = {
#define TokenKindName(name) #name,
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
    const char *charset = "+-/\\*~<>=@%|&?!^";
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

beacon_OrderedCollection_t *beacon_scanSourceCode(beacon_context_t *context, beacon_SourceCode_t *sourceCode)
{
    beacon_OrderedCollection_t *tokens = beacon_OrderedCollection_new(context);
    return tokens;
}