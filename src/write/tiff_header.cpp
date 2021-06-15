#include "internal/tiff_header.h"

#include <cassert>

#include "libraw/libraw.h"
#include "libraw/libraw_internal.h"
#include "internal/defines.h"

struct deferred_write_handle
{
  deferred_write_handle(uint32_t a_WriteLocation, uint16_t a_ReservedBytes)
      : WriteLocation(a_WriteLocation), ReservedBytes(a_ReservedBytes)
  {
  }

  const uint32_t WriteLocation;
  const uint16_t ReservedBytes;
};

class data_writer
{
public:
  explicit data_writer(FILE *a_target) : m_target(a_target) {}

  template <typename TSimpleType,
            typename std::is_fundamental<TSimpleType>::type * = nullptr>
  void Write(TSimpleType a_value)
  {
    fwrite(&a_value, sizeof(TSimpleType), 1, m_target);
  }

  void Write(const void *a_Data, uint32_t a_byteCount)
  {
    fwrite(a_Data, a_byteCount, 1, m_target);
  }

  template <typename TSimpleType,
            typename std::is_fundamental<TSimpleType>::type * = nullptr>
  void Write(deferred_write_handle a_handle, TSimpleType a_value)
  {
    uint32_t currentWritePos = GetBytePosition();
    fseek(m_target, a_handle.WriteLocation, SEEK_SET);
    fwrite(&a_value, sizeof(TSimpleType), 1, m_target);
    fseek(m_target, currentWritePos, SEEK_SET);
  }

  deferred_write_handle DeferWrite(uint16_t a_ByteCount)
  {
    deferred_write_handle handle(GetBytePosition(), a_ByteCount);
    const char nullChar = 0;
    fwrite(&nullChar, 1, a_ByteCount, m_target);
    return handle;
  }

  uint32_t GetBytePosition() const { return ftell(m_target); }

private:
  FILE *m_target;
};

struct tiff_file_header
{
  tiff_file_header() : t_order(htonl(0x4d4d4949) >> 16), magic(42) {}

  const uint16_t t_order, magic;

  void WriteTo(data_writer &a_Writer)
  {
    a_Writer.Write(t_order);
    a_Writer.Write(magic);
  }
};

struct metadata_payload_write_handle
{
  metadata_payload_write_handle(deferred_write_handle a_OffsetDataHandle,
                                const std::vector<uint8_t> &a_ValueToWrite);
  const deferred_write_handle OffsetDataHandle;
  const std::vector<uint8_t> &ValueToWrite;
};

metadata_payload_write_handle::metadata_payload_write_handle(
    deferred_write_handle a_OffsetDataHandle,
    const std::vector<uint8_t> &a_ValueToWrite)
    : OffsetDataHandle(a_OffsetDataHandle), ValueToWrite(a_ValueToWrite)
{
}

metadata_rational::metadata_rational(uint32_t a_numerator,
                                     uint32_t a_denominator)
    : Numerator(a_numerator), Denominator(a_denominator)
{
}

tiff_metadata_value::tiff_metadata_value(uint16_t a_Id, uint32_t a_Value)
    : Id(a_Id), Type(image_metadata_value_type::UInt32), Count(1),
      ValueInt(a_Value)
{
}

tiff_metadata_value::tiff_metadata_value(uint16_t a_Id, uint16_t a_Value)
    : Id(a_Id), Type(image_metadata_value_type::UInt16),
      Count(1), ValueShort{a_Value, 0}
{
}

tiff_metadata_value::tiff_metadata_value(uint16_t a_Id, uint8_t a_Value)
    : Id(a_Id), Type(image_metadata_value_type::UInt8),
      Count(1), ValueBytes{a_Value, 0, 0, 0}
{
}

