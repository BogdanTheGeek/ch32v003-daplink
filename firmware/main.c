//------------------------------------------------------------------------------
//       Filename: main.c
//------------------------------------------------------------------------------
//       Bogdan Ionescu (c) 2025
//------------------------------------------------------------------------------
//       Purpose : Application entry point
//------------------------------------------------------------------------------
//       Notes : None
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Module includes
//------------------------------------------------------------------------------
#include <inttypes.h>
#include <stdbool.h>

#include "ch32fun.h"
#include "log.h"
#include "rv003usb.h"

#include "DAP_config.h"

#include "DAP.h"

//------------------------------------------------------------------------------
// Module constant defines
//------------------------------------------------------------------------------
#define TAG "main"

//------------------------------------------------------------------------------
// External variables
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// External functions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Module type definitions
//------------------------------------------------------------------------------

typedef struct
{
    int get;
    int set;
    int out;
    int in;
    int control;
    int other;
} Stats_t;

typedef struct
{
    uint32_t volatile head;
    uint32_t volatile tail;
    uint8_t items[DAP_PACKET_COUNT][DAP_PACKET_SIZE];
    bool volatile full;
} Ring_t;

//------------------------------------------------------------------------------
// Module static variables
//------------------------------------------------------------------------------
volatile uint32_t SysTick_ms = 0; // Externed yuck!!

static uint8_t scratch[DAP_PACKET_SIZE];

static Ring_t s_req = {0};
static Ring_t s_resp = {0};

static Stats_t s_stats = {0};

//------------------------------------------------------------------------------
// Module static function prototypes
//------------------------------------------------------------------------------
static void SysTick_Init(void);
static void WDT_Init(uint16_t reload_val, uint8_t prescaler);
static void WDT_Pet(void);
uint8_t usbd_hid_process(void);

//------------------------------------------------------------------------------
// Module externally exported functions
//------------------------------------------------------------------------------

/**
 * @brief  Application entry point
 * @param  None
 * @return None
 */
int main(void)
{
    SystemInit();

    SysTick_Init();

    funGpioInitAll();

    const bool debuggerAttached = !WaitForDebuggerToAttach(1000);
    if (debuggerAttached)
    {
        LOG_Init(eLOG_LEVEL_DEBUG, (uint32_t *)&SysTick_ms);
    }
    else
    {
        LOG_Init(eLOG_LEVEL_NONE, (uint32_t *)&SysTick_ms);
    }

    DAP_Setup();

    usb_setup();

    WDT_Init(0x0FFF, IWDG_Prescaler_128);

    uint32_t last_ms = SysTick_ms;

    while (1)
    {
        WDT_Pet();

        (void)usbd_hid_process();
        // VCOM_TransferData();

        if (SysTick_ms - last_ms >= 1000)
        {
            last_ms = SysTick_ms;
            LOGD(TAG, "get=%d set=%d out=%d in=%d control=%d other=%d",
                 s_stats.get,
                 s_stats.set,
                 s_stats.out,
                 s_stats.in,
                 s_stats.control,
                 s_stats.other);
        }
    }
}

//------------------------------------------------------------------------------
// Module static functions
//------------------------------------------------------------------------------

/**
 * @brief  Enable the SysTick module
 * @param  None
 * @return None
 */
static void SysTick_Init(void)
{
    // Disable default SysTick behavior
    SysTick->CTLR = 0;

    // Enable the SysTick IRQ
    NVIC_EnableIRQ(SysTicK_IRQn);

    // Set the tick interval to 1ms for normal op
    SysTick->CMP = SysTick->CNT + (FUNCONF_SYSTEM_CORE_CLOCK / 1000) - 1;

    // Start at zero
    SysTick_ms = 0;

    // Enable SysTick counter, IRQ, HCLK/1
    SysTick->CTLR = SYSTICK_CTLR_STE | SYSTICK_CTLR_STIE | SYSTICK_CTLR_STCLK;
}

/**
 * @brief  Initialize the watchdog timer
 * @param reload_val - the value to reload the counter with
 * @param prescaler - the prescaler to use
 * @return None
 */
static void WDT_Init(uint16_t reload_val, uint8_t prescaler)
{
    IWDG->CTLR = 0x5555;
    IWDG->PSCR = prescaler;

    IWDG->CTLR = 0x5555;
    IWDG->RLDR = reload_val & 0xfff;

    IWDG->CTLR = 0xCCCC;
}

/**
 * @brief  Pet the watchdog timer
 * @param  None
 * @return None
 */
static void WDT_Pet(void)
{
    IWDG->CTLR = 0xAAAA;
}

/**
 * @brief  SysTick interrupt handler
 * @param  None
 * @return None
 * @note   __attribute__((interrupt)) syntax is crucial!
 */
void SysTick_Handler(void) __attribute__((interrupt));
void SysTick_Handler(void)
{
    // Set the next interrupt to be in 1/1000th of a second
    SysTick->CMP += (FUNCONF_SYSTEM_CORE_CLOCK / 1000);

    // Clear IRQ
    SysTick->SR = 0;

    // Update counter
    SysTick_ms++;
}
/**
 * @brief  Process USB HID requests
 * @param  None
 * @return 1 if a request was processed, 0 otherwise
 */
