// Runs some basic tests on specified data and serves as an example for using
// the code.

#include <iostream>
#include <string>
#include <vector>

#include "./hsi_data_reader.h"

using hsi::HSIData;
using hsi::HSIDataOptions;
using hsi::HSIDataReader;

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "Required argument: path to HSI file." << std::endl;
    return -1;
  }
  const std::string file_path(argv[1]);

  // Set range of data we want to read.
  hsi::HSIDataRange data_range;
  data_range.start_row = 3380;
  data_range.end_row = 3383;
  data_range.start_col = 7030;
  data_range.end_col = 7038;
  data_range.start_band = 1000;
  data_range.end_band = 1506;

  // Set data options.
  HSIDataOptions data_options(file_path);
  data_options.interleave_format = hsi::HSI_INTERLEAVE_BSQ;
  data_options.data_type = hsi::HSI_DATA_TYPE_FLOAT;
  data_options.big_endian = false;
  data_options.header_offset = 0;
  data_options.num_data_rows = 11620;
  data_options.num_data_cols = 11620;
  data_options.num_data_bands = 1506;

  HSIDataReader reader(data_options);
  std::cout << "Reading data from file '" << file_path << "'." << std::endl;
  const bool success = reader.ReadData(data_range);
  if (!success) {
    return -1;
  }

  const HSIData& hsi_data = reader.GetData();
  std::cout << "Successfully loaded " << hsi_data.NumDataPoints()
            << " values." << std::endl;

  // Write the data.
  const std::string temp_save_path = "./tmp_data";
  if (!reader.WriteData(temp_save_path)) {
    return -1;
  }

  // Read the written-out data and check if it's the same as before.
  HSIDataOptions data_options_2(temp_save_path);
  data_options_2.interleave_format = hsi::HSI_INTERLEAVE_BSQ;
  data_options_2.data_type = hsi::HSI_DATA_TYPE_FLOAT;
  data_options_2.big_endian = false;
  data_options_2.header_offset = 0;
  data_options_2.num_data_rows = hsi_data.num_rows;
  data_options_2.num_data_cols = hsi_data.num_cols;
  data_options_2.num_data_bands = hsi_data.num_bands;
  HSIDataReader reader_2(data_options_2);
  hsi::HSIDataRange data_range_2;
  data_range_2.start_row = 0;
  data_range_2.end_row = hsi_data.num_rows;
  data_range_2.start_col = 0;
  data_range_2.end_col = hsi_data.num_cols;
  data_range_2.start_band = 0;
  data_range_2.end_band = hsi_data.num_bands;
  reader_2.ReadData(data_range_2);
  const HSIData& hsi_data_2 = reader_2.GetData();
  std::cout << "Successfully re-loaded " << hsi_data.NumDataPoints()
            << " saved values." << std::endl;
  if (hsi_data.NumDataPoints() != hsi_data_2.NumDataPoints()) {
    std::cerr << "Number of data points does not match." << std::endl;
    return -1;
  }
  for (int i = 0; i < hsi_data.NumDataPoints(); ++i) {
    if (hsi_data.data[i] != hsi_data_2.data[i]) {
      std::cerr << "Mismatched value " << i << ": "
                << hsi_data.data[i] << " vs. " << hsi_data_2.data[i]
                << "." << std::endl;
    }
  }

  // Print the original results.
  for (int row = 0; row < hsi_data.num_rows; ++row) {
    for (int col = 0; col < hsi_data.num_cols; ++col) {
      for (int band = 0; band < hsi_data.num_bands; ++band) {
        std::cout << hsi_data.GetValue(row, col, band) << std::endl;
      }
      std::cout << "-----" << std::endl;
    }
  }

  const std::vector<float> band_0 = hsi_data.GetSpectrum(0, 0);
  for (const float val : band_0) {
//    std::cout << val << std::endl;
  }

  return 0;
}
