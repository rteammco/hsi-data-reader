#include <iostream>

#include <string>

#include "hsi_data_reader.h"

using namespace hsi;

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "Required argument: path to HSI file." << std::endl;
    return -1;
  }
  const std::string file_path(argv[1]);

  // Set range of data we want to read.
  const int start_row = 3380;
  const int end_row = 3384;
  const int start_col = 7030;
  const int end_col = 7034;
  const int start_band = 1000;
  const int end_band = 1506;

  //const int start_row = 10600;
  //const int end_row = 10605;
  //const int start_col = 120;
  //const int end_col = 125;
  //const int start_band = 370;
  //const int end_band = 390;

  // Set data options.
  HSIDataOptions data_options(file_path);
  data_options.interleave_format = HSI_INTERLEAVE_BSQ;
  data_options.data_type = HSI_DATA_TYPE_FLOAT;
  data_options.big_endian = false;
  data_options.header_offset = 0;
  data_options.num_data_rows = 11620;
  data_options.num_data_cols = 11620;
  data_options.num_data_bands = 1506;

  HSIDataReader reader(data_options);
  std::cout << "Reading data from file '" << file_path << "'." << std::endl;
  const bool success = reader.ReadData(
      start_row, end_row, start_col, end_col, start_band, end_band);
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
  data_options_2.interleave_format = HSI_INTERLEAVE_BSQ;
  data_options_2.data_type = HSI_DATA_TYPE_FLOAT;
  data_options_2.big_endian = false;
  data_options_2.header_offset = 0;
  data_options_2.num_data_rows = hsi_data.num_rows;
  data_options_2.num_data_cols = hsi_data.num_cols;
  data_options_2.num_data_bands = hsi_data.num_bands;
  HSIDataReader reader_2(data_options_2);
  reader_2.ReadData(
      0, hsi_data.num_rows, 0, hsi_data.num_cols, 0, hsi_data.num_bands);
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
    } else {
      std::cout << hsi_data.data[i] << std::endl;
    }
  }

  return 0;
}
