#define _CRT_SECURE_NO_DEPRECATE

#ifdef WIN32
#include <windows.h>
#endif

#include "loadlib.h"
#include "..\mpq_libmpq.h"

class MPQFile;

FileLoader::FileLoader()
{
    data = 0;
    data_size = 0;
    version = 0;
}

FileLoader::~FileLoader()
{
    free();
}

bool FileLoader::loadFile(char *filename)
{
    free();
    MPQFile mf(filename);
    if(mf.isEof())
    {
        printf("No such file %s\n", filename);
        return false;
    }

    data_size = mf.getSize();

    data = new uint8 [data_size];
    if (data)
    {
        mf.read(data, data_size);
        mf.close();
        if (prepareLoadedData())
            return true;
    }
    printf("Error loading %s", filename);
    mf.close();
    free();
    return false;
}

bool FileLoader::prepareLoadedData()
{
    // Check version
    version = (file_MVER *) data;
    if (version->fcc != 'MVER')
        return false;
    if (version->ver != FILE_FORMAT_VERSION)
        return false;
    return true;
}

void FileLoader::free()
{
    if (data) delete[] data;
    data = 0;
    data_size = 0;
    version = 0;
}