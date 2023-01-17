// fontinfonamesmapping.cpp

// Copyright (C) 2022-2023 by
// Charlie Jiang.

#include "fontinfo.hpp"

#include <unordered_map>

#include <freetype/ttnameid.h>


#define FTI_UnknownID 0xFFFE

// No more Qt containers since there's no any apparent advantage.
using TableType = std::unordered_map<unsigned short, QString>;
TableType ttSFNTNames;

TableType ttPlatformNames;
TableType ttEncodingUnicodeNames;
TableType ttEncodingMacNames;
TableType ttEncodingWindowsNames;
TableType ttEncodingISONames;
TableType ttEncodingAdobeNames;

TableType ttLanguageMacNames;
TableType ttLanguageWindowsNames;


QString*
mapSFNTNameIDToName(unsigned short nameID)
{
  if (ttSFNTNames.empty())
  {
    ttSFNTNames[FTI_UnknownID] = "Unknown";
    ttSFNTNames[TT_NAME_ID_COPYRIGHT] = "Copyright";
    ttSFNTNames[TT_NAME_ID_FONT_FAMILY] = "Font Family";
    ttSFNTNames[TT_NAME_ID_FONT_SUBFAMILY] = "Font Subfamily";
    ttSFNTNames[TT_NAME_ID_UNIQUE_ID] = "Unique Font ID";
    ttSFNTNames[TT_NAME_ID_FULL_NAME] = "Full Name";
    ttSFNTNames[TT_NAME_ID_VERSION_STRING] = "Version String";
    ttSFNTNames[TT_NAME_ID_PS_NAME] = "PostScript Name";
    ttSFNTNames[TT_NAME_ID_TRADEMARK] = "Trademark";
    ttSFNTNames[TT_NAME_ID_MANUFACTURER] = "Manufacturer";
    ttSFNTNames[TT_NAME_ID_DESIGNER] = "Designer";
    ttSFNTNames[TT_NAME_ID_DESCRIPTION] = "Description";
    ttSFNTNames[TT_NAME_ID_VENDOR_URL] = "Vendor URL";
    ttSFNTNames[TT_NAME_ID_DESIGNER_URL] = "Designer URL";
    ttSFNTNames[TT_NAME_ID_LICENSE] = "License";
    ttSFNTNames[TT_NAME_ID_LICENSE_URL] = "License URL";
    ttSFNTNames[TT_NAME_ID_TYPOGRAPHIC_FAMILY] = "Typographic Family";
    ttSFNTNames[TT_NAME_ID_TYPOGRAPHIC_SUBFAMILY] = "Typographic Subfamily";
    ttSFNTNames[TT_NAME_ID_MAC_FULL_NAME] = "Mac Full Name";
    ttSFNTNames[TT_NAME_ID_SAMPLE_TEXT] = "Sample Text";
    ttSFNTNames[TT_NAME_ID_WWS_FAMILY] = "WWS Family Name";
    ttSFNTNames[TT_NAME_ID_WWS_SUBFAMILY] = "WWS Subfamily Name";
    ttSFNTNames[TT_NAME_ID_LIGHT_BACKGROUND] = "Light Background Palette";
    ttSFNTNames[TT_NAME_ID_DARK_BACKGROUND] = "Dark Background Palette";
    ttSFNTNames[TT_NAME_ID_VARIATIONS_PREFIX]
      = "Variations PostScript Name Prefix";
  }

  auto it = ttSFNTNames.find(nameID);
  if (it == ttSFNTNames.end())
    return &ttSFNTNames[FTI_UnknownID];
  return &it->second;
}


QString*
mapTTPlatformIDToName(unsigned short platformID)
{
  if (ttPlatformNames.empty())
  {
    ttPlatformNames[FTI_UnknownID] = "Unknown Platform";
    // Unicode codepoints are encoded as UTF-16BE.
    ttPlatformNames[TT_PLATFORM_APPLE_UNICODE] = "Apple (Unicode)";
    ttPlatformNames[TT_PLATFORM_MACINTOSH] = "Macintosh";
    ttPlatformNames[TT_PLATFORM_ISO] = "ISO (deprecated)";
    ttPlatformNames[TT_PLATFORM_MICROSOFT] = "Microsoft";
    ttPlatformNames[TT_PLATFORM_CUSTOM] = "Custom";
    ttPlatformNames[TT_PLATFORM_ADOBE] = "Adobe";
  }

  auto it = ttPlatformNames.find(platformID);
  if (it == ttPlatformNames.end())
    return &ttPlatformNames[FTI_UnknownID];
  return &it->second;
}


