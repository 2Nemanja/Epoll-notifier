#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#include "libflex.h"

void serialize_result_factory(struct serialize_result *result) { // resetovanje result-a fje
    result->size = -1;
    result->reply = FLX_REPLY_UNSET;
}
// 
void serialize(uint8_t buffer[FLX_PKT_MAXIMUM_SIZE], struct flex_msg *msg,
                 struct serialize_result *result) {
    serialize_result_factory(result);

    
}