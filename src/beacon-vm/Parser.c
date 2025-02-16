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

beacon_ParseTreeNode_t *parser_parseExpression(beacon_parserState_t *state);
beacon_ParseTreeNode_t *parser_parseSequenceUntilEndOrDelimiter(beacon_parserState_t *state, beacon_TokenKind_t delimiter);
beacon_ArrayList_t *parser_parseExpressionListUntilEndOrDelimiter(beacon_parserState_t *state, beacon_TokenKind_t delimiter);
beacon_ParseTreeNode_t *parser_parseMethodSyntaxWithDelimiter(beacon_parserState_t *state, beacon_TokenKind_t delimiter);
beacon_ParseTreeNode_t *parser_parseLiteralArrayElement(beacon_parserState_t *state);
bool parser_isBinaryExpressionOperator(beacon_TokenKind_t kind);


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
    BeaconAssert(state->context, state->position < state->tokenCount);
    ++state->position;
}

beacon_ScannerToken_t *parserState_next(beacon_parserState_t *state)
{
    BeaconAssert(state->context, state->position < state->tokenCount);
    beacon_ScannerToken_t *token = (beacon_ScannerToken_t*)beacon_ArrayList_at(state->context, state->tokens, state->position);
    ++state->position;
    return token;
}

beacon_ParseTreeNode_t *parserState_advanceWithExpectedError(beacon_parserState_t *state, const char *message)
{
    if (parserState_peekKind(state, 0) == BeaconTokenError)
    {
        beacon_ScannerToken_t *errorToken = parserState_next(state);
        beacon_ParseTreeErrorNode_t *errorNode = beacon_allocateObjectWithBehavior(state->context->heap, state->context->classes.parseTreeErrorNodeClass, sizeof(beacon_ParseTreeNode_t), BeaconObjectKindBytes);
        errorNode->errorMessage = errorToken->errorMessage;
        return &errorNode->super;
    }
    else if (parserState_atEnd(state))
    {
        beacon_ParseTreeErrorNode_t *errorNode = beacon_allocateObjectWithBehavior(state->context->heap, state->context->classes.parseTreeErrorNodeClass, sizeof(beacon_ParseTreeNode_t), BeaconObjectKindBytes);
        errorNode->errorMessage = beacon_importCString(state->context, message);
        return &errorNode->super;
    }
    else
    {
        beacon_ParseTreeErrorNode_t *errorNode = beacon_allocateObjectWithBehavior(state->context->heap, state->context->classes.parseTreeErrorNodeClass, sizeof(beacon_ParseTreeNode_t), BeaconObjectKindBytes);
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
    beacon_ScannerToken_t *token;
    if(state->position == 1)
        token = (beacon_ScannerToken_t *)beacon_ArrayList_at(state->context, state->tokens, 1);
    else
        token = (beacon_ScannerToken_t *)beacon_ArrayList_at(state->context, state->tokens, state->position - 1);
    return token->sourcePosition;
}

beacon_SourcePosition_t *parserState_currentSourcePosition(beacon_parserState_t *state)
{
    if (state->position >= 1 && state->position < state->tokenCount)
    {
        beacon_ScannerToken_t *token = (beacon_ScannerToken_t *)beacon_ArrayList_at(state->context, state->tokens, state->position);
        return token->sourcePosition;
    }

    BeaconAssert(state->context, state->tokenCount > 0);
    beacon_ScannerToken_t *token = (beacon_ScannerToken_t *)beacon_ArrayList_at(state->context, state->tokens, state->tokenCount);
    return token->sourcePosition;
}

beacon_SourcePosition_t *parserState_sourcePositionFrom(beacon_parserState_t *state, size_t startingPosition)
{
    BeaconAssert(state->context, startingPosition <= state->tokenCount);
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
    beacon_ParseTreeErrorNode_t *errorNode = beacon_allocateObjectWithBehavior(state->context->heap, state->context->classes.parseTreeErrorNodeClass, sizeof(beacon_ParseTreeNode_t), BeaconObjectKindBytes);
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

    beacon_ParseTreeErrorNode_t *errorNode = beacon_allocateObjectWithBehavior(state->context->heap, state->context->classes.parseTreeErrorNodeClass, sizeof(beacon_ParseTreeNode_t), BeaconObjectKindBytes);
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
    BeaconAssert(state->context,beacon_decodeSmallInteger(token->kind) == BeaconTokenInteger);

    intptr_t parsedConstant = parser_parseIntegerConstant(token);
    beacon_ParseTreeLiteralNode_t *literal = beacon_allocateObjectWithBehavior(state->context->heap, state->context->classes.parseTreeLiteralNodeClass, sizeof(beacon_ParseTreeLiteralNode_t), BeaconObjectKindPointers);
    literal->super.sourcePosition = token->sourcePosition;
    literal->value = parsedConstant;
    return &literal->super;
}

beacon_ParseTreeNode_t *parser_parseLiteralFloat(beacon_parserState_t *state)
{
    beacon_ScannerToken_t *token = parserState_next(state);
    BeaconAssert(state->context, beacon_decodeSmallInteger(token->kind) == BeaconTokenInteger);

    char *literalBuffer = calloc(1, beacon_decodeSmallInteger(token->textSize) + 1);
    memcpy(literalBuffer, token->sourcePosition->sourceCode->text->data, token->textSize);
    double value = atof(literalBuffer);

    beacon_ParseTreeLiteralNode_t *literal = beacon_allocateObjectWithBehavior(state->context->heap, state->context->classes.parseTreeLiteralNodeClass, sizeof(beacon_ParseTreeLiteralNode_t), BeaconObjectKindPointers);
    literal->super.sourcePosition = token->sourcePosition;
    literal->value = beacon_encodeSmallDoubleValue(value);
    return &literal->super;
}

beacon_ParseTreeNode_t *parser_parseLiteralCharacter(beacon_parserState_t *state)
{
    beacon_ScannerToken_t *token = parserState_next(state);
    BeaconAssert(state->context, beacon_decodeSmallInteger(token->kind) == BeaconTokenCharacter);

    char parsedConstant = token->sourcePosition->sourceCode->text->data[beacon_decodeSmallInteger(token->textPosition) + 1];
    beacon_ParseTreeLiteralNode_t *literal = beacon_allocateObjectWithBehavior(state->context->heap, state->context->classes.parseTreeLiteralNodeClass, sizeof(beacon_ParseTreeLiteralNode_t), BeaconObjectKindPointers);
    literal->super.sourcePosition = token->sourcePosition;
    literal->value = beacon_encodeCharacter(parsedConstant);
    return &literal->super;
}

beacon_ParseTreeNode_t *parser_parseLiteralString(beacon_parserState_t *state)
{
    beacon_ScannerToken_t *token = parserState_next(state);
    BeaconAssert(state->context, beacon_decodeSmallInteger(token->kind) == BeaconTokenString);

    const char *textData = (const char*)token->sourcePosition->sourceCode->text->data + beacon_decodeSmallInteger(token->textPosition);
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

    beacon_String_t *stringLiteral = beacon_allocateObjectWithBehavior(state->context->heap, state->context->classes.stringClass, sizeof(beacon_String_t) + textConstantSize, BeaconObjectKindBytes);
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
        }
    }

    beacon_ParseTreeLiteralNode_t *literal = beacon_allocateObjectWithBehavior(state->context->heap, state->context->classes.parseTreeLiteralNodeClass, sizeof(beacon_ParseTreeLiteralNode_t), BeaconObjectKindPointers);
    literal->super.sourcePosition = token->sourcePosition;
    literal->value = (beacon_oop_t)stringLiteral;
    return &literal->super;
}

