#ifndef IMAGE_BYTES_H
#define IMAGE_BYTES_H

#include <cstdint>
#include <ostream>
#include <spdlog/spdlog.h>
#include <vector>

enum image_array_element_types {
  UNKNOWN = 0, // 0 to 3 are values already used in the Alpaca
  INT16 = 1,
  INT32 = 2,
  DOUBLE = 3,
  SINGLE = 4, // 4 to 9 are an extension to include other numeric types
  UINT64 = 5,
  BYTE = 6,
  INT64 = 7,
  UINT16 = 8,
  UINT32 = 9
};

template <typename T> struct image_bytes_t {
  int metadata_version = 1;
  int error_number = 0;
  uint32_t client_transaction_number;
  uint32_t server_transaction_number;
  int data_start = 44;
  int image_element_type = 2;
  int transmission_element_type;
  int rank = 2;
  int dimension1;
  int dimension2;
  int dimension3 = 0;
  std::vector<T> image_data_1d;

  std::ostream &serialize(std::ostream &os) const {
    char null = '\0';
    os.write((char *)&metadata_version, sizeof(metadata_version));
    os.write((char *)&error_number, sizeof(error_number));
    os.write((char *)&client_transaction_number,
             sizeof(client_transaction_number));
    os.write((char *)&server_transaction_number,
             sizeof(server_transaction_number));
    os.write((char *)&data_start, sizeof(data_start));
    os.write((char *)&image_element_type, sizeof(client_transaction_number));
    os.write((char *)&transmission_element_type,
             sizeof(transmission_element_type));
    os.write((char *)&rank, sizeof(rank));
    os.write((char *)&dimension1, sizeof(dimension1));
    os.write((char *)&dimension2, sizeof(dimension2));
    os.write((char *)&dimension3, sizeof(dimension3));

    os.write(reinterpret_cast<char const *>(image_data_1d.data()),
             image_data_1d.size() * sizeof(T));

    return os;
  }
};

#endif