uint8_t usbd_hid_process(void)
{
    uint32_t n;
    if ((s_req.tail != s_req.head) || s_req.full)
    {
        /* If response ring is full, don't process a new request yet.
         * We always enqueue responses now; this prevents overwriting. */
        if ((s_resp.head == s_resp.tail) && s_resp.full)
        {
            return 0;
        }

        LOGI(TAG, "Processing request 0x%x", (int)s_req.items[s_req.tail][0]);
        (void)DAP_ProcessCommand(s_req.items[s_req.tail], s_resp.items[s_resp.head]);

        n = s_req.tail + 1;
        if (n == DAP_PACKET_COUNT) n = 0;
        s_req.tail = n;

        if (s_req.tail == s_req.head) s_req.full = 0;

        n = s_resp.head + 1;
        if (n == DAP_PACKET_COUNT) n = 0;
        s_resp.head = n;

        if (s_resp.head == s_resp.tail) s_resp.full = 1;

        return 1;
    }

    return 0;
}

/**
 * @brief  Handle USB user in requests. Called when host requests IN data
 * @param  e - the endpoint
 * @param  scratchpad - the scratchpad buffer
 * @param  endp - the endpoint number
 * @param  tok - the token to send
 * @param  ist - the internal state
 * @return None
 * @note usb_hande_interrupt_in is OBLIGATED to call usb_send_data or usb_send_empty.
 */
void usb_handle_user_in_request(struct usb_endpoint *e, uint8_t *scratchpad, int endp, uint32_t tok, struct rv003usb_internal *ist)
{
    static uint32_t send_index = 0; // current send index

    // Make sure we only deal with control messages. Like get/set feature reports.
    if (endp == 1)
    {
        if ((s_resp.tail != s_resp.head) || s_resp.full)
        {
            uint8_t *const data = s_resp.items[s_resp.tail];
            usb_send_data(data + send_index, 8, 0, tok);

            send_index += 8;
            if (send_index >= DAP_PACKET_SIZE)
            {
                send_index = 0;
                ++s_stats.in;

                s_resp.tail++;
                if (s_resp.tail == DAP_PACKET_COUNT)
                    s_resp.tail = 0;

                if (s_resp.tail == s_resp.head)
                    s_resp.full = 0;
            }
        }
        else
        {
            usb_send_empty(tok);
        }
    }
    else
    {
        usb_send_empty(tok);
    }
}

/**
 * @brief  Handle USB user data. Called when we receive OUT data from host
 * @param      e - the endpoint
 * @param      current_endpoint - the current endpoint
 * @param[in]  data - the data
 * @param      len - the length
 * @param      ist - the internal state
 * @return None
 */
void usb_handle_user_data(struct usb_endpoint *e, int current_endpoint, uint8_t *data, int len, struct rv003usb_internal *ist)
{
    static int offset = 0;

    memcpy(scratch + offset, data, len);
    e->count++;
    offset += len;

    // if last packet
    if (offset >= DAP_PACKET_SIZE)
    {
        offset = 0;
        ++s_stats.out;

        if (scratch[0] == ID_DAP_TransferAbort)
        {
            DAP_TransferAbort = 1;
            return;
        }

        if ((s_req.head == s_req.tail) && s_req.full) // Request  Buffer Full
            return;

        memcpy(s_req.items[s_req.head], scratch, DAP_PACKET_SIZE);

        s_req.head++;
        if (s_req.head == DAP_PACKET_COUNT)
            s_req.head = 0;

        if (s_req.head == s_req.tail)
            s_req.full = 1;
    }
}

void usb_handle_hid_get_report_start(struct usb_endpoint *e, int reqLen, uint32_t lValueLSBIndexMSB)
{
    // You can check the lValueLSBIndexMSB word to decide what you want to do here
    // But, whatever you point this at will be returned back to the host PC where
    // it calls hid_get_feature_report.
    //
    // Please note, that on some systems, for this to work, your return length must
    // match the length defined in HID_REPORT_COUNT, in your HID report, in usb_config.h

    e->opaque = NULL;
    e->max_len = 0;

    ++s_stats.get;
}

void usb_handle_hid_set_report_start(struct usb_endpoint *e, int reqLen, uint32_t lValueLSBIndexMSB)
{
    // Here is where you get an alert when the host PC calls hid_send_feature_report.
    //
    // You can handle the appropriate message here.  Please note that in this
    // example, the data is chunked into groups-of-8-bytes.
    //
    // Note that you may need to make this match HID_REPORT_COUNT, in your HID
    // report, in usb_config.h

    // if (reqLen > sizeof(scratch)) reqLen = sizeof(scratch);
    e->max_len = DAP_PACKET_SIZE;
    ++s_stats.set;
}

/**
 * @brief  Handle USB control messages
 * @param  e - the endpoint
 * @param  s - the URB
 * @param  ist - the internal state
 * @return None
 */
void usb_handle_other_control_message(struct usb_endpoint *e, struct usb_urb *s, struct rv003usb_internal *ist)
{
    LogUEvent(SysTick->CNT, s->wRequestTypeLSBRequestMSB, s->lValueLSBIndexMSB, s->wLength);
    // e->opaque = (uint8_t *)1;
    ++s_stats.control;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
