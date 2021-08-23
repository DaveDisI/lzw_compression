#include "utilities.h"

#include <stdlib.h>
#include <stdio.h>
#include <windows.h>

struct String{
    u8 chars[256] = {};
    u32 length = 0;

    void operator=(String& s){
        for(u32 i = 0; i < s.length; i++){
            chars[i] = s.chars[i];
        }
        length = s.length;
    }

    void operator=(const u8* str){
        u8* s = (u8*)str;
        while(*s != '\0'){
            chars[length++] = *s;
            s++;
        }
        
    }

    void operator+=(String& s){
        for(u32 i = 0; i < s.length; i++){
            chars[length++] = s.chars[i];
        }
        length += s.length;
    }

    void operator+=(u8 c){
        chars[length++] = c;
    }

    bool operator==(String& s){
        if(s.length != length) return false;
        for(u32 i = 0; i < length; i++){
            if(chars[i] != s.chars[i]) return false;
        }
        return true;
    }
};


struct Dictionary{
    u16 codes[4096] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255};
    String sequences[4096];
    u32 totalCodes = 256;

    Dictionary(){
        for(u32 i = 0; i < totalCodes; i++){
            sequences[i].chars[0] = (u8)i;
            sequences[i].length = 1;
        }
    }

    void add(String& s){
        codes[totalCodes] = totalCodes;
        sequences[totalCodes++] = s;
    }

    bool contains(u16 code){
        return binarySearch(codes, code, 0, totalCodes, 0);
    }

    bool contains(String seq){
        for(u32 i = 0; i < totalCodes; i++){
            if(seq == sequences[i]){
                return true;
            }
        }
        return false;
    }

    u16 get(String seq){
        for(u32 i = 0; i < totalCodes; i++){
            if(seq == sequences[i]){
                return i;
            }
        }
        return -1;
    }

    String get(u16 code){
        return sequences[code];
    }
};

static String operator+(String& a, String& b){
    String s = a;
    s += b;
    return s;
}

static String operator+(String& a, u8 c){
    String s = a;
    s.chars[s.length++] = c;
    return s; 
}

static u32 readBitsFromArray(u8* array, u32 numBits, u32* offset){
    u8* currentByte = array + (*offset / 8);
    u8 bt = *offset % 8;
    u32 result = 0;
    for(u32 i = 0; i < numBits; i++){
        result |= ((*currentByte >> bt) & 1) << i;
        bt++;
        if(bt % 8 == 0){
            bt = 0;
            currentByte++;
        }
    }

    *offset += numBits;
    return result;
}

static void packBitsIntoArray(u8* array, u32 bits, u32 numBits, u32* offset){
    u8* currentByte = array + (*offset / 8);
    u8 bt = *offset % 8;

    for(u32 i = 0; i <= numBits; i++){
        *currentByte |= ((bits >> i) & 1) << bt;
        bt++;
        if(bt % 8 == 0){
            bt = 0;
            currentByte++;
        }
    }

    *offset += numBits;
}

static u8* compressLZW(u8* data, u32 dataSize, u32* compressedDataSize){
    Dictionary* dict = new Dictionary();

    String s;
    char ch;
    u8* endData = data + dataSize;
    u16* outputCodes = (u16*)malloc(sizeof(u16) * dataSize); 
    u32 totalOutputCodes = 0;
    u32 highestOutputCode = 0;
    u32 numBits = 12;
    for(u8* ptr = data; ptr < endData; ptr++){
        ch = *ptr;
        String tmp = s + ch;
        if(dict->contains(tmp)){
            s = tmp;
        }else{
            u16 oc = dict->get(s);
            if(oc > highestOutputCode) highestOutputCode = oc;
            outputCodes[totalOutputCodes++] = oc;
            dict->add(tmp);
            s.length = 1;
            s.chars[0] = ch;
        }   
    }

    if(s.length > 0){
        outputCodes[totalOutputCodes++] = dict->get(s);
    }

    if(highestOutputCode < 512)       numBits = 9;
    else if(highestOutputCode < 1024) numBits = 10;
    else if(highestOutputCode < 2048) numBits = 11;

    u32 compressedDataSizeInBits = numBits * totalOutputCodes + 5;
    u32 compressedDataSizeInBytes = (compressedDataSizeInBits / 8) + ((compressedDataSizeInBits % 8) > 0 ? 1 : 0);
    u8* outputData = (u8*)malloc(compressedDataSizeInBytes);
    memset(outputData, 0, compressedDataSizeInBytes);

    u32 outputDataOffset = 0;
    packBitsIntoArray(outputData, numBits, 5, &outputDataOffset);
    for(u32 i = 0; i < totalOutputCodes; i++){
        packBitsIntoArray(outputData, outputCodes[i], numBits, &outputDataOffset);
    }

    *compressedDataSize = compressedDataSizeInBits;
    delete dict;
    free(outputCodes);
    return outputData;
}

static void decompressLZW(u8* data, u32 dataSize, u8* outputBuffer, u32* outputSize){
    u32 offset = 0; 
    u32 numBits = readBitsFromArray(data, 5, &offset);
    u32 totalCodes = (dataSize - 5) / numBits; 
    Dictionary* dict = new Dictionary();
    u32 ctr = 0;
    String w;
    w += (u8)readBitsFromArray(data, numBits, &offset);
    outputBuffer[ctr++] = w.chars[0];
    for(u32 i = 1; i < totalCodes; i++){
        String entry;
        u16 k = (u16)readBitsFromArray(data, numBits, &offset);
        if(dict->contains(k)){
            entry = dict->get(k);
        }else if(k == dict->totalCodes){
            entry = w + w.chars[0];
        }else{
            printf("ERROR!!!!\n");
            exit(1);
        }

        for(u32 j = 0; j < entry.length; j++){
            outputBuffer[ctr++] = entry.chars[j];
        }        

        dict->add(w + entry.chars[0]);

        w = entry;
    }

    *outputSize = ctr;
    delete(dict);
}

static u8 decompressedData[MEGABYTE(1)];
int main(){
    u8 c[] = {'T','O','B','E','O','R','N','O','T','T','O','B','E','O','R','T','O','B','E','O','R','N','O','T'};
    u32 compressedDataSize = 0;
    u8* compressedData = compressLZW(c, sizeof(c), &compressedDataSize);
    
    u32 decompressedDataSize = 0;
    decompressLZW(compressedData, compressedDataSize, decompressedData, &decompressedDataSize);

    return 0;
}