Window subclass: #MorphicWindow instanceVariables: #(rootMorph).

MorphicWindow ![
rootMorph: aRootMorph
    rootMorph := aRootMorph
].

MorphicWindow ![
renderWithBuilder: renderingBuilder
    rootMorph == nil ifTrue: [^ self].

    rootMorph position: 0@0.
    rootMorph extent: width @ height..
    rootMorph fullRenderWithBuilder: renderingBuilder
].

Morph ![
openInWindow
    ^ MorphicWindow new
        extent: bounds extent;
        rootMorph: self;
        open;
        yourself
].

MorphicWindow ![
onMouseButtonDown: event
    rootMorph processEvent: (MorphMouseButtonDownEvent new
        button: event button;
        position: event x @ event y;
        yourself)
].

MorphicWindow ![
onMouseButtonUp: event
    rootMorph processEvent: (MorphMouseButtonUpEvent new
        button: event button;
        position: event x @ event y;
        yourself)
].

MorphicWindow ![
onMouseMotion: event
    rootMorph processEvent: (MorphMouseMotionEvent new
        buttons: event buttons;
        position: event x @ event y;
        deltaPosition: event xrel @ event yrel;
        yourself)
].

MorphicWindow ![
onKeyPressed: event
    rootMorph processEvent: (MorphKeyboardDownEvent new
        scancode: event scancode;
        symbol: event symbol;
        yourself)
].

MorphicWindow ![
onKeyReleased: event
    rootMorph processEvent: (MorphKeyboardUpEvent new
        scancode: event scancode;
        symbol: event symbol;
        yourself)
].

MorphicWindow ![
onSizeChanged
    rootMorph bounds: (0@0 corner: self width @ self height).
].