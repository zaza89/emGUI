 /*
	emGUI Library V1.0.0 - Copyright (C) 2013 Roman Savrulin <romeo.deepmind@gmail.com>

    This file is part of the emGUI Library distribution.

    emGUI Library is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Roman Savrulin AND MODIFIED BY the emGUI Library exception.
	
    >>>NOTE<<< The modification to the GPL is included to allow you to
    distribute a combined work that includes emGUI Library without being obliged to
    provide the source code for proprietary components outside of the emGUI Library.
	emGUI Library is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
    more details. You should have received a copy of the GNU General Public
    License and the emGUI Library license exception along with emGUI Library; if not it
    can be obtained by writing to Roman Savrulin <romeo.deepmind@gmail.com>.
	
	Created on: 06.03.2013
*/


#ifdef __cplusplus
extern "C" {
#endif
  
#include "include.h"
#include "PictureStorage.h"
#include "Widgets/Label.h"
#include "Widgets/Button.h"
#include "Widgets/StatusBar.h"
#include "Widgets/Window.h"
#include "Widgets/MenuButton.h"
#include "GuiInterface.h"
#include "GsmInterface.h"
#include "Memory.h"
#include <string.h>
#include "Widgets/ProgressBar.h"
#include "Fonts.h"
  
#include "ModalDialog.h"

#define MODAL_DIALOG_MAX_BUTTONS    3
#define MODAL_DIALOG_MAX_COUNT      10
#define MODAL_DIALOG_MAX_MSG_LENGTH 255
#define PB_BORDER                   20

static xWindow *xThisWnd;

static xLabel         *xMessageHeader;
static xLabel         *xMessage;
static xProgressBar   *xPBar;
static xMenuButton    *xButtons[MODAL_DIALOG_MAX_BUTTONS]; // y(ok)/n/c �������� � ������� ����� 4 ������.

//������������� ��� �������������� ��������
u16 usDlgID = MODAL_AUTO + 1;

typedef struct xModalDialog_t xModalDialog;

struct xModalDialog_t {
  void (*pxClickHandlers[MODAL_DIALOG_MAX_BUTTONS])   ();

  void (*pxDefaultHandler)   ();

  //bool bActive
  char sDialogConfig[MODAL_DIALOG_MAX_BUTTONS + 1];
  u16  usDlgID;
  char const* sHdr;
  char const* sMsg;
  signed char cProgress;
  bool bCanClose;
  xModalDialog * pxPrev;
};

typedef struct xModalDialogPictureSet_t{
  xPicture xPicMain;
  xPicture xPicMainPress;
  char* strLabel;
  //bool (*pxClickHandler)   (xWidget *);
}xModalDialogPictureSet;

xModalDialog *xMDActive = NULL;

//char cDialogCount = 0;
#define MDCurrent (cDialogCount - 1)

static void prvDlgShowActive();

static inline xModalDialog *prvGetNextDlg( xModalDialog *xDlg ){
  if(!xDlg)
    return NULL;
  return xDlg->pxPrev;
}

static void prvResetDlgWnd(){
  for(int c = 0; c < MODAL_DIALOG_MAX_BUTTONS; c++){
    vWidgetHide(xButtons[c]);
  }

  pcLabelSetText(xMessageHeader, "");
  pcLabelSetText(xMessage, "");
  vWidgetHide(xPBar);
}

bool static prvOnOpenHandler(xWidget *pxW){
  prvDlgShowActive();
  return TRUE;
}

bool static prvOnOpenRequestHandler(xWidget *pxW){
  if(xMDActive)
    return TRUE;
  else
    return FALSE;
}

bool static prvOnCloseHandler(xWidget *pxW){
  return TRUE;
}

bool static prvOnCloseRequestHandler(xWidget *pxW){
  if(xMDActive)
    vModalDialogClose(xMDActive->usDlgID, TRUE);

  if(xMDActive)
    return FALSE;
  else
    return TRUE;
}

bool static prvButtonHandler(xWidget *pxW){
  //������ � ����������� ����� ������� ��� ���� ������.
  //��� ���� ID ��������� ����������. ��� ���������� ����������
  //��� ����������. 
  int usDlgId = xMDActive->usDlgID;
  if(!xMDActive)
    return FALSE;
  for(int c = 0; c < MODAL_DIALOG_MAX_BUTTONS; c++){
    if(xButtons[c] == pxW && xMDActive->pxClickHandlers[c]){
      xMDActive->pxClickHandlers[c]();
      break;
    }
  }
  vModalDialogClose(usDlgId, FALSE);
  return TRUE;
}

xWidget * pxModalDialogWindowCreate(){

  // X0, Y0 - ���������� ������������ ��������
  u16 usX, usY;

  xThisWnd = pxWindowCreate(WINDOW_MODAL);
  vWidgetSetBgColor(xThisWnd, ColorEcgBackground, FALSE);
  vWindowSetOnOpenHandler(xThisWnd, prvOnOpenHandler);
  vWindowSetOnOpenRequestHandler(xThisWnd, prvOnOpenRequestHandler);
  vWindowSetOnCloseHandler(xThisWnd, prvOnCloseHandler);
  vWindowSetOnCloseRequestHandler(xThisWnd, prvOnCloseRequestHandler);

  xMessageHeader = pxLabelCreate(0, 0, usWidgetGetW(xThisWnd), usStatusBarGetH(), "ModalDialogHeader", FONT_ASCII_16_X, 0, xThisWnd);
  vWidgetSetBgColor(xMessageHeader, ColorMessageHeaderBackground, FALSE);
  vLabelSetTextAlign(xMessageHeader, LABEL_ALIGN_CENTER);
  vLabelSetVerticalAlign(xMessageHeader, LABEL_ALIGN_MIDDLE);
  vLabelSetTextColor(xMessageHeader, ColorMessageHeaderText);

  usY = (usInterfaceGetWindowH() * 4 )/10 - usStatusBarGetH();

  xMessage = pxLabelCreate(0, usWidgetGetH(xMessageHeader), usWidgetGetW(xThisWnd), usY, "ModalDialogText", FONT_ASCII_16_X, MODAL_DIALOG_MAX_MSG_LENGTH, xThisWnd);
  bLabelSetMultiline(xMessage, TRUE);
  vLabelSetTextAlign(xMessage, LABEL_ALIGN_CENTER);
  vLabelSetVerticalAlign(xMessage, LABEL_ALIGN_MIDDLE);

  usY = usWidgetGetY1(xMessage, FALSE);

  xPBar = pxProgressBarCreate(PB_BORDER, usY, usWidgetGetW(xThisWnd) - PB_BORDER * 2, 30, xThisWnd);
  vProgressBarSetProcExec(xPBar, 55);

  usY = (usInterfaceGetWindowH()/2 + LCD_TsBtn_SIZE/3);
  usX = 0;

  for(int c = 0; c < MODAL_DIALOG_MAX_BUTTONS; c++){
    xButtons[c] = pxMenuButtonCreate(usX, usY, pxPictureGet(Pic_ButtonOk), "", prvButtonHandler, xThisWnd);
    usX += LCD_TsBtn_SIZE;
    vWidgetHide(xButtons[c]);
  }
  return xThisWnd;
}

xModalDialogPictureSet prvGetPicSet(char cType){
  xModalDialogPictureSet xPicSet;
  switch(cType){
  /*case 'y':
    xPicSet.xPicMain = pxPictureGet(Pic_ButtonOk);
    xPicSet.xPicMainPress = pxPictureGet(Pic_ButtonOk_press);
    xPicSet.xPicLabel = pxPictureGet(Pic_b2_yes);*/
  case 'n':
    xPicSet.xPicMain = pxPictureGet(Pic_ButtonNo);
    xPicSet.strLabel = "���";
    break;
  case 'c':
    xPicSet.xPicMain = pxPictureGet(Pic_return);
    xPicSet.strLabel = "������";
    break;
  case 'o':
    xPicSet.xPicMain = pxPictureGet(Pic_ButtonOk);
    xPicSet.strLabel = "��";
    break;
  case 'e':
    xPicSet.xPicMain = pxPictureGet(Pic_ButtonNo);
    xPicSet.strLabel = "��";
    break;
  default:
    xPicSet.xPicMain = pxPictureGet(Pic_ButtonOk);
    xPicSet.strLabel = "��";
    break;
  }

  return xPicSet;
}

static inline void prvShowPB(xModalDialog * xDlg){
  if(xDlg->cProgress >= 0){
    vWidgetShow(xPBar);
    vProgressBarSetProcExec(xPBar, xDlg->cProgress);
  }else{
    vWidgetHide(xPBar);
  }
}

static void prvDlgShowActive(){
  xModalDialog * xDlg = xMDActive;

  if(!xDlg){
    //return;
    vInterfaceCloseWindow(WINDOW_MODAL);
    //TODO: ��������� ���-�� �������� �������� � 0
  }
  
  char cBtnCnt = strlen(xDlg->sDialogConfig);
  xModalDialogPictureSet xPicSet;

  char * sBtns = xDlg->sDialogConfig;

  xMenuButton * xBtn;

  u16 betweenBtnsX,
        usX, usY;

  prvResetDlgWnd();

  betweenBtnsX = (usInterfaceGetW() - cBtnCnt * usWidgetGetW(xButtons[0])) / (cBtnCnt + 1);
  usY = usWidgetGetY0(xButtons[0], FALSE);
  usX = betweenBtnsX;
  
  pcLabelSetText(xMessageHeader, xDlg->sHdr);
  pcLabelSetText(xMessage, xDlg->sMsg);
  
  prvShowPB(xDlg);

  for(int c = 0; c < cBtnCnt; c++){
    xBtn = xButtons[c];
    xPicSet = prvGetPicSet(sBtns[c]);

    bWidgetMoveTo(xBtn, usX, usY);
    vWidgetShow(xBtn);

    pxMenuButtonSetMainPic(xBtn, xPicSet.xPicMain);
    pxMenuButtonSetPushPic(xBtn, xPicSet.xPicMainPress);
    pxMenuButtonSetLabelText(xBtn, xPicSet.strLabel);

    usX += betweenBtnsX + usWidgetGetW(xBtn);
  }

}

void prvIncDlgId(){
  //TODO: thread protection???
  usDlgID++;
  if(usDlgID <= MODAL_AUTO)
    usDlgID = MODAL_AUTO + 1;
}

xModalDialog *prvDlgIsActive(int iDlgId){
  if(!xMDActive)
    return NULL;
  
  if(xMDActive->usDlgID == iDlgId)
    return xMDActive;

  return NULL;
}

xModalDialog *prvDlgIsOpened(int iDlgId, xModalDialog ** pxNext){
  xModalDialog * xDlg = xMDActive;
  
  *pxNext = NULL;

  while(xDlg){
    if(xDlg->usDlgID == iDlgId)
      return xDlg;
    *pxNext = xDlg;
    xDlg = prvGetNextDlg(xDlg);
  }

  return NULL;
}

inline xModalDialog *prvDelDlgFromStack(xModalDialog *pxN, xModalDialog *pxNext){

  xModalDialog * pxPrev; //����. ������ � �����

  pxPrev = pxN->pxPrev;
  pxNext->pxPrev = pxPrev;

  return pxN;
}

void prvDlgRefresh(xModalDialog * xDlg, char const* sBtns, char const* sHdr, char const* sMsg){
  if(!xDlg)
    return;

  xDlg->sHdr = sHdr;
  xDlg->sMsg = sMsg;
  xDlg->cProgress = -1;
  xDlg->bCanClose = TRUE;
  memcpy(xDlg->sDialogConfig, sBtns, MODAL_DIALOG_MAX_BUTTONS + 1);
  xDlg->sDialogConfig[MODAL_DIALOG_MAX_BUTTONS] = '\0';

  for(int c = 0; c < MODAL_DIALOG_MAX_BUTTONS; c++)
    xDlg->pxClickHandlers[c] = NULL;

  xDlg->pxDefaultHandler = NULL;
}

int iModalDialogOpen(int iDlgId, char const * sBtns, char const * sHdr, char const* sMsg){
  xModalDialog * xDlg;
  xModalDialog * xDlgNext;

  /*/�������� ����������� ����. ���������� �������� ��������
  if(cDialogCount >= MODAL_DIALOG_MAX_COUNT){
    vInterfaceOpenWindow(WINDOW_MODAL);
    return -1;
  }*/

  //���� ����� ������ ��� ������� - �������� ��� ����������� � ������������.
  if((xDlg = prvDlgIsActive(iDlgId))){
    prvDlgRefresh(xDlg, sBtns, sHdr, sMsg);
    prvDlgShowActive();
    vInterfaceOpenWindow(WINDOW_MODAL);
    return -1;
  }

  //������ ���� ���-�� � �����, ��� ���� �������
  if((xDlg = prvDlgIsOpened(iDlgId, &xDlgNext))){
    prvDlgRefresh(xDlg, sBtns, sHdr, sMsg);
    prvDelDlgFromStack(xDlg, xDlgNext);
  }else{
    //������� � ����� ���, ������� ����� ���������
    xDlg = pvMemoryMalloc(sizeof(xModalDialog), MEMORY_EXT);
    //cDialogCount ++;
    memcpy(xDlg->sDialogConfig, sBtns, MODAL_DIALOG_MAX_BUTTONS + 1);
    xDlg->sDialogConfig[MODAL_DIALOG_MAX_BUTTONS] = '\0';
    xDlg->sHdr = sHdr;
    xDlg->sMsg = sMsg;
    xDlg->cProgress = -1;
    xDlg->bCanClose = TRUE;
    xDlg->pxDefaultHandler = NULL;
    
    for(int c = 0; c < MODAL_DIALOG_MAX_BUTTONS; c++)
      xDlg->pxClickHandlers[c] = NULL;

    //���������� ������������� �������, �� ��������
    //����� ����������� ���������� ������ � ���� ��������.
    if(iDlgId != MODAL_AUTO)
      xDlg->usDlgID = iDlgId;
    else{
      xDlg->usDlgID = usDlgID;
      prvIncDlgId();
    }
  }
  
  //������ ������ ��������
  xDlg->pxPrev = xMDActive;
  xMDActive = xDlg;

  prvDlgShowActive();
  
  vInterfaceOpenWindow(WINDOW_MODAL);
  vWidgetInvalidate(xThisWnd);
  
  return xDlg->usDlgID;
}

void vModalDialogSetHandler(int iDlgID, char cHandler, void (*pxHandler)()){
  xModalDialog * xDlg;
  xModalDialog * xDlgNext;

  if(!(xDlg = prvDlgIsOpened(iDlgID, &xDlgNext)))
    return;

  for(int c = 0; c < MODAL_DIALOG_MAX_BUTTONS; c++){
    if(xDlg->sDialogConfig[c] == cHandler){
      xDlg->pxClickHandlers[c] = pxHandler;
      return;
    }
  }
  xDlg->pxDefaultHandler = pxHandler;
}

void vModalDialogSetCloseable(int iDlgID, bool bCanClose){
  xModalDialog * xDlg;
  xModalDialog * xDlgNext;

  if(!(xDlg = prvDlgIsOpened(iDlgID, &xDlgNext)))
    return;

  xDlg->bCanClose = bCanClose;
}


void vModalDialogSetProgress(int iDlgID, int iProgress){
  xModalDialog * xDlg;
  xModalDialog * xDlgNext;

  if(!(xDlg = prvDlgIsOpened(iDlgID, &xDlgNext)))
    return;

  if(iProgress < 0)
    iProgress = -1;

  if(iProgress > 100)
    iProgress = 100;

  if(xDlg->cProgress == iProgress)
    return;

  xDlg->cProgress = iProgress;

  if(prvDlgIsActive(iDlgID)){
    prvShowPB(xDlg);
  }
}

void vModalDialogClose(int iDlgID, bool bFireDefault){
  xModalDialog * xDlg;
  xModalDialog * xDlgNext;

  xDlg = prvDlgIsOpened(iDlgID, &xDlgNext);

  //����� ������ �� ��� � �����
  if(!xDlg)
    return;

  //���� ������ ������ ���������
  if(!xDlg->bCanClose)
    return;

  //���� ������ ��������, �� ������ �������� ����������.
  if(prvDlgIsActive(iDlgID)){
    xMDActive = xMDActive->pxPrev;
  }else
    prvDelDlgFromStack(xDlg, xDlgNext);

  //���������� ������� �� ���������, ���� ���� � ��� ����.
  if(bFireDefault){
    if(xDlg->pxDefaultHandler)
      xDlg->pxDefaultHandler(NULL);
  }
  vMemoryFree(xDlg);
  prvDlgShowActive();
}


#ifdef __cplusplus
}
#endif
