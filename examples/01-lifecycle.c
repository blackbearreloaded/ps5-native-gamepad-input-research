#include "ps5_pad.h"

typedef struct pad_session {
    int32_t handle;
    bool owns_user_service;
} pad_session_t;

void pad_session_close(pad_session_t *session);

int pad_session_open(pad_session_t *session)
{
    int32_t user_id = -1;
    int32_t result;

    session->handle = -1;
    result = sceUserServiceInitialize(NULL);
    session->owns_user_service = result == 0;

    result = sceUserServiceGetInitialUser(&user_id);
    if (result < 0) {
        pad_session_close(session);
        return result;
    }
    result = scePadInit();
    if (result < 0) {
        pad_session_close(session);
        return result;
    }

    session->handle = scePadOpen(
        user_id, PS5_PAD_PORT_TYPE_STANDARD, 0, NULL);
    if (session->handle < 0) {
        result = session->handle;
        pad_session_close(session);
        return result;
    }
    return 0;
}

void pad_session_close(pad_session_t *session)
{
    if (session->handle >= 0) {
        ps5_pad_vibration_t stop = {0};
        (void)scePadSetVibration(session->handle, &stop);
        (void)scePadResetLightBar(session->handle);
        (void)scePadClose(session->handle);
        session->handle = -1;
    }
    if (session->owns_user_service) {
        (void)sceUserServiceTerminate();
        session->owns_user_service = false;
    }
}
