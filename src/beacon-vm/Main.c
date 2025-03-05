#include "beacon-lang/Context.h"
#include "beacon-lang/Scanner.h"
#include "beacon-lang/SourceCode.h"
#include "beacon-lang/Parser.h"
#include "beacon-lang/SyntaxCompiler.h"
#include "beacon-lang/Exceptions.h"
#include "beacon-lang/AgpuRendering.h"
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

void printObject(beacon_oop_t object)
{
    beacon_String_t *printedObject = (beacon_String_t *)beacon_perform(context, object, (beacon_oop_t)beacon_internCString(context, "printString"));
    BeaconAssert(context, printedObject);
    size_t stringSize = printedObject->super.super.super.super.super.header.slotCount;
    printf("%.*s\n", (int)stringSize, printedObject->data);
}

void evaluateStringAndPrint(const char *string)
{
    beacon_SourceCode_t *sourceCode = beacon_makeSourceCodeFromString(context, "CLI", string);
    beacon_oop_t result = beacon_evaluateSourceCode(context, sourceCode);
    printObject(result);
}

void evaluateFileNamed(const char *fileName)
{
    beacon_SourceCode_t *sourceCode = beacon_makeSourceCodeFromFileNamed(context, fileName);
    beacon_evaluateSourceCode(context, sourceCode);
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
                evaluateStringAndPrint(script);
            }
            else if(!strcmp(arg, "-gplatform"))
            {
                context->roots.agpuCommon->platformIndex = atoi(argv[++i]);
            }
            else if(!strcmp(arg, "-gpu"))
            {
                context->roots.agpuCommon->deviceIndex = atoi(argv[++i]);
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