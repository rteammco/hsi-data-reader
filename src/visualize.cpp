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
#include "opencv2/imgproc/imgproc.hpp"

// Names of the windows to be displayed.
static const std::string kMainWindowName = "HSI Image Visualization";
static const std::string kSpectrumWindowName = "Pixel Spectrum";

// Spectrum plot window properties.
constexpr int kSpectrumPlotHeight = 250;
constexpr int kSpectrumPlotWidth = 600;
constexpr int kSpectrumPlotLineThickness = 1;
static const cv::Scalar kSpectrumPlotBackgroundColor(255, 255, 255);
static const cv::Scalar kSpectrumPlotLineColor(25, 25, 255);

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

// Generates a line plot for the spectrum consisting of the given vector of
// values. The plot is returned as a regular OpenCV image.
cv::Mat CreatePlot(const std::vector<float>& plot_values) {
  const auto min_max_value =
      std::minmax_element(plot_values.begin(), plot_values.end());
  const float min_value = *min_max_value.first;
  const float max_value = *min_max_value.second;
  // How much space between each spectrum value along the X axis:
  const double space_between_points =
      static_cast<double>(kSpectrumPlotWidth) /
      static_cast<double>(plot_values.size());
  // The number of pixels between a single whole Y value:
  const double y_scale = kSpectrumPlotHeight / (max_value - min_value);
  // Create the image:
  cv::Mat plot_image =
      cv::Mat::zeros(kSpectrumPlotHeight, kSpectrumPlotWidth, CV_8UC3);
  plot_image.setTo(kSpectrumPlotBackgroundColor);
  for (int i = 0; i < plot_values.size() - 1; ++i) {
    const cv::Point point1(
        i * space_between_points,
        kSpectrumPlotHeight - (y_scale * plot_values[i]));
    const cv::Point point2(
        (i + 1) * space_between_points,
        kSpectrumPlotHeight - (y_scale * plot_values[i + 1]));
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
    const std::vector<hsi::HSIDataValue> spectrum =
        hsi_data->GetSpectrum(y_pos, x_pos);
    std::vector<float> spectrum_floats;
    for (const hsi::HSIDataValue value : spectrum) {
      spectrum_floats.push_back(value.value_as_float);
    }
    cv::Mat spectrum_plot = CreatePlot(spectrum_floats);
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
