Morph subclass: #CubeMorph instanceVariables: #(cameraLocation cameraEulerAngles cameraOrientationMatrix cubeModel floorModel outputTextureHandle).

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
    floorModel := MeshBuilder new
        addCubeWithMinX: -5 minY: -1.5 minZ: -5 maxX: 5 maxY: -0.5 maxZ: 5;
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
    | extent floorObject leftObject middleObject rightObject spotOrientation |
    extent := self extent.
    rendererHandle begin3DFrameRenderingWithWidth: extent x height: extent y.

    cameraOrientationMatrix := (Matrix3x3 yRotation: cameraEulerAngles y) * (Matrix3x3 xRotation: cameraEulerAngles x).

    floorObject := RenderObject3D new
        model: floorModel;
        modelMatrix: Matrix4x4 identity;
        yourself.

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

    spotOrientation := Quaternion xRotation: Float halfPi negated.

    rendererHandle 
        addSceneCamera: (SceneCamera new
            location: cameraLocation;
            orientation: cameraOrientationMatrix;
            yourself);
        addLightSource: (LightSource new
            position: (Vector3 x: -2 y: 2 z: 0);
            color: Color green intensity: 20;
            influenceRadius: 10;
            castShadows: false;
            spotOrientation: spotOrientation;
            innerSpotCutoff: 60.0;
            outerSpotCutoff: 70.0;
            castShadows: true;
            yourself);
        addLightSource: (LightSource new
            position: (Vector3 x: 0 y: 2 z: 0);
            color: Color white intensity: 20;
            influenceRadius: 10;
            castShadows: true;
            yourself);
        addLightSource: (LightSource new
            position: (Vector3 x: 2 y: 2 z: 0);
            color: Color red intensity: 20;
            castShadows: false;
            influenceRadius: 10;
            spotOrientation: spotOrientation;
            innerSpotCutoff: 60.0;
            outerSpotCutoff: 70.0;
            castShadows: true;
            yourself);
        addRenderObject: floorObject;
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