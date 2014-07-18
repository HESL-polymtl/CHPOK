#include <stdio.h>
#include <string.h>
#include <arinc653/buffer.h>
#include <arinc653/partition.h>
#include <arinc653/time.h>
#include <arinc653/queueing.h>

QUEUING_PORT_ID_TYPE QP1, QP2;

static void check_ret(RETURN_CODE_TYPE ret)
{
    if (ret != NO_ERROR) {
        printf("error: %d\n", (int) ret);
    }
}

static void first_process(void)
{
    RETURN_CODE_TYPE ret;

    while (1) {
        char buffer[64];
        MESSAGE_SIZE_TYPE len;

        SEND_QUEUING_MESSAGE(QP1, "LOL", 4, INFINITE_TIME_VALUE, &ret);
        check_ret(ret);


        RECEIVE_QUEUING_MESSAGE(QP2, INFINITE_TIME_VALUE, buffer, &len, &ret);
        check_ret(ret);

        printf("received: %s\n", buffer);
        
        TIMED_WAIT(10LL * 1000 * 1000 * 1000, &ret);
    }
}

int main(void)
{
    RETURN_CODE_TYPE ret;
    BUFFER_ID_TYPE id;
    PROCESS_ID_TYPE pid;
    PROCESS_ATTRIBUTE_TYPE process_attrs = {
        .PERIOD = INFINITE_TIME_VALUE,
        .TIME_CAPACITY = INFINITE_TIME_VALUE,
        .STACK_SIZE = 8096, // the only accepted stack size!
        .BASE_PRIORITY = MIN_PRIORITY_VALUE,
        .DEADLINE = SOFT,
    };

    // create process 1
    process_attrs.ENTRY_POINT = first_process;
    strncpy(process_attrs.NAME, "process 1", sizeof(PROCESS_NAME_TYPE));

    CREATE_PROCESS(&process_attrs, &pid, &ret);
    if (ret != NO_ERROR) {
        printf("couldn't create process 1: %d\n", (int) ret);
        return 1;
    }
    
    START(pid, &ret);
    if (ret != NO_ERROR) {
        printf("couldn't start process 1: %d\n", (int) ret);
        return 1;
    }

    // create ports
    CREATE_QUEUING_PORT("QP1", 64, 10, SOURCE, FIFO, &QP1, &ret);
    CREATE_QUEUING_PORT("QP2", 64, 10, DESTINATION, FIFO, &QP2, &ret);

    printf("going to NORMAL mode...\n");

    // transition to NORMAL operating mode
    // N.B. if everything is OK, this never returns
    SET_PARTITION_MODE(NORMAL, &ret);

    if (ret != NO_ERROR) {
        printf("couldn't transit to normal operating mode: %d\n", (int) ret);
    } 

    STOP_SELF();
    return 0;
}
