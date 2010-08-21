/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2009-2010 Alan Wright. All rights reserved.
// Distributable under the terms of either the Apache License (Version 2.0)
// or the GNU Lesser General Public License.
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "IndexInput.h"

namespace Lucene
{
    IndexInput::IndexInput()
    {
        preUTF8Strings = false;
    }
    
    IndexInput::~IndexInput()
    {
    }
    
    void IndexInput::readBytes(uint8_t* b, int32_t offset, int32_t length, bool useBuffer)
    {
        // default to ignoring useBuffer entirely
        readBytes(b, offset, length);
    }
    
    int32_t IndexInput::readInt()
    {
        int32_t i = (readByte() & 0xff) << 24;
        i |= (readByte() & 0xff) << 16;
        i |= (readByte() & 0xff) << 8;
        i |= (readByte() & 0xff);
        return i;
    }
    
    int32_t IndexInput::readVInt()
    {
        uint8_t b = readByte();
        int32_t i = (b & 0x7f);
        
        for (int32_t shift = 7; (b & 0x80) != 0; shift += 7)
        {
            b = readByte();
            i |= (b & 0x7f) << shift;
        }
        return i;
    }
    
    int64_t IndexInput::readLong()
    {
        int64_t i = (int64_t)readInt() << 32;
        i |= (readInt() & 0xffffffffLL);
        return i;
    }
    
    int64_t IndexInput::readVLong()
    {
        uint8_t b = readByte();
        int64_t i = (b & 0x7f);
        
        for (int32_t shift = 7; (b & 0x80) != 0; shift += 7)
        {
            b = readByte();
            i |= (b & 0x7f) << shift;
        }
        return i;
    }
    
    void IndexInput::setModifiedUTF8StringsMode()
    {
        preUTF8Strings = true;
    }
    
    String IndexInput::readString()
    {
        if (preUTF8Strings)
            return readModifiedUTF8String();
        int32_t length = readVInt();
        if (!bytes)
            bytes = ByteArray::newInstance((int32_t)((double)length * 1.25));
        if (length > bytes.length())
            bytes.resize((int32_t)((double)length * 1.25));
        readBytes(bytes.get(), 0, length);
        return StringUtils::toUnicode(bytes.get(), length);
    }
    
    String IndexInput::readModifiedUTF8String()
    {
        int32_t length = readVInt();
        if (!chars)
            chars = CharArray::newInstance(length);
        if (length > chars.length())
            chars.resize(length);
        readChars(chars.get(), 0, length);
        return String(chars.get(), length);
    }
    
    void IndexInput::readChars(wchar_t* buffer, int32_t start, int32_t length)
    {
        int32_t end = start + length;
        for (int32_t i = start; i < end; ++i)
        {
            uint8_t b = readByte();
            if ((b & 0x80) == 0)
                buffer[i] = (wchar_t)(b & 0x7f);
            else if ((b & 0xe0) != 0xe0)
                buffer[i] = (wchar_t)(((b & 0x1f) << 6) | (readByte() & 0x3f));
            else
            {
                uint32_t ch = ((b & 0x0f) << 12);
                ch |= (readByte() & 0x3f) << 6;
                ch |= (readByte() & 0x3f);
                buffer[i] = (wchar_t)ch;
            }
        }
    }
    
    void IndexInput::skipChars(int32_t length)
    {
        for (int32_t i = 0; i < length; ++i)
        {
            uint8_t b = readByte();
            if ((b & 0x80) == 0)
            { // do nothing, we only need one byte
            }
            else if ((b & 0xe0) != 0xe0)
                readByte(); // read an additional byte
            else
            {
                // read two additional bytes
                readByte();
                readByte();
            }
        }
    }
    
    MapStringString IndexInput::readStringStringMap()
    {
        MapStringString map(MapStringString::newInstance());
        int32_t count = readInt();
        for (int32_t i = 0; i < count; ++i)
        {
            String key(readString());
            String val(readString());
            map.put(key, val);
        }
        return map;
    }
    
    LuceneObjectPtr IndexInput::clone(LuceneObjectPtr other)
    {
        IndexInputPtr cloneIndexInput(boost::dynamic_pointer_cast<IndexInput>(LuceneObject::clone(other)));
        cloneIndexInput->preUTF8Strings = preUTF8Strings;
        cloneIndexInput->bytes.reset();
        cloneIndexInput->chars.reset();
        return cloneIndexInput;
    }
}