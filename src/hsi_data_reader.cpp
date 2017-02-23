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
  if (start_row < 0 || end_row >= data_options_.num_data_rows) {
    std::cerr << "Invalid row range: must be between 0 and "
              << (data_options_.num_data_rows - 1) << std::endl;
    return false;
  }
  if (start_col < 0 || end_col >= data_options_.num_data_cols) {
    std::cerr << "Invalid column range: must be between 0 and "
              << (data_options_.num_data_cols - 1) << std::endl;
    return false;
  }
  if (start_band < 0 || end_band >= data_options_.num_data_bands) {
    std::cerr << "Invalid band range: must be between 0 and "
              << (data_options_.num_data_bands - 1) << std::endl;
    return false;
  }

  // Determine the data size.
  const int data_size = sizeof(float);  // TODO: depends on options!

  // TODO: Implement here.
  std::ifstream data_file(data_options_.hsi_file_path);
  if (!data_file.is_open()) {
    std::cerr << "File " << data_options_.hsi_file_path
              << " could not be open." << std::endl;
    return false;
  }

  // TODO: Set size of vector. And this should be a private variable.
  std::vector<float> hsi_data;

  // TODO: something like this:
  // Skip the header offset.
  //data_file.seekg(data_options_.header_offset);
  // BSQ. TODO: adapt to other interleave formats.
  int current_index = 0;  // The current file pointer position.
  const int num_pixels =
      data_options_.num_data_rows * data_options_.num_data_cols;
  for (int band = start_band; band < end_band; ++band) {
    const int band_index = band * num_pixels;
    for (int row = start_row; row < end_row; ++row) {
      for (int col = start_col; col < end_col; ++col) {
        const int pixel_index = row * data_options_.num_data_cols + col;
        const int next_index = band_index + pixel_index;
        // Skip to next position if necessary.
        if (next_index > (current_index + 1)) {
          //data_file.seekg(offset);
        }
        char value[data_size];
        data_file.read(value, data_size);
        // TODO: any necessary endian conversions?
        hsi_data.push_back(*((float*)value));
        current_index = next_index;
      }
    }
  }

  // TODO: finish.
  for (const float x : hsi_data) {
    std::cout << x << std::endl;
  }

  return true;
}

}
