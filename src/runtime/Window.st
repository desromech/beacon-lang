Window ![
initialize
    width := 640.
    height := 480.
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
handle
    ^ handle
].

Window ![
render
    Stdio stdout nextPutAll: 'Window render.'; lf
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
