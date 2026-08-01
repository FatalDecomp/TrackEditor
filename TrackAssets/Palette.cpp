#include "Palette.h"
#include <cstdint>
#include <cstring>
#include <fstream>
#include <vector>
#include "Logging.h"
//-------------------------------------------------------------------------------------------------
#if defined(_DEBUG) && defined(IS_WINDOWS)
  #define new new(_CLIENT_BLOCK, __FILE__, __LINE__)
#endif
//-------------------------------------------------------------------------------------------------
CPalette::CPalette()
{
  ClearData();
}

//-------------------------------------------------------------------------------------------------

CPalette::~CPalette()
{

}

//-------------------------------------------------------------------------------------------------

void CPalette::ClearData()
{
  memset(m_paletteAy, 0, sizeof(m_paletteAy));
  m_bLoaded = false;
}

//-------------------------------------------------------------------------------------------------

bool CPalette::LoadPalette(const std::string &sFilename)
{
  ClearData();

  if (sFilename.empty()) {
    Logging::LogMessage("Palette filename empty");
    return false;
  }

  //open file
  std::ifstream file(sFilename.c_str(), std::ios::binary);
  if (!file.is_open()) {
    Logging::LogMessage("Failed to open palette: %s", sFilename.c_str());
    return false;
  }

  file.seekg(0, file.end);
  size_t length = file.tellg();
  file.seekg(0, file.beg);
  if (length <= 0) {
    Logging::LogMessage("Palette file %s is empty", sFilename.c_str());
    return false;
  }

  //read file
  std::vector<std::uint8_t> data(length);
  file.read(reinterpret_cast<char *>(data.data()), static_cast<std::streamsize>(length));
  if (!file)
    return false;

  if (length != static_cast<size_t>(PALETTE_SIZE * 3))
    return false;

  for (int i = 0; i < PALETTE_SIZE; ++i) {
    std::uint8_t byR = data[i * 3] << 2;
    std::uint8_t byG = data[i * 3 + 1] << 2;
    std::uint8_t byB = data[i * 3 + 2] << 2;
    m_paletteAy[i] = glm::vec<3, std::uint8_t>(byR, byG, byB);
  }

  file.close();

  m_bLoaded = true;
  Logging::LogMessage("Loaded palette: %s", sFilename.c_str());
  return true;
}

//-------------------------------------------------------------------------------------------------