beacon_ParseTreeNode_t *parser_parseLiteralStringSymbol(beacon_ScannerToken_t *token, beacon_parserState_t *state)
{

    const char *textData = (const char*)token->sourcePosition->sourceCode->text->data + beacon_decodeSmallInteger(token->textPosition);
    size_t textDataSize = beacon_decodeSmallInteger(token->textSize);

    size_t textConstantSize = 0;
    for(size_t i = 2; i < textDataSize; ++i)
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

    beacon_String_t *stringLiteral = beacon_allocateObjectWithBehavior(state->context->heap, state->context->classes.stringClass, sizeof(beacon_String_t) + textConstantSize, BeaconObjectKindBytes);
    size_t destIndex = 0;
    for(size_t i = 2; i < textDataSize; ++i)
    {
        if(textData[i] == '\'')
        {
            if(i + 1 < textDataSize && textData[i+1] == '\'')
            {
                ++i;
                stringLiteral->data[destIndex++] = '\'';
            }
        }
        else
        {   
            stringLiteral->data[destIndex++] = textData[i];
        }
    }

    beacon_Symbol_t *symbolLiteral = beacon_internString(state->context, stringLiteral);
    beacon_ParseTreeLiteralNode_t *literal = beacon_allocateObjectWithBehavior(state->context->heap, state->context->classes.parseTreeLiteralNodeClass, sizeof(beacon_ParseTreeLiteralNode_t), BeaconObjectKindPointers);
    literal->super.sourcePosition = token->sourcePosition;
    literal->value = (beacon_oop_t)symbolLiteral;
    return &literal->super;
}

beacon_ParseTreeNode_t *parser_parseLiteralSymbol(beacon_parserState_t *state)
{
    beacon_ScannerToken_t *token = parserState_next(state);
    BeaconAssert(state->context, beacon_decodeSmallInteger(token->kind) == BeaconTokenSymbol);

    const char *textData = (const char*)token->sourcePosition->sourceCode->text->data + beacon_decodeSmallInteger(token->textPosition);
    intptr_t textDataSize = beacon_decodeSmallInteger(token->textSize);
    if(textDataSize >= 2 && textData[0] == '#' && textData[1] == '\'')
        return parser_parseLiteralStringSymbol(token, state);

    intptr_t symbolSize = textDataSize - 1;
    beacon_String_t *stringLiteral = beacon_allocateObjectWithBehavior(state->context->heap, state->context->classes.stringClass, sizeof(beacon_String_t) + symbolSize, BeaconObjectKindBytes);
    memcpy(stringLiteral->data, textData + 1, symbolSize);
    
    beacon_Symbol_t *symbolLiteral = beacon_internString(state->context, stringLiteral);
    beacon_ParseTreeLiteralNode_t *literal = beacon_allocateObjectWithBehavior(state->context->heap, state->context->classes.parseTreeLiteralNodeClass, sizeof(beacon_ParseTreeLiteralNode_t), BeaconObjectKindPointers);
    literal->super.sourcePosition = token->sourcePosition;
    literal->value = (beacon_oop_t)symbolLiteral;
    return &literal->super;
}

beacon_ParseTreeNode_t *parser_parseLiteralKeyword(beacon_parserState_t *state)
{
    beacon_ScannerToken_t *token = parserState_next(state);
    BeaconAssert(state->context,
        beacon_decodeSmallInteger(token->kind) == BeaconTokenKeyword ||
        beacon_decodeSmallInteger(token->kind) == BeaconTokenMultiKeyword ||
        parser_isBinaryExpressionOperator(beacon_decodeSmallInteger(token->kind)));

    const char *textData = (const char*)token->sourcePosition->sourceCode->text->data + beacon_decodeSmallInteger(token->textPosition);
    intptr_t textDataSize = beacon_decodeSmallInteger(token->textSize);

    beacon_Symbol_t *symbolLiteral = beacon_internStringWithSize(state->context, textDataSize, textData);

    beacon_ParseTreeLiteralNode_t *literal = beacon_allocateObjectWithBehavior(state->context->heap, state->context->classes.parseTreeLiteralNodeClass, sizeof(beacon_ParseTreeLiteralNode_t), BeaconObjectKindPointers);
    literal->super.sourcePosition = token->sourcePosition;
    literal->value = (beacon_oop_t)symbolLiteral;
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
    case BeaconTokenSymbol:
        return parser_parseLiteralSymbol(state);
    default:
        return parserState_advanceWithExpectedError(state, "Expected a literal");
    }
}

beacon_ParseTreeNode_t *parser_parseIdentifier(beacon_parserState_t *state)
{
    beacon_ScannerToken_t *token = parserState_next(state);
    BeaconAssert(state->context, beacon_decodeSmallInteger(token->kind) == BeaconTokenIdentifier);

    const char *textData = (const char*)token->sourcePosition->sourceCode->text->data + beacon_decodeSmallInteger(token->textPosition);
    intptr_t textDataSize = beacon_decodeSmallInteger(token->textSize);
    
    beacon_String_t *stringLiteral = beacon_allocateObjectWithBehavior(state->context->heap, state->context->classes.stringClass, sizeof(beacon_String_t) + textDataSize, BeaconObjectKindBytes);
    memcpy(stringLiteral->data, textData, textDataSize);

    beacon_Symbol_t *symbol = beacon_internString(state->context, stringLiteral);

    beacon_ParseTreeIdentifierReferenceNode_t *identifierReference = beacon_allocateObjectWithBehavior(state->context->heap, state->context->classes.parseTreeIdentifierReferenceNodeClass, sizeof(beacon_ParseTreeIdentifierReferenceNode_t), BeaconObjectKindPointers);
    identifierReference->super.sourcePosition = token->sourcePosition;
    identifierReference->identifier = (beacon_oop_t)symbol;
    return &identifierReference->super;
}