QString*
mapTTEncodingIDToName(unsigned short platformID,
                      unsigned short encodingID)
{
  if (ttEncodingUnicodeNames.empty())
  {
    // Note: different from the Apple doc.
    ttEncodingUnicodeNames[FTI_UnknownID] = "Unknown Encoding";
    ttEncodingUnicodeNames[TT_APPLE_ID_DEFAULT] = "Unicode 1.0";
    ttEncodingUnicodeNames[TT_APPLE_ID_UNICODE_1_1] = "Unicode 1.1";
    ttEncodingUnicodeNames[TT_APPLE_ID_ISO_10646] = "ISO/IEC 10646";
    ttEncodingUnicodeNames[TT_APPLE_ID_UNICODE_2_0]
        = "Unicode 2.0 or later (BMP only)";
    ttEncodingUnicodeNames[TT_APPLE_ID_UNICODE_32]
        = "Unicode 2.0 or later (non-BMP characters allowed)";
    //ttEncodingUnicodeNames[TT_APPLE_ID_VARIANT_SELECTOR] = "Variant Selector";
    //ttEncodingUnicodeNames[TT_APPLE_ID_FULL_UNICODE] = ???;
  }

  if (ttEncodingMacNames.empty())
  {
    ttEncodingMacNames[FTI_UnknownID] = "Unknown Encoding";
    ttEncodingMacNames[0] = "Roman";
    ttEncodingMacNames[1] = "Japanese";
    ttEncodingMacNames[2] = "Chinese (Traditional)";
    ttEncodingMacNames[3] = "Korean";
    ttEncodingMacNames[4] = "Arabic";
    ttEncodingMacNames[5] = "Hebrew";
    ttEncodingMacNames[6] = "Greek";
    ttEncodingMacNames[7] = "Russian";
    ttEncodingMacNames[8] = "RSymbol";
    ttEncodingMacNames[9] = "Devanagari";
    ttEncodingMacNames[10] = "Gurmukhi";
    ttEncodingMacNames[11] = "Gujarati";
    ttEncodingMacNames[12] = "Oriya";
    ttEncodingMacNames[13] = "Bengali";
    ttEncodingMacNames[14] = "Tamil";
    ttEncodingMacNames[15] = "Telugu";
    ttEncodingMacNames[16] = "Kannada";
    ttEncodingMacNames[17] = "Malayalam";
    ttEncodingMacNames[18] = "Sinhalese";
    ttEncodingMacNames[19] = "Burmese";
    ttEncodingMacNames[20] = "Khmer";
    ttEncodingMacNames[21] = "Thai";
    ttEncodingMacNames[22] = "Laotian";
    ttEncodingMacNames[23] = "Georgian";
    ttEncodingMacNames[24] = "Armenian";
    ttEncodingMacNames[25] = "Chinese (Simplified)";
    ttEncodingMacNames[26] = "Tibetan";
    ttEncodingMacNames[27] = "Mongolian";
    ttEncodingMacNames[28] = "Geez";
    ttEncodingMacNames[29] = "Slavic";
    ttEncodingMacNames[30] = "Vietnamese";
    ttEncodingMacNames[31] = "Sindhi";
    ttEncodingMacNames[32] = "Uninterpreted";
  }

  if (ttEncodingWindowsNames.empty())
  {
    ttEncodingMacNames[FTI_UnknownID] = "Unknown Encoding";
    ttEncodingWindowsNames[0] = "Symbol";
    ttEncodingWindowsNames[1] = "Unicode BMP";
    ttEncodingWindowsNames[2] = "ShiftJIS";
    ttEncodingWindowsNames[3] = "GBK";
    ttEncodingWindowsNames[4] = "Big5";
    ttEncodingWindowsNames[5] = "Wansung";
    ttEncodingWindowsNames[6] = "Johab";
    ttEncodingWindowsNames[7] = "Reserved";
    ttEncodingWindowsNames[8] = "Reserved";
    ttEncodingWindowsNames[9] = "Reserved";
    ttEncodingWindowsNames[10] = "Unicode full repertoire";
  }

  if (ttEncodingISONames.empty())
  {
    ttEncodingISONames[FTI_UnknownID] = "Unknown Encoding";
    ttEncodingISONames[TT_ISO_ID_7BIT_ASCII] = "ASCII";
    ttEncodingISONames[TT_ISO_ID_10646] = "ISO/IEC 10646";
    ttEncodingISONames[TT_ISO_ID_8859_1] = "ISO 8859-1";
  }

  if (ttEncodingAdobeNames.empty())
  {
    ttEncodingAdobeNames[FTI_UnknownID] = "Unknown Encoding";
    ttEncodingAdobeNames[TT_ADOBE_ID_STANDARD] = "Adobe Standard";
    ttEncodingAdobeNames[TT_ADOBE_ID_EXPERT] = "Adobe Expert";
    ttEncodingAdobeNames[TT_ADOBE_ID_CUSTOM] = "Adobe Custom";
    ttEncodingAdobeNames[TT_ADOBE_ID_LATIN_1] = "Adobe Latin 1";
  }

  TableType* table = NULL;

  switch (platformID)
  {
  case TT_PLATFORM_APPLE_UNICODE:
    table = &ttEncodingUnicodeNames;
    break;
  case TT_PLATFORM_MACINTOSH:
    table = &ttEncodingMacNames;
    break;
  case TT_PLATFORM_MICROSOFT:
    table = &ttEncodingWindowsNames;
    break;
  case TT_PLATFORM_ISO:
    table = &ttEncodingISONames;
    break;
  case TT_PLATFORM_ADOBE:
    table = &ttEncodingAdobeNames;
    break;

  default:
    return &ttEncodingUnicodeNames[FTI_UnknownID];
  }

  auto it = table->find(encodingID);
  if (it == table->end())
    return &(*table)[FTI_UnknownID];
  return &it->second;
}


