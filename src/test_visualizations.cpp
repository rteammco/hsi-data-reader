// If OpenCV is available, this code will be compiled by Cmake and will allow
// additiona testing of the HSIDataReader by visualizing the data.

#include <iostream>
#include <string>

#include "./hsi_data_reader.h"

#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "Required argument: path to HSI file." << std::endl;
    return -1;
  }
  const std::string file_path(argv[1]);

  // Set range of data we want to read.
  hsi::HSIDataRange data_range;
  data_range.start_row = 0;
  data_range.end_row = 660;
  data_range.start_col = 0;
  data_range.end_col = 790;
  data_range.start_band = 380;
  data_range.end_band = 400;

  // Set data options.
  hsi::HSIDataOptions data_options(file_path);
  data_options.interleave_format = hsi::HSI_INTERLEAVE_BSQ;
  data_options.data_type = hsi::HSI_DATA_TYPE_FLOAT;
  data_options.big_endian = false;
  data_options.header_offset = 0;
  data_options.num_data_rows = 660;
  data_options.num_data_cols = 790;
  data_options.num_data_bands = 1506;

  // Read the data.
  hsi::HSIDataReader reader(data_options);
  std::cout << "Reading data from file '" << file_path << "'." << std::endl;
  const bool success = reader.ReadData(data_range);
  if (!success) {
    return -1;
  }

  // Visualize the images.
  const hsi::HSIData& hsi_data = reader.GetData();
  for (int band = 0; band < hsi_data.num_bands; ++band) {
    cv::Mat m(cv::Size(hsi_data.num_cols, hsi_data.num_rows), CV_32FC1);
    for (int row = 0; row < hsi_data.num_rows; ++row) {
      for (int col = 0; col < hsi_data.num_cols; ++col) {
        m.at<float>(row, col) = hsi_data.GetValue(row, col, band);
      }
    }
    cv::imshow("Band Image", m);
    cv::waitKey(0);
  }

  return 0;
}
