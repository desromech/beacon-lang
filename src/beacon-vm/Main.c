#include "beacon-lang/Context.h"
#include <stdio.h>
#include <string.h>

void printHelp(void)
{
    printf("beacon-vm [options] <input-files>\n");
}

void printVersion(void)
{
    printf("beacon-vm version 0.1\n");
}

int main(int argc, const char **argv)
{
    beacon_context_t *context = beacon_context_new();
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
                printf("TODO: eval script %s\n", script);
            }
        }
        else
        {

        }
    }

    beacon_context_destroy(context);
    return 0;
}