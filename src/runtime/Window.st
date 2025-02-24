Window ![
initialize
    width := 640.
    height := 480.
    drawingForm := nil.
].

Window ![
width
    ^ width
].

Window ![
width: aNumber
    width := aNumber
].

Window ![
height
    ^ height
].

Window ![
height: aNumber
    height := aNumber
].

Window ![
extent
    ^ width @ height
].

Window ![
extent: aPoint
    width := aPoint x.
    height := aPoint y.
].

Window ![
handle
    ^ handle
].

Window ![
ensureDrawingFormHasCorrectExtent

    (drawingForm ~~ nil and: [
    drawingForm width == textureWidth and: [
    drawingForm height == textureHeight]]) ifTrue: [ ^ self ].

    Stdio stdout nextPutAll: 'ensureDrawingFormHasCorrectExtent'; lf.
    drawingForm := Form extent: textureWidth@textureHeight depth: 32.
].

Window ![
render
    self ensureDrawingFormHasCorrectExtent.

    self displayForm: drawingForm.
].

Window ![
onExpose: event
    self render.
].

Window ![
onMouseButtonDown: event
    Stdio stdout nextPutAll: 'Window onMouseButtonDown.'; lf
].

Window ![
onMouseButtonUp: event
    Stdio stdout nextPutAll: 'Window onMouseButtonUp.'; lf
].

Window ![
onMouseMotion: event
    Stdio stdout nextPutAll: 'Window onMouseMotion.'; lf
].

Window ![
onKeyPressed: event
    Stdio stdout nextPutAll: 'Window onKeyPressed.'; lf
].

Window ![
onKeyReleased: event
    Stdio stdout nextPutAll: 'Window onKeyReleased.'; lf
].