QString*
mapTTLanguageIDToName(unsigned short platformID,
                      unsigned short languageID)
{
  if (ttLanguageMacNames.empty())
  {
    ttLanguageMacNames[FTI_UnknownID] = "Unknown Language";
    ttLanguageMacNames[0] = "English";
    ttLanguageMacNames[1] = "French";
    ttLanguageMacNames[2] = "German";
    ttLanguageMacNames[3] = "Italian";
    ttLanguageMacNames[4] = "Dutch";
    ttLanguageMacNames[5] = "Swedish";
    ttLanguageMacNames[6] = "Spanish";
    ttLanguageMacNames[7] = "Danish";
    ttLanguageMacNames[8] = "Portuguese";
    ttLanguageMacNames[9] = "Norwegian";
    ttLanguageMacNames[10] = "Hebrew";
    ttLanguageMacNames[11] = "Japanese";
    ttLanguageMacNames[12] = "Arabic";
    ttLanguageMacNames[13] = "Finnish";
    ttLanguageMacNames[14] = "Greek";
    ttLanguageMacNames[15] = "Icelandic";
    ttLanguageMacNames[16] = "Maltese";
    ttLanguageMacNames[17] = "Turkish";
    ttLanguageMacNames[18] = "Croatian";
    ttLanguageMacNames[19] = "Chinese (Traditional)";
    ttLanguageMacNames[20] = "Urdu";
    ttLanguageMacNames[21] = "Hindi";
    ttLanguageMacNames[22] = "Thai";
    ttLanguageMacNames[23] = "Korean";
    ttLanguageMacNames[24] = "Lithuanian";
    ttLanguageMacNames[25] = "Polish";
    ttLanguageMacNames[26] = "Hungarian";
    ttLanguageMacNames[27] = "Estonian";
    ttLanguageMacNames[28] = "Latvian";
    ttLanguageMacNames[29] = "Sami";
    ttLanguageMacNames[30] = "Faroese";
    ttLanguageMacNames[31] = "Farsi/Persian";
    ttLanguageMacNames[32] = "Russian";
    ttLanguageMacNames[33] = "Chinese (Simplified)";
    ttLanguageMacNames[34] = "Flemish";
    ttLanguageMacNames[35] = "Irish Gaelic";
    ttLanguageMacNames[36] = "Albanian";
    ttLanguageMacNames[37] = "Romanian";
    ttLanguageMacNames[38] = "Czech";
    ttLanguageMacNames[39] = "Slovak";
    ttLanguageMacNames[40] = "Slovenian";
    ttLanguageMacNames[41] = "Yiddish";
    ttLanguageMacNames[42] = "Serbian";
    ttLanguageMacNames[43] = "Macedonian";
    ttLanguageMacNames[44] = "Bulgarian";
    ttLanguageMacNames[45] = "Ukrainian";
    ttLanguageMacNames[46] = "Byelorussian";
    ttLanguageMacNames[47] = "Uzbek";
    ttLanguageMacNames[48] = "Kazakh";
    ttLanguageMacNames[49] = "Azerbaijani (Cyrillic script)";
    ttLanguageMacNames[50] = "Azerbaijani (Arabic script)";
    ttLanguageMacNames[51] = "Armenian";
    ttLanguageMacNames[52] = "Georgian";
    ttLanguageMacNames[53] = "Moldavian";
    ttLanguageMacNames[54] = "Kirghiz";
    ttLanguageMacNames[55] = "Tajiki";
    ttLanguageMacNames[56] = "Turkmen";
    ttLanguageMacNames[57] = "Mongolian (Mongolian script)";
    ttLanguageMacNames[58] = "Mongolian (Cyrillic script)";
    ttLanguageMacNames[59] = "Pashto";
    ttLanguageMacNames[60] = "Kurdish";
    ttLanguageMacNames[61] = "Kashmiri";
    ttLanguageMacNames[62] = "Sindhi";
    ttLanguageMacNames[63] = "Tibetan";
    ttLanguageMacNames[64] = "Nepali";
    ttLanguageMacNames[65] = "Sanskrit";
    ttLanguageMacNames[66] = "Marathi";
    ttLanguageMacNames[67] = "Bengali";
    ttLanguageMacNames[68] = "Assamese";
    ttLanguageMacNames[69] = "Gujarati";
    ttLanguageMacNames[70] = "Punjabi";
    ttLanguageMacNames[71] = "Oriya";
    ttLanguageMacNames[72] = "Malayalam";
    ttLanguageMacNames[73] = "Kannada";
    ttLanguageMacNames[74] = "Tamil";
    ttLanguageMacNames[75] = "Telugu";
    ttLanguageMacNames[76] = "Sinhalese";
    ttLanguageMacNames[77] = "Burmese";
    ttLanguageMacNames[78] = "Khmer";
    ttLanguageMacNames[79] = "Lao";
    ttLanguageMacNames[80] = "Vietnamese";
    ttLanguageMacNames[81] = "Indonesian";
    ttLanguageMacNames[82] = "Tagalog";
    ttLanguageMacNames[83] = "Malay (Roman script)";
    ttLanguageMacNames[84] = "Malay (Arabic script)";
    ttLanguageMacNames[85] = "Amharic";
    ttLanguageMacNames[86] = "Tigrinya";
    ttLanguageMacNames[87] = "Galla";
    ttLanguageMacNames[88] = "Somali";
    ttLanguageMacNames[89] = "Swahili";
    ttLanguageMacNames[90] = "Kinyarwanda/Ruanda";
    ttLanguageMacNames[91] = "Rundi";
    ttLanguageMacNames[92] = "Nyanja/Chewa";
    ttLanguageMacNames[93] = "Malagasy";
    ttLanguageMacNames[94] = "Esperanto";
    ttLanguageMacNames[128] = "Welsh";
    ttLanguageMacNames[129] = "Basque";
    ttLanguageMacNames[130] = "Catalan";
    ttLanguageMacNames[131] = "Latin";
    ttLanguageMacNames[132] = "Quechua";
    ttLanguageMacNames[133] = "Guarani";
    ttLanguageMacNames[134] = "Aymara";
    ttLanguageMacNames[135] = "Tatar";
    ttLanguageMacNames[136] = "Uighur";
    ttLanguageMacNames[137] = "Dzongkha";
    ttLanguageMacNames[138] = "Javanese (Roman script)";
    ttLanguageMacNames[139] = "Sundanese (Roman script)";
    ttLanguageMacNames[140] = "Galician";
    ttLanguageMacNames[141] = "Afrikaans";
    ttLanguageMacNames[142] = "Breton";
    ttLanguageMacNames[143] = "Inuktitut";
    ttLanguageMacNames[144] = "Scottish Gaelic";
    ttLanguageMacNames[145] = "Manx Gaelic";
    ttLanguageMacNames[146] = "Irish Gaelic (with dot above)";
    ttLanguageMacNames[147] = "Tongan";
    ttLanguageMacNames[148] = "Greek (polytonic)";
    ttLanguageMacNames[149] = "Greenlandic";
    ttLanguageMacNames[150] = "Azerbaijani (Roman script)";
  }

  if (ttLanguageWindowsNames.empty())
  {
    ttLanguageWindowsNames[FTI_UnknownID] = "Unknown Language";
    ttLanguageWindowsNames[0x0436] = "Afrikaans";
    ttLanguageWindowsNames[0x041C] = "Albanian";
    ttLanguageWindowsNames[0x0402] = "Bulgarian";
    ttLanguageWindowsNames[0x0484] = "Alsatian";
    ttLanguageWindowsNames[0x045E] = "Amharic";
    ttLanguageWindowsNames[0x0405] = "Czech";
    ttLanguageWindowsNames[0x1401] = "Arabic";
    ttLanguageWindowsNames[0x3C01] = "Arabic";
    ttLanguageWindowsNames[0x0C01] = "Arabic";
    ttLanguageWindowsNames[0x0401] = "Arabic";
    ttLanguageWindowsNames[0x0404] = "Chinese";
    ttLanguageWindowsNames[0x0406] = "Danish";
    ttLanguageWindowsNames[0x0423] = "Belarusian";
    ttLanguageWindowsNames[0x0445] = "Bengali";
    ttLanguageWindowsNames[0x0465] = "Divehi";
    ttLanguageWindowsNames[0x0801] = "Arabic";
    ttLanguageWindowsNames[0x2C01] = "Arabic";
    ttLanguageWindowsNames[0x0403] = "Catalan";
    ttLanguageWindowsNames[0x0413] = "Dutch";
    ttLanguageWindowsNames[0x0483] = "Corsican";
    ttLanguageWindowsNames[0x0804] = "Chinese";
    ttLanguageWindowsNames[0x0813] = "Dutch";
    ttLanguageWindowsNames[0x0845] = "Bengali";
    ttLanguageWindowsNames[0x1001] = "Arabic";
    ttLanguageWindowsNames[0x1004] = "Chinese";
    ttLanguageWindowsNames[0x1009] = "English";
    ttLanguageWindowsNames[0x1404] = "Chinese";
    ttLanguageWindowsNames[0x1801] = "Arabic";
    ttLanguageWindowsNames[0x2001] = "Arabic";
    ttLanguageWindowsNames[0x2401] = "Arabic";
    ttLanguageWindowsNames[0x2801] = "Arabic";
    ttLanguageWindowsNames[0x3001] = "Arabic";
    ttLanguageWindowsNames[0x3401] = "Arabic";
    ttLanguageWindowsNames[0x3801] = "Arabic";
    ttLanguageWindowsNames[0x4001] = "Arabic";
    ttLanguageWindowsNames[0x1C01] = "Arabic";
    ttLanguageWindowsNames[0x042B] = "Armenian";
    ttLanguageWindowsNames[0x044D] = "Assamese";
    ttLanguageWindowsNames[0x082C] = "Azeri (Cyrillic)";
    ttLanguageWindowsNames[0x042C] = "Azeri (Latin)";
    ttLanguageWindowsNames[0x046D] = "Bashkir";
    ttLanguageWindowsNames[0x042D] = "Basque";
    ttLanguageWindowsNames[0x201A] = "Bosnian (Cyrillic)";
    ttLanguageWindowsNames[0x141A] = "Bosnian (Latin)";
    ttLanguageWindowsNames[0x047E] = "Breton";
    ttLanguageWindowsNames[0x0C04] = "Chinese";
    ttLanguageWindowsNames[0x041A] = "Croatian";
    ttLanguageWindowsNames[0x101A] = "Croatian (Latin)";
    ttLanguageWindowsNames[0x048C] = "Dari";
    ttLanguageWindowsNames[0x0C09] = "English";
    ttLanguageWindowsNames[0x0407] = "German";
    ttLanguageWindowsNames[0x0408] = "Greek";
    ttLanguageWindowsNames[0x0409] = "English";
    ttLanguageWindowsNames[0x0410] = "Italian";
    ttLanguageWindowsNames[0x0411] = "Japanese";
    ttLanguageWindowsNames[0x0421] = "Indonesian";
    ttLanguageWindowsNames[0x0425] = "Estonian";
    ttLanguageWindowsNames[0x0434] = "isiXhosa";
    ttLanguageWindowsNames[0x0435] = "isiZulu";
    ttLanguageWindowsNames[0x0437] = "Georgian";
    ttLanguageWindowsNames[0x0438] = "Faroese";
    ttLanguageWindowsNames[0x0439] = "Hindi";
    ttLanguageWindowsNames[0x0447] = "Gujarati";
    ttLanguageWindowsNames[0x0453] = "Khmer";
    ttLanguageWindowsNames[0x0456] = "Galician";
    ttLanguageWindowsNames[0x0462] = "Frisian";
    ttLanguageWindowsNames[0x0464] = "Filipino";
    ttLanguageWindowsNames[0x0468] = "Hausa (Latin)";
    ttLanguageWindowsNames[0x0470] = "Igbo";
    ttLanguageWindowsNames[0x0486] = "K’iche";
    ttLanguageWindowsNames[0x0807] = "German";
    ttLanguageWindowsNames[0x0809] = "English";
    ttLanguageWindowsNames[0x0810] = "Italian";
    ttLanguageWindowsNames[0x1007] = "German";
    ttLanguageWindowsNames[0x1407] = "German";
    ttLanguageWindowsNames[0x1409] = "English";
    ttLanguageWindowsNames[0x1809] = "English";
    ttLanguageWindowsNames[0x2009] = "English";
    ttLanguageWindowsNames[0x2409] = "English";
    ttLanguageWindowsNames[0x2809] = "English";
    ttLanguageWindowsNames[0x3009] = "English";
    ttLanguageWindowsNames[0x3409] = "English";
    ttLanguageWindowsNames[0x4009] = "English";
    ttLanguageWindowsNames[0x4409] = "English";
    ttLanguageWindowsNames[0x4809] = "English";
    ttLanguageWindowsNames[0x1C09] = "English";
    ttLanguageWindowsNames[0x2C09] = "English";
    ttLanguageWindowsNames[0x040B] = "Finnish";
    ttLanguageWindowsNames[0x080C] = "French";
    ttLanguageWindowsNames[0x0C0C] = "French";
    ttLanguageWindowsNames[0x040C] = "French";
    ttLanguageWindowsNames[0x140c] = "French";
    ttLanguageWindowsNames[0x180C] = "French";
    ttLanguageWindowsNames[0x100C] = "French";
    ttLanguageWindowsNames[0x0C07] = "German";
    ttLanguageWindowsNames[0x046F] = "Greenlandic";
    ttLanguageWindowsNames[0x040D] = "Hebrew";
    ttLanguageWindowsNames[0x040E] = "Hungarian";
    ttLanguageWindowsNames[0x040F] = "Icelandic";
    ttLanguageWindowsNames[0x045D] = "Inuktitut";
    ttLanguageWindowsNames[0x085D] = "Inuktitut (Latin)";
    ttLanguageWindowsNames[0x083C] = "Irish";
    ttLanguageWindowsNames[0x044B] = "Kannada";
    ttLanguageWindowsNames[0x043F] = "Kazakh";
    ttLanguageWindowsNames[0x0412] = "Korean";
    ttLanguageWindowsNames[0x0426] = "Latvian";
    ttLanguageWindowsNames[0x0427] = "Lithuanian";
    ttLanguageWindowsNames[0x0440] = "Kyrgyz";
    ttLanguageWindowsNames[0x0441] = "Kiswahili";
    ttLanguageWindowsNames[0x0454] = "Lao";
    ttLanguageWindowsNames[0x0457] = "Konkani";
    ttLanguageWindowsNames[0x0481] = "Maori";
    ttLanguageWindowsNames[0x0487] = "Kinyarwanda";
    ttLanguageWindowsNames[0x082E] = "Lower Sorbian";
    ttLanguageWindowsNames[0x046E] = "Luxembourgish";
    ttLanguageWindowsNames[0x042F] = "Macedonian";
    ttLanguageWindowsNames[0x083E] = "Malay";
    ttLanguageWindowsNames[0x043E] = "Malay";
    ttLanguageWindowsNames[0x044C] = "Malayalam";
    ttLanguageWindowsNames[0x043A] = "Maltese";
    ttLanguageWindowsNames[0x047A] = "Mapudungun";
    ttLanguageWindowsNames[0x044E] = "Marathi";
    ttLanguageWindowsNames[0x047C] = "Mohawk";
    ttLanguageWindowsNames[0x0414] = "Norwegian (Bokmal)";
    ttLanguageWindowsNames[0x0415] = "Polish";
    ttLanguageWindowsNames[0x0416] = "Portuguese";
    ttLanguageWindowsNames[0x0417] = "Romansh";
    ttLanguageWindowsNames[0x0418] = "Romanian";
    ttLanguageWindowsNames[0x0419] = "Russian";
    ttLanguageWindowsNames[0x0446] = "Punjabi";
    ttLanguageWindowsNames[0x0448] = "Odia (formerly Oriya)";
    ttLanguageWindowsNames[0x0450] = "Mongolian (Cyrillic)";
    ttLanguageWindowsNames[0x0461] = "Nepali";
    ttLanguageWindowsNames[0x0463] = "Pashto";
    ttLanguageWindowsNames[0x0482] = "Occitan";
    ttLanguageWindowsNames[0x0814] = "Norwegian (Nynorsk)";
    ttLanguageWindowsNames[0x0816] = "Portuguese";
    ttLanguageWindowsNames[0x0850] = "Mongolian (Traditional)";
    ttLanguageWindowsNames[0x046B] = "Quechua";
    ttLanguageWindowsNames[0x086B] = "Quechua";
    ttLanguageWindowsNames[0x0C6B] = "Quechua";
    ttLanguageWindowsNames[0x243B] = "Sami (Inari)";
    ttLanguageWindowsNames[0x103B] = "Sami (Lule)";
    ttLanguageWindowsNames[0x143B] = "Sami (Lule)";
    ttLanguageWindowsNames[0x0C3B] = "Sami (Northern)";
    ttLanguageWindowsNames[0x043B] = "Sami (Northern)";
    ttLanguageWindowsNames[0x083B] = "Sami (Northern)";
    ttLanguageWindowsNames[0x203B] = "Sami (Skolt)";
    ttLanguageWindowsNames[0x183B] = "Sami (Southern)";
    ttLanguageWindowsNames[0x1C3B] = "Sami (Southern)";
    ttLanguageWindowsNames[0x044F] = "Sanskrit";
    ttLanguageWindowsNames[0x1C1A] = "Serbian (Cyrillic)";
    ttLanguageWindowsNames[0x0C1A] = "Serbian (Cyrillic)";
    ttLanguageWindowsNames[0x181A] = "Serbian (Latin)";
    ttLanguageWindowsNames[0x081A] = "Serbian (Latin)";
    ttLanguageWindowsNames[0x046C] = "Sesotho sa Leboa";
    ttLanguageWindowsNames[0x0432] = "Setswana";
    ttLanguageWindowsNames[0x045B] = "Sinhala";
    ttLanguageWindowsNames[0x041B] = "Slovak";
    ttLanguageWindowsNames[0x0424] = "Slovenian";
    ttLanguageWindowsNames[0x2C0A] = "Spanish";
    ttLanguageWindowsNames[0x400A] = "Spanish";
    ttLanguageWindowsNames[0x340A] = "Spanish";
    ttLanguageWindowsNames[0x240A] = "Spanish";
    ttLanguageWindowsNames[0x140A] = "Spanish";
    ttLanguageWindowsNames[0x1C0A] = "Spanish";
    ttLanguageWindowsNames[0x300A] = "Spanish";
    ttLanguageWindowsNames[0x440A] = "Spanish";
    ttLanguageWindowsNames[0x100A] = "Spanish";
    ttLanguageWindowsNames[0x480A] = "Spanish";
    ttLanguageWindowsNames[0x080A] = "Spanish";
    ttLanguageWindowsNames[0x4C0A] = "Spanish";
    ttLanguageWindowsNames[0x180A] = "Spanish";
    ttLanguageWindowsNames[0x3C0A] = "Spanish";
    ttLanguageWindowsNames[0x280A] = "Spanish";
    ttLanguageWindowsNames[0x500A] = "Spanish";
    ttLanguageWindowsNames[0x0C0A] = "Spanish (Modern Sort)";
    ttLanguageWindowsNames[0x040A] = "Spanish (Traditional Sort)";
    ttLanguageWindowsNames[0x540A] = "Spanish";
    ttLanguageWindowsNames[0x380A] = "Spanish";
    ttLanguageWindowsNames[0x200A] = "Spanish";
    ttLanguageWindowsNames[0x081D] = "Swedish";
    ttLanguageWindowsNames[0x041D] = "Swedish";
    ttLanguageWindowsNames[0x045A] = "Syriac";
    ttLanguageWindowsNames[0x0420] = "Urdu";
    ttLanguageWindowsNames[0x0428] = "Tajik (Cyrillic)";
    ttLanguageWindowsNames[0x085F] = "Tamazight (Latin)";
    ttLanguageWindowsNames[0x0443] = "Uzbek (Latin)";
    ttLanguageWindowsNames[0x0444] = "Tatar";
    ttLanguageWindowsNames[0x0449] = "Tamil";
    ttLanguageWindowsNames[0x044A] = "Telugu";
    ttLanguageWindowsNames[0x041E] = "Thai";
    ttLanguageWindowsNames[0x0422] = "Ukrainian";
    ttLanguageWindowsNames[0x0442] = "Turkmen";
    ttLanguageWindowsNames[0x0451] = "Tibetan";
    ttLanguageWindowsNames[0x041F] = "Turkish";
    ttLanguageWindowsNames[0x0480] = "Uighur";
    ttLanguageWindowsNames[0x042E] = "Upper Sorbian";
    ttLanguageWindowsNames[0x0452] = "Welsh";
    ttLanguageWindowsNames[0x0478] = "Yi";
    ttLanguageWindowsNames[0x0485] = "Yakut";
    ttLanguageWindowsNames[0x0488] = "Wolof";
    ttLanguageWindowsNames[0x0843] = "Uzbek (Cyrillic)";
    ttLanguageWindowsNames[0x042A] = "Vietnamese";
    ttLanguageWindowsNames[0x046A] = "Yoruba";
  }

  TableType* table = NULL;

  switch (platformID)
  {
  case TT_PLATFORM_MACINTOSH:
    table = &ttLanguageMacNames;
    break;
  case TT_PLATFORM_MICROSOFT:
    table = &ttLanguageWindowsNames;
    break;

  default:
    return &ttLanguageWindowsNames[FTI_UnknownID];
  }

  auto it = table->find(languageID);
  if (it == table->end())
    return &(*table)[FTI_UnknownID];
  return &it->second;
}


// end of fontinfonamesmapping.cpp
