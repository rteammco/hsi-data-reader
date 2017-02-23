#ifndef HSI_DATA_READER_H_
#define HSI_DATA_READER_H_

#include <string>

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

class HSIDataReader {
 public:
  HSIDataReader(const HSIDataOptions& data_options)
      : data_options_(data_options) {}

  // Read the data in the specified range. The range must be valid, within the
  // specified HSIDataOptions data size.
  void ReadData(
      const int start_row,
      const int end_row,
      const int start_col,
      const int end_col,
      const int start_band,
      const int end_band);

 private:
  const HSIDataOptions data_options_;
};

}

#endif  // HSI_DATA_READER_H_
