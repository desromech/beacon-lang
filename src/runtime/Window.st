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
    Stdio stdout nextPutAll: 'Window on expose.'; nextPut: 10
].

Window ![
onMouseButtonDown: event
    Stdio stdout nextPutAll: 'Window on onMouseButtonDown.'; nextPut: 10
].

Window ![
onMouseButtonUp: event
    Stdio stdout nextPutAll: 'Window on onMouseButtonUp.'; nextPut: 10
].