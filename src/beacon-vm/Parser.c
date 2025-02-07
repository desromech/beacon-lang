#include "beacon-lang/Parser.h"
#include "beacon-lang/Context.h"
#include "beacon-lang/ArrayList.h"
#include "beacon-lang/SourceCode.h"
#include <stdlib.h>
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

beacon_SourcePosition_t *parserState_sourcePositionFrom(beacon_parserState_t *state, size_t startingPosition)
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

intptr_t parser_parseIntegerConstant(beacon_ScannerToken_t *token)
{
    size_t constantStringSize = beacon_decodeSmallInteger(token->textSize);
    const char *constantString = (const char*)token->sourcePosition->sourceCode->text->data + beacon_decodeSmallInteger(token->textPosition);
    
    intptr_t result = 0;
    intptr_t radix = 10;
    bool hasSeenRadix = false;

    for (size_t i = 0; i < constantStringSize; ++i)
    {
        char c = constantString[i];
        if (!hasSeenRadix && (c == 'r' || c == 'R'))
        {
            hasSeenRadix = true;
            radix = result;
            result = 0;
        }
        else
        {
            if ('0' <= c && c <= '9')
                result = result * radix + (intptr_t)(c - '0');
            else if ('A' <= c && c <= 'Z')
                result = result * radix + (intptr_t)(c - 'A' + 10);
            else if ('a' <= c && c <= 'z')
                result = result * radix + (intptr_t)(c - 'a' + 10);
        }
    }

    return beacon_encodeSmallInteger(result);
}

beacon_ParseTreeNode_t *parser_parseLiteralInteger(beacon_parserState_t *state)
{
    beacon_ScannerToken_t *token = parserState_next(state);
    assert(beacon_decodeSmallInteger(token->kind) == BeaconTokenInteger);

    intptr_t parsedConstant = parser_parseIntegerConstant(token);
    beacon_ParseTreeLiteralNode_t *literal = beacon_allocateObjectWithBehavior(state->context->heap, state->context->roots.parseTreeLiteralNodeClass, sizeof(beacon_ParseTreeLiteralNode_t), BeaconObjectKindPointers);
    literal->super.sourcePosition = token->sourcePosition;
    literal->value = parsedConstant;
    return &literal->super;
}

beacon_ParseTreeNode_t *parser_parseLiteralFloat(beacon_parserState_t *state)
{
    beacon_ScannerToken_t *token = parserState_next(state);
    assert(beacon_decodeSmallInteger(token->kind) == BeaconTokenInteger);

    char *literalBuffer = calloc(1, beacon_decodeSmallInteger(token->textSize) + 1);
    memcpy(literalBuffer, token->sourcePosition->sourceCode->text->data, token->textSize);
    double value = atof(literalBuffer);

    beacon_ParseTreeLiteralNode_t *literal = beacon_allocateObjectWithBehavior(state->context->heap, state->context->roots.parseTreeLiteralNodeClass, sizeof(beacon_ParseTreeLiteralNode_t), BeaconObjectKindPointers);
    literal->super.sourcePosition = token->sourcePosition;
    literal->value = beacon_encodeSmallDoubleValue(value);
    return &literal->super;
}

beacon_ParseTreeNode_t *parser_parseLiteralCharacter(beacon_parserState_t *state)
{
    beacon_ScannerToken_t *token = parserState_next(state);
    assert(beacon_decodeSmallInteger(token->kind) == BeaconTokenCharacter);

    char parsedConstant = token->sourcePosition->sourceCode->text->data[beacon_decodeSmallInteger(token->textPosition) + 1];
    beacon_ParseTreeLiteralNode_t *literal = beacon_allocateObjectWithBehavior(state->context->heap, state->context->roots.parseTreeLiteralNodeClass, sizeof(beacon_ParseTreeLiteralNode_t), BeaconObjectKindPointers);
    literal->super.sourcePosition = token->sourcePosition;
    literal->value = beacon_encodeCharacter(parsedConstant);
    return &literal->super;
}

