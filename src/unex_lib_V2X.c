#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include "error_code_user.h"
#include "poti_caster_service.h"
#include "eu_caster_service.h"
#include "itsmsg_codec.h"

/*
 *******************************************************************************
 * Macros
 *******************************************************************************
 */

#define APP_SIGALTSTACK_SIZE 8192
#define EMERGENCY (1 << 1)
#define REQUEST_FOR_FREE_CROSSING_AT_A_TRAFFIC_LIGHT (1 << 5)

/*
 *******************************************************************************
 * Data type definition
 *******************************************************************************
 */

typedef enum app_thread_type
{
    APP_THREAD_TX = 0,
    APP_THREAD_RX = 1
} app_thread_type_t;

typedef struct
{
    unsigned char *data;
    size_t len;
} data_t;

/*
 *******************************************************************************
 * Functions declaration
 *******************************************************************************
 */

static int send(data_t tx_data, int caster_id, bool is_secured);
static data_t recv(int caster_id);
static void set_tx_info(eu_caster_tx_info_t *tx_info, bool is_secured);
static int32_t app_set_thread_name_and_priority(pthread_t thread, app_thread_type_t type, char *p_name, int32_t priority);

/*
 *******************************************************************************
 * Functions
 *******************************************************************************
 */

void app_signal_handler(int sig_num)
{
    if (sig_num == SIGINT)
    {
        printf("SIGINT signal!\n");
    }
    if (sig_num == SIGTERM)
    {
        printf("SIGTERM signal!\n");
    }
}

char app_sigaltstack[8192];
int app_setup_signals(void)
{
    stack_t sigstack;
    struct sigaction sa;
    int ret = -1;

    sigstack.ss_sp = app_sigaltstack;
    sigstack.ss_size = SIGSTKSZ;
    sigstack.ss_flags = 0;
    if (sigaltstack(&sigstack, NULL) == -1)
    {
        perror("signalstack()");
        goto END;
    }

    sa.sa_handler = app_signal_handler;
    sa.sa_flags = SA_ONSTACK;
    if (sigaction(SIGINT, &sa, 0) != 0)
    {
        perror("sigaction()");
        goto END;
    }
    if (sigaction(SIGTERM, &sa, 0) != 0)
    {
        perror("sigaction()");
        goto END;
    }

    ret = 0;
END:
    return ret;
}

static data_t recv(int caster_id)
{
    data_t rx_data = {NULL, 0};

    int ret;
    ITSMsgConfig cfg;
    caster_thread_info_t caster_thread_info;
    caster_comm_config_t config;

    ret = app_setup_signals();
    if (!IS_SUCCESS(ret))
    {
        printf("Fail to app_setup_signals\n");
        return rx_data;
    }

    ret = itsmsg_init(&cfg);
    if (!IS_SUCCESS(ret))
    {
        printf("Fail to init ITS message\n");
        return rx_data;
    }

    config.ip = "192.168.1.3";
    config.caster_id = caster_id;

    eu_caster_init();
    eu_caster_thread_info_get(&caster_thread_info);
    ret = app_set_thread_name_and_priority(pthread_self(), APP_THREAD_RX, caster_thread_info.rx_thread_name, caster_thread_info.rx_thread_priority_low);

    eu_caster_rx_info_t rx_info = {0};
    uint8_t rx_buf[EU_CASTER_PKT_SIZE_MAX];
    size_t rx_data_len;
    caster_handler_t rx_handler = INVALID_CASTER_HANDLER;

    config.caster_comm_mode = CASTER_MODE_RX;
    ret = eu_caster_create(&rx_handler, &config);
    if (!IS_SUCCESS(ret))
    {
        printf("Cannot link to V2Xcast Service, V2Xcast Service create ret: [%d] %s!\n", ret, ERROR_MSG(ret));
        return rx_data;
    }

    printf("-----------------------\n");
    ret = eu_caster_rx_timeout(rx_handler, &rx_info, rx_buf, sizeof(rx_buf), &rx_data_len, 10);
    if (IS_SUCCESS(ret))
    {
        printf("Received %zu bytes!\n", rx_data_len);
        printf("RX data:\n");

        for (size_t i = 0; i < rx_data_len; i++)
        {
            printf("%02X ", rx_buf[i]);
        }
        printf("\n");
    }
    else
    {
        printf("Failed to receive data, err code is: [%d] %s\n", ret, ERROR_MSG(ret));
        eu_caster_release(rx_handler);
        eu_caster_deinit();
        return rx_data;
    }

    fflush(stdout);

    eu_caster_release(rx_handler);
    eu_caster_deinit();

    unsigned char *rx_buffer = (unsigned char *)malloc(rx_data_len);
    if (rx_buffer == NULL)
    {
        printf("Memory allocation failed!\n");
        return rx_data;
    }

    memcpy(rx_buffer, rx_buf, rx_data_len);

    rx_data.data = rx_buffer;
    rx_data.len = rx_data_len;

    return rx_data;
}

