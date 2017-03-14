// If OpenCV is available, this code will be compiled by Cmake and will allow
// additiona testing of the HSIDataReader by visualizing the data.
//
// Usage:
// $ Visualize /path/to/config/file
//
// See data/config.txt for an example.

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
  const std::string config_path(argv[1]);

  // Read in the data information from the config file.
  hsi::HSIDataOptions data_options;
  data_options.ReadHeaderFromFile(config_path);
  hsi::HSIDataRange data_range;
  data_range.ReadRangeFromFile(config_path);

  // Read the data.
  hsi::HSIDataReader reader(data_options);
  std::cout << "Reading data from file '" << data_options.hsi_file_path
            << "'." << std::endl;
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
