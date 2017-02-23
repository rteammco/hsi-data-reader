#include "hsi_data_reader.h"

#include <fstream>
#include <iostream>
#include <vector>

namespace hsi {

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
  const int num_rows_to_read = end_row - start_row;
  const int num_cols_to_read = end_col - start_col;
  const int num_bands_to_read = end_band - start_band;
  if (num_rows_to_read <= 0) {
    std::cerr << "Row range must be positive." << std::endl;
    return false;
  }
  if (num_cols_to_read <= 0) {
    std::cerr << "Column range must be positive." << std::endl;
    return false;
  }
  if (num_bands_to_read <= 0) {
    std::cerr << "Band range must be positive." << std::endl;
    return false;
  }

  // TODO: Implement here.
  std::ifstream data_file(data_options_.hsi_file_path);
  if (!data_file.is_open()) {
    std::cerr << "File " << data_options_.hsi_file_path
              << " could not be open." << std::endl;
    return false;
  }

  // Set the size of the data vector.
  const long num_data_points =
      num_rows_to_read * num_cols_to_read * num_bands_to_read;
  hsi_data_.data.reserve(num_data_points);

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
    for (int col = start_col; col < end_col; ++col) {
      for (int row = start_row; row < end_row; ++row) {
        const long pixel_index = row * data_options_.num_data_cols + col;
        //const long pixel_index = col * data_options_.num_data_cols + row;
        const long next_index = band_index + pixel_index;
        // Skip to next position if necessary.
        if (next_index > (current_index + 1)) {
          data_file.seekg(next_index * data_size);
        }
        float value;
        data_file.read((char*)(&value), data_size);
        // TODO: Reverse bytes based on endian mode.
        //float reversed_value;
        //unsigned char* raw_value = (unsigned char*)(&value);
        //unsigned char* rev_value = (unsigned char*)(&reversed_value);
        //rev_value[0] = raw_value[3];
        //rev_value[1] = raw_value[2];
        //rev_value[2] = raw_value[1];
        //rev_value[3] = raw_value[0];
        //hsi_data.push_back(reversed_value);
        hsi_data_.data.push_back(value);
        std::cout << value /*<< " or " << reversed_value*/ << std::endl;
        current_index = next_index;
      }
    }
  }

  // TODO: finish.
  std::cout << hsi_data_.data.size() << std::endl;

  return true;
}

}
