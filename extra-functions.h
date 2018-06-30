/*
 * extra-functions.c
 *
 *  Created on: Jul 14, 2018
 *      Author: arthur
 */

void sendDeviceAction(int id,int detail,int type);
int getActivePointerId();
void sendButtonPress(int button);
void sendButtonRelease(int button);
void sendKeyPress(int keycode);
void sendKeyRelease(int keycode);
