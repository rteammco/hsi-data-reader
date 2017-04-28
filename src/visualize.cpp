// If OpenCV is available, this code will be compiled by Cmake and will allow
// additiona testing of the HSIDataReader by visualizing the data.
//
// Usage:
// $ Visualize /path/to/config/file
//
// See data/config.txt for an example.

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#include "./hsi_data_reader.h"

#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

// Names of the windows to be displayed.
static const std::string kMainWindowName = "HSI Image Visualization";
static const std::string kSpectrumWindowName = "Pixel Spectrum";

// Spectrum plot window properties.
constexpr int kSpectrumPlotHeight = 400;
constexpr int kSpectrumPlotWidth = 800;
constexpr int kSpectrumPlotLineThickness = 1;
static const cv::Scalar kSpectrumPlotBackgroundColor(255, 255, 255);
static const cv::Scalar kSpectrumPlotLineColor(25, 25, 255);
static const cv::Scalar kSpectrumZeroLineColor(0, 255, 0);

// Returns the OpenCV matrix type to use, depending on the HSI data type.
int GetOpenCVMatrixType(const hsi::HSIDataType data_type) {
  switch (data_type) {
  case hsi::HSI_DATA_TYPE_BYTE:
    return CV_8SC1;
  case hsi::HSI_DATA_TYPE_INT16:
    return CV_16SC1;
  case hsi::HSI_DATA_TYPE_INT32:
    return CV_32SC1;
  case hsi::HSI_DATA_TYPE_DOUBLE:
    return CV_64FC1;
  case hsi::HSI_DATA_TYPE_UNSIGNED_INT16:
    return CV_16UC1;
  case hsi::HSI_DATA_TYPE_UNSIGNED_INT32:
    return CV_32SC1;  // TODO: OpenCV doesn't support uint32.
  case hsi::HSI_DATA_TYPE_UNSIGNED_LONG:
  case hsi::HSI_DATA_TYPE_UNSIGNED_INT64:
    return CV_32SC1;  // TODO: OpenCV doesn't support s/uint64.
  case hsi::HSI_DATA_TYPE_FLOAT:
  default:
    return CV_32FC1;
  }
}

// Set the pixel value of the matrix, interprented based on the type of data.
void SetBandImagePixel(
    const int row,
    const int col,
    const hsi::HSIDataValue value,
    const hsi::HSIDataType data_type,
    cv::Mat* band_image) {


  switch (data_type) {
  case hsi::HSI_DATA_TYPE_BYTE:
    band_image->at<char>(row, col) = value.value_as_byte;
  case hsi::HSI_DATA_TYPE_INT16:
    band_image->at<int16_t>(row, col) = value.value_as_int16;
    break;
  case hsi::HSI_DATA_TYPE_INT32:
    band_image->at<int32_t>(row, col) = value.value_as_int32;
    break;
  case hsi::HSI_DATA_TYPE_DOUBLE:
    band_image->at<double>(row, col) = value.value_as_double;
    break;
  case hsi::HSI_DATA_TYPE_UNSIGNED_INT16:
    band_image->at<uint16_t>(row, col) = value.value_as_uint16;
    break;
  case hsi::HSI_DATA_TYPE_UNSIGNED_INT32:
    band_image->at<uint32_t>(row, col) = value.value_as_uint16;
    break;
  case hsi::HSI_DATA_TYPE_UNSIGNED_INT64:
  case hsi::HSI_DATA_TYPE_UNSIGNED_LONG:
    // TODO: OpenCV doesn't support s/uint64.
    band_image->at<int32_t>(row, col) = value.value_as_int32;
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

// Generates a line plot for the spectrum consisting of the given vector of
// values. The plot is returned as a regular OpenCV image.
cv::Mat CreatePlot(const std::vector<double>& plot_values) {
  const auto min_max_value =
      std::minmax_element(plot_values.begin(), plot_values.end());
  const double min_raw_value = *min_max_value.first;
  const double max_raw_value = *min_max_value.second;
  const double range_from_zero =
      std::max(std::abs(min_raw_value), std::abs(max_raw_value));
  // How much space between each spectrum value along the X axis:
  const double space_between_points =
      static_cast<double>(kSpectrumPlotWidth) /
      static_cast<double>(plot_values.size());
  // The number of pixels between a single whole Y value:
  const double y_scale = kSpectrumPlotHeight / (2 * range_from_zero);
  // Create the image:
  cv::Mat plot_image =
      cv::Mat::zeros(kSpectrumPlotHeight, kSpectrumPlotWidth, CV_8UC3);
  plot_image.setTo(kSpectrumPlotBackgroundColor);
  // Draw line:
  cv::line(
      plot_image,
      cv::Point(0, y_scale * range_from_zero),
      cv::Point(kSpectrumPlotWidth, y_scale * range_from_zero),
      kSpectrumZeroLineColor,
      kSpectrumPlotLineThickness);
  // Draw the spectrum:
  for (int i = 0; i < plot_values.size() - 1; ++i) {
    const cv::Point point1(
        i * space_between_points,
        kSpectrumPlotHeight - (y_scale * (plot_values[i] + range_from_zero)));
    const cv::Point point2(
        (i + 1) * space_between_points,
        kSpectrumPlotHeight -
            (y_scale * (plot_values[i + 1] + range_from_zero)));
    cv::line(
        plot_image,
        point1,
        point2,
        kSpectrumPlotLineColor,
        kSpectrumPlotLineThickness);
  }
  return plot_image;
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
    const std::vector<double> spectrum =
        hsi_data->GetSpectrumAsDoubles(y_pos, x_pos);
    cv::Mat spectrum_plot = CreatePlot(spectrum);
    cv::namedWindow(kSpectrumWindowName);
    cv::imshow(kSpectrumWindowName, spectrum_plot);
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
  cv::destroyAllWindows();

  return 0;
}
