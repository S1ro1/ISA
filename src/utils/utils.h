// Matej Sirovatka, xsirov00

#ifndef ISA_PROJECT_UTILS_H
#define ISA_PROJECT_UTILS_H

#include <iostream>

/**
 * @brief Enum representing state of the TFTP protocol
 */
enum class Mode {
  DOWNLOAD,
  UPLOAD,
};

#ifdef DEBUG_LOG
#define LOG(x) std::cout << x << std::endl;
#else
#define LOG(x)
#endif

#endif//ISA_PROJECT_UTILS_H
