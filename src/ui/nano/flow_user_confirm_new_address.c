#include "os.h"
#include "ux.h"
#include "glyphs.h"

#include "ui_common.h"
#include "flow_user_confirm.h"
#include "flow_user_confirm_new_address.h"
#include "abstraction.h"

extern flowdata_t flow_data;

// gcc doesn't know this and ledger's SDK cannot be compiled with Werror!
//#pragma GCC diagnostic error "-Werror"
#pragma GCC diagnostic error "-Wpedantic"
#pragma GCC diagnostic error "-Wall"
#pragma GCC diagnostic error "-Wextra"

static void cb_address_preinit();
static void cb_bip32_preinit();

static void cb_accept();

// clang-format off
#ifdef TARGET_NANOS    
UX_STEP_NOCB_INIT(
    ux_step_new_address,
    bn_paging,
    cb_address_preinit(),
    {
        "Receive Address", (const char*) flow_data.scratch[0]
    }
);
#else
UX_STEP_NOCB_INIT(
    ux_step_new_address,
    bn_paging,
    cb_address_preinit(),
    {
        "Address", (const char*) flow_data.scratch[0]
    }
);
#endif

#ifdef TARGET_NANOS    
UX_STEP_NOCB_INIT(
    ux_step_new_remainder,
    bn_paging,
    cb_address_preinit(),
    {
        "Remainder", (const char*) flow_data.scratch[0]
    }
);
#else
UX_STEP_NOCB_INIT(
    ux_step_new_remainder,
    bn_paging,
    cb_address_preinit(),
    {
        // in paging mode, "New Remainder" doesn't fit without 
        // wrapping in the next line
        "New Remainder", (const char*) flow_data.scratch[0]
    }
);
#endif

#ifdef TARGET_NANOS    
UX_STEP_NOCB_INIT(
    ux_step_na_bip32,
    bn_paging,
    cb_bip32_preinit(),
    {
        "BIP32 Path", (const char*) flow_data.scratch[0]
    }
);
#else
UX_STEP_NOCB_INIT(
    ux_step_na_bip32,
    bn,
    cb_bip32_preinit(),
    {
        "BIP32 Path", (const char*) flow_data.scratch[0]
    }
);
#endif

UX_STEP_CB(
    ux_step_ok,
    pb,
    cb_accept(),
    {
        &C_x_icon_check,
        "Ok"
    }
);

UX_FLOW(
    ux_flow_new_address,
    &ux_step_new_address,
    &ux_step_na_bip32,
    &ux_step_ok,
    FLOW_LOOP
);

UX_FLOW(
    ux_flow_new_remainder,
    &ux_step_new_remainder,
    &ux_step_na_bip32,
    &ux_step_ok,
    FLOW_LOOP
);

// clang-format on

static void cb_address_preinit()
{
    // clear buffer
    memset(flow_data.scratch[0], 0, sizeof(flow_data.scratch[0]));
    memset(flow_data.scratch[1], 0, sizeof(flow_data.scratch[1]));

    // generate bech32 address including the address_type
    // we only have a single address in the buffer starting at index 0
    address_encode_bech32(flow_data.api->data.buffer, flow_data.scratch[1],
                          sizeof(flow_data.scratch[1]));

    // insert line-breaks
    MUST_THROW(string_insert_chars_each(
        flow_data.scratch[1], sizeof(flow_data.scratch[1]),
        flow_data.scratch[0], sizeof(flow_data.scratch[0]), 16, 3, '\n'));

#ifdef TARGET_NANOS
    // NOP - paging of nanos is fine
#else
    memcpy(flow_data.scratch[1], flow_data.scratch[0],
           sizeof(flow_data.scratch[1]));
    // insert another line-break (2 lines per page)
    MUST_THROW(string_insert_chars_each(
        flow_data.scratch[1], sizeof(flow_data.scratch[1]),
        flow_data.scratch[0], sizeof(flow_data.scratch[0]), 33, 1, '\n'));
#endif
}

static void cb_bip32_preinit()
{
    // clear buffer
    memset(flow_data.scratch[0], 0, sizeof(flow_data.scratch[0]));

    format_bip32_with_line_breaks(flow_data.api->bip32_path,
                                  flow_data.scratch[0],
                                  sizeof(flow_data.scratch[0]));
}

static void cb_accept()
{
    if (flow_data.accept_cb) {
        flow_data.accept_cb();
    }
    flow_stop();
}

void flow_start_new_address(const API_CTX *api, accept_cb_t accept_cb,
                            timeout_cb_t timeout_cb)
{
    flow_start_user_confirm(api, accept_cb, 0, timeout_cb);

    if (flow_data.api->bip32_path[BIP32_CHANGE_INDEX] & 0x1) {
        ux_flow_init(0, ux_flow_new_remainder, &ux_step_new_remainder);
    }
    else {
        ux_flow_init(0, ux_flow_new_address, &ux_step_new_address);
    }
}