tiff_metadata_value::tiff_metadata_value(uint16_t a_Id,
                                         metadata_rational a_Value)
    : Id(a_Id), Type(image_metadata_value_type::Rational), Count(1),
      Data(reinterpret_cast<uint8_t *>(&a_Value),
           reinterpret_cast<uint8_t *>(&a_Value) + sizeof(a_Value))
{
}

tiff_metadata_value::tiff_metadata_value(
    uint16_t a_Id, const std::initializer_list<uint16_t> &a_Value)
    : Id(a_Id), Type(image_metadata_value_type::UInt16),
      Count(static_cast<uint32_t>(a_Value.size())),
      Data(reinterpret_cast<const uint8_t *>(a_Value.begin()),
           reinterpret_cast<const uint8_t *>(a_Value.end()))
{
}

tiff_metadata_value::tiff_metadata_value(uint16_t a_Id,
                                         const std::string &a_Value)
    : tiff_metadata_value(a_Id, image_metadata_value_type::ASCII,
                          reinterpret_cast<const uint8_t *>(a_Value.data()),
                          sizeof(uint8_t), static_cast<int32_t>(a_Value.size()),
                          static_cast<int32_t>(a_Value.size()))
{
}

tiff_metadata_value::tiff_metadata_value(uint16_t a_Id, const char *a_Value,
                                         size_t a_padValueToSize)
    : tiff_metadata_value(a_Id, image_metadata_value_type::ASCII,
                          reinterpret_cast<const uint8_t *>(a_Value),
                          sizeof(uint8_t),
                          static_cast<uint32_t>(strlen(a_Value)),
                          static_cast<uint32_t>(a_padValueToSize))
{
}

tiff_metadata_value::tiff_metadata_value(uint16_t a_Id,
                                         image_metadata_value_type a_valueType,
                                         int32_t a_count)
    : Id(a_Id), Type(a_valueType), Count(a_count)
{
}

tiff_metadata_value::tiff_metadata_value(uint16_t a_Id,
                                         image_metadata_value_type a_ValueType,
                                         const void *a_ValueStart,
                                         size_t a_valueSize,
                                         uint32_t a_valueCount,
                                         int32_t a_writtenBufferSizeBytes)
    : Id(a_Id), Type(a_ValueType)
{
  assert(a_valueSize * a_valueCount <= a_writtenBufferSizeBytes ||
         a_writtenBufferSizeBytes == -1);

  if (a_valueSize * a_valueCount <= 4 && a_writtenBufferSizeBytes <= 4)
  {
    ValueInt = 0;
    std::memcpy(&ValueInt, a_ValueStart, a_valueSize * a_valueCount);
  }
  else
  {
    Data = std::vector<uint8_t>(a_writtenBufferSizeBytes > 0
                                    ? a_writtenBufferSizeBytes
                                    : a_valueSize * a_valueCount,
                                '\0');
    std::copy_n(static_cast<const uint8_t *>(a_ValueStart),
                a_valueSize * a_valueCount, Data.data());
  }
  Count = (a_writtenBufferSizeBytes > 0)
              ? a_writtenBufferSizeBytes / a_valueSize
              : a_valueCount;
}

void tiff_metadata_value::WriteIDFEntryTo(
    data_writer &a_writer,
    std::vector<metadata_payload_write_handle> &a_payloadQueue) const
{
  a_writer.Write(Id);
  a_writer.Write(Type);
  a_writer.Write(Count);

  if (Data.empty())
  {
    a_writer.Write(ValueInt);
  }
  else
  {
    deferred_write_handle handle =
        a_writer.DeferWrite(static_cast<uint16_t>(sizeof(Offset)));
    a_payloadQueue.emplace_back(handle, Data);
  }
}

void tiff_header::SetMetadata(image_metadata_section section,
                              const tiff_metadata_value &value)
{
  m_MetadataSections[static_cast<int>(section)].emplace_back(value);
}

