Morph subclass: #CubeMorph instanceVariables: #(outputTextureHandle).

CubeMorph class ![
defaultExtent
    ^ 1024@576
].

CubeMorph ![
renderWithBuilder: renderingBuilder
    outputTextureHandle == nil ifTrue: [ ^ self].

    renderingBuilder texture: outputTextureHandle rectangle: self localBounds.
].

CubeMorph ![
render3DEnvironmentWith: rendererHandle
    | extent |
    extent := self extent.
    rendererHandle begin3DFrameRenderingWithWidth: extent x height: extent y.

    rendererHandle addTestCube.

    rendererHandle end3DFrameRendering.
    outputTextureHandle := rendererHandle get3DOutputTextureHandle.
].