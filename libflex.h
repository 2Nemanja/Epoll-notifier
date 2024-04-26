#include <stdint.h>
typedef uint8_t flx_act;
typedef uint8_t flx_opt;

#define FLX_ACT_WATCH                                           0x00
#define FLX_ACT_QUIT                                            0x01
#define FLX_ACT_NOTIFY                                          0x02
#define FLX_ACT_REPLY                                           0x03
#define FLX_ACT_STATUS                                          0x04
#define FLX_ACT_UNSET                                           0xFF

#define FLX_WATCH_ADD                                           0x00
#define FLX_WATCH_REM                                           0x01

#define FLX_QUIT_USER                                           0x00
#define FLX_QUIT_ERROR                                          0x01

#define FLX_NOTIFY_CREATE                                       0x00
#define FLX_NOTIFY_DELETE                                       0x01
#define FLX_NOTIFY_ACCESS                                       0x02
#define FLX_NOTIFY_CLOSE                                        0x03
#define FLX_NOTIFY_MODIFY                                       0x04
#define FLX_NOTIFY_MOVE                                         0x05

#define FLX_REPLY_VALID                                         0x00
#define FLX_REPLY_BAD_SIZE                                      0x01
#define FLX_REPLY_BAD_ACTION                                    0x02
#define FLX_REPLY_BAD_OPTION                                    0x03
#define FLX_REPLY_BAD_PATH                                      0x04
#define FLX_REPLY_INVALID_DATA                                  0x05
#define FLX_REPLY_UNSET                                         0xFF

#define FLX_STATUS_SUCCESS                                      0x00
#define FLX_STATUS_ERR_INIT_NOTIFY                              0x00
#define FLX_STATUS_ADD_WATCH                                    0x00
#define FLX_STATUS_READ_INOTIFY                                 0x00

#define FLX_UNSET_UNSET                                         0xFF

#define FLX_PKT_MINIMUM_SIZE                                    3
#define FLX_PKT_MAXIMUM_SIZE                                    255

#define FLX_DLEN_WATCH                                          1
#define FLX_DLEN_QUIT                                           0
#define FLX_DLEN_NOTIFY                                         2
#define FLX_DLEN_REPLY                                          0
#define FLX_DLEN_STATUS                                         0
#define FLX_DLEN_UNSET                                          0

struct flex_msg {
    flx_act action;
    flx_opt option;
    uint8_t size;

    char ** data;
    int dataLen;
};

struct serialize_result { //potrebno da bismo mogli da returnujemo metadata nakon poziva fje serialize, kako bismo znali kako je prosla serializacija
    int size;
    flx_opt reply; // ako dodje do errora prilikom serilizacije, pozivamo ovo kako bi user zano da je doslo do errora
};