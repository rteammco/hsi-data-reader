// Provides the HSIDataReader class which can read binary ENVI hyperspectral
// image data. Use HSIDataOptions to set the data properties as needed, and use
// HSIDataReader to read the desired range of the data. Loaded data is stored
// in the HSIData struct.

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
  // The size of the data.
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
  // The ordering of the data depends on the interleave format used.
  //
  // TODO: Implement this.
  float GetValue(const int row, const int col, const int band) const {
    const int index = 0;
    return data[index];
  }

  // The raw data.
  std::vector<float> data;
};

// The HSIDataReader is responsible for loading the data and storing it in
// memory.
class HSIDataReader {
 public:
  HSIDataReader(const HSIDataOptions& data_options);

  // Read the data in the specified range. The range must be valid, within the
  // specified HSIDataOptions data size. Returns true on success.
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

#endif  // HSI_DATA_READER_H_
