# HSI Data Reader

A simple C++ class that reads hyperspectral image data (ENVI format).

## Install

Just include `hsi_data_reader.h` and `hsi_data_reader.cpp` into your project. No installation or compilation required.

## Use

See the included `test_reader.cpp` as an example. You just need to provide an HSI data file path and either a header file path or information about the file.

#### If You Have an HSI Header File
```
#include "hsi_data_reader.h"
using namespace hsi;

int main(int argc, char** argv) {
  // Set paths to your files.
  const std::string hsi_data_path = "/path/to/hsi/file";
  const std::string hsi_header_path = "/path/to/hsi/header";
  
  HSIDataOptions data_options(hsi_data_path);
  data_options.ReadHeaderFromFile(hsi_header_path);
  
  // Read rows 0-100, cols 50-150, spectral bands 10-20.
  HSIDataReader reader(data_options);
  reader.ReadData(0, 100, 50, 150, 10, 20);
  
  // Access data. Get value at row 2, col 3, band 4.
  const HSIData& hsi_data = reader.GetData();
  const float v = hsi_data.GetValue(2, 3, 4);
  
  return 0;
}
```

#### Otherwise Specify Data Parameters
```
#include "hsi_data_reader.h"
using namespace hsi;

int main(int argc, char** argv) {
  // Set paths to your files.
  const std::string hsi_data_path = "/path/to/hsi/file";
  
  // Set data options.
  HSIDataOptions data_options(hsi_data_path);
  data_options.interleave_format = HSI_INTERLEAVE_BSQ;
  data_options.data_type = HSI_DATA_TYPE_FLOAT;
  data_options.big_endian = false;
  data_options.header_offset = 0;
  data_options.num_data_rows = 200;
  data_options.num_data_cols = 200;
  data_options.num_data_bands = 100;
  
  // Read rows 0-100, cols 50-150, spectral bands 10-20.
  HSIDataReader reader(data_options);
  reader.ReadData(0, 100, 50, 150, 10, 20);
  
  // Access data. Get value at row 2, col 3, band 4.
  const HSIData& hsi_data = reader.GetData();
  const float v = hsi_data.GetValue(2, 3, 4);
  
  return 0;
}
```

## TODO

<ul>
  <li> Currently only BSQ interleave and float data types are supported. </li>
</ul>