static int send(data_t tx_data, int caster_id, bool is_secured)
{
    int ret;
    ITSMsgConfig cfg;
    caster_thread_info_t caster_thread_info;
    caster_comm_config_t config;

    ret = app_setup_signals();
    if (!IS_SUCCESS(ret))
    {
        printf("Fail to app_setup_signals\n");
        return -1;
    }

    ret = itsmsg_init(&cfg);
    if (!IS_SUCCESS(ret))
    {
        printf("Fail to init ITS message\n");
        return -1;
    }

    config.ip = "192.168.1.3";
    config.caster_id = caster_id;

    eu_caster_init();

    eu_caster_thread_info_get(&caster_thread_info);
    ret = app_set_thread_name_and_priority(pthread_self(), APP_THREAD_TX, caster_thread_info.tx_thread_name, caster_thread_info.tx_thread_priority_low);

    poti_fix_data_t fix_data = {0};
    poti_gnss_info_t gnss_info = {0};
    eu_caster_tx_info_t tx_info = {0};
    caster_handler_t tx_handler = INVALID_CASTER_HANDLER;
    caster_handler_t poti_handler = INVALID_CASTER_HANDLER;

    config.caster_comm_mode = CASTER_MODE_TX;
    ret = eu_caster_create(&tx_handler, &config);
    if (!IS_SUCCESS(ret))
    {
        printf("Cannot link to V2Xcast Service, V2Xcast Service create ret: [%d] %s!\n", ret, ERROR_MSG(ret));
        printf("Please confirm network connection by ping the Unex device then upload a V2Xcast config to create a V2Xcast Service.\n");
        return -1;
    }

    config.caster_comm_mode = CASTER_MODE_POTI;
    ret = eu_caster_create(&poti_handler, &config);
    if (!IS_SUCCESS(ret))
    {
        printf("Fail to create POTI caster, ret:%d!\n", ret);
        return -1;
    }

    printf("-----------------------\n");

    ret = poti_caster_fix_data_get(poti_handler, &fix_data);

    if (ret != 0)
    {
        printf("Fail to receive GNSS fix data from POTI caster service, %d\n", ret);
        usleep(1000000);
    }
    else if (fix_data.mode < POTI_FIX_MODE_2D)
    {
        printf("GNSS not fix, fix mode: %d\n", fix_data.mode);

        ret = poti_caster_gnss_info_get(poti_handler, &gnss_info);
        if (IS_SUCCESS(ret))
        {
            printf("GNSS antenna status:%d, time sync status: %d\n", gnss_info.antenna_status, gnss_info.time_sync_status);
        }

        usleep(100000);
    }

    if ((isnan(fix_data.latitude) == 1) || (isnan(fix_data.longitude) == 1) || (isnan(fix_data.altitude) == 1) || (isnan(fix_data.horizontal_speed) == 1) || (isnan(fix_data.course_over_ground) == 1))
    {
        printf("GNSS fix data has NAN value, latitude: %f, longitude : %f, altitude : %f, horizontal speed : %f, course over ground : %f\n", fix_data.latitude, fix_data.longitude, fix_data.altitude, fix_data.horizontal_speed, fix_data.course_over_ground);
    }

    ret = poti_caster_gnss_info_get(poti_handler, &gnss_info);
    if (IS_SUCCESS(ret))
    {
        printf("Access layer time sync state: %d , unsync times: %d\n", gnss_info.acl_time_sync_state, gnss_info.acl_unsync_times);
    }

    if (tx_data.data != NULL)
    {
        printf("Encoded data:\n");
        set_tx_info(&tx_info, is_secured);

        for (size_t i = 0; i < tx_data.len; i++)
        {
            printf("%02X ", tx_data.data[i]);
        }
        printf("\n");

        ret = eu_caster_tx(tx_handler, &tx_info, tx_data.data, tx_data.len);
        if (IS_SUCCESS(ret))
        {
            printf("Transmitted %ld bytes!\n", tx_data.len);
            printf("Transmitted message\n");
        }
        else
        {
            printf("Failed to transmit data, err code is: [%d] %s\n", ret, ERROR_MSG(ret));
            printf("Failed to transmit message\n");
        }
    }
    fflush(stdout);

    eu_caster_release(tx_handler);
    eu_caster_release(poti_handler);
    return 0;
}

