// HELP.CPP  Turbo Vision 1.0
// Copyright (c) 1991 by Borland International
// @SERG
//
#include <pp.h>
#include <ppdlgs.h>
#pragma hdrstop
#include <help.h>
#include <string.h>
#include <sys\stat.h>
#include <io.h>

#define cHelpViewer     "\x03\x06\x07\x08"
#define cHelpWindow     "\x08\x4E\x4F\x50\x51\x52\x53\x54\x55"

#pragma warn -dsz
//
// THelpViewer
//
THelpViewer::THelpViewer(const TRect& bounds, TScrollBar* aHScrollBar,
	TScrollBar* aVScrollBar, THelpFile *aHelpFile, int context) :
	TScroller(bounds, aHScrollBar, aVScrollBar)
{
	options = (options | ofSelectable);
	growMode = gfGrowHiX | gfGrowHiY;
	hFile = aHelpFile;
	topic = 0;
	stack = 0;
	stackLim = stackTop = 0;
	limit.x = 78;
	initTopic(context);
}

THelpViewer::~THelpViewer()
{
	delete hFile;
	delete topic;
	delete stack;
}

void THelpViewer::push(int val)
{
	if(stackTop >= stackLim) {
		int * tmp = new int[stackLim + 8];
		if(tmp) {
			if(stack) {
				memcpy(tmp, stack, stackLim * sizeof(int));
				delete stack;
			}
			stackLim += 8;
			stack = tmp;
		}
		else
			return;
	}
	stack[stackTop++] = val;
}

int THelpViewer::pop()
{
	return stackTop ? stack[--stackTop] : 0xFFFF;
}

int THelpViewer::empty()
{
	return (stackTop == 0);
}

void THelpViewer::changeBounds(const TRect& bounds)
{
	TScroller::changeBounds(bounds);
	topic->setWidth(size.x);
	setLimit(limit.x, topic->numLines());
}

void THelpViewer::draw()
{
	TDrawBuffer b;
	char        line[256];
	int         i, j, l;
	int         keyCount   = 0;
	int         normal     = getColor(1);
	int         keyword    = getColor(2);
	int         selKeyword = getColor(3);
	int         c;
	TPoint      keyPoint;
	int         keyLength;
	int         keyRef;
	keyPoint.x = 0;
	keyPoint.y = 0;
	topic->setWidth(size.x);
	int numCrossRefs = topic->getNumCrossRefs();
	if(numCrossRefs > 0)
		do
			topic->getCrossRef(keyCount++, &keyPoint, &keyLength, &keyRef);
		while((keyCount<numCrossRefs) && keyPoint.y <= delta.y);
	for(i = 1; i <= size.y; ++i) {
		b.moveChar(0, ' ', normal, size.x);
		topic->getLine(line,i + delta.y);
		if(strlen(line) > delta.x) {
			line[delta.x + size.x] = 0;
			b.moveStr(0, line + delta.x, normal);
		}
		else
			b.moveStr(0, "", normal);
		while(i + delta.y == keyPoint.y) {
			l = keyLength;
			if(keyPoint.x < delta.x) {
				l -= (delta.x - keyPoint.x);
				keyPoint.x = delta.x;
			}
			c = (keyCount == selected) ? selKeyword : keyword;
			for(j = 0; j < l; ++j)
				b.putAttribute(keyPoint.x - delta.x + j, c);
			if(++keyCount <= numCrossRefs)
				topic->getCrossRef(keyCount-1, &keyPoint, &keyLength, &keyRef);
			else
				keyPoint.y = 0;
		}
		writeLine(0, i-1, size.x, 1, b);
	}
}

static char *_pal = cHelpViewer;

TPalette &THelpViewer::getPalette() const
{
	return *(TPalette*)&_pal;
}

void THelpViewer::makeSelectVisible(int selected, TPoint *keyPoint,
	int *keyLength, int *keyRef)
{
	TPoint d;
	topic->getCrossRef(selected, keyPoint, keyLength, keyRef);
	d = delta;
	if(keyPoint->x < d.x)             d.x = keyPoint->x;
	if(keyPoint->x > d.x + size.x)    d.x = keyPoint->x - size.x + 2;
	if(keyPoint->y <= d.y)            d.y = keyPoint->y - 1;
	if(keyPoint->y > d.y + size.y)    d.y = keyPoint->y - size.y;
	if((d.x != delta.x) || (d.y != delta.y))
		scrollTo(d.x, d.y);
}

void THelpViewer::switchToTopic(int keyRef)
{
	if(keyRef != current) {
		push(current);
		initTopic(keyRef);
	}
}

void THelpViewer::switchToPrevTopic()
{
	if(!empty())
		initTopic(pop());
}

void THelpViewer::initTopic(int context)
{
	if(topic)
		delete topic;
	topic = hFile->getTopic(context);
	topic->setWidth(size.x);
	scrollTo(0, 0);
	setLimit(limit.x, topic->numLines());
	selected = 1;
	current = context;
}

IMPL_HANDLE_EVENT(THelpViewer)
{
	TPoint keyPoint, p;
	int    keyLength;
	int    keyRef;
	int    keyCount;
	TScroller::handleEvent(event);
	switch(event.what) {
		case evBroadcast:
			switch(event.message.command) {
				case cmHelpReply:
					break;
				case cmHelpPrevTopic:
					switchToPrevTopic();
					drawView();
					break;
				case cmHelpSwitchToTopic:
					switchToTopic(*(ushort*)event.message.infoPtr);
					drawView();
					break;
			}
			clearEvent(event);
			break;
		case evKeyDown:
			switch(event.keyDown.keyCode) {
				case kbTab:
					if(++selected > topic->getNumCrossRefs())
						selected = 1;
					if(topic->getNumCrossRefs())
						makeSelectVisible(selected-1,&keyPoint,&keyLength,&keyRef);
					break;
				case kbShiftTab:
					if(--selected == 0)
						selected = topic->getNumCrossRefs();
					if(topic->getNumCrossRefs())
						makeSelectVisible(selected-1,&keyPoint,&keyLength,&keyRef);
					break;
				case kbEnter:
					if(selected <= topic->getNumCrossRefs()) {
						topic->getCrossRef(selected-1, &keyPoint, &keyLength, &keyRef);
						switchToTopic(keyRef);
					}
					break;
				case kbAltF1:
					switchToPrevTopic();
					break;
				case kbEsc:
					event.what = evCommand;
					event.message.command = cmClose;
					putEvent(event);
					break;
				default:
					return;
			}
			drawView();
			clearEvent(event);
			break;
		case evMouseDown:
			p = makeLocal(event.mouse.where);
			p.x += delta.x;
			p.y += delta.y;
			keyCount = 0;
			do {
				if(++keyCount > topic->getNumCrossRefs())
					return;
				topic->getCrossRef(keyCount-1, &keyPoint, &keyLength, &keyRef);
			} while (!((keyPoint.y == p.y+1) && (p.x >= keyPoint.x) &&
				  (p.x < keyPoint.x + keyLength)));
			if(event.mouse.doubleClick)
				switchToTopic(keyRef);
			else
				selected = keyCount;
			drawView();
			clearEvent(event);
			break;
		case evCommand:
			if(event.message.command == cmClose && owner->state & sfModal) {
				clearEvent(event);
				endModal(cmClose);
				return; // После endModal не следует обращаться к this
			}
			break;
		}
}
//
// THelpWindow
//
TRect THelpWindow::lastBounds(0, 1, 80, 23);

THelpWindow::THelpWindow(THelpFile *hFile, int context) : TWindow(THelpWindow::lastBounds, "Џ®¬®йм", wnNoNumber)
{
	TRect r = getExtent();
	r.grow(-1, -1);
	insert(new THelpViewer(r, standardScrollBar(sbHorizontal | sbHandleKeyboard),
		standardScrollBar(sbVertical | sbHandleKeyboard), hFile, context));
}

void THelpWindow::changeBounds(const TRect &r)
{
	TWindow::changeBounds(r);
	THelpWindow::lastBounds = r;
}

static char * __pal = cHelpWindow;

TPalette &THelpWindow::getPalette() const
{
	return *(TPalette*)&__pal;
}

#pragma warn .dsz