beacon_ParseTreeNode_t *parser_parseParenthesis(beacon_parserState_t *state)
{
    beacon_ScannerToken_t *token = parserState_next(state);
    BeaconAssert(state->context, beacon_decodeSmallInteger(token->kind) == BeaconTokenLeftParent);
    
    beacon_ParseTreeNode_t *expression = parser_parseSequenceUntilEndOrDelimiter(state, BeaconTokenRightParent);
    expression = parserState_expectAddingErrorToNode(state, BeaconTokenRightParent, expression);
    return expression;
}

beacon_Array_t *parser_parseDelimitedListOfTemporaries(beacon_parserState_t *state)
{
    if(parserState_peekKind(state, 0) != BeaconTokenBar)
        return (beacon_Array_t*)state->context->roots.emptyArray;

    beacon_ArrayList_t *localVariables = beacon_ArrayList_new(state->context);
    if (parserState_peekKind(state, 0) == BeaconTokenBar)
    {
        parserState_advance(state);
        while (parserState_peekKind(state, 0) == BeaconTokenIdentifier)
        {
            beacon_ScannerToken_t *localNameToken = parserState_next(state);
            const char *textData = (const char*)localNameToken->sourcePosition->sourceCode->text->data + beacon_decodeSmallInteger(localNameToken->textPosition);
            intptr_t textDataSize = beacon_decodeSmallInteger(localNameToken->textSize);
            beacon_Symbol_t *localName = beacon_internStringWithSize(state->context, textDataSize, textData);

            beacon_ParseTreeLocalVariableDefinitionNode_t *localDefinition = beacon_allocateObjectWithBehavior(state->context->heap, state->context->classes.parseTreeLocalVariableDefinitionNodeClass, sizeof(beacon_ParseTreeLocalVariableDefinitionNode_t), BeaconObjectKindPointers);
            localDefinition->super.sourcePosition = localNameToken->sourcePosition;
            localDefinition->name = localName;

            beacon_ArrayList_add(state->context, localVariables, (beacon_oop_t)localDefinition);
        }

        if(parserState_peekKind(state, 0) == BeaconTokenBar)
        {
            parserState_advance(state);
        }
        else
        {
            beacon_ArrayList_add(state->context, localVariables, (beacon_oop_t)parserState_advanceWithExpectedError(state, "Expected a bar after the local variable definitions."));
        }
    }

    return beacon_ArrayList_asArray(state->context, localVariables);
}

beacon_ParseTreeNode_t *parser_parseBlockClosure(beacon_parserState_t *state)
{
    size_t startingPosition = state->position;
    beacon_ScannerToken_t *token = parserState_next(state);
    BeaconAssert(state->context, beacon_decodeSmallInteger(token->kind) == BeaconTokenLeftBracket);

    beacon_ArrayList_t *arguments = beacon_ArrayList_new(state->context);
    bool hasArguments = false;
    while (parserState_peekKind(state, 0) == BeaconTokenColon)
    {
        hasArguments = true;
        parserState_advance(state);
        beacon_ScannerToken_t *argumentNameToken = parserState_next(state);
        BeaconAssert(state->context, beacon_decodeSmallInteger(argumentNameToken->kind) == BeaconTokenIdentifier);
        
        const char *textData = (const char*)argumentNameToken->sourcePosition->sourceCode->text->data + beacon_decodeSmallInteger(argumentNameToken->textPosition);
        intptr_t textDataSize = beacon_decodeSmallInteger(argumentNameToken->textSize);
        beacon_Symbol_t *argumentName = beacon_internStringWithSize(state->context, textDataSize, textData);

        beacon_ParseTreeArgumentDefinitionNode_t *argumentDefinition = beacon_allocateObjectWithBehavior(state->context->heap, state->context->classes.parseTreeArgumentDefinitionNodeClass, sizeof(beacon_ParseTreeArgumentDefinitionNode_t), BeaconObjectKindPointers);
        argumentDefinition->super.sourcePosition = argumentNameToken->sourcePosition;
        argumentDefinition->name = argumentName;

        beacon_ArrayList_add(state->context, arguments, (beacon_oop_t)argumentDefinition);
    }

    if (parserState_peekKind(state, 0) == BeaconTokenBar)
    {
        parserState_advance(state);
    }
    else if(hasArguments)
    {
        beacon_ArrayList_add(state->context, arguments, (beacon_oop_t)parserState_advanceWithExpectedError(state, "Expected a bar | after the argument names"));
    }

    // Local variables
    beacon_Array_t *localVariables = parser_parseDelimitedListOfTemporaries(state);
    
    // Body expression
    beacon_ParseTreeNode_t *expression = parser_parseSequenceUntilEndOrDelimiter(state, BeaconTokenRightBracket);
    expression = parserState_expectAddingErrorToNode(state, BeaconTokenRightBracket, expression);

    // Emit the block closure
    beacon_ParseTreeBlockClosureNode_t *blockClosure = beacon_allocateObjectWithBehavior(state->context->heap, state->context->classes.parseTreeBlockClosureNodeClass, sizeof(beacon_ParseTreeBlockClosureNode_t), BeaconObjectKindPointers);
    blockClosure->super.sourcePosition = parserState_sourcePositionFrom(state, startingPosition);
    blockClosure->arguments = beacon_ArrayList_asArray(state->context, arguments);
    blockClosure->localVariables = localVariables;
    blockClosure->expression = expression;

    return &blockClosure->super;
}

beacon_ParseTreeNode_t *parser_parseMethodBlock(beacon_parserState_t *state)
{
    size_t startingPosition = state->position;
    beacon_ScannerToken_t *token = parserState_next(state);
    BeaconAssert(state->context, beacon_decodeSmallInteger(token->kind) == BeaconTokenBangLeftBracket);


    beacon_ParseTreeNode_t *methodNode = parser_parseMethodSyntaxWithDelimiter(state, BeaconTokenRightBracket);
    methodNode = parserState_expectAddingErrorToNode(state, BeaconTokenRightBracket, methodNode);

    methodNode->sourcePosition = parserState_sourcePositionFrom(state, startingPosition);
    return methodNode;
}

