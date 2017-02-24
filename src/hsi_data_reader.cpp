#include "hsi_data_reader.h"

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace hsi {

template <typename T>
T ReverseBytes(const T value) {
  T reversed_value;
  const unsigned char* original_bytes = (const unsigned char*)(&value);
  unsigned char* reversed_bytes  = (unsigned char*)(&reversed_value);
  const int num_bytes = sizeof(T);
  for (int i = 0; i < num_bytes; ++i) {
    reversed_bytes[i] = original_bytes[num_bytes - 1 - i];
  }
  return reversed_value;
}

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

bool HSIDataReader::ReadData(
    const int start_row,
    const int end_row,
    const int start_col,
    const int end_col,
    const int start_band,
    const int end_band) {

  // Check that the given ranges are valid.
  if (start_row < 0 || end_row > data_options_.num_data_rows) {
    std::cerr << "Invalid row range: must be between 0 and "
              << data_options_.num_data_rows << std::endl;
    return false;
  }
  if (start_col < 0 || end_col > data_options_.num_data_cols) {
    std::cerr << "Invalid column range: must be between 0 and "
              << data_options_.num_data_cols << std::endl;
    return false;
  }
  if (start_band < 0 || end_band > data_options_.num_data_bands) {
    std::cerr << "Invalid band range: must be between 0 and "
              << data_options_.num_data_bands << std::endl;
    return false;
  }

  // Check that the ranges are positive / valid.
  hsi_data_.num_rows = end_row - start_row;
  hsi_data_.num_cols = end_col - start_col;
  hsi_data_.num_bands = end_band - start_band;
  if (hsi_data_.num_rows <= 0) {
    std::cerr << "Row range must be positive." << std::endl;
    return false;
  }
  if (hsi_data_.num_cols <= 0) {
    std::cerr << "Column range must be positive." << std::endl;
    return false;
  }
  if (hsi_data_.num_bands <= 0) {
    std::cerr << "Band range must be positive." << std::endl;
    return false;
  }

  // Try to open the file.
  std::ifstream data_file(data_options_.hsi_file_path);
  if (!data_file.is_open()) {
    std::cerr << "File " << data_options_.hsi_file_path
              << " could not be opened for reading." << std::endl;
    return false;
  }

  // Set the size of the data vector and the HSI data struct.
  hsi_data_.data.clear();
  const long num_data_points =
      hsi_data_.num_rows * hsi_data_.num_cols * hsi_data_.num_bands;
  hsi_data_.data.reserve(num_data_points);
  hsi_data_.interleave_format = data_options_.interleave_format;

  // Determine the data type size.
  const int data_size = sizeof(float);  // TODO: Type depends on options!

  // Skip the header offset.
  long current_index = data_options_.header_offset;
  data_file.seekg(current_index * data_size);

  // BSQ. TODO: adapt to other interleave formats.
  const long num_pixels =
      data_options_.num_data_rows * data_options_.num_data_cols;
  for (int band = start_band; band < end_band; ++band) {
    const long band_index = band * num_pixels;
    for (int row = start_row; row < end_row; ++row) {
      for (int col = start_col; col < end_col; ++col) {
        const long pixel_index = row * data_options_.num_data_cols + col;
        //const long pixel_index = col * data_options_.num_data_cols + row;
        const long next_index = band_index + pixel_index;
        // Skip to next position if necessary.
        if (next_index > (current_index + 1)) {
          data_file.seekg(next_index * data_size);
        }
        float value;
        data_file.read((char*)(&value), data_size);
        if (data_options_.big_endian != machine_big_endian_) {
          value = ReverseBytes<float>(value);
        }
        hsi_data_.data.push_back(value);
        current_index = next_index;
      }
    }
  }

  return true;
}

bool HSIDataReader::WriteData(const std::string& save_file_path) const {
  std::ofstream data_file(save_file_path);
  if (!data_file.is_open()) {
    std::cerr << "File " << save_file_path
              << " could not be opened for writing." << std::endl;
    return false;
  }

  // TODO: data type may not necessarily be "float".
  const int data_size = sizeof(float);
  for (const float value : hsi_data_.data) {
    float write_value = value;
    if (data_options_.big_endian != machine_big_endian_) {
      write_value = ReverseBytes<float>(value);
    }
    data_file.write((char*)(&write_value), data_size);
  }

  data_file.close();
  return true;
}

}  // namespace hsi
