// Desk-test stand-in for GUIslice: the parts include/test_GSLC.h and
// src/MenuCallbacks.cpp use. Elements live in a small pool so the test can find
// them by ID and read their image and slider position. Nothing is drawn.
//
// gslc_ElemXSliderSetPos() behaves like GUIslice's XSlider.c: it clips the
// position and, if it changed, calls the position callback synchronously. That
// is what makes the firmware's echo guards necessary, so the test exercises them.
#pragma once
#include <stdint.h>
#include <stddef.h>
#include "GUIslice_th.h"

typedef struct { uint8_t r, g, b; } gslc_tsColor;
#define GSLC_COL_BLACK    (gslc_tsColor){0, 0, 0}
#define GSLC_COL_WHITE    (gslc_tsColor){255, 255, 255}
#define GSLC_COL_GRAY_LT2 (gslc_tsColor){160, 160, 160}

typedef struct { int16_t x, y; uint16_t w, h; } gslc_tsRect;

typedef enum { GSLC_IMGREF_FMT_RAW1 = 1 } gslc_teImgRefFlags;
typedef struct { const unsigned char *pImgBuf; } gslc_tsImgRef;

typedef enum { GSLC_FONTREF_PTR = 1 } gslc_teFontRefType;
typedef enum { GSLC_ALIGN_MID_MID = 5 } gslc_teAlign;
typedef enum { GSLC_TOUCH_NONE = 0, GSLC_TOUCH_DOWN_IN, GSLC_TOUCH_MOVE_IN, GSLC_TOUCH_UP_IN } gslc_teTouch;
typedef enum { GSLC_REDRAW_NONE = 0, GSLC_REDRAW_INC, GSLC_REDRAW_FULL } gslc_teRedrawType;

#define GSLC_ID_AUTO (-1)

typedef bool (*GSLC_CB_TOUCH)(void *pvGui, void *pvElemRef, gslc_teTouch eTouch, int16_t nX, int16_t nY);
typedef bool (*GSLC_CB_XSLIDER_POS)(void *pvGui, void *pvElemRef, int16_t nPos);

typedef struct {
    int16_t       nId;
    int16_t       nPage;
    void         *pXData;     // gslc_tsXSlider* for sliders
    gslc_tsImgRef img;        // current image (image buttons)
    GSLC_CB_TOUCH pfuncTouch;
    const char   *txt;
} gslc_tsElem;

typedef struct { gslc_tsElem *pElem; } gslc_tsElemRef;

typedef struct {
    int16_t nPos, nPosMin, nPosMax;
    GSLC_CB_XSLIDER_POS pfuncXPos;
} gslc_tsXSlider;

typedef struct { bool bOn; } gslc_tsXTogglebtn;
typedef struct { bool bOn; } gslc_tsXToggleImgbtn;

typedef struct { int16_t nPageCur; int16_t nPopup; } gslc_tsGui;
typedef struct { int dummy; } gslc_tsDriver;
typedef struct { int dummy; } gslc_tsFont;
typedef struct { int dummy; } gslc_tsPage;

typedef int16_t (*GSLC_CB_DEBUG_OUT)(char ch);

bool gslc_Init(gslc_tsGui *pGui, gslc_tsDriver *pDrv, gslc_tsPage *asPage, uint8_t nMaxPage,
               gslc_tsFont *asFont, uint8_t nMaxFont);
bool gslc_InitDebug(GSLC_CB_DEBUG_OUT pfunc);
void gslc_InitTouchHandler(TouchHandler *pTH);
bool gslc_FontSet(gslc_tsGui *pGui, int16_t nFontId, gslc_teFontRefType eFontRefType,
                  const void *pvFontRef, uint16_t nFontSz);
void gslc_PageAdd(gslc_tsGui *pGui, int16_t nPageId, gslc_tsElem *psElem, uint16_t nMaxElem,
                  gslc_tsElemRef *psElemRef, uint16_t nMaxElemRef);
void gslc_SetPageCur(gslc_tsGui *pGui, int16_t nPageId);
void gslc_SetBkgndColor(gslc_tsGui *pGui, gslc_tsColor nCol);
bool gslc_GuiRotate(gslc_tsGui *pGui, uint8_t nRotation);
void gslc_PageRedrawSet(gslc_tsGui *pGui, bool bRedraw);
void gslc_Update(gslc_tsGui *pGui);
void gslc_PopupShow(gslc_tsGui *pGui, int16_t nPageId, bool bModal);
void gslc_PopupHide(gslc_tsGui *pGui);

gslc_tsImgRef   gslc_GetImageFromProg(const unsigned char *pImgBuf, gslc_teImgRefFlags eFmt);
gslc_tsElemRef *gslc_PageFindElemById(gslc_tsGui *pGui, int16_t nPageId, int16_t nElemId);
gslc_tsElem    *gslc_GetElemFromRef(gslc_tsGui *pGui, gslc_tsElemRef *pElemRef);

gslc_tsElemRef *gslc_ElemCreateBox(gslc_tsGui *pGui, int16_t nElemId, int16_t nPage, gslc_tsRect rElem);
gslc_tsElemRef *gslc_ElemCreateLine(gslc_tsGui *pGui, int16_t nElemId, int16_t nPage,
                                    int16_t nX0, int16_t nY0, int16_t nX1, int16_t nY1);
gslc_tsElemRef *gslc_ElemCreateTxt(gslc_tsGui *pGui, int16_t nElemId, int16_t nPage, gslc_tsRect rElem,
                                   char *pStrBuf, uint8_t nStrBufMax, int16_t nFontId);
gslc_tsElemRef *gslc_ElemCreateBtnTxt(gslc_tsGui *pGui, int16_t nElemId, int16_t nPage, gslc_tsRect rElem,
                                      char *acStr, uint8_t nStrBufMax, int16_t nFontId, GSLC_CB_TOUCH cbTouch);
gslc_tsElemRef *gslc_ElemCreateBtnImg(gslc_tsGui *pGui, int16_t nElemId, int16_t nPage, gslc_tsRect rElem,
                                      gslc_tsImgRef sImgRef, gslc_tsImgRef sImgRefSel, GSLC_CB_TOUCH cbTouch);
void gslc_ElemSetCol(gslc_tsGui *pGui, gslc_tsElemRef *pElemRef, gslc_tsColor colFrame,
                     gslc_tsColor colFill, gslc_tsColor colFillGlow);
void gslc_ElemSetTxtCol(gslc_tsGui *pGui, gslc_tsElemRef *pElemRef, gslc_tsColor colVal);
void gslc_ElemSetTxtAlign(gslc_tsGui *pGui, gslc_tsElemRef *pElemRef, unsigned nAlign);
void gslc_ElemSetTxtStr(gslc_tsGui *pGui, gslc_tsElemRef *pElemRef, const char *pStr);
void gslc_ElemSetRoundEn(gslc_tsGui *pGui, gslc_tsElemRef *pElemRef, bool bRoundEn);
void gslc_ElemSetFillEn(gslc_tsGui *pGui, gslc_tsElemRef *pElemRef, bool bFillEn);
void gslc_ElemSetFrameEn(gslc_tsGui *pGui, gslc_tsElemRef *pElemRef, bool bFrameEn);
void gslc_ElemSetImage(gslc_tsGui *pGui, gslc_tsElemRef *pElemRef, gslc_tsImgRef sImgRef, gslc_tsImgRef sImgRefSel);

gslc_tsElemRef *gslc_ElemXSliderCreate(gslc_tsGui *pGui, int16_t nElemId, int16_t nPage, gslc_tsXSlider *pXData,
                                       gslc_tsRect rElem, int16_t nPosMin, int16_t nPosMax, int16_t nPos,
                                       uint16_t nThumbSz, bool bVert);
void gslc_ElemXSliderSetStyle(gslc_tsGui *pGui, gslc_tsElemRef *pElemRef, bool bTrimStyle, gslc_tsColor colTrim,
                              uint16_t nTickDiv, int16_t nTickLen, gslc_tsColor colTick);
void gslc_ElemXSliderSetPosFunc(gslc_tsGui *pGui, gslc_tsElemRef *pElemRef, GSLC_CB_XSLIDER_POS funcCb);
void gslc_ElemXSliderSetPos(gslc_tsGui *pGui, gslc_tsElemRef *pElemRef, int16_t nPos);

gslc_tsElemRef *gslc_ElemXTogglebtnCreate(gslc_tsGui *pGui, int16_t nElemId, int16_t nPage,
                                          gslc_tsXTogglebtn *pXData, gslc_tsRect rElem,
                                          gslc_tsColor colThumb, gslc_tsColor colOnState, gslc_tsColor colOffState,
                                          bool bCircular, bool bChecked, GSLC_CB_TOUCH cbTouch);
bool gslc_ElemXTogglebtnGetState(gslc_tsGui *pGui, gslc_tsElemRef *pElemRef);
