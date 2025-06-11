#ifndef _WIN32
#include "Context.h"
#include "Exceptions.h"

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <dirent.h>

static beacon_oop_t beacon_DirectoryClass_open(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    beacon_oop_t pathOop = arguments[0];
    BeaconAssert(context, !beacon_isImmediate(pathOop));

    beacon_ObjectHeader_t *pathHeader = (beacon_ObjectHeader_t*) pathOop;
    char *pathString = malloc(pathHeader->slotCount + 1);
    memcpy(pathString, pathHeader + 1, pathHeader->slotCount);
    pathString[pathHeader->slotCount] = 0;

    DIR *handle = opendir(pathString);
    if(!handle)
        return 0;

    beacon_Directory_t *directoryObject = beacon_allocateObjectWithBehavior(context->heap, context->classes.directoryClass, sizeof(beacon_Directory_t), BeaconObjectKindPointers);
    directoryObject->name = pathOop;
    directoryObject->handle = beacon_boxExternalAddress(context, handle);
    return (beacon_oop_t)directoryObject;
}

static beacon_oop_t beacon_Directory_nextEntry(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    beacon_Directory_t *directoryObject = (beacon_Directory_t*)receiver;
    BeaconAssert(context, directoryObject->handle);
    DIR *dir = beacon_unboxExternalAddress(context, directoryObject->handle);
    struct dirent *entry = readdir(dir);
    if(!entry)
        return 0;

    beacon_DirectoryEntry_t *dirEntry = beacon_allocateObjectWithBehavior(context->heap, context->classes.directoryEntryClass, sizeof(beacon_DirectoryEntry_t), BeaconObjectKindPointers);
    dirEntry->name = (beacon_oop_t)beacon_importCString(context, entry->d_name);
    dirEntry->isRegularFile = (entry->d_type == DT_REG) ? context->roots.trueValue : context->roots.falseValue;
    dirEntry->isDirectory = (entry->d_type == DT_DIR) ? context->roots.trueValue : context->roots.falseValue;
    return (beacon_oop_t)dirEntry;
}

static beacon_oop_t beacon_Directory_rewind(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    beacon_Directory_t *directoryObject = (beacon_Directory_t*)receiver;
    BeaconAssert(context, directoryObject->handle);
    rewinddir(beacon_unboxExternalAddress(context, directoryObject->handle));
    return receiver;
}

static beacon_oop_t beacon_Directory_close(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    beacon_Directory_t *directoryObject = (beacon_Directory_t*)receiver;
    if(directoryObject->handle)
    {
        closedir(beacon_unboxExternalAddress(context, directoryObject->handle));
        directoryObject->handle = 0;
    }

    return receiver;
}

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

    intptr_t startingIndex = beacon_decodeSmallInteger(arguments[1]) - 1;
    intptr_t bufferCount = beacon_decodeSmallInteger(arguments[2]);

    beacon_ObjectHeader_t* bufferObjectHeader = (beacon_ObjectHeader_t*)bufferOop;
    BeaconAssert(context, startingIndex + bufferCount <= (intptr_t)bufferObjectHeader->slotCount);

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

    intptr_t startingIndex = beacon_decodeSmallInteger(arguments[1]) - 1;
    intptr_t bufferCount = beacon_decodeSmallInteger(arguments[2]);

    beacon_ObjectHeader_t* bufferObjectHeader = (beacon_ObjectHeader_t*)bufferOop;
    BeaconAssert(context, startingIndex + bufferCount <= (intptr_t)bufferObjectHeader->slotCount);

    uint8_t *writeBuffer = (uint8_t *)(bufferObjectHeader + 1) + startingIndex;
    ssize_t writeCount = 0;
    do
    {
        writeCount = write(fd, writeBuffer, bufferCount);
    } while (writeCount < 0 && errno == EINTR);
    
    return beacon_encodeSmallInteger(writeCount);
}

static beacon_oop_t beacon_File_getPosition(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    beacon_File_t *fileObject = (beacon_File_t*)receiver;
    int fd = beacon_decodeSmallInteger(fileObject->handle);
    intptr_t position = 0;
    if(fd >= 0)
        position = lseek(fd, 0, SEEK_CUR);
    
    return beacon_encodeSmallInteger(position);
}

static beacon_oop_t beacon_File_setPosition(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    beacon_File_t *fileObject = (beacon_File_t*)receiver;
    int fd = beacon_decodeSmallInteger(fileObject->handle);
    intptr_t position = beacon_decodeSmallInteger(arguments[0]);
    if(fd >= 0)
        position = lseek(fd, position, SEEK_SET);
    
    return beacon_encodeSmallInteger(position);
}

static beacon_oop_t beacon_File_getSize(beacon_context_t *context, beacon_oop_t receiver, size_t argumentCount, beacon_oop_t *arguments)
{
    beacon_File_t *fileObject = (beacon_File_t*)receiver;
    int fd = beacon_decodeSmallInteger(fileObject->handle);
    intptr_t size = 0;
    if(fd >= 0)
    {
        off_t position = lseek(fd, 0, SEEK_CUR);
        size = lseek(fd, 0, SEEK_END);
        lseek(fd, position, SEEK_SET);
    }
    
    return beacon_encodeSmallInteger(size);
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
    beacon_addPrimitiveToClass(context, beacon_getClass(context, (beacon_oop_t)context->classes.directoryClass), "open:", 0, beacon_DirectoryClass_open);
    beacon_addPrimitiveToClass(context, context->classes.directoryClass, "nextEntry", 0, beacon_Directory_nextEntry);
    beacon_addPrimitiveToClass(context, context->classes.directoryClass, "rewind", 0, beacon_Directory_rewind);
    beacon_addPrimitiveToClass(context, context->classes.directoryClass, "close", 0, beacon_Directory_close);

    beacon_addPrimitiveToClass(context, beacon_getClass(context, (beacon_oop_t)context->classes.fileClass), "open:writeable:truncated:", 0, beacon_FileClass_open);
    beacon_addPrimitiveToClass(context, context->classes.fileClass, "readInto:startingAt:count:", 0, beacon_File_readIntoStartingAtCount);
    beacon_addPrimitiveToClass(context, context->classes.fileClass, "writeFrom:startingAt:count:", 0, beacon_File_writeFromStartingAtCount);
    beacon_addPrimitiveToClass(context, context->classes.fileClass, "position", 0, beacon_File_getPosition);
    beacon_addPrimitiveToClass(context, context->classes.fileClass, "position:", 0, beacon_File_setPosition);
    beacon_addPrimitiveToClass(context, context->classes.fileClass, "size", 0, beacon_File_getSize);
    beacon_addPrimitiveToClass(context, context->classes.fileClass, "close", 0, beacon_File_close);
}

#endif  