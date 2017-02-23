#ifndef HSI_DATA_READER_H_
#define HSI_DATA_READER_H_

#include <string>
#include <vector>

namespace hsi {

// Interleave format: BSQ, BIP, or BIL.
// TODO: comment on what each of these means.
enum HSIDataInterleaveFormat {
  HSI_INTERLEAVE_BSQ,
  HSI_INTERLEAVE_BIP,
  HSI_INTERLEAVE_BIL
};

// The precision/type of the data.
// TODO: allow more than just float data type.
enum HSIDataType {
  HSI_DATA_TYPE_FLOAT
};

// Options that specify the location and format of the data. Needed to
// correctly parse the file.
struct HSIDataOptions {
  HSIDataOptions(const std::string& hsi_file_path)
      : hsi_file_path(hsi_file_path) {}

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
  // The size of the data.
  int num_rows = 0;
  int num_cols = 0;
  int num_bands = 0;

  HSIDataInterleaveFormat interleave_format = HSI_INTERLEAVE_BSQ;

  // Return the index value at the given index into the hyperspectral cube.
  // This treats the image as a cube where rows and cols define the image Y
  // (height) and X (width) axes, respectively, and the third is the spectral
  // dimension.
  //
  // The ordering of the data depends on the interleave format used.
  //
  // TODO: Implement this.
  float GetValue(const int row, const int col, const int band) {
    const int index = 0;
    return data[index];
  }

  // The raw data.
  std::vector<float> data;
};

class HSIDataReader {
 public:
  HSIDataReader(const HSIDataOptions& data_options)
      : data_options_(data_options) {}

  // Read the data in the specified range. The range must be valid, within the
  // specified HSIDataOptions data size. Returns true on success.
  bool ReadData(
      const int start_row,
      const int end_row,
      const int start_col,
      const int end_col,
      const int start_band,
      const int end_band);

  const HSIData& GetData() const {
    return hsi_data_;
  }

 private:
  const HSIDataOptions data_options_;

  HSIData hsi_data_;
};

}

#endif  // HSI_DATA_READER_H_
