# Serial port over USB

```bash
ls -la /dev/cu.*
screen /dev/cu.usbmodem1301 115200
```

For exit: Ctrl-A + d

# Bluetooth Low Energy (BLE)

## GAP: Generic Access Profile
- **Roles**:
  - **Broadcaster + Observer**: Connectionless communication, mainly involves advertising and scanning.
  - **Peripheral + Central**: Connection-oriented communication, where Peripheral devices offer services and Central devices use these services.
- **State Machine**:
  - **Advertising**: Device broadcasts its availability to others.
  - **Scanning**: Device listens for advertisements from others.
  - **Initiating**: Central device initiates a connection with a Peripheral.
  - **Connecting**: Establishing a connection between devices.
  - **Connected**: Devices are connected and can communicate.
  - **Disconnecting**: Process of terminating a connection.
  - **Disconnected**: Devices are not connected.

## ATT: Attribute Protocol
- **Handles**: Unique identifiers for each attribute on a device.
- **UUIDs**: Universally Unique Identifiers used to define specific services and characteristics.
- **Values**: The data contained within an attribute.
- **Permissions**: Defines how an attribute can be used (read, write, etc.).
- **Methods**:
  - **Read**: Clients read attribute values from a server.
  - **Write**: Clients write new values to an attribute on a server.
  - **Notify**: Server notifies clients about attribute value changes without client request.
  - **Indicate**: Similar to Notify, but requires client acknowledgment.

## GATT: Generic Attribute Profile
- **Structure**:
  - **Services**: Collections of characteristics and relationships to other services that encapsulate the behavior of part of a device.
  - **Characteristics**: Data points used for data transfer, consisting of a value and optionally some descriptors.
- **Descriptors**: Optional attributes describing a characteristic value.
- **Operations**:
  - **Discovery**: Clients find out about services and characteristics of a server.
  - **Read/Write Operations**: For reading from and writing to characteristic values.
  - **Notifications and Indications**: For server-initiated updates to a characteristic value.
- **Profiles**: Predefined sets of services and characteristics that specify how to use specific device capabilities or applications.


# BTStack generate random address

In development is useful because many devices cache GATT services and characteristics based on the MAC address of the device. If you change the MAC address, you can force the device to re-discover the services and characteristics.

```c
uint8_t btMac[6] = { 0x43, 0x43, 0xA2, 0x01, 0x02, 0x03 };
gap_random_address_set(btMac);
```