beacon_ParseTreeNode_t *parser_parseByteArray(beacon_parserState_t *state)
{
    size_t startingPosition = state->position;
    beacon_ScannerToken_t *token = parserState_next(state);
    BeaconAssert(state->context, beacon_decodeSmallInteger(token->kind) == BeaconTokenByteArrayStart);

    beacon_ArrayList_t *byteArrayElements = beacon_ArrayList_new(state->context);
    while(!parserState_atEnd(state) && parserState_peekKind(state, 0) != BeaconTokenRightBracket)
    {   
        if(parserState_peekKind(state, 0) == BeaconTokenInteger)
        {
            beacon_ParseTreeNode_t *integerLiteral = parser_parseLiteralInteger(state);
            beacon_ArrayList_add(state->context, byteArrayElements, (beacon_oop_t)integerLiteral);
        }
        else
        {
            beacon_ArrayList_add(state->context, byteArrayElements, (beacon_oop_t)parserState_advanceWithExpectedError(state, "Expected only integers in the byte array"));
        }
    }

    beacon_ParseTreeByteArrayNode_t *byteArrayNode = beacon_allocateObjectWithBehavior(state->context->heap, state->context->classes.parseTreeByteArrayNodeClass, sizeof(beacon_ParseTreeByteArrayNode_t), BeaconObjectKindPointers);
    byteArrayNode->super.sourcePosition = parserState_sourcePositionFrom(state, startingPosition);
    byteArrayNode->elements = beacon_ArrayList_asArray(state->context, byteArrayElements);

    beacon_ParseTreeNode_t *expression = parserState_expectAddingErrorToNode(state, BeaconTokenRightBracket, &byteArrayNode->super);
    return expression;
}

beacon_ParseTreeNode_t *parser_parseArray(beacon_parserState_t *state)
{
    size_t startingPosition = state->position;
    beacon_ScannerToken_t *token = parserState_next(state);
    BeaconAssert(state->context, beacon_decodeSmallInteger(token->kind) == BeaconTokenLeftCurlyBracket);

    beacon_ArrayList_t *arrayList = parser_parseExpressionListUntilEndOrDelimiter(state, BeaconTokenRightCurlyBracket);

    beacon_ParseTreeArrayNode_t *arrayNode = beacon_allocateObjectWithBehavior(state->context->heap, state->context->classes.parseTreeArrayNodeClass, sizeof(beacon_ParseTreeArrayNode_t), BeaconObjectKindPointers);
    
    arrayNode->super.sourcePosition = parserState_sourcePositionFrom(state, startingPosition);
    arrayNode->elements = beacon_ArrayList_asArray(state->context, arrayList);

    beacon_ParseTreeNode_t *expression = parserState_expectAddingErrorToNode(state, BeaconTokenRightCurlyBracket, &arrayNode->super);
    return expression;
}

beacon_ParseTreeNode_t *parser_parseLiteralArray(beacon_parserState_t *state)
{
    size_t startingPosition = state->position;
    beacon_ScannerToken_t *token = parserState_next(state);
    BeaconAssert(state->context, beacon_decodeSmallInteger(token->kind) == BeaconTokenLiteralArrayStart ||
                                beacon_decodeSmallInteger(token->kind) == BeaconTokenLeftParent);

    beacon_ArrayList_t *literalArrayElements = beacon_ArrayList_new(state->context);
    while(!parserState_atEnd(state) && parserState_peekKind(state, 0) != BeaconTokenRightParent)
    {   
        beacon_ParseTreeNode_t *element = parser_parseLiteralArrayElement(state);
        beacon_ArrayList_add(state->context, literalArrayElements, (beacon_oop_t)element);
    }

    beacon_ParseTreeLiteralArrayNode_t *literalArrayNode = beacon_allocateObjectWithBehavior(state->context->heap, state->context->classes.parseTreeLiteralArrayNodeClass, sizeof(beacon_ParseTreeLiteralArrayNode_t), BeaconObjectKindPointers);
    literalArrayNode->super.sourcePosition = parserState_sourcePositionFrom(state, startingPosition);
    literalArrayNode->elements = beacon_ArrayList_asArray(state->context, literalArrayElements);

    beacon_ParseTreeNode_t *expression = parserState_expectAddingErrorToNode(state, BeaconTokenRightParent, &literalArrayNode->super);
    return expression;
}

beacon_ParseTreeNode_t *parser_parseLiteralArrayElement(beacon_parserState_t *state)
{
    switch (parserState_peekKind(state, 0))
    {
    case BeaconTokenIdentifier:
        return parser_parseIdentifier(state);
    case BeaconTokenByteArrayStart:
        return parser_parseByteArray(state);
    case BeaconTokenKeyword:
    case BeaconTokenMultiKeyword:
        return parser_parseLiteralKeyword(state);
    case BeaconTokenLeftParent:
    case BeaconTokenLiteralArrayStart:
        return parser_parseLiteralArray(state);
    default:
        return parser_parseLiteral(state);
    }
}

beacon_ParseTreeNode_t *parser_parseTerm(beacon_parserState_t *state)
{
    switch (parserState_peekKind(state, 0))
    {
    case BeaconTokenIdentifier:
        return parser_parseIdentifier(state);
    case BeaconTokenLeftParent:
        return parser_parseParenthesis(state);
    case BeaconTokenLeftBracket:
        return parser_parseBlockClosure(state);
    case BeaconTokenBangLeftBracket:
        return parser_parseMethodBlock(state);
    case BeaconTokenLeftCurlyBracket:
        return parser_parseArray(state);
    case BeaconTokenByteArrayStart:
        return parser_parseByteArray(state);
    case BeaconTokenLiteralArrayStart:
        return parser_parseLiteralArray(state);
    default:
        return parser_parseLiteral(state);
    }
}

bool parser_isBinaryExpressionOperator(beacon_TokenKind_t kind)
{
    switch (kind)
    {
    case BeaconTokenOperator:
    case BeaconTokenLessThan:
    case BeaconTokenGreaterThan:
    case BeaconTokenBar:
        return true;
    default:
        return false;
    }
}

