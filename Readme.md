# Modbus Gateway for Windows Service

## Description
This project provides a versatile gateway between Modbus TCP ↔ Modbus RTU over TCP using a Windows service for a console application.

## Technical description
1. Listens on a TCP Socket for new connections.
2. On incoming connection a new Thread will handles the incoming connection and connect to the target with its own socket.
3. In case of tcp mode: MBAP Header gets removed, CRC get's added, sent to target - response gets checked for CRC, then CRC removed and MBAP reconstructed and responded request.
3. In case of rtu mode: CRC get's removed, MBAP added, sent to target - response MBAP gets removed, CRC added and responded request.
4. In case of socket errors connection gets closed

Note: Due to the nature of the RTU protocoll over TCP, desyncs can appear in combination with timeouts, for this reason the connections get closed in many error scenarios.

## Features
- **Modbus TCP ↔ Modbus RTU Over TCP**: Realizes communication between Modbus TCP devices and Modbus RTU devices via a TCP connection. It can work also vice versa (Modbus RTU over TCP ↔ Modbus TCP) and allows for multiple masters to single target.
- **Windows Service Support**: Can run as a background service, ensuring it remains active even if the user logs off or closes the terminal.
- **Command Line Arguments for Configuration**: Allows setting up the service with different configurations from the command line.


## Usage
The program can be run directly from the console as well:

```sh
modbus_gateway.exe rtu 1503 192.168.1.100 503
```

This will start the service immediately in the foreground, without becoming a Windows service.

## Arguments

1. **MODE tcp|rtu**: (Default `tcp`) Defines the listener protocol (either `tcp` or `rtu`).
    `tcp`: listens for tcp, it will forward data as rtu over tcp.
    `rtu`: listens for rtu, it will forward data as tcp.

2. **LISTEN_PORT**: (Default `1502`) TCP Port to listen for incoming connections

3. **TARGET_HOST**: (Default `127.0.0.1`) Host-name/IP-adress to forward data 

4. **TARGET_PORT**: (Default `502`) Host port to forward data

## Examples

example: modbus_gateway
example: modbus_gateway tcp 1502 127.0.0.1 502
both examples are providing a tcp to rtu over tcp gateway.

For testing scenarios you can also couple tcp and rtu gateways:
modbus_slave_simulator (ex. pyModSlave) on port 502
modbus_gateway rtu 1503 127.0.0.1 502
modbus_gateway tcp 1502 127.0.0.1 1503
modbus_master_tester (ex. qMopMaster) to 127.0.0.1 1502
result:
modbus_master_tester -> modbus_gateway tcp -> modbus_gateway rtu -> modbus_slave_simulator


## Windows Service Installation
The provided `win-service-install.cmd` script helps in installing the program as a Windows Service. Follow these steps:

1. **Run the Script as Administrator**:
   Ensure that you run the `win-service-install.cmd` script with administrative privileges. This is necessary to create and manage Windows services. In case of missing privileges a new batch with requesting privileges will open automatically.

2. **Configure Arguments (Optional)**:
   You can specify additional arguments when running the script to customize the service name, display name, listen port, target host, and target port. For example:
   ```sh
   win-service-install.cmd rtu 1503 192.168.1.100 503 MyService "My Modbus Gateway Service"
   ```
   If no arguments are provided, default values will be used.

3. **Install the Service**:
   The script will guide you through the installation process and create a Windows service using the provided parameters. Once installed, the service can be managed via the Services Management Console or command line tools like `sc`.

## Example
To install a service that listens on port 1503, targets Modbus RTU at IP address `192.168.1.100` and port `503`, with a custom name and display name:

```sh
win-service-install.cmd rtu 1503 192.168.1.100 503 MyService "My Modbus Gateway Service"
```

This will create a Windows service that can be started, stopped, and managed using the Services Management Console.