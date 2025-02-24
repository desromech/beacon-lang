Object subclass: #MorphEvent instanceVariables: #().
Object subclass: #MorphEventVisitor instanceVariables: #().

MorphEvent subclass: #MorphMouseButtonEvent instanceVariables: #(button position).
MorphMouseButtonEvent subclass: #MorphMouseButtonDownEvent.
MorphMouseButtonEvent subclass: #MorphMouseButtonUpEvent.
MorphEvent subclass: #MorphMouseMotionEvent instanceVariables: #(buttons position deltaPosition).
MorphEvent subclass: #MorphKeyboardEvent instanceVariables: #(scancode symbol).
MorphKeyboardEvent subclass: #MorphKeyboardDownEvent instanceVariables: #().
MorphKeyboardEvent subclass: #MorphKeyboardUpEvent instanceVariables: #().

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

MorphKeyboardEvent ![
accept: aVisitor
    ^ aVisitor handleMorphKeyboardEvent: self
].

MorphEventVisitor ![
handleMorphKeyboardEvent: anEvent
    ^ self handleMorphEvent: anEvent
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

MorphMouseButtonEvent ![
button
    ^ button
].

MorphMouseButtonEvent ![
button: anInteger
    button := anInteger
].

MorphMouseButtonEvent ![
position
    ^ position
].

MorphMouseButtonEvent ![
position: aPoint
    position := aPoint
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
position
    ^ position
].

MorphMouseMotionEvent ![
position: aPoint
    position := aPoint
].

MorphMouseMotionEvent ![
deltaPosition
    ^ deltaPosition
].

MorphMouseMotionEvent ![
deltaPosition: aPoint
    deltaPosition := aPoint
].
