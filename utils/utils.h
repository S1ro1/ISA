//
// Created by Matej Sirovatka on 14.10.2023.
//

#ifndef ISA_PROJECT_UTILS_H
#define ISA_PROJECT_UTILS_H

enum class TFTPState {
  INIT,
  SENT_RRQ,
  RECEIVED_RRQ,
  SENT_WRQ,
  RECEIVED_WRQ,
  FINAL_ACK,
  ERROR
};

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
