#include "./hsi_data_reader.h"

#include <algorithm>
#include <fstream>
#include <functional>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace hsi {

/*******************************************************************************
*** Support Functions and Objects
*******************************************************************************/

// These functions are used to report errors and quit the program if necessary.
void Error(const std::string& message) {
  std::cerr << "Error: \"" << message << "\"." << std::endl;
}
void FatalError(const std::string& message) {
  Error(message);
  std::cerr << "Terminating program with fatal error." << std::endl;
  exit(-1);
}

// Trims all whitespace from the sides of the string (including new lines).
// Taken from:
// http://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
std::string TrimString(const std::string& string_to_trim) {
  std::string trimmed_string = string_to_trim;
  // Trim left.
  trimmed_string.erase(trimmed_string.begin(), std::find_if(
      trimmed_string.begin(),
      trimmed_string.end(),
      std::not1(std::ptr_fun<int, int>(std::isspace))));
  // Trim right.
  trimmed_string.erase(
      std::find_if(
          trimmed_string.rbegin(),
          trimmed_string.rend(),
          std::not1(std::ptr_fun<int, int>(std::isspace))).base(),
      trimmed_string.end());
  return trimmed_string;
}

// Returns a map of the configuration key/value pairs stored in the given file.
// Assumes one key/value pair per line, delimited by a '=' character.
std::unordered_map<std::string, std::string> GetConfigFileValues(
    const std::string& config_file_path) {

  std::unordered_map<std::string, std::string> config_values;

  std::ifstream config_file(config_file_path);
  if (!config_file.is_open()) {
    Error("Configuration file '" + config_file_path +
          "' could not be opened for reading.");
    return config_values;
  }

  std::string line;
  while (std::getline(config_file, line)) {
    // Skip comment lines.
    if (line.find('#') == 0) {
      continue;
    }
    // Find the '=' delimiter.
    const int split_position = line.find('=');
    if (split_position <= 0) {
      continue;
    }
    const std::string key = TrimString(line.substr(0, split_position));
    const std::string value = TrimString(line.substr(split_position + 1));
    config_values[key] = value;
  }
  config_file.close();

  return config_values;
}

// Returns the size of the data value based on the given HSIDataType.
int GetDataSize(const HSIDataType& data_type) {
  switch (data_type) {
    case HSI_DATA_TYPE_BYTE:
      return sizeof(char);
    case HSI_DATA_TYPE_INT16:
      return sizeof(int16_t);
    case HSI_DATA_TYPE_INT32:
      return sizeof(int32_t);
    case HSI_DATA_TYPE_DOUBLE:
      return sizeof(double);
    case HSI_DATA_TYPE_UNSIGNED_INT16:
      return sizeof(uint16_t);
    case HSI_DATA_TYPE_UNSIGNED_INT32:
      return sizeof(uint32_t);
    case HSI_DATA_TYPE_UNSIGNED_INT64:
      return sizeof(uint64_t);
    case HSI_DATA_TYPE_UNSIGNED_LONG:
      return sizeof(unsigned long);
    case HSI_DATA_TYPE_FLOAT:
    default:
      return sizeof(float);
  }
}

// Reverse the bytes in the given bytes array. Assumes that the given array
// contains data_size values.
void ReverseBytes(const int data_size, char* bytes) {
  for (int i = 0; i < data_size / 2; ++i) {
    const int end_index = data_size - 1 - i;
    const char temp = bytes[i];
    bytes[i] = bytes[end_index];
    bytes[end_index] = temp;
  }
}

// Reads the next value in the file from the given file value index. This is
// a generic binary data read, and can be used to read the next value (of any
// bye size) from an HSI file.
void ReadNextValue(
    const long next_value_index,
    const long current_value_index,
    const int data_size,
    std::ifstream* data_file,
    std::vector<char>* raw_data,
    const bool reverse_byte_order) {

  // Skip to next position if necessary.
  if (next_value_index > (current_value_index + 1)) {
    data_file->seekg(next_value_index * data_size);
  }
  char next_bytes[data_size];  // NOLINT
  data_file->read(next_bytes, data_size);
  if (reverse_byte_order) {
    ReverseBytes(data_size, next_bytes);
  }
  raw_data->insert(raw_data->end(), next_bytes, next_bytes + data_size);
}

