

/*
SCUTEST(client_list) {
    xcb_ewmh_get_windows_reply_t reply;
    assert(xcb_ewmh_get_client_list_reply(ewmh, xcb_ewmh_get_client_list(ewmh, defaultScreenNumber), &reply, nullptr));
    auto size = reply.windows_len;
    assertEquals(size, getAllWindows()->size);
    for(uint32_t i = 0; i <  size; i++)
        assertEquals(reply.windows[i], getAllWindows()[i]->getID());
    xcb_ewmh_get_windows_reply_wipe(&reply);
    // we get focus out event before destroy event
    CRASH_ON_ERRORS = 0;
    setLogLevel(LOG_LEVEL_NONE);
    FOR_EACH(WindowInfo, winInfo, getAllWindows()) {
        assert(!destroyWindow(getID(winInfo)));
    }
    waitUntilIdle();
    assert(xcb_ewmh_get_client_list_reply(ewmh, xcb_ewmh_get_client_list(ewmh, defaultScreenNumber), &reply, nullptr));
    assertEquals(reply.windows_len, 0);
    xcb_ewmh_get_windows_reply_wipe(&reply);
}

SCUTEST_SET_ENV(onSimpleStartup, cleanupXServer);
SCUTEST(test_toggle_show_desktop) {
    WindowID win = mapWindow(createNormalWindow());
    WindowID desktop = mapWindow(createWindowWithType(ewmh->_NET_WM_WINDOW_TYPE_DESKTOP));

    WindowID stackingOrder[] = {desktop, win, desktop};
    flush();
    scan(root);
    lowerWindow(getWindowInfo(desktop));
    raiseWindow(getWindowInfo(win));
    assert(checkStackingOrder(stackingOrder, 2));
    setShowingDesktop(1);
    flush();
    assert(isShowingDesktop());
    assert(checkStackingOrder(stackingOrder + 1, 2));
    setShowingDesktop(0);
    flush();
    assert(!isShowingDesktop());
    assert(checkStackingOrder(stackingOrder, 2));
}

SCUTEST(test_client_show_desktop) {
    xcb_ewmh_request_change_showing_desktop(ewmh, defaultScreenNumber, 1);
    flush();
    WAIT_UNTIL_TRUE(isShowingDesktop());
    xcb_ewmh_request_change_showing_desktop(ewmh, defaultScreenNumber, 0);
    flush();
    WAIT_UNTIL_TRUE(!isShowingDesktop());
}
SCUTEST_ITER(test_client_ping, 2) {
    WindowID win = _i ? getPrivateWindow() : root;
    getAllWindows().add(new WindowInfo(win));
    WindowInfo* winInfo = getWindowInfo(win);
    assert(winInfo);
    if(_i) {
        const uint32_t data[] = { ewmh->_NET_WM_PING, 0, win};
        xcb_ewmh_send_client_message(ewmh->connection, win, root, ewmh->WM_PROTOCOLS, sizeof(data), data);
    }
    else
        xcb_ewmh_send_wm_ping(ewmh, getID(winInfo), 0);
    flush();
    WAIT_UNTIL_TRUE(getPingTimeStamp(winInfo));
    waitUntilIdle();
}
SCUTEST(test_client_ping_bad) {
    ATOMIC(xcb_ewmh_send_wm_ping(ewmh, destroyWindow(createNormalWindow()), 0));
    waitUntilIdle();
}
*/
