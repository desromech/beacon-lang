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
onExpose: event
    Stdio stdout nextPutAll: 'Window expose.'; lf
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
onKeyPressed: event
    Stdio stdout nextPutAll: 'Window onKeyPressed.'; lf
].

Window ![
onKeyReleased: event
    Stdio stdout nextPutAll: 'Window onKeyReleased.'; lf
].
