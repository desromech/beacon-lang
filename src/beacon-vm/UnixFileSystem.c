#ifndef _WIN32
#include "Context.h"
#include "Exceptions.h"

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

static beacon_oop_t beacon_FileClass_open(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    beacon_oop_t pathOop = arguments[0];
    bool isWriteable = arguments[1] == context->roots.trueValue;
    bool isTruncated = arguments[2] == context->roots.trueValue;
    BeaconAssert(context, !beacon_isImmediate(pathOop));

    beacon_ObjectHeader_t *pathHeader = (beacon_ObjectHeader_t*) pathOop;
    char *pathString = malloc(pathHeader->slotCount + 1);
    memcpy(pathString, pathHeader + 1, pathHeader->slotCount);
    pathString[pathHeader->slotCount] = 0;

    int fd = 0;
    int openFlags = O_CLOEXEC;
    int mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    if(isWriteable)
    {
        openFlags |= O_RDWR | O_CREAT;
        if(isTruncated)
            openFlags |= O_TRUNC;
    }
    else
    {
        openFlags = O_RDONLY;
    }

    do
    {
        fd = open(pathString, openFlags, mode);
    } while(fd < 0 && errno == EINTR);

    free(pathString);

    if(fd < 0)
        beacon_exception_error(context, "Failed to open file");

    beacon_File_t *fileObject = beacon_allocateObjectWithBehavior(context->heap, context->classes.fileClass, sizeof(beacon_File_t), BeaconObjectKindPointers);
    fileObject->name = pathOop;
    fileObject->handle = beacon_encodeSmallInteger(fd);
    return (beacon_oop_t)fileObject;
}

static beacon_oop_t beacon_File_readIntoStartingAtCount(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    beacon_File_t *fileObject = (beacon_File_t*)receiver;
    int fd = beacon_decodeSmallInteger(fileObject->handle);
    BeaconAssert(context, fd >= 0);

    beacon_oop_t bufferOop = arguments[0];
    BeaconAssert(context, !beacon_isImmediate(bufferOop));

    intptr_t startingIndex = beacon_decodeSmallInteger(arguments[1]);
    intptr_t bufferCount = beacon_decodeSmallInteger(arguments[2]);

    beacon_ObjectHeader_t* bufferObjectHeader = (beacon_ObjectHeader_t*)bufferOop;
    BeaconAssert(context, startingIndex + bufferCount < (intptr_t)bufferObjectHeader->slotCount);

    uint8_t *readBuffer = (uint8_t *)(bufferObjectHeader + 1) + startingIndex;
    ssize_t readCount = 0;
    do
    {
        readCount = read(fd, readBuffer, bufferCount);
    } while (readCount < 0 && errno == EINTR);
    
    return beacon_encodeSmallInteger(readCount);
}

static beacon_oop_t beacon_File_writeFromStartingAtCount(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    beacon_File_t *fileObject = (beacon_File_t*)receiver;
    int fd = beacon_decodeSmallInteger(fileObject->handle);
    BeaconAssert(context, fd >= 0);

    beacon_oop_t bufferOop = arguments[0];
    BeaconAssert(context, !beacon_isImmediate(bufferOop));

    intptr_t startingIndex = beacon_decodeSmallInteger(arguments[1]);
    intptr_t bufferCount = beacon_decodeSmallInteger(arguments[2]);

    beacon_ObjectHeader_t* bufferObjectHeader = (beacon_ObjectHeader_t*)bufferOop;
    BeaconAssert(context, startingIndex + bufferCount < (intptr_t)bufferObjectHeader->slotCount);

    uint8_t *writeBuffer = (uint8_t *)(bufferObjectHeader + 1) + startingIndex;
    ssize_t writeCount = 0;
    do
    {
        writeCount = write(fd, writeBuffer, bufferCount);
    } while (writeCount < 0 && errno == EINTR);
    
    return beacon_encodeSmallInteger(writeCount);
}

static beacon_oop_t beacon_File_close(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    beacon_File_t *fileObject = (beacon_File_t*)receiver;
    int fd = beacon_decodeSmallInteger(fileObject->handle);
    if(fd >= 0)
    {
        close(fd);
        fileObject->handle = beacon_encodeSmallInteger(-1);
    }

    return receiver;
}

void beacon_context_registerFileSystemPrimitives(beacon_context_t *context)
{
    beacon_addPrimitiveToClass(context, beacon_getClass(context, (beacon_oop_t)context->classes.fileClass), "open:writeable:truncated:", 0, beacon_FileClass_open);
    beacon_addPrimitiveToClass(context, context->classes.fileClass, "readInto:startingAt:count:", 0, beacon_File_readIntoStartingAtCount);
    beacon_addPrimitiveToClass(context, context->classes.fileClass, "writeFrom:startingAt:count:", 0, beacon_File_writeFromStartingAtCount);
    beacon_addPrimitiveToClass(context, context->classes.fileClass, "close", 0, beacon_File_close);
}

#endif  