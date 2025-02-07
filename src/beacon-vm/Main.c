#include "beacon-lang/Context.h"
#include "beacon-lang/Scanner.h"
#include "beacon-lang/SourceCode.h"
#include "beacon-lang/Parser.h"
#include <stdio.h>
#include <string.h>

static beacon_context_t *context;

void printHelp(void)
{
    printf("beacon-vm [options] <input-files>\n");
}

void printVersion(void)
{
    printf("beacon-vm version 0.1\n");
}

void evaluateSourceCode(beacon_SourceCode_t *sourceCode)
{
    beacon_ArrayList_t *scannedSource = beacon_scanSourceCode(context, sourceCode);
    intptr_t tokenCount = beacon_ArrayList_size(scannedSource);
    for(intptr_t i = 1; i <= tokenCount; ++i)
    {
        beacon_ScannerToken_t *token = (beacon_ScannerToken_t *)beacon_ArrayList_at(context, scannedSource, i);
        beacon_TokenKind_t kind = beacon_decodeSmallInteger(token->kind);
        if(kind == BeaconTokenError)
            beacon_exception_scannerError(context, token);
           
        //printf("Token %d: %s\n", (int)i, beacon_TokenKind_toString());
    }

    beacon_ParseTreeNode_t *parseTree = beacon_parseTokenList(context, sourceCode, scannedSource);;
}


void evaluateString(const char *string)
{
    beacon_SourceCode_t *sourceCode = beacon_makeSourceCodeFromString(context, "CLI", string);
    evaluateSourceCode(sourceCode);
}

void evaluateFileNamed(const char *fileName)
{
    beacon_SourceCode_t *sourceCode = beacon_makeSourceCodeFromFileNamed(context, fileName);
    evaluateSourceCode(sourceCode);

}

int main(int argc, const char **argv)
{
    context = beacon_context_new();
    if(!context)
    {
        fprintf(stderr, "Failed to create the Beacon context.\n");
        return 1;
    }

    for(int i = 1; i < argc; ++i)
    {
        const char *arg = argv[i];
        if(*arg == '-')
        {
            if(!strcmp(arg, "-h"))
            {
                printHelp();
                beacon_context_destroy(context);
                return 0;
            }
            else if(!strcmp(arg, "-v"))
            {
                printVersion();
                beacon_context_destroy(context);
                return 0;
            }
            else if(!strcmp(arg, "-eval"))
            {
                const char *script = argv[++i];
                evaluateString(script);
            }
        }
        else
        {
            evaluateFileNamed(arg);
        }
    }

    beacon_context_destroy(context);
    return 0;
}