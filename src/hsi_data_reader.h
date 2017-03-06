// Provides the HSIDataReader class which can read binary ENVI hyperspectral
// image data. Use HSIDataOptions to set the data properties as needed, and use
// HSIDataReader to read the desired range of the data. Loaded data is stored
// in the HSIData struct.

#ifndef SRC_HSI_DATA_READER_H_
#define SRC_HSI_DATA_READER_H_

#include <string>
#include <vector>

namespace hsi {

// Interleave format: BSQ, BIP, or BIL. The data files are a stream of bytes,
// and the values in the data are stored in one of the interleave orderings.
// TODO: implement support for BIP and BIL.
enum HSIDataInterleaveFormat {
  // BSQ (band sequential) format is organized in order of bands(rows(cols)).
  // For example, for a file with 2 bands, 2 rows, and 2 columns, the order
  // would be as follows:
  //   b0,r0,c0
  //   b0,r0,c1
  //   b0,r1,c0
  //   b0,r1,c1
  //   b1,r0,c0
  //   b1,r0,c1
  //   b1,r1,c0
  //   b1,r1,c1
  HSI_INTERLEAVE_BSQ,

//  HSI_INTERLEAVE_BIP,
//  HSI_INTERLEAVE_BIL
};

// The precision/type of the data.
// TODO: Add support for more than just float data type.
enum HSIDataType {
  HSI_DATA_TYPE_FLOAT
};

// Options that specify the location and format of the data. Needed to
// correctly parse the file.
struct HSIDataOptions {
  explicit HSIDataOptions(const std::string& hsi_file_path)
      : hsi_file_path(hsi_file_path) {}

  // Attempts to read the header information from an HSI header file. Returns
  // true if the read was successful and the information was loaded.
  bool ReadHeaderFromFile(const std::string& header_file_path);

  // Path to the binary hyperspectral data file.
  const std::string hsi_file_path;

  // The format and type of the data.
  HSIDataInterleaveFormat interleave_format = HSI_INTERLEAVE_BSQ;
  HSIDataType data_type = HSI_DATA_TYPE_FLOAT;
  bool big_endian = false;

  // Offset of the header (if the header is attached to the data).
  int header_offset = 0;

  // The size of the data. This is NOT the size of the chunk of data you want
  // to read, but rather of the entire data, even if you don't read everything.
  // These must all be non-zero.
  int num_data_rows = 0;
  int num_data_cols = 0;
  int num_data_bands = 0;
};

// This struct stores and provides access to hyperspectral data. All data is
// stored in a single vector, but can be indexed to access individual values.
struct HSIData {
  // The size of the data. This only counts the size of the data read in the
  // specified ranges (i.e. not necessarily the size of the entire data file).
  int num_rows = 0;
  int num_cols = 0;
  int num_bands = 0;

  HSIDataInterleaveFormat interleave_format = HSI_INTERLEAVE_BSQ;

  int NumDataPoints() const {
    return num_rows * num_cols * num_bands;
  }

  // Return the index value at the given index into the hyperspectral cube.
  // This treats the image as a cube where rows and cols define the image Y
  // (height) and X (width) axes, respectively, and the third is the spectral
  // dimension.
  //
  // All dimensions are zero-indexed. Indices are relative to the data in
  // memory, and not absolute positions in the entire data file. For example,
  // if data was read with start_row = 10, then row index 0 in this HSIData
  // would correspond to row 10 in the original data file.
  //
  // TODO: The ordering of the data depends on the interleave format used.
  // TODO: Check for valid index ranges and report error if it's invalid.
  float GetValue(const int row, const int col, const int band) const {
    const int num_pixels = num_rows * num_cols;
    const int band_index = num_pixels * band;
    const int pixel_index = row * num_cols + col;
    const int index = band_index + pixel_index;
    return data[index];
  }

  // Returns a vector containing the spectrum of the pixel at the given row
  // and col of the image.
  std::vector<float> GetSpectrum(const int row, const int col) const {
    std::vector<float> spectrum;
    spectrum.reserve(num_bands);
    for (int band = 0; band < num_bands; ++band) {
      spectrum.push_back(GetValue(row, col, band));
    }
    return spectrum;
  }

  // The raw data.
  std::vector<float> data;
};

// The HSIDataReader is responsible for loading the data and storing it in
// memory.
class HSIDataReader {
 public:
  explicit HSIDataReader(const HSIDataOptions& data_options);

  // Read the data in the specified range. The range must be valid, within the
  // specified HSIDataOptions data size. Returns true on success.
  //
  // Ranges are 0-indexed and end is non-inclusive. For example,
  //   start_row = 2,
  //   end_row = 7
  // will return rows (2, 3, 4, 5, 6) where the first row in the data is row 0.
  bool ReadData(
      const int start_row,
      const int end_row,
      const int start_col,
      const int end_col,
      const int start_band,
      const int end_band);

  // Writes the data currently stored in hsi_data_ in the order that it was
  // loaded in. Endian format is preserved from the original data. Returns true
  // on success.
  bool WriteData(const std::string& save_file_path) const;

  // Returns the HSIData struct containing any data loaded in from ReadData().
  const HSIData& GetData() const {
    return hsi_data_;
  }

 private:
  // Contains options and information about the data file which is necessary
  // for the ReadData() method to correctly read in the HSI data.
  const HSIDataOptions data_options_;

  // This will be true if the machine is big endian. This is required for
  // reading in the data correctly, which may not match the byte order of the
  // machine it's being read on.
  bool machine_big_endian_;

  // The data struct will get filled in in the ReadData() method.
  HSIData hsi_data_;
};

}  // namespace hsi

#endif  // SRC_HSI_DATA_READER_H_
