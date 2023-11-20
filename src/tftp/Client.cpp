// Matej Sirovatka, xsirov00

#include "Client.h"

volatile sig_atomic_t runningClient = 1;

void ClientSigintHandler([[maybe_unused]] int signum) {
  runningClient = 0;
}

void TFTP::Client::transmit() {
  if (mMode == Mode::DOWNLOAD) {
    requestRead();
  } else {
    requestWrite();
  }
}

TFTP::Client::Client(const ClientArgs &args, Options::map_t opts) : mOptions(std::move(opts)) {
  mSocketFd = socket(AF_INET, SOCK_DGRAM, 0);

  mClientAddress = {};

  mClientAddress.sin_family = AF_INET;
  mClientAddress.sin_port = htons(0);
  mClientAddress.sin_addr.s_addr = htonl(INADDR_ANY);

  socklen_t client_len = sizeof(mClientAddress);

  bind(mSocketFd, (struct sockaddr *) &mClientAddress, client_len);

  getsockname(mSocketFd, (struct sockaddr *) &mClientAddress, &client_len);

  // Set up the SIGINT handler
  struct sigaction sa;
  sa.sa_handler = ClientSigintHandler;

  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGINT, &sa, NULL);

  mClientPort = mClientAddress.sin_port;
  mTransmissionMode = "octet";

  mBlockNumber = 1;
  mDestFilePath = args.mDestFilePath;
  mState = State::INIT;
  mErrorPacket = std::nullopt;

  if (args.mSrcFilePath.has_value()) {
    mMode = Mode::DOWNLOAD;
    mSrcFilePath = args.mSrcFilePath.value();
  } else
    mMode = Mode::UPLOAD;

  memset(&mServerAddress, 0, sizeof(mServerAddress));

  mServerAddress.sin_family = AF_INET;
  mServerAddress.sin_port = htons(args.mPort);
  inet_pton(AF_INET, args.mAddress.c_str(), &mServerAddress.sin_addr);
}

void TFTP::Client::sendPacket(const Packet &packet) {
  std::vector<uint8_t> data = packet.serialize();
  sendto(mSocketFd, data.data(), data.size(), 0, (struct sockaddr *) &mServerAddress,
         sizeof(mServerAddress));
}

std::unique_ptr<TFTP::Packet> TFTP::Client::receivePacket() {
  std::vector<uint8_t> buffer(65535);
  sockaddr_in from_address = {};
  socklen_t from_length = sizeof(from_address);
  ssize_t received = recvfrom(mSocketFd, buffer.data(), buffer.size(), 0, (struct sockaddr *) &from_address,
                              &from_length);

  if (received <= 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      throw TFTP::TimeoutException();
    } else {
      throw TFTP::UndefinedException();
    }
  }

  buffer.resize(received);
  std::unique_ptr<Packet> packet;
  try {
    packet = Packet::deserialize(buffer);
  } catch (TFTP::PacketFormatException &e) {
    throw e;
  }

  std::cerr << packet->formatPacket(inet_ntoa(from_address.sin_addr), ntohs(from_address.sin_port),
                                    ntohs(mClientPort));

  if (mState == State::SENT_RRQ || mState == State::SENT_WRQ) {
    mServerAddress.sin_port = from_address.sin_port;
    mServerAddress.sin_addr.s_addr = from_address.sin_addr.s_addr;
    mState = State::DATA_TRANSFER;
  }

  if (mServerAddress.sin_port != from_address.sin_port || mServerAddress.sin_addr.s_addr != from_address.sin_addr.s_addr) {
    throw TFTP::InvalidTIDException();
  }

  return packet;
}

