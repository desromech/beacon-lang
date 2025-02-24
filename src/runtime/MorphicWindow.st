Window subclass: #MorphicWindow instanceVariables: #(rootMorph).

MorphicWindow ![
render
    Stdio stdout nextPutAll: 'Morphic window render.'; lf
].

MorphicWindow ![
onExpose: event
    self render.
].