// Does a data read assuming the data is in BSQ format.
// BSQ is ordered as band > row > col.
void ReadDataBSQ(
    const HSIDataOptions& data_options,
    const bool machine_big_endian,
    const HSIDataRange& data_range,
    const long start_index,
    std::ifstream* data_file,
    HSIData* hsi_data) {

  const int data_size = GetDataSize(hsi_data->data_type);

  // Skip to current index.
  long current_index = start_index;
  data_file->seekg(current_index * data_size);

  const bool reverse_byte_order =
      (data_options.big_endian != machine_big_endian);
  const long num_pixels_per_band =
      data_options.num_data_rows * data_options.num_data_cols;
  for (int band = data_range.start_band; band < data_range.end_band; ++band) {
    const long band_index = band * num_pixels_per_band;
    for (int row = data_range.start_row; row < data_range.end_row; ++row) {
      for (int col = data_range.start_col; col < data_range.end_col; ++col) {
        const long pixel_index = row * data_options.num_data_cols + col;
        const long next_index = band_index + pixel_index;
        ReadNextValue(
            next_index,
            current_index,
            data_size,
            data_file,
            &(hsi_data->raw_data),
            reverse_byte_order);
        current_index = next_index;
      }
    }
  }
}

// Does a data read assuming the data is in BIL format.
// BIL is ordered as row > band > col.
void ReadDataBIL(
    const HSIDataOptions& data_options,
    const bool machine_big_endian,
    const HSIDataRange& data_range,
    const long start_index,
    std::ifstream* data_file,
    HSIData* hsi_data) {

  const int data_size = GetDataSize(hsi_data->data_type);

  // Skip to current index.
  long current_index = start_index;
  data_file->seekg(current_index * data_size);

  const bool reverse_byte_order =
      (data_options.big_endian != machine_big_endian);
  const long num_values_per_row =
      data_options.num_data_bands * data_options.num_data_cols;
  for (int row = data_range.start_row; row < data_range.end_row; ++row) {
    const long row_index = row * num_values_per_row;
    for (int band = data_range.start_band; band < data_range.end_band; ++band) {
      for (int col = data_range.start_col; col < data_range.end_col; ++col) {
        const long next_index =
            row_index + band * data_options.num_data_cols + col;
        ReadNextValue(
            next_index,
            current_index,
            data_size,
            data_file,
            &(hsi_data->raw_data),
            reverse_byte_order);
        current_index = next_index;
      }
    }
  }
}

// Does a data read assuming the data is in BIP format.
// BIP is ordered as row > col > band.
void ReadDataBIP(
    const HSIDataOptions& data_options,
    const bool machine_big_endian,
    const HSIDataRange& data_range,
    const long start_index,
    std::ifstream* data_file,
    HSIData* hsi_data) {

  const int data_size = GetDataSize(hsi_data->data_type);

  // Skip to current index.
  long current_index = start_index;
  data_file->seekg(current_index * data_size);

  const bool reverse_byte_order =
      (data_options.big_endian != machine_big_endian);
  const long num_values_per_row =
      data_options.num_data_bands * data_options.num_data_cols;
  for (int row = data_range.start_row; row < data_range.end_row; ++row) {
    const long row_index = row * num_values_per_row;
    for (int col = data_range.start_col; col < data_range.end_col; ++col) {
      for (int band = data_range.start_band;
           band < data_range.end_band;
           ++band) {
        const long next_index =
            row_index + col * data_options.num_data_bands + band;
        ReadNextValue(
            next_index,
            current_index,
            data_size,
            data_file,
            &(hsi_data->raw_data),
            reverse_byte_order);
        current_index = next_index;
      }
    }
  }
}

/*******************************************************************************
*** HSIDataOptions
*******************************************************************************/

