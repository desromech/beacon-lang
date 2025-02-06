#include "beacon-lang/Parser.h"
#include "beacon-lang/Context.h"
#include "beacon-lang/ArrayList.h"
#include "beacon-lang/SourceCode.h"
#include <stddef.h>
#include <stdbool.h>

typedef struct beacon_parserState_s
{
    beacon_context_t *context;
    beacon_SourceCode_t *sourceCode;
    size_t tokenCount;
    beacon_ArrayList_t *tokens;
    size_t position;
}beacon_parserState_t;

bool parserState_atEnd(beacon_parserState_t *state)
{
    return state->position > state->tokenCount;
}

beacon_TokenKind_t parserState_peekKind(beacon_parserState_t *state, int offset)
{
    size_t peekPosition = state->position + offset;
    if (peekPosition <= state->tokenCount)
    {
        beacon_ScannerToken_t *token = (beacon_ScannerToken_t*)beacon_ArrayList_at(state->context, state->tokens, peekPosition);
        return beacon_decodeSmallInteger(token->kind);
    }
    else
        return BeaconTokenEndOfSource;
}

beacon_ScannerToken_t *parserState_peek(beacon_parserState_t *state, int offset)
{
    size_t peekPosition = state->position + offset;
    if (peekPosition <= state->tokenCount)
    {
        beacon_ScannerToken_t *token = (beacon_ScannerToken_t *)beacon_ArrayList_at(state->context, state->tokens, peekPosition);
        return token;
    }
    else
        return NULL;
}

void parserState_advance(beacon_parserState_t *state)
{
    assert(state->position < state->tokenCount);
    ++state->position;
}

beacon_ScannerToken_t *parserState_next(beacon_parserState_t *state)
{
    assert(state->position < state->tokenCount);
    beacon_ScannerToken_t *token = (beacon_ScannerToken_t*)beacon_ArrayList_at(state->context, state->tokens, state->position);
    ++state->position;
    return token;
}

beacon_ParseTreeNode_t *parserState_advanceWithExpectedError(beacon_parserState_t *state, const char *message)
{
    if (parserState_peekKind(state, 0) == BeaconTokenError)
    {
        beacon_ScannerToken_t *errorToken = parserState_next(state);
        beacon_ParseTreeErrorNode_t *errorNode = beacon_allocateObjectWithBehavior(state->context->heap, state->context->roots.parseTreeErrorNodeClass, sizeof(beacon_ParseTreeNode_t), BeaconObjectKindBytes);
        errorNode->errorMessage = errorToken->errorMessage;
        return &errorNode->super;
    }
    else if (parserState_atEnd(state))
    {
        beacon_ParseTreeErrorNode_t *errorNode = beacon_allocateObjectWithBehavior(state->context->heap, state->context->roots.parseTreeErrorNodeClass, sizeof(beacon_ParseTreeNode_t), BeaconObjectKindBytes);
        errorNode->errorMessage = beacon_importCString(state->context, message);
        return &errorNode->super;
    }
    else
    {
        beacon_ParseTreeErrorNode_t *errorNode = beacon_allocateObjectWithBehavior(state->context->heap, state->context->roots.parseTreeErrorNodeClass, sizeof(beacon_ParseTreeNode_t), BeaconObjectKindBytes);
        parserState_advance(state);
        errorNode->errorMessage = beacon_importCString(state->context, message);
        return &errorNode->super;
    }
}

size_t parserState_memento(beacon_parserState_t *state)
{
    return state->position;
}

void parserState_restore(beacon_parserState_t *state, size_t memento)
{
    state->position = memento;
}

beacon_SourcePosition_t *parserState_previousSourcePosition(beacon_parserState_t *state)
{
    assert(state->position > 1);
    beacon_ScannerToken_t *token = (beacon_ScannerToken_t *)beacon_ArrayList_at(state->context, state->tokens, state->position - 1);
    return token->sourcePosition;
}

beacon_SourcePosition_t *parserState_currentSourcePosition(beacon_parserState_t *state)
{
    if (state->position >= 1 && state->position < state->tokenCount)
    {
        beacon_ScannerToken_t *token = (beacon_ScannerToken_t *)beacon_ArrayList_at(state->context, state->tokens, state->position);
        return token->sourcePosition;
    }

    assert(state->tokenCount > 0);
    beacon_ScannerToken_t *token = (beacon_ScannerToken_t *)beacon_ArrayList_at(state->context, state->tokens, state->tokenCount);
    return token->sourcePosition;
}

beacon_SourcePosition_t *sparserState_sourcePositionFrom(beacon_parserState_t *state, size_t startingPosition)
{
    assert(startingPosition < state->tokenCount);
    beacon_ScannerToken_t *startingToken = (beacon_ScannerToken_t *)beacon_ArrayList_at(state->context, state->tokens, startingPosition);
    beacon_SourcePosition_t *startSourcePosition = startingToken->sourcePosition;
    if (state->position > 0)
    {
        beacon_SourcePosition_t *endSourcePosition = parserState_previousSourcePosition(state);
        return beacon_sourcePosition_to(state->context, startSourcePosition, endSourcePosition);
    }
    else
    {
        beacon_SourcePosition_t *endSourcePosition = parserState_previousSourcePosition(state);
        return beacon_sourcePosition_until(state->context, startSourcePosition, endSourcePosition);
    }
}

beacon_ParseTreeNode_t *parserState_makeErrorAtCurrentSourcePosition(beacon_parserState_t *state, const char *errorMessage)
{
    beacon_ParseTreeErrorNode_t *errorNode = beacon_allocateObjectWithBehavior(state->context->heap, state->context->roots.parseTreeErrorNodeClass, sizeof(beacon_ParseTreeNode_t), BeaconObjectKindBytes);
    errorNode->errorMessage = beacon_importCString(state->context, errorMessage);
    return &errorNode->super;
}

beacon_ParseTreeNode_t *parserState_expectAddingErrorToNode(beacon_parserState_t *state, beacon_TokenKind_t expectedKind, beacon_ParseTreeNode_t *node)
{
    if (parserState_peekKind(state, 0) == expectedKind)
    {
        parserState_advance(state);
        return node;
    }

    beacon_ParseTreeErrorNode_t *errorNode = beacon_allocateObjectWithBehavior(state->context->heap, state->context->roots.parseTreeErrorNodeClass, sizeof(beacon_ParseTreeNode_t), BeaconObjectKindBytes);
    errorNode->errorMessage = beacon_importCString(state->context, "Expected a specific token kind.");
    errorNode->innerNode = node;
    return &errorNode->super;
}
