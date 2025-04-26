#include <iostream>
#include <drivers/lx200_commands.hpp>
#include "common/alpaca_hub_serial.hpp"

asio::io_context _io_context;
char stop_on_char = '\n';

std::string send_command_to_mount(const std::string &cmd, asio::serial_port &_serial_port) {
 
   try {
    spdlog::trace("sending: {} to mount", cmd);
    char buf[512] = {0};
    _serial_port.write_some(asio::buffer(cmd));

    std::string rsp;

      // TODO: we may need to make the read timeout configurable here
      alpaca_hub_serial::blocking_reader reader(cmd, _serial_port, 250,
                                                _io_context);
      char c;
      while (reader.read_char(c)) {
        rsp += c;
        if (c == stop_on_char || stop_on_char == '\0' || c == '#') {
          break;
        }
      }
    

    std::cout << "mount returned: " << rsp << std::endl;
    return rsp;

  } catch (std::exception &ex) {
    throw alpaca_exception(
        alpaca_exception::DRIVER_ERROR,
        fmt::format("Problem sending command to mount: ", ex.what()));
  }

}

int main(int argc, char **argv) {
  std::cout << "Serial Console" << std::endl;
  
  std::string port_name = "/dev/ttyUSB0";


  asio::serial_port _serial_port(_io_context, port_name);
  _serial_port.set_option(asio::serial_port_base::baud_rate(9600));
  _serial_port.set_option(asio::serial_port_base::character_size(8));
  _serial_port.set_option(asio::serial_port_base::flow_control(
      asio::serial_port_base::flow_control::none));
  _serial_port.set_option(
      asio::serial_port_base::parity(asio::serial_port_base::parity::none));
  _serial_port.set_option(asio::serial_port_base::stop_bits(
      asio::serial_port_base::stop_bits::one));

  std::string command;
  while (std::getline(std::cin, command)) {
    std::string rsp = send_command_to_mount(command, _serial_port);
  }

  return 0;
}