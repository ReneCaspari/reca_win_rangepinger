# Network Range Pinger

Network Range Pinger is a C++ tool designed to ping a specified range of IP addresses, retrieve their MAC addresses, and identify the manufacturer based on the MAC address. The tool utilizes multithreading to perform pings in parallel, resulting in faster execution. It also includes functionality to load configuration settings and manufacturer data from files.

## Features

- **Ping a Range of IP Addresses**: Specify a range of IP addresses to ping.
- **Retrieve MAC Addresses**: Get the MAC address for each IP address.
- **Identify Manufacturer**: Identify the manufacturer from the MAC address using an OUI file.
- **Multithreading**: Perform pings in parallel to speed up the process.
- **Configurable**: Load IP ranges and ping timeout settings from a configuration file.
- **Formatted Output**: Display the results in a neatly formatted table.

## Getting Started

### Prerequisites

- Windows operating system
- C++ compiler (e.g., MSVC)
- Winsock2 library

### Installation

1. **Clone the repository**:

    ```bash
    git clone https://github.com/ReneCaspari/reca_win_rangepinger.git
    cd reca_win_rangepinger
    ```

2. **Open the project**:

   - Open the project in your preferred C++ IDE (e.g., Visual Studio).

3. **Build the project**:

   - Ensure the project is set to build with the required libraries (iphlpapi.lib and ws2_32.lib).

### Configuration

1. **Configuration File (`config.txt`)**:
   - The configuration file should be located in the same directory as the executable.
   - Example content:
     ```yaml
     Range1
     192.168.0.1
     192.168.0.254
     Range2
     192.168.3.1
     192.168.3.254
     PingTimeout
     1000
     ```

2. **OUI File (`oui.txt`)**:
   - The OUI file should contain the MAC address prefixes and their corresponding manufacturers.
   - Example content:
     ```
     3a-fb-65 Apple
     00-1A-79 Dell
     ```

### Usage

1. **Run the executable**:
   - Execute the program. You will be prompted to enter the range name to ping.
   - The program will ping the specified range, retrieve the MAC addresses, identify the manufacturers, and display the results in a formatted table.

### Example Output

```plaintext
Enter the range name to ping: Range1
+------------------+----------------------+-----------------------+-------------------------+
| IP Address       | Time (ms)            | MAC Address           | Manufacturer            |
+------------------+----------------------+-----------------------+-------------------------+
| 192.168.0.1      | 5                    | 3a-fb-65-98-ed-30     | Apple                   |
| 192.168.0.2      | 10                   | 00-1A-79-4B-11-8D     | Dell                    |
+------------------+----------------------+-----------------------+-------------------------+
```

## Contributing

Contributions are welcome! Please fork this repository and submit pull requests for any enhancements or bug fixes.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- This project uses the Winsock2 library for network operations.
- Thanks to the authors of the `iphlpapi` and `ws2_32` libraries.
