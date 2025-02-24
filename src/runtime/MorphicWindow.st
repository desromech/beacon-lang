Window subclass: #MorphicWindow instanceVariables: #(rootMorph).

MorphicWindow ![
rootMorph: aRootMorph
    rootMorph := aRootMorph
].

MorphicWindow ![
renderWithBuilder: renderingBuilder
    rootMorph == nil ifTrue: [^ self].

    rootMorph bounds: (0@0 corner: self width @ self height).
    rootMorph renderWithBuilder: renderingBuilder
].

Morph ![
openInWindow
    MorphicWindow new
        extent: bounds extent;
        rootMorph: self;
        open
].

MorphicWindow ![
onMouseButtonDown: event
    Stdio stdout nextPutAll: 'MorphicWindow onMouseButtonDown.'; lf
].

MorphicWindow ![
onMouseButtonUp: event
    Stdio stdout nextPutAll: 'MorphicWindow onMouseButtonUp.'; lf
].

MorphicWindow ![
onMouseMotion: event
    Stdio stdout nextPutAll: 'MorphicWindow onMouseMotion.'; lf
].

MorphicWindow ![
onKeyPressed: event
    Stdio stdout nextPutAll: 'MorphicWindow onKeyPressed.'; lf
].

MorphicWindow ![
onKeyReleased: event
    Stdio stdout nextPutAll: 'MorphicWindow onKeyReleased.'; lf
].

MorphicWindow ![
onSizeChanged
    rootMorph bounds: (0@0 corner: self width @ self height).
].