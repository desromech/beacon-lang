Morph subclass: #CubeMorph instanceVariables: #(outputTexture).

CubeMorph class ![
defaultExtent
    ^ 1024@576
].

CubeMorph ![
renderWithBuilder: renderingBuilder
].

CubeMorph ![
render3DEnvironmentWith: rendererHandle
    | extent |
    extent := self extent.
    rendererHandle begin3DFrameRenderingWithWidth: extent x height: extent y.

    rendererHandle end3DFrameRendering.
].