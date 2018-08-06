#include "../UnitTests.h"
#include "../TestX11Helper.h"

START_TEST(test_default_layouts){
    createContext(size);
    for(int i=0;i<getNumberOfWorkspaces();i++){
        assert(getSize(getWorkspaceByIndex(i)->layouts)==NUMBER_OF_DEFAULT_LAYOUTS);
    }
}END_TEST
START_TEST(test_layouts){
    INIT
    createContext(1);
    onStartup();
    connectToXserver();
    Monitor*m=getValue(getAllMonitors());
    int targetArea=m->viewWidth*m->viewHeight;
    if(_i==LAST_LAYOUT){
        setDefaultWindowMasks(FULLSCREEN_MASK);
        targetArea=m->width*m->height;
        _i=0;
    }
    getWorkspaceByIndex(0)->activeLayout=&DEFAULT_LAYOUTS[_i];
    START_MY_WM

    int area;
    xcb_get_geometry_reply_t*reply1,*reply2;
    for(int i=1;i<=size;i++){
        mapWindow(dc, createArbitraryWindow(dc));
        WAIT_FOR(getNumberOfTileableWindows(getActiveWindowStack())<i)

        do{
            assert(getNumberOfTileableWindows(getActiveWindowStack())==i);
            assert(getNumberOfTileableWindows(getActiveWindowStack())==getSize(getActiveWindowStack()));
            Node*p1=getActiveWindowStack();
            area=0;
            FOR_EACH(p1,
                reply1=xcb_get_geometry_reply(dc->dis, xcb_get_geometry(dc->dis, getIntValue(p1)), NULL);
                if(isWindowVisible(getValue(p1))){

                    area+=reply1->width*reply1->height;
                }
            )
            msleep(10);

        }while(area!=targetArea);
    }
    Node*p1=getActiveWindowStack();
    FOR_EACH(p1,
        if(isWindowVisible(getValue(p1))){
            reply1=xcb_get_geometry_reply(dc->dis, xcb_get_geometry(dc->dis, getIntValue(p1)), NULL);
            Node*p2=p1->next;
            if(p2==NULL)break;
            FOR_EACH(p2,
                if(isWindowVisible(getValue(p2))){
                    reply2=xcb_get_geometry_reply(dc->dis, xcb_get_geometry(dc->dis, getIntValue(p2)), NULL);
                    assert(intersects(&reply1->x, &reply2->x)==0);
                }
            )
        }
    )
    KILL_MY_WM
    END
}END_TEST

Suite *layoutSuite(void) {
    Suite*s = suite_create("Layouts");
    TCase* tc_core = tcase_create("Events");
    tcase_add_test(tc_core, test_layouts);
    suite_add_tcase(s, tc_core);
    tc_core = tcase_create("Layouts");
        tcase_add_test(tc_core, test_default_layouts);
        suite_add_tcase(s, tc_core);
    return s;
}
