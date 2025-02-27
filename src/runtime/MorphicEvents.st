Object subclass: #MorphEvent instanceVariables: #(wasHandled).

Object subclass: #MorphEventVisitor instanceVariables: #().

MorphEvent subclass: #MorphMouseEvent instanceVariables: #(position).
MorphMouseEvent subclass: #MorphMouseButtonEvent instanceVariables: #(button).
MorphMouseButtonEvent subclass: #MorphMouseButtonDownEvent.
MorphMouseButtonEvent subclass: #MorphMouseButtonUpEvent.
MorphMouseEvent subclass: #MorphMouseMotionEvent instanceVariables: #(buttons deltaPosition).

MorphMouseEvent subclass: #MorphMouseEnterEvent instanceVariables: #().
MorphMouseEvent subclass: #MorphMouseLeaveEvent instanceVariables: #().

MorphEvent subclass: #MorphKeyboardEvent instanceVariables: #(scancode symbol).
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