void HSIDataOptions::ReadHeaderFromFile(const std::string& header_file_path) {
  std::unordered_map<std::string, std::string> header_values =
      GetConfigFileValues(header_file_path);
  if (header_values.empty()) {
    FatalError("No header values available.");
  }
  std::unordered_map<std::string, std::string>::const_iterator itr;

  itr = header_values.find("data");
  if (itr != header_values.end()) {
    hsi_file_path = itr->second;
  }

  // If a header file path is specified (in a config file), set the data
  // parameters from that header instead.
  itr = header_values.find("header");
  if (itr != header_values.end()) {
    std::cout << "Reading header info from " << itr->second << std::endl;
    return ReadHeaderFromFile(itr->second);
  }

  itr = header_values.find("interleave");
  if (itr != header_values.end()) {
    if (itr->second == "bsq") {
      interleave_format = HSI_INTERLEAVE_BSQ;
      std::cout << "Option set: interleave BSQ." << std::endl;
    } else if (itr->second == "bip") {
      interleave_format = HSI_INTERLEAVE_BIP;
      std::cout << "Option set: interleave BIP." << std::endl;
    } else if (itr->second == "bil") {
      interleave_format = HSI_INTERLEAVE_BIL;
      std::cout << "Option set: interleave BIL." << std::endl;
    } else {
      FatalError("Unsupported/unknown data interleave format: " + itr->second);
    }
  }

  itr = header_values.find("data type");
  std::string data_type_name;
  if (itr != header_values.end()) {
    if (itr->second == "1" || itr->second == "byte") {
      data_type = HSI_DATA_TYPE_BYTE;
      data_type_name = "byte";
    } else if (itr->second == "2" || itr->second == "int16") {
      data_type = HSI_DATA_TYPE_INT16;
      data_type_name = "int16";
    } else if (itr->second == "3" || itr->second == "int32") {
      data_type = HSI_DATA_TYPE_INT32;
      data_type_name = "int32";
    } else if (itr->second == "4" || itr->second == "float") {
      data_type = HSI_DATA_TYPE_FLOAT;
      data_type_name = "float";
    } else if (itr->second == "5" || itr->second == "double") {
      data_type = HSI_DATA_TYPE_DOUBLE;
      data_type_name = "double";
    } else if (itr->second == "12" || itr->second == "uint16") {
      data_type = HSI_DATA_TYPE_UNSIGNED_INT16;
      data_type_name = "unsigned int16";
    } else if (itr->second == "13" || itr->second == "uint32") {
      data_type = HSI_DATA_TYPE_UNSIGNED_INT32;
      data_type_name = "unsigned int32";
    } else if (itr->second == "14" || itr->second == "uint64") {
      data_type = HSI_DATA_TYPE_UNSIGNED_INT64;
      data_type_name = "unsigned int64";
    } else if (itr->second == "15" || itr->second == "ulong") {
      data_type = HSI_DATA_TYPE_UNSIGNED_LONG;
      data_type_name = "unsigned long";
    } else {
      FatalError("Unsupported/unknown data type: " + itr->second);
    }
  }
  std::cout << "Option set: data type " << data_type_name << "." << std::endl;

  itr = header_values.find("byte order");
  if (itr != header_values.end()) {
    big_endian = (itr->second == "1");
    std::cout << "Option set: big endian = "
              << (big_endian ? "true" : "false") << "." << std::endl;
  }

  itr = header_values.find("header offset");
  if (itr != header_values.end()) {
    header_offset = std::atoi(itr->second.c_str());
    std::cout << "Header offset = " << header_offset << "." << std::endl;
  }

  if (interleave_format == HSI_INTERLEAVE_BSQ) {
    itr = header_values.find("samples");
  } else {
    itr = header_values.find("lines");
  }
  if (itr != header_values.end()) {
    num_data_rows = std::atoi(itr->second.c_str());
    std::cout << "Number of rows = " << num_data_rows << "." << std::endl;
  }

  if (interleave_format == HSI_INTERLEAVE_BSQ) {
    itr = header_values.find("lines");
  } else {
    itr = header_values.find("samples");
  }
  if (itr != header_values.end()) {
    num_data_cols = std::atoi(itr->second.c_str());
    std::cout << "Number of columns = " << num_data_cols << "." << std::endl;
  }

  itr = header_values.find("bands");
  if (itr != header_values.end()) {
    num_data_bands = std::atoi(itr->second.c_str());
    std::cout << "Number of bands = " << num_data_bands << "." << std::endl;
  }
}

