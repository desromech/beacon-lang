Object subclass: #MorphEvent instanceVariables: #(wasHandled).

Object subclass: #MorphEventVisitor instanceVariables: #().

MorphEvent subclass: #MorphMouseEvent instanceVariables: #(position).
MorphMouseEvent subclass: #MorphMouseButtonEvent instanceVariables: #(button).
MorphMouseButtonEvent subclass: #MorphMouseButtonDownEvent.
MorphMouseButtonEvent subclass: #MorphMouseButtonUpEvent.
MorphMouseEvent subclass: #MorphMouseMotionEvent instanceVariables: #(buttons deltaPosition).
MorphMouseEvent subclass: #MorphMouseWheelEvent instanceVariables: #(scrollDelta).

MorphMouseEvent subclass: #MorphMouseEnterEvent instanceVariables: #().
MorphMouseEvent subclass: #MorphMouseLeaveEvent instanceVariables: #().

MorphEvent subclass: #MorphKeyboardEvent instanceVariables: #(scancode symbol modstate).
MorphKeyboardEvent subclass: #MorphKeyboardDownEvent instanceVariables: #().
MorphKeyboardEvent subclass: #MorphKeyboardUpEvent instanceVariables: #().

MorphEvent subclass: #MorphTextInputEvent instanceVariables: #(text).

MorphEvent subclass: #MorphGotFocusEvent instanceVariables: #().
MorphEvent subclass: #MorphLostFocusEvent instanceVariables: #().

MorphEvent ![
isMouseEvent
    ^ false
].

MorphMouseEvent ![
isMouseEvent
    ^ true
].

MorphEvent ![
isKeyboardEvent
    ^ false
].

MorphKeyboardEvent ![
isKeyboardEvent
    ^ true
].

MorphEvent ![
isTextInputEvent
    ^ false
].

MorphTextInputEvent ![
isTextInputEvent
    ^ true
].

MorphTextInputEvent ![
text
    ^ text
].

MorphTextInputEvent ![
text: aString
    text := aString
].

MorphEvent ![
initialize
    super initialize.
    wasHandled := false
].

MorphEvent ![
wasHandled
    ^ wasHandled
].

MorphEvent ![
wasHandled: aBoolean
    wasHandled := aBoolean
].

MorphEvent ![
accept: aVisitor
    ^ aVisitor handleMorphEvent: self
].

MorphEventVisitor ![
handleMorphEvent: anEvent

].

MorphMouseButtonEvent ![
accept: aVisitor
    ^ aVisitor handleMorphMouseEvent: self
].

MorphEventVisitor ![
handleMorphMouseEvent: anEvent
    ^ self handleMorphEvent: anEvent
].

MorphMouseButtonEvent ![
accept: aVisitor
    ^ aVisitor handleMorphMouseButtonEvent: self
].

MorphEventVisitor ![
handleMorphMouseButtonEvent: anEvent
    ^ self handleMorphMouseEvent: anEvent
].

MorphMouseButtonDownEvent ![
accept: aVisitor
    ^ aVisitor handleMorphMouseButtonDownEvent: self
].

MorphEventVisitor ![
handleMorphMouseButtonDownEvent: anEvent
    ^ self handleMorphMouseButtonEvent: anEvent
].

MorphMouseButtonUpEvent ![
accept: aVisitor
    ^ aVisitor handleMorphMouseButtonUpEvent: self
].

MorphEventVisitor ![
handleMorphMouseButtonUpEvent: anEvent
    ^ self handleMorphMouseButtonEvent: anEvent
].

MorphMouseMotionEvent ![
accept: aVisitor
    ^ aVisitor handleMorphMouseMotionEvent: self
].

MorphEventVisitor ![
handleMorphMouseMotionEvent: anEvent
    ^ self handleMorphMouseEvent: anEvent
].

MorphMouseWheelEvent ![
accept: aVisitor
    ^ aVisitor handleMorphMouseWheelEvent: self
].

MorphEventVisitor ![
handleMorphMouseWheelEvent: anEvent
    ^ self handleMorphMouseEvent: anEvent
].

MorphMouseEnterEvent ![
accept: aVisitor
    ^ aVisitor handleMorphMouseEnterEvent: self
].

MorphEventVisitor ![
handleMorphMouseEnterEvent: anEvent
    ^ self handleMorphMouseEvent: anEvent
].

MorphMouseLeaveEvent ![
accept: aVisitor
    ^ aVisitor handleMorphMouseLeaveEvent: self
].

MorphEventVisitor ![
handleMorphMouseLeaveEvent: anEvent
    ^ self handleMorphMouseEvent: anEvent
].

MorphKeyboardEvent ![
accept: aVisitor
    ^ aVisitor handleMorphKeyboardEvent: self
].

MorphEventVisitor ![
handleMorphKeyboardEvent: anEvent
    ^ self handleMorphEvent: anEvent
].

