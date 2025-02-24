Window subclass: #MorphicWindow instanceVariables: #(rootMorph).

MorphicWindow ![
rootMorph: aRootMorph
    rootMorph := aRootMorph
].

MorphicWindow ![
render
    Stdio stdout nextPutAll: 'Morphic window render.'; lf
].

MorphicWindow ![
onExpose: event
    self render.
].

Morph ![
openInWindow
    MorphicWindow new
        extent: bounds extent;
        rootMorph: self;
        open
].