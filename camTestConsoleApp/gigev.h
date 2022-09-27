#ifndef GIGEV_H
#define GIGEV_H

/* NOTE: Taken from "https://github.com/rknall/wireshark/blob/master/epan/dissectors/packet-gvsp.c" */
#define GEV_STATUS_SUCCESS                             (0x0000)
#define GEV_STATUS_PACKET_RESEND                       (0x0100)
#define GEV_STATUS_NOT_IMPLEMENTED                     (0x8001)
#define GEV_STATUS_INVALID_PARAMETER                   (0x8002)
#define GEV_STATUS_INVALID_ADDRESS                     (0x8003)
#define GEV_STATUS_WRITE_PROTECT                       (0x8004)
#define GEV_STATUS_BAD_ALIGNMENT                       (0x8005)
#define GEV_STATUS_ACCESS_DENIED                       (0x8006)
#define GEV_STATUS_BUSY                                (0x8007)
#define GEV_STATUS_LOCAL_PROBLEM                       (0x8008) /* deprecated */
#define GEV_STATUS_MSG_MISMATCH                        (0x8009) /* deprecated */
#define GEV_STATUS_INVALID_PROTOCOL                    (0x800A) /* deprecated */
#define GEV_STATUS_NO_MSG                              (0x800B) /* deprecated */
#define GEV_STATUS_PACKET_UNAVAILABLE                  (0x800C)
#define GEV_STATUS_DATA_OVERRUN                        (0x800D)
#define GEV_STATUS_INVALID_HEADER                      (0x800E)
#define GEV_STATUS_WRONG_CONFIG                        (0x800F) /* deprecated */
#define GEV_STATUS_PACKET_NOT_YET_AVAILABLE            (0x8010)
#define GEV_STATUS_PACKET_AND_PREV_REMOVED_FROM_MEMORY (0x8011)
#define GEV_STATUS_PACKET_REMOVED_FROM_MEMORY          (0x8012)
#define GEV_STATUS_ERROR                               (0x8FFF)

#endif // GIGEV_H