void tiff_header::Populate(const libraw_data_t &a_data,
                           const libraw_internal_data_t &a_internalData,
                           bool a_populateFullData)
{
  PopulateTiffMetadata(m_MetadataSections[static_cast<int>(
                           image_metadata_section::TiffMetadata)],
                       a_data, a_internalData, a_populateFullData);
  PopulateExifMetadata(
      m_MetadataSections[static_cast<int>(image_metadata_section::Exif)],
      a_data);
  PopulateGpsMetadata(
      m_MetadataSections[static_cast<int>(image_metadata_section::GPS)],
      a_data);
}

void tiff_header::PopulateTiffMetadata(
    MetadataValueList &a_target, const libraw_data_t &a_data,
    const libraw_internal_data_t &a_internalData, bool a_populateFullData)
{
  a_target.emplace_back(tiff_params::NewSubfileType, static_cast<uint32_t>(0u));
  a_target.emplace_back(tiff_params::ImageWidth,
                        static_cast<uint32_t>(a_data.sizes.width));
  a_target.emplace_back(tiff_params::ImageHeight,
                        static_cast<uint32_t>(a_data.sizes.height));
  uint16_t bitsPerSample = static_cast<uint16_t>(a_data.params.output_bps);
  a_target.emplace_back(tiff_params::BitsPerSample,
                        std::initializer_list<uint16_t>{
                            bitsPerSample, bitsPerSample, bitsPerSample});
  a_target.emplace_back(tiff_params::Compression, static_cast<uint16_t>(1));
  a_target.emplace_back(tiff_params::PhotometricInterpretation,
                        static_cast<uint16_t>(1 + (a_data.idata.colors > 1)));
  a_target.emplace_back(tiff_params::ImageTitle, a_data.other.desc,
                        sizeof(a_data.other.desc));
  a_target.emplace_back(tiff_params::Make, a_data.idata.make,
                        sizeof(a_data.idata.make));
  a_target.emplace_back(tiff_params::Model, a_data.idata.model,
                        sizeof(a_data.idata.model));
  if (a_populateFullData)
  {
    uint32_t outputProfileSize =
        (a_internalData.output_data.oprof != nullptr)
            ? ntohl(a_internalData.output_data.oprof[0])
            : 0;
    a_target.emplace_back(tiff_params::ImageDataLocation,
                          static_cast<uint32_t>(4096 + outputProfileSize));
    a_target.emplace_back(tiff_params::SamplesPerPixel,
                          static_cast<uint16_t>(a_data.idata.colors));
    a_target.emplace_back(tiff_params::RowsPerStrip,
                          static_cast<uint16_t>(a_data.sizes.height));
    a_target.emplace_back(
        tiff_params::StripByteCounts,
        static_cast<uint32_t>(a_data.sizes.width * a_data.sizes.height *
                              a_data.idata.colors * a_data.params.output_bps /
                              8));
    if (outputProfileSize > 0)
    {
      a_target.emplace_back(tiff_params::ICCProfile,
                            image_metadata_value_type::Undefined,
                            outputProfileSize);
    }
  }
  else
  {
    a_target.emplace_back(
        tiff_params::Orientation,
        static_cast<uint16_t>("12435867"[a_data.sizes.flip] - '0'));
  }

  a_target.emplace_back(tiff_params::XResolution, metadata_rational(300, 1));
  a_target.emplace_back(tiff_params::YResolution, metadata_rational(300, 1));
  a_target.emplace_back(tiff_params::PlanarConfiguration,
                        static_cast<uint16_t>(1));
  a_target.emplace_back(tiff_params::ResolutionUnit, static_cast<uint16_t>(2));
  a_target.emplace_back(tiff_params::Software, "dcraw v" DCRAW_VERSION, 64);

  char dateTimeBuffer[20];
  struct tm *t = localtime(&a_data.other.timestamp);
  sprintf_s(dateTimeBuffer, "%04d:%02d:%02d %02d:%02d:%02d", t->tm_year + 1900,
            t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
  a_target.emplace_back(tiff_params::DateTime, dateTimeBuffer, 20);
  a_target.emplace_back(tiff_params::Artist, a_data.other.artist,
                        sizeof(a_data.other.artist));
  a_target.emplace_back(tiff_params::ExifIFDPointer, 0u);
  a_target.emplace_back(tiff_params::GPSIDF, 0u);
}

void tiff_header::PopulateExifMetadata(MetadataValueList &a_target,
                                       const libraw_data_t &a_data)
{
  static constexpr uint32_t FractionPrecision = 1000000;
  a_target.emplace_back(
      exif_params::ExposureTime,
      metadata_rational(
          static_cast<uint32_t>(FractionPrecision * a_data.other.shutter),
          FractionPrecision));
  a_target.emplace_back(
      exif_params::FNumber,
      metadata_rational(
          static_cast<uint32_t>(FractionPrecision * a_data.other.aperture),
          FractionPrecision));
  a_target.emplace_back(exif_params::IsoSpeedRatings,
                        static_cast<uint16_t>(a_data.other.iso_speed));
}

void tiff_header::PopulateGpsMetadata(MetadataValueList &a_target,
                                      const libraw_data_t &a_data)
{
  uint8_t latRef[4] = {static_cast<uint8_t>(a_data.other.gpsdata[29]), 0, 0, 0},
          lonRef[4] = {static_cast<uint8_t>(a_data.other.gpsdata[30]), 0, 0, 0};
  uint32_t gpsTagValue = 0x202;
  a_target.emplace_back(0, image_metadata_value_type::UInt8, &gpsTagValue, 1u,
                        static_cast<uint32_t>(sizeof(gpsTagValue)));
  a_target.emplace_back(1, image_metadata_value_type::ASCII, latRef, 2, 2);
  a_target.emplace_back(2, image_metadata_value_type::Rational,
                        &a_data.other.gpsdata[0], sizeof(metadata_rational), 3);
  a_target.emplace_back(3, image_metadata_value_type::ASCII, lonRef, 2, 2);
  a_target.emplace_back(4, image_metadata_value_type::Rational,
                        &a_data.other.gpsdata[4], sizeof(metadata_rational), 3);
  a_target.emplace_back(5, static_cast<uint8_t>(a_data.other.gpsdata[31]));
  a_target.emplace_back(
      6, metadata_rational(a_data.other.gpsdata[18], a_data.other.gpsdata[19]));
  a_target.emplace_back(
      6, metadata_rational(a_data.other.gpsdata[18], a_data.other.gpsdata[19]));
  a_target.emplace_back(7, image_metadata_value_type::Rational,
                        &a_data.other.gpsdata[12], sizeof(metadata_rational),
                        3);
  a_target.emplace_back(18, image_metadata_value_type::ASCII,
                        &a_data.other.gpsdata[20], sizeof(uint8_t), 12);
  a_target.emplace_back(29, image_metadata_value_type::ASCII,
                        &a_data.other.gpsdata[23], sizeof(uint8_t), 12);
}

void tiff_header::Write(FILE *a_targetStream)
{
  TryReplaceMetadataValue(
      image_metadata_section::TiffMetadata,
      tiff_metadata_value(tiff_params::ExifIFDPointer,
                          CalculateIFDOffset(image_metadata_section::Exif)));
  TryReplaceMetadataValue(
      image_metadata_section::TiffMetadata,
      tiff_metadata_value(tiff_params::GPSIDF,
                          CalculateIFDOffset(image_metadata_section::GPS)));

  uint32_t iccProfileSize = 0;
  uint32_t totalHeaderSize = GetTotalTiffHeaderSize();
  tiff_metadata_value *iccValue = nullptr;
  if (TryFindMetadataValue(image_metadata_section::TiffMetadata,
                           tiff_params::ICCProfile, iccValue))
  {
    iccValue->Offset = totalHeaderSize;
    iccProfileSize = iccValue->Count;
  }

  TryReplaceMetadataValue(
      image_metadata_section::TiffMetadata,
      tiff_metadata_value(tiff_params::ImageDataLocation,
                          totalHeaderSize + iccProfileSize));

  std::vector<metadata_payload_write_handle> payloadQueue{};

  data_writer writer(a_targetStream);
  tiff_file_header header;
  header.WriteTo(writer);
  writer.Write(CalculateIFDOffset(image_metadata_section::TiffMetadata));

  for (int i = 0; i < static_cast<int>(
                          image_metadata_section::tiff_metadata_section_count);
       ++i)
  {
    WriteIFDSection(writer, m_MetadataSections[i], payloadQueue);
    // Turns out, if we add offsets EXIF and GPS will be picked up as regular
    // IDF entries. We are always writing a 0 offset to avoid the metadata be
    // parsed twice. In the future if we want to have IFD1 or further this needs
    // to change.
    writer.Write(0u);
  }

  for (const metadata_payload_write_handle &payload : payloadQueue)
  {
    writer.Write(payload.OffsetDataHandle, writer.GetBytePosition());
    writer.Write(payload.ValueToWrite.data(),
                 static_cast<uint32_t>(payload.ValueToWrite.size()));
  }

  assert(GetTotalTiffHeaderSize() == writer.GetBytePosition());
}

uint32_t tiff_header::CalculateIFDOffset(image_metadata_section a_section) const
{
  uint32_t offset = sizeof(tiff_file_header) +
                    /*ifdStartOffset*/ sizeof(uint16_t) +
                    /*ifdEntryCount*/ sizeof(uint16_t);
  for (int i = 0; i < static_cast<int>(a_section); ++i)
  {
    offset +=
        m_MetadataSections[i].size() * tiff_metadata_value::WrittenSizeBytes;
    offset += /* nextIdfOffset */ sizeof(uint32_t) +
              /*ifdEntryCount*/ sizeof(uint16_t);
  }
  return offset;
}

uint32_t tiff_header::GetTotalTiffHeaderSize() const
{
  uint32_t offset =
      CalculateIFDOffset(image_metadata_section::tiff_metadata_section_count);
  return offset + GetDataPayloadSize();
}

uint32_t tiff_header::GetDataPayloadSize() const
{
  uint32_t payloadSize = 0;
  for (int i = 0; i < static_cast<int>(
                          image_metadata_section::tiff_metadata_section_count);
       ++i)
  {
    for (const tiff_metadata_value &value : m_MetadataSections[i])
    {
      payloadSize += value.Data.size();
    }
  }
  return payloadSize;
}

bool tiff_header::TryReplaceMetadataValue(image_metadata_section a_section,
                                          tiff_metadata_value a_newValue)
{
  tiff_metadata_value *result;
  if (TryFindMetadataValue(a_section, a_newValue.Id, result))
  {
    *result = a_newValue;
    return true;
  }
  return false;
}

bool tiff_header::TryFindMetadataValue(image_metadata_section a_section,
                                       uint16_t a_tag,
                                       tiff_metadata_value *&a_result) const
{
  const MetadataValueList &targetContainer =
      m_MetadataSections[static_cast<int>(a_section)];
  auto it = std::find_if(
      targetContainer.begin(), targetContainer.end(),
      [a_tag](const tiff_metadata_value &val) { return val.Id == a_tag; });
  if (it != targetContainer.end())
  {
    a_result = it._Ptr;
    return true;
  }
  return false;
}

void tiff_header::WriteIFDSection(
    data_writer &a_targetStream, const MetadataValueList &a_container,
    std::vector<metadata_payload_write_handle> &a_payloadQueue)
{
  uint16_t ifdEntryCount = static_cast<uint16_t>(a_container.size());
  a_targetStream.Write(ifdEntryCount);

  for (const tiff_metadata_value &value : a_container)
  {
    value.WriteIDFEntryTo(a_targetStream, a_payloadQueue);
  }
}