/*******************************************************************************
*** HSIDataRange
*******************************************************************************/

void HSIDataRange::ReadRangeFromFile(const std::string& range_config_file) {
  std::unordered_map<std::string, std::string> range_values =
      GetConfigFileValues(range_config_file);
  if (range_values.empty()) {
    FatalError("No range values available.");
  }
  std::unordered_map<std::string, std::string>::const_iterator itr;

  itr = range_values.find("start row");
  if (itr != range_values.end()) {
    start_row = std::atoi(itr->second.c_str());
  }

  itr = range_values.find("end row");
  if (itr != range_values.end()) {
    end_row = std::atoi(itr->second.c_str());
  }

  itr = range_values.find("start col");
  if (itr != range_values.end()) {
    start_col = std::atoi(itr->second.c_str());
  }

  itr = range_values.find("end col");
  if (itr != range_values.end()) {
    end_col = std::atoi(itr->second.c_str());
  }

  itr = range_values.find("start band");
  if (itr != range_values.end()) {
    start_band = std::atoi(itr->second.c_str());
  }

  itr = range_values.find("end band");
  if (itr != range_values.end()) {
    end_band = std::atoi(itr->second.c_str());
  }
}

/*******************************************************************************
*** HSIData
*******************************************************************************/

HSIDataValue HSIData::GetValue(
    const int row, const int col, const int band) const {

  if (row < 0 || row >= num_rows) {
    Error("Row index out of range: " + std::to_string(row) +
          " must be between 0 and " + std::to_string(num_rows - 1));
    return HSIDataValue();
  }
  if (col < 0 || col >= num_cols) {
    Error("Column index out of range: " + std::to_string(col) +
          " must be between 0 and " + std::to_string(num_cols - 1));
    return HSIDataValue();
  }
  if (band < 0 || band >= num_bands) {
    Error("Band index out of range: " + std::to_string(band) +
          " must be between 0 and " + std::to_string(num_bands - 1));
    return HSIDataValue();
  }
  int index = 0;
  if (interleave_format == HSI_INTERLEAVE_BSQ) {
    // BSQ: band > row > col.
    const int band_index = (num_rows * num_cols) * band;
    index = band_index + (row * num_cols) + col;
  } else if (interleave_format == HSI_INTERLEAVE_BIL) {
    // BIL: row > band > col.
    const int row_index = (num_cols * num_bands) * row;
    index = row_index + (band * num_cols) + col;
  } else if (interleave_format == HSI_INTERLEAVE_BIP) {
    // BIP: row > col > band.
    const int row_index = (num_cols * num_bands) * row;
    index = row_index + (col * num_bands) + band;
  } else {
    Error("Unknown/unsupported interleave format.");
  }
  const int data_size = GetDataSize(data_type);
  const char* bytes = &(raw_data[index * data_size]);
  HSIDataValue value;
  // TODO: The byte order may change depending on machine endian.
  std::copy(bytes, bytes + data_size, value.bytes);
  return value;
}

std::vector<HSIDataValue> HSIData::GetSpectrum(
    const int row, const int col) const {

  std::vector<HSIDataValue> spectrum;
  spectrum.reserve(num_bands);
  for (int band = 0; band < num_bands; ++band) {
    spectrum.push_back(GetValue(row, col, band));
  }
  return spectrum;
}

/*******************************************************************************
*** HSIDataReader
*******************************************************************************/

HSIDataReader::HSIDataReader(const HSIDataOptions& data_options)
    : data_options_(data_options) {

  // Determine the machine endian. Union of memory means "unsigned int value"
  // and "unsigned char bytes" share the same memory.
  union UnsignedNumber {
    unsigned int value;
    unsigned char bytes[sizeof(unsigned int)];
  };

  // Set the value to unsigned 1, and check the byte array to see if it is in
  // big endian or little endian order. The left-most byte will be empty (zero)
  // if the machine is big endian.
  UnsignedNumber number;
  number.value = 1U;  // Unsigned int 1.
  machine_big_endian_ = (number.bytes[0] != 1U);
}

