#pragma once

enum class image_metadata_section
{
  TiffMetadata,
  Exif,
  GPS,

  tiff_metadata_section_count
};

struct tiff_params
{
  static constexpr uint16_t NewSubfileType = 254;            // UInt32
  static constexpr uint16_t ImageWidth = 256;                // UInt32
  static constexpr uint16_t ImageHeight = 257;               // UInt32
  static constexpr uint16_t BitsPerSample = 258;             // UInt16, 3
  static constexpr uint16_t Compression = 259;               // UInt16
  static constexpr uint16_t PhotometricInterpretation = 262; // UInt16
  static constexpr uint16_t ImageTitle = 270;                // ASCII
  static constexpr uint16_t Make = 271;                      // ASCII
  static constexpr uint16_t Model = 272;                     // ASCII
  static constexpr uint16_t ImageDataLocation = 273;         // UInt32
  static constexpr uint16_t Orientation = 274;               // UInt16
  static constexpr uint16_t SamplesPerPixel = 277;           // UInt16
  static constexpr uint16_t RowsPerStrip = 278;              // UInt32
  static constexpr uint16_t StripByteCounts = 279;           // UInt32
  static constexpr uint16_t XResolution = 282;               // Rational
  static constexpr uint16_t YResolution = 283;               // Rational
  static constexpr uint16_t PlanarConfiguration = 284;       // UInt16
  static constexpr uint16_t ResolutionUnit = 296;            // UInt16
  static constexpr uint16_t Software = 305;                  // ASCII
  static constexpr uint16_t DateTime = 306;                  // ASCII, 20 fixed
  static constexpr uint16_t Artist = 315;                    // ASCII
  static constexpr uint16_t ExifIFDPointer = 34665;          // UInt32
  static constexpr uint16_t ICCProfile = 34675;              // Undefined
  static constexpr uint16_t GPSIDF = 34853;                  // UInt32
};

struct exif_params
{
  static constexpr uint16_t ExposureTime = 33434;          // Rational
  static constexpr uint16_t FNumber = 33437;               // Rational
  static constexpr uint16_t IsoSpeedRatings = 34855;       // Uint16, N
  static constexpr uint16_t FocalLength = 37386;           // Rational
  static constexpr uint16_t FocalLengthIn35mmFilm = 41989; // Short
};

enum class image_metadata_value_type : uint16_t
{
  UInt8 = 1,    // Byte
  ASCII = 2,    // ASCII String, must include NUL
  UInt16 = 3,   // Short
  UInt32 = 4,   // Long
  Rational = 5, // Rational based out of 2 UInt32 (LONG) values. First
                // numerator, second denominator.
  Int8 = 6,       // SByte
  Undefined = 7,  // 8-bit byte field that may contain anything.
  Int16 = 8,      // SShort
  Int32 = 9,      // SLong
  SRational = 10, // Two Int32 (SLONG) where first represents numerator, second
                  // denominator of a fraction.
  Float = 11, //
  Double = 12 //
};

struct metadata_rational
{
  metadata_rational(uint32_t a_numerator, uint32_t a_denominator);

  const uint32_t Numerator, Denominator;
};
