#include <iostream>

#include <string>

#include "hsi_data_reader.h"

using namespace hsi;

int main(int argc, char** argv) {

  const std::string file_path = "path/to/file";
  HSIDataOptions data_options(file_path);
  data_options.interleave_format = HSI_INTERLEAVE_BSQ;
  data_options.data_type = HSI_DATA_TYPE_FLOAT;
  data_options.big_endian = false;
  data_options.header_offset = 0;
  data_options.num_data_rows = 11620;
  data_options.num_data_cols = 11620;
  data_options.num_data_bands = 1506;

  const HSIDataReader reader(data_options);
  

  return 0;
}
