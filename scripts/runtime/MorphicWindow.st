Window subclass: #MorphicWindow instanceVariables: #(rootMorph).

MorphicWindow ![
rootMorph: aRootMorph
    rootMorph := aRootMorph
].

MorphicWindow ![
renderWithBuilder: renderingBuilder
    rootMorph == nil ifTrue: [^ self].

    rootMorph position: 0@0.
    rootMorph extent: width @ height.
    rootMorph fullRenderWithBuilder: renderingBuilder
].

MorphicWindow ![
render3DEnvironmentWith: rendererHandle
    rootMorph == nil ifTrue: [^ self].

    rootMorph position: 0@0.
    rootMorph extent: width @ height.
    rootMorph fullRender3DEnvironmentWith: rendererHandle
].

MorphicWindow ![
childChanged: aChild
    self render
].

Morph ![
openInWindow
    | morphicWindow fillLayout |
    morphicWindow := MorphicWindow new.
    fillLayout := FillMorphLayout new morph: self; yourself.

    ^ morphicWindow
        extent: extent;
        rootMorph: (RootMorph new
            extent: extent;
            ownerWindow: morphicWindow;
            layout: fillLayout;
            addMorph: self;
            yourself);
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
onMouseWheel: event
    rootMorph processEvent: (MorphMouseWheelEvent new
        position: event x @ event y;
        scrollDelta: event scrollX @ event scrollY;
        yourself)
].

MorphicWindow ![
onKeyPressed: event
    rootMorph processEvent: (MorphKeyboardDownEvent new
        scancode: event scancode;
        symbol: event symbol;
        modstate: event modstate;
        yourself)
].

MorphicWindow ![
onKeyReleased: event
    rootMorph processEvent: (MorphKeyboardUpEvent new
        scancode: event scancode;
        symbol: event symbol;
        modstate: event modstate;
        yourself)
].

MorphicWindow ![
onTextInput: event
    rootMorph processEvent: (MorphTextInputEvent new
        text: event text;
        yourself)
].

MorphicWindow ![
onSizeChanged
    rootMorph extent: self width @ self height.
].