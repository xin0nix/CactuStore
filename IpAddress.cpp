#include "IpAddress.hpp"

#include <arpa/inet.h>
#include <sstream>

namespace cactus {
IpAddress::IpAddress(std::array<uint8_t, 4> address, uint16_t port) noexcept
    : mAddress(address), mPort(port) {}

std::tuple<std::string, uint16_t> IpAddress::getFullHostAddress() const {
  std::stringstream addressStream;
  addressStream << static_cast<unsigned int>(mAddress[0]) << '.'
                << static_cast<unsigned int>(mAddress[1]) << '.'
                << static_cast<unsigned int>(mAddress[2]) << '.'
                << static_cast<unsigned int>(mAddress[3]);

  return {addressStream.str(), mPort};
}
} // namespace cactus