beacon_ParseTreeNode_t *parser_parseLiteralString(beacon_parserState_t *state)
{
    beacon_ScannerToken_t *token = parserState_next(state);
    assert(beacon_decodeSmallInteger(token->kind) == BeaconTokenString);

    const char *textData = token->sourcePosition->sourceCode->text->data + beacon_decodeSmallInteger(token->textPosition);
    size_t textDataSize = beacon_decodeSmallInteger(token->textSize);

    size_t textConstantSize = 0;
    for(size_t i = 1; i < textDataSize; ++i)
    {
        if(textData[i] == '\'')
        {
            if(i + 1 < textDataSize && textData[i +1] == '\'')
            {
                ++i;
                ++textConstantSize;
            }
        }
        else
        {
            ++textConstantSize;
        }
    }

    beacon_String_t *stringLiteral = beacon_allocateObjectWithBehavior(state->context->heap, state->context->roots.stringClass, sizeof(beacon_String_t) + textConstantSize, BeaconObjectKindBytes);
    size_t destIndex = 0;
    for(size_t i = 1; i < textDataSize; ++i)
    {
        if(textData[i] == '\'')
        {
            if(i + 1 < textDataSize && textData[i +1] == '\'')
            {
                ++i;
                stringLiteral->data[destIndex++] = '\'';
            }
        }
        else
        {   
            stringLiteral->data[destIndex++] = textData[i];
            ++textConstantSize;
        }
    }

    char parsedConstant = token->sourcePosition->sourceCode->text->data[beacon_decodeSmallInteger(token->textPosition) + 1];
    beacon_ParseTreeLiteralNode_t *literal = beacon_allocateObjectWithBehavior(state->context->heap, state->context->roots.parseTreeLiteralNodeClass, sizeof(beacon_ParseTreeLiteralNode_t), BeaconObjectKindPointers);
    literal->super.sourcePosition = token->sourcePosition;
    literal->value = (beacon_oop_t)stringLiteral;
    return &literal->super;
}


beacon_ParseTreeNode_t *parser_parseLiteral(beacon_parserState_t *state)
{
    switch (parserState_peekKind(state, 0))
    {
    case BeaconTokenInteger:
        return parser_parseLiteralInteger(state);
    case BeaconTokenFloat:
        return parser_parseLiteralFloat(state);
    case BeaconTokenCharacter:
        return parser_parseLiteralCharacter(state);
    case BeaconTokenString:
        return parser_parseLiteralString(state);
    /*case BeaconTokenSymbol:
        return parseLiteralSymbol(state);*/
    default:
        return parserState_advanceWithExpectedError(state, "Expected a literal");
    }
}


beacon_ParseTreeNode_t *parser_parseExpression(beacon_parserState_t *state)
{
    return parser_parseLiteral(state);
}

beacon_ArrayList_t *parser_parseExpressionListUntilEndOrDelimiter(beacon_parserState_t *state, beacon_TokenKind_t delimiter)
{
    beacon_ArrayList_t *elements = beacon_ArrayList_new(state->context);

    // Leading dots.
    while (parserState_peekKind(state, 0) == BeaconTokenDot)
        parserState_advance(state);

    bool expectsExpression = true;

    while (!parserState_atEnd(state) && parserState_peekKind(state, 0) != delimiter)
    {
        if (!expectsExpression)
            beacon_ArrayList_add(state->context, elements, (beacon_oop_t)parserState_makeErrorAtCurrentSourcePosition(state, "Expected dot before expression."));

        beacon_ParseTreeNode_t *expression = parser_parseExpression(state);
        beacon_ArrayList_add(state->context, elements, (beacon_oop_t)expression);

        // Trailing dots.
        while (parserState_peekKind(state, 0) == BeaconTokenDot)
        {
            expectsExpression = true;
            parserState_advance(state);
        }
    }

    return elements;
}

beacon_ParseTreeNode_t *parser_parseSequenceUntilEndOrDelimiter(beacon_parserState_t *state, beacon_TokenKind_t delimiter)
{
    size_t startingPosition = state->position;
    beacon_ArrayList_t *expressions = parser_parseExpressionListUntilEndOrDelimiter(state, delimiter);
    if (beacon_ArrayList_size(expressions) == 1)
        return (beacon_ParseTreeNode_t*)beacon_ArrayList_at(state->context, expressions, 1);

    beacon_ParseTreeSequenceNode_t *sequenceNode = beacon_allocateObjectWithBehavior(state->context->heap, state->context->roots.parseTreeSequenceNodeClass, sizeof(beacon_ParseTreeSequenceNode_t), BeaconObjectKindPointers);
    sequenceNode->super.sourcePosition = parserState_sourcePositionFrom(state, startingPosition);
    sequenceNode->elements = beacon_ArrayList_asArray(state->context, expressions);
    return &sequenceNode->super;
}

beacon_ParseTreeNode_t *parser_parseTopLevelExpressions(beacon_parserState_t *state)
{
    return parser_parseSequenceUntilEndOrDelimiter(state, BeaconTokenEndOfSource);
}
beacon_ParseTreeNode_t *beacon_parseTokenList(beacon_context_t *context, beacon_SourceCode_t *sourceCode, beacon_ArrayList_t *tokenList)
{
    beacon_parserState_t state = {
        .context = context,
        .sourceCode = sourceCode,
        .tokenCount = beacon_ArrayList_size(tokenList),
        .tokens = tokenList,
        .position = 1,
    };

    return parser_parseTopLevelExpressions(&state);
}