void TFTP::Client::requestRead() {
  std::unique_ptr<IOutputWrapper> outputFile;
  if (mTransmissionMode == "octet") {
    outputFile = std::make_unique<Octet::OutputFile>(mDestFilePath);
  } else {
    outputFile = std::make_unique<NetAscii::OutputFile>(mDestFilePath);
  }

  mState = State::SENT_RRQ;
  Options::map_t opts = {};
  mLastPacket = std::make_unique<RRQPacket>(mSrcFilePath, mTransmissionMode, opts);

  while (mState != State::ERROR && mState != State::FINAL_ACK && runningClient) {

    auto packet = exchangePackets(*mLastPacket, true);
    if (!packet) {
      break;
    }
    auto data_packet = dynamic_cast<DataPacket *>(packet.get());
    auto error_packet = dynamic_cast<ErrorPacket *>(packet.get());
    if (error_packet) {
      mState = State::ERROR;
      break;
    }

    if (!data_packet) {
      mState = State::ERROR;
      mErrorPacket = std::optional(ErrorPacket{4, "Illegal TFTP operation"});
      break;
    } else if (data_packet->getBlockNumber() == mBlockNumber) {
      outputFile->write(data_packet->getData());
      if (data_packet->getData().size() < Options::get("blksize", mOptions)) {
        mState = State::FINAL_ACK;
      }
      // Increment block number only after it is valid packet
      mBlockNumber++;
      mLastPacket = std::make_unique<ACKPacket>(mBlockNumber - 1);
    } else if (data_packet->getBlockNumber() > mBlockNumber) {
      mState = State::ERROR;
      mErrorPacket = std::optional(ErrorPacket{4, "Illegal TFTP operation"});
      break;
    } else if (data_packet->getBlockNumber() < mBlockNumber) {
      mLastPacket = std::make_unique<ACKPacket>(mBlockNumber - 1);
    }
  }

  if (mState == State::FINAL_ACK) {
    auto ack_packet = ACKPacket(mBlockNumber - 1);
    sendPacket(ack_packet);
  } else if (mErrorPacket.has_value()) {
    sendPacket(*mErrorPacket);
  }
}

void TFTP::Client::requestWrite() {
  std::unique_ptr<IInputWrapper> inputFile;
  if (mTransmissionMode == "octet") {
    inputFile = std::make_unique<Octet::InputStdin>();
  } else {
    inputFile = std::make_unique<NetAscii::InputStdin>();
  }
  mState = State::SENT_WRQ;
  Options::map_t opts = {};
  mLastPacket = std::make_unique<WRQPacket>(mDestFilePath, mTransmissionMode, opts);
  mBlockNumber = 0;

  bool toSend = true;

  while (mState != State::ERROR && mState != State::FINAL_ACK && runningClient) {
    auto packet = exchangePackets(*mLastPacket, toSend);

    if (!packet) {
      break;
    }

    auto ack_packet = dynamic_cast<ACKPacket *>(packet.get());
    auto error_packet = dynamic_cast<ErrorPacket *>(packet.get());
    if (error_packet) {
      mState = State::ERROR;
      break;
    }

    if (!ack_packet) {
      mState = State::ERROR;
      mErrorPacket = std::optional(ErrorPacket{4, "Illegal TFTP operation"});
      break;
    } else if (ack_packet->getBlockNumber() == mBlockNumber) {

      auto sent_packet = dynamic_cast<DataPacket *>(mLastPacket.get());
      if (sent_packet && sent_packet->getData().size() < Options::get("blksize", mOptions)) {
        mState = State::FINAL_ACK;
        break;
      }

      mBlockNumber++;
      toSend = true;
      std::vector<uint8_t> data(Options::get("blksize", mOptions));
      inputFile->read(reinterpret_cast<char *>(data.data()), data.size());
      data.resize(std::cin.gcount());
      mLastPacket = std::make_unique<DataPacket>(mBlockNumber, data);
    } else if (ack_packet->getBlockNumber() > mBlockNumber) {
      mState = State::ERROR;
      mErrorPacket = std::optional(ErrorPacket{4, "Illegal TFTP operation"});
      break;
    } else if (ack_packet->getBlockNumber() < mBlockNumber) {
      toSend = false;
    }
  }

  if (mErrorPacket.has_value()) {
    sendPacket(*mErrorPacket);
  }
}

std::unique_ptr<TFTP::Packet> TFTP::Client::exchangePackets(const Packet &packet, bool send) {
  if (send) sendPacket(packet);
  try {
    return receivePacket();
  } catch (TFTP::TimeoutException &e) {
    mErrorPacket = std::optional(ErrorPacket{0, "Timeout"});
  } catch (TFTP::InvalidTIDException &e) {
    mErrorPacket = std::optional(ErrorPacket{5, "Unknown transfer ID"});
  } catch (TFTP::UndefinedException &e) {
    mErrorPacket = std::optional(ErrorPacket{0, "Undefined error"});
  } catch (TFTP::PacketFormatException &e) {
    mErrorPacket = std::optional(ErrorPacket{4, "Illegal TFTP operation"});
  }
  mState = State::ERROR;
  return nullptr;
}