beacon_ParseTreeNode_t *parser_parseUnaryPostfixExpression(beacon_parserState_t *state)
{
    size_t startPosition = state->position;
    beacon_ParseTreeNode_t *receiver = parser_parseTerm(state);

    while(parserState_peekKind(state, 0) == BeaconTokenIdentifier ||
        parserState_peekKind(state, 0) == BeaconTokenBangLeftBracket)
    {
        if(parserState_peekKind(state, 0) == BeaconTokenBangLeftBracket)
        {
            beacon_ParseTreeNode_t *method = parser_parseMethodBlock(state);
            beacon_ParseTreeAddMethod_t *addMethod = beacon_allocateObjectWithBehavior(state->context->heap, state->context->classes.parseTreeAddMethodNodeClass, sizeof(beacon_ParseTreeAddMethod_t), BeaconObjectKindPointers);
            addMethod->super.sourcePosition = parserState_sourcePositionFrom(state, startPosition);
            addMethod->behavior = receiver;
            addMethod->method = method;
            receiver = &addMethod->super;
            continue;            
        }

        beacon_ScannerToken_t *unaryToken = parserState_next(state);
        
        const char *textData = (const char*)unaryToken->sourcePosition->sourceCode->text->data + beacon_decodeSmallInteger(unaryToken->textPosition);
        intptr_t textDataSize = beacon_decodeSmallInteger(unaryToken->textSize);

        beacon_String_t *stringLiteral = beacon_allocateObjectWithBehavior(state->context->heap, state->context->classes.stringClass, sizeof(beacon_String_t) + textDataSize, BeaconObjectKindBytes);
        memcpy(stringLiteral->data, textData, textDataSize);

        beacon_Symbol_t *selectorSymbol = beacon_internString(state->context, stringLiteral);
        beacon_ParseTreeLiteralNode_t *selectorLiteral = beacon_allocateObjectWithBehavior(state->context->heap, state->context->classes.parseTreeLiteralNodeClass, sizeof(beacon_ParseTreeLiteralNode_t), BeaconObjectKindPointers);
        selectorLiteral->super.sourcePosition = unaryToken->sourcePosition;
        selectorLiteral->value = (beacon_oop_t)selectorSymbol;

        beacon_Array_t *argumentsArray = beacon_allocateObjectWithBehavior(state->context->heap, state->context->classes.arrayClass, sizeof(beacon_Array_t), BeaconObjectKindPointers);;

        beacon_ParseTreeMessageSendNode_t *messageSend = beacon_allocateObjectWithBehavior(state->context->heap, state->context->classes.parseTreeMessageSendNodeClass, sizeof(beacon_ParseTreeMessageSendNode_t), BeaconObjectKindPointers);
        messageSend->super.sourcePosition = parserState_sourcePositionFrom(state, startPosition);
        messageSend->receiver = receiver;
        messageSend->selector = &selectorLiteral->super;
        messageSend->arguments = argumentsArray;

        receiver = &messageSend->super;
    }

    return receiver;
}

beacon_ParseTreeNode_t *parser_parseBinaryExpressionSequence(beacon_parserState_t *state)
{
    size_t startPosition = state->position;
    beacon_ParseTreeNode_t *receiver = parser_parseUnaryPostfixExpression(state);

    while(parser_isBinaryExpressionOperator(parserState_peekKind(state, 0)))
    {
        beacon_ScannerToken_t *operatorToken = parserState_next(state);
        
        const char *textData = (const char*)operatorToken->sourcePosition->sourceCode->text->data + beacon_decodeSmallInteger(operatorToken->textPosition);
        intptr_t textDataSize = beacon_decodeSmallInteger(operatorToken->textSize);

        beacon_String_t *stringLiteral = beacon_allocateObjectWithBehavior(state->context->heap, state->context->classes.stringClass, sizeof(beacon_String_t) + textDataSize, BeaconObjectKindBytes);
        memcpy(stringLiteral->data, textData, textDataSize);

        beacon_Symbol_t *operatorSymbol = beacon_internString(state->context, stringLiteral);
        beacon_ParseTreeLiteralNode_t *operatorLiteral = beacon_allocateObjectWithBehavior(state->context->heap, state->context->classes.parseTreeLiteralNodeClass, sizeof(beacon_ParseTreeLiteralNode_t), BeaconObjectKindPointers);
        operatorLiteral->super.sourcePosition = operatorToken->sourcePosition;
        operatorLiteral->value = (beacon_oop_t)operatorSymbol;

        beacon_ParseTreeNode_t *rightOperand = parser_parseUnaryPostfixExpression(state);
        beacon_Array_t *argumentsArray = beacon_allocateObjectWithBehavior(state->context->heap, state->context->classes.arrayClass, sizeof(beacon_Array_t) + sizeof(beacon_oop_t), BeaconObjectKindPointers);
        argumentsArray->elements[0] = (beacon_oop_t)rightOperand;

        beacon_ParseTreeMessageSendNode_t *messageSend = beacon_allocateObjectWithBehavior(state->context->heap, state->context->classes.parseTreeMessageSendNodeClass, sizeof(beacon_ParseTreeMessageSendNode_t), BeaconObjectKindPointers);
        messageSend->super.sourcePosition = parserState_sourcePositionFrom(state, startPosition);
        messageSend->receiver = receiver;
        messageSend->selector = &operatorLiteral->super;
        messageSend->arguments = argumentsArray;

        receiver = &messageSend->super;
    }

    return receiver;
}

beacon_ParseTreeNode_t *parser_parseCascadedMessage(beacon_parserState_t *state)
{
    if(parserState_peekKind(state, 0) != BeaconTokenKeyword 
    && parserState_peekKind(state, 0) != BeaconTokenIdentifier
    && !parser_isBinaryExpressionOperator(parserState_peekKind(state, 0)))
       return parserState_advanceWithExpectedError(state, "Expected a keyword or cascaded message.");

    size_t startPosition = state->position;
    beacon_ArrayList_t *keywords = beacon_ArrayList_new(state->context);
    beacon_ArrayList_t *arguments = beacon_ArrayList_new(state->context);
    size_t selectorSize = 0;

    // Parse unary
    if(parserState_peekKind(state, 0) == BeaconTokenIdentifier)
    {
        beacon_ScannerToken_t *unaryToken = parserState_next(state);
        beacon_Symbol_t *selector = beacon_internStringWithSize(state->context, (size_t)beacon_decodeSmallInteger(unaryToken->textSize), (char*)unaryToken->sourcePosition->sourceCode->text->data + beacon_decodeSmallInteger(unaryToken->textPosition));

        beacon_ParseTreeLiteralNode_t *selectorNode = beacon_allocateObjectWithBehavior(state->context->heap, state->context->classes.parseTreeLiteralNodeClass, sizeof(beacon_ParseTreeLiteralNode_t), BeaconObjectKindPointers);
        selectorNode->super.sourcePosition = unaryToken->sourcePosition;
        selectorNode->value = (beacon_oop_t)selector;

        beacon_ParseTreeCascadedMessageNode_t *cascadedMessage = beacon_allocateObjectWithBehavior(state->context->heap, state->context->classes.parseTreeCascadedMessageNodeClass, sizeof(beacon_ParseTreeCascadedMessageNode_t), BeaconObjectKindPointers);
        cascadedMessage->super.sourcePosition = parserState_sourcePositionFrom(state, startPosition);
        cascadedMessage->selector = &selectorNode->super;
        cascadedMessage->arguments = (beacon_Array_t*)state->context->roots.emptyArray;

        return &cascadedMessage->super;
    }

    // Parse binary
    if(parser_isBinaryExpressionOperator(parserState_peekKind(state, 0)))
    {
        beacon_ScannerToken_t *operatorToken = parserState_next(state);
        beacon_Symbol_t *selector = beacon_internStringWithSize(state->context, (size_t)beacon_decodeSmallInteger(operatorToken->textSize), (char*)operatorToken->sourcePosition->sourceCode->text->data + beacon_decodeSmallInteger(operatorToken->textPosition));

        beacon_ParseTreeLiteralNode_t *selectorNode = beacon_allocateObjectWithBehavior(state->context->heap, state->context->classes.parseTreeLiteralNodeClass, sizeof(beacon_ParseTreeLiteralNode_t), BeaconObjectKindPointers);
        selectorNode->super.sourcePosition = operatorToken->sourcePosition;
        selectorNode->value = (beacon_oop_t)selector;

        beacon_ParseTreeNode_t *argument = parser_parseBinaryExpressionSequence(state);
        beacon_Array_t *arguments = beacon_allocateObjectWithBehavior(state->context->heap, state->context->classes.arrayClass, sizeof(beacon_Array_t) + sizeof(beacon_oop_t), BeaconObjectKindPointers);
        arguments->elements[0] = (beacon_oop_t)argument;

        beacon_ParseTreeCascadedMessageNode_t *cascadedMessage = beacon_allocateObjectWithBehavior(state->context->heap, state->context->classes.parseTreeCascadedMessageNodeClass, sizeof(beacon_ParseTreeCascadedMessageNode_t), BeaconObjectKindPointers);
        cascadedMessage->super.sourcePosition = parserState_sourcePositionFrom(state, startPosition);
        cascadedMessage->selector = &selectorNode->super;
        cascadedMessage->arguments = arguments;

        return &cascadedMessage->super;
    }

    // Parse the keywords and arguments.
    while(parserState_peekKind(state, 0) == BeaconTokenKeyword)
    {
        beacon_ScannerToken_t *keywordToken = parserState_next(state);
        selectorSize += beacon_decodeSmallInteger(keywordToken->textSize);
        beacon_ArrayList_add(state->context, keywords, (beacon_oop_t)keywordToken);

        beacon_ParseTreeNode_t *argument = parser_parseBinaryExpressionSequence(state);
        beacon_ArrayList_add(state->context, arguments, (beacon_oop_t)argument);
    }

    // Reconstruct the selector;
    beacon_String_t *selectorString = beacon_allocateObjectWithBehavior(state->context->heap, state->context->classes.stringClass, sizeof(beacon_String_t) + selectorSize, BeaconObjectKindBytes);
    size_t destIndex = 0;
    intptr_t keywordCount = beacon_ArrayList_size(keywords);
    for(intptr_t i = 1; i <= keywordCount; ++i)
    {
        beacon_ScannerToken_t *keywordToken = (beacon_ScannerToken_t*)beacon_ArrayList_at(state->context, keywords, i);
        intptr_t keywordSize = beacon_decodeSmallInteger(keywordToken->textSize);
        intptr_t keywordPosition = beacon_decodeSmallInteger(keywordToken->textPosition);
        for(intptr_t j = 0; j < keywordSize; ++j)
        {
            char c = keywordToken->sourcePosition->sourceCode->text->data[keywordPosition + j];
            selectorString->data[destIndex++] = c;
        }
    }

    beacon_ParseTreeLiteralNode_t *selectorLiteral = beacon_allocateObjectWithBehavior(state->context->heap, state->context->classes.parseTreeLiteralNodeClass, sizeof(beacon_ParseTreeLiteralNode_t), BeaconObjectKindPointers);
    selectorLiteral->super.sourcePosition = parserState_sourcePositionFrom(state, startPosition);
    selectorLiteral->value = (beacon_oop_t)beacon_internString(state->context, selectorString);

    // Make the cascaded message.
    beacon_ParseTreeCascadedMessageNode_t *cascadedMessage = beacon_allocateObjectWithBehavior(state->context->heap, state->context->classes.parseTreeCascadedMessageNodeClass, sizeof(beacon_ParseTreeCascadedMessageNode_t), BeaconObjectKindPointers);
    cascadedMessage->super.sourcePosition = parserState_sourcePositionFrom(state, startPosition);
    cascadedMessage->selector = &selectorLiteral->super;
    cascadedMessage->arguments = beacon_ArrayList_asArray(state->context, arguments);

    return &cascadedMessage->super;
}

beacon_ParseTreeNode_t *parser_parseKeywordMessageSend(beacon_parserState_t *state)
{
    size_t startPosition = state->position;
    beacon_ParseTreeNode_t *receiver = parser_parseBinaryExpressionSequence(state);
    beacon_ParseTreeNode_t *firstCascaded = NULL;

    if (parserState_peekKind(state, 0) == BeaconTokenKeyword)
    {
        firstCascaded = parser_parseCascadedMessage(state);
    }

    if (parserState_peekKind(state, 0) == BeaconTokenSemicolon)
    {
        beacon_ArrayList_t *cascadedMessages = beacon_ArrayList_new(state->context);
        if(firstCascaded)
            beacon_ArrayList_add(state->context, cascadedMessages, (beacon_oop_t)firstCascaded);
        while (parserState_peekKind(state, 0) == BeaconTokenSemicolon)
        {
            parserState_advance(state);
            beacon_ParseTreeNode_t *cascadedMessage = parser_parseCascadedMessage(state);
            beacon_ArrayList_add(state->context, cascadedMessages, (beacon_oop_t)cascadedMessage);
        }

        beacon_ParseTreeMessageCascadeNode_t *messageCascade = beacon_allocateObjectWithBehavior(state->context->heap, state->context->classes.parseTreeMessageCascadeNodeClass, sizeof(beacon_ParseTreeMessageCascadeNode_t), BeaconObjectKindPointers);
        messageCascade->super.sourcePosition = parserState_sourcePositionFrom(state, startPosition);
        messageCascade->receiver = receiver;
        messageCascade->cascadedMessages = beacon_ArrayList_asArray(state->context, cascadedMessages);
        return &messageCascade->super;

    }
    else if(firstCascaded)
    {
        beacon_ParseTreeCascadedMessageNode_t *onlyCascadedMessage = (beacon_ParseTreeCascadedMessageNode_t*)firstCascaded;

        beacon_ParseTreeMessageSendNode_t *messageSend = beacon_allocateObjectWithBehavior(state->context->heap, state->context->classes.parseTreeMessageSendNodeClass, sizeof(beacon_ParseTreeMessageSendNode_t), BeaconObjectKindPointers);
        messageSend->super.sourcePosition = parserState_sourcePositionFrom(state, startPosition);
        messageSend->receiver = receiver;
        messageSend->selector = onlyCascadedMessage->selector;
        messageSend->arguments = onlyCascadedMessage->arguments;
        return &messageSend->super;
    }
    else
    {
        return receiver;
    }

}

beacon_ParseTreeNode_t *parser_assignmentExpression(beacon_parserState_t *state)
{
    size_t startingPosition = state->position;
    beacon_ParseTreeNode_t *storage = parser_parseKeywordMessageSend(state);
    if(parserState_peekKind(state, 0) != BeaconTokenAssignment)
        return storage;
    parserState_advance(state);

    beacon_ParseTreeNode_t *value = parser_assignmentExpression(state);

    beacon_ParseTreeAssignment_t *assignment = beacon_allocateObjectWithBehavior(state->context->heap, state->context->classes.parseTreeAssignmentNodeClass, sizeof(beacon_ParseTreeAssignment_t), BeaconObjectKindPointers);
    assignment->super.sourcePosition = parserState_sourcePositionFrom(state, startingPosition);
    assignment->storage = storage;
    assignment->valueToStore = value;
    return &assignment->super;
}

beacon_ParseTreeNode_t *parser_parseExpression(beacon_parserState_t *state)
{
    if (parserState_peekKind(state, 0) == BeaconTokenCaret)
    {
        size_t startingPosition = state->position;
        parserState_advance(state);
        beacon_ParseTreeNode_t *resultValue = parser_assignmentExpression(state);

        beacon_ParseTreeReturnNode_t *returnNode = beacon_allocateObjectWithBehavior(state->context->heap, state->context->classes.parseTreeReturnNodeClass, sizeof(beacon_ParseTreeReturnNode_t), BeaconObjectKindPointers);
        returnNode->super.sourcePosition = parserState_sourcePositionFrom(state, startingPosition);
        returnNode->expression = resultValue;
        return &returnNode->super;

    }
    return parser_assignmentExpression(state);
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

    beacon_ParseTreeSequenceNode_t *sequenceNode = beacon_allocateObjectWithBehavior(state->context->heap, state->context->classes.parseTreeSequenceNodeClass, sizeof(beacon_ParseTreeSequenceNode_t), BeaconObjectKindPointers);
    sequenceNode->super.sourcePosition = parserState_sourcePositionFrom(state, startingPosition);
    sequenceNode->elements = beacon_ArrayList_asArray(state->context, expressions);
    return &sequenceNode->super;
}

