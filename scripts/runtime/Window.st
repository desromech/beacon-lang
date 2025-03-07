Window ![
initialize
    width := 640.
    height := 480.
    drawingForm := nil.
    useAcceleratedRendering := true.
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

    drawingForm := Form extent: textureWidth@textureHeight depth: 32.
].

Window ![
renderWithBuilder: renderingBuilder
].

Window ![
render3DEnvironmentWith: rendererHandle

].

Window ![
useAcceleratedRendering
    ^ useAcceleratedRendering
].

Window ![
useAcceleratedRendering: aBoolean
    useAcceleratedRendering := aBoolean
].

Window ![
renderViaForm
    | formRenderingBuilder | 
    self ensureDrawingFormHasCorrectExtent.
    
    formRenderingBuilder := FormRenderingElementBuilder new
        scaleFactorX: textureWidth // width;
        scaleFactorY: textureHeight // height;
        yourself.
    self renderWithBuilder: formRenderingBuilder.
    formRenderingBuilder drawInForm: drawingForm.

    self displayForm: drawingForm.
].

Window ![
renderViaQuadCommands
    | formRenderingBuilder |
    rendererHandle beginFrame.
    self render3DEnvironmentWith: rendererHandle.

    formRenderingBuilder := FormRenderingElementBuilder new
        scaleFactorX: textureWidth // width;
        scaleFactorY: textureHeight // height;
        yourself.
    self renderWithBuilder: formRenderingBuilder.

    rendererHandle
        renderQuadList: formRenderingBuilder allQuads;
        endFrame
].

Window ![
render
    useAcceleratedRendering ifFalse: [
        self renderViaForm
    ] ifTrue: [
        self renderViaQuadCommands
    ]
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
onMouseWheel: event
    Stdio stdout nextPutAll: 'Window onMouseWheel.'; lf
].

Window ![
onKeyPressed: event
    Stdio stdout nextPutAll: 'Window onKeyPressed.'; lf
].

Window ![
onKeyReleased: event
    Stdio stdout nextPutAll: 'Window onKeyReleased.'; lf
].

Window ![
onTextInput: event
    Stdio stdout nextPutAll: 'Window onTextInput: '; nextPutAll: event text; lf
].

Window ![
onSizeChanged
    Stdio stdout nextPutAll: 'Window onSizeChanged.'; lf
].

Window ![
onCloseRequest
    self close
].
