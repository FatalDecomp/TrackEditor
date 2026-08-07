#define _CRT_SECURE_NO_DEPRECATE
#include "Unmangler.h"
#include <string.h>
#include <algorithm>
//-------------------------------------------------------------------------------------------------
#if defined(_DEBUG) && defined(IS_WINDOWS)
  #define new new(_CLIENT_BLOCK, __FILE__, __LINE__)
#endif
//-------------------------------------------------------------------------------------------------

int Unmangler::GetUnmangledLength(const uint8_t *pSource, int iSourceLen)
{
  if (iSourceLen < 4)
    return -1;

  return *(int *)pSource;
}

//-------------------------------------------------------------------------------------------------

bool Unmangler::UnmangleFile(const uint8_t *pSource, int iSourceLen, uint8_t *pDestination, int iDestLength)
{
  // start positions
  int iInputPos = 4;
  int iOutputPos = 0;

  while ((iInputPos < iSourceLen) && (iOutputPos < iDestLength)) {
    int iValue = (int)pSource[iInputPos];

    if (iValue <= 0x3F) // 0x00 to 0x3F: read bytes from input
    {
      uint8_t* pTempArray = new uint8_t[iValue];
      memcpy(pTempArray, &pSource[iInputPos + 1], iValue);
      memcpy(&pDestination[iOutputPos], pTempArray, iValue);
      iInputPos += iValue + 1;
      iOutputPos += iValue;
      delete[] pTempArray;
    } else if (iValue <= 0x4F) // 0x40 to 0x4F: generate ascending bytes based on last 2 bytes
    {
      int iDelta = pDestination[iOutputPos - 1] - pDestination[iOutputPos - 2];
      for (int i = 0; i < ((iValue & 0x0F) + 3); i++) {
        pDestination[iOutputPos] = ((pDestination[iOutputPos - 1] + iDelta) & 0xFF);
        iOutputPos++;
      }
      iInputPos++;
    } else if (iValue <= 0x5F) // 0x50 to 0x5F: generate ascending words based on last 2 words
    {
      short sDelta = *(short*)(pDestination + iOutputPos - 2) - *(short*)(pDestination + iOutputPos - 4);
      for (int i = 0; i < ((iValue & 0x0F) + 2); i++) {
        short sNewShort = (*(short *)(pDestination + iOutputPos - 2)) + sDelta;
        pDestination[iOutputPos] = (uint8_t)(sNewShort & 0xFF);
        pDestination[iOutputPos + 1] = (uint8_t)((sNewShort >> 8) & 0xFF);
        iOutputPos += 2;
      }
      iInputPos++;
    } else if (iValue <= 0x6F) // 0x60 to 0x6F: clone last byte in output
    {
      for (int i = 0; i < ((iValue & 0x0F) + 3); i++) {
        pDestination[iOutputPos] = pDestination[iOutputPos - 1];
        iOutputPos++;
      }
      iInputPos++;
    } else if (iValue <= 0x7F) // 0x70 to 0x7F: clone last word in output
    {
      for (int i = 0; i < ((iValue & 0x0F) + 2); i++) {
        pDestination[iOutputPos] = pDestination[iOutputPos - 2];
        pDestination[iOutputPos + 1] = pDestination[iOutputPos - 1];
        iOutputPos += 2;
      }
      iInputPos++;
    } else if (iValue <= 0xBF) // 0x80 to 0xBF: clone 3 bytes using offset
    {
      int iOffset = (iValue & 0x3F);
      if (iOutputPos - iOffset - 3 < 0 || iOutputPos - iOffset - 3 >= iDestLength)
        return false;
      if (iOutputPos - iOffset - 1 < 0 || iOutputPos - iOffset - 1 >= iDestLength)
        return false;
      pDestination[iOutputPos] = pDestination[iOutputPos - iOffset - 3];
      pDestination[iOutputPos + 1] = pDestination[iOutputPos - iOffset - 2];
      pDestination[iOutputPos + 2] = pDestination[iOutputPos - iOffset - 1];
      iOutputPos += 3;
      iInputPos++;
    } else if (iValue <= 0xDF) // 0xC0 to 0xDF: clone using offset and length from next byte
    {
      int iOffset = ((iValue & 0x03) << 8) + (int)(pSource[iInputPos + 1]) + 3;
      int iLength = ((iValue >> 2) & 0x07) + 4;
      for (int i = 0; i < iLength; i++) {
        if (iOutputPos - iOffset < 0 || iOutputPos - iOffset >= iDestLength)
          return false;
        pDestination[iOutputPos] = pDestination[iOutputPos - iOffset];
        iOutputPos++;
      }
      iInputPos += 2;
    } else // 0xE0 to 0xFF: clone using offset and length from next 2 bytes
    {
      int iOffset = ((iValue & 0x1F) << 8) + (int)(pSource[iInputPos + 1]) + 3;
      int iLength = (int)(pSource[iInputPos + 2]) + 5;
      for (int i = 0; i < iLength; i++) {
        if (iOutputPos - iOffset < 0 || iOutputPos - iOffset >= iDestLength)
          return false;
        pDestination[iOutputPos] = pDestination[iOutputPos - iOffset];
        iOutputPos++;
      }
      iInputPos += 3;
    }
  }
  return iOutputPos == iDestLength;
}


// find next string of repeating bytes with length of at least 3, starting in the next 0x3F bytes
std::vector<int> FindMaxRepeats(const std::vector<uint8_t> &inputData, int startPos)
{
  int offset = 0, length = 0;
  for (int i = std::max(startPos, 1); (i < (int)inputData.size()) && (i < startPos + 0x3F); i++) {
    length = 1;
    while ((i + length < (int)inputData.size()) && (inputData[i + length] == inputData[i + length - 1])) {
      length++;
    }
    if (length >= 3) {
      offset = i - startPos;
      break;
    }
  }
  int adj = 1;
  if (startPos != 0) {
    if ((offset == 0) && (inputData[startPos] == inputData[startPos - 1])) {
      adj = 0; // continuation of previous bytes
    }
  }
  std::vector<int> output = { offset + adj, length - adj };
  return output;
}

//-------------------------------------------------------------------------------------------------

void Unmangler::MangleFile(const std::vector<uint8_t> &source, std::vector<uint8_t> &destination)
{
  destination.push_back(0);
  destination.push_back(0);
  destination.push_back(0);
  destination.push_back(0);
  int iSourceLen = (int)source.size();
  for (int i = 0; i < 4; i++)
    destination[3 - i] = (uint8_t)(iSourceLen >> (i * 8));
  int iInputPos = 0;
  while (iInputPos < iSourceLen) {
    std::vector<int> nextRepeats = FindMaxRepeats(source, iInputPos);
    int iOffset = nextRepeats[0];
    int iLength = std::min(nextRepeats[1], 18);
    if (iLength >= 3) // clone last byte in output
    {
      if (iOffset > 0) // output non-repeating bytes
      {
        destination.push_back((uint8_t)iOffset);
        for (int i = iInputPos; i < iInputPos + iOffset; ++i) {
          destination.push_back(source[i]);
        }
        iInputPos += iOffset;
      }
      uint8_t byVal = (uint8_t)(iLength - 3) + 0x60;
      destination.push_back(byVal);
      iInputPos += iLength;
    } else // no repeating bytes
    {
      int iNextLength = std::min((int)source.size() - iInputPos, 0x3F);
      destination.push_back((uint8_t)iNextLength);
      for (int i = iInputPos; i < iInputPos + iNextLength; ++i)
        destination.push_back(source[i]);
      iInputPos += iNextLength;
    }
  }
}

//-------------------------------------------------------------------------------------------------
