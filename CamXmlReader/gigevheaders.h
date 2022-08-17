#ifndef GIGEVHEADERS_H
#define GIGEVHEADERS_H

#include <QObject>
#include <QtEndian>

/* Device XML related defines. */
#define DEVICE_MANIFEST_TABLE_ADDRESS   (0x0934)
#define DEVICE_FIRST_URL_ADDRESS        (0X0200)
#define DEVICE_SECOND_URL_ADDRESS       (0x0400)
#define DEVICE_URL_ADDRESS_REG_LENGTH   (512)

/* Default GVP Port. */
#define GVCP_DEFAULT_UDP_PORT (3956)

/* Hardcoded keycode for message validation. */
#define GVCP_CMD_HARDCODED_KEYCODE (0x42)

/* Command and acknowledge IDs */
#define GVCP_DISCOVERY_CMD (0x0002)
#define GVCP_DISCOVERY_ACK (0x0003)
#define GVCP_FORCEIP_CMD (0x0004)
#define GVCP_FORCEIP_ACK (0x0005)
#define GVCP_PACKETRESEND_CMD (0x0040)
#define GVCP_PACKETRESEND_ACK (0x0041)
#define GVCP_READREG_CMD (0x0080)
#define GVCP_READREG_ACK (0x0081)
#define GVCP_WRITEREG_CMD (0x0082)
#define GVCP_WRITEREG_ACK (0x0083)
#define GVCP_READMEM_CMD (0x0084)
#define GVCP_READMEM_ACK (0x0085)
#define GVCP_WRITEMEM_CMD (0x0086)
#define GVCP_WRITEMEM_ACK (0x0087)
#define GVCP_PENDING_ACK (0x0089)
#define GVCP_EVENT_CMD (0x00C0)
#define GVCP_EVENT_ACK (0x00C1)
#define GVCP_EVENTDATA_CMD (0x00C2)
#define GVCP_EVENTDATA_ACK (0x00C3)
#define GVCP_ACTION_CMD (0x0100)
#define GVCP_ACTION_ACK (0x0101)

/* General program defines. */
#define BIT(x) (1 << x)
#define MAX_ACK_FETCH_RETRY_COUNT (3)
#define GVCP_MAX_PAYLOAD_LENGTH (0x218)

/* Command header flag bit fields. */
#define GVCP_CMD_FLAG_ACKNOWLEDGE BIT(7)
#define GVCP_CMD_FLAG_DISCOVERY_BROADCAST_ACK BIT(3)

typedef enum  {
    DEVICE,
    WEB,
    LOCAL

} enum_xml_location;

/* Generic command header. */
typedef struct __attribute__ ((__packed__))
{
    quint8      key_code;
    quint8      flag;
    quint16_be  be_command;
    quint16_be  be_length;
    quint16_be  be_req_id;

} str_gvcp_cmd_hdr;

/* Generic acknowledgement header. */
typedef struct __attribute__ ((__packed__))
{
    quint16_be  be_status;
    quint16_be  be_acknowledge;
    quint16_be  be_length;
    quint16_be  be_ack_id;

} str_gvcp_ack_hdr;

/* Read memory command header. */
typedef struct __attribute__ ((__packed__))
{
    quint32_be  address;
    quint16_be  reserved;
    quint16_be  count;

} str_gvcp_cmd_read_mem_hdr;

/* Write register command header. */
typedef struct __attribute__ ((__packed__))
{
    quint32_be  register_address;
    quint32_be  register_data;

} str_gvcp_cmd_write_reg_hdr;

/* Read register command header. */
typedef struct __attribute__ ((__packed__))
{
    quint32_be  register_address;

} str_gvcp_cmd_read_reg_hdr;

/* Discovery acknowledgement header. */
typedef struct __attribute__ ((__packed__))
{
    quint16_be          spec_version_major;
    quint16_be          spec_version_minor;
    quint32_be          device_mode;
    quint16_be          reserved0;
    quint16_be          device_mac_addr_high;
    quint32_be          device_mac_addr_low;
    quint32_be          ip_config_options;
    quint32_be          ip_config_current;
    quint32_be          reserved1[3];
    quint32_be          current_ip;
    quint32_be          reserved2[3];
    quint32_be          current_subnet_mask;
    quint32_be          reserved3[3];
    quint32_be          default_gateway;
    quint32_be          manufacturer_name[32/4];
    quint32_be          model_name[32/4];
    quint32_be          device_version[32/4];
    quint32_be          manufacturer_specific_info[48/4];
    quint32_be          serial_number[16/4];
    quint32_be          user_defined_name[16/4];

} str_gvcp_ack_discovery_hdr;

/* Discovery acknowledgement header. */
typedef struct __attribute__ ((__packed__))
{
    quint32_be  address;
    QByteArray  data;

} str_gvcp_ack_mem_read_hdr;

/* Non standard acknowledgement header. */
typedef struct {
    str_gvcp_ack_hdr    generic_ack_hdr;
    quint32             ack_hdr_type;
    void                *cmd_specific_ack_hdr;

} str_non_std_gvcp_ack_hdr;

/* Non standard command header. */
typedef struct {
    str_gvcp_cmd_hdr    generic_cmd_hdr;
    quint32             cmd_specific_data_length;
    void                *cmd_specific_data;

} str_non_std_gvcp_cmd_hdr;

#endif // GIGEVHEADERS_H
