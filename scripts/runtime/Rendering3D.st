RenderingUploadedElement ![
initialize
    super initialize.
    lastUploadedFrame := -1.
    uploadedIndex := -1.
].

RenderingUploadedElement ![
lastUploadedFrame
    ^ lastUploadedFrame
].

RenderingUploadedElement ![
uploadedIndex
    ^ uploadedIndex
].

MeshMetallicRoughnessMaterial ![
initialize
    super initialize.
    baseColor := Color white.
    metallicFactor := 1.0.
    roughnessFactor := 1.0.
    occlusionFactor := 1.0.
    isUnlit := false
].

MeshMetallicRoughnessMaterial ![
baseColor
    ^ baseColor
].

MeshMetallicRoughnessMaterial ![
baseColor: aColor
    baseColor := aColor
].

MeshMetallicRoughnessMaterial ![
baseColorTexture
    ^ baseColorTexture
].

MeshMetallicRoughnessMaterial ![
baseColorTexture: aTexture
    baseColorTexture := aTexture
].

MeshMetallicRoughnessMaterial ![
metallicFactor
    ^ metallicFactor
].

MeshMetallicRoughnessMaterial ![
metallicFactor: aNumber
    metallicFactor := aNumber
].

MeshMetallicRoughnessMaterial ![
roughnessFactor
    ^ roughnessFactor
].

MeshMetallicRoughnessMaterial ![
roughnessFactor: aNumber
    roughnessFactor := aNumber
].

MeshMetallicRoughnessMaterial ![
metallicRoughnessTexture
    ^ metallicRoughnessTexture
].

MeshMetallicRoughnessMaterial ![
metallicRoughnessTexture: aTexture
    metallicRoughnessTexture := aTexture
].


MeshMetallicRoughnessMaterial ![
occlusionFactor
    ^ occlusionFactor
].

MeshMetallicRoughnessMaterial ![
occlusionFactor: aNumber
    occlusionFactor := aNumber
].

MeshMetallicRoughnessMaterial ![
occlusionTexture
    ^ occlusionTexture
].

MeshMetallicRoughnessMaterial ![
occlusionTexture: aTexture
    occlusionTexture := aTexture
].

MeshPrimitive ![
initialize
    super initialize.
    positions := #().
    normals := #().
    texcoords := #().
    tangents4 := #().
    boundingBox := AABox empty.
].


MeshPrimitive ![
material
    ^ material
].

MeshPrimitive ![
material: aMaterial
    material := aMaterial
].

MeshPrimitive ![
positions
    ^ positions
].

MeshPrimitive ![
positions: anArrayOfVector3
    positions := anArrayOfVector3
].

MeshPrimitive ![
normals
    ^ normals
].

MeshPrimitive ![
normals: anArrayOfVector3
    normals := anArrayOfVector3
].

MeshPrimitive ![
texcoords
    ^ texcoords
].

MeshPrimitive ![
texcoords: anArrayOfVector2
    texcoords := anArrayOfVector2
].

MeshPrimitive ![
tangents4
    ^ tangents4
].

MeshPrimitive ![
tangents4: anArrayOfVector4
    tangents4 := anArrayOfVector4
].

MeshPrimitive ![
indices
    ^ indices
].

MeshPrimitive ![
indices: anArrayOfInteger
    indices := anArrayOfInteger
].

MeshPrimitive ![
boundingBox
    ^ boundingBox
].

MeshPrimitive ![
boundingBox: aBoundingBox
    boundingBox := aBoundingBox
].

Model3D ![
initialize
    super initialize.
    primitives := #().
    boundingBox := AABox empty.
].

Model3D ![
primitives
    ^ primitives
].

Model3D ![
primitives: anArrayOfPrimitives
    primitives := anArrayOfPrimitives
].

Model3D ![
boundingBox
    ^ boundingBox
].

Model3D ![
boundingBox: aBoundingBox
    boundingBox := aBoundingBox
].

RenderObject3D ![
initialize
    super initialize.
    isSelected := false.
    selectionColor := Color yellow.
].

RenderObject3D ![
model
    ^ model
].

RenderObject3D ![
model: aModel
    model := aModel
].

RenderObject3D ![
modelMatrix
    ^ modelMatrix
].

RenderObject3D ![
modelMatrix: aMatrix
    modelMatrix := aMatrix
].

RenderObject3D ![
isSelected
    ^ isSelected
].

RenderObject3D ![
isSelected: aBoolean
    isSelected := aBoolean
].

RenderObject3D ![
selectionColor
    ^ selectionColor
].

RenderObject3D ![
selectionColor: aColor
    selectionColor := aColor
].

LightSource ![
initialize
    super initialize.
	
    position := Vector3 zeros.
    isDirectional := false.

	intensity := Vector3 ones.
	influenceRadius := 20.

	orientation := Quaternion identity.

	innerSpotCutoff := 180.0.
	outerSpotCutoff := 180.0.
	
    castShadows := false.
	shadowMapNormalBiasFactor := 0.005.
].

LightSource ![
position
    ^ position
].

LightSource ![
position: aVector3
    position := aVector3
].

LightSource ![
isDirectional
    ^ isDirectional
].

LightSource ![
isDirectional: aBoolean
    isDirectional := aBoolean
].

LightSource ![
color: aColor intensity: anIntensity
    intensity := Vector3 x: aColor r * anIntensity y: aColor g * anIntensity z: aColor b * anIntensity
].

LightSource ![
influenceRadius
    ^ influenceRadius
].

LightSource ![
influenceRadius: aRadius
    influenceRadius := aRadius
].

LightSource ![
castShadows
    ^ castShadows
].

LightSource ![
castShadows: aBoolean
    castShadows := aBoolean
].

LightSource ![
orientation
    ^ orientation
].

LightSource ![
orientation: anOrientation
    orientation := anOrientation
].

LightSource ![
innerSpotCutoff
    ^ innerSpotCutoff
].

LightSource ![
innerSpotCutoff: aCutoffAngle
    innerSpotCutoff := aCutoffAngle
].

LightSource ![
outerSpotCutoff
    ^ outerSpotCutoff
].

LightSource ![
outerSpotCutoff: aCutoffAngle
    outerSpotCutoff := aCutoffAngle
].

SceneCamera ![
initialize
    super initialize

    location := Vector3 zeros.
    orientation := Matrix3x3 identity.
    nearDistance := 0.01.
    farDistance := 1000.0.
    fovY := 60.0.
    focalDistance := 10.0.
    isPerspective := true.
    
    timeOfSimulation := 0.0.
    timeOfDay := 0.0.
    exposure := 1.0.

    ambientLightSource := Vector3 x: 0.2 y: 0.2 z: 0.2.
].

SceneCamera ![
location
    ^ location
].

SceneCamera ![
location: aVector3
    location := aVector3
].

SceneCamera ![
orientation
    ^ orientation
].

SceneCamera ![
orientation: aMatrix3x3
    orientation := aMatrix3x3
].

SceneCamera ![
nearDistance
    ^ nearDistance
].

SceneCamera ![
nearDistance: aDistance
    nearDistance := aDistance
].

SceneCamera ![
farDistance
    ^ farDistance
].

SceneCamera ![
farDistance: aDistance
    farDistance := aDistance
].

SceneCamera ![
fovY: aFieldOfView
    fovY := aFieldOfView
].
