Window subclass: #MorphicWindow instanceVariables: #().

MorphicWindow ![
render
    Stdio stdout nextPutAll: 'Morphic window render.'; lf
].

MorphicWindow ![
onExpose: event
    self render.
].