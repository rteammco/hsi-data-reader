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
  const int start_row = 10600;
  const int end_row = 10605;
  const int start_col = 120;
  const int end_col = 125;
  const int start_band = 370;
  const int end_band = 390;

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

  return 0;
}
