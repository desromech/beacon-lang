Morph subclass: #CubeMorph instanceVariables: #(cameraLocation cameraEulerAngles cameraOrientationMatrix outputTextureHandle).

CubeMorph class ![
defaultExtent
    ^ 1024@576
].

CubeMorph ![
initialize
    super initialize.
    cameraLocation := Vector3 x: 0 y: 1 z:3.
    cameraEulerAngles := Vector3 zeros.
    cameraOrientationMatrix := Matrix3x3 identity.
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

    cameraOrientationMatrix := (Matrix3x3 yRotation: cameraEulerAngles y) * (Matrix3x3 xRotation: cameraEulerAngles x).

    rendererHandle 
        addTestCameraWithLocation: cameraLocation orientation: cameraOrientationMatrix;
        addTestLight;
        addTestCubeWithLocation: (Vector3 x: -1 y: 0 z: 0);
        addTestCubeWithLocation: (Vector3 x: 1 y: 0 z: 0).

    rendererHandle end3DFrameRendering.
    outputTextureHandle := rendererHandle get3DOutputTextureHandle.
].

CubeMorph ![
handleMorphMouseMotionEvent: event
    event hasLeftButtonPressed ifTrue: [
        cameraEulerAngles := cameraEulerAngles + (Vector3 x: event deltaPosition y * -0.01 y: event deltaPosition x * -0.01 z: 0).
    ].

    event hasRightButtonPressed ifTrue: [
        cameraLocation := cameraLocation + (cameraOrientationMatrix * (Vector3 x: event deltaPosition x *0.01 y: event deltaPosition y * -0.01 z: 0)).
    ].

    event wasHandled: true.
    (event hasLeftButtonPressed or: [event hasRightButtonPressed]) ifTrue: [
        self changed
    ]
].

CubeMorph ![
handleMorphMouseWheelEvent: event
    Stdio stdout print: event scrollDelta; lf.

    cameraLocation := cameraLocation + (cameraOrientationMatrix * (Vector3 x: 0 y: 0 z: event scrollDelta y * -0.5)).
    self changed.
    event wasHandled: true
].