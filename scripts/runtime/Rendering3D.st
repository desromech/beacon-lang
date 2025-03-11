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

Model3D ![
initialize
    super initialize.
    primitives := #().
].

Model3D ![
primitives
    ^ primitives
].

Model3D ![
primitives: anArrayOfPrimitives
    primitives := anArrayOfPrimitives
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

LightSource ![
initialize
    super initialize.
	
    positionOrDirection := Vector4 zeros.

	intensity := Vector3 ones.
	influenceRadius := 20.

	spotDirection := Vector3 x: 0 y: 0 z: -1.
	innerSpotCosCutoff := -1.
	outerSpotCosCutoff := -1.
	
    castShadows := false.
	shadowMapViewportScale := Vector2 zeros.
	
	shadowMapNormalBiasFactor := 0.005.

	shadowMapCascadeDistanceWorldTransform := Vector4 zeros.
	shadowMapCascadeOffsets := Vector4 zeros.
	
	modelMatrices = #().
	inverseModelMatrices = #().

	projectionMatrices := #().
	inverseProjectionMatrices := #().

	shadowMapViewportOffsets := #().
].

LightSource ![
positionOrDirection
    ^ positionOrDirection
].

LightSource ![
positionOrDirection: aVector4
    positionOrDirection := aVector4
].

LightSource ![
position: aVector3
    positionOrDirection := Vector4 xyz: aVector3 w: 1.0
].

LightSource ![
direction: aVector3
    positionOrDirection := Vector4 xyz: aVector3 w: 0.0
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