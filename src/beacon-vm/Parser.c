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
        }
    }

    beacon_ParseTreeLiteralNode_t *literal = beacon_allocateObjectWithBehavior(state->context->heap, state->context->roots.parseTreeLiteralNodeClass, sizeof(beacon_ParseTreeLiteralNode_t), BeaconObjectKindPointers);
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

    beacon_String_t *stringLiteral = beacon_allocateObjectWithBehavior(state->context->heap, state->context->roots.stringClass, sizeof(beacon_String_t) + textConstantSize, BeaconObjectKindBytes);
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
    beacon_ParseTreeLiteralNode_t *literal = beacon_allocateObjectWithBehavior(state->context->heap, state->context->roots.parseTreeLiteralNodeClass, sizeof(beacon_ParseTreeLiteralNode_t), BeaconObjectKindPointers);
    literal->super.sourcePosition = token->sourcePosition;
    literal->value = (beacon_oop_t)symbolLiteral;
    return &literal->super;
}

beacon_ParseTreeNode_t *parser_parseLiteralSymbol(beacon_parserState_t *state)
{
    beacon_ScannerToken_t *token = parserState_next(state);
    assert(beacon_decodeSmallInteger(token->kind) == BeaconTokenSymbol);

    const char *textData = (const char*)token->sourcePosition->sourceCode->text->data + beacon_decodeSmallInteger(token->textPosition);
    intptr_t textDataSize = beacon_decodeSmallInteger(token->textSize);
    if(textDataSize >= 2 && textData[0] == '#' && textData[1] == '\'')
        return parser_parseLiteralStringSymbol(token, state);

    intptr_t symbolSize = textDataSize - 1;
    beacon_String_t *stringLiteral = beacon_allocateObjectWithBehavior(state->context->heap, state->context->roots.stringClass, sizeof(beacon_String_t) + symbolSize, BeaconObjectKindBytes);
    memcpy(stringLiteral->data, textData + 1, symbolSize);
    
    beacon_Symbol_t *symbolLiteral = beacon_internString(state->context, stringLiteral);
    beacon_ParseTreeLiteralNode_t *literal = beacon_allocateObjectWithBehavior(state->context->heap, state->context->roots.parseTreeLiteralNodeClass, sizeof(beacon_ParseTreeLiteralNode_t), BeaconObjectKindPointers);
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
    assert(beacon_decodeSmallInteger(token->kind) == BeaconTokenIdentifier);

    const char *textData = (const char*)token->sourcePosition->sourceCode->text->data + beacon_decodeSmallInteger(token->textPosition);
    intptr_t textDataSize = beacon_decodeSmallInteger(token->textSize);
    
    beacon_String_t *stringLiteral = beacon_allocateObjectWithBehavior(state->context->heap, state->context->roots.stringClass, sizeof(beacon_String_t) + textDataSize, BeaconObjectKindBytes);
    memcpy(stringLiteral->data, textData, textDataSize);

    beacon_Symbol_t *symbol = beacon_internString(state->context, stringLiteral);

    beacon_ParseTreeIdentifierReferenceNode_t *identifierReference = beacon_allocateObjectWithBehavior(state->context->heap, state->context->roots.parseTreeIdentifierReferenceNodeClass, sizeof(beacon_ParseTreeIdentifierReferenceNode_t), BeaconObjectKindPointers);
    identifierReference->super.sourcePosition = token->sourcePosition;
    identifierReference->identifier = (beacon_oop_t)symbol;
    return &identifierReference->super;
}

beacon_ParseTreeNode_t *parser_parseParenthesis(beacon_parserState_t *state)
{
    beacon_ScannerToken_t *token = parserState_next(state);
    assert(beacon_decodeSmallInteger(token->kind) == BeaconTokenLeftParent);
    
    beacon_ParseTreeNode_t *expression = parser_parseSequenceUntilEndOrDelimiter(state, BeaconTokenRightParent);
    expression = parserState_expectAddingErrorToNode(state, BeaconTokenRightParent, expression);
    return expression;
}


beacon_ParseTreeNode_t *parser_parseTerm(beacon_parserState_t *state)
{
    switch (parserState_peekKind(state, 0))
    {
    case BeaconTokenIdentifier:
        return parser_parseIdentifier(state);
    case BeaconTokenLeftParent:
        return parser_parseParenthesis(state);
    /*case BeaconTokenLeftBracket:
        return parser_parseBlock(state);
    case BeaconTokenLeftCurlyBracket:
        return parser_parseArray(state);
    case BeaconTokenByteArrayStart:
        return parser_parseByteArray(state);    */
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

    while(parserState_peekKind(state, 0) == BeaconTokenIdentifier)
    {
        beacon_ScannerToken_t *unaryToken = parserState_next(state);
        
        const char *textData = (const char*)unaryToken->sourcePosition->sourceCode->text->data + beacon_decodeSmallInteger(unaryToken->textPosition);
        intptr_t textDataSize = beacon_decodeSmallInteger(unaryToken->textSize);

        beacon_String_t *stringLiteral = beacon_allocateObjectWithBehavior(state->context->heap, state->context->roots.stringClass, sizeof(beacon_String_t) + textDataSize, BeaconObjectKindBytes);
        memcpy(stringLiteral->data, textData, textDataSize);

        beacon_Symbol_t *selectorSymbol = beacon_internString(state->context, stringLiteral);
        beacon_ParseTreeLiteralNode_t *selectorLiteral = beacon_allocateObjectWithBehavior(state->context->heap, state->context->roots.parseTreeLiteralNodeClass, sizeof(beacon_ParseTreeLiteralNode_t), BeaconObjectKindPointers);
        selectorLiteral->super.sourcePosition = unaryToken->sourcePosition;
        selectorLiteral->value = (beacon_oop_t)selectorSymbol;

        beacon_Array_t *argumentsArray = beacon_allocateObjectWithBehavior(state->context->heap, state->context->roots.arrayClass, sizeof(beacon_Array_t), BeaconObjectKindPointers);;

        beacon_ParseTreeMessageSendNode_t *messageSend = beacon_allocateObjectWithBehavior(state->context->heap, state->context->roots.parseTreeMessageSendNodeClass, sizeof(beacon_ParseTreeMessageSendNode_t), BeaconObjectKindPointers);
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

        beacon_String_t *stringLiteral = beacon_allocateObjectWithBehavior(state->context->heap, state->context->roots.stringClass, sizeof(beacon_String_t) + textDataSize, BeaconObjectKindBytes);
        memcpy(stringLiteral->data, textData, textDataSize);

        beacon_Symbol_t *operatorSymbol = beacon_internString(state->context, stringLiteral);
        beacon_ParseTreeLiteralNode_t *operatorLiteral = beacon_allocateObjectWithBehavior(state->context->heap, state->context->roots.parseTreeLiteralNodeClass, sizeof(beacon_ParseTreeLiteralNode_t), BeaconObjectKindPointers);
        operatorLiteral->super.sourcePosition = operatorToken->sourcePosition;
        operatorLiteral->value = (beacon_oop_t)operatorSymbol;

        beacon_ParseTreeNode_t *rightOperand = parser_parseUnaryPostfixExpression(state);
        beacon_Array_t *argumentsArray = beacon_allocateObjectWithBehavior(state->context->heap, state->context->roots.arrayClass, sizeof(beacon_Array_t) + sizeof(beacon_oop_t), BeaconObjectKindPointers);
        argumentsArray->elements[0] = (beacon_oop_t)rightOperand;

        beacon_ParseTreeMessageSendNode_t *messageSend = beacon_allocateObjectWithBehavior(state->context->heap, state->context->roots.parseTreeMessageSendNodeClass, sizeof(beacon_ParseTreeMessageSendNode_t), BeaconObjectKindPointers);
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
    if(parserState_peekKind(state, 0) != BeaconTokenKeyword)
       return parserState_advanceWithExpectedError(state, "Expecteda keyword or cascaded message.");

    size_t startPosition = state->position;
    beacon_ArrayList_t *keywords = beacon_ArrayList_new(state->context);
    beacon_ArrayList_t *arguments = beacon_ArrayList_new(state->context);
    size_t selectorSize = 0;

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
    beacon_String_t *selectorString = beacon_allocateObjectWithBehavior(state->context->heap, state->context->roots.stringClass, sizeof(beacon_String_t) + selectorSize, BeaconObjectKindBytes);
    size_t destIndex = 0;
    intptr_t keywordCount = beacon_ArrayList_size(keywords);
    for(intptr_t i = 1; i <= keywordCount; ++i)
    {
        beacon_ScannerToken_t *keywordToken = (beacon_ScannerToken_t*)beacon_ArrayList_at(state->context, keywords, i);
        intptr_t keywordSize = beacon_decodeSmallInteger(keywordToken->textSize);
        for(intptr_t j = 0; j < keywordSize; ++j)
        {
            char c = keywordToken->sourcePosition->sourceCode->text->data[j];
            selectorString->data[destIndex++] = c;
        }
    }

    beacon_ParseTreeLiteralNode_t *selectorLiteral = beacon_allocateObjectWithBehavior(state->context->heap, state->context->roots.parseTreeLiteralNodeClass, sizeof(beacon_ParseTreeLiteralNode_t), BeaconObjectKindPointers);
    selectorLiteral->super.sourcePosition = parserState_sourcePositionFrom(state, startPosition);
    selectorLiteral->value = (beacon_oop_t)beacon_internString(state->context, selectorString);

    // Make the cascaded message.
    beacon_ParseTreeCascadedMessageNode_t *cascadedMessage = beacon_allocateObjectWithBehavior(state->context->heap, state->context->roots.parseTreeCascadedMessageNodeClass, sizeof(beacon_ParseTreeCascadedMessageNode_t), BeaconObjectKindPointers);
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
        beacon_ArrayList_add(state->context, cascadedMessages, (beacon_oop_t)firstCascaded);
        while (parserState_peekKind(state, 0) == BeaconTokenSemicolon)
        {
            parserState_advance(state);
            beacon_ParseTreeNode_t *cascadedMessage = parser_parseCascadedMessage(state);
            beacon_ArrayList_add(state->context, cascadedMessages, (beacon_oop_t)cascadedMessage);
        }

        beacon_ParseTreeMessageCascadeNode_t *messageCascade = beacon_allocateObjectWithBehavior(state->context->heap, state->context->roots.parseTreeMessageCascadeNodeClass, sizeof(beacon_ParseTreeMessageCascadeNode_t), BeaconObjectKindPointers);
        messageCascade->super.sourcePosition = parserState_sourcePositionFrom(state, startPosition);
        messageCascade->receiver = receiver;
        messageCascade->cascadedMessages = beacon_ArrayList_asArray(state->context, cascadedMessages);
        return &messageCascade->super;

    }
    else if(firstCascaded)
    {
        beacon_ParseTreeCascadedMessageNode_t *onlyCascadedMessage = (beacon_ParseTreeCascadedMessageNode_t*)firstCascaded;

        beacon_ParseTreeMessageSendNode_t *messageSend = beacon_allocateObjectWithBehavior(state->context->heap, state->context->roots.parseTreeMessageSendNodeClass, sizeof(beacon_ParseTreeMessageSendNode_t), BeaconObjectKindPointers);
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
beacon_ParseTreeNode_t *parser_parseExpression(beacon_parserState_t *state)
{
    return parser_parseKeywordMessageSend(state);
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