void HSIDataReader::ReadData(const HSIDataRange& data_range) {
  // Check that the given ranges are valid.
  if (data_range.start_row < 0 ||
      data_range.end_row > data_options_.num_data_rows) {
    FatalError("Invalid row range: must be between 0 and " +
               std::to_string(data_options_.num_data_rows));
  }
  if (data_range.start_col < 0 ||
      data_range.end_col > data_options_.num_data_cols) {
    FatalError("Invalid column range: must be between 0 and " +
               std::to_string(data_options_.num_data_cols));
  }
  if (data_range.start_band < 0 ||
      data_range.end_band > data_options_.num_data_bands) {
    FatalError("Invalid band range: must be between 0 and " +
               std::to_string(data_options_.num_data_bands));
  }

  // Check that the ranges are positive / valid.
  hsi_data_.num_rows = data_range.end_row - data_range.start_row;
  hsi_data_.num_cols = data_range.end_col - data_range.start_col;
  hsi_data_.num_bands = data_range.end_band - data_range.start_band;
  if (hsi_data_.num_rows <= 0) {
    FatalError("Row range must be positive.");
  }
  if (hsi_data_.num_cols <= 0) {
    FatalError("Column range must be positive.");
  }
  if (hsi_data_.num_bands <= 0) {
    FatalError("Band range must be positive.");
  }

  // Try to open the file.
  std::ifstream data_file(data_options_.hsi_file_path);
  if (!data_file.is_open()) {
    FatalError("File " + data_options_.hsi_file_path +
               " could not be opened for reading.");
  }

  // Set the size of the data vector and the HSI data struct.
  hsi_data_.raw_data.clear();
  const long num_data_points =
      hsi_data_.num_rows * hsi_data_.num_cols * hsi_data_.num_bands;
  const long num_bytes = num_data_points * GetDataSize(hsi_data_.data_type);
  hsi_data_.raw_data.reserve(num_bytes);
  hsi_data_.interleave_format = data_options_.interleave_format;
  hsi_data_.data_type = data_options_.data_type;

  if (data_options_.interleave_format == HSI_INTERLEAVE_BSQ) {
    ReadDataBSQ(
        data_options_,
        machine_big_endian_,
        data_range,
        data_options_.header_offset,
        &data_file,
        &hsi_data_);
  } else if (data_options_.interleave_format == HSI_INTERLEAVE_BIL) {
    ReadDataBIL(
        data_options_,
        machine_big_endian_,
        data_range,
        data_options_.header_offset,
        &data_file,
        &hsi_data_);
  } else if (data_options_.interleave_format == HSI_INTERLEAVE_BIP) {
    ReadDataBIP(
        data_options_,
        machine_big_endian_,
        data_range,
        data_options_.header_offset,
        &data_file,
        &hsi_data_);
  }
}

void HSIDataReader::WriteData(const std::string& save_file_path) const {
  std::ofstream data_file(save_file_path);
  if (!data_file.is_open()) {
    FatalError("File " + save_file_path +
               " could not be opened for writing.");
  }

  const bool reverse_byte_order =
      (data_options_.big_endian != machine_big_endian_);
  const int data_size = GetDataSize(hsi_data_.data_type);
  const int num_data_points = hsi_data_.raw_data.size() / data_size;
  for (long i = 0; i < num_data_points; ++i) {
    const long byte_index = i * data_size;
    char bytes[data_size];  // NOLINT
    std::copy(
        hsi_data_.raw_data.begin() + byte_index,
        hsi_data_.raw_data.begin() + byte_index + data_size,
        bytes);
    if (reverse_byte_order) {
      ReverseBytes(data_size, bytes);
    }
    data_file.write(bytes, data_size);
  }

  data_file.close();
}

}  // namespace hsi