MorphKeyboardEvent ![
isReturn
    ^ symbol = 13
].

MorphKeyboardEvent ![
isBackspace
    ^ symbol = 8
].

MorphKeyboardEvent ![
isLeft
    ^ symbol = 1073741904
].

MorphKeyboardEvent ![
isRight
    ^ symbol = 1073741903
].

MorphKeyboardEvent ![
isUp
    ^ symbol = 1073741906
].

MorphKeyboardEvent ![
isDown
    ^ symbol = 1073741905
].

MorphKeyboardEvent ![
isPageUp
    ^ symbol = 1073741899
].

MorphKeyboardEvent ![
isPageDown
    ^ symbol = 1073741902
].

MorphKeyboardEvent ![
isHome
    ^ symbol = 1073741898
].

MorphKeyboardEvent ![
isEnd
    ^ symbol = 1073741901
].

MorphKeyboardEvent ![
isDelete
    ^ symbol = 127
].
"
    KMOD_NONE = 0x0000,
    KMOD_LSHIFT = 0x0001,
    KMOD_RSHIFT = 0x0002,
    KMOD_LCTRL = 0x0040,
    KMOD_RCTRL = 0x0080,
    KMOD_LALT = 0x0100,
    KMOD_RALT = 0x0200,
    KMOD_LGUI = 0x0400,
    KMOD_RGUI = 0x0800,
    KMOD_NUM = 0x1000,
    KMOD_CAPS = 0x2000,
    KMOD_MODE = 0x4000,
    KMOD_SCROLL = 0x8000,

    KMOD_CTRL = KMOD_LCTRL | KMOD_RCTRL,
    KMOD_SHIFT = KMOD_LSHIFT | KMOD_RSHIFT,
    KMOD_ALT = KMOD_LALT | KMOD_RALT,
    KMOD_GUI = KMOD_LGUI | KMOD_RGUI,
"

MorphKeyboardEvent ![
hasControl
    ^ modstate anyMask: 16r0040 | 16r0080
].

MorphKeyboardEvent ![
hasShift
    ^ modstate anyMask: 16r0001 | 16r0002
].

MorphKeyboardEvent ![
hasAlt
    ^ modstate anyMask: 16r0100 | 16r0200
].

MorphKeyboardEvent ![
modstate
    ^ modstate
].

MorphKeyboardEvent ![
modstate: aModState
    modstate := aModState
].

MorphKeyboardDownEvent ![
accept: aVisitor
    ^ aVisitor handleMorphKeyboardDownEvent: self
].

MorphEventVisitor ![
handleMorphKeyboardDownEvent: anEvent
    ^ self handleMorphKeyboardEvent: anEvent
].

MorphKeyboardUpEvent ![
accept: aVisitor
    ^ aVisitor handleMorphKeyboardUpEvent: self
].

MorphEventVisitor ![
handleMorphKeyboardUpEvent: anEvent
    ^ self handleMorphKeyboardEvent: anEvent
].

MorphEventVisitor ![
handleMorphTextInputEvent: anEvent

].

MorphTextInputEvent ![
accept: aVisitor
    ^ aVisitor handleMorphTextInputEvent: self
].

MorphGotFocusEvent ![
accept: aVisitor
    ^ aVisitor handleMorphGotFocusEvent: self
].

MorphEventVisitor ![
handleMorphGotFocusEvent: anEvent
].

MorphLostFocusEvent ![
accept: aVisitor
    ^ aVisitor handleMorphLostFocusEvent: self
].

MorphEventVisitor ![
handleMorphLostFocusEvent: anEvent
].

MorphMouseEvent ![
position
    ^ position
].

MorphMouseEvent ![
position: aPoint
    position := aPoint
].

MorphMouseEvent ![
translatedBy: deltaPosition during: aBlock
    | oldPosition |
    oldPosition := position.
    position := position + deltaPosition.

    aBlock value.

    position := oldPosition.
].

MorphMouseButtonEvent ![
button
    ^ button
].

MorphMouseButtonEvent ![
button: anInteger
    button := anInteger
].

MorphMouseMotionEvent ![
buttons
    ^ buttons
].

MorphMouseMotionEvent ![
buttons: anInteger
    buttons := anInteger
].

MorphMouseMotionEvent ![
deltaPosition
    ^ deltaPosition
].

MorphMouseMotionEvent ![
deltaPosition: aPoint
    deltaPosition := aPoint
].

MorphMouseWheelEvent ![
scrollDelta
    ^ scrollDelta
].

MorphMouseWheelEvent ![
scrollDelta: aPoint
    scrollDelta := aPoint
].

MorphKeyboardEvent ![
scancode
    ^ scancode
].

MorphKeyboardEvent ![
scancode: aScancode
    scancode := aScancode
].

MorphKeyboardEvent ![
symbol
    ^ symbol
].

MorphKeyboardEvent ![
symbol: aSymbol
    symbol := aSymbol
].
