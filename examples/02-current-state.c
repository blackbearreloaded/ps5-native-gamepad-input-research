#include "ps5_pad.h"

#include <string.h>

static void make_neutral(ps5_pad_data_t *state)
{
    memset(state, 0, sizeof(*state));
    state->left_stick.x = state->left_stick.y = 128;
    state->right_stick.x = state->right_stick.y = 128;
    state->orientation.w = 1.0f;
}

int read_current_state(int32_t handle, ps5_pad_data_t *state)
{
    int32_t result;

    make_neutral(state);
    result = scePadReadState(handle, state);
    if (result < 0)
        return result;
    if (!state->connected ||
        (state->buttons & PS5_PAD_BUTTON_INTERCEPTED) != 0)
        make_neutral(state);
    return 0;
}
