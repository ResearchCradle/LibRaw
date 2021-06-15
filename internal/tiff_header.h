#pragma once
#include <vector>
#include <string>

#include "libraw/libraw_types.h"
#include "libraw/image_metadata.h"

class data_writer;
struct deferred_write_handle;
struct metadata_payload_write_handle;
struct libraw_data_t;
struct libraw_internal_data_t;

class tiff_metadata_value
{
public:
  static constexpr int WrittenSizeBytes = 12;

  tiff_metadata_value(uint16_t a_Id, uint32_t a_Value);
  tiff_metadata_value(uint16_t a_Id, uint16_t a_Value);
  tiff_metadata_value(uint16_t a_Id, uint8_t a_Value);
  tiff_metadata_value(uint16_t a_Id, metadata_rational a_Value);
  tiff_metadata_value(uint16_t a_Id,
                      const std::initializer_list<uint16_t> &a_Value);
  tiff_metadata_value(uint16_t a_Id, const std::string &a_Value);
  tiff_metadata_value(uint16_t a_Id, const char *a_Value,
                      size_t a_padValueToSize);
  tiff_metadata_value(uint16_t a_Id, image_metadata_value_type a_valueType,
                     int32_t a_count);
  tiff_metadata_value(uint16_t a_Id, image_metadata_value_type a_ValueType,
                      const void *a_ValueStart, size_t a_valueSize,
                      uint32_t a_valueCount,
                      int32_t a_writtenBufferSizeBytes = -1);

  uint16_t Id;
  image_metadata_value_type Type;
  uint32_t Count; //Number of values.
  union
  {
    uint8_t ValueBytes[4];
    uint16_t ValueShort[2];
    uint32_t ValueInt;

    uint32_t Offset{}; // File offset in bytes of the Value field.
                             // Expected to start at a WORD boundary.
  };
  std::vector<uint8_t> Data{};

  void WriteIDFEntryTo(data_writer &a_writer, std::vector<metadata_payload_write_handle>& a_payloadQueue) const;
};

class tiff_header
{
    using MetadataValueList = std::vector<tiff_metadata_value>;
public:
    void SetMetadata(image_metadata_section section, const tiff_metadata_value& value);

  void Populate(const libraw_data_t &a_data,
                  const libraw_internal_data_t &a_internalData,
                bool a_populateFullData);
    void PopulateTiffMetadata(MetadataValueList& a_target,
                  const libraw_data_t &a_data,
                  const libraw_internal_data_t &a_internalData,
                  bool a_populateFullData);
  void PopulateExifMetadata(MetadataValueList &a_target,
                            const libraw_data_t &a_data);
    void PopulateGpsMetadata(MetadataValueList &a_target,
                             const libraw_data_t &a_data);
  void Write(FILE *a_targetStream);

private:
  uint32_t CalculateIFDOffset(image_metadata_section a_section) const;
  uint32_t GetTotalTiffHeaderSize() const;
  uint32_t GetDataPayloadSize() const;
  bool TryReplaceMetadataValue(image_metadata_section a_section,
                              tiff_metadata_value a_newValue);
  bool TryFindMetadataValue(image_metadata_section a_section, uint16_t a_tag, tiff_metadata_value*& a_result) const;

  void WriteIFDSection(data_writer &a_targetStream, const MetadataValueList &a_container,
                  std::vector<metadata_payload_write_handle> &a_payloadQueue);

  MetadataValueList m_MetadataSections[static_cast<int>(image_metadata_section::tiff_metadata_section_count)];
};