// If OpenCV is available, this code will be compiled by Cmake and will allow
// additiona testing of the HSIDataReader by visualizing the data.
//
// Usage:
// $ Visualize /path/to/config/file
//
// See data/config.txt for an example.

#include <iostream>
#include <string>
#include <vector>

#include "./hsi_data_reader.h"

#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"

static const std::string kMainWindowName = "HSI Image Visualization";

// Returns the OpenCV matrix type to use, depending on the HSI data type.
// TODO: Add support for more types.
int GetOpenCVMatrixType(const hsi::HSIDataType data_type) {
  int matrix_type;
  switch (data_type) {
  case hsi::HSI_DATA_TYPE_INT16:
    matrix_type = CV_16SC1;
    break;
  case hsi::HSI_DATA_TYPE_DOUBLE:
    matrix_type = CV_64FC1;
    break;
  case hsi::HSI_DATA_TYPE_FLOAT:
  default:
    matrix_type = CV_32FC1;
    break;
  }
  return matrix_type;
}

// Set the pixel value of the matrix, interprented based on the type of data.
// TODO: Add support for more types.
void SetBandImagePixel(
    const int row,
    const int col,
    const hsi::HSIDataValue value,
    const hsi::HSIDataType data_type,
    cv::Mat* band_image) {

  switch (data_type) {
  case hsi::HSI_DATA_TYPE_INT16:
    band_image->at<int16_t>(row, col) = value.value_as_int16;
    break;
  case hsi::HSI_DATA_TYPE_DOUBLE:
    band_image->at<double>(row, col) = value.value_as_double;
    break;
  case hsi::HSI_DATA_TYPE_FLOAT:
  default:
    band_image->at<float>(row, col) = value.value_as_float;
    break;
  }
}

// Callback function for the slider, which will switch the display between
// different bands of the hyperspectral image.
void SliderMovedCallback(int slider_value, void* hsi_image_bands_ptr) {
  std::vector<cv::Mat>* hsi_image_bands =
      reinterpret_cast<std::vector<cv::Mat>*>(hsi_image_bands_ptr);
  cv::imshow(kMainWindowName, hsi_image_bands->at(slider_value));
}

// Callback function for mouse events. When the user clicks on a pixel, display
// the spectrum in a separate window.
void MouseEventCallback(
    const int event,
    const int x_pos,
    const int y_pos,
    const int flags,
    void* hsi_data_ptr) {

  if (event == CV_EVENT_LBUTTONDOWN) {
    const hsi::HSIData* hsi_data =
        reinterpret_cast<hsi::HSIData*>(hsi_data_ptr);
    const std::vector<hsi::HSIDataValue> spectrum =
        hsi_data->GetSpectrum(x_pos, y_pos);
    std::cout << "Clicked at " << x_pos << ", " << y_pos
              << ", got spectrum of size: " << spectrum.size() << std::endl;
  }
}

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
  reader.ReadData(data_range);

  // Create OpenCV images from the data.
  const hsi::HSIData& hsi_data = reader.GetData();
  const cv::Size band_image_size(hsi_data.num_cols, hsi_data.num_rows);
  std::vector<cv::Mat> hsi_image_bands;
  const int matrix_type = GetOpenCVMatrixType(hsi_data.data_type);
  for (int band = 0; band < hsi_data.num_bands; ++band) {
    cv::Mat band_image(band_image_size, matrix_type);
    for (int row = 0; row < hsi_data.num_rows; ++row) {
      for (int col = 0; col < hsi_data.num_cols; ++col) {
        SetBandImagePixel(
            row,
            col,
            hsi_data.GetValue(row, col, band),
            hsi_data.data_type,
            &band_image);
      }
    }
    hsi_image_bands.push_back(band_image);
  }
  if (hsi_image_bands.size() == 0) {
    std::cerr << "No bands to visualize. Quitting." << std::endl;
    return 0;
  }

  // Visualize the images so that the user can view them per-channel.
  cv::namedWindow(kMainWindowName);
  cv::setMouseCallback(
      kMainWindowName,
      MouseEventCallback,
      const_cast<void*>(reinterpret_cast<const void*>(&hsi_data)));
  int slider_value;
  cv::createTrackbar(
      "Band Selector",
      kMainWindowName,
      &slider_value,
      hsi_image_bands.size() - 1,
      SliderMovedCallback,
      reinterpret_cast<void*>(&hsi_image_bands));
  cv::imshow(kMainWindowName, hsi_image_bands[0]);
  std::cout << "Visualizing data. Press any key to close window." << std::endl;
  cv::waitKey(0);

  return 0;
}