static void set_tx_info(eu_caster_tx_info_t *tx_info, bool is_secured)
{
    /* set data rate*/
    tx_info->data_rate_is_present = true;
    tx_info->data_rate = 12; /* 12 (500kbps) = 6 (Mbps) */

    if (is_secured)
    {
        /* set security*/
        tx_info->security_is_present = true;
        /*
         * Assign CAM service specific permissions according to the actual content in payload.
         * Please refer to ETSI TS 103 097, ETSI EN 302 637-2 for more details.
         * Please refer to Unex-APG-ETSI-GN-BTP for more information of build-in certificates
         */
        /* SSP Version control */
        tx_info->security.ssp[0] = 0x0;
        /* Service-specific parameter */
        tx_info->security.ssp[1] = EMERGENCY;                                    /* Emergency container */
        tx_info->security.ssp[2] = REQUEST_FOR_FREE_CROSSING_AT_A_TRAFFIC_LIGHT; /* EmergencyPriority */
        tx_info->security.ssp_len = 3;
    }

    return;
}

static int32_t app_set_thread_name_and_priority(pthread_t thread, app_thread_type_t type, char *p_name, int32_t priority)
{
    int32_t result;
    caster_thread_info_t limited_thread_config;

#ifdef __SET_PRIORITY__
    struct sched_param param;
#endif // __SET_PRIORITY__
    if (p_name == NULL)
    {
        return -1;
    }

    /* Check thread priority is in the limited range */
    eu_caster_thread_info_get(&limited_thread_config);

    if (APP_THREAD_TX == type)
    {
        /* Check the limited range for tx thread priority */
        if ((priority < limited_thread_config.tx_thread_priority_low) || (priority > limited_thread_config.tx_thread_priority_high))
        {
            /* Thread priority is out of range */
            printf("The tx thread priority is out of range (%d-%d): %d \n", limited_thread_config.tx_thread_priority_low, limited_thread_config.tx_thread_priority_high, priority);
            return -1;
        }
    }
    else if (APP_THREAD_RX == type)
    {
        /* Check the limited range for rx thread priority */
        if ((priority < limited_thread_config.rx_thread_priority_low) || (priority > limited_thread_config.rx_thread_priority_high))
        {
            /* Thread priority is out of range */
            printf("The rx thread priority is out of range (%d-%d): %d \n", limited_thread_config.rx_thread_priority_low, limited_thread_config.rx_thread_priority_high, priority);
            return -1;
        }
    }
    else
    {
        /* Target thread type is unknown */
        printf("The thread type is unknown: %d \n", type);
        return -1;
    }

    result = pthread_setname_np(thread, p_name);
    if (result != 0)
    {
        printf("Can't set thread name: %d (%s) \n", result, strerror(result));
        return -1;
    }

#ifdef __SET_PRIORITY__
    param.sched_priority = priority;
    result = pthread_setschedparam(thread, SCHED_FIFO, &param);
    if (result != 0)
    {
        printf("Can't set thread priority: %d (%s) \n", result, strerror(result));
        return -1;
    }
#endif // __SET_PRIORITY__
    return 0;
}

static PyObject *receive_function(PyObject *self, PyObject *args)
{
    (void)self;
    int caster_id;

    if (!PyArg_ParseTuple(args, "i", &caster_id))
        return NULL;

    data_t rx_data = recv(caster_id);
    return Py_BuildValue("y#", rx_data.data, rx_data.len);
}

static PyObject *send_function(PyObject *self, PyObject *args)
{
    (void)self;
    int sts, is_secured, caster_id;
    data_t tx_data;

    if (!PyArg_ParseTuple(args, "y#ii", &tx_data.data, &tx_data.len, &caster_id, &is_secured))
        return NULL;

    sts = send(tx_data, caster_id, (bool)is_secured);
    return Py_BuildValue("i", sts);
}

static PyMethodDef methods[] = {
    {"receive", receive_function, METH_VARARGS, "Receives message"},
    {"send", send_function, METH_VARARGS, "Sends message"},
    {NULL, NULL, 0, NULL}};

static struct PyModuleDef unex_lib_V2X = {
    PyModuleDef_HEAD_INIT,
    "unex_lib_V2X",
    "doc",
    -1,
    methods,
    NULL,
    NULL,
    NULL,
    NULL};

PyMODINIT_FUNC PyInit_unex_lib_V2X(void)
{
    return PyModule_Create(&unex_lib_V2X);
}
