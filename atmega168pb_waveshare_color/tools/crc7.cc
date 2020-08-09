//From https://raw.githubusercontent.com/hazelnusse/crc7/master/crc7.cc

#include <iostream>
#include <cstdint>
#include <vector>
#include <array>

uint8_t CRCTable[256];
 
void GenerateCRCTable()
{
  int i, j;
  uint8_t CRCPoly = 0x89;  // the value of our CRC-7 polynomial
 
  // generate a table value for all 256 possible byte values
  for (i = 0; i < 256; ++i) {
    CRCTable[i] = (i & 0x80) ? i ^ CRCPoly : i;
    for (j = 1; j < 8; ++j) {
        CRCTable[i] <<= 1;
        if (CRCTable[i] & 0x80)
            CRCTable[i] ^= CRCPoly;
    }
  }
}
 
// adds a message byte to the current CRC-7 to get a the new CRC-7
uint8_t CRCAdd(uint8_t CRC, uint8_t message_byte)
{
    return CRCTable[(CRC << 1) ^ message_byte];
}
 
// returns the CRC-7 for a message of "length" bytes
uint8_t getCRC(uint8_t message[], int length)
{
  int i;
  uint8_t CRC = 0;

  for (i = 0; i < length; ++i)
    CRC = CRCAdd(CRC, message[i]);

  return CRC;
}

void PrintFrame(std::array<uint8_t, 6> & f)
{
  for (auto e : f)
    std::cout << std::hex << (int) e << " ";
  std::cout << std::endl;
}

int main()
{
  GenerateCRCTable();

  std::vector<std::array<uint8_t, 6>> CommandFrames;
  CommandFrames.push_back({{0x40, 0, 0, 0, 0, 0}});
  CommandFrames.push_back({{0x40+59, 0, 0, 0, 0 }});
  CommandFrames.push_back({{0x48, 0, 0, 1, 0xAA, 0}});
  CommandFrames.push_back({{0x69, 0x40, 0, 0, 0, 0}});
  CommandFrames.push_back({{0x77, 0x00, 0, 0, 0, 0}});
  CommandFrames.push_back({{0x7A, 0x00, 0, 0, 0, 0}});

  std::cout << "Command, Argument, CRC7" << std::endl;
  for (auto &Frame : CommandFrames) {
    Frame[5] = (getCRC(Frame.data(), 5) << 1) | 1;
    PrintFrame(Frame);
  }
}

