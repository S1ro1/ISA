//
// Created by Matej Sirovatka on 14.10.2023.
//

#ifndef ISA_PROJECT_UTILS_H
#define ISA_PROJECT_UTILS_H

#include <iostream>

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