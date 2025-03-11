Morph subclass: #CubeMorph instanceVariables: #(cameraLocation cameraEulerAngles cameraOrientationMatrix cubeModel outputTextureHandle).

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
    cubeModel := MeshBuilder new
        addCubeWithMinX: -0.5 minY: -0.5 minZ: -0.5 maxX: 0.5 maxY: 0.5 maxZ: 0.5;
        finishPrimitive;
        finishModel.
].

CubeMorph ![
renderWithBuilder: renderingBuilder
    outputTextureHandle == nil ifTrue: [ ^ self].

    renderingBuilder texture: outputTextureHandle rectangle: self localBounds.
].

CubeMorph ![
render3DEnvironmentWith: rendererHandle
    | extent leftObject middleObject rightObject |
    extent := self extent.
    rendererHandle begin3DFrameRenderingWithWidth: extent x height: extent y.

    cameraOrientationMatrix := (Matrix3x3 yRotation: cameraEulerAngles y) * (Matrix3x3 xRotation: cameraEulerAngles x).

    leftObject := RenderObject3D new
        model: cubeModel;
        modelMatrix: (Matrix4x4 translation: (Vector3 x: -2 y: 0 z: 0));
        yourself.

    middleObject := RenderObject3D new
        model: cubeModel;
        modelMatrix: (Matrix4x4 identity);
        yourself.

    rightObject := RenderObject3D new
        model: cubeModel;
        modelMatrix: (Matrix4x4 translation: (Vector3 x: 2 y: 0 z: 0));
        yourself.

    rendererHandle 
        addTestCameraWithLocation: cameraLocation orientation: cameraOrientationMatrix;
        "addTestLightWithLocation: (Vector3 x: 0 y: 1 z: 0) color: Color white intensity: 20 influenceRadius: 10;"
        addLightSource: (LightSource new
            position: (Vector3 x: 0 y: 1 z: 0);
            color: Color white intensity: 20;
            influenceRadius: 10;
            yourself);
        addRenderObject: leftObject;
        addRenderObject: middleObject;
        addRenderObject: rightObject.

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
    cameraLocation := cameraLocation + (cameraOrientationMatrix * (Vector3 x: 0 y: 0 z: event scrollDelta y * -0.5)).
    self changed.
    event wasHandled: true
].