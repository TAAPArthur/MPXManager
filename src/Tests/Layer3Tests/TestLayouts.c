#include "../UnitTests.h"
#include "../TestX11Helper.h"

#include "../../layouts.h"
#include "../../wmfunctions.h"
#include "../../functions.h"
#include "../../monitors.h"
#include "../../state.h"
#include "../../devices.h"


START_TEST(test_layouts){
    Monitor*m=getValue(getAllMonitors());
    int targetArea=m->viewWidth*m->viewHeight;
    int layoutIndex=_i;
    if(layoutIndex==NUMBER_OF_DEFAULT_LAYOUTS){
        DEFAULT_WINDOW_MASKS=FULLSCREEN_MASK;
        targetArea=m->width*m->height;
        layoutIndex=0;
    }
    setActiveLayout(&DEFAULT_LAYOUTS[layoutIndex]);

    int size=5;
    int limit=size-1;
    DEFAULT_LAYOUTS[layoutIndex].args[LAYOUT_LIMIT_INDEX]=limit;
    tileWorkspace(getActiveWorkspaceIndex());

    assert(!isNotEmpty(getAllWindows()));
    START_MY_WM
    int area;
    xcb_get_geometry_reply_t*reply1,*reply2;
    for(int i=1;i<=size;i++){
        lock();
        createNormalWindow();
        flush();
        unlock();
        WAIT_UNTIL_TRUE(getNumberOfWindowsToTile(getActiveWindowStack(),0)>=i)

        do{
            assert(getNumberOfWindowsToTile(getActiveWindowStack(),0)==i);
            assert(getNumberOfWindowsToTile(getActiveWindowStack(),0)==getSize(getActiveWindowStack()));
            Node*p1=getActiveWindowStack();
            area=0;

            FOR_AT_MOST(p1,limit,
                reply1=xcb_get_geometry_reply(dis, xcb_get_geometry(dis, getIntValue(p1)), NULL);
                if(isWindowVisible(getValue(p1))){
                    area+=reply1->width*reply1->height;
                }
                free(reply1);
            )
            msleep(10);
        }while(area!=targetArea);
    }
    Node*p1=getActiveWindowStack();
    int count=0;
    FOR_AT_MOST(p1,limit,
        count++;
        if(isWindowVisible(getValue(p1))){
            reply1=xcb_get_geometry_reply(dis, xcb_get_geometry(dis, getIntValue(p1)), NULL);
            Node*p2=p1->next;
            if(p2)
                FOR_AT_MOST(p2,limit-count,
                    if(isWindowVisible(getValue(p2))){
                        reply2=xcb_get_geometry_reply(dis, xcb_get_geometry(dis, getIntValue(p2)), NULL);
                        assert(intersects(&reply1->x, &reply2->x)==0);
                        free(reply2);
                    }
                )
            free(reply1);
        }
    )
}END_TEST

START_TEST(test_layouts_transition){

    LOAD_SAVED_STATE=0;
    int layoutIndex=_i;
    setActiveLayout(&DEFAULT_LAYOUTS[layoutIndex]);
    int size=10;
    int startingWorkspace = 0;
    switchToWorkspace(startingWorkspace);
    int numOfMasters=5;
    for(int i=0;i<numOfMasters-1;i++)
        createMasterDevice((char[]){'A'+i,0});
    initCurrentMasters();
    WindowInfo*markedWindows[numOfMasters];
    Master*masterList[numOfMasters];
    Node*masters=getAllMasters();

    int i=0;
    int idle;
    FOR_AT_MOST(masters,numOfMasters,masterList[i++]=getValue(masters));
    START_MY_WM
    lock();
    for(int i=0;i<size;i++)
        createNormalWindow();
    flush();
    unlock();
    WAIT_UNTIL_TRUE(getNumberOfWindowsToTile(getActiveWindowStack(),0)==size)
    lock();
    Node*stack=getLast(getActiveWindowStack());
    i=0;
    FOR_AT_MOST_REVERSED(stack,numOfMasters,markedWindows[i++]=getValue(stack));
    unlock();
    for(int i=0;i<numOfMasters;i++){
        lock();
        setActiveMaster(masterList[i]);
        activateWindow(markedWindows[i]);
        flush();
        unlock();
        idle=getIdleCount();
        WAIT_UNTIL_TRUE(getIdleCount()!=idle);
    }

    lock();
    switchToWorkspace(!startingWorkspace);
    assert(getActiveWorkspaceIndex()!=startingWorkspace);
    assert(!isNotEmpty(getActiveWindowStack()));
    createNormalWindow();
    idle=getIdleCount();
    unlock();
    WAIT_UNTIL_TRUE(getIdleCount()!=idle);

    assert(getActiveWorkspaceIndex()!=startingWorkspace);
    lock();
    WindowInfo* tempWindow=(getValue(getActiveWindowStack()));
    for(int i=0;i<numOfMasters;i++){
        setActiveMaster(masterList[i]);
        focusWindowInfo(tempWindow);
    }
    flush();
    idle=getIdleCount();
    unlock();
    WAIT_UNTIL_TRUE(getIdleCount()!=idle);
    lock();
    for(int i=0;i<numOfMasters;i++){
        assert(getValue(getFocusedWindowByMaster(masterList[i]))==tempWindow);
    }
    switchToWorkspace(startingWorkspace);
    assert(getActiveWorkspaceIndex()==startingWorkspace);
    flush();
    idle=getIdleCount();
    unlock();
    WAIT_UNTIL_TRUE(getIdleCount()!=idle);
    for(int i=0;i<numOfMasters;i++){
        assert(getValue(getFocusedWindowByMaster(masterList[i]))==markedWindows[i]);
    }
    for(int i=0;i<numOfMasters;i++)
        destroyMasterDevice(masterList[i]->id, 2, 3);
    flush();
    WAIT_UNTIL_TRUE(getSize(getAllMasters())==1);

}END_TEST
START_TEST(test_fixed_position_windows){
    int size=4;
    for(int i=1;i<=size;i++)
        mapWindow(createNormalWindow());
    flush();
    scan(root);
    short config[]={1,2,3,4};
    WindowInfo*winInfo=getValue(getAllWindows());
    setConfig(winInfo, config);
    setActiveLayout(&DEFAULT_LAYOUTS[_i]);
    tileWorkspace(getActiveWorkspaceIndex());
    xcb_get_geometry_reply_t *reply=xcb_get_geometry_reply(dis, xcb_get_geometry(dis, winInfo->id), NULL);

    assert(memcmp(config, &reply->x, sizeof(short)*LEN(config))==0);
    free(reply);


}END_TEST


Suite *layoutSuite() {
    Suite*s = suite_create("Layouts");
    TCase* tc_core = tcase_create("Layouts");
    tcase_add_checked_fixture(tc_core, onStartup, fullCleanup);
    tcase_add_loop_test(tc_core, test_layouts,0,NUMBER_OF_DEFAULT_LAYOUTS+1);
    tcase_add_loop_test(tc_core, test_layouts_transition,0,NUMBER_OF_DEFAULT_LAYOUTS);
    tcase_add_loop_test(tc_core, test_fixed_position_windows,0,NUMBER_OF_DEFAULT_LAYOUTS);
    suite_add_tcase(s, tc_core);
    return s;
}
