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

// The maximum exposure that the image can be displayed with.
constexpr int kMaxExposurePercent = 500;

// This struct is passed to callback functions to update the visualization
// window with appropriate band image and exposure level.
struct DisplayState {
  int current_exposure_percent = 100;
  int current_displayed_band = 0;
  std::vector<cv::Mat>* hsi_image_bands;
};

// Displays the appropriate band image given the current DisplayState. This is
// used when swapping bands to display or when changing the exposure.
void DisplayBandImage(const DisplayState* display_state) {
  const double exposure_ratio =
      static_cast<double>(display_state->current_exposure_percent) / 100.0;
  const int band_index = display_state->current_displayed_band;
  cv::Mat display_image =
      display_state->hsi_image_bands->at(band_index).clone();
  display_image *= exposure_ratio;
  cv::imshow(kMainWindowName, display_image);
}

// Callback function for the exposure selection slider, which will update the
// displayed band image with a new exposure ratio.
void ExposureSliderMovedCallback(int slider_value, void* display_state_ptr) {
  DisplayState* display_state =
      reinterpret_cast<DisplayState*>(display_state_ptr);
  display_state->current_exposure_percent = slider_value;
  DisplayBandImage(display_state);
}

// Callback function for the band selection slider, which will switch the
// display between different bands of the hyperspectral image.
void BandSliderMovedCallback(int slider_value, void* display_state_ptr) {
  DisplayState* display_state =
      reinterpret_cast<DisplayState*>(display_state_ptr);
  display_state->current_displayed_band = slider_value;
  DisplayBandImage(display_state);
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
  double min_value = 0;
  double max_value = 0;
  for (int band = 0; band < hsi_data.num_bands; ++band) {
    cv::Mat band_image(band_image_size, CV_64FC1);
    for (int row = 0; row < hsi_data.num_rows; ++row) {
      for (int col = 0; col < hsi_data.num_cols; ++col) {
        const double pixel_value = hsi_data.GetValueAsDouble(row, col, band);
        band_image.at<double>(row, col) = pixel_value;
        min_value = std::min(min_value, pixel_value);
        max_value = std::max(max_value, pixel_value);
      }
    }
    hsi_image_bands.push_back(band_image);
  }
  if (hsi_image_bands.size() == 0) {
    std::cerr << "No bands to visualize. Quitting." << std::endl;
    return 0;
  }
  // Normalize the band images between 0 and 1 for visualization purposes.
  const double range = max_value - min_value;
  for (int i = 0; i < hsi_image_bands.size(); ++i) {
    hsi_image_bands[i] -= min_value;
    hsi_image_bands[i] /= range;
  }

  // Visualize the images so that the user can view them per-channel.
  DisplayState display_state;
  display_state.hsi_image_bands = &hsi_image_bands;
  cv::namedWindow(kMainWindowName);
  cv::setMouseCallback(
      kMainWindowName,
      MouseEventCallback,
      const_cast<void*>(reinterpret_cast<const void*>(&hsi_data)));
  int exposure_slider_value = 100;
  cv::createTrackbar(
      "Exposure",
      kMainWindowName,
      &exposure_slider_value,
      kMaxExposurePercent,
      ExposureSliderMovedCallback,
      reinterpret_cast<void*>(&display_state));
  int band_slider_value = 0;
  cv::createTrackbar(
      "Band Selector",
      kMainWindowName,
      &band_slider_value,
      hsi_image_bands.size() - 1,
      BandSliderMovedCallback,
      reinterpret_cast<void*>(&display_state));
  cv::imshow(kMainWindowName, hsi_image_bands[0]);
  std::cout << "Visualizing data. Press any key to close window." << std::endl;
  cv::waitKey(0);
  cv::destroyAllWindows();

  return 0;
}