beacon_ParseTreeMethodNode_t *parser_parseMethodHeader(beacon_parserState_t *state)
{

    size_t startPosition = state->position;

    // Parse unary
    if(parserState_peekKind(state, 0) == BeaconTokenIdentifier)
    {
        beacon_ScannerToken_t *unaryToken = parserState_next(state);
        beacon_Symbol_t *selector = beacon_internStringWithSize(state->context, (size_t)beacon_decodeSmallInteger(unaryToken->textSize), (char*)unaryToken->sourcePosition->sourceCode->text->data + beacon_decodeSmallInteger(unaryToken->textPosition));

        beacon_ParseTreeMethodNode_t *methodNode = beacon_allocateObjectWithBehavior(state->context->heap, state->context->classes.parseTreeMethodNode, sizeof(beacon_ParseTreeMethodNode_t), BeaconObjectKindPointers);
        methodNode->selector = selector;
        methodNode->arguments = (beacon_Array_t*)state->context->roots.emptyArray;
        methodNode->localVariables = (beacon_Array_t*)state->context->roots.emptyArray;
        methodNode->super.sourcePosition = parserState_sourcePositionFrom(state, startPosition);
        return methodNode;
    }

    // Parse binary
    if(parser_isBinaryExpressionOperator(parserState_peekKind(state, 0)))
    {
        beacon_ScannerToken_t *operatorToken = parserState_next(state);
        beacon_Symbol_t *selector = beacon_internStringWithSize(state->context, (size_t)beacon_decodeSmallInteger(operatorToken->textSize), (char*)operatorToken->sourcePosition->sourceCode->text->data + beacon_decodeSmallInteger(operatorToken->textPosition));

        beacon_ScannerToken_t *argumentNameToken = parserState_next(state);
        BeaconAssert(state->context, beacon_decodeSmallInteger(argumentNameToken->kind) == BeaconTokenIdentifier);
        
        const char *textData = (const char*)argumentNameToken->sourcePosition->sourceCode->text->data + beacon_decodeSmallInteger(argumentNameToken->textPosition);
        intptr_t textDataSize = beacon_decodeSmallInteger(argumentNameToken->textSize);
        beacon_Symbol_t *argumentName = beacon_internStringWithSize(state->context, textDataSize, textData);

        beacon_ParseTreeArgumentDefinitionNode_t *argumentDefinition = beacon_allocateObjectWithBehavior(state->context->heap, state->context->classes.parseTreeArgumentDefinitionNodeClass, sizeof(beacon_ParseTreeArgumentDefinitionNode_t), BeaconObjectKindPointers);
        argumentDefinition->super.sourcePosition = argumentNameToken->sourcePosition;
        argumentDefinition->name = argumentName;

        beacon_Array_t *argumentDefinitionArray = beacon_allocateObjectWithBehavior(state->context->heap, state->context->classes.arrayClass, sizeof(beacon_Array_t) + sizeof(beacon_oop_t), BeaconObjectKindPointers);
        argumentDefinitionArray->elements[0] = (beacon_oop_t)argumentDefinition;

        beacon_ParseTreeMethodNode_t *methodNode = beacon_allocateObjectWithBehavior(state->context->heap, state->context->classes.parseTreeMethodNode, sizeof(beacon_ParseTreeMethodNode_t), BeaconObjectKindPointers);
        methodNode->selector = selector;
        methodNode->arguments = argumentDefinitionArray;
        methodNode->localVariables = (beacon_Array_t*)state->context->roots.emptyArray;
        methodNode->super.sourcePosition = parserState_sourcePositionFrom(state, startPosition);
        return methodNode;
    }

    // Parse the keywords and arguments.
    beacon_ArrayList_t *keywords = beacon_ArrayList_new(state->context);
    beacon_ArrayList_t *arguments = beacon_ArrayList_new(state->context);
    size_t selectorSize = 0;

    while(parserState_peekKind(state, 0) == BeaconTokenKeyword)
    {
        beacon_ScannerToken_t *keywordToken = parserState_next(state);
        selectorSize += beacon_decodeSmallInteger(keywordToken->textSize);
        beacon_ArrayList_add(state->context, keywords, (beacon_oop_t)keywordToken);

        beacon_ScannerToken_t *argumentNameToken = parserState_next(state);
        BeaconAssert(state->context, beacon_decodeSmallInteger(argumentNameToken->kind) == BeaconTokenIdentifier);
        
        const char *textData = (const char*)argumentNameToken->sourcePosition->sourceCode->text->data + beacon_decodeSmallInteger(argumentNameToken->textPosition);
        intptr_t textDataSize = beacon_decodeSmallInteger(argumentNameToken->textSize);
        beacon_Symbol_t *argumentName = beacon_internStringWithSize(state->context, textDataSize, textData);

        beacon_ParseTreeArgumentDefinitionNode_t *argumentDefinition = beacon_allocateObjectWithBehavior(state->context->heap, state->context->classes.parseTreeArgumentDefinitionNodeClass, sizeof(beacon_ParseTreeArgumentDefinitionNode_t), BeaconObjectKindPointers);
        argumentDefinition->super.sourcePosition = argumentNameToken->sourcePosition;
        argumentDefinition->name = argumentName;

        beacon_ArrayList_add(state->context, arguments,  (beacon_oop_t)argumentDefinition);
    }

    // Reconstruct the selector.
    beacon_String_t *selectorString = beacon_allocateObjectWithBehavior(state->context->heap, state->context->classes.stringClass, sizeof(beacon_String_t) + selectorSize, BeaconObjectKindBytes);
    size_t destIndex = 0;
    intptr_t keywordCount = beacon_ArrayList_size(keywords);
    for(intptr_t i = 1; i <= keywordCount; ++i)
    {
        beacon_ScannerToken_t *keywordToken = (beacon_ScannerToken_t*)beacon_ArrayList_at(state->context, keywords, i);
        intptr_t keywordSize = beacon_decodeSmallInteger(keywordToken->textSize);
        intptr_t keywordPosition = beacon_decodeSmallInteger(keywordToken->textPosition);
        for(intptr_t j = 0; j < keywordSize; ++j)
        {
            char c = keywordToken->sourcePosition->sourceCode->text->data[keywordPosition + j];
            selectorString->data[destIndex++] = c;
        }
    }

    beacon_ParseTreeMethodNode_t *methodNode = beacon_allocateObjectWithBehavior(state->context->heap, state->context->classes.parseTreeMethodNode, sizeof(beacon_ParseTreeMethodNode_t), BeaconObjectKindPointers);
    methodNode->selector = beacon_internString(state->context, selectorString);
    methodNode->arguments = beacon_ArrayList_asArray(state->context, arguments);
    methodNode->localVariables = (beacon_Array_t*)state->context->roots.emptyArray;
    methodNode->super.sourcePosition = parserState_sourcePositionFrom(state, startPosition);
    return methodNode;
}

beacon_ParseTreeNode_t *parser_parseMethodSyntaxWithDelimiter(beacon_parserState_t *state, beacon_TokenKind_t delimiter)
{
    if(parserState_peekKind(state, 0) != BeaconTokenKeyword 
    && parserState_peekKind(state, 0) != BeaconTokenIdentifier
    && !parser_isBinaryExpressionOperator(parserState_peekKind(state, 0)))
       return parserState_advanceWithExpectedError(state, "Expected a keyword, binary or unary message description.");

    beacon_ParseTreeMethodNode_t *methodNode = parser_parseMethodHeader(state);
    methodNode->localVariables = parser_parseDelimitedListOfTemporaries(state);

    beacon_ParseTreeNode_t *expression = parser_parseSequenceUntilEndOrDelimiter(state, delimiter);
    methodNode->expression = expression;
    return &methodNode->super;

}

beacon_ParseTreeNode_t *parser_parseWorkspaceScript(beacon_parserState_t *state)
{
    size_t startingPosition = state->position;
    beacon_ParseTreeWorkspaceScriptNode_t *script= beacon_allocateObjectWithBehavior(state->context->heap,state->context->classes.parseTreeWorkspaceScriptNode, sizeof(beacon_ParseTreeWorkspaceScriptNode_t), BeaconObjectKindPointers);
    script->localVariables = parser_parseDelimitedListOfTemporaries(state);
    script->expression = parser_parseSequenceUntilEndOrDelimiter(state, BeaconTokenEndOfSource);
    script->super.sourcePosition = parserState_sourcePositionFrom(state, startingPosition);
    return &script->super;
}

beacon_ParseTreeNode_t *beacon_parseWorkspaceTokenList(beacon_context_t *context, beacon_SourceCode_t *sourceCode, beacon_ArrayList_t *tokenList)
{
    beacon_parserState_t state = {
        .context = context,
        .sourceCode = sourceCode,
        .tokenCount = beacon_ArrayList_size(tokenList),
        .tokens = tokenList,
        .position = 1,
    };

    return parser_parseWorkspaceScript(&state);
}

beacon_ParseTreeNode_t *beacon_parseMethodTokenList(beacon_context_t *context, beacon_SourceCode_t *sourceCode, beacon_ArrayList_t *tokenList)
{
    beacon_parserState_t state = {
        .context = context,
        .sourceCode = sourceCode,
        .tokenCount = beacon_ArrayList_size(tokenList),
        .tokens = tokenList,
        .position = 1,
    };

    return parser_parseMethodSyntaxWithDelimiter(&state, BeaconTokenEndOfSource);